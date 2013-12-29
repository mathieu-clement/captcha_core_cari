#pragma once
#ifndef CAPTCHA_COMMON_H
#define CAPTCHA_COMMON_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define IMG_WIDTH 150 //!< Image width in pixels
#define IMG_HEIGHT 60 //!< Image height in pixels

/** 
 * Maximum number of adjacent pixel zones. Is checked in a safe way.
 * Because there are 6 letters in each captcha, 20 is already a lot too much.
 * TODO: Includes noise artifacts?
 */
#define CAPTCHA_ARR_SIZE 20 

/**
 * Coordinate
 *
 * \param x column from the left
 * \param y row from the top
 */
typedef struct { int16_t x, y; } Coord;

/**
 * Get above pixel.
 *
 * \param c a coordinate"
 * \return coordinate of pixel above
 */
Coord get_north_coord (Coord c);

/**
 * Get bottom pixel.
 *
 * \param c a coordinate"
 * \return coordinate of pixel bottom
 */
Coord get_south_coord (Coord c);

/**
 * Get bottom pixel.
 *
 * \param c a coordinate"
 * \return coordinate of pixel bottom.
 */
Coord get_west_coord  (Coord c);

/**
 * Get pixel on the right
 *
 * \param c a coordinate"
 * \return coordinate of the pixel on the right
 */
Coord get_east_coord (Coord c); 

/**
 * Get pixel on the bottom right
 *
 * \param c a coordinate"
 * \return coordinate of the pixel on the bottom right
 */
Coord get_south_east_coord (Coord c);

/**
 * Get pixel on the bottom left
 *
 * \param c a coordinate"
 * \return coordinate of the pixel on the bottom left
 */
Coord get_south_west_coord (Coord c);

/**
 * Get pixel on the above right
 *
 * \param c a coordinate"
 * \return coordinate of the pixel on the above right
 */
Coord get_north_east_coord (Coord c);

/**
 * Get pixel on the above left
 *
 * \param c a coordinate"
 * \return coordinate of the pixel on the above left
 */
Coord get_north_west_coord (Coord c);

/**
 * Returns true if coordinate is outside of image.
 *
 * \param 
 * \return true if coordinate is outside of image.
 */
bool is_out_coord (Coord c); 

/**
 * Convert coordinate to index in 1-dimension pixel array.
 *
 * \param x column from left
 * \param y row from top
 * \return index of coordinate in 1-dimension array.
 */
int get_index (int x, int y);

/**
 * Convert coordinate to index in 1-dimension pixel array.
 *
 * \param c a coordinate
 * \return index of coordinate in 1-dimension array.
 */
int get_coord_index (Coord c);

typedef struct {
    uint16_t nbGroups;
    uint8_t* pixels;
} pixels_struct;

int char_to_int(char *char_array, size_t len);

pixels_struct convert_txt_to_1dim_array(FILE* inputf);

typedef float median_elem_type;

median_elem_type median(median_elem_type m[], int n);

median_elem_type mean(median_elem_type m[], int n);

#endif
