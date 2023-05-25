# Thermidity

[![make](https://github.com/gitdode/thermidity/actions/workflows/build.yml/badge.svg)](https://github.com/gitdode/thermidity/actions/workflows/build.yml)

AVR MCU based battery powered thermometer and hygrometer with e-ink display 
(work in progress).  
Focus is on stable, accurate measurements and low power consumption. 
And of course on a nice, simple display.

The project is based on [avrink](https://github.com/gitdode/avrink) and uses the
following components:

* MCU AVR ATmega328P
* Voltage Reference MCP1501 3.000V
* Temperature Sensor TMP36
* Humidity Sensor HIH-5030
* E-Ink display Adafruit Monochrome 2.13" 250x122
    * Driver SSD1680
    * SRAM 23K640
    * SD Card Reader (not used) 

<img src="https://luniks.net/other/Thermidity/Thermidity-04.jpg"/>

## Accuracy

ADC conversion is done with 16x oversampling, yielding 12-bit virtual resolution 
with the 10-bit ADC, provided the signal contains some noise.

Each time measurements were taken, a moving average is updated which is used to 
calculate and display temperature, humidity and battery voltage.

Low voltage precision sensors and a high-precision buffered voltage reference 
are used:

| Component | Accuracy |
|-----------|---------:|
| MCP1501   |     0.1% |
| TMP36     |    ±2°C  |
| HIH-5030  |    ±3%RH |


## Power Consumption

Between taking the measurements and updating the display, the MCU is set to 
power-save sleep mode, with Timer2 operated in asynchronous mode clocked from 
an external 32kHz watch crystal used as wake-up source.

Power consumption then is at about 390µA at 3.8V:

| Component | Data Sheet | Measured |
|-----------|-----------:|---------:|
| MCP1501   |      140µA |    140µA |
| TMP36     |      <50µA |     22µA |
| HIH-5030  |      200µA |    200µA |
| V-Divider |       ¹2µA |      2µA |

¹Calculated

The remainder of 26µA is probably due to measurement error + MCU power 
consumption (datasheet says 0.9µA at 3V).

When measuring temperature, humidity and battery voltage in ADC noise reduction 
mode, consumption briefly goes above 400µA.

When updating the display, consumption is at around 6mA for about 2-3 seconds.

When no measurement has changed, the display is not updated to extend the 
liftime and to further save power.

The clock of unused modules such as Timer0, Timer1 and USART is switched off to 
reduce power consumption.

> When the timer is configured to wake up the MCU every 8 seconds, consumption 
> steadily goes up to ~4mA within each interval. Similar behaviour can be observed 
> with the watchdog barking every 8 seconds. With the timer waking up the MCU each 
> second, consumption steadily stays at 390µA.
