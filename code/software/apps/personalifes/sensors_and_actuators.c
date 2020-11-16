#include "sensors_and_actuators.h"
#include "gpio.h"

//*********************
// Initialization for sensors
//*********************

// set up as digital input
void initialize_motion_sensor() {
	gpio_config(MOTION_PIN, INPUT);
}

// setup as analog inputs
// setup the SAAADC
void initialize_light_sensors() {

	//TODO set up SAADC and analog in

}

// set up as digital input
void initialize_touch_sensors() {
	gpio_config(TOUCH0_PIN, INPUT);
	gpio_config(TOUCH1_PIN, INPUT);
	gpio_config(TOUCH2_PIN, INPUT);
	gpio_config(TOUCH3_PIN, INPUT);
	gpio_config(TOUCH4_PIN, INPUT);
}


//*********************
// Initialization for actuators
//*********************

//initialize kobuki
void initialize_motor(){
  kobukiInit();
  printf("Kobuki initialized!\n");
}

void initialize_SMA(){
	gpio_config(SMA_PIN, OUTPUT);
}

// must configure PWM
void initialize_LED(){
	gpio_config(LED_R_PIN, OUTPUT);
	gpio_config(LED_G_PIN, OUTPUT);
	gpio_config(LED_B_PIN, OUTPUT);

	//TODO configure PWM
}

//*********************
// Read functions
//*********************

bool read_motion_sensor(){
	return gpio_read(MOTION_PIN);
}

light_values_t read_light_sensors(){
	light_values_t result;
	//todo read from SAADC
}

touch_values_t read_touch_sensors(){
	touch_values_t result;
	result.touch0 = gpio_read(TOUCH0_PIN);
	result.touch1 = gpio_read(TOUCH1_PIN);
	result.touch2 = gpio_read(TOUCH2_PIN);
	result.touch3 = gpio_read(TOUCH3_PIN);
	result.touch4 = gpio_read(TOUCH4_PIN);
	return result;
}

//*********************
// Write functions
//*********************


void turn_SMA_on(){
	gpio_set(SMA_PIN);
}

void turn_SMA_off(){
	gpio_clear(SMA_PIN);
}

void set_LED_color(uint8_t r, uint8_t g, uint8_t b){
	//TODO write via PWM module
}

void flashLEDs(){
	//write flash LEDs order
}

