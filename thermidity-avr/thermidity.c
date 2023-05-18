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

/* Timer2 interrupts per second */
#define INTS_SEC    F_CPU / (1024UL * 255)

static volatile uint32_t ints = INTS_SEC * 191;

ISR(TIMER2_COMPA_vect) {
    ints++;
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
 * Sets up the timer.
 */
static void initTimer(void) {
    // timer2 clear timer on compare match mode, TOP OCR2A
    TCCR2A |= (1 << WGM21);
    // timer2 clock prescaler/1024/255 ~ 31 Hz @ 8 Mhz
    TCCR2B |= (1<<CS22) | (1<<CS21) | (1<<CS20);
    OCR2A = 255;

    // enable timer2 compare match A interrupt
    TIMSK2 |= (1 << OCIE2A);
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
    initTimer();
    initADC();
    // initUSART();

    // enable global interrupts
    sei();

    while (true) {

        // measure and average temperature and relative humidity every 16 seconds
        if (ints % (INTS_SEC * 16) == 0) {
            measureValues();

            // display should not be updated more frequently than once every 180 seconds
            if (ints >= (INTS_SEC * 192)) {
                ints = 0;
                displayValues();            
            }            
        }
        
        set_sleep_mode(SLEEP_MODE_PWR_SAVE);
        sleep_mode();
    }

    return 0;
}
