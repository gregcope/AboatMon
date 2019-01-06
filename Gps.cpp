#include <Arduino.h>
#include "Gps.h"

Gps::Gps(byte pin) {
  // Constructor
  // Takes an int as pin to power up
  _powerPin = pin;
  pinMode(_powerPin, OUTPUT);
}

boolean Gps::on(void) {
  // Switch device on by putting pin HIGH
  digitalWrite(_powerPin, HIGH);
  _powerState = true;
  Serial1.begin(9600);
  Serial1.flush();
  Serial.println("GPS device on!");
  Serial.flush();
  setupGPS();
  if ( Serial1.available() > 0) {
    // spitting something out so assume on!
    return true;
  } else {
    return false;  
  } 
}

void Gps::off(void) {
  // turn off the Device by putting pin LOW
  digitalWrite(_powerPin, LOW);
  Serial1.flush();
  //delay(500);
  _powerState = false;
  Serial.println("GPS device off!");
}

boolean Gps::isOn(void) {
  // return state 
  // on == true
  // off == false
  //DEBUG("gps state is:");
  //DEBUGln(_powerState);
  return _powerState;  
}

boolean Gps::updateFix(unsigned long timeout) {

  // takes a nmea timeout
  // assume timeout is 12 secs - 12000
  // run until the nmea object is updated
  // at least 10 times?
  // return true or false

  //DEBUGln("updateFix")
  
  nmeaTimeoutMs = millis() + timeout;
  nmeaUpdates = nmea.sentencesWithFix() + 10;
  
  // make sure GPS is on (may not be needed)
  if ( !isOn() ) {
    //DEBUGln("gps is off, turning it on")
    on();
  }

  //DEBUG("nmeaUpdates: ")
  //DEBUG(nmeaUpdates)
  //DEBUG(", nmea.sentencesWithFix()")
  //DEBUG(nmea.sentencesWithFix())
  //DEBUG(", nmeaTimeoutMs: ")
  //DEBUG(nmeaTimeoutMs)
  //DEBUG(", millis is: ")
  //DEBUGln(millis())
  
  while ( nmeaTimeoutMs > millis() ) {
    
    drainNmea();

    //if ( nmea.location.isUpdated() ) {
    //  // if the object was updated, update counter
    //  DEBUGln("nmea.location.isUpdated")
    //  nmeaUpdated ++;
    //}
    
    // when we have 10 fix updates (10 secs ish)
    // return
    //DEBUG("nmea.sentencesWithFix():")
    //DEBUGln(nmea.sentencesWithFix());
    if ( nmeaUpdates <= nmea.sentencesWithFix() ) {
      //DEBUGln("Got 10 nmea updates")
      return true;
    }
    
  }

  if ( ( nmeaUpdates - nmea.sentencesWithFix() ) < 10 ) {
      //DEBUG("nmeaUpdates: ")
      //DEBUG(nmeaUpdates)
      //DEBUG(", nmea.sentencesWithFix()")
      //DEBUG(nmea.sentencesWithFix())
      //DEBUG(", nmeaTimeoutMs: ")
      //DEBUG(nmeaTimeoutMs)
      //DEBUG(", millis is: ")
      //DEBUGln(millis())
      // got some fixes, just not 10 ...
      return true;
  }
  // fell out of timeout with insufficient nmea updates
  return false;
}

boolean Gps::getUpdatedFix(unsigned long timeout) {
  
  on();
  updateFix(timeout);
  off();

  if ( nmea.location.isUpdated() ) {
    printGPSData();
    return true; 
  } else {
    return false;  
  }
  return true;
}

unsigned long Gps::getInitialFix(unsigned long timeout) {

  // takes a gps fix tmeout
  // returns millis to get a fix OR zero if failed

  gpsFixTimeoutReached = false;
  gpsTimerStart = millis();
  gpsFixTimeoutMs = gpsTimerStart + timeout;
  gpsTimeToFixMs = 0;
  initialHDOP = 0;
  finalHDOP = 0;
  hdop = 0;
  now = 0;
  //nmeaUpdated = 0;

  on();
  
  DEBUGln("getInitialFix");

  while ( !gpsFixTimeoutReached ) {
    // capture now
    now = millis();

    // update the nmea object lots of times
    if ( updateFix(12000) ) {
      // do nothing - it works
    } else {
      // no nmea ... bail
      // no updateds bail
      DEBUGln("no nmea .... bailing")
      return 0;
    }
    //while ( nmeaUpdated < 9 ) {
      //drainNmea();
   //   if ( nmea.hdop.isUpdated() ) {
   //     nmeaUpdated ++;
   //     DEBUG(".");  
   //   }
   // }

    // see if we got a sensible fix
    DEBUG(now);

    DEBUG(", hdop.isUpdated: ");
    if ( nmea.hdop.isUpdated() ) {
      DEBUG("TRUE");
    } else {
      DEBUG("FALSE");
    }

    hdop = nmea.hdop.value();

    DEBUG(", hdop.age: ");
    DEBUG(nmea.hdop.age());

    DEBUG(", loc.isUpdated: ");
    if (nmea.location.isUpdated() ) {
      DEBUG("TRUE");
    } else {
      DEBUG("FALSE");  
    }

    DEBUG(", HDOP:");
    DEBUG(hdop);
    DEBUG(", Serial1.ava: ");
    DEBUG(Serial1.available());
    DEBUG(", ACCEPTABLE HDOP: ");
    DEBUG(ACCEPTABLE_GPS_HDOP_FOR_FIX);
    DEBUG(", initial HDOP: ");
    DEBUG(initialHDOP);
    DEBUG(", gpsFixTimeoutMs: ");
    DEBUG(gpsFixTimeoutMs);
    DEBUG(", nmea.passCksum: ");
    DEBUGln(nmea.passedChecksum());
    
    // do we have an acceptable fix yet?
    if ( ( hdop != 0 && hdop <= ACCEPTABLE_GPS_HDOP_FOR_FIX )  && initialHDOP == 0 ) {
      DEBUGln("gpsFix is true - we have an inital acceptable fix!!!!");
      DEBUG("nmea.hdop.value(): ");
      DEBUGln(hdop);
      // stops this if statement next time
      initialHDOP = hdop;  
      gpsTimeToFixMs = now - gpsTimerStart;
      DEBUG("gpsTimeToFixMs: ");
      DEBUGln(gpsTimeToFixMs);
      printGPSData();
      //off();
      //return gpsTimeToFixMs;
    }

   // do we have a good fix?
   if ( hdop != 0 && hdop <= GOOD_GPS_HDOP_FOR_FIX ) {
      DEBUGln("gpsFix is true, hdop low - we have a good fix!!!!");
      DEBUG("nmea.hdop.value(): ");
      DEBUGln(hdop);
      finalHDOP = hdop;  
      gpsTimeToFixMs = now - gpsTimerStart;
      DEBUG("gpsTimeToFixMs: ");
      DEBUGln(gpsTimeToFixMs);
      printGPSData();
      off();
      return gpsTimeToFixMs;
    }

    // we reached the timeout ... too bad
    if ( ( gpsFixTimeoutMs <= now ) && initialHDOP == 0) {
      
      //gpsFixTimeoutReached = true;
      // we failed to get a fix ...
      DEBUGln("We failed to get fix !!!");
      off();
      return 0; 
    }
    
    // we reached a timeout, lets bomb
    if ( gpsFixTimeoutMs <= now ) {
      gpsFixTimeoutReached = true;
    }

 
  } // we fell out of loop with a reasonable GPS fix.

  DEBUGln("We only got a reasonable fix !!!");
  DEBUG("gpsTimeToFixMs: ");
  DEBUGln(gpsTimeToFixMs); 
  printGPSData();
  off();
  return gpsTimeToFixMs;
}

boolean Gps::drainNmea(void) {

  // grab all avaliable data and feed it to the gps nmea parser

  //DEBUGln("drainNmea()");
  serial1Output = false;
  nmeaOutput = false;

  while (Serial1.available() > 0) {
    serial1Output = true;
    if (nmea.encode(Serial1.read())) {
      nmeaOutput = true;
      // returns true if a sentence is complete
      // updates nmea.passedChecksum()
    }
  }
  
  return serial1Output;
}

void Gps::setupGPS(void) {

  // Function to setup GPS
  //DEBUGln("in setupGPS")
  
  // http://forums.adafruit.com/viewtopic.php?f=19&p=143502
  //DEBUGln("  Enable SBAS sat search");
  Serial1.print("$PMTK313,1*2E\r\n");  // Enable to search a SBAS satellite
  //DEBUGln("  Enable WAAS as DPGS source");
  Serial1.print("$PMTK301,2*2E\r\n");  // Enable WAAS as DGPS Source
  // https://code.google.com/p/ardupilot-mega/source/browse/libraries/AP_GPS/AP_GPS_NMEA.h?spec=svneff9f9d0906c1e3cd4e217b0363d0f9d44394e75&name=f401de6d52&r=eff9f9d0906c1e3cd4e217b0363d0f9d44394e75
  //DEBUGln("  Set GPS hz to 1hz");
  Serial1.print("$PMTK220,1000*1F\r\n"); // 1HZ
  // http://aprs.gids.nl/nmea/#gga
  // 0 = Invalid, 1 = GPS fix, 2 = DGPS fix
  // 1 in this example
  // $GPGGA,064951.000,2307.1256,N,12016.4438,E,1,8,0.95,39.9,M,17.8,M,,*65
  TinyGPSCustom fixqual(nmea, "GPGGA", 6); // $GPGGA sentence, 6th element

}

void Gps::printGPSData(void) {

  Serial.print(F("GPS Location: "));
  if (nmea.location.isValid())
  { 
    Serial.print(nmea.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(nmea.location.lng(), 6);
  }
  else
  { 
    Serial.print(F("INVALID"));
  }

  // print date /time in ISO 8601 format
  // YYYY-MM-DDTHHMMSSCCZ
  Serial.print(F(", Date/Time: "));
  if (nmea.date.isValid())
  { 
    Serial.print(nmea.date.year());
    Serial.print(F("-"));
    Serial.print(nmea.date.month());
    Serial.print(F("-")); 
    if (nmea.date.day() < 10) Serial.print(F("0"));
    Serial.print(nmea.date.day());
  }
  else
  { 
    Serial.print(F("INVALID"));
  }

  Serial.print(F("T"));
  if (nmea.time.isValid())
  { 
    if (nmea.time.hour() < 10) Serial.print(F("0"));
    Serial.print(nmea.time.hour());
    Serial.print(F(":"));
    if (nmea.time.minute() < 10) Serial.print(F("0"));
    Serial.print(nmea.time.minute());
    Serial.print(F(":"));
    if (nmea.time.second() < 10) Serial.print(F("0"));
    Serial.print(nmea.time.second());
    Serial.print(F("."));
    if (nmea.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(nmea.time.centisecond());
    Serial.print("Z");
  }
  else
  { 
    Serial.print(F("INVALID"));
  } 

  Serial.print(F(", "));
  if (nmea.hdop.isValid())
  {

    Serial.print(F("HDOP: "));
    Serial.print(nmea.hdop.value());
    Serial.print(F(" HDOP is: "));

    if (nmea.hdop.value() < 150  && nmea.hdop.value() != 0) {
      Serial.print(F("GOOD"));
    } else {
      Serial.print(F("BAD"));
    }

  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(", "));
  if (nmea.satellites.isValid())
  {
    Serial.print(F("SATs in use: "));
    Serial.print(nmea.satellites.value());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.println();
  Serial.flush();
}
