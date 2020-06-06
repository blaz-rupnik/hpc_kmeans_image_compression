#!/bin/bash

# Arg 1: number of program repetitions
# Arg 2: input test image
# Arg 3: number of clusters
# Arg 4: number of iterations
# Example: sbatch sbatch_opencl.sh 1 test_images/alps_1600_900.png 64 10
# Run without sbatch: srun -n1 --reservation=fri --constraint=gpu ./parallel_opencl test_images/alps_1600_900.png 64 10

#SBATCH --ntasks=1
#SBATCH --reservation=fri
#SBATCH --constraint=gpu
#SBATCH --output=output/opencl_output.txt

for ((i=0;i<$1;i++))
do
    srun ./parallel_opencl $2 $3 $4
done