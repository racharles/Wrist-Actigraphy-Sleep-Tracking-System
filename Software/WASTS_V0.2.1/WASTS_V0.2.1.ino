/*******************************************************************************
WASTS
Rachel Bai
v0.2.1
Last edited: 8/25/19

Collects data from ADXL343 accelerometer using the activity HW interrupt and
continous polling, then prints the timestamp and data to the serial monitor and
micro-SD card.

HW Config:
Arduino Nano
micro-SD module
ADXL343

UPDATE: uses activity interrupt from ADXL343 for data collection

Libraries from Arduino and Adafruit
*******************************************************************************/

//add inactivity
#include <Wire.h>
#include <RTClib.h>
#include <SD.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL343.h>

//constants
const int baud_rate = 9600;
const int print_delay = 30;

//assign an I2C address to the ADXL343
Adafruit_ADXL343 accel = Adafruit_ADXL343(12345);

RTC_DS3231 rtc; //create RTC object
File data; //create new file on SD card to store data in

//pin for activity interrupt on the Arduino
const int INPUT_PIN_INT1 = 2;

//struct stores the # of activity interrupts fired
struct adxl_int_stats {
    uint32_t activity;
};

struct adxl_int_stats g_int_stats = { 0 }; //global stats block
uint32_t g_ints_fired = 0; //counter for number of unused interrupts
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
    g_ints_fired++; //trigger interrupt handler in loop
}

/**
 * Name: config_interrupts
 * Inputs: void
 * Returns: void
 * Configuration for the ADXL343 INT1 HW interrupt
*/
void config_interrupts(void) {
    //attach the INT1 input pin on the Arduino to the ISR for the INT1 on the ADXL343
    pinMode(INPUT_PIN_INT1, INPUT);
    attachInterrupt(digitalPinToInterrupt(INPUT_PIN_INT1), int1_isr, RISING);

    //enable only the activity interrupt on the ADXL343
    g_int_config_enabled.bits.activity = true;
    accel.enableInterrupts(g_int_config_enabled);

    //map the activity interrupt to INT1
    g_int_config_map.bits.activity = ADXL343_INT1;
    accel.mapInterrupts(g_int_config_map);
}

/**
 * Name: setup
 * Inputs: void
 * Returns: void
 * Built-in function for Arduino, runs once at the start of the program,
 * initialization for program
*/
void setup() {
    Serial.begin(baud_rate);

    pinMode(10, OUTPUT); //CS for SD module, CS is pin 10

    while (!Serial); //pause program and wait for serial monitor initialization

    //initialization
    if(!accel.begin()) {
        Serial.println("Oops, no ADXL343 detected ... Check your wiring!");
        while(1);
    }
    if (!SD.begin(10)) {
        Serial.println("no SD card detected, check wiring");
        while(1);
    }
    if (!rtc.begin()) {
        Serial.println("RTC not found");
        while(1);
    }

    //print title and skip a line
    Serial.println("WASTS V0.2.1");
    Serial.println("");

    //set to maximum sensitivity of ADXL343
    accel.setRange(ADXL343_RANGE_2_G);

    //set activity threshold register, scale factor is 62.5 mg/LSB
    accel.writeRegister(0x24, 20);

    //turn on XYZ axis and AC mode in ACT_INACT_CTL register to 1110000, dec = 112
    accel.writeRegister(0x27, 112);

    //configure INT1
    config_interrupts();

    //clear SD file
    SD.remove("data.txt");
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

    delay(print_delay); //allow arduino time to process and print to serial

    //trigger when activity detected
    while (g_ints_fired) {

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

        //print timestamp to serial monitor
        Serial.print(now.hour(), DEC); Serial.print(':');
        Serial.print(now.minute(), DEC); Serial.print(':');
        Serial.println(now.second(), DEC);

        //print the acceleration data from the moment activity detected
        Serial.print(event.acceleration.x); Serial.print(" ");
        Serial.print(event.acceleration.y); Serial.print(" ");
        Serial.println(event.acceleration.z);

        //print the number of times activity detected to sd card
        data.print("\tActivity Count: "); data.println(g_int_stats.activity, DEC);
        data.close(); //save file on SD card

        //print the number of times activity detected to serial monitor
        Serial.println("Activity detected!");
        Serial.print("\tActivity Count: "); Serial.println(g_int_stats.activity, DEC);

        //clear interrupt by checking the source and decrementing counter
        uint8_t source = accel.checkInterrupts();
        g_ints_fired--;
    }
}
