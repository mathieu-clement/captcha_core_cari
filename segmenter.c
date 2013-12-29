/**
 * \file
 *
 * \brief Main program (Alone pixels remover)
 *
 * Sometimes symbols contain additional thin lines, not part of the symbol 
 * and thus considered as noise.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "captcha_common.h"

/**
 * To be considered alone, a pixel should have less than this number
 * of brothers (adjacent pixels from the same group, including diagonal).
 */
#define MIN_BROTHERS 2


#define min(a,b) ((a < b) ? (a) : (b))
#define max(a,b) ((a > b) ? (a) : (b))

int withinBounds(int f, int low, int high)
{
    if (f >= low && f < high) return 1; else return -1;
}

// In hole = !hasExit
bool has_exit(int x, int y, int firstX, int firstY, int lastX, int lastY, 
                 uint8_t *pixels, int groupId, uint8_t *visited, uint8_t *exit_array)
{
    int index = get_index(x,y);
    if (visited[index]) return exit_array[index];
    visited[index] = true;

    // On a pixel from the group = wall.
    int p = pixels[index];
    bool is_wall = p == groupId;
    if (is_wall) {
        exit_array[index] = false;
        return false;
    }

    bool has_exit_somewhere = false;
    for (int varX = x-1; varX <= x+1 && !has_exit_somewhere; varX++) {
        for (int varY = y-1; varY <= y+1 && !has_exit_somewhere; varY++) {
            if (varX == x && varY == y) continue; // Ignore pixel corresponding to x,y params.

            has_exit_somewhere = (varX < firstX||varX > lastX||varY < firstY||varY > lastY) || 
                has_exit(varX, varY, firstX, firstY, lastX, lastY, 
                         pixels, groupId, visited, exit_array);
        }
    }
    exit_array[index] = has_exit_somewhere;
    return has_exit_somewhere;
}
 
// Returns true if pixel (x,y) is in a hole of the symbol (letters O, D, B, etc.)
// x: absolute x
// y: absolute y
// firstX: first existent x
// firstY: first existent y
// lastX: last existent x
// lastY: last existent y
// groupId: id of symbol as found in pixels array
bool in_hole(int x, int y, int firstX, int firstY, int lastX, int lastY, uint8_t *pixels,
             size_t pixels_len, int groupId)
{
    if (pixels[get_index(x,y)] == groupId) return false;

    uint8_t *visited = calloc(pixels_len, 1);
    uint8_t *exit_array = calloc(pixels_len, 1);
    bool r = has_exit(x, y, firstX, firstY, lastX, lastY, pixels, groupId, visited, exit_array);
    
    free(visited);
    free(exit_array);
    
    return !r;
}

void remove_alone_pixels(char* input_filename, char* output_filename)
{
    // Create file handles or exit
    FILE* inputf = fopen(input_filename, "r");
    if (inputf == NULL) {
        fprintf(stderr, "Error opening input file %s.\n", input_filename);
        exit(EXIT_FAILURE);
    }

    bool has_outputf = output_filename != NULL;
    bool is_stdout = false;
    FILE *outputf;
    if (has_outputf) {
        if (strcmp("-", output_filename) == 0) {
            outputf = stdout;
            is_stdout = true;
        } else {
            outputf = fopen(output_filename, "w");
        }
        if (outputf == NULL) {
            fprintf(stderr, "Error opening output file %s.\n", output_filename);
            fclose(inputf);
            exit(EXIT_FAILURE);
        }
    }

    // Convert to one dimension array
    pixels_struct pstruct = convert_txt_to_1dim_array(inputf);
    uint16_t nbGroups = pstruct.nbGroups;
    uint8_t *pixels = pstruct.pixels;

    /*
     * If pixel is on top or bottom symbol and has no left and right brother
     * remove it.
     */
    // 1st pass: Determine "altitude" of highest and lowest pixel of symbol
    uint16_t yMins[nbGroups+1]; // Minimums, index is groupID. First = 1
    uint16_t yMaxs[nbGroups+1]; // Maximums, index is groupID. First = 1
    // Same with left - right
    uint16_t xMins[nbGroups+1]; // Minimums, index is groupID. First = 1
    uint16_t xMaxs[nbGroups+1]; // Maximums, index is groupID. First = 1
    for (int i = 0; i < nbGroups+1; i++) {
        yMins[i] = UINT8_MAX;
        yMaxs[i] = 0; // minimum of unsigned is 0.
        xMins[i] = UINT8_MAX;
        xMaxs[i] = 0; // minimum of unsigned is 0.
    }

    for (int x = 0; x < IMG_WIDTH; x++) {
        for (int y = 0; y < IMG_HEIGHT; y++) {
            uint16_t groupId = pixels[get_index(x,y)];
            if (groupId != 0) {
                if (y < yMins[groupId])
                    yMins[groupId] = y;
                if (y > yMaxs[groupId])
                    yMaxs[groupId] = y;
                if (x < xMins[groupId])
                    xMins[groupId] = x;
                if (x > xMaxs[groupId])
                    xMaxs[groupId] = x;
            }
        }
    } // end for x

    // Associate dot of letters i and j with the bottom of the letter
    int deleted_groups[nbGroups+1]; // key: old group Id, value = new group Id
                                    // any item with a positive value in this array
                                    // is very probably a i or j
    for (int i = 1; i < nbGroups + 1; i++) deleted_groups[i] = false;
    int nbDeletedGroups = 0;
    
    for (int groupId = 1; groupId < nbGroups + 1; groupId++) {
        // detect if symbol within bounds
        int nearGroupId = -1;
        for (int otherGroupId = 1; otherGroupId < nbGroups + 1; otherGroupId++) {
            if (groupId == otherGroupId) continue;

            // 3 is to give some flexibility when the symbol is rotated / skewed
            if (xMins[groupId] >= xMins[otherGroupId] - 3 &&
                xMaxs[groupId] <= xMaxs[otherGroupId] + 3) {
                nearGroupId = otherGroupId;
                break;
            }
        }
        if (nearGroupId != -1 && !deleted_groups[nearGroupId]) {
            for (int x = xMins[groupId]; x <= xMaxs[groupId]; x++) {
                for (int y = yMins[groupId]; y <= yMaxs[groupId]; y++) {
                    int index = get_index(x,y);
                    if (pixels[index] == groupId) {
                        pixels[index] = nearGroupId;
                    }
                }
            }
            yMins[nearGroupId] = min(yMins[groupId], yMins[nearGroupId]);
            xMins[nearGroupId] = min(xMins[groupId], xMins[nearGroupId]);
            yMaxs[nearGroupId] = max(yMaxs[groupId], yMaxs[nearGroupId]);
            xMaxs[nearGroupId] = max(xMaxs[groupId], xMaxs[nearGroupId]);

            deleted_groups[groupId] = nearGroupId;
            nbDeletedGroups++;
        }
    }


    // 2nd pass: if top and bottom pixels have no brother on the same height, remove them.
    /*
    for (int groupId = 1; groupId < nbGroups+1; groupId++) { // for each group
        uint16_t min = yMins[groupId];
        uint16_t max = yMaxs[groupId];

        for (int x = 1; x < IMG_WIDTH - 1; x++) { // for pixels on the bottom and top line
            if (pixels[get_index(x, min)] == groupId) {
                if (pixels[get_index(x-1, min)] != groupId && 
                    pixels[get_index(x+1, min)] != groupId) {
                        pixels[get_index(x, min)] = 0;
                }
            }
            if (pixels[get_index(x, max)] == groupId) {
                if (pixels[get_index(x-1, max)] != groupId && 
                    pixels[get_index(x+1, max)] != groupId) {
                        pixels[get_index(x, max)] = 0;
                }
            }
        }
    }
    */

    int mapping[nbGroups+1]; // mapping between groupId and idShown

    printf("Number of symbols: %d\n", nbGroups - nbDeletedGroups);
    printf("START GLOBAL DRAWING\n");

    // Debug display
    for (int y = 0; y < IMG_HEIGHT; y++) {
        for (int x = 0; x < IMG_WIDTH; x++) {
            int val = pixels[get_index(x,y)];
            if (val) printf("%d", val);
            else printf(" ");
        }
        printf("\n");
    }

    printf("STOP GLOBAL DRAWING\n");
    

    int idShown = 1;

    /**
     * Print ragged left version
     */
    for (int groupId = 1; groupId < nbGroups+1; groupId++) { // for each group
        if (deleted_groups[groupId]) continue;

        mapping[groupId] = idShown;

        printf("-------- Group %d --------\n", idShown);
        int width = xMaxs[groupId] - xMins[groupId];
        int height = yMaxs[groupId] - yMins[groupId];
        printf("%d x %d\n\n", width, height);

        printf("START SYMBOL %d\n", idShown);
        printf("\n");

        // Temporary work variables
        bool last_pixel_on = false; // horizontal        
        median_elem_type *lengths = malloc( (height+1) * sizeof(median_elem_type));
        int distance_from_center_horiz_total = 0;
        int distance_from_center_horiz_measurements = 0;
        int distance_from_center_vert_total = 0;
        int distance_from_center_vert_measurements = 0;

        /**
         * FEATURES
         */
        int length = 0; // number of pixels in symbol
        int broadest_segment = 0; // longest horizontal line of continuous pixels
        bool has_hole = false; // 1 if letter has a hole, exhaustive list: B, D, G, O, P, Q, R
        int max_horiz_transitions = 0; // max number of transitions in all rows
        int max_vert_transitions = 0; // max number of transitions in all columns
        int some_horiz_transitions[] = { 0, 0, 0, 0, 0 }; // other transitions (1/3, 1/2, 2/3, 1/4, 3/4)
        int some_vert_transitions[] =  { 0, 0, 0, 0, 0 }; // other transitions (1/3, 1/2, 2/3, 1/4, 3/4)
        // Taken from document "Optical character recognition system using
        // support vecto machines" by "Eugen-Dumitru Tautu and Florin Leon"
        float mean_distance_from_center_horiz = 0;
        float mean_distance_from_center_vert = 0;

        // Visit pixels of symbol zone
        for (int y = yMins[groupId]; y < yMaxs[groupId] + 1; y++) {
            int firstX = -1;
            last_pixel_on = false;
            int relY = y - yMins[groupId]; // relative Y (0, 1, 2, 3...)
            int theseTransitions = 0;

            for (int x = xMins[groupId]; x < xMaxs[groupId] + 1; x++) {
                int relX = x - xMins[groupId]; // relative X (0, 1, 2, 3...)

                int val = pixels[get_index(x,y)];
                if (val == groupId) { // pixel part of the symbol
                    printf("%d", val);

                    distance_from_center_horiz_total += abs(width / 2 - relX);
                    distance_from_center_horiz_measurements++;
                    distance_from_center_vert_total += abs(height / 2 - relY);
                    distance_from_center_vert_measurements++;

                    if (!last_pixel_on) {
                        firstX = x; // used for broadest segment
                        theseTransitions++;

                        if (height/3 == relY) some_horiz_transitions[0]++;
                        else if (height/2 == relY) some_horiz_transitions[1]++;
                        else if (2*height/3 == relY) some_horiz_transitions[2]++;
                        else if (height/4 == relY) some_horiz_transitions[3]++;
                        else if (3*height/4 == relY) some_horiz_transitions[4]++;
                    }

                    // update broadest segment
                    if (x == xMaxs[groupId]) {
                        if ((x - firstX) > broadest_segment) {
                            broadest_segment = (x - firstX);
                        }

                        lengths[relY] = (median_elem_type) (x - firstX);
                    }

                    last_pixel_on = true;
                    length++;
                }
                else { // background pixel or from another symbol
                    printf(" ");

                    // update broadest segment
                    if (last_pixel_on) {
                        theseTransitions++;

                        if ((x - firstX) > broadest_segment) {
                            broadest_segment = x - firstX;
                        }

                        lengths[relY] = (median_elem_type) (x - firstX);
                    } // end if last pixel on

                    last_pixel_on = false;
                } // end if part of symbol
            } // end for x

            if (theseTransitions > max_horiz_transitions) {
                max_horiz_transitions = theseTransitions;
            }

            printf("\n");
        } // end for y
        
        for (int x = xMins[groupId]; x < xMaxs[groupId] + 1; x++) {
            last_pixel_on = false;
            int theseTransitions = 0;

            for (int y = yMins[groupId]; y < yMaxs[groupId] + 1; y++) {
                int relY = y - yMins[groupId]; // relative Y (0, 1, 2, 3...)
                
                int val = pixels[get_index(x,y)];
                if (val == groupId) { // pixel part of the symbol
                    if (!last_pixel_on) {
                        if (width/3 == relY) some_vert_transitions[0]++;
                        else if (width/2 == relY) some_vert_transitions[1]++;
                        else if (2*width/3 == relY) some_vert_transitions[2]++;
                        else if (width/4 == relY) some_vert_transitions[3]++;
                        else if (3*width/4 == relY) some_vert_transitions[4]++;

                        theseTransitions++;
                    } // end if last pixel not on
                    last_pixel_on = true;
                } else {
                    if (last_pixel_on) {
                        theseTransitions++;
                    } // end if last pixel on
                    last_pixel_on = false;
                } // end if part of symbol
            } // end for y (2)
            if (theseTransitions > max_vert_transitions) {
                max_vert_transitions = theseTransitions;
            }
        } // end for x (2)
        printf("\n");
        printf("STOP SYMBOL %d\n", idShown);


        // Detect holes
        // The idea is to find our way to an "exit". If this is not possible,
        // we are trapped within the symbol, and so, there is a "hole" in it.
        // TODO: Choose random pixels instead with a bias in the center.
        for (int y = yMins[groupId]; y < yMaxs[groupId] + 1; y++) {
            if (has_hole) break;

            for (int x = xMins[groupId]; x < xMaxs[groupId] + 1; x++) {

                if (in_hole(x, y, 
                            xMins[groupId], yMins[groupId], 
                            xMaxs[groupId], yMaxs[groupId], 
                            pixels, IMG_WIDTH * IMG_HEIGHT, groupId)) {
                    has_hole = true;
                    break;
                }
            } // end for x (hole)
        } // end for y (hole)

        /**
         * Zoning
         */
        static const int H_ZONES = 3;
        static const int V_ZONES = 3;
        int zone_width = width/H_ZONES;
        int zone_height = height/V_ZONES;
        int zone_counts[H_ZONES * V_ZONES];
        float zone_scaled[H_ZONES * V_ZONES];
        for (int i=0; i < H_ZONES * V_ZONES; i++) {
            zone_counts[i] = 0;
        }
        
        for (int h = 0; h < H_ZONES; h++) {
            for (int v = 0; v < V_ZONES; v++) {
                for (int y = yMins[groupId] + v * zone_height; y < yMins[groupId] + (v+1) * zone_height; y++) {
                    for (int x = xMins[groupId] + h * zone_width; x < xMins[groupId] + (h+1) * zone_width; x++) {
                        if (pixels[get_index(x, y)] == groupId) {
                            zone_counts[h * V_ZONES + v]++;
                        }
                    } // end for x (x coordinate)
                } // end for y (y coordinate)
            } // end for v (vertical zones)
        } // end for h (horizontal zones)
        
        for (int i=0; i < H_ZONES * V_ZONES; i++) {
            zone_scaled[i] = 2.0 * ((float) zone_counts[i] / (zone_height * zone_width)) - 1;
            if (isnan(zone_scaled[i])) zone_scaled[i] = 0;
        }


        /**
         * Light matches
         * Draw a line somewhere in the character and look if any on pixel is on it.
         */
        float light_matches[7];
        int n, o;
        // The following code is for...
        // -----------
        // |         |
        // |         |
        // |         |
        // |    |    | <== that line in the middle, at 8/10 of height
        // -----------
        n = 0;
        o = 0;
        int xMiddle = xMins[groupId] + 1.0 * (xMaxs[groupId] - xMins[groupId] + 1) / 2.0;
        for ( int y = yMins[groupId] + 0.8 * (yMaxs[groupId] - yMins[groupId] + 1);
                  y <= yMaxs[groupId]; y++) {
            if (pixels[get_index(xMiddle,y)] == groupId) {
                n++;
            }
            o++;
        }
        #define Update_light_matches(lm) do { light_matches[lm] = 2.0 * ((float) n / o) - 1;if (isnan(light_matches[lm])) fprintf(stderr, "Light match %d is Nan.\n", lm); } while (0)
        Update_light_matches(0);

        // -----------
        // |         |
        // |-- < here|
        // |         |
        // |         |
        // |         |
        // -----------
        n = 0;
        o = 0;
        int y14 = yMins[groupId] + 0.25 * (yMaxs[groupId] - yMins[groupId] + 1);
        for ( int x = xMins[groupId];
                  x <= xMins[groupId] + 0.33 * (xMaxs[groupId] - xMins[groupId] + 1);
                  x++) {
            if (pixels[get_index(x,y14)] == groupId) {
                n++;
            }
            o++;
        }
        Update_light_matches(1);
        
        // -----------
        // |         |
        // |         |
        // |         |
        // |-- < here|
        // |         |
        // -----------
        n = 0;
        o = 0;
        int y34 = yMins[groupId] + 0.75 * (yMaxs[groupId] - yMins[groupId] + 1);
        for ( int x = xMins[groupId];
                  x <= xMins[groupId] + 0.33 * (xMaxs[groupId] - xMins[groupId] + 1);
                  x++) {
            if (pixels[get_index(x,y34)] == groupId) {
                n++;
            }
            o++;
        }
        Update_light_matches(2);

        // -----------
        // |         |
        // |here > --|
        // |         |
        // |         |
        // |         |
        // -----------
        n = 0;
        o = 0;
        int y14r = yMins[groupId] + 0.33 * (yMaxs[groupId] - yMins[groupId] + 1);
        for ( int x = xMins[groupId] + 0.66 * (xMaxs[groupId] - xMins[groupId] + 1);
                  x <= xMaxs[groupId];
                  x++) {
            if (pixels[get_index(x,y14r)] == groupId) {
                n++;
            }
            o++;
        }
        Update_light_matches(3);

        // -----------
        // |         |
        // |         |
        // |         |
        // |here > --|
        // |         |
        // -----------
        n = 0;
        o = 0;
        int y34r = yMins[groupId] + 0.66 * (yMaxs[groupId] - yMins[groupId] + 1);
        for ( int x = xMins[groupId] + 0.66 * (xMaxs[groupId] - xMins[groupId] + 1);
                  x <= xMaxs[groupId];
                  x++) {
            if (pixels[get_index(x,y34r)] == groupId) {
                n++;
            }
            o++;
        }
        Update_light_matches(4);


        // -----------
        // |         |
        // |         |
        // |here > --|
        // |         |
        // |         |
        // -----------
        n = 0;
        o = 0;
        int y12r = yMins[groupId] + 0.50 * (yMaxs[groupId] - yMins[groupId] + 1);
        for ( int x = xMins[groupId] + 0.66 * (xMaxs[groupId] - xMins[groupId] + 1);
                  x <= xMaxs[groupId];
                  x++) {
            if (pixels[get_index(x,y12r)] == groupId) {
                n++;
            }
            o++;
        }
        Update_light_matches(5);


        // -----------
        // |         |
        // |         |
        // |-- < here|
        // |         |
        // |         |
        // -----------
        n = 0;
        o = 0;
        int y12 = yMins[groupId] + 0.50 * (yMaxs[groupId] - yMins[groupId] + 1);
        for ( int x = xMins[groupId];
                  x <= xMaxs[groupId] + 0.33 * (xMaxs[groupId] - xMins[groupId] + 1);
                  x++) {
            if (pixels[get_index(x,y12)] == groupId) {
                n++;
            }
            o++;
        }
        Update_light_matches(6);


        /**
         * Check if symbol has a dot
         */
        int dot_feature = -1;
        for (int sym = 1; sym <= nbGroups; sym++) {
            if (sym == groupId) continue;
            if (deleted_groups[sym] == groupId) {
                dot_feature = +1;
                break;
            }
        }

        // Print features and measurements
        printf("\n");
        float aan_relative_length = (2.0 * length / (width*height)) - 1;
        float aan_relative_broadest_segment = (2.0 * broadest_segment / width) - 1;
        mean_distance_from_center_horiz = (float) distance_from_center_horiz_total /
                                          distance_from_center_horiz_measurements;
        mean_distance_from_center_vert = (float) distance_from_center_vert_total /
                                          distance_from_center_vert_measurements;
        printf("\n");
        printf("CODED FEATURES "
                "%.3f " // symbol height (on 22)
                "%.4f " // relative length
                "%.3f " // relative broadest segment
                "%.3f " // max horiz transitions (max 8 scaled from -1 to +1)
                "%.3f " // max vert transitions (max 8 scaled from -1 to +1)
                "%.3f " // mean distance from center (horizontal)
                "%.3f " // mean distance from center (vertical)
                "%d "   // horiz. transitions first 1/3 height
                "%d "   // horiz. transitions half height
                "%d "   // horiz. transitions second 1/3 height
                "%d "   // horiz. transitions 1/4
                "%d "   // horiz. transitions 3/4
                "%.3f "   // light match 0
                "%.3f "   // light match 1
                "%.3f "   // light match 2
                "%.3f "   // light match 3
                "%.3f "   // light match 4
                "%.3f "   // light match 5
                "%.3f "   // light match 6
                "%d "   // dot feature
                "%d "   // yMax
                "%d ",  // has hole
               (2.0 * (height > 22 ? 22 : height)) / 22 - 1,
               aan_relative_length,
               aan_relative_broadest_segment,
               (2.0 * (max_horiz_transitions > 14 ? 14 : max_horiz_transitions)) / 14 - 1,
               (2.0 * (max_vert_transitions  > 14 ? 14 : max_vert_transitions )) / 14 - 1,
               (2.0 * (mean_distance_from_center_horiz / width/2)) - 1,
               (2.0 * (mean_distance_from_center_vert / height/2)) - 1,
               some_horiz_transitions[0],
               some_horiz_transitions[1],
               some_horiz_transitions[2],
               some_horiz_transitions[3],
               some_horiz_transitions[4],
               light_matches[0],
               light_matches[1],
               light_matches[2],
               light_matches[3],
               light_matches[4],
               light_matches[5],
               light_matches[6],
               dot_feature,
               (yMaxs[groupId] > 42) ? 1 : -1,
               has_hole ? 1 : -1
               );
        for (int i=0; i < H_ZONES * V_ZONES; i++) {
            printf("%.3f ", zone_scaled[i]);
        }
        printf("\n");
        printf("\n");

        // Cleaning
        free(lengths);

        idShown++;
    } // end for each group

    // Reading order
    printf("READING ORDER ");
    int alreadySeenIndex = -1;
    int alreadySeen[6];
    for (int x = 0; x < IMG_WIDTH; x++) {
        for (int y = 0; y < IMG_HEIGHT; y++) {
            int val = pixels[get_index(x, y)];
            if (val != 0) {
                bool found = false;
                for (int w=0; w <= alreadySeenIndex; w++) {
                    if (alreadySeen[w] == val) {
                        found = true;
                        break;
                    }
                } // end for
                if (!found) {
                    printf("%d ", mapping[val]);
                    alreadySeen[++alreadySeenIndex] = val;
                } // end if !found
            } // end if val
        } // end for y
    } // end for x
    printf("\n");


    // Cleaning
    fclose(inputf);
    if (has_outputf && !is_stdout) fclose(outputf);
    free(pixels);

} // end remove_alone_pixels()

int main (int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s input_txt_file [outputf]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char* output_filename = NULL;
    if (argc > 2)  output_filename = argv[2];

    remove_alone_pixels(argv[1], output_filename);
} // end main()
