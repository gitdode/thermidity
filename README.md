# Thermidity

[![make](https://github.com/gitdode/thermidity/actions/workflows/build.yml/badge.svg)](https://github.com/gitdode/thermidity/actions/workflows/build.yml)

AVR MCU based battery powered thermometer and hygrometer with e-ink display.  
Focus is on stable, accurate measurements and low power consumption. 
And of course on a nice, simple display.

The project is based on [avrink](https://github.com/gitdode/avrink) and uses the
following components:

* MCU AVR ATmega328P
* Thermistor NTC 100kΩ 0.1°C
* Humidity Sensor HIH-5030
* E-Ink display Adafruit Monochrome 2.13" 250x122
    * Driver SSD1680
    * SRAM 23K640
    * SD Card Reader (not used) 

<img src="https://luniks.net/other/Thermidity/Thermidity-07.jpg"/>

## Accuracy

AD conversion is done with 16x oversampling, yielding 12-bit virtual resolution 
with the 10-bit ADC, provided the signal contains some noise.

Each time measurements were taken, a moving average is updated which is used to 
calculate and display temperature, humidity and battery voltage.

A precision thermistor and precision low-voltage humidity sensor are used:

| Component  | Accuracy |
|------------|---------:|
| Thermistor |  ¹±0.1°C |
| HIH-5030   |    ±3%RH |

¹Series resistor 0.1% tolerance

## Power Consumption

Estimated average power consumption is about 90µA when measuring every 32 
seconds and updating the display once in about 5 minutes, hopefully giving an
operating time of at least 12 months with 3 AAA batteries (1100 mAh, 6% 
self-discharge). 

The consumption of each component at 3.8V and 22°C is about:

| Component  | Data Sheet | Measured |
|------------|-----------:|---------:|
| ATmega328P |     ¹4.2µA |    4.5µA |
| Thermistor |        N/A |     24µA |
| HIH-5030   |     ²200µA |    211µA |
| V-Divider  |        N/A |      2µA |
| Display    |       ³N/A |     14µA |

¹VCC = 3V, power-down sleep mode  
²VCC = 3.3V  
³Display in deep sleep mode + SRAM + SD Card Reader + Bus Transceiver  

Between taking measurements, the MCU is set to power-down sleep mode with the
watchdog used as wake-up source. Additionally, the thermistor and humidity 
sensor are powered off and between display updates, the display is set to deep 
sleep mode. Power consumption (measured) then is about 19µA at 3.8V for MCU 
and display.  

When measuring temperature, humidity and battery voltage in ADC noise reduction 
mode, consumption seems to be somewhere around 2mA for 6ms, plus a brief 
additional MCU awake period for updating the moving average with measured 
values. Before measuring, the sensors are powered on and given 100ms to settle, 
consuming about 240µA.

When updating the display, consumption is at around 6mA for about 3 seconds. 
Before that, the MCU has to calculate and format the average measurements and 
buffer the frame in SRAM.

The display is updated in fast update mode, taking 1.5s instead of 3s for full 
update mode, saving a considerable amount of power. To avoid ghosting effects, 
every 10th update is a full update.

When no measurement has changed, the display is not updated at all to extend its 
lifetime and to save more power.

Below the cutoff voltage of 3.0V, the watchdog is disabled to stop measuring and
updating the display, to at least delay total discharge of the batteries.
Consumption of the MCU then is at about 1µA and 19µA of the display.

The clock of unused modules TWI, all three timers and USART is switched off to 
reduce power consumption.

Disabling the ADC between measurements and the SPI between display updates both 
contributes to a significant reduction of power consumption. With an active SPI, 
consumption of the display including its peripheral is extremely unstable and 
varies between some tens of microamps and several milliamps. When driving the 
enable pin of the display low, consumption is stable at about 60µA. When not 
driving the enable pin low and disabling SPI (driving SCK pin low) consumption 
is at about 14µA.
