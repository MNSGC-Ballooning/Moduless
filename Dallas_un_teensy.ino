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
char filename[] = "LESS00.csv";
bool SDactive = false;

bool startRead = false;

void setup() {
  XBee_Serial.begin(9600);
  xBee.enterATmode();       //Configure XBee as a relay
  xBee.atCommand("ATDL0");
  xBee.atCommand("ATMY1");
  xBee.exitATmode();

  Serial.print("Initializing SD card...");
  if(!SD.begin(chipSelect))                                 //attempt to start SD communication
    Serial.println("Card failed, or not present");          //print out error if failed; remind user to check card
  else {                                                    //if successful, attempt to create file
    Serial.println("Card initialized.\nCreating File...");
    for (byte i = 0; i < 100; i++) {                        //can create up to 100 files with similar names, but numbered differently
      filename[4] = '0' + i/10;
      filename[5] = '0' + i%10;
      if (!SD.exists(filename)) {                           //if a given filename doesn't exist, it's available
        datalog = SD.open(filename, FILE_WRITE);            //create file with that name
        SDactive = true;                                    //activate SD logging since file creation was successful
        Serial.println("Logging to: " + String(filename));  //Tell user which file contains the data for this run of the program
       // xBee.println("Logging to: " + String(filename));
        break;                                              //Exit the for loop now that we have a file
      }
    }
    if (!SDactive) Serial.println("No available file names; clear SD card to enable logging");
  }
  if(SDactive)
  {
    datalog.println("Beginning data recording.");
    datalog.println("Time,Temperature (C),Solar Temperature (C),Pressure (atm),Temperature(F),Number of Satellites,Latitude,Longitude,Altitude (m),Photoresistor,Switch");
    datalog.close();
  }
  
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
   // xBee.println("Air mode successfully set.");
  }
  else
  {
    Serial.println("WARNING: Failed to set to air mode. Altitude data may be unreliable.");
   // xBee.println("WARNING: Failed to set to air mode. Altitude data may be unreliable.");
  }
  
  //pinMode(A0, INPUT);
  //pinMode(A2, INPUT);
  
  Serial.print("Deez ");
  delay(2500);
  Serial.println("nutz");
 // xBee.println("Starting data acquisition");
}

void saveAll()
{
  String saveData = GPStime + "," + String(Temp) + "," + String(solarTemp) + "," + String(pressure) + "," + String(temp) + "," + String(gps.getSats()) + "," + String(gps.getLat(),10) + "," + String(gps.getLon(),10) + "," + String(gps.getAlt_meters()) + "," + String(analogRead(A0)) + "," + String(analogRead(A2));
  datalog = SD.open(filename, FILE_WRITE);
  /*datalog.println(String(Temp));
  datalog.println(String(solarTemp));
  datalog.println(String(pressure));
  datalog.println(String(temp));
  datalog.println(GPStime);
  datalog.println(GPSlocation);
  datalog.println(String(gps.getAlt_meters()));*/
  datalog.println(saveData);
  datalog.close();
}

void sendAll()
{
  xBee.println("Less: Temp (C): " + String(Temp) + " Temp (C): (SOLAR)" + String(solarTemp) + "Pressure (atm): " + String(pressure) + " Temp (F): " + String(temp) + GPStime + GPSlocation);
//  xBee.println("Temp (C): (SOLAR)" + String(solarTemp));
//  xBee.println("Pressure (atm): " + String(pressure) + "  Temp (F): " + String(temp));
//  xBee.println(GPStime);
//  xBee.println(GPSlocation);
}

void loop() {
  sensors.requestTemperatures();          //read most recent temp data for all sensors
  Temp = sensors.getTempCByIndex(0);    
  sensors1.requestTemperatures();          //read most recent temp data for all sensors
  solarTemp = sensors1.getTempCByIndex(0);     //get values from each sensor

  gps.update();

  pressure = sensor.readPressure()/101325;   // for altimeter/pressure sensor
  temp = sensor.readTempF();            // for altimeter/pressure sensor

  Serial.println(analogRead(A0));
  Serial.println("Switch " + String(analogRead(A2)));

  if(analogRead(A2) > 900)
  {
    startRead = true;
  }
  
  if( nextSendTime < millis() && startRead && analogRead(A0) > 150)
  {
    Serial.println("Temp (C): " + String(Temp));
    Serial.println("Temp (C): (SOLAR)" + String(solarTemp));
    
    Serial.println("Pressure (atm): " + String(pressure) + "  Temp (F): " + String(temp)); //print data to serial monitor
  
    //Serial.println(analogRead(A0));

    GPStime = String(gps.getHour()-5) + ":" + String(gps.getMinute()) + ":" + String(gps.getSecond());
    GPSlocation = "Number of Satellites " + String(gps.getSats()) + " Lat/Lon: " + String(gps.getLat(),10) + "/" + String(gps.getLon(),10) + " Alt(m): " + String(gps.getAlt_meters());
    
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
  if(nextSaveTime < millis() && startRead)
  {
    saveAll();
    Serial.println("Saved");
    Serial.println();
    nextSaveTime = millis() + 1000L;
  }
}
