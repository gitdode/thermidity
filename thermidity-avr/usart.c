/*
 * usart.c
 *
 *  Created on: 19.05.2015
 *      Author: dode@luniks.net
 */

#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/setbaud.h>
#include "usart.h"
#include "utils.h"

static volatile bool usartReceived = false;

static char usartData[USART_LENGTH];

/**
 * Called when data was received via USART.
 */
ISR(USART_RX_vect) {
    if (bit_is_set(UCSR0A, RXC0) && !usartReceived) {
        char data = UDR0;
        size_t length = strlen(usartData);
        if (length < USART_LENGTH - 1 && data != '\n' && data != '\r') {
            usartData[length] = data;
        } else {
            usartData[length] = '\0';
            usartReceived = true;
        }
    }
}

void initUSART(void) {
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;

    UCSR0B = (1 << TXEN0) | (1 << RXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
    
    // enable USART RX complete interrupt 0
    UCSR0B |= (1 << RXCIE0);
}

bool isUSARTReceived(void) {
    return usartReceived;
}

void getUSARTData(char *data, size_t size) {
    if (size > 0) {
        data[0] = '\0';
        strncat(data, usartData, size - 1);
        memset(usartData, 0, USART_LENGTH);
        usartReceived = false;
    }
}

void printString(const char *data) {
    uint8_t i = 0;
    char c;
    while ((c = data[i++]) != '\0') {
        loop_until_bit_is_set(UCSR0A, UDRE0);
        UDR0 = c;
    }
}

void printUint(uint8_t data) {
    char buf[6];
    snprintf(buf, sizeof (buf), "%d\r\n", data);
    printString(buf);
}

void printByte(uint8_t byte) {
    char string[] = {'0', 'b', '?', '?', '?', '?', '?', '?', '?', '?', '\r', '\n', '\0'};
    for (uint8_t i = 8; i-- > 0; ) {
        string[9 - i] = byte & (1 << i) ? '1' : '0';
    }
    printString(string);
}