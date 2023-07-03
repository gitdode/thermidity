/* 
 * File:   eink.c
 * Author: torsten.roemer@luniks.net
 * Thanks to https://github.com/adafruit/Adafruit_EPD for helping me out!
 *
 * Created on 1. April 2023, 20:48
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include "eink.h"
#include "pins.h"
#include "sram.h"
#include "spi.h"

/**
 * Does a hardware reset.
 */
static void hwReset(void) {
    PORT_DISP &= ~(1 << PIN_RST);
    _delay_ms(10);
    PORT_DISP |= (1 << PIN_RST);
}

/**
 * Waits until the display is no longer busy.
 */
static void waitBusy(void) {
    loop_until_bit_is_clear(PINP_DISP, PIN_BUSY);
}

void displayCmd(uint8_t cmd) {
    PORT_DSPI &= ~(1 << PIN_DC);
    transmit(cmd);
}

void displayData(uint8_t data) {
    PORT_DSPI |= (1 << PIN_DC);
    transmit(data);
}

void initDisplay(void) {
    // 1. Power On
    // VCI already supplied - could supply by MCU output pin?
    // - Supply VCI
    // - Wait 10ms
    _delay_ms(10);

    // 2. Set Initial Configuration
    // board selects 4-wire SPI by pulling BS1 low
    // - Define SPI interface to communicate with MCU

    // - HW Reset
    hwReset();
    _delay_ms(100);
    waitBusy();

    // - SW Reset by Command 0x12
    displaySel();
    displayCmd(SW_RESET);
    displayDes();

    // - Wait 10ms
    waitBusy(); // datasheet mentions BUSY is high during reset
    _delay_ms(10);
    
    // 3. Send Initialization Code
    // - Set gate driver output by Command 0x01
    displaySel();
    displayCmd(DRIVER_OUTPUT_CONTROL);
    displayData(DISPLAY_WIDTH - 1);
    displayData((DISPLAY_WIDTH - 1) >> 8);
    displayData(0x00); // GD=0 [POR], SM=0 [POR], TB = 0 [POR]
    displayDes();
    
    // - Set display RAM size by Command 0x11, 0x44, 0x45
    displaySel();
    displayCmd(DATA_ENTRY_MODE_SETTING);
    displayData(0x03); // A[2:0] = 011 [POR]
    displayDes();
    
    displaySel();
    displayCmd(RAM_X_ADDRESS_POSITION);
    displayData(0x00 + RAM_X_OFFSET);
    displayData(DISPLAY_H_BYTES - 1 + RAM_X_OFFSET);
    displayDes();
    
    displaySel();
    displayCmd(RAM_Y_ADDRESS_POSITION);
    displayData(0x00);
    displayData(0x00);
    displayData(DISPLAY_WIDTH - 1);
    displayData((DISPLAY_WIDTH - 1) >> 8);
    displayDes();
    
    // - Set panel border by Command 0x3C
    displaySel();
    displayCmd(BORDER_WAVEFORM_CONTROL);
    displayData(0x05); // ?
    displayDes();

    // 4. Load Waveform LUT
    // - Sense temperature by int/ext TS by Command 0x18
    displaySel();
    displayCmd(TEMP_SENSOR_CONTROL);
    displayData(0x80); // A[7:0] = 80h Internal temperature sensor
    displayDes();
    
    // done at the end, wait for BUSY low anyway
    // - Load waveform LUT from OTP by Command 0x22, 0x20 or by MCU
    // - Wait BUSY Low
    waitBusy();
}

void resetAddressCounter(void) {
    // 5. Write Image and Drive Display Panel
    // - Write image data in RAM by Command 0x4E, 0x4F, 0x24, 0x26
    displaySel();
    displayCmd(RAM_X_ADDRESS_COUNTER);
    displayData(RAM_X_OFFSET);
    displayDes();
    
    displaySel();
    displayCmd(RAM_Y_ADDRESS_COUNTER);
    displayData(0);
    displayData(0);
    displayDes();
}

void imageWrite(uint8_t data) {
    // 5. Write Image and Drive Display Panel
    // - Write image data in RAM by Command 0x4E, 0x4F, 0x24, 0x26
    displaySel();
    displayCmd(WRITE_RAM_BW);
    displayData(data);
    displayDes();
}

void updateDisplay(void) {
    // - Set softstart setting by Command 0x0C
    displaySel();
    displayCmd(BOOSTER_SOFT_START_CONTROL);
    displayData(0x8b); // A[7:0] -> Soft start setting for Phase1 = 8Bh [POR]
    displayData(0x9c); // B[7:0] -> Soft start setting for Phase2 = 9Ch [POR]
    displayData(0x96); // C[7:0] -> Soft start setting for Phase3 = 96h [POR]
    displayData(0x0f); // D[7:0] -> Duration setting = 0Fh [POR]
    displayDes();
    
    // - Drive display panel by Command 0x22, 0x20
    // 0xf4, 0xf5, 0xf6, 0xf7 do full update (DISPLAY mode 1)
    // 0xfc, 0xfd, 0xfe, 0xff do partial update (DISPLAY mode 2)
    // fast update as mentioned in the Good Display data sheet is not supported?
    displaySel();
    displayCmd(DISPLAY_UPDATE_CONTROL2);
    displayData(0xf4);
    displayDes();
    
    displaySel();
    displayCmd(MASTER_ACTIVATION);
    displayDes();
    
    // - Wait BUSY Low
    waitBusy();

    // 6. Power Off
    // - Deep sleep by Command 0x10
    displaySel();
    displayCmd(DEEP_SLEEP_MODE);
    displayData(0x11); // Deep Sleep Mode 2 (no need to retain RAM data)
    displayDes();
    
    // - Power OFF
    // see 1. Power On
}