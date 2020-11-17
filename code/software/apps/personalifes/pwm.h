#pragma once

#include "nrf.h"
#include "stdbool.h"

#define PWM0_addr (volatile uint32_t *) 0x4001C000
#define PWM1_addr (volatile uint32_t *) 0x40021000
#define PWM2_addr (volatile uint32_t *) 0x40022000

#define TASKS_OFFSET  0x004
#define CONFIG_OFFSET  0x500
#define SEQ_0_OFFSET  0x520
#define SEQ_1_OFFSET  0x540
#define PSEL_OFFSET  0x560

typedef struct {
	uint32_t TASKS_STOP;
	uint32_t TASKS_SEQSTART_0;
	uint32_t TASKS_SEQSTART_1;
	uint32_t TASKS_NEXTSTEP;
	uint32_t EVENTS_STOPPED;
	uint32_t EVENTS_SEQSTARTED_0;
	uint32_t EVENTS_SEQSTARTED_1;
	uint32_t EVENTS_SEQEND_0;
	uint32_t EVENTS_SEQEND_1;
	uint32_t EVENTS_PWMPERIODEND;
	uint32_t EVENTS_LOOPSDONE;
} PWM_TASKS_struct;

typedef struct {
	uint32_t ENABLE;
	uint32_t MODE;
	uint32_t COUNTERTOP;
	uint32_t PRESCALER;
	uint32_t DECODER;
	uint32_t LOOP;
} PWM_CONFIG_struct;

typedef struct {
	uint32_t SEQ_PTR;
	uint32_t SEQ_CNT;
	uint32_t SEQ_REFRESH;
	uint32_t SEQ_ENDDELAY;
} PWM_SEQ_0_struct;

typedef struct {
	uint32_t SEQ_PTR;
	uint32_t SEQ_CNT;
	uint32_t SEQ_REFRESH;
	uint32_t SEQ_ENDDELAY;
} PWM_SEQ_1_struct;

typedef struct {
	uint32_t PSEL_OUT_0;
	uint32_t PSEL_OUT_1;
	uint32_t PSEL_OUT_2;
	uint32_t PSEL_OUT_3;
} PWM_PSEL_struct;

void pwm_configure_pin(uint32_t channel, uint32_t pin, uint32_t connect);
void pwm_enable();
void pwm_disable();
void pwm_set_mode(uint32_t mode);
void pwm_set_prescaler(uint32_t prescaler);
void pwm_set_countertop(uint32_t top);
void pwm_set_loop(uint32_t playback);
void pwm_set_decoder(uint32_t load, uint32_t mode);
void pwm_set_sequence(uint32_t sequence, uint32_t ptr, uint32_t count, uint32_t refresh, uint32_t enddelay);
void pwm_start(uint32_t sequence);
void pwm_stop();
