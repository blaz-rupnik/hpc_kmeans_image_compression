#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <string.h>
#include <math.h>
#include <time.h>
#include "FreeImage.h"

void printImage(unsigned char *image, int size){
    for(int i = 0; i < (size); i++){
        printf("%d %d \n", i, image[i]);
    }
}

void initCentroids(int *centroids, int num_of_clusters){
    //alpha channel is always 255 because of no transparent images
    for(int i = 0; i < (num_of_clusters * 4); i = i + 4){
        centroids[i] = rand() % 256;
        centroids[i + 1] = rand() % 256;
        centroids[i + 2] = rand() % 256;
        centroids[i + 3] = 255;
    }
}

int findClosestCentroid(int *centroids, int num_of_clusters, int blue, int green, int red){
    //we init index and distance to 1st centroid, than compute for the remaining ones
    int centroidIndex = 0;
    float minimum_distance = sqrt(pow((float)(centroids[0] - blue), 2.0) + pow((float)(centroids[1] - green), 2.0) + pow((float)(centroids[2] - red), 2.0));

    for(int i = 4; i < (num_of_clusters * 4); i = i + 4){
        float current_distance = sqrt(pow((float)(centroids[i] - blue), 2.0) + pow((float)(centroids[i+1] - green), 2.0) + pow((float)(centroids[i+2] - red), 2.0));
        if(current_distance < minimum_distance){
            centroidIndex = i / 4;
            minimum_distance = current_distance;
        }
    }

    return centroidIndex;
}

int main(int argc, char *argv[]){
    //Load image from file
    //1st argument is image name including format
	FIBITMAP *imageLoad = FreeImage_Load(FIF_PNG, argv[1], 0);
	//Convert it to a 32-bit image
    FIBITMAP *imageLoad32 = FreeImage_ConvertTo32Bits(imageLoad);
	
    //Get image dimensions
    int width = FreeImage_GetWidth(imageLoad32);
	int height = FreeImage_GetHeight(imageLoad32);
	int pitch = FreeImage_GetPitch(imageLoad32);
    printf("width: %d \n", width);

	//Prepare room for a raw data copy of the image
    unsigned char *imageIn = (unsigned char *)malloc(height*pitch * sizeof(unsigned char));

    //Extract raw data from the image
	FreeImage_ConvertToRawBits(imageIn, imageLoad32, pitch, 32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE);

    //Free source image data free
	FreeImage_Unload(imageLoad32);
	FreeImage_Unload(imageLoad);

    //get number of clusters from 2nd argument and num of iteration from 3rd argument
    int num_of_clusters = atoi(argv[2]);
    int num_of_iterations = atoi(argv[3]); 

    //initialize random seed
    srand((unsigned int)time(NULL));

    //centroid init array
    int *centroids = (int*)malloc(num_of_clusters * 4 * sizeof(int));
    initCentroids(centroids, num_of_clusters);

    //init array for keeping indices of closest centroid
    int *closest_centroid_indices = (int*)malloc(width * height * sizeof(int));

    for(int iteration = 0; iteration < (num_of_iterations); iteration++){
        //step 1: go through all points and find closest centroid
        for(int point = 0; point < (width*height); point++){
            int imageStartingPointIndex = point * 4;
            int blue = imageIn[imageStartingPointIndex];
            int green = imageIn[imageStartingPointIndex + 1];
            int red = imageIn[imageStartingPointIndex + 2];
            closest_centroid_indices[point] = findClosestCentroid(centroids, num_of_clusters, blue, green, red);
        }
        //step 2 TODO
    }
    
}