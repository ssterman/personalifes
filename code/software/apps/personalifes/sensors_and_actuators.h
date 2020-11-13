
// Light sensors (4) require analog inputs.  
uint8_t LIGHT0_PIN = 2;  // AIN0
uint8_t LIGHT1_PIN = 3;  // AIN1
uint8_t LIGHT2_PIN = 4;  // AIN2
uint8_t LIGHT3_PIN = 5;  // AIN3

// Motion sensor (1) requires digital input.
uint8_t MOTION_PIN = 7;

//Touch sensor (up to 5) requires digital input. 
uint8_t TOUCH0_PIN = 12;
uint8_t	TOUCH1_PIN = 13;
uint8_t	TOUCH2_PIN = 14;
uint8_t	TOUCH3_PIN = 15;
uint8_t	TOUCH4_PIN = 16;

// Motor uses Pin 6 and Pin 8 for UART commands to the robot

// SMA requires 1 digital output per strand: 
uint8_t SMA_PIN = 11;

// LEDS require 3 PWM'd digital outputs per addressable LED
// PWM module can accept any GPIO pin 0 - 31
// FOR NOW, assuming that all LEDs are the same colors
uint8_t LED_R_PIN = 18;
uint8_t LED_G_PIN = 19;
uint8_t LED_B_PIN = 20;

typedef struct {
	uint8_t light1;
	uint8_t light2;
	uint8_t light3;
	uint8_t light4;
} light_values_t;

typedef struct {
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

void set_LED_color(uint8_t r, uint8_t g, uint8_t b);

