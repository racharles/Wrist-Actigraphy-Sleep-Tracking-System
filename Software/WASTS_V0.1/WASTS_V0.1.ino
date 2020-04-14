/*******************************************************************************
WASTS
Rachel Bai
v0.1
Last edited: 8/25/19

Collects data from ADXL343 accelerometer using continous polling,
then prints the timestamp and data to the serial monitor and micro-SD card.

HW Config:
Arduino Nano
micro-SD module
ADXL343

Libraries from Arduino and Adafruit
*******************************************************************************/

#include <Wire.h>
#include <RTClib.h>
#include <SD.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL343.h>

Adafruit_ADXL343 accel = Adafruit_ADXL343(12345);
RTC_DS3231 rtc;
File data;

void setup() {
    Serial.begin(9600);

    delay(100); //wait for initialization

    //pin 10 = CS for SD module
    pinMode(10, OUTPUT);

    //check initialization
    if (!SD.begin(10)) {
        Serial.println("no SD card detected, check wiring");
        while(1);
    }
    if (!accel.begin()) {
        Serial.println("Accelerometer not detected, check wiring");
        while(1);
    }
    if (!rtc.begin()) {
        Serial.println("RTC not found");
        while(1); //stop program
    }

    //set sensitivity
    accel.setRange(ADXL343_RANGE_2_G);
    Serial.println("initialization finished");

    //reset file
    SD.remove("data.txt");
}


void loop() {
    //get current time from rtc
    DateTime now = rtc.now();

    //get sensor event
    sensors_event_t event;
    accel.getEvent(&event);

    //write data to file and print to serial
    File data = SD.open("data.txt", FILE_WRITE);

    if (data) {
        //write to data file
        //time
        data.print(now.hour(), DEC); data.print(':');
        data.print(now.minute(), DEC); data.print(':');
        data.print(now.second(), DEC);

        //accelerometer data
        data.print(" x:"); data.print(event.acceleration.x);
        data.print(" y:"); data.print(event.acceleration.y);
        data.print(" z:"); data.println(event.acceleration.z);
        data.close();

        //Display results in serial monitor
        //display time
        Serial.print(now.hour(), DEC); Serial.print(':');
        Serial.print(now.minute(), DEC); Serial.print(':');
        Serial.print(now.second(), DEC);

        //display accelerometer data
        Serial.print(event.acceleration.x);
        Serial.print(" "); Serial.print(event.acceleration.y);
        Serial.print(" "); Serial.println(event.acceleration.z);
    } else {
        Serial.println("error opening data.txt");
    }

    delay(200);
}
