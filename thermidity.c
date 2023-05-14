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

/* Timer0 interrupts per second */
#define INTS_SEC    F_CPU / (64UL * 255)

static volatile uint32_t ints = INTS_SEC * 175;

ISR(TIMER0_COMPA_vect) {
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
    // timer0 clear timer on compare match mode, TOP OCR0A
    TCCR0A |= (1 << WGM01);
    // timer0 clock prescaler/64/255 ~ 61 Hz @ 1 MHz ~ 490 Hz @ 8 Mhz
    TCCR0B |= (1 << CS01) | (1 << CS00);
    OCR0A = 255;

    // enable timer0 compare match A interrupt
    TIMSK0 |= (1 << OCIE0A);
}

/**
 * Sets up the ADC.
 */
static void initADC(void) {
    set_sleep_mode(SLEEP_MODE_IDLE);

    // use AVCC as reference voltage
    ADMUX |= (1 << REFS0);
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
    initTimer();
    initADC();
    initUSART();

    // enable global interrupts
    sei();

    while (true) {
        
        // measure and average temperature and relative humidity every 10 seconds
        if (ints % (INTS_SEC * 10) == 0) {
            measureValues();

            // display should not be updated more frequently than once every 180 seconds
            if (ints >= (INTS_SEC * 180)) {
                ints = 0;
                displayValues();            
            }
        }
    }

    return 0;
}
