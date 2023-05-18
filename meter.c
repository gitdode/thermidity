/* 
 * File:   meter.c
 * Author: torsten.roemer@luniks.net
 *
 * Created on 1. Mai 2023, 23:43
 */

#include <stdio.h>
#include <stdlib.h>
#include <avr/sleep.h>
#include "meter.h"
#include "pins.h"
#include "font.h"
#include "unifont.h"
#include "dejavu.h"
#include "bitmaps.h"
#include "display.h"
#include "usart.h"
#include "utils.h"

static uint32_t mVAvgTmp = -1;
static uint32_t mvAvgRh = -1;
static uint32_t mvAvgBat = -1;

static int16_t prevTmpx10;
static int16_t prevRh;
static int8_t  prevVBatx10;

/*
 * Converts the voltage at the given pin with 16x oversampling during
 * idle sleep mode to reduce digital noise, updates the given exponential
 * weighted moving average and returns it.
 */
static uint32_t convert(uint8_t pin, uint32_t mVAvg) {
    ADMUX = (0xf0 & ADMUX) | pin;
    set_sleep_mode(SLEEP_MODE_IDLE);

    uint32_t overValue = 0;
    for (uint8_t i = 0; i < 16; i++) {
        ADCSRA |= (1 << ADSC);
        sleep_mode();
        overValue += ADC;
    }

    uint32_t mV = ((overValue >> 2) * AREF_MV) >> 12;
    
    if (mVAvg == -1) {
        // use first measurement as initial value for average
        return mV << EWMA_BS;
    } else {
        return mV + mVAvg - (mVAvg >> EWMA_BS);
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
    static char buf[9];
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
    
    static char buf[6];
    snprintf(buf, sizeof (buf), "%3d%%", rh);
    
    return buf;
}

/**
 * Returns the bitmap index for the given battery voltage in V multiplied by 10.
 * @param vBatx10
 * @return index
 */
static uint8_t bitmapBat(int8_t vBatx10) {
    if (vBatx10 < 33) return BAT_0PCT;
    if (vBatx10 < 36) return BAT_25PCT;
    if (vBatx10 < 39) return BAT_50PCT;
    if (vBatx10 < 42) return BAT_75PCT;
    return BAT_100PCT;
}

/**
 * Formats the given battery voltage in mV and returns it.
 * @param mVBat
 * @return string
 */
static char * formatBat(int16_t mVBat) {
    uint8_t vx10 = divRoundNearest(mVBat, 100);
    div_t v = div(vx10, 10);
    static char buf[7];
    snprintf(buf, sizeof (buf), "%d.%dV", v.quot, v.rem);
    
    return buf;
}

void measureValues(void) {
    mVAvgTmp = convert(PIN_TMP, mVAvgTmp);
    mvAvgRh = convert(PIN_RH, mvAvgRh);
    mvAvgBat = convert(PIN_BAT, mvAvgBat);
}

void displayValues(void) {
    if (mVAvgTmp == -1 || mvAvgRh == -1) {
        // do nothing if not yet measured
        return;
    }

    // temperature in °C multiplied by 10
    int16_t tmpx10 = (mVAvgTmp >> EWMA_BS) - TMP36_MV_0C;
    // relative humidity in % multiplied by 10 at 3V Vdc
    int16_t rhx10 = (mvAvgRh * 100 - (45450UL << EWMA_BS)) / (191UL << EWMA_BS);
    // temperature compensation of relative humidity
    rhx10 = ((int32_t)rhx10 * 1000000) / (1054600 - tmpx10 * 216UL);
    int16_t rh = divRoundNearest(rhx10, 10);
    // battery voltage in mV (half by voltage divider)
    int16_t mVBat = (mvAvgBat >> (EWMA_BS - 1));
    int8_t vBatx10 = divRoundNearest(mVBat, 100);
    
    if (tmpx10 == prevTmpx10 && rh == prevRh && vBatx10 == prevVBatx10) {
        // skip update of display if no change in measurements
        return;
    }
    
    prevTmpx10 = tmpx10;
    prevRh = rh;
    prevVBatx10 = vBatx10;
    
    Font unifont = getUnifont();
    Font dejavu = getDejaVu();
    
    setFrame(0x00);
    writeString(0, 0, dejavu, formatTmp(tmpx10));
    writeString(0, 182, unifont, formatBat(mVBat));
    writeBitmap(0, 216, bitmapBat(vBatx10));
    writeString(4, 152, unifont, "Temperature");
    writeString(8, 0, dejavu, formatRh(rh));
    writeString(12, 152, unifont, "Humidity");
    doDisplay();
}