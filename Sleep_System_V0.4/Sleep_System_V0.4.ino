/*******************************************************************************
Smart Sleep System
Rachel Bai
v0.4
Last edited: 1/02/20

Reads accelerometer, temperature, and humidity data from the LSM9DS1 and HTS221
sensors of the Arduino 33 BLE Sense and prints it to a micro-SD card.

HW config:
Arduino Nano BLE 33 Sense
Adafruit Micro-SD Breakout Board
Sandisk 16GB Micro-SD card
2000mAh 3.7 lipo battery
Adafruit PowerBoost 500

Libraries from Arduino
*******************************************************************************/

#include <Arduino_LSM9DS1.h>
#include <Arduino_HTS221.h>
#include <SD.h>

boolean debug_mode = false;

unsigned long previousMillis = 0;
const long interval = 500;

void config_sd() {
    pinMode(10, OUTPUT); //SD module CS connected to pin 10 of the Arduino

    //check if sd card is connected
    if (!SD.begin(10)) {
        Serial.println("no SD card detected, check wiring");
        while (1);
    }
}

void setup() {
    if (debug_mode == true) {
        //initialize serial monitor
        Serial.begin(9600);
        Serial.println("Started");
    }

    //initialize accelerometer
    if (!IMU.begin()) {
        Serial.println("IMU initialization failed");
        while (1);
    }
    //initialize humidity and temperature sensors
    if (!HTS.begin()) {
        Serial.println("HTS221 initialization failed");
        while (1);
    }

    config_sd();
}

void loop() {
    unsigned long currentMillis = millis();

    //run only if time interval has been reached
    if(currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        float x,y,z;
        float temperature = HTS.readTemperature();
        float humidity = HTS.readHumidity();

        File data = SD.open("data.txt", FILE_WRITE);

        if (IMU.accelerationAvailable()) {
            IMU.readAcceleration(x, y, z);

            if (debug_mode == true) {
                Serial.print(x);
                Serial.print('\t');
                Serial.print(y);
                Serial.print('\t');
                Serial.println(z);
            }

            data.print(x);
            data.print('\t');
            data.print(y);
            data.print('\t');
            data.println(z);
        }

        if (debug_mode == true) {
            Serial.print("Temperature = ");
            Serial.print(temperature);
            Serial.println(" °C");

            Serial.print("Humidity    = ");
            Serial.print(humidity);
            Serial.println(" %");
        }

        //print temperature and humidity to SD card
        data.print("Temperature = ");
        data.print(temperature);
        data.println(" °C");

        data.print("Humidity = ");
        data.print(humidity);
        data.println(" %");

        //save files on SD card
        data.close();
    }
}
