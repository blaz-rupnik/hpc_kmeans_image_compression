#!/bin/bash

# Arg 1: input test image
# Arg 2: number of clusters
# Arg 3: number of iterations
# Arg 4 (optional): version of parallel opencl implementation (defaults to latest)
# Run without sbatch: srun -n1 --reservation=fri --constraint=gpu ./parallel_opencl test_images/lake_4000_2667.png 64 10

#SBATCH --ntasks=1
#SBATCH --reservation=fri
#SBATCH --constraint=gpu
#SBATCH --output=output/opencl_output.txt

versions=(1 2 3)
clusters=(2 4 8 16 32 48 64 92 128 192 256 384 512 768 1024)

for v in ${versions[@]}
do
    for c in ${clusters[@]}
    do
        srun ./parallel_opencl test_images/lake_4000_2667.png $c 10 $v
    done
done