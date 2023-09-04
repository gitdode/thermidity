/* 
 * File:   font.c
 * Author: torsten.roemer@luniks.net
 *
 * Created on 25. April 2023, 21:00
 */

#include <stdio.h>
#include <avr/pgmspace.h>
#include "font.h"

Glyph getGlyph(Font *font, uint16_t code) {
    
    // https://en.wikipedia.org/wiki/Binary_search_algorithm
    int16_t l = 0;
    int16_t r = font->length - 1;
    
    while (l <= r) {
        uint8_t m = (l + r) / 2;
        uint16_t c = pgm_read_word(&font->glyphs[m].code);
        if (c < code) {
            l = m + 1;
        } else if (c > code) {
            r = m - 1;
        } else {
            // found code point, get and return glyph
            static Glyph glyph;
            memcpy_P(&glyph, &font->glyphs[m], sizeof (Glyph));
            
            return glyph;
        }
    }
    
    // return question mark if unknown code point
    return getGlyph(font, 0x003f);
}