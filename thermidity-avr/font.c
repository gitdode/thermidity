/* 
 * File:   font.c
 * Author: torsten.roemer@luniks.net
 *
 * Created on 25. April 2023, 21:00
 */

#include <stdio.h>
#include <avr/pgmspace.h>
#include "font.h"

Glyph *getGlyph(Font *font, uint16_t code) {
    for (size_t i = 0; i < font->length; i++) {
        if (pgm_read_word(&font->glyphs[i].code) == code) {
            static Glyph glyph;
            memcpy_P(&glyph, &font->glyphs[i], sizeof (Glyph));
            
            return &glyph;
        }
    }
    
    // return question mark if unknown code point
    return getGlyph(font, 0x003f);
}