/* 
 * File:   meter.h
 * Author: torsten.roemer@luniks.net
 *
 * Created on 1. Mai 2023, 23:42
 */

#ifndef METER_H
#define METER_H

#define AREF_MV     5000

/** Output of the TMP36 is 750 mV @ 25°C, 10 mV per °C */
#define TMP36_MV_0C 500

/** Weight of the exponential weighted moving average as bit shift */
#define EWMA_BS     3

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

