/* 
 * File:   eink.h
 * Author: torsten.roemer@luniks.net
 *
 * Created on 1. April 2023, 20:48
 */

#ifndef EINK_H
#define EINK_H

#define DRIVER_OUTPUT_CONTROL       0x01
#define BOOSTER_SOFT_START_CONTROL  0x0c
#define DEEP_SLEEP_MODE             0x10
#define DATA_ENTRY_MODE_SETTING     0x11
#define SW_RESET                    0x12
#define TEMP_SENSOR_CONTROL         0x18
#define MASTER_ACTIVATION           0x20
#define DISPLAY_UPDATE_CONTROL2     0x22
#define WRITE_RAM_BW                0x24
#define WRITE_RAM_RED               0x26
#define BORDER_WAVEFORM_CONTROL     0x3c
#define RAM_X_ADDRESS_POSITION      0x44
#define RAM_Y_ADDRESS_POSITION      0x45
#define RAM_X_ADDRESS_COUNTER       0x4e
#define RAM_Y_ADDRESS_COUNTER       0x4f

#define DISPLAY_WIDTH   250
#define DISPLAY_HEIGHT  122
#define DISPLAY_H_BYTES ((DISPLAY_HEIGHT + 8 - DISPLAY_HEIGHT % 8) >> 3)
#define RAM_X_OFFSET    1

/**
 * Send the given command to the display.
 */
void displayCmd(uint8_t cmd);

/**
 * Send the given data to the display.
 */
void displayData(uint8_t data);

/**
 * Resets the display and initializes it.
 */
void initDisplay(void);

/**
 * Sets the RAM address pointer to the starting position.
 */
void resetAddressCounter(void);

/**
 * Appends the given 8 pixels (0 = Black, 1 = White) to display RAM.
 * @param data
 */
void imageWrite(uint8_t data);

/**
 * Updates the display and puts it in deep sleep mode.
 */
void updateDisplay(void);

#endif /* EINK_H */

