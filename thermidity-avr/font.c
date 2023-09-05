/* 
 * File:   font.c
 * Author: torsten.roemer@luniks.net
 *
 * Created on 25. April 2023, 21:00
 */

#include <stdio.h>
#include "font.h"

const __flash Glyph* getGlyphAddress(const __flash Font *font, uint16_t code) {
    
    // https://en.wikipedia.org/wiki/Binary_search_algorithm
    int16_t l = 0;
    int16_t r = font->length - 1;
    
    while (l <= r) {
        uint8_t m = (l + r) / 2;
        const __flash Glyph *pglyph = &font->glyphs[m];
        if (pglyph->code < code) {
            l = m + 1;
        } else if (pglyph->code > code) {
            r = m - 1;
        } else {
            // found code point, return address of glyph
            return pglyph;
        }
    }

    // return question mark if unknown code point
    return getGlyphAddress(font, 0x003f);
}
