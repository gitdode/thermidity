/* 
 * File:   meter.c
 * Author: torsten.roemer@luniks.net
 *
 * Created on 1. Mai 2023, 23:43
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include "meter.h"
#include "pins.h"
#include "font.h"
#include "unifont.h"
#include "dejavu.h"
#include "bitmaps.h"
#include "display.h"
#include "utils.h"

static uint32_t avgADCTmp = -1;
static uint32_t avgADCRh = -1;
static uint32_t avgMVBat = -1;

static int16_t prevTmpx10;
static int16_t prevRh;
static int8_t  prevVBatx10;

/*
 * Converts ratiometric or absolute with the given reference voltage at the 
 * given pin with 16x oversampling during ADC noise reduction mode to reduce 
 * digital noise, updates the given exponential weighted moving average and 
 * returns it.
 */
static uint32_t convert(uint8_t aref, uint8_t pin, bool ratio, uint32_t avg) {
    ADMUX = 0x00 | aref | pin;
    set_sleep_mode(SLEEP_MODE_ADC);

    uint16_t over = 0;
    for (uint8_t i = 0; i < 17; i++) {
        ADCSRA |= (1 << ADSC);
        sleep_mode();
        // discard first conversion result after switching reference voltage
        if (i > 0) over += ADC;
    }

    uint32_t val = (over >> 2);    
    if (!ratio) {        
        val = (val * AREF_MV) >> 12;
    }
    
    if (avg == -1) {
        // use first measurement as initial value for average
        return val << EWMA_BS;
    } else {
        return val + avg - (avg >> EWMA_BS);
    }
}

/**
 * Formats the given temperature value multiplied by 10 and returns it.
 * @param tmpx10
 * @return string
 */
static char * formatTmp(int16_t tmpx10) {
    if (tmpx10 > 999) {
        return "+99.9°";
    }
    
    div_t tmp = div(tmpx10, 10);
    static char buf[15];
    snprintf(buf, sizeof (buf), "%3d.%d°", tmp.quot, abs(tmp.rem));
    
    return buf;
}

/**
 * Formats the given relative humidity value and returns it.
 * @param rh
 * @return string
 */
static char * formatRh(int16_t rh) {
    if (rh > 99) {
        return "+99%";
    }
    
    static char buf[8];
    snprintf(buf, sizeof (buf), "%3d%%", rh);
    
    return buf;
}

/**
 * Returns the bitmap index for the given battery voltage in V multiplied by 10.
 * @param vBatx10
 * @return index
 */
static uint8_t bitmapBat(int8_t vBatx10) {    
    if (vBatx10 < 31) return BAT_0PCT;
    if (vBatx10 < 33) return BAT_13PCT;
    if (vBatx10 < 35) return BAT_25PCT;
    if (vBatx10 < 37) return BAT_38PCT;
    if (vBatx10 < 39) return BAT_50PCT;
    if (vBatx10 < 41) return BAT_63PCT;
    if (vBatx10 < 43) return BAT_75PCT;
    if (vBatx10 < 45) return BAT_88PCT;
    return BAT_100PCT;
}

/**
 * Formats the given battery voltage in V multiplied by 10 and returns it.
 * @param vBatx10
 * @return string
 */
static char * formatBat(int16_t vBatx10) {
    div_t v = div(vBatx10, 10);
    static char buf[7];
    snprintf(buf, sizeof (buf), "%d.%dV", v.quot, v.rem);
    
    return buf;
}

int16_t getMVBat(void) {
    return (avgMVBat >> EWMA_BS);
}

void measureValues(void) {
    avgADCTmp = convert(AREF_AVCC, PIN_TMP, true, avgADCTmp);
    avgADCRh = convert(AREF_AVCC, PIN_RH, true, avgADCRh);
    // give the capacitor between AREF and GND some time to discharge
    _delay_us(150);
    avgMVBat = convert(AREF_INT, PIN_BAT, false, avgMVBat);
}

bool displayValues(bool fast) {    
    // resistance of the thermistor
    float resTh = (4096.0 / fmax(1, (avgADCTmp >> EWMA_BS)) - 1) * TH_SERI;
    // temperature in °C
    float tmp = 1.0 / (1.0 / TH_BETA * log(resTh / TH_RESI) + 1.0 / TH_TEMP) - TMP_0C;
    // somehow I don't like to work with floats
    int16_t tmpx10 = tmp * 10;
    
    // relative humidity in %
    int32_t rh = divRoundNearest(((avgADCRh >> EWMA_BS) - RH_ADC_0) * 100, RH_ADC);    
    // temperature compensation of relative humidity
    rh = divRoundNearest(rh * 1000000, 1054600 - tmpx10 * 216UL);
    
    // battery voltage in V x10 (measured one fifth by voltage divider)
    int8_t vBatx10 = divRoundNearest((avgMVBat >> EWMA_BS), 20);
    
    if (tmpx10 == prevTmpx10 && rh == prevRh && vBatx10 == prevVBatx10) {
        // skip update of display if no change in measurements
        return false;
    }
    
    prevTmpx10 = tmpx10;
    prevRh = rh;
    prevVBatx10 = vBatx10;
    
    Font *unifont = getUnifont();
    Font *dejavu = getDejaVu();
    
    // clear frame
    setFrame(0x00);
    // battery voltage and bitmap
    writeString(0, 182, unifont, formatBat(vBatx10));
    writeBitmap(0, 216, bitmapBat(vBatx10));
    // temperature with label
    writeString(1, 0, dejavu, formatTmp(tmpx10));
    writeString(5, 144, unifont, "Temperature");
    // humidity with label
    writeString(8, 0, dejavu, formatRh(rh));
    writeString(12, 144, unifont, "Humidity");
    // update display
    doDisplay(fast);
    
    return true;
}
