/*
 * thermidity.c
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2.
 *
 * AVR MCU based thermometer and hygrometer with e-ink display.
 *
 * Created on: 01.05.2023
 *     Author: torsten.roemer@luniks.net
 *
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "pins.h"
#include "meter.h"
#include "spi.h"
#include "sram.h"
#include "eink.h"
#include "display.h"
#include "utils.h"
#include "usart.h"

#define SECS_PER_BARK   8
#define MEASURE_BARKS   2 // 16 seconds
// display should not be updated more frequently than once every 180 seconds
#define DISP_UPD_BARKS  24 // 192 seconds

static volatile uint16_t barks = DISP_UPD_BARKS;

ISR(WDT_vect) {
    barks++;
}

EMPTY_INTERRUPT(ADC_vect);

/**
 * Sets up the pins.
 */
static void initPins(void) {
    // set MOSI and SCK as output pin
    DDR_SPI |= (1 << PIN_MOSI);
    DDR_SPI |= (1 << PIN_SCK);
    // pull SS (ensure master) and MISO high
    PORT_SPI |= (1 << PIN_SS);
    PORT_SPI |= (1 << PIN_MISO);

    // set SRAM CS pin as output pin
    DDR_SRDI |= (1 << PIN_SRCS);

    // set display CS, D/C and RST pin as output pin
    DDR_SRDI |= (1 << PIN_ECS);
    DDR_SRDI |= (1 << PIN_DC);
    DDR_SRDI |= (1 << PIN_RST);

    // enable pullup on all output pins
    PORT_SRDI |= (1 << PIN_SRCS);
    PORT_SRDI |= (1 << PIN_ECS);
    PORT_SRDI |= (1 << PIN_DC);
    PORT_SRDI |= (1 << PIN_RST);

    // set display BUSY pin as input pin
    DDR_SRDI &= ~(1 << PIN_BUSY);
}

/**
 * Enables SPI master mode.
 */
static void initSPI(void) {
    SPCR |= (1 << SPR0);
    SPCR |= (1 << MSTR);
    SPCR |= (1 << SPE);
}

/**
 * Sets up the watchdog.
 */
static void initWatchdog(void) {
    cli();
    wdt_reset();
    // watchdog change enable
    WDTCSR |= (1 << WDCE) | (1 << WDE);
    // disable system reset, enable interrupt, bark every 8 seconds
    WDTCSR = (1 << WDIE) | (0 << WDE) | (1 << WDP3) | (1 << WDP0);
}

/**
 * Sets up the ADC.
 */
static void initADC(void) {
    // disable digital input on the ADC inputs to reduce digital noise
    DIDR0 = 0b00111111;
    // ADC clock prescaler/64 ~ 125 kHz @ 8 MHz
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1);
    // enable ADC interrupt
    ADCSRA |= (1 << ADIE);
    // enable ADC
    ADCSRA |= (1 << ADEN);
}

int main(void) {
    
    initPins();
    initSPI();
    initWatchdog();
    initADC();
    initUSART();

    // enable global interrupts
    sei();
    
    while (true) {
        // measure and average temperature and relative humidity
        if (barks % MEASURE_BARKS == 0) {
            measureValues();

            // update display
            if (barks >= DISP_UPD_BARKS) {
                displayValues();
                barks = 0;
            }
        }
        
        wdt_reset();
        set_sleep_mode(SLEEP_MODE_PWR_SAVE);
        sleep_mode();
    }

    return 0;
}
