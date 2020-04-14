/*******************************************************************************
WASTS
Rachel Bai
v1.0
Last edited: 1/26/20

Samples accelerometer, temperature, and humidity data from the LSM9DS1 and HTS221
sensors of the Arduino 33 BLE Sense and prints data to a micro-SD card.

HW config:
Arduino Nano BLE 33 Sense
Adafruit Micro-SD Breakout Board
Kingston 8GB Micro-SD card
1000mAh 3.7v lipo battery
Adafruit PowerBoost 500

UPDATES: Changed microcontroller to Arduino Nano BLE 33 Sense, uses the built-in
sensors

Libraries from Arduino
*******************************************************************************/

#include <Arduino_LSM9DS1.h>
#include <Arduino_HTS221.h>
#include <SD.h>


boolean debug_mode = false;

unsigned long previousMillis = 0;
const long interval = 200;

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

        unsigned long time = millis(); //seconds since start of sketch

        float x,y,z;
        float temperature = HTS.readTemperature();
        float humidity = HTS.readHumidity();

        File data = SD.open("data.txt", FILE_WRITE);

        if (IMU.accelerationAvailable()) {
            IMU.readAcceleration(x, y, z);

            if (debug_mode == true) {
                Serial.print(time);
                Serial.print(':');
                Serial.print(x);
                Serial.print(' ');
                Serial.print(y);
                Serial.print(' ');
                Serial.print(z);
                Serial.print(' ');
            }

            data.print(time);
            data.print(':');
            data.print(x);
            data.print(' ');
            data.print(y);
            data.print(' ');
            data.print(z);
            data.print(' ');
        }

        if (debug_mode == true) {
            Serial.print(temperature);
            Serial.print(' ');
            Serial.println(humidity);
        }

        //print temperature and humidity to SD card
        data.print(temperature);
        data.print(' ');
        data.println(humidity);

        //save files on SD card
        data.close();

        //change the time
        time += interval;
    }
}
