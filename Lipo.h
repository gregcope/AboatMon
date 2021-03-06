#ifndef Lipo_h
#define Lipo_h

/*
 * Lipo
 * 
 * Library to wrap Lipo battery readings
 * 
 * Greg Cope <greg.cope@gmail.com>
 * 
 */

#include <Arduino.h>

#define DEBUG(input)   {Serial.print(input); Serial.flush();}
#define DEBUGln(input) {Serial.println(input); Serial.flush();}

// 3.3 (Vcc) / 1024 (ADC precision) =  0.0032258
// Motineo example 1m+470k = 470 / ( 1000 +470 ) = 0.32 rounded up
// Ours 1.8m + 3.3m = 3300000 / ( 1800000 + 3300000 ) = 0.64705 or 1.55 (inverse)
//#define BATT_FORMULA(reading) reading * 0.0032258 * 1.5455 // >>> fine tune this parameter to match your voltage when fully charged
                                                       // details on how this works: https://lowpowerlab.com/forum/index.php/topic,1206.0.html
// measured                                                
// #define BATT_FORMULA(reading) reading * 0.004782608696
// 3.876V is 780 = 0.004969230769
//#define BATT_FORMULA(reading) reading * 0.004969230769
// 3.86 is 777 = 0.004967824968
//#define BATT_FORMULA(reading) reading * 0.004967824968
// 3.85 is 776 = 0.004961340206
#define BATT_FORMULA(reading) reading * 0.004961340206

class Lipo
{
  public:
    Lipo(int pin);
    float read(void);
    boolean isVoltageLow(void);
  private:
    unsigned long runTime;
    unsigned int lipoBatteryReadings;
    int _voltageDividerPin;
    float lipoBatteryVolts;
};

#endif
