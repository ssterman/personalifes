#include "saadc.h"

volatile SAADC_CONFIG_struct* SAADC_configuration = (volatile SAADC_CONFIG_struct *) SAADC_CONFIG_addr;
volatile SAADC_SAMPLE_struct* SAADC_sample = (volatile SAADC_SAMPLE_struct *) SAADC_SAMPLE_addr;

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

void set_result_pointer(uint32_t ptr) {
	SAADC_sample->RESULT_PTR = ptr;
}

void set_result_maxcnt(uint32_t number_words) {
	SAADC_sample->RESULT_MAXCNT = number_words;
}


//********************
// set pin
//********************
void saadc_set_pin_channel(uint32_t channel, uint32_t analog_pin) {
	uint32_t config; 
	switch(channel) {
		case 0:
			config = SAADC_configuration->CH0_PSELP;
			break;
		case 1:
			config = SAADC_configuration->CH1_PSELP;
			break;
		case 2:
			config = SAADC_configuration->CH2_PSELP;
			break;
		case 3:
			config = SAADC_configuration->CH3_PSELP;
			break;
		case 4:	
			config = SAADC_configuration->CH4_PSELP;
			break;
		case 5:
			config = SAADC_configuration->CH5_PSELP;
			break;
		case 6:
			config = SAADC_configuration->CH6_PSELP;
			break;
		case 7:
			config = SAADC_configuration->CH7_PSELP;
			break;
		default:
			return;
	}
	config = analog_pin;
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
	(volatile uint32_t *) config; 
	switch(channel) {
		case 0:
			config = SAADC_configuration->CH0_CONFIG;
			break;
		case 1:
			config = SAADC_configuration->CH1_CONFIG;
			break;
		case 2:
			config = SAADC_configuration->CH2_CONFIG;
			break;
		case 3:
			config = SAADC_configuration->CH3_CONFIG;
			break;
		case 4:	
			config = SAADC_configuration->CH4_CONFIG;
				break;
		case 5:
			config = SAADC_configuration->CH5_CONFIG;
			break;
		case 6:
			config = SAADC_configuration->CH6_CONFIG;
			break;
		case 7:
			config = SAADC_configuration->CH7_CONFIG;
			break;
		default:
			return;
	}
	config |= (resp);
	config |= (resn << 4);
	config |= (gain << 8);
	config |= (refsel << 12);
	config |= (tacq << 16);
	config |= (mode << 20);
	config |= (burst << 24);
}