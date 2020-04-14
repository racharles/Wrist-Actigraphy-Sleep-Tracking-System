/*******************************************************************************
WASTS
Rachel Bai
v0.2.3
Last edited: 10/6/2019

Samples data from ADXL343 accelerometer and BME280 humidity sensor using
activity and inactivity HW interrupts and prints data w/ timestamps to sd card,
serial monitor, and LCD.

HW config:
Arduino Nano
micro-SD module
LCD 16x2 display
ADXL343
BME280
2000mAh 3.7v lipo battery
Adafruit PowerBoost 500
DS3231 RTC

CHANGES: Added BME280 for humidity and temperature sensing

Libraries from Arduino and Adafruit
*******************************************************************************/

#include <Wire.h>
#include <RTClib.h>
#include <SD.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL343.h>
#include <Adafruit_BME280.h>
#include <LiquidCrystal.h>

//constants
const int baud_rate = 9600;
const int print_delay = 50;

//lcd pins
const int rs = 8, en = 9, d4 = 4, d5 = 5, d6 = 6, d7 = 7;

//create lcd object
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

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

    //set inactivity threshold register, scale factor is 62.5 mg/LSB
    accel.writeRegister(0x24, 20);
    accel.writeRegister(0x25, 100);
    accel.writeRegister(0x26, 5);

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

    SD.remove("data.txt"); //clear SD file
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

    //set up lcd's number of columns and rows
    lcd.begin(16,2);

    //check if rtc is connected
    if (!rtc.begin()) {
        Serial.println("RTC not found");
        while(1);
    }

    config_sd();
    config_adxl();
    config_bme();

    //print title and skip a line
    Serial.println("WASTS V0.2.3");
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

    //get sensor event
    sensors_event_t event;
    accel.getEvent(&event);

    delay(print_delay); //pause to allow arduino time to process and print to serial

    //read humidity and temperature data from bme280
    Serial.print("Humidity: "); Serial.println(bme.readHumidity());
    Serial.print("Temperature: "); Serial.println(bme.readTemperature());

    //triggers when activity detected
    while (g_int1_fired) {

        /* Print to SD card
        --------------------------------*/
        //write data with timestamp to data.txt on SD card
        File data = SD.open("data.txt", FILE_WRITE);

        //timestamp
        data.print(now.hour(), DEC); data.print(':');
        data.print(now.minute(), DEC); data.print(':');
        data.print(now.second(), DEC);

        data.println("activity detected");

        //print the acceleration data from the moment activity detected
        data.print(event.acceleration.x); data.print(" ");
        data.print(event.acceleration.y); data.print(" ");
        data.println(event.acceleration.z);

        //print the number of times activity detected to sd card
        data.print("\tActivity Count: "); data.println(g_int_stats.activity, DEC);
        data.close(); //save file on SD card

        /* Print to Serial Monitor
        --------------------------------*/
        //print timestamp
        Serial.print(now.hour(), DEC); Serial.print(':');
        Serial.print(now.minute(), DEC); Serial.print(':');
        Serial.println(now.second(), DEC);

        //print the acceleration data from the moment activity detected
        Serial.print(event.acceleration.x); Serial.print(" ");
        Serial.print(event.acceleration.y); Serial.print(" ");
        Serial.println(event.acceleration.z);

        //print the number of times activity detected to serial monitor
        Serial.println("Activity detected!");
        Serial.print("\tActivity Count: "); Serial.println(g_int_stats.activity, DEC);

        /* Print to LCD
        --------------------------------*/
        //print on first row
        lcd.setCursor(0,0);
        lcd.print("Activity Detected!");

        //clear interrupt by checking the source and decrementing counter
        uint8_t source = accel.checkInterrupts();
        g_int1_fired--;
    }

    //triggers when inactivity detected
    while (g_int2_fired) {

        /* Print to SD card
        --------------------------------*/
        //write data with timestamp to data.txt on SD card
        File data = SD.open("data.txt", FILE_WRITE);

        //timestamp
        data.print(now.hour(), DEC); data.print(':');
        data.print(now.minute(), DEC); data.print(':');
        data.print(now.second(), DEC);

        data.println("inactivity detected");

        //print the acceleration data from the moment inactivity detected to
        data.print(event.acceleration.x); data.print(" ");
        data.print(event.acceleration.y); data.print(" ");
        data.println(event.acceleration.z);

        //print the number of times inactivity detected to sd card
        data.print("\tInactivity Count: "); data.println(g_int_stats.inactivity, DEC);
        data.close(); //save file on SD card

        /* Print to Serial Monitor
        --------------------------------*/
        //print timestamp
        Serial.print(now.hour(), DEC); Serial.print(':');
        Serial.print(now.minute(), DEC); Serial.print(':');
        Serial.println(now.second(), DEC);

        //print the acceleration data from the moment inactivity detected
        Serial.print(event.acceleration.x); Serial.print(" ");
        Serial.print(event.acceleration.y); Serial.print(" ");
        Serial.println(event.acceleration.z);

        //print the number of times inactivity detected to serial monitor
        Serial.println("Inactivity detected!");
        Serial.print("\tInactivity Count: "); Serial.println(g_int_stats.inactivity, DEC);

        /* Print to LCD
        --------------------------------*/
        //print on second row
        lcd.setCursor(0,1);
        lcd.print("Inactivity Detected!");

        //clear interrupt by checking the source and decrementing counter
        uint8_t source = accel.checkInterrupts();
        g_int2_fired--;
    }
}
