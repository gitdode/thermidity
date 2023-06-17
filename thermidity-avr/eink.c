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

void displayCmd(void) {
    PORT_DSPI &= ~(1 << PIN_DC);
}

void displayData(void) {
    PORT_DSPI |= (1 << PIN_DC);
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
    displayCmd();
    displaySel();
    transmit(SW_RESET);
    displayDes();

    // - Wait 10ms
    waitBusy(); // datasheet mentions BUSY is high during reset
    _delay_ms(10);
    
    // 3. Send Initialization Code
    // - Set gate driver output by Command 0x01
    displayCmd();
    displaySel();
    transmit(DRIVER_OUTPUT_CONTROL);
    displayData();
    transmit(DISPLAY_WIDTH - 1);
    transmit((DISPLAY_WIDTH - 1) >> 8);
    transmit(0x00); // GD=0 [POR], SM=0 [POR], TB = 0 [POR]
    displayDes();
    
    // - Set display RAM size by Command 0x11, 0x44, 0x45
    displayCmd();
    displaySel();
    transmit(DATA_ENTRY_MODE_SETTING);
    displayData();
    transmit(0x03); // A[2:0] = 011 [POR]
    displayDes();
    
    displayCmd();
    displaySel();
    transmit(RAM_X_ADDRESS_POSITION);
    displayData();
    transmit(0x00 + RAM_X_OFFSET);
    transmit(DISPLAY_H_BYTES - 1 + RAM_X_OFFSET);
    displayDes();
    
    displayCmd();
    displaySel();
    transmit(RAM_Y_ADDRESS_POSITION);
    displayData();
    transmit(0x00);
    transmit(0x00);
    transmit(DISPLAY_WIDTH - 1);
    transmit((DISPLAY_WIDTH - 1) >> 8);
    displayDes();
    
    // - Set panel border by Command 0x3C
    displayCmd();
    displaySel();
    transmit(BORDER_WAVEFORM_CONTROL);
    displayData();
    transmit(0x05); // ?
    displayDes();

    // 4. Load Waveform LUT
    // - Sense temperature by int/ext TS by Command 0x18
    displayCmd();
    displaySel();
    transmit(TEMP_SENSOR_CONTROL);
    displayData();
    transmit(0x80); // A[7:0] = 80h Internal temperature sensor
    displayDes();
    
    // done at the end, wait for BUSY low anyway
    // - Load waveform LUT from OTP by Command 0x22, 0x20 or by MCU
    // - Wait BUSY Low
    waitBusy();
}

void resetAddressCounter(void) {
    // 5. Write Image and Drive Display Panel
    // - Write image data in RAM by Command 0x4E, 0x4F, 0x24, 0x26
    displayCmd();
    displaySel();
    transmit(RAM_X_ADDRESS_COUNTER);
    displayData();
    transmit(RAM_X_OFFSET);
    displayDes();
    
    displayCmd();
    displaySel();
    transmit(RAM_Y_ADDRESS_COUNTER);
    displayData();
    transmit(0);
    transmit(0);
    displayDes();
}

void imageWrite(uint8_t data) {
    // 5. Write Image and Drive Display Panel
    // - Write image data in RAM by Command 0x4E, 0x4F, 0x24, 0x26
    displayCmd();
    displaySel();
    transmit(WRITE_RAM_BW);
    displayData();
    transmit(data);
    displayDes();
}

void updateDisplay(void) {
    // - Set softstart setting by Command 0x0C
    displayCmd();
    displaySel();
    transmit(BOOSTER_SOFT_START_CONTROL);
    displayData();
    transmit(0x8b); // A[7:0] -> Soft start setting for Phase1 = 8Bh [POR]
    transmit(0x9c); // B[7:0] -> Soft start setting for Phase2 = 9Ch [POR]
    transmit(0x96); // C[7:0] -> Soft start setting for Phase3 = 96h [POR]
    transmit(0x0f); // D[7:0] -> Duration setting = 0Fh [POR]
    displayDes();
    
    // - Drive display panel by Command 0x22, 0x20
    displayCmd();
    displaySel();
    transmit(DISPLAY_UPDATE_CONTROL2);
    displayData();
    transmit(0xf4); // 0xf4, 0xf5, 0xf6, 0xf7 seem to do the same
    displayDes();
    
    displayCmd();
    displaySel();
    transmit(MASTER_ACTIVATION);
    displayDes();
    
    // - Wait BUSY Low
    waitBusy();

    // 6. Power Off
    // - Deep sleep by Command 0x10
    displayCmd();
    displaySel();
    transmit(DEEP_SLEEP_MODE);
    displayData();
    transmit(0x01); // Enter Deep Sleep Mode 1
    displayDes();
    
    // - Power OFF
    // see 1. Power On
}