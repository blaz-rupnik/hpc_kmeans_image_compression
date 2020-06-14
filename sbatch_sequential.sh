#!/bin/bash

#SBATCH --ntasks=1
#SBATCH --reservation=fri
#SBATCH --constraint=AMD
#SBATCH --output=output/sequential_optimized_output.txt
#SBATCH --time=10:00:00

threads=(2 4 8 16 32 64 128 256 512)
inputs=("sea_400_200.png" "alps_1600_900.png" "lake_4000_2667.png" "ocean_7777_5016.png")

for img in ${inputs[@]}
do
    for t in ${threads[@]}
    do
        srun ./sequential_optimized test_images/$img $t 10
    done
done