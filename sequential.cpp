#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <string.h>
#include <math.h>
#include <time.h>
#include "FreeImage.h"

void printImage(unsigned char *image, int size){
    //helper function for debugging purposes
    for(int i = 0; i < (size); i = i + 4){
        printf("index: %d, B: %d, G: %d, R: %d \n", i, image[i], image[i+1], image[i+2]);
    }
}

void printArrayWithStep4(int* arrayToPrint, int size){
    //helper function for debugging purposes
    for(int c = 0; c < (size); c = c + 4){
        printf("%d %d %d %d \n", arrayToPrint[c], arrayToPrint[c+1], arrayToPrint[c+2], arrayToPrint[c+3]);
    }
}

void initCentroids(int *centroids, int num_of_clusters, unsigned char* imageIn){
    //alpha channel is always 255 because of no transparent images
    int counter = 0;
    for(int i = 0; i < (num_of_clusters * 4); i = i + 4){
        centroids[i] = imageIn[counter];
        centroids[i + 1] = imageIn[counter+1];
        centroids[i + 2] = imageIn[counter+2];
        centroids[i + 3] = 255;
        counter = counter + 4;
    }
}

void applyNewCentroidValue(int centroidIndex, int* centroids, int* closest_centroid_indices, unsigned char *image, int size){
    int count = 0;
    int blue = 0;
    int green = 0;
    int red = 0;
    //go through all points and compute average for all belonging to this cluster
    for(int i = 0; i < (size); i++){
        if(closest_centroid_indices[i] == centroidIndex){
            count++;
            blue = blue + image[i * 4];
            green = green + image[i * 4 + 1];
            red = red + image[i * 4 + 2];
        }
    }
    //compute average, TODO: How to handle empty cluster
    if(count == 0){
        printf("Warning! Cluster %d has no points. \n", centroidIndex);
    }else {
        centroids[centroidIndex * 4] = blue / count;
        centroids[centroidIndex * 4 + 1] = green / count;
        centroids[centroidIndex * 4 + 2] = red / count;
        //we leave alpha untouched
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

void applyNewColoursToImage(unsigned char* image, int* closest_centroid_indices,  int size, int num_of_clusters, int* centroids){
    //for each pixel in image assign it new centroid colour
    for(int i = 0; i < (size); i = i + 4){
        //find colour centroid for this pixel
        int closestCentroid = closest_centroid_indices[i/4];
        //apply centroid colour to this pixel
        image[i] = centroids[closestCentroid * 4];
        image[i+1] = centroids[closestCentroid * 4 + 1];
        image[i+2] = centroids[closestCentroid * 4 + 2];
    }
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
    initCentroids(centroids, num_of_clusters, imageIn);

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
        //step 2: for each centroid compute average which will be new centroid
        for(int centroid = 0; centroid < (num_of_clusters); centroid++){
            applyNewCentroidValue(centroid, centroids, closest_centroid_indices, imageIn, width*height);
        }
    }

    //apply new colours to input image
    applyNewColoursToImage(imageIn, closest_centroid_indices, pitch*height, num_of_clusters, centroids);

    //printf("IMAGE: \n");
    //printImage(imageIn, width * height);

    //encode image back TODO FREEIMAGE
}