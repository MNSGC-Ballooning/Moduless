#include <Wire.h>
#include <SoftwareSerial.h>
#include <XBee.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include "SparkFunMPL3115A2.h"
#include <FlightGPS.h>
#include <UbloxGPS.h>
#include <TinyGPS++.h>
#include <SD.h>

#define oneWireBus 25
#define solarbus 24
#define chipSelect BUILTIN_SDCARD

#define XBee_Serial Serial4

XBee xBee = XBee(&XBee_Serial);
OneWire oneWire(oneWireBus);
OneWire solarwire(solarbus);
DallasTemperature sensors(&oneWire);
DallasTemperature sensors1(&solarwire);
double Temp, solarTemp;

String ID = "LESS";   //Choose an ID for your XBee - 2-4 character string, A-Z and 0-9 only please
unsigned long nextSendTime = 10000;
unsigned long nextSaveTime = 10000;

MPL3115A2 sensor;       //create pressure sensor object
double pressure, temp;   //variables for measured data

UbloxGPS gps = UbloxGPS(&Serial1);
String GPStime;
String GPSlocation;

File datalog;
char filename[] = "LESSLOG04.csv";

void setup() {
  XBee_Serial.begin(9600);
  xBee.enterATmode();       //Configure XBee as a relay
  xBee.atCommand("ATDL0");
  xBee.atCommand("ATMY1");
  xBee.exitATmode();

  datalog = SD.open(filename, FILE_WRITE);
  datalog.println("Beginning data recording.");
  datalog.close();

  Wire.begin();         //Begin I2C communication on the 0 pins

  Serial.begin(9600);     //start Serial communication

  sensors.begin();         //Starts I2C for temperature
  sensors1.begin();
  
  sensor.begin();       // for altimeter
  sensor.setModeBarometer();    //Read pressure directly in Pa
  sensor.setOversampleRate(7);  //Recommended setting for altimeter
  sensor.enableEventFlags();    //Recommended setting for altemiter
  
  Serial1.begin(UBLOX_BAUD);
  gps.init(); // starting communication with the GPS receiver
  if(gps.setAirborne())
  {
    Serial.println("Air mode successfully set.");
    xBee.println("Air mode successfully set.");
  }
  else
  {
    Serial.println("WARNING: Failed to set to air mode. Altitude data may be unreliable.");
    xBee.println("WARNING: Failed to set to air mode. Altitude data may be unreliable.");
  }
  
  pinMode(A0, INPUT);
  pinMode(A2, INPUT);
  
  Serial.print("Deez ");
  delay(2500);
  Serial.println("nutz");
  xBee.println("Deez nutz");
}

void saveAll()
{
  datalog = SD.open(filename, FILE_WRITE);
  datalog.println("Temp (C): " + String(Temp));
  datalog.println("Temp (C): (SOLAR)" + String(solarTemp));
  datalog.println("Pressure (atm): " + String(pressure) + "  Temp (F): " + String(temp));
  datalog.println(GPStime);
  datalog.println(GPSlocation);
  datalog.close();
}

void sendAll()
{
  xBee.println("Temp (C): " + String(Temp));
  xBee.println("Temp (C): (SOLAR)" + String(solarTemp));
  xBee.println("Pressure (atm): " + String(pressure) + "  Temp (F): " + String(temp));
  xBee.println(GPStime);
  xBee.println(GPSlocation);
}

void loop() {
  sensors.requestTemperatures();          //read most recent temp data for all sensors
  Temp = sensors.getTempCByIndex(0);    
  sensors1.requestTemperatures();          //read most recent temp data for all sensors
  solarTemp = sensors1.getTempCByIndex(0);     //get values from each sensor

  gps.update();

  pressure = sensor.readPressure()/101325;   // for altimeter/pressure sensor
  temp = sensor.readTempF();            // for altimeter/pressure sensor

  //Serial.println(analogRead(A0));
  //Serial.println("Switch " + String(analogRead(A2)));
  
  if( nextSendTime < millis() && analogRead(A2) < 200 && analogRead(A0) > 250)
  {
    Serial.println("Temp (C): " + String(Temp));
    Serial.println("Temp (C): (SOLAR)" + String(solarTemp));
    
    Serial.println("Pressure (atm): " + String(pressure) + "  Temp (F): " + String(temp)); //print data to serial monitor
  
    //Serial.println(analogRead(A0));

    GPStime = String(gps.getHour()-5) + ":" + String(gps.getMinute()) + ":" + String(gps.getSecond());
    GPSlocation = "Number of Satellites " + String(gps.getSats()) + " Lat/Lon: " + String(gps.getLat(),10) + "/" + String(gps.getLon(),10) + " Alt(m): " + String(gps.getAlt_meters());
    
    /*Serial.print(gps.getYear());                ///< [year], Year (UTC)
    Serial.print("\t");
    Serial.print(gps.getMonth());               ///< [month], Month, range 1..12 (UTC)
    Serial.print("\t");
    Serial.print(gps.getDay());                 ///< [day], Day of month, range 1..31 (UTC)
    Serial.print("\t");*/
    Serial.print(gps.getHour()-5);                ///< [hour], Hour of day, range 0..23 (UTC)
    Serial.print(":");
    Serial.print(gps.getMinute());                 ///< [min], Minute of hour, range 0..59 (UTC)
    Serial.print(":");
    Serial.print(gps.getSecond());                 ///< [s], Seconds of minute, range 0..60 (UTC)
    Serial.print("\t");
    Serial.print("Number of Satellites ");
    Serial.print(gps.getSats());       ///< [ND], Number of satellites used in Nav Solution
    Serial.print("\t");
    Serial.print("Lat/Lon ");
    Serial.print(gps.getLat(),10);     ///< [deg], Latitude
    Serial.print("\t");
    Serial.print(gps.getLon(),10);    ///< [deg], Longitude
    Serial.print("\t");
    Serial.print("Alt (m)");
    Serial.println(gps.getAlt_meters());      ///< [ft], Height above mean sea level
    Serial.println();

    sendAll();
    
    nextSendTime = millis() + 10000L;
  }
  if(nextSaveTime < millis())
  {
    saveAll();
    nextSaveTime = millis() + 1000L;
  }
}
