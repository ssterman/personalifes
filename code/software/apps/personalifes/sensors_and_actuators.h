#pragma once

#include "nrf.h"
#include "stdbool.h"

// Light sensors (4) require analog inputs.
// NOTE the defined values are the controls; the PINs used are in comments  
// #define LIGHT0_PIN 1 // Pin 2  // AIN0
// #define LIGHT1_PIN 2 // Pin 3  // AIN1
// #define LIGHT2_PIN 3 // Pin 4  // AIN2
// #define LIGHT3_PIN 4 // Pin 5  // AIN3

#define LIGHT1_PIN 1 // Pin 2  // AIN0
#define LIGHT3_PIN 2 // Pin 3  // AIN1
#define LIGHT2_PIN 3 // Pin 4  // AIN2
#define LIGHT0_PIN 4 // Pin 5  // AIN3

// Motion sensor (1) requires digital input.
#define MOTION_PIN  7

//Touch sensor (up to 5) requires digital input. 
#define TOUCH0_PIN  12
#define	TOUCH1_PIN  13
#define	TOUCH2_PIN  14
#define	TOUCH3_PIN  17
#define	TOUCH4_PIN  16

// Motor uses Pin 6 and Pin 8 for UART commands to the robot

// SMA requires 1 digital output per strand: 
#define SMA_PIN  11

// LEDS require 3 PWM'd digital outputs per addressable LED
// PWM module can accept any GPIO pin 0 - 31
// FOR NOW, assuming that all LEDs are the same colors
#define LED_R_PIN  18
#define LED_G_PIN  20
#define LED_B_PIN  19

//light sensors
#define MAX_LIGHT 150
#define MIN_LIGHT 10
//#define SCALER (255/(MAX_LIGHT-MIN_LIGHT))
#define SCALER 1.82

//LIGHT SENSOR DIRECTIONS
#define FRONT 1 // pin 2
#define RIGHT 2 // pin 3
#define BACK 3 // pin 4
#define LEFT 4 // pin 5

typedef struct {
	int16_t light1;
	int16_t light2;
	int16_t light3;
	int16_t light4;
} light_values_t;

typedef struct {
	bool touch0;
	bool touch1;
	bool touch2;
	bool touch3;
	bool touch4;
} touch_values_t;

//*********************
// Initialization for sensors
//*********************
void initialize_motion_sensor();

void initialize_light_sensors();

void initialize_touch_sensors();


//*********************
// Initialization for actuators
//*********************

void initialize_motor();

void initialize_SMA();

void initialize_LED();

//*********************
// Read functions
//*********************

bool read_motion_sensor();

light_values_t read_light_sensors();

touch_values_t read_touch_sensors();

//*********************
// Write functions
//*********************

// Motor is controlled via kobukiDriveDirect, which must be called repeatedly. 

void turn_SMA_on();

void turn_SMA_off();

void set_LED_color(uint32_t r, uint32_t g, uint32_t b);

void flashLEDs(uint32_t time_diff);

void LEDS_ON();

void LEDS_OFF();
