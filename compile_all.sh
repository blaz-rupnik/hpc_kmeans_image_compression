#!/bin/bash

g++ sequential.cpp -Wl,-rpath,./ -L./ -l:"libfreeimage.so.3" -o sequential
g++ parallel_opencl.cpp -O2 -I/usr/include/cuda -L/usr/lib64 -l:"libOpenCL.so.1" -Wl,-rpath,./ -L./ -l:"libfreeimage.so.3" -o parallel_opencl
