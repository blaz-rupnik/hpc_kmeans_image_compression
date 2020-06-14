#pragma OPENCL EXTENSION cl_khr_int64_base_atomics: enable

__kernel void find_closest_centroids(int num_of_clusters,
                                    int num_of_points,
                                    __global int *centroids,
                                    __global int *closest_centroid_indices,
                                    __global const unsigned char *image_in) {
    
    // Get the index of the work-item
    int gid = get_global_id(0);
    int lid = get_local_id(0);

    if (gid < num_of_points) {

        int image_point_index = gid * 4;

        int blue = image_in[image_point_index];
        int green = image_in[image_point_index + 1];
        int red = image_in[image_point_index + 2];
        int alpha = image_in[image_point_index + 3];

        // Init index and distance to 1st centroid, than compute for the remaining ones
        int centroidIndex = 0;
        int delta_blue = centroids[0] - blue;
        int delta_green = centroids[1] - green;
        int delta_red = centroids[2] - red;
        int delta_alpha = centroids[3] - alpha;
        float minimum_distance = sqrt((float) delta_blue * delta_blue + delta_green * delta_green + delta_red * delta_red + delta_alpha * delta_alpha);

        for (int i = 4; i < (num_of_clusters * 4); i = i + 4) {
            delta_blue = centroids[i] - blue;
            delta_green = centroids[i+1] - green;
            delta_red = centroids[i+2] - red;
            delta_alpha = centroids[i+3] - alpha;
            float current_distance = sqrt((float) delta_blue * delta_blue + delta_green * delta_green + delta_red * delta_red + delta_alpha * delta_alpha);
            if (current_distance < minimum_distance) {
                centroidIndex = i / 4;
                minimum_distance = current_distance;
            }
        }

        closest_centroid_indices[gid] = centroidIndex;
    }
    
}


__kernel void update_centroids(int num_of_clusters,
                                int num_of_points,
                                __global int *centroids,
                                __global int *closest_centroid_indices,
                                __global const unsigned char *image_in) {
    
    // Get the index of the work-item
    int gid = get_global_id(0);
    int lid = get_local_id(0);
    
    if (gid < num_of_clusters) {

        int count = 0;
        int blue = 0;
        int green = 0;
        int red = 0;
        int alpha = 0;
        //go through all points and compute average for all belonging to this cluster
        for(int i = 0; i < num_of_points; i++){
            if(closest_centroid_indices[i] == gid){
                count++;
                blue = blue + image_in[i * 4];
                green = green + image_in[i * 4 + 1];
                red = red + image_in[i * 4 + 2];
                alpha = alpha + image_in[i * 4 + 3];
            }
        }
        
        if(count != 0) {
            centroids[gid * 4] = blue / count;
            centroids[gid * 4 + 1] = green / count;
            centroids[gid * 4 + 2] = red / count;
            centroids[gid * 4 + 3] = alpha / count;
        }
    }

}

__kernel void find_closest_centroids_2(int num_of_clusters,
                                    int num_of_points,
                                    __global int *centroids,
                                    __global long *centroids_sums,
                                    __global int *closest_centroid_indices,
                                    __global const unsigned char *image_in) {
    
    // Get the index of the work-item
    int gid = get_global_id(0);
    int lid = get_local_id(0);

    if (gid < num_of_points) {

        int image_point_index = gid * 4;

        int blue = image_in[image_point_index];
        int green = image_in[image_point_index + 1];
        int red = image_in[image_point_index + 2];
        int alpha = image_in[image_point_index + 3];

        // Init index and distance to 1st centroid, than compute for the remaining ones
        int centroidIndex = 0;
        int delta_blue = centroids[0] - blue;
        int delta_green = centroids[1] - green;
        int delta_red = centroids[2] - red;
        int delta_alpha = centroids[3] - alpha;
        float minimum_distance = sqrt((float) delta_blue * delta_blue + delta_green * delta_green + delta_red * delta_red + delta_alpha * delta_alpha);

        for (int i = 4; i < (num_of_clusters * 4); i = i + 4) {
            delta_blue = centroids[i] - blue;
            delta_green = centroids[i+1] - green;
            delta_red = centroids[i+2] - red;
            delta_alpha = centroids[i+3] - alpha;
            float current_distance = sqrt((float) delta_blue * delta_blue + delta_green * delta_green + delta_red * delta_red + delta_alpha * delta_alpha);
            if (current_distance < minimum_distance) {
                centroidIndex = i / 4;
                minimum_distance = current_distance;
            }
        }

        closest_centroid_indices[gid] = centroidIndex;
        atomic_add(&centroids_sums[centroidIndex * 5], blue);
        atomic_add(&centroids_sums[centroidIndex * 5 + 1], green);
        atomic_add(&centroids_sums[centroidIndex * 5 + 2], red);
        atomic_add(&centroids_sums[centroidIndex * 5 + 3], alpha);
        atomic_add(&centroids_sums[centroidIndex * 5 + 4], 1);
    }
    
}


__kernel void update_centroids_2(int num_of_clusters,
                                __global int *centroids,
                                __global long *centroids_sums) {
    
    // Get the index of the work-item
    int gid = get_global_id(0);
    
    if (gid < num_of_clusters) {

        int blue = centroids_sums[gid * 5];
        int green = centroids_sums[gid * 5 + 1];
        int red = centroids_sums[gid * 5 + 2];
        int alpha = centroids_sums[gid * 5 + 3];
        int count = centroids_sums[gid * 5 + 4];

        if (count != 0) {
            centroids[gid * 4] = blue / count;
            centroids[gid * 4 + 1] = green / count;
            centroids[gid * 4 + 2] = red / count;
            centroids[gid * 4 + 3] = alpha / count;
        }

        centroids_sums[gid * 5] = 0;
        centroids_sums[gid * 5 + 1] = 0;
        centroids_sums[gid * 5 + 2] = 0;
        centroids_sums[gid * 5 + 3] = 0;
        centroids_sums[gid * 5 + 4] = 0;

    }

}

// This kernel is optimization for small number of clusters
__kernel void find_closest_centroids_3(int num_of_clusters,
                                    int num_of_points,
                                    __global int *centroids,
                                    __global long *centroids_sums,
                                    __local int *centroids_sums_local,
                                    __global int *closest_centroid_indices,
                                    __global const unsigned char *image_in) {
    
    // Get the index of the work-item
    int gid = get_global_id(0);
    int lid = get_local_id(0);
    int loc_size = get_local_size(0);

    // Divide work
    int displ = lid * num_of_clusters / loc_size;
    int num_of_work = (lid + 1)*num_of_clusters / loc_size - lid * num_of_clusters / loc_size;

    // Init local centroid sums
    for (int i = displ; i < displ + num_of_work; i++) {
        centroids_sums_local[i * 5 + 0] = 0;
        centroids_sums_local[i * 5 + 1] = 0;
        centroids_sums_local[i * 5 + 2] = 0;
        centroids_sums_local[i * 5 + 3] = 0;
        centroids_sums_local[i * 5 + 4] = 0;
    }

    if (gid < num_of_points) {

        int image_point_index = gid * 4;

        int blue = image_in[image_point_index];
        int green = image_in[image_point_index + 1];
        int red = image_in[image_point_index + 2];
        int alpha = image_in[image_point_index + 3];

        // Init index and distance to 1st centroid, than compute for the remaining ones
        int centroidIndex = 0;
        int delta_blue = centroids[0] - blue;
        int delta_green = centroids[1] - green;
        int delta_red = centroids[2] - red;
        int delta_alpha = centroids[3] - alpha;
        float minimum_distance = sqrt((float) delta_blue * delta_blue + delta_green * delta_green + delta_red * delta_red + delta_alpha * delta_alpha);

        for (int i = 4; i < (num_of_clusters * 4); i = i + 4) {
            delta_blue = centroids[i] - blue;
            delta_green = centroids[i+1] - green;
            delta_red = centroids[i+2] - red;
            delta_alpha = centroids[i+3] - alpha;
            float current_distance = sqrt((float) delta_blue * delta_blue + delta_green * delta_green + delta_red * delta_red + delta_alpha * delta_alpha);
            if (current_distance < minimum_distance) {
                centroidIndex = i / 4;
                minimum_distance = current_distance;
            }
        }

        closest_centroid_indices[gid] = centroidIndex;

        barrier(CLK_LOCAL_MEM_FENCE);

        // First add to local centroids sums 
        atomic_add(&centroids_sums_local[centroidIndex * 5], blue);
        atomic_add(&centroids_sums_local[centroidIndex * 5 + 1], green);
        atomic_add(&centroids_sums_local[centroidIndex * 5 + 2], red);
        atomic_add(&centroids_sums_local[centroidIndex * 5 + 3], alpha);
        atomic_add(&centroids_sums_local[centroidIndex * 5 + 4], 1);

    }

    barrier(CLK_LOCAL_MEM_FENCE);

    // Write to global memory for each cluster
    for (int i = displ; i < displ + num_of_work; i++) {
        atomic_add(&centroids_sums[i * 5], centroids_sums_local[i * 5]);
        atomic_add(&centroids_sums[i * 5 + 1], centroids_sums_local[i * 5 + 1]);
        atomic_add(&centroids_sums[i * 5 + 2], centroids_sums_local[i * 5 + 2]);
        atomic_add(&centroids_sums[i * 5 + 3], centroids_sums_local[i * 5 + 3]);
        atomic_add(&centroids_sums[i * 5 + 4], centroids_sums_local[i * 5 + 4]);
    }
    
}