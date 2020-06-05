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

        // Init index and distance to 1st centroid, than compute for the remaining ones
        int centroidIndex = 0;
        int delta_blue = centroids[0] - blue;
        int delta_green = centroids[1] - green;
        int delta_red = centroids[2] - red;
        float minimum_distance = sqrt((float) delta_blue * delta_blue + delta_green * delta_green + delta_red * delta_red);

        for (int i = 4; i < (num_of_clusters * 4); i = i + 4) {
            delta_blue = centroids[i] - blue;
            delta_green = centroids[i+1] - green;
            delta_red = centroids[i+2] - red;
            float current_distance = sqrt((float) delta_blue * delta_blue + delta_green * delta_green + delta_red * delta_red);
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
        //go through all points and compute average for all belonging to this cluster
        for(int i = 0; i < num_of_points; i++){
            if(closest_centroid_indices[i] == gid){
                count++;
                blue = blue + image_in[i * 4];
                green = green + image_in[i * 4 + 1];
                red = red + image_in[i * 4 + 2];
            }
        }
        //compute average, TODO: How to handle empty cluster
        if(count == 0){
            
        } else {
            centroids[gid * 4] = blue / count;
            centroids[gid * 4 + 1] = green / count;
            centroids[gid * 4 + 2] = red / count;
            //we leave alpha untouched
        }
    }

}