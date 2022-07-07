#!/bin/bash
proc=1
size=1
#Size in MB

num_algorithms=1
#determine encodings later
num_types=1

#sbatch --output=$proc.$size.out --export=proc=$proc,size=$size mpi.grace_job

for init_type in {0..2}
do
  for size in {26,28,30}
  do
    proc=1
    while [ $proc -le 64 ]
    do	output=2.$init_type.$proc.$size.csv
      #echo "Algorithm: $algorithm" 
      #echo "Initial sort status: $init_type" 
      #echo "Processors: $proc" 
      #echo "Size: $size" 
      
      sbatch --output=$output --export=proc=$proc,size=$size,algorithm=2,init_type=$init_type mpi.grace_job	
      proc=$((2 * $proc))
    done
  done
done

echo "Complete"
