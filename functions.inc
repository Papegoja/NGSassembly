#!/bin/bash

# Functions file
# --------------
# build basespace binary from genome refseq
# build colorspace binary from genome refseq
# create jump database from colorspace binary
# align genome in colorspace 
# sorting of reads according to coordinates, resolve mate-pairs
# convert alignment file to BAM 
# mark duplicates with Picard MarkDuplicates
# add read-groups and merge all alignments, pipe to FreeBayes calling SNPs


################# Parallel threads ####################
function Build {					## Create binary file in basespace from genome FASTA file
		# -fr,-fr2 bases for 1st mate and 2nd mate
		# -fq,-fq2 base qualities for 1st mate and 2nd mate
		# -assignQual assign a quality value to all bases, bad setting beacuse it discards qualities completely
		# -oa outfile for reference sequence; -out outfile when converting reads
		# -st specifies sequencing technology; -ds description; -ln library name
		# -mfl specify median fragment length for mate-pairs
		
		# -ga genome assembly ID, eg. galGal3
		# -sn species name, eg. "gallus gallus domesticus"
		# -sam sample name, HS_MP_2x50bp 	MW_MP_2x50bp 	HS_SE_35bp_1 	HS_SE35bp_2 	MW_SE_35bp_1 	MW_SE_35bp_2
		
		
		# Archive gallus_high_2x50bp_matepairs_csfasta.qual.tar.gz contains: 
		# ugc_208_209_MP_2x50bp_solid0105_20091127_ugc_208_F3.csfasta
		# ugc_208_209_MP_2x50bp_solid0105_20091127_ugc_208_F3_QV.qual
		# ugc_208_209_MP_2x50bp_solid0105_20091127_ugc_208_R3.csfasta
		# ugc_208_209_MP_2x50bp_solid0105_20091127_ugc_208_R3_QV.qual
		# Library: HIGH_LINE	LOW_LINE
		# Group: HS_MP_2x50bp 	MW_MP_2x50bp 	HS_SE_35bp_1 	HS_SE35bp_2 	MW_SE_35bp_1 	MW_SE_35bp_2

		# ~/mosaik-aligner/bin/MosaikBuild -fr ugc_208_209_MP_2x50bp_solid0105_20091127_ugc_208_F3.csfasta.gz -fq ugc_208_209_MP_2x50bp_solid0105_20091127_ugc_208_F3_QV.qual.gz -fr2 ugc_208_209_MP_2x50bp_solid0105_20091127_ugc_208_R3.csfasta.gz -fq2 ugc_208_209_MP_2x50bp_solid0105_20091127_ugc_208_R3_QV.qual.gz -st solid -ln HIGH_LINE -id HS_MP_2x50bp -ds "gallus high HS 2x50bp mate-pairs" -sn "gallus gallus domesticus" -mfl 400 -out gallus_HS_2x50bp.dat | tee -a build.gallus_HS_2x50bp.log
		#  -ln HIGH_LINE -id HS_MP_2x50bp -ds "gallus high HS 2x50bp mate-pairs" -sn "gallus gallus domesticus"

		# using old version MosaikBuild 1.0.1388
		# ~kiero/gallus/bin/MosaikBuild -fr ugc_208_209_MP_2x50bp_solid0105_20091127_ugc_208_F3.csfasta -fq ugc_208_209_MP_2x50bp_solid0105_20091127_ugc_208_F3_QV.qual -fr2 ugc_208_209_MP_2x50bp_solid0105_20091127_ugc_208_R3.csfasta -fq2 ugc_208_209_MP_2x50bp_solid0105_20091127_ugc_208_R3_QV.qual -st solid -ln HIGH_LINE -id HS_MP_2x50bp -ds "gallus high HS 2x50bp mate-pairs" -sn "gallus gallus domesticus" -mfl 400 -out Mosaik1.0_gallus_HS_2x50bp.dat | tee -a build.Mosaik1.0_gallus_HS_2x50bp.log

		# Quality filter and mate-pair pairing
		# mean Phred quality score 20 per read
		# perl ~/SOLiD_preprocess_meanFilter_v2.pl -i mp -f ugc_208_209_MP_2x50bp_solid0105_20091127_ugc_208_F3.csfasta -g ugc_208_209_MP_2x50bp_solid0105_20091127_ugc_208_F3_QV.qual -r ugc_208_209_MP_2x50bp_solid0105_20091127_ugc_208_R3.csfasta -s ugc_208_209_MP_2x50bp_solid0105_20091127_ugc_208_R3_QV.qual -p on -q 20 -a on -v on -o gallus_HS_MP_2x50bp | tee -a preprocess_meanFilter.log
		
		# after quality filtering >20 and mate-pair pairing, 163.00 reads passed out of 338,21 reads total
		# ~/mosaik-aligner/bin/MosaikBuild -fr gallus_HS_MP_2x50bp_T_mp_F3.csfasta -fq gallus_HS_MP_2x50bp_T_mp_F3_QV.qual -fr2 gallus_HS_MP_2x50bp_T_mp_R3.csfasta -fq2 gallus_HS_MP_2x50bp_T_mp_R3_QV.qual -st solid -ln HIGH_LINE -id HS_MP_2x50bp -ds "gallus high HS 2x50bp mate-pairs" -sn "gallus gallus domesticus" -mfl 400 -out gallus_HS_2x50bp.dat | tee -a build.gallus_HS_2x50bp.log

	echo -e "\tCreating binary file in basespace for $genome..." | tee -a $results_dir/pipeline.log
	$mosaik_dir/MosaikBuild -fr $refseq_dir/$file -oa $results_dir/$file".dat" | tee -a $results_dir/build.log
	$mosaik_dir/samtools faidx $refseq_dir/$file 2>/dev/null
	$mosaik_dir/fixlog $results_dir/build.log
}


function BuildCS {    				## Create binary file in colorspace from genome FASTA file
	echo -e "\tCreating binary file in colorspace for $genome..." | tee -a $results_dir/pipeline.log
	$mosaik_dir/MosaikBuild -fr $refseq_dir/$file -oa $results_dir/$file".cs.dat" -cs | tee -a $results_dir/buildCS.log
	$mosaik_dir/fixlog $results_dir/buildCS.log
}


function JumpDB {					## Create jump database for genome 
		# -kd, keeps keys database on disk
		# -mem, amount of RAM used when sorting, hard-coded to 70 GB
		# -mhp 100, maximum hash positions, default 100
		
	st=$SECONDS
	echo -e "\tCreating jumps database for $genome..." | tee -a $results_dir/pipeline.log
	$mosaik_dir/MosaikJump -ia $results_dir/$file".cs.dat" -hs $hash_size -mhp $mhp -mem 36 -out $results_dir/$file".jmp" | tee -a $results_dir/mosaik.log
	$mosaik_dir/fixlog $results_dir/mosaik.log
	en=$SECONDS
	exectime=$((en - st))
	echo -e "\tJumps database done in $exectime seconds." | tee -a $results_dir/pipeline.log
}


function Align {					## Align
		# -in SOLiD reads dat file; -ia refseq in colorspace; -ibs -ibs refseq in basespace
		# -ls search radius for mate-pairs
		# -bw banded Smith-Waterman alignment algorithm 
		# -mhp maximum hash positions 
		# -act alignment candidate threshold
		# -mm mismatches
		# -mmp threshold of ALIGNMENT QUALITY, AlignmentQualityThreshold, number between 0 & 99, =10 for conservative filtering
		# -j jumpdatabase, $results_dir/$chromosome/$chromosome".jmp
 		# 
	

		# single end, practical example
		# ~/mosaik-aligner/bin/MosaikAligner -in /home/gallus/gallus_high_35_1.dat -ia chr2.cs.dat -ibs chr2.dat -a all -m all -hs 15 -p 16 -bw 13 -act 20 -mm 4 -mhp 100 -j chr2.cs.jmp -out results.gal_high_35bp_1.16cores/chr2.aligned.dat | tee -a results.gal_high_35bp_1.16cores/aligner.log
		
		# mate-pairs, practical example
		# ~/mosaik-aligner/bin/MosaikAligner -in /home/gallus/gallus_high_50.dat -ia chr2.cs.dat -ibs chr2.dat -a all -m all -hs 15 -p 16 -bw 17 -act 25 -mm 6 -ls 100 -mhp 100 -j chr2.cs.jmp -out results.gal_high_50bp.16cores/chr2.aligned.dat | tee -a results.gal_high_50bp.16cores/aligner.log

		# Alignment after quality filtering >20 and mate-pair pairing (163.00 Mb passed out of 338,21 Mb)
		# ERROR: This sequencing technology is not currently supported for local alignment search.
		# ~/mosaik-aligner/bin/MosaikAligner -in /home/gallus/gallus_HS_2x50bp.dat -ia chr2.cs.dat -ibs chr2.dat -a all -m all -hs 15 -p 16 -bw 17 -act 25 -mm 6 -ls 100 -mhp 100 -j chr2.cs.jmp -out results.gallus_HS_2x50bp/chr2.aligned.dat | tee -a results.gallus_HS_2x50bp/aligner.log
	
		# Alignment of Mosaik1.0_gallus_HS_2x50bp built using MosaikBuild 1.0.1388 from F3.csfasta + F3.qual and R3.csfasta + R3.qual 
		# ERROR: This sequencing technology is not currently supported for local alignment search.
		# Working after omitting flag -ls 100 (radius search of median fragment length), local alignment search of SOLiD mate-pairs in colorspace not supported ?? 
		# ~/mosaik-aligner/bin/MosaikAligner -in /home/gallus/Mosaik1.0_gallus_HS_2x50bp.dat -ia chr2.cs.dat -ibs chr2.dat -a all -m all -hs 15 -p 16 -act 25 -mm 6 -bw 17 -ls 100 -mhp 100 -j chr2.cs.jmp -out results.Mosaik1.0_gallus_HS_2x50bp/chr2.aligned.dat | tee -a results.Mosaik1.0_gallus_HS_2x50bp/aligner.log
		
		# Alignment of Mosaik1.0_gallus HS_2x50bp.mfl3528 built using MosaikBuild 1.0.1388 with flag -mfl 3528 (mean median fragment length) 
		# ~/mosaik-aligner/bin/MosaikAligner -in /home/gallus/Mosaik1.0_gallus_HS_2x50bp.mfl3528.dat -ia chr2.cs.dat -ibs chr2.dat -a all -m all -hs 15 -p 16 -act 25 -mm 6 -bw 17 -ls 600 -mhp 100 -j chr2.cs.jmp -out results.Mosaik1.0_gallus_HS_2x50bp.mfl3528/chr2.aligned.dat | tee -a results.Mosaik1.0_gallus_HS_2x50bp.mfl3528/aligner.log
		
	st=$SECONDS
	echo -e "\tAligning genome $genome..." | tee -a $results_dir/pipeline.log
	
	case $genome in
		*"35bp"*) 
			mismatches=$SE_mismatches
			bandwidth=$SE_bandwidth
			act=$SE_act
			;;
		*"50bp"*) 
			mismatches=$MP_mismatches
			bandwidth=$MP_bandwidth
			act=$MP_act
			;;
	esac

	($mosaik_dir/MosaikAligner -in $reads_dir/$genome -ia $results_dir/$file".cs.dat" -ibs $results_dir/$file".dat" -hs $hash_size -mm $mismatches -bw $bandwidth -act $act -mhp $mhp -m all -a all -p $proc -out $results_dir/$genome/$genome".aligned.dat" -j $results_dir/$file".jmp" 2>&1 | tee -a $results_dir/mosaik.log ; exit ${PIPESTATUS[0]} )
	#((some_command 2>&1 1>&3 | tee /dev/stderr ; exit ${PIPESTATUS[0]}) 3>&1)
	sleep 6
	$mosaik_dir/fixlog $results_dir/mosaik.log
	en=$SECONDS
	exectime=$((en - st))
	echo -e "\tAlignment done in $exectime seconds." | tee -a $results_dir/pipeline.log
}


function Sort {					## Sorting, two different approaches for SE and MP
		# simply sorts SE
		# resolves MP, fragment length is calculated using confidence interval, min,max,mean of distribution is returned
		# -mem number of alingments in memory, sets sorting cache size, default 6000000
		# -nu include non-unique reads, ONLY for Single-End
		
		# -afl allow all fragment lengths for PE/MP reads
		# -sa sample all unique vs non-unique fragment lengths
		# -rmm resolve multiple vs multiple read pairs
		
	st=$SECONDS
	echo -e "\tSorting aligned reads for $genome..." | tee -a $results_dir/pipeline.log
	$mosaik_dir/MosaikSort -in $results_dir/$genome/$genome".aligned.dat" -mem 108000000 -nu -afl -sa -rmm -out $results_dir/$genome/$genome".aligned.sorted.dat" | tee -a $results_dir/$genome/MosaikSort.Text.log
	sleep 2
	rm -rf $results_dir/$genome/$genome".aligned.dat" 2>/dev/null
	rm -rf /tmp/*.tmp 2>/dev/null
	$mosaik_dir/fixlog $results_dir/$genome/MosaikSort.Text.log
	en=$SECONDS
	exectime=$((en - st))
	echo -e "\tSorting done in $exectime seconds." | tee -a $results_dir/pipeline.log
}


function Text {					## Convert assembly archive to BAM file format for analysis in FreeBayes
	st=$SECONDS
	echo -e "\tConverting alignment file for $genome to BAM file..." | tee -a $results_dir/pipeline.log
	$mosaik_dir/MosaikText -in $results_dir/$genome/$genome".aligned.sorted.dat" -bam $results_dir/$genome/$genome".aligned.sorted.bam" | tee -a $results_dir/$genome/MosaikSort.Text.log
	sleep 2
	rm -rf $results_dir/$genome/$genome".aligned.sorted.dat" 2>/dev/null
	$mosaik_dir/fixlog $results_dir/$genome/MosaikSort.Text.log
	en=$SECONDS
	exectime=$((en -st))
	echo -e "\tConversion done in $exectime seconds." | tee -a $results_dir/pipeline.log
}


function MarkDuplicates {			## Remove duplicate PCR products to improve SNP detection, non-detachable
	st=$SECONDS
	echo -e "\tRemoving duplicate reads (dedup) for $genome..." | tee -a $results_dir/pipeline.log
		# -Xms, initial (start) java heap size
		# -Xmx, maximum java heap size 
		# MAX_RECORDS_IN_RAM=25M reads, for ~100bp reads set 250k reads per 1GB RAM, 0.5 * 50 * 250k = 25M
		# 
	
	# CONSIDER REMOVAL OF DUPLICATE READS FOR SMALLER FILE SIZES AND FASTER DOWNSTREAM EXECUTION	
		
	java -d64 -Xmx56G -Xms50G -jar $mosaik_dir/MarkDuplicates.jar MAX_RECORDS_IN_RAM=25000000 INPUT=$results_dir/$genome/$genome".aligned.sorted.bam" OUTPUT=$results_dir/$genome/$genome".aligned.sorted.dedup.bam" METRICS_FILE=$results_dir/$genome/$genome".dedup.metrics.txt" REMOVE_DUPLICATES=true ASSUME_SORTED=true |& tee -a $results_dir/$genome/Picard.MarkDuplicates.Removed.log
	sleep 2
	rm -rf $results_dir/$genome/$genome".aligned.sorted.bam" 2>/dev/null
	en=$SECONDS
	exectime=$((en - st))
	echo -e "\tRemoved duplicates in $exectime seconds." | tee -a $results_dir/pipeline.log
}


#function GATKBAQ {
#	GATKLocalRealignment
#	BAQAdjustment
#}
#
#
#function GATKLocalRealignment {			## Local realignment around INDELs using GATK, parallel threads
#	st=$SECONDS
#	echo -e "\tLocal realignment using GATK for $genome..." | tee -a $results_dir/pipeline.log
#
#	# 1h30 + 45m
#	# detachable processes ? 
#	# RealignerTargetCreator -> IndelRealigner	
#	$mosaik_dir/bamtools index -in $results_dir/$genome/$results_dir/$genome/$genome".aligned.sorted.dedup.bam" 
#	java -Xmx7G -jar $mosaik_dir/GenomeAnalysisTK.jar -T RealignerTargetCreator --maxReadsInMemory 300000 -I $results_dir/$genome/$results_dir/$genome/$genome".aligned.sorted.dedup.bam" -R $refseq_dir/$file -o GATK.output.intervals |& tee -a $results_dir/$genome/GATK.realignment.log
#	java -Xmx7G -jar $mosaik_dir/GenomeAnalysisTK.jar -T IndelRealigner --maxReadsInMemory 300000 -I $results_dir/$genome/$results_dir/$genome/$genome".aligned.sorted.dedup.bam" -R $refseq_dir/$file -o $results_dir/$genome/$results_dir/$genome/$genome".aligned.sorted.dedup.GATKrealigned.bam" |& tee -a $results_dir/$genome/GATK.realignment.log
#	sleep 5
#	rm -rf $results_dir/$genome/$genome".aligned.sorted.dedup.bam" 2>/dev/null 
#	en=$SECONDS
#	exectime=$((en - st))
#	echo -e "\tLocal realignment done in $exectime seconds." | tee -a $results_dir/pipeline.log
#}
#
#
#function BAQAdjustment {				## BAQ adjustment
#	st=$SECONDS	
#	echo -e "\tBAM left-align and BAQ calculation for $genome..." | tee -a $results_dir/pipeline.log
#		# samtools view for piping to...
#		# bamleftalign
#		# samtools fillmd 
#		# output new BAM file leftaligned.BAQ
#	
#	($mosaik_dir/samtools view -b $results_dir/$genome/$results_dir/$genome/$genome".aligned.sorted.dedup.GATKrealigned.bam" | $mosaik_dir/samtools fillmd -br - $refseq_dir/$file > $results_dir/$genome/$results_dir/$genome/$genome".aligned.sorted.dedup.GATKrealigned.BAQ.bam" 2>&1 |& tee -a $results_dir/$genome/samtools.BAQ.log ; exit ${PIPESTATUS[0]} )
#	sleep 5
#	rm -rf $results_dir/$genome/$results_dir/$genome/$genome".aligned.sorted.dedup.GATKrealigned.bam" 2>/dev/null
#	en=$SECONDS
#	exectime=$((en - st))
#	echo -e "\tBAM left-align and BAQ calculation done in $exectime seconds." | tee -a $results_dir/pipeline.log
#}

####################### SNP call pipe #####################
# all previous functions must have completed before this step

function CallSNP {				## Merge BAM alignments and call SNPs and short INDELs
		# Pipe entire SNP call command from stdout to stdin; bamaddrg -> freebayes
		# Bamtools merge piped to bamleftalign for homogenization of positional distribution of IN/DELs, computationally inexpensive
		# bamtools merge xxx | bamleftalign xxx | freebayes --stdin -f $refseq_dir/$chomosome/$file --ploidy diploid --indel -dd --pvar $poly_prob --min-coverage $CAL -v $results_dir/$chromosome/$chromosome".called.vcf" --log $results_dir/$chromosome/$genome$/freebayes.merged.leftaligned.log
		# -f fasta refseq, $file is current chromosome basename in iteration 		
		# -v, output file
		# --ploidy
		# -d, debugging output
		# --pvar, reporting probaility limit
		# --no-filters / -0, do not use any input base and mapping quality filters
		# --min-coverage
		# --min-mapping-quality
		# --min-base-quality
		# --use-mapping-quality
		# --ignore-reference-allele
		# --use-duplicate-reads, DO NOT USE

#	SNPstart=$SECONDS
#	echo -e "\tSNP calling in $genome..." | tee -a $results_dir/pipeline.log

#	# Add read group names with bamaddrg
#	# bamaddrg settings: 
#		# -s sample name, set to chromosome
#		# -r read group, set to genome/reads filename (gallus_high_35_1, gallus_low_50, ...)
	
# 	bamleftalign only required for MP reads when INDEL calling
#	$mosaik_dir/bamleftalign -f $refseq_dir/$file/

	bam_files=`find $results_dir -name \*.dedup.bam -print 	#| awk -F "/" '{print $NF}'`	# lists all bam-files in current line
	infiles=""
	for merging_name in $bam_files; do
		RG_basename=`echo $merging_name | awk -F "/" '{print $NF}' | awk -F "." '{print $1}'`
		case $merging_name in 
			*"high"*)
				infiles="$infiles -b $merging_name -s HIGH -r $RG_basename"
				;;
			*"low"*)
				infiles="$infiles -b $merging_name -s LOW -r $RG_basename"
				;;
		esac
#		$mosaik_dir/bamtools index $results_dir/$merging_name
	done
	$mosaik_dir/bamaddrg $infiles | $mosaik_dir/freebayes --stdin -f $refseq_dir/$file --pooled --ploidy $ploidy -dd --pvar $poly_prob --no-filters --min-coverage $CAL --min-mapping-quality $MAQ --ignore-reference-allele --use-mapping-quality -v $results_dir/$file".called.vcf"

#	SNPend=$SECONDS
#	SNPtime=$((SNPend - SNPstart))
#	echo -e "\tSNP calling done in $exectime seconds." | tee -a $results_dir/pipeline.log
}


################# Multi-core functions ####################
function queue {
	QUEUE="$QUEUE $1"
	NUM=$(($NUM+1))
}


function regeneratequeue {
	OLDREQUEUE=$QUEUE
	QUEUE=""
	NUM=0
	for PID in $OLDREQUEUE
	do
		if [ -d /proc/$PID ] ; then				# If directory exists = PID running
			QUEUE="$QUEUE $PID"
			NUM=$(($NUM+1))
		fi
	done
}


function checkqueue {
	OLDCHQUEUE=$QUEUE
	for PID in $OLDCHQUEUE
	do
		if [ ! -d /proc/$PID ] ; then 			# If directory does not exist -> start new process
			regeneratequeue					# new queue if some job/PID has finished
			break							# exit if-loop
		fi
	done
}

