#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <CL/cl.h>
#include "FreeImage.h"

#define MAX_SOURCE_SIZE	(16384)

void printImage(unsigned char *image, int size){
    //helper function for debugging purposes
    for(int i = 0; i < (size); i = i + 4){
        int image_position = i / 4;
        printf("index: %d, B: %d, G: %d, R: %d, A: %d \n", image_position, image[i], image[i+1], image[i+2], image[i+3]);
    }
}

void printArrayWithStep4(int* arrayToPrint, int size){
    //helper function for debugging purposes
    for(int c = 0; c < (size); c = c + 4){
        printf("%d %d %d %d \n", arrayToPrint[c], arrayToPrint[c+1], arrayToPrint[c+2], arrayToPrint[c+3]);
    }
}

void initCentroids(int *centroids, int num_of_clusters, unsigned char* imageIn, int imageSize){
    int counter = 0;
    //we use interval to select starting centroids from image, spreaded equally across
    int interval = imageSize / num_of_clusters;
    for(int i = 0; i < (num_of_clusters * 4); i = i + 4){
        centroids[i] = imageIn[counter];
        centroids[i + 1] = imageIn[counter+1];
        centroids[i + 2] = imageIn[counter+2];
        centroids[i + 3] = imageIn[counter+3];
        counter = counter + (interval * 4);
    }
}

void applyNewColoursToImage(unsigned char* image, int* closest_centroid_indices,  int size, int num_of_clusters, int* centroids){
    //for each pixel in image assign it new centroid colour
    for(int i = 0; i < (size); i = i + 4){
        //find colour centroid for this pixel
        int closestCentroid = closest_centroid_indices[i/4];
        //apply centroid colour to this pixel
        image[i] = centroids[closestCentroid * 4];
        image[i+1] = centroids[closestCentroid * 4 + 1];
        image[i+2] = centroids[closestCentroid * 4 + 2];
        image[i+3] = centroids[closestCentroid * 4 + 3];
    }
}

int main(int argc, char *argv[]) {

    //Load image from file
    //1st argument is image name including format
	FIBITMAP *imageLoad = FreeImage_Load(FIF_PNG, argv[1], 0);
	//Convert it to a 32-bit image
    FIBITMAP *imageLoad32 = FreeImage_ConvertTo32Bits(imageLoad);
	
    //Get image dimensions
    int width = FreeImage_GetWidth(imageLoad32);
	int height = FreeImage_GetHeight(imageLoad32);
	int pitch = FreeImage_GetPitch(imageLoad32);
    int num_pixels = width * height;

	//Prepare room for a raw data copy of the image
    unsigned char *imageIn = (unsigned char *)malloc(height*pitch * sizeof(unsigned char));

    //Extract raw data from the image
	FreeImage_ConvertToRawBits(imageIn, imageLoad32, pitch, 32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE);

    //Free source image data free
	FreeImage_Unload(imageLoad32);
	FreeImage_Unload(imageLoad);

    //get number of clusters from 2nd argument and num of iterations from 3rd argument
    int num_of_clusters = atoi(argv[2]);
    int num_of_iterations = atoi(argv[3]);

    //centroid init array
    int *centroids = (int*)malloc(num_of_clusters * 4 * sizeof(int));
    initCentroids(centroids, num_of_clusters, imageIn, width * height);

    //init array for keeping indices of closest centroid
    int *closest_centroid_indices = (int*)malloc(width * height * sizeof(int));


    cl_int clStatus;
	int n_pixels = height * width;

    // Read kernel from file
    FILE *fp;
    char *source_str;
    size_t source_size;

    fp = fopen("kernels.cl", "r");
    if (!fp) {
		fprintf(stderr, "Open file error.\n");
        exit(1);
    }
    source_str = (char*)malloc(MAX_SOURCE_SIZE);
    source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	source_str[source_size] = '\0';
    fclose(fp);
 
    // Get platforms
    cl_uint num_platforms;
    clStatus = clGetPlatformIDs(0, NULL, &num_platforms);
    cl_platform_id *platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id)*num_platforms);
    clStatus = clGetPlatformIDs(num_platforms, platforms, NULL);

    if (num_platforms == 0) {
        fprintf(stderr, "No cl platforms. Is GPU available?\n");
        exit(1);
    }

    // Get platform devices
    cl_uint num_devices;
    clStatus = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 0, NULL, &num_devices);
    num_devices = 1; // limit to one device
    cl_device_id *devices = (cl_device_id *)malloc(sizeof(cl_device_id)*num_devices);
    clStatus = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, num_devices, devices, NULL);

    // Context
    cl_context context = clCreateContext(NULL, num_devices, devices, NULL, NULL, &clStatus);
 
    // Command queue
    cl_command_queue command_queue = clCreateCommandQueue(context, devices[0], CL_QUEUE_PROFILING_ENABLE, &clStatus);

    // Create and build a program
    cl_program program = clCreateProgramWithSource(context,	1, (const char **)&source_str, NULL, &clStatus);
    clStatus = clBuildProgram(program, 1, devices, NULL, NULL, NULL);

	// Build log for kernel debugging
	size_t build_log_len;
	char *build_log;
	clStatus = clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &build_log_len);
    if (build_log_len > 2) {
        build_log =(char *)malloc(sizeof(char)*(build_log_len+1));
        clStatus = clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, 
                                        build_log_len, build_log, NULL);
        printf("%s", build_log);
        free(build_log);
        return 1;
    }

	// Set global size (pixels kernel) to multiple of local size 
    size_t local_size = 256;
    size_t global_size_pixels = num_pixels;
    int mod = num_pixels % local_size;
    if (mod != 0)
        global_size_pixels = num_pixels + (local_size - mod);

	// Set global size (clusters kernel) to multiple of local size 
	size_t global_size_clusters = num_of_clusters;
    mod = num_of_clusters % local_size;
    if (mod != 0)
        global_size_clusters = num_of_clusters + (local_size - mod);
    

    // Allocate memory on device
	cl_mem centroids_d = clCreateBuffer(context, CL_MEM_READ_WRITE, num_of_clusters * 4 * sizeof(int), NULL, &clStatus);
	cl_mem closest_centroid_indices_d = clCreateBuffer(context, CL_MEM_READ_WRITE, num_pixels * sizeof(int), NULL, &clStatus);
    cl_mem image_in_d = clCreateBuffer(context, CL_MEM_READ_ONLY, num_pixels * 4 * sizeof(unsigned char), NULL, &clStatus);

    // Start measuring time
    struct timeval clock_start, clock_end;
    gettimeofday(&clock_start, NULL);
	
	// Transfer data to device
	clStatus = clEnqueueWriteBuffer(command_queue, centroids_d, CL_TRUE, 0, num_of_clusters * 4 * sizeof(int), centroids, 0, NULL, NULL);
	clStatus = clEnqueueWriteBuffer(command_queue, image_in_d, CL_TRUE, 0, num_pixels * 4 * sizeof(unsigned char), imageIn, 0, NULL, NULL);
  
    // Create kernels and set arguments
	cl_kernel kernel_find_closest_centroids = clCreateKernel(program, "find_closest_centroids", &clStatus);
    clStatus = clSetKernelArg(kernel_find_closest_centroids, 0, sizeof(int), (void *)&num_of_clusters);
    clStatus |= clSetKernelArg(kernel_find_closest_centroids, 1, sizeof(int), (void *)&num_pixels);
    clStatus |= clSetKernelArg(kernel_find_closest_centroids, 2, sizeof(cl_mem), (void *)&centroids_d);
    clStatus |= clSetKernelArg(kernel_find_closest_centroids, 3, sizeof(cl_mem), (void *)&closest_centroid_indices_d);
    clStatus |= clSetKernelArg(kernel_find_closest_centroids, 4, sizeof(cl_mem), (void *)&image_in_d);


    cl_kernel kernel_update_centroids = clCreateKernel(program, "update_centroids", &clStatus);
    clStatus = clSetKernelArg(kernel_update_centroids, 0, sizeof(int), (void *)&num_of_clusters);
    clStatus |= clSetKernelArg(kernel_update_centroids, 1, sizeof(int), (void *)&num_pixels);
    clStatus |= clSetKernelArg(kernel_update_centroids, 2, sizeof(cl_mem), (void *)&centroids_d);
    clStatus |= clSetKernelArg(kernel_update_centroids, 3, sizeof(cl_mem), (void *)&closest_centroid_indices_d);
    clStatus |= clSetKernelArg(kernel_update_centroids, 4, sizeof(cl_mem), (void *)&image_in_d);

    // Main loop
    for (int iteration = 0; iteration < (num_of_iterations); iteration++) {
        
        cl_event kernel1_event, kernel2_event;

        // Step 1: go through all points and find closest centroid
	    clStatus = clEnqueueNDRangeKernel(command_queue, kernel_find_closest_centroids, 1, NULL, &global_size_pixels, &local_size, 0, NULL, &kernel1_event);

        // Step 2: for each centroid compute average which will be new centroid
	    clStatus = clEnqueueNDRangeKernel(command_queue, kernel_update_centroids, 1, NULL, &global_size_clusters, &local_size, 0, NULL, &kernel2_event);
    }

	clStatus = clFlush(command_queue);
    clStatus = clFinish(command_queue);

    // Copy data back to host
	clStatus = clEnqueueReadBuffer(command_queue, centroids_d, CL_TRUE, 0, num_of_clusters * 4 * sizeof(int), centroids, 0, NULL, NULL);
	clStatus = clEnqueueReadBuffer(command_queue, closest_centroid_indices_d, CL_TRUE, 0, num_pixels * sizeof(int), closest_centroid_indices, 0, NULL, NULL);

    // Stop measuring time
    gettimeofday(&clock_end, NULL);
    float milisecs = ((((clock_end.tv_sec - clock_start.tv_sec) * 1000000) + clock_end.tv_usec) - (clock_start.tv_usec))/1000.0;
    printf("%.2f\n", milisecs);

    // release & free
    clStatus = clFlush(command_queue);
    clStatus = clFinish(command_queue);
    clStatus = clReleaseKernel(kernel_find_closest_centroids);
    clStatus = clReleaseKernel(kernel_update_centroids);
    clStatus = clReleaseProgram(program);
    clStatus = clReleaseMemObject(centroids_d);
    clStatus = clReleaseMemObject(closest_centroid_indices_d);
    clStatus = clReleaseMemObject(image_in_d);
    clStatus = clReleaseCommandQueue(command_queue);
    clStatus = clReleaseContext(context);
	free(devices);
    free(platforms);


	// Kernel time (in ms) measurements with OpenCL events
    /*
    cl_ulong time_start, time_end;
    double elapsed_time1, elapsed_time2, elapsed_time3;
    clGetEventProfilingInfo(kernel1_event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
    clGetEventProfilingInfo(kernel1_event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
	elapsed_time1 = (time_end - time_start) / 1000000.0;
	clGetEventProfilingInfo(kernel2_event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
    clGetEventProfilingInfo(kernel2_event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
	elapsed_time2 = (time_end - time_start) / 1000000.0;
    printf("%f    %f\n", elapsed_time1, elapsed_time2);
    */

    //apply new colours to input image
    applyNewColoursToImage(imageIn, closest_centroid_indices, pitch*height, num_of_clusters, centroids);

    //printf("IMAGE: \n");
    //printImage(imageIn, width * height);

    // Save image
	FIBITMAP *imageOutBitmap = FreeImage_ConvertFromRawBits(imageIn, width, height, pitch, 32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE);
	FreeImage_Save(FIF_PNG, imageOutBitmap, "output/test_opencl.png", 0);
	FreeImage_Unload(imageOutBitmap);
}