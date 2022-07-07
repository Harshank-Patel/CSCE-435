#!/bin/bash
proc=1
size=1
#Size in MB

num_algorithms=1
#determine encodings later
num_types=1

#sbatch --output=$proc.$size.out --export=proc=$proc,size=$size mpi.grace_job

for algorithm in {0..1}
do
  for init_type in {0..2}
  do
    while [ $size -le 128 ]
    do
      proc=2

      while [ $proc -le 64 ]
      do
	output=$algorithm.$init_type.$proc.$size.out
        #echo "Algorithm: $algorithm" 
        #echo "Initial sort status: $init_type" 
        #echo "Processors: $proc" 
        #echo "Size: $size" 
        
        sbatch --output=$output --export=proc=$proc,size=$size,algorithm=$algorithm,init_type=$init_type mpi.grace_job
	

        proc=$((2 * $proc))
      done

      size=$(($size * 2))

    done

  done

done


echo "Complete"
