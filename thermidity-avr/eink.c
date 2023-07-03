/* 
 * File:   eink.c
 * Author: torsten.roemer@luniks.net
 * Thanks to https://github.com/adafruit/Adafruit_EPD for helping me out!
 *
 * Created on 1. April 2023, 20:48
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
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

void initDisplay(bool fast) {
    // 1. Power On
    // VCI already supplied - could supply by MCU output pin?
    // - Supply VCI
    // - Wait 10ms
    _delay_ms(10);
    
    displaySel();

    // 2. Set Initial Configuration
    // board selects 4-wire SPI by pulling BS1 low
    // - Define SPI interface to communicate with MCU

    // - HW Reset
    hwReset();
    _delay_ms(100);
    waitBusy();

    // - SW Reset by Command 0x12
    displayCmd(SW_RESET);

    // - Wait 10ms
    waitBusy(); // datasheet mentions BUSY is high during reset
    _delay_ms(10);
    
    // 3. Send Initialization Code
    // - Set gate driver output by Command 0x01
    displayCmd(DRIVER_OUTPUT_CONTROL);
    displayData(DISPLAY_WIDTH - 1);
    displayData((DISPLAY_WIDTH - 1) >> 8);
    displayData(0x00); // GD=0 [POR], SM=0 [POR], TB = 0 [POR]
    
    // - Set display RAM size by Command 0x11, 0x44, 0x45
    displayCmd(DATA_ENTRY_MODE_SETTING);
    displayData(0x03); // A[2:0] = 011 [POR]
    
    displayCmd(RAM_X_ADDRESS_POSITION);
    displayData(0x00 + RAM_X_OFFSET);
    displayData(DISPLAY_H_BYTES - 1 + RAM_X_OFFSET);
    
    displayCmd(RAM_Y_ADDRESS_POSITION);
    displayData(0x00);
    displayData(0x00);
    displayData(DISPLAY_WIDTH - 1);
    displayData((DISPLAY_WIDTH - 1) >> 8);
    
    // - Set panel border by Command 0x3C
    displayCmd(BORDER_WAVEFORM_CONTROL);
    displayData(0x05); // ?

    // 4. Load Waveform LUT
    // - Sense temperature by int/ext TS by Command 0x18
    displayCmd(TEMP_SENSOR_CONTROL);
    displayData(0x80); // A[7:0] = 80h Internal temperature sensor
    
    if (fast) {
        // Load temperature value
        displayCmd(DISPLAY_UPDATE_CONTROL2);
        displayData(0xb1);
        displayCmd(MASTER_ACTIVATION);
        waitBusy();

        // Write temperature value
        displayCmd(WRITE_TO_TEMP_REGISTER);
        displayData(0x64);
        displayData(0x00);

        // Load temperature value
        displayCmd(DISPLAY_UPDATE_CONTROL2);
        displayData(0x91);
        displayCmd(MASTER_ACTIVATION);
        waitBusy();
    }
    
    // done at the end, wait for BUSY low anyway
    // - Load waveform LUT from OTP by Command 0x22, 0x20 or by MCU
    // - Wait BUSY Low
    waitBusy();
    
    displayDes();
}

void resetAddressCounter(void) {
    displaySel();
    
    // 5. Write Image and Drive Display Panel
    // - Write image data in RAM by Command 0x4E, 0x4F, 0x24, 0x26
    displayCmd(RAM_X_ADDRESS_COUNTER);
    displayData(RAM_X_OFFSET);
    
    displayCmd(RAM_Y_ADDRESS_COUNTER);
    displayData(0);
    displayData(0);
    
    displayDes();
}

void imageWrite(uint8_t data) {
    displaySel();
    
    // 5. Write Image and Drive Display Panel
    // - Write image data in RAM by Command 0x4E, 0x4F, 0x24, 0x26
    displayCmd(WRITE_RAM_BW);
    displayData(data);
    
    displayDes();
}

void updateDisplay(bool fast) {
    displaySel();
    
    // - Set softstart setting by Command 0x0C
    /*
    displaySel();
    displayCmd(BOOSTER_SOFT_START_CONTROL);
    displayData(0x8b); // A[7:0] -> Soft start setting for Phase1 = 8Bh [POR]
    displayData(0x9c); // B[7:0] -> Soft start setting for Phase2 = 9Ch [POR]
    displayData(0x96); // C[7:0] -> Soft start setting for Phase3 = 96h [POR]
    displayData(0x0f); // D[7:0] -> Duration setting = 0Fh [POR]
    displayDes();
     */
    
    // - Drive display panel by Command 0x22, 0x20
    // 0xf4, 0xf5, 0xf6, 0xf7 do full update (DISPLAY mode 1)
    // 0xfc, 0xfd, 0xfe, 0xff do partial update (DISPLAY mode 2)
    // 0xc7 does fast update
    displayCmd(DISPLAY_UPDATE_CONTROL2);
    displayData(fast ? 0xc7 : 0xf4);
    displayCmd(MASTER_ACTIVATION);
    
    // - Wait BUSY Low
    waitBusy();

    // 6. Power Off
    // - Deep sleep by Command 0x10
    displayCmd(DEEP_SLEEP_MODE);
    displayData(0x11); // Deep Sleep Mode 2 (no need to retain RAM data)
    
    // - Power OFF
    // see 1. Power On
    
    displayDes();
}