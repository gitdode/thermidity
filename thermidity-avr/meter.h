/* 
 * File:   meter.h
 * Author: torsten.roemer@luniks.net
 *
 * Created on 1. Mai 2023, 23:42
 */

#ifndef METER_H
#define METER_H

/** Use AVCC as reference voltage */
#define AREF_AVCC   (1 << REFS0)
/** Use internal 1.1V reference voltage */
#define AREF_INT    (1 << REFS1) | (1 << REFS0)

#define AREF_MV     1100 // 1136

#define BAT_LOW     3300

/** Output of the TMP36 is 750 mV @ 25°C, 10 mV per °C */
#define TMP36_MV_0C 500

/** Ratiometric response of the HIH-5030 at 0%RH and 100%RH with 12-bit ADC */
#define RH_ADC_0    620  // Vout = Vsupply * 0.1515
#define RH_ADC_100  3225 // Vout = Vsupply * 0.7875
#define RH_ADC      RH_ADC_100 - RH_ADC_0

/** Weight of the exponential weighted moving average as bit shift */
#define EWMA_BS     4

/**
 * Measures temperature and relative humidity and updates 
 * the average values.
 */
void measureValues(void);

/**
 * Calculates, formats and displays the averaged temperature 
 * and relative humidity values.
 */
void displayValues(void);

#endif /* METER_H */

