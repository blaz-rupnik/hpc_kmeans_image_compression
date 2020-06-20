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

void applyNewCentroidValue(int centroidIndex, int* centroids, long *centroids_sums){
    
    // get sum of all colors for this centroid from centroids_sums
    int blue = centroids_sums[centroidIndex*5];
    int green = centroids_sums[centroidIndex*5 + 1];
    int red = centroids_sums[centroidIndex*5 + 2];
    int alpha = centroids_sums[centroidIndex*5 + 3];
    int count = centroids_sums[centroidIndex*5 + 4];

    // compute average for all non empty clusters
    if(count == 0){
        //printf("Warning! Cluster %d has no points. \n", centroidIndex);
    }else {
        centroids[centroidIndex * 4] = blue / count;
        centroids[centroidIndex * 4 + 1] = green / count;
        centroids[centroidIndex * 4 + 2] = red / count;
        centroids[centroidIndex * 4 + 3] = alpha / count;
    }

    // reset centroids_sums for next iteration
    for (int i = 0; i < 5; i++)
        centroids_sums[centroidIndex*5 + i] = 0;
}

int findClosestCentroid(int *centroids, int num_of_clusters, int blue, int green, int red, int alpha){
    //we init index and distance to 1st centroid, than compute for the remaining ones
    int centroidIndex = 0;
    float minimum_distance = sqrt(pow((float)(centroids[0] - blue), 2.0) + pow((float)(centroids[1] - green), 2.0) + pow((float)(centroids[2] - red), 2.0)
        + pow((float)(centroids[3] - alpha), 2.0));

    for(int i = 4; i < (num_of_clusters * 4); i = i + 4){
        float current_distance = sqrt(pow((float)(centroids[i] - blue), 2.0) + pow((float)(centroids[i+1] - green), 2.0) + pow((float)(centroids[i+2] - red), 2.0)
            + pow((float)(centroids[i+3] - alpha), 2.0));
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
        image[i+3] = centroids[closestCentroid * 4 + 3];
    }
}

float calculateDistance(int blue1, int blue2, int green1, int green2, int red1, int red2, int alpha1, int alpha2){
    return sqrt(pow((float)(blue1 - blue2), 2.0) + pow((float)(green1 - green2), 2.0) + pow((float)(red1 - red2), 2.0)
        + pow((float)(alpha1 - alpha2), 2.0));
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

    //small optimization
    float approximation_error = sqrt(pow(1.0, 2.0) + pow(1.0, 2.0) + pow(1.0, 2.0) + pow(1.0, 2.0)) / (float)(num_of_clusters);
    int previous_closest_centroid = -1;
    int blue_previous = 0;
    int green_previous = 0;
    int red_previous = 0;
    int alpha_previous = 0; 

    //centroid init array
    int *centroids = (int*)malloc(num_of_clusters * 4 * sizeof(int));
    initCentroids(centroids, num_of_clusters, imageIn, width * height);

    //init array for keeping centroid current sums (sums of colors and number of points in centroid)
    long *centroids_sums = (long*)calloc(num_of_clusters * 5, sizeof(long));

    //init array for keeping indices of closest centroid
    int *closest_centroid_indices = (int*)malloc(width * height * sizeof(int));

    printf("%s clusters:%s\n", argv[1], argv[2]);

    for(int iteration = 0; iteration < (num_of_iterations); iteration++){

        // Start measuring time
        struct timespec clock_start, clock_end;
        clock_gettime(CLOCK_MONOTONIC, &clock_start);

        //step 1: go through all points and find closest centroid
        for(int point = 0; point < (width*height); point++){
            int imageStartingPointIndex = point * 4;
            int blue = imageIn[imageStartingPointIndex];
            int green = imageIn[imageStartingPointIndex + 1];
            int red = imageIn[imageStartingPointIndex + 2];
            int alpha = imageIn[imageStartingPointIndex + 3];

            int closest_centroid = 0;
            if(previous_closest_centroid > -1 && calculateDistance(blue, blue_previous, green, green_previous, red, red_previous, alpha, alpha_previous) < approximation_error){
                closest_centroid = previous_closest_centroid;
                closest_centroid_indices[point] = closest_centroid;
            }else {
                closest_centroid = findClosestCentroid(centroids, num_of_clusters, blue, green, red, alpha);
                closest_centroid_indices[point] = closest_centroid;

                previous_closest_centroid = closest_centroid;
                blue_previous = blue;
                green_previous = green;
                red_previous = red;
                alpha_previous = alpha; 
            }
            
            // also save colors to centroids_sums which will be used to update centroids
            centroids_sums[closest_centroid*5] += blue;
            centroids_sums[closest_centroid*5 + 1] += green;
            centroids_sums[closest_centroid*5 + 2] += red;
            centroids_sums[closest_centroid*5 + 3] += alpha;
            centroids_sums[closest_centroid*5 + 4] += 1;
        }
        //step 2: for each centroid compute average which will be new centroid
        for(int centroid = 0; centroid < (num_of_clusters); centroid++){
            applyNewCentroidValue(centroid, centroids, centroids_sums);
        }

        // Stop measuring time
        clock_gettime(CLOCK_MONOTONIC, &clock_end);
        long nanosecs = ((((clock_end.tv_sec - clock_start.tv_sec)*1000*1000*1000) + clock_end.tv_nsec) - (clock_start.tv_nsec));
        printf("%.4f\n", nanosecs/(1000.0*1000.0));
    }

    //apply new colours to input image
    applyNewColoursToImage(imageIn, closest_centroid_indices, pitch*height, num_of_clusters, centroids);

    //printf("IMAGE: \n");
    //printImage(imageIn, width * height);

    // Save image
	FIBITMAP *imageOutBitmap = FreeImage_ConvertFromRawBits(imageIn, width, height, pitch, 32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE);
	FreeImage_Save(FIF_PNG, imageOutBitmap, "output/test_sequential_appx.png", 0);
	FreeImage_Unload(imageOutBitmap);
}