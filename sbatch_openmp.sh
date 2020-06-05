#!/bin/bash
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=64

#Multithreading
#export OMP_PLACES=cores
#export OMP_PROC_BIND=TRUE
#export OMP_NUM_THREADS=64

./parallel_openmp test_images/nature_1600_1200.png 2 5 >> ./output/openmp_output.txt