/* 
 * File:   bitmaps.h
 * Author: torsten.roemer@luniks.net
 *
 * Created on 16. April 2023, 18:19
 */

#ifndef BITMAPS_H
#define BITMAPS_H

#include <stdint.h>
#include "types.h"

#define BAT_0PCT    0
#define BAT_13PCT   1
#define BAT_25PCT   2
#define BAT_38PCT   3
#define BAT_50PCT   4
#define BAT_63PCT   5
#define BAT_75PCT   6
#define BAT_88PCT   7
#define BAT_100PCT  8

/**
 * A bitmap with its width and height, and data.
 */
typedef struct {
    /** Width of the bitmap, must be a multiple of 8. */
    const width_t width;
    /** Height of the bitmap, must be a multiple of 8. */
    const height_t height;
    /** The actual bitmap. */
    const __flash uint8_t *bitmap;
} Bitmap;

/**
 * Available bitmaps.
 */
extern const __flash Bitmap bitmaps[];

#endif /* BITMAPS_H */

