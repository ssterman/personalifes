#include "sensors_and_actuators.h"
#include "gpio.h"
#include "saadc.h"
#include "pwm.h"
#include "nrf_delay.h"
#include "kobukiActuator.h"
#include "kobukiSensorPoll.h"
#include "kobukiSensorTypes.h"
#include "kobukiUtilities.h"

//*********************
// Sensor Config
//*********************

// Light Sensors
typedef struct {
	int16_t result1;
	int16_t result2;
	int16_t result3;
	int16_t result4;
} LIGHT_SENSOR_RESULT_DATA;

LIGHT_SENSOR_RESULT_DATA lsrd;

uint32_t resolution = 0; // 8 bit
uint32_t cc = 80;  //don't think this matters b/c in sample mode
uint32_t samplemode = 0; // use sample mode
uint32_t * result_ptr = (uint32_t *) &lsrd;
uint32_t maxcnt = 4; //number of 32 bit words; must be >= num channels
uint32_t resp = 1; //pulldown to ground
uint32_t resn = 0; //bypass
uint32_t gain = 5; //1
uint32_t refsel = 0; //internal 
uint32_t tacq = 2; //10 microseconds
uint32_t mode = 0; //single ended
uint32_t burst = 0; //off

//*********************
// Initialization for sensors
//*********************

// set up as digital input
void initialize_motion_sensor() {
	gpio_config(MOTION_PIN, INPUT);
}

void initialize_light_sensors() {
	// Enable ADC
	saadc_enable();

	//configure mode: 
	saadc_set_resolution(resolution);
	set_sample_rate(cc,  samplemode);
	set_result_pointer(result_ptr);
	set_result_maxcnt(maxcnt);

	// configure pins for ch0-3; pselp and config
	saadc_set_pin_channel(0,  LIGHT0_PIN);
	saadc_set_pin_channel(1,  LIGHT1_PIN);
	saadc_set_pin_channel(2,  LIGHT2_PIN);
	saadc_set_pin_channel(3,  LIGHT3_PIN);

	saadc_configure_channel(0,  resp,  resn,  gain,  refsel,  tacq,  mode,  burst);
	saadc_configure_channel(1,  resp,  resn,  gain,  refsel,  tacq,  mode,  burst);
	saadc_configure_channel(2,  resp,  resn,  gain,  refsel,  tacq,  mode,  burst);
	saadc_configure_channel(3,  resp,  resn,  gain,  refsel,  tacq,  mode,  burst);

	// START adc
	saadc_start();
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

light_values_t read_light_sensors() {
	saadc_sample();
	while (!saadc_result_ready()) {
		nrf_delay_ms(1);
	}
	saadc_clear_result_ready();

	light_values_t lights; 
	lights.light1 = lsrd.result1;
	lights.light2 = lsrd.result2;
	lights.light3 = lsrd.result3;
	lights.light4 = lsrd.result4;

	return lights;
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

void set_LED_color(uint32_t r, uint32_t g, uint32_t b){
	//TODO write via PWM module
}

void flashLEDs(){
	//write flash LEDs order
}

