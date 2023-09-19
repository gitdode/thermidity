/* 
 * File:   font.h
 * Author: torsten.roemer@luniks.net
 *
 * Created on 24. April 2023, 21:21
 */

#ifndef FONT_H
#define FONT_H

#include "types.h"

/**
 * A glyph with its pseudo UTF-8 code point, width and bitmap.
 */
typedef struct {
    /** Pseudo UTF-8 code point of the glyph. */
    const code_t code;
    /** Width of the glyph. */
    const width_t width;
    /** Bitmap of the glyph. */
    const __flash uint8_t *bitmap;
} Glyph;

/**
 * Fonts available here. Since the height is the same for all glyphs,
 * it is stored in the font instead of redundantly in each glyph.
 */
typedef struct {
    /** Glyphs of this font. */
    const __flash Glyph *glyphs;
    /** Number of glyphs of this font. */
    const length_t length;
    /** Height of (the glyphs of) this font. */
    const height_t height;
} Font;

/**
 * Returns the flash address of the glyph at the given pseudo UTF-8 code
 * point, i.e. 0x00f6 for U+00F6 from the given font.
 * If there is no glyph for that code point, a question mark is returned.
 * @param font
 * @param code
 * @return Glyph
 */
const __flash Glyph* getGlyphAddress(const __flash Font *font, code_t code);

#endif /* FONT_H */
