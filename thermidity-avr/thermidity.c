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
#include <util/delay.h>
#include <util/atomic.h>

#include "pins.h"
#include "meter.h"
#include "spi.h"
#include "sram.h"
#include "eink.h"
#include "display.h"
#include "utils.h"
#include "usart.h"

/* Measure and average temperature and relative humidity every ~32 seconds */
#define MEASURE_SECS    32 // should be a power of 2 to avoid division 
/* Display should not be updated more frequently than once every 180 seconds */
#define DISP_UPD_SECS   MEASURE_SECS * 9
/* Number of fast updates until a full update is done to avoid ghosting */
#define DISP_MAX_FAST   9

static volatile uint16_t secs = DISP_UPD_SECS - 1;

static uint8_t updates = DISP_MAX_FAST + 1;

ISR(WDT_vect) {
    secs++;
}

EMPTY_INTERRUPT(ADC_vect);

/**
 * Sets up the pins.
 */
static void initPins(void) {
    // set sensor power pin as output pin
    DDR_SENS |= (1 << PIN_PWR);
    // drive sensor power pin low
    PORT_SENS &= ~(1 << PIN_PWR);

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

    // drive SPI and display output pins high
    PORT_SSPI |= (1 << PIN_SRCS);
    PORT_DSPI |= (1 << PIN_ECS);
    PORT_DSPI |= (1 << PIN_DC);
    PORT_DISP |= (1 << PIN_RST);

    // set display BUSY pin as input pin (default)
    DDR_DISP &= ~(1 << PIN_BUSY);

    // pull all unused pins high/set to defined level to reduce current
    // consumption when not in sleep mode
    PORTB |= (1 << PB6);
    PORTB |= (1 << PB7);
    PORTC |= (1 << PC4);
    PORTC |= (1 << PC5);
    PORTD |= (1 << PD0);
    PORTD |= (1 << PD1);
    PORTD |= (1 << PD2);
    PORTD |= (1 << PD3);
    PORTD |= (1 << PD4);
}

/**
 * Enables SPI master mode.
 */
static void initSPI(void) {
    SPCR |= (1 << SPR0);
    SPCR |= (1 << MSTR);
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
 * Powers on the sensors.
 */
static void powerOnSensors(void) {
    PORT_SENS |= (1 << PIN_PWR);
}

/**
 * Powers off the sensors.
 */
static void powerOffSensors(void) {
    PORT_SENS &= ~(1 << PIN_PWR);
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
 * Enables SPI.
 */
static void enableSPI(void) {
    SPCR |= (1 << SPE);
}

/**
 * Disables SPI.
 */
static void disableSPI(void) {
    SPCR &= ~(1 << SPE);
    PORT_SPI &= ~(1 << PIN_SCK);
}

/**
 * Stops the clock of unused modules to reduce power consumption.
 */
static void reducePower(void) {
    // stop TWI, Timer0, Timer1, Timer2 and USART
    PRR |= (1 << PRTWI) | (1 << PRTIM0) | (1 << PRTIM1) | (1 << PRTIM2) | (1 << PRUSART0);
}

/**
 * Disables global interrupts and the watchdog to stop measuring 
 * and updating the display when batteries are too weak, to limit 
 * discharging below cutoff voltage.
 */
static void powerDown(void) {
    cli();
    wdt_disable();
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

    uint16_t secsCopy;
    while (true) {
        ATOMIC_BLOCK(ATOMIC_FORCEON) {
            secsCopy = secs;
        }

        if (secsCopy % MEASURE_SECS == 0) {
            powerOnSensors();
            // give the humidity sensor time to settle
            _delay_ms(100);
            enableADC();
            measureValues();
            disableADC();
            powerOffSensors();

            if (secsCopy >= DISP_UPD_SECS) {
                ATOMIC_BLOCK(ATOMIC_FORCEON) {
                    secs = 0;
                }

                // measured battery voltage is /5 by voltage divider
                if (getMVBat() < BAT_LOW / 5) {
                    powerDown();
                } else {
                    enableSPI();
                    if (updates > DISP_MAX_FAST) {
                        // make a full update after a certain number of fast 
                        // updates to avoid ghosting
                        displayValues(false);
                        updates = 0;
                    } else {
                        if (displayValues(true)) {
                            updates++;
                        }
                    }
                    disableSPI();
                }
            }
        }

        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_mode();
    }

    return 0;
}
