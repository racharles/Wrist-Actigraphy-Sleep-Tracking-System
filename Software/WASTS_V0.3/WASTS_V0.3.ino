/*******************************************************************************
WASTS
Rachel Bai
v0.3
Last edited: 11/12/2019

Samples accelerometer, humidity, and temperature data from ADXL343 and BME280
sensors using activity and inactivity HW interrupts and prints the data to a
micro-SD card.

HW config:
Arduino Nano
micro-SD module
ADXL343
BME280
2000mAh 3.7v lipo battery
Adafruit PowerBoost 500
DS3231 RTC

CHANGES: Removed LCD, split data types into different files

Libraries from Arduino and Adafruit
*******************************************************************************/

#include <Wire.h>
#include <RTClib.h>
#include <SD.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL343.h>
#include <Adafruit_BME280.h>

//constants
const int baud_rate = 9600;
const int print_delay = 50;

unsigned long previousMillis = 0;
const long read_interval = 2000; //read from bme280 every 2 seconds


//assign an I2C address to the ADXL343
Adafruit_ADXL343 accel = Adafruit_ADXL343(12345);

//assign an I2C address to BME280
Adafruit_BME280 bme;

RTC_DS3231 rtc; //create RTC object
File data; //create new file on SD card to store data in

//pin for activity interrupt on the Arduino
const int INPUT_PIN_INT1 = 2;
const int INPUT_PIN_INT2 = 3;

//struct stores the # of activity/inactivity interrupts fired
struct adxl_int_stats {
    uint32_t inactivity;
    uint32_t activity;
};

struct adxl_int_stats g_int_stats = { 0 }; //global stats block
uint32_t g_int1_fired = 0; //counter for number of unused interrupts
uint32_t g_int2_fired = 0; //counter for number of unused interrupts
int_config g_int_config_enabled = { 0 }; //variable for enabled interrupts
int_config g_int_config_map = { 0 }; //variable for mapping INT pin interrupts


/**
 * Name: int1_isr
 * Inputs: void
 * Returns: void
 * Interrupt service routine for INT1 pin event, triggers interrupt handler in
 * the main loop and increments counter for number of times the activity interrupt
 * is triggered
*/
void int1_isr(void) {
    g_int_stats.activity++; //counter
    g_int1_fired++; //trigger interrupt handler in loop
}

/**
 * Name: int2_isr
 * Inputs: void
 * Returns: void
 * Interrupt service routine for INT1 pin event, triggers interrupt handler in
 * the main loop and increments counter for number of times the inactivity interrupt
 * is triggered
*/
void int2_isr(void) {
    g_int_stats.inactivity++; //counter
    g_int2_fired++; //trigger interrupt handler in loop
}

/**
 * Name: config_interrupts
 * Inputs: void
 * Returns: void
 * Configuration for the ADXL343 INT1 and INT2 HW interrupts
*/
void config_interrupts(void) {
    //attach the INT1 input pin on the Arduino to the ISR for the INT1 on the ADXL343
    pinMode(INPUT_PIN_INT1, INPUT);
    attachInterrupt(digitalPinToInterrupt(INPUT_PIN_INT1), int1_isr, RISING);

    //attach the INT2 input pin on the Arduino to the ISR for the INT2 on the ADXL343
    pinMode(INPUT_PIN_INT2, INPUT);
    attachInterrupt(digitalPinToInterrupt(INPUT_PIN_INT2), int2_isr, RISING);

    //enable activity and inactivity interrupts on the ADXL343
    g_int_config_enabled.bits.activity = true;
    g_int_config_enabled.bits.inactivity = true;
    accel.enableInterrupts(g_int_config_enabled);

    //map the activity interrupt to INT1 and inactivity interrupt to INT2
    g_int_config_map.bits.activity = ADXL343_INT1;
    g_int_config_map.bits.inactivity = ADXL343_INT2;
    accel.mapInterrupts(g_int_config_map);
}

/**
 * Name: config_adxl
 * Inputs: void
 * Returns: void
 * Configuration for the ADXL343
*/
void config_adxl(void) {
    //check if adxl is connected
    if(!accel.begin()) {
        Serial.println("Oops, no ADXL343 detected ... Check your wiring!");
        while(1);
    }
    //set to maximum sensitivity of ADXL343
    accel.setRange(ADXL343_RANGE_2_G);

    //set activity and inactivity threshold register, scale factor is 62.5 mg/LSB
    accel.writeRegister(0x24, 20);
    accel.writeRegister(0x25, 100);
    accel.writeRegister(0x26, 4); // 4 seconds

    //turn on XYZ axis and AC mode for activity and inactivity interrupts
    accel.writeRegister(0x27, 255); //ACT_INACT_CTL register to 11111111

    //configure INT1/INT2
    config_interrupts();
}

void config_bme(void) {
    if (!bme.begin()) {
        Serial.println("Could not find a valid BME280 sensor, check wiring");
        //print sensor id for debugging
        Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
        while (1);
    }
}

/**
 * Name: config_sd
 * Inputs: void
 * Returns: void
 * Configuration for the SD module
*/
void config_sd() {
    pinMode(10, OUTPUT); //CS for SD module, CS is pin 10

    // check if sd card is connected
    if (!SD.begin(10)) {
        Serial.println("no SD card detected, check wiring");
        while(1);
    }

    //clear SD card data
    SD.remove("activity.txt");
    SD.remove("data.txt");
    SD.remove("humidity.txt");
}

/**
 * Name: setup
 * Inputs: void
 * Returns: void
 * Built-in function for Arduino, runs once at the start of the program,
 * used for initialization
*/
void setup() {
    // initialize Serial Monitor at 9600 baud
    Serial.begin(baud_rate);
    while (!Serial); //pause program and wait for serial monitor initialization

    //check if rtc is connected
    if (!rtc.begin()) {
        Serial.println("RTC not found");
        while(1);
    }

    config_sd();
    config_adxl();
    config_bme();

    //print title and skip a line
    Serial.println("WASTS V0.3");
    Serial.println("");
}

/**
 * Name: loop
 * Inputs: void
 * Returns: void
 * built-in function for Arduino, runs consecutively after setup function ends
*/
void loop() {
    //get current time from rtc
    DateTime now = rtc.now();

    //get current time to check if time to read from bme280
    //unsigned long currentMillis = millis();

    //get sensor event
    sensors_event_t event;
    accel.getEvent(&event);

    delay(print_delay); //pause to allow arduino time to process and print to serial

    //triggers when activity detected
    while (g_int1_fired) {

        /* Print to SD card
        // --------------------------------*/
        //write data with timestamp to data.txt on SD card
        File activity = SD.open("activity.txt", FILE_WRITE);

        //timestamp
        activity.print(now.hour(), DEC); activity.print(':');
        activity.print(now.minute(), DEC); activity.print(':');
        activity.print(now.second(), DEC);

        activity.println("activity detected");

        //print the acceleration data from the moment activity detected
        activity.print(event.acceleration.x); activity.print(" ");
        activity.print(event.acceleration.y); activity.print(" ");
        activity.println(event.acceleration.z);

        //print the number of times activity detected to sd card
        activity.print("\tActivity Count: "); activity.println(g_int_stats.activity, DEC);

        Serial.println("Activity Detected");

        //clear interrupt by checking the source and decrementing counter
        uint8_t source = accel.checkInterrupts();
        g_int1_fired--;
        activity.close(); //save file on SD card
    }

    //write data with timestamp to data.txt on SD card
    File data = SD.open("data.txt", FILE_WRITE);

    //triggers when inactivity detected
    while (g_int2_fired) {

        /* Print to SD card
        --------------------------------*/

        //timestamp
        data.print(now.hour(), DEC); data.print(':');
        data.print(now.minute(), DEC); data.print(':');
        data.print(now.second(), DEC);

        //print the acceleration data from the moment inactivity detected to
        data.print(event.acceleration.x); data.print(" ");
        data.print(event.acceleration.y); data.print(" ");
        data.println(event.acceleration.z);

        //print the number of times inactivity detected to sd card
        data.print("\tInactivity Count: "); data.println(g_int_stats.inactivity, DEC);

        Serial.println("inactivity detected"); //debugging

        //clear interrupt by checking the source and decrementing counter
        uint8_t source = accel.checkInterrupts();
        g_int2_fired--;
    }
    data.close(); //save file on SD card

    //check if time to read from bme280
    File humidity = SD.open("humidity.txt", FILE_WRITE);

    if (currentMillis - previousMillis >= read_interval) {
        //save last time bme280 read
        previousMillis = currentMillis;

        /* Print humidity to SD card
        --------------------------------*/

        //timestamp
        humidity.print(now.hour(), DEC); humidity.print(':');
        humidity.print(now.minute(), DEC); humidity.print(':');
        humidity.print(now.second(), DEC); humidity.print(" ");

        //humidity data
        humidity.println(bme.readHumidity());
    }
    humidity.close(); //save file on SD card
}
