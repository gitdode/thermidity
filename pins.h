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

/* SPI pins */
#define DDR_SPI   DDRB
#define PORT_SPI  PORTB
#define PIN_SS    PB2
#define PIN_MOSI  PB3
#define PIN_MISO  PB4
#define PIN_SCK   PB5

/* Registers shared by SRAM and Display */
#define DDR_SRDI  DDRD
#define PORT_SRDI PORTD
#define PINP_SRDI PIND

/* SRAM pins */
#define PIN_SRCS  PD3 // SRAM chip select

/* Display pins */
#define PIN_ECS   PD5 // display chip select
#define PIN_DC    PD6 // display data/command
#define PIN_RST   PD7 // display reset
#define PIN_BUSY  PD4 // display busy

#endif /* PINS_H */

