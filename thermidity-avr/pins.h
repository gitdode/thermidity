/* 
 * File:   pins.h
 * Author: torsten.roemer@luniks.net
 *
 * Created on 1. April 2023, 19:25
 */

#ifndef PINS_H
#define PINS_H

#include <avr/io.h>

#define PIN_TMP   PC0
#define PIN_RH    PC1
#define PIN_BAT   PC2

/* SPI */
#define DDR_SPI   DDRB
#define PORT_SPI  PORTB
#define PIN_SS    PB2
#define PIN_MOSI  PB3
#define PIN_MISO  PB4
#define PIN_SCK   PB5

/* SRAM */
#define DDR_SSPI  DDRD
#define PORT_SSPI PORTD
#define PIN_SRCS  PD7 // SRAM chip select

/* Display */
#define DDR_DSPI  DDRB
#define PORT_DSPI PORTB
#define PIN_ECS   PB1 // display chip select
#define PIN_DC    PB0 // display data/command

#define DDR_DISP  DDRD
#define PORT_DISP PORTD
#define PINP_DISP PIND
#define PIN_RST   PD6 // display reset
#define PIN_BUSY  PD5 // display busy

#endif /* PINS_H */
