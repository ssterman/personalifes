#include "saadc.h"

volatile SAADC_CONFIG_struct* SAADC_configuration = (volatile SAADC_CONFIG_struct *) SAADC_CONFIG_addr;
volatile SAADC_SAMPLE_struct* SAADC_sample = (volatile SAADC_SAMPLE_struct *) SAADC_SAMPLE_addr;
volatile SAADC_TASKS_struct* SAADC_tasks = (volatile SAADC_TASKS_struct *) SAADC_TASKS_addr;
volatile SAADC_EVENTS_struct* SAADC_events = (volatile SAADC_EVENTS_struct *) SAADC_EVENTS_addr;

//*******************
// enable or disable
//*******************

void saadc_enable() {
	(*SAADC_ENABLE_addr) = 1;
}

void saadc_disable() {
	(*SAADC_ENABLE_addr) = 0;
}

//*******************
// sampling
//*******************

void saadc_start() {
	SAADC_tasks->TASKS_START = 1;
}

void saadc_sample() {
	SAADC_tasks->TASKS_SAMPLE = 1;
}

bool saadc_result_ready() {
	return SAADC_events->EVENTS_END;
}

void saadc_clear_result_ready() {
	SAADC_events->EVENTS_END = 0;
}

//*******************
// 8bit 	0 
// 10bit 	1 
// 12bit 	2 
// 14bit 	3 
//*******************
void saadc_set_resolution(uint32_t resolution) {
	SAADC_sample->RESOLUTION = resolution;
}

//*******************
// CC: [80..2047] Capture and compare value. Sample rate is 16 MHz/CC
// Select mode for sample rate control
// 		Task 	0	Rate is controlled from SAMPLE task
// 		Timers  1 	Rate is controlled from local timer (use CC to control the rate)
//*******************
void set_sample_rate(uint32_t cc, uint32_t mode) {
	SAADC_sample->SAMPLERATE |= cc;
	SAADC_sample->SAMPLERATE |= (mode << 12);
}


//*******************
// set retrieval info
//*******************

void set_result_pointer(uint32_t * ptr) {
	SAADC_sample->RESULT_PTR = (uint32_t) ptr;
}

void set_result_maxcnt(uint32_t number_words) {
	SAADC_sample->RESULT_MAXCNT = number_words;
}


//********************
// set pin
//********************
void saadc_set_pin_channel(uint32_t channel, uint32_t analog_pin) {
	switch(channel) {
		case 0:
			SAADC_configuration->CH0_PSELP = analog_pin;
			break;
		case 1:
			SAADC_configuration->CH1_PSELP = analog_pin;
			break;
		case 2:
			SAADC_configuration->CH2_PSELP = analog_pin;
			break;
		case 3:
			SAADC_configuration->CH3_PSELP = analog_pin;
			break;
		case 4:	
			SAADC_configuration->CH4_PSELP = analog_pin;
			break;
		case 5:
			SAADC_configuration->CH5_PSELP = analog_pin;
			break;
		case 6:
			SAADC_configuration->CH6_PSELP = analog_pin;
			break;
		case 7:
			SAADC_configuration->CH7_PSELP = analog_pin;
			break;
		default:
			return;
	}
}

//********************
// configure channel
//********************

//defaults:
// burst: 0 (off)
// mode: 0 (single ended)
// tacq: 2 (10 microseconds)
// refsel: 0 (internal)
// gain: 5 (1)
// resn: 0 (bypass)
// resp: 1 (pulldown to ground)
void saadc_configure_channel(uint32_t channel, uint32_t resp, uint32_t resn, uint32_t gain, uint32_t refsel, uint32_t tacq, uint32_t mode, uint32_t burst) {
	uint32_t value = (resp) +  (resn << 4) + (gain << 8) + (refsel << 12) + (tacq << 16) + (mode << 20) + (burst << 24);
	switch(channel) {
		case 0:
			SAADC_configuration->CH0_CONFIG = value;
			break;
		case 1:
			SAADC_configuration->CH1_CONFIG = value;
			break;
		case 2:
			SAADC_configuration->CH2_CONFIG = value;
			break;
		case 3:
			SAADC_configuration->CH3_CONFIG = value;
			break;
		case 4:	
			SAADC_configuration->CH4_CONFIG = value;
				break;
		case 5:
			SAADC_configuration->CH5_CONFIG = value;
			break;
		case 6:
			SAADC_configuration->CH6_CONFIG = value;
			break;
		case 7:
			SAADC_configuration->CH7_CONFIG = value;
			break;
		default:
			return;
	}
}
