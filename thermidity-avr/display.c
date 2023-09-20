/* 
 * File:   display.h
 * Author: torsten.roemer@luniks.net
 *
 * Created on 18. April 2023, 21:56
 */

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "display.h"
#include "unifont.h"
#include "dejavu.h"
#include "bitmaps.h"
#include "spi.h"
#include "sram.h"
#include "eink.h"
#include "utils.h"

/**
 * Writes the given byte at the given index for the given bitmap height
 * to its address/location. 
 * @param index
 * @param address
 * @param height
 * @param byte
 */
static void bufferByte(uint16_t index, uint16_t index_mod_height, 
                       uint16_t *address, height_t height, uint8_t byte) {
    // if (index % height == 0) {
    if (index_mod_height == 0) {
        if (index > 0) {
            *address -= height / 8 - 1;
        }
    } else if (index % 8 == 0) {
        *address += 8 * DISPLAY_H_BYTES + 1;
    }

    sramWrite(*address, byte);
    *address -= DISPLAY_H_BYTES;
}

/**
 * Writes the given bitmap stored in program memory with the given width  
 * and height to the given row and column to SRAM. Width and height must be 
 * multiples of 8.
 * @param row (8 pixels)
 * @param col (1 pixel)
 * @param bitmap
 * @param width
 * @param height
 */
static void bufferBitmap(row_t row, col_t col,
                         const __flash uint8_t *bitmap,
                         width_t width, height_t height) {
    uint16_t size = width * height / 8;
    uint16_t origin = DISPLAY_WIDTH * DISPLAY_H_BYTES + row - col * DISPLAY_H_BYTES;

    // rotate each 8 x 8 pixel 90° clockwise and flip horizontally
    uint8_t rotated[8];
    memset(rotated, 0, 8);
    uint16_t n = 0, x = 0;
    uint16_t i_div_height = 0;
    height_t i_mod_height = 0, x_mod_height = 0;
    for (uint16_t i = 0; i < size; i++) {
        uint8_t next = bitmap[n];
                
        // read bytes column by column
        n += width / 8;
        // if ((i + 1) % height == 0) {
        if (i_mod_height == height - 1) {
            // n = i / height + 1;
            n = i_div_height + 1;
        }
        
        if (++i_mod_height == height) {
            i_mod_height = 0;
            i_div_height += 1;
        }

        // rotate 8 x 8 pixel
        uint16_t m = i / 8 * 8;
        for (uint8_t r = 0; r < 8; r++) {
            uint8_t bit = (next & (1 << (7 - r))) ? 1 : 0;
            rotated[r] |= bit << (7 - i + m);
        }

        // buffer 8 x 8 rotated pixel
        if ((i + 1) % 8 == 0) {
            for (uint8_t r = 0; r < 8; r++) {
                bufferByte(x, x_mod_height, &origin, height, rotated[r]);
                x++;
                
                if (++x_mod_height == height) {
                    x_mod_height = 0;
                }
            }
            memset(rotated, 0, 8);
        }
    }
}

void sramToDisplay(void) {
    uint16_t bytes = DISPLAY_WIDTH * DISPLAY_H_BYTES;
    
    sramWriteStatus(SRAM_SEQU);
    
    sramSel();
    sramInitRead(0x0);
    
    displaySel();
    displaySetCmd();
    uint8_t byte = transmit(WRITE_RAM_BW);
    displaySetData();
    for (uint16_t i = 0; i < bytes; i++) {
        // remove negation for dark mode :)
        byte = transmit(~byte);
    }
    displayDes();
    
    sramDes();
    sramWriteStatus(SRAM_BYTE);
}

void setFrame(uint8_t byte) {
    uint16_t bytes = DISPLAY_WIDTH * DISPLAY_H_BYTES;
    
    sramWriteStatus(SRAM_SEQU);
    
    sramSel();
    sramInitWrite(0x0);
    for (uint16_t i = 0; i < bytes; i++) {
        transmit(byte);
    }
    sramDes();
    sramWriteStatus(SRAM_BYTE);
}

width_t writeBitmap(row_t row, col_t col, uint16_t index) {
    const __flash Bitmap *bitmap = &bitmaps[index];
    bufferBitmap (row, col, bitmap->bitmap, bitmap->width, bitmap->height);
    
    return bitmap->width;
}

width_t writeGlyph(row_t row, col_t col, const __flash Font *font, code_t code) {
    const __flash Glyph *glyph = getGlyphAddress(font, code);
    bufferBitmap(row, col, glyph->bitmap, glyph->width, font->height);
    
    return glyph->width;
}

void writeString(row_t row, col_t col, const __flash Font *font, char *string) {
    uint8_t offset = 0;
    for (; *string != '\0'; string++) {
        uint8_t c = (uint8_t) *string;
        if (c == 194) {
            // multibyte
        } else if (c == 195) {
            // multibyte, add 64 to get code point
            offset = 64;
        } else {
            code_t code = c + offset;
            col += writeGlyph(row, col, font, code);
            offset = 0;
        }
    }
}

void doDisplay(bool fast) {
    initDisplay(fast);
    resetAddressCounter();
    sramToDisplay();
    updateDisplay(fast);
}
