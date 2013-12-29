/**
 * \file
 *
 * \brief Main program
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <magick/MagickCore.h>
#include <math.h>
#include "captcha_common.h"

#define ERR_PACKET 2  // TODO: Make other constants for errors

/**
 * Artifact size threshold.
 * Measure the height and width of black areas. If h or w > ths
 * the black area is not considered as noise.
 */
#define ARTIFACT_THR 0

/**
 * Show some debug message and more importantly, 
 * the pixel groups (without noise) as text. ID's are modulo 10.
 */
bool verbose_flag = false;

/**
 * Returns true if pixel packet is black.
 *
 * \param packet a pixel packet
 * \return true if pixel packet is black.
 */
bool is_black (PixelPacket* packet) { return packet->blue > 60000; }

// TODO Make local
uint16_t pixel_groups_index = 1;

/**
 * Read pixels from array and find adjacent black pixels (pixel groups) 
 * to the specified coordinate,
 *
 * \param coord starting coordinate
 * \param packets pixels from image
 * \param visited_pixels array to remember visited pixels
 * \param pixel_groups array whose values are the IDs of the pixel groups.
 *                     Single pixels have a value of 0.
 *                     Noise artifacts are also given an ID.
 *
 * \return true if pixel at the starting coordinate is black.
 */
bool mark_noise_rec (
    Coord coord,
    PixelPacket *packets, 
    int16_t *visited_pixels,
    uint16_t *pixel_groups)
{
    if (is_out_coord(coord)) return false;
    int index = get_coord_index(coord);
    if (visited_pixels[index]) return false;
    
    visited_pixels[index] = 1;

    if (is_black(&packets[index])) {
        pixel_groups[index] = pixel_groups_index;
        mark_noise_rec(get_north_coord(coord), packets, visited_pixels, pixel_groups);
        mark_noise_rec(get_south_coord(coord), packets, visited_pixels, pixel_groups);
        mark_noise_rec(get_east_coord(coord), packets, visited_pixels, pixel_groups);
        mark_noise_rec(get_west_coord (coord), packets, visited_pixels, pixel_groups);
        mark_noise_rec(get_north_east_coord(coord), packets, visited_pixels, pixel_groups);
        mark_noise_rec(get_south_east_coord(coord), packets, visited_pixels, pixel_groups);
        mark_noise_rec(get_north_west_coord(coord), packets, visited_pixels, pixel_groups);
        mark_noise_rec(get_south_west_coord(coord), packets, visited_pixels, pixel_groups);
        return true;
    } else {
        return false;
    } // end if-else
} // end mark_noise_rec()

/**
 * Identifies pixel groups.
 *
 * \param counters uninitialized pointer to a pointer of an array, whose index
 *                 is the ID of a pixel group and the value is the number of
 *                 pixels within that group.
 * \see mark_noise_rec
 */
void mark_noise(PixelPacket *packets, uint16_t *pixel_groups, uint16_t **counters)
{
    int16_t *visited_pixels = calloc(IMG_HEIGHT * IMG_WIDTH, 2);

    for (int row=0; row < IMG_HEIGHT; row++) {
        for (int col = 0; col < IMG_WIDTH; col++) {
            if (mark_noise_rec((Coord) {col, row}, packets, visited_pixels, pixel_groups)) {
                pixel_groups_index++;
            }
        } // end for col
    } // end for row

    // Count pixels per group
    uint16_t *my_counters = calloc(pixel_groups_index+1, 2);
    for (int j=0; j < IMG_WIDTH * IMG_HEIGHT; j++) {
        if (pixel_groups[j] > 0) {
            my_counters[pixel_groups[j]]++;
        }
    } // end for j
    *counters = my_counters;

} // end mark_noise()

/**
 * Sort the left array and report the changes in the second.
 * In other words sort two arrays based on the values of only one.
 * If several elements have the same left value, sort by the right value
 * as well.
 * Arrays are said to be "parallel".
 *
 * \param lefts Left array
 * \param rights Right array
 * \param array_length Number of elements in array
 */
void sort_two_arrays_based_on_first(uint16_t *lefts, uint16_t *rights, int array_length)
{
    // Selection sort on left array. Right array is updated in the very same way.
    uint16_t min;
    uint16_t tmp_left, tmp_right;
    int min_index;
    for (int i=0; i < array_length; i++) {
        min_index = i;
        min = lefts[i];

        for (int j=i+1; j < array_length; j++) {
            if (lefts[j] < min) {
                min_index = j;
                min = lefts[j];
            } // end if
        }

        tmp_left = lefts[i];                tmp_right = rights[i];
        lefts[i] = min;                     rights[i] = rights[min_index];
        lefts[min_index] = tmp_left;        rights[min_index] = tmp_right;

    } // end for

    // Arrays are sorted based on the left.
    // We have to consider the case where left coord is the same but right isn't.
    
    // Find zones with same left indice
    uint16_t *ends   = malloc(sizeof(int) * array_length); // index: beginning, value: end
                                                           // of portion with same left indice.
    for (int i=0; i < array_length; i++) {
        ends[i] = i;
        for (int j=i+1; j < array_length; j++) {
            if (lefts[i] == lefts[j]) ends[i] = j;
        } // end for j
    } // end for i

    // Selection sort on right array
    for (int e=0; e < array_length; e++) {
        for (int i=e; i <= ends[e]; i++) {
            min_index = i;
            min = rights[i];

            for (int j=i+1; j < array_length; j++) {
                if (rights[j] < min) {
                    min_index = j;
                    min = rights[j];
                } // end if

                tmp_right = rights[i];
                rights[i] = rights[min_index];
                rights[min_index] = tmp_right;
            } // end for j
        } // end for i
    } // end for e

    free(ends);
}

/**
 * \brief Remove noise from image
 *
 * Remove noise artifacts from an image, and optionally generate a clean
 * output image, and optionally write pixel groups (without noise) to a
 * text file.
 *
 * \param pgm the execution path of the current ImageMagick client
 * \param inputf the input image in any format supported by ImageMagick
 * \param outputf the output image in any format supported by ImageMagick,
 *                with noise artifacts removed. Set to NULL to not create
 *                the image.
 * \param txt_filename a text file containing the mapping between particular
 *                     pixels and the groups they are in.
 *                     Each line is in the format 
 *                      "%d %d %d", group_id, pixel_row, pixel_column.
 *                     Lines are terminated by "\n".
 */
void remove_noise (char* pgm, char* inputf, char* outputf, char* txt_filename)
{
    // Init
    ExceptionInfo* exception;
    Image *image;
    ImageInfo *image_info;

    MagickCoreGenesis(pgm, MagickTrue);
    exception = AcquireExceptionInfo();
    image_info = CloneImageInfo((ImageInfo *) NULL);
    strcpy(image_info->filename, inputf);

    bool has_txt_file = txt_filename != NULL;
    FILE *txt_file;
    if (has_txt_file) {
        txt_file = fopen(txt_filename, "w");
        if (txt_file == NULL) {
            fprintf(stderr, "Error opening txt file %s.\n", txt_filename);
            exit(5);
        }
    }

    bool has_output_img_file = outputf != NULL;

    // Read input image
    image = ReadImage(image_info, exception);
    if (exception->severity != UndefinedException) CatchException(exception);
    if (image == (Image *) NULL) {
        fprintf(stderr, "Cannot read image %s.\n", inputf);
        exit(EXIT_FAILURE);
    }

    /*************************************************************************/
    /*                              Real work                                */
    /*************************************************************************/

    // Get pixels from source image
    PixelPacket* packets = GetAuthenticPixels (
            image, 0, 0, IMG_WIDTH, IMG_HEIGHT, exception );

    if (exception->severity != UndefinedException) CatchException(exception);
    if (packets == NULL) {
        fprintf(stderr, "Cannot read pixels from image %s.\n", inputf);
        exit(ERR_PACKET);
    }

    // Look for adjacent black pixels
    uint16_t *pixel_groups = calloc(IMG_WIDTH * IMG_HEIGHT, 2);
    uint16_t *counters;
    mark_noise(packets, pixel_groups, &counters);

    // Debug display
    for (int j=0; j < IMG_HEIGHT; j++) {
        for (int i=0; i < IMG_WIDTH; i++) {
            int n = pixel_groups[get_index(i, j)];
            if (counters[n] > ARTIFACT_THR && n != 0) {
                if (verbose_flag) printf("%1d", n % 10);
                if (has_txt_file) fprintf(txt_file, "%d %d %d\n", n, i, j);
            }
            //else if (counters[n] != 0) printf("x");
            else {
                if (verbose_flag) printf(" ");
            }
        }
        if (verbose_flag) printf("\n");
    }

    // Create a new image with the extracted letters from captcha (still skewed)
    MagickPixelPacket background = { .storage_class = DirectClass,
                                     //.colorspace = GRAYColorspace,
                                     .colorspace = RGBColorspace,
                                     .matte = MagickTrue,
                                     .fuzz = 0,
                                     .depth = 0,
                                     .red = 65535,
                                     .green = 65535,
                                     .blue = 65535,
                                     .opacity = 0, // 65535 for transparency
                                     .index = 0 };
                                    
    Image *output_image = NewMagickImage(image_info, IMG_WIDTH, IMG_HEIGHT, &background);
    packets = GetAuthenticPixels (
        output_image, 0, 0, IMG_WIDTH, IMG_HEIGHT, exception);
    
    if (exception->severity != UndefinedException) CatchException(exception);
    if (packets == NULL) exit(3);

    // Match zones of captcha (6 letters) with ajacent pixel zones found earlier
    uint16_t *captcha_groups_matching = calloc(CAPTCHA_ARR_SIZE, 2); // captchas contain captcha_groups_ind letters
    int captcha_groups_ind = 0;
    uint16_t *captcha_lefts  = malloc(CAPTCHA_ARR_SIZE * 2); // most  left coordinate of a symbol
    for (int i=0; i < captcha_groups_ind; i++) captcha_lefts[i] = IMG_WIDTH - 1;
    uint16_t *captcha_rights = calloc(CAPTCHA_ARR_SIZE, 2); // most right coordinate of a symbol

    for (int j=0; j < IMG_HEIGHT; j++) {
        for (int i=0; i < IMG_WIDTH; i++) {
            int index = get_index(i, j);
            int n = pixel_groups[index];
            if (counters[n] > ARTIFACT_THR && n != 0) {

                // Test if symbol already added
                int captcha_index = -1;
                for (int k=0; k < captcha_groups_ind; k++) {
                    if (captcha_groups_matching[k] == n) {
                        captcha_index = k;
                        break;
                    }
                }
                // Add if not existent
                if(captcha_index == -1) {
                    captcha_groups_matching[captcha_groups_ind++] = n;
                    if (captcha_groups_ind == CAPTCHA_ARR_SIZE + 1) {
                        fprintf(stderr, "Too many captcha groups.\n");
                        exit(EXIT_FAILURE);
                    }
                    captcha_index = captcha_groups_ind-1;
                }

                // Adjust left and right bounds
                if (i < captcha_lefts[captcha_index])  captcha_lefts[captcha_index]  = i;
                if (i > captcha_rights[captcha_index]) captcha_rights[captcha_index] = i;

                PixelPacket *packet = &packets[index];
                packet->blue = 0;
                packet->green = 0;
                packet->red = 0;
                packet->opacity = 0;
            } // end if
        } // end for column
    } // end for row

    // Sort arrays
    sort_two_arrays_based_on_first(captcha_lefts, captcha_rights, captcha_groups_ind);

    // Display bounds of each symbol in captcha
    if (verbose_flag) {
        for (int i=0; i < captcha_groups_ind; i++) {
            printf("%d %d\n", captcha_lefts[i], captcha_rights[i]);
        }
    }

    if (SyncAuthenticPixels(output_image, exception) == MagickFalse) exit(3);
    if (exception->severity != UndefinedException) CatchException(exception);


    // Write output image
    if (has_output_img_file) {
        strcpy(image_info->filename, outputf);
        WriteImages(image_info, output_image, outputf, exception);
        if (exception->severity != UndefinedException) CatchException(exception);
    }

    /*************************************************************************/

    // Dealloc
    image_info = DestroyImageInfo(image_info);
    exception = DestroyExceptionInfo(exception);
    MagickCoreTerminus();

    free(captcha_lefts);
    free(captcha_rights);

    // Close txt file
    if (has_txt_file) fclose(txt_file);
}

/**
 * See help and usage.
 */
int main (int argc, char** argv)
{
    char usage_str[] = "Usage: %s [-h] [-v] input_image [output_txt_file] [output_image]\n";

    if ( (argc > 1 && strcmp("-h", argv[1]) == 0) ||
         (argc > 2 && strcmp("-h", argv[2]) == 0) ) {
        printf("%s"
          "Remove noise artifacts from an image and detect groups of pixels.\n"
          "\n"
          "By default, an artifact is any group of pixels counting less than 15 pixels.\n"
          "\n"
          "Parameters\n"
          "==========\n"
          "Input image:        195x50 pixels image in any format supported by ImageMagick.\n"
          "Output text file:   If provided, an ASCII text file, with LF (\\n) terminated lines,\n"
          "                    will be generated. Each line contains three positive integer values\n"
          "                    separated by a space:\n"
          "                        * Pixel group ID (non consecutive)\n"
          "                        * Column (starting from 0, left)\n"
          "                        * Line (starting from 0, top)\n"
          "                    You can easily sort them out with 'sort -n' or similar methods.\n"
          "Output image:       195x50 pixels image, RGB color space. PNG is recommended but you\n"
          "                    can use any format supported by ImageMagick.\n"
          "\n"
          "\n"
          "Mathieu Clément <mathieu.clement@freebourg.org>\n"
          "\n", usage_str);
        exit(EXIT_SUCCESS);
    } 

    // At least input_image should be provided
    if (argc < 2) {
        printf(usage_str, argv[0]);
        exit(EXIT_FAILURE);
    }
    
    verbose_flag = strcmp(argv[1], "-v") == 0;

    // Offset in case of "-v" in front of positional arguments
    int arg_offset = verbose_flag ? 1 : 0;

    // Input image
    char *inputf = argv[1+arg_offset];
    
    // Output txt file
    char *txt_filename = NULL;
    if (argc == 3+arg_offset) { // with output text file
        txt_filename = argv[2+arg_offset];
    }

    // Output image
    char *outputf = NULL;
    if (argc == 4+arg_offset) {
        outputf = argv[3+arg_offset];
    }

    remove_noise (argv[0], inputf, outputf, txt_filename);
} // end main
