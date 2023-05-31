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

/* Measure and average temperature and relative humidity every 16 seconds */
#define MEASURE_SECS    16
/* Display should not be updated more frequently than once every 180 seconds */
#define DISP_UPD_SECS   192

static volatile uint16_t secs = DISP_UPD_SECS - 1;

ISR(WDT_vect) {
    secs++;
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
    DDR_SSPI |= (1 << PIN_SRCS);

    // set display CS, D/C and RST pin as output pin
    DDR_DSPI |= (1 << PIN_ECS);
    DDR_DSPI |= (1 << PIN_DC);
    DDR_DISP |= (1 << PIN_RST);

    // enable pullup on all output pins
    PORT_SSPI |= (1 << PIN_SRCS);
    DDR_DSPI |= (1 << PIN_ECS);
    DDR_DSPI |= (1 << PIN_DC);
    PORT_DISP |= (1 << PIN_RST);

    // set display BUSY pin as input pin
    PORT_DISP &= ~(1 << PIN_BUSY);
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
    // enable interrupt, disable system reset, bark every 1 seconds
    WDTCSR = (1 << WDIE) | (0 << WDE) | (1 << WDP2) | (1 << WDP1);
}

/**
 * Sets up the ADC.
 */
static void initADC(void) {
    // disable digital input on the ADC inputs to reduce digital noise
    DIDR0 = 0b00111111;
    // ADC clock prescaler/64 ~ 125kHz @ 8MHz
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1);
    // enable ADC interrupt
    ADCSRA |= (1 << ADIE);
}

/**
 * Enables the ADC.
 */
static void enableADC(void) {
    ADCSRA |= (1 << ADEN);
}

/**
 * Disables the ADC.
 */
static void disableADC(void) {
    ADCSRA &= ~(1 << ADEN);
}

/**
 * Stops the clock of unused modules to reduce power consumption.
 */
static void reducePower(void) {
    // stop TWI, Timer0, Timer1 and USART
    PRR |= (1 << PRTWI) | (1 << PRTIM0) | (1 << PRTIM1) | (1 << PRUSART0);
}

int main(void) {

    reducePower();
    initPins();
    initSPI();
    initWatchdog();
    initADC();
    // initUSART();

    // enable global interrupts
    sei();

    while (true) {
        if (secs % MEASURE_SECS == 0) {
            enableADC();
            // give the ADC some time after sleep mode, 
            // otherwise the first measurement will be quite off.  
            _delay_ms(1);
            measureValues();
            // disable ADC before entering sleep mode to save power
            disableADC();

            if (secs >= DISP_UPD_SECS) {
                secs = 0;
                displayValues();
            }
        }
                
        set_sleep_mode(SLEEP_MODE_PWR_SAVE);
        sleep_mode();
    }

    return 0;
}
