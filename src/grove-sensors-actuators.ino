/*
 * Project grove-sensors-actuators
 * Description: Grove Sensors and Actuators
 * Author: Ponlawat W.
 * Date: 9 November 2020
 * 
 * In this code, allows users to read one sensor and write one actuator at the time.
 * The sensor must send normalised value between 0 to 1, and the actuator must act depending on the value.
 */

#include "DHT.h"
#include "Grove_4Digit_Display.h"
#include "Grove_ChainableLED.h"
#include "Grove-Ultrasonic-Ranger.h"

// Enumeration Constants

#define ACTUATOR_7SEG 1
#define ACTUATOR_BUZZ 2
#define ACTUATOR_LED  3

#define SENSOR_BUTTON           1
#define SENSOR_LIGHT            2
#define SENSOR_ROTARY           3
#define SENSOR_ULTRASONIC       4
#define SENSOR_DHT_HUMIDITY     5
#define SENSOR_DHT_TEMPERATURE  6

// Customisation //

// ↓ interval of delay in loop()
#define INTERVAL      10
// ↓ how many times loop() executed to publish message to Particle server with publishNumber() function
#define PUBLISH_WAIT  100

#define SENSOR_BUTTON_PIN     D4
#define SENSOR_DHT_PIN        D2
#define SENSOR_DHT_TYPE       DHT11
#define SENSOR_LIGHT_PIN      A4
#define SENSOR_ROTARY_PIN     A2
#define SENSOR_ULTRASONIC_PIN D2

// ↓ For temperature sensor in degree celcius, which temperature will be minimum of being considered as 0
#define TEMP_MIN    10
// ↓ For temperature sensor, TEMP_MIN + TEMP_RANGE will be considered as 1
#define TEMP_RANGE  20

#define ACTUATOR_7SEG_CLK D2
#define ACTUATOR_7SEG_DIO D3

#define ACTUATOR_BUZZ_PIN A0

#define ACTUATOR_LED_1  D4
#define ACTUATOR_LED_2  D5

// Sensors and Actuators //

// ↓ Select desired sensor here, 0 when read no sensors
const int SENSOR = SENSOR_ROTARY;
// ↓ Select desired actuator here, 0 when write to no actuators
const int ACTUATOR = ACTUATOR_7SEG;
// ↓ false when sensor read 0 to 1, true when sensor read 1 to 0
const bool REVERSE = false;

// Library Initialisation //

ChainableLED leds(ACTUATOR_LED_1, ACTUATOR_LED_2, 1);
DHT dht(SENSOR_DHT_PIN, SENSOR_DHT_TYPE);
TM1637 tm1637(ACTUATOR_7SEG_CLK, ACTUATOR_7SEG_DIO);
Ultrasonic ultrasonic(SENSOR_ULTRASONIC_PIN);

float SensorValue = 0; // initial normalised sensor value to be zero
int PublishWaitCount = PUBLISH_WAIT;

/**
 * Publish number in parameter to Particle cloud
*/
void publishNumber(float number) {
  if (++PublishWaitCount > PUBLISH_WAIT) {
    char valueStr[10];
    sprintf(valueStr, "%f", number);
    Particle.publish(valueStr, PRIVATE);
    PublishWaitCount = 0;
  }
}

// Sensor Read Functions //

/**
 * Read value from button, returns 1.0 when pressed, 0.0 when released
*/
float readButton() {
  return digitalRead(SENSOR_BUTTON_PIN) == HIGH ? 1.0 : 0.0;
}

/**
 * Read humidity from dht sensor, returns 1.0 when 100% humidity, 0.0 when 0% humidity
*/
float readDhtHumidity(float previousValue) {
  float value = dht.readHumidity(true);
  return isnan(value) ? previousValue : value / 100.0;
}

/**
 * Read temperature from dht sensor, returns 0.0 when TEMP_MIN, 1.0 when TEMP_MIN + TEMP_RANGE
*/
float readDhtTemperature(float previousValue) {
  float value = dht.readTemperature(false, true);
  if (isnan(value)) {
    return previousValue;
  }
  value -= TEMP_MIN;
  value /= TEMP_RANGE;
  return value < 0 ? 0 : (value > 1 ? 1 : value);
}

/**
 * Read brightness from light sensor, the sensor should return value from 0 to 1023,
 * then it will be normalised to be between 0.0 and 1.0
*/
float readLight() {
  float value = 1 - ((float)analogRead(SENSOR_LIGHT_PIN) / 1023.0);
  return value < 0 ? 0 : (value > 1 ? 1 : value);
}

/**
 * Read value from rotary sensor, returns between 0.0 and 1.0 depends on rotation degree.
 * The sensor should originally be read integer between 0 and 4096
*/
float readRotary() {
  float value = (float)analogRead(SENSOR_ROTARY_PIN);
  value = value < 26 ? 0 : value;
  return value / 4096.0;
}

/**
 * Read value from ultrasonic sensor, returns 0.0 to 1.0 if the object is between 0 and 1 metre.
 * If the object is further than 1 metre, returns 1.0
*/
float readUltrasonic() {
  float range = (float)ultrasonic.MeasureInCentimeters() / 100.0;
  range = (range > 1 ? 1 : (range < 0 ? 0 : range));
  return range;
}

/**
 * Write value to 4-digit 7-segment display
 * 0.0 will be displayed 0000, and 1.0 will be displayed 9999
*/
void write7Seg(float value) {
  int displayValue = (int)(round(value * 100000.0 / 10.0));
  displayValue = displayValue > 9999 ? 9999 : (displayValue < 0 ? 0 : displayValue);

  tm1637.display(0, (displayValue / 1000) % 10);
  tm1637.display(1, (displayValue / 100) % 10);
  tm1637.display(2, (displayValue / 10) % 10);
  tm1637.display(3, displayValue % 10);
}

/**
 * Write value to buzzer, value 0.0 will mute the buzzer, the more the value is, the lounder the buzzer buzzes.
*/
void writeBuzz(float value) {
  value *= 255.0;
  value = value < 0 ? 0 : (value > 255 ? 255 : value);
  analogWrite(ACTUATOR_BUZZ_PIN, value);
}

/**
 * Write value to chainable LED, the colour will always be white, brightness will depend on value from 0: darkest, 1: brightness
*/
void writeLed(float value) {
  value *= 255.0;
  value = value < 0 ? 0 : (value > 255 ? 255 : value);
  leds.setColorRGB(0, (int)value, (int)value, (int)value);
}

void setup() {
  switch (SENSOR) {
    case SENSOR_BUTTON:
      pinMode(SENSOR_BUTTON_PIN, INPUT); break;
    case SENSOR_DHT_HUMIDITY:
      dht.begin(); break;
    case SENSOR_DHT_TEMPERATURE:
      dht.begin(); break;
    case SENSOR_LIGHT:
      pinMode(SENSOR_LIGHT_PIN, INPUT); break;
    case SENSOR_ROTARY:
      pinMode(SENSOR_ROTARY_PIN, INPUT); break;
  }

  switch (ACTUATOR) {
    case ACTUATOR_7SEG:
      tm1637.init();
      tm1637.set(BRIGHT_TYPICAL);
      tm1637.point(POINT_OFF);
      break;
    case ACTUATOR_BUZZ:
      pinMode(ACTUATOR_BUZZ_PIN, OUTPUT); break;
    case ACTUATOR_LED:
      leds.init(); break;
  }
}

void loop() {
  switch (SENSOR) {
    case SENSOR_BUTTON:
      SensorValue = readButton(); break;
    case SENSOR_DHT_HUMIDITY:
      SensorValue = readDhtHumidity(SensorValue); break;
    case SENSOR_DHT_TEMPERATURE:
      SensorValue = readDhtTemperature(SensorValue); break;
    case SENSOR_LIGHT:
      SensorValue = readLight(); break;
    case SENSOR_ROTARY:
      SensorValue = readRotary(); break;
    case SENSOR_ULTRASONIC:
      SensorValue = readUltrasonic(); break;
  }

  if (REVERSE) {
    SensorValue = 1 - SensorValue;
  }

  // ↓ Uncomment to publish SensorValue to Particle cloud every PUBLISH_WAIT loops
  // publishNumber(SensorValue);

  switch (ACTUATOR) {
    case ACTUATOR_7SEG:
      write7Seg(SensorValue); break;
    case ACTUATOR_BUZZ:
      writeBuzz(SensorValue); break;
    case ACTUATOR_LED:
      writeLed(SensorValue); break;
  }

  delay(INTERVAL);
}
