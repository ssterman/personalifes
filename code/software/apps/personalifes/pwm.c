
#include "pwm.h"

// For PWM 0
// need to make some changes if also want to use PWM 1 or 2
volatile PWM_CONFIG_struct* PWM_configuration = (volatile PWM_CONFIG_struct *) (PWM0_addr + CONFIG_OFFSET);
volatile PWM_PSEL_struct* PWM_pin_select = (volatile PWM_PSEL_struct *) (PWM0_addr + PSEL_OFFSET);


void pwm_configure_pin(uint32_t channel, uint32_t pin, uint32_t connect) {
	uint32_t * addr; 
	switch(channel) {
		case 0:
			addr = PWM_PSEL_struct->PSEL_OUT_0;
			break;
		case 0:
			addr = PWM_PSEL_struct->PSEL_OUT_1;
			break;
		case 0:
			addr = PWM_PSEL_struct->PSEL_OUT_2;
			break;
		case 0:
			addr = PWM_PSEL_struct->PSEL_OUT_3;
			break;
		default:
			return;
	}
	*addr = (pin + (connect << 31)) + 0x7FFFFFE0;
}

void pwm_enable() {
	PWM_configuration->ENABLE = 1;
}

void pwm_disable() {
	PWM_configuration->ENABLE = 0;
}

// Up     0 
// UpAndDown     1
void pwm_set_mode(uint32_t mode) {
	PWM_configuration->MODE = mode;
}

// Pre-scaler of PWM_CLK
// 0	Divide by 1 (16MHz)
// 1 	Divide by 2 ( 8MHz) 
// 2 	Divide by 4 ( 4MHz) 
// 3 	Divide by 8 ( 2MHz) 
// 4 	Divide by 16 ( 1MHz) 
// 5 	Divide by 32 ( 500kHz) 
// 6 	Divide by 64 ( 250kHz) 
// 7 	Divide by 128 ( 125kHz)
void pwm_set_prescaler(uint32_t prescaler) {
	PWM_configuration->PRESCALER = prescaler;
}

// values: [3..32767]
// Value up to which the pulse generator counter counts. 
// This register is ignored when DECODER.MODE=WaveForm and only values from RAM will be used.
void pwm_set_countertop(uint32_t top) {
	PWM_configuration->COUNTERTOP = top;
}

void pwm_set_loop(uint32_t playback) {
	PWM_configuration->LOOP = playback;
}

void pwm_set_decoder() {

}

void pwm_set_sequence() {

}

void pwm_start() {

}

void pwm_stop() {

}
