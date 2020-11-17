
#include "pwm.h"

// For PWM 0
// need to make some changes if also want to use PWM 1 or 2
volatile PWM_TASKS_struct* PWM_tasks = (volatile PWM_TASKS_struct *) (PWM0_addr + TASKS_OFFSET);
volatile PWM_CONFIG_struct* PWM_configuration = (volatile PWM_CONFIG_struct *) (PWM0_addr + CONFIG_OFFSET);
volatile PWM_PSEL_struct* PWM_pin_select = (volatile PWM_PSEL_struct *) (PWM0_addr + PSEL_OFFSET);
volatile PWM_SEQ_0_struct* PWM_seq_0 = (volatile PWM_SEQ_0_struct *) (PWM0_addr + SEQ_0_OFFSET);
volatile PWM_SEQ_1_struct* PWM_seq_1 = (volatile PWM_SEQ_1_struct *) (PWM0_addr + SEQ_1_OFFSET);


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

void pwm_set_decoder(uint32_t load, uint32_t mode) {
	PWM_configuration->DECODER = load + (mode << 8);
}

void pwm_set_sequence(uint32_t sequence, uint32_t ptr, uint32_t count, uint32_t refresh, uint32_t enddelay) {
	if (sequence == 0) {
		PWM_seq_0->SEQ_PTR = ptr;
		PWM_seq_0->SEQ_CNT = count;
		PWM_seq_0->SEQ_REFRESH = refresh;
		PWM_seq_0->SEQ_ENDDELAY = enddelay; 
	} else {
		PWM_seq_1->SEQ_PTR = ptr;
		PWM_seq_1->SEQ_CNT = count;
		PWM_seq_1->SEQ_REFRESH = refresh;
		PWM_seq_1->SEQ_ENDDELAY = enddelay; 
	}
}

void pwm_start(uint32_t sequence) {
	if (sequence == 0) {
		PWM_tasks->TASKS_SEQSTART_0 = 1;
	} else {
		PWM_tasks->TASKS_SEQSTART_1 = 1;
	}
}

void pwm_stop() {
	PWM_tasks->TASKS_STOP = 1;
}
