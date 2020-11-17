#pragma once

#include "nrf.h"
#include "stdbool.h"


#define SAADC_TASKS_addr (volatile uint32_t *) 0x40007000
#define SAADC_EVENTS_addr (volatile uint32_t *) 0x40007100
#define SAADC_INTERRUPTS_addr (volatile uint32_t *) 0x40007300
#define SAADC_STATUS_addr (volatile uint32_t *) 0x40007400
#define SAADC_ENABLE_addr (volatile uint32_t *) 0x40007500
#define SAADC_CONFIG_addr (volatile uint32_t *) 0x40007510
#define SAADC_SAMPLE_addr (volatile uint32_t *) 0x400075F0
#define SAADC_RESULT_addr (volatile uint32_t *) 0x4000762C

// 0 4 8 C 10; 
typedef struct {
	uint32_t TASKS_START;
	uint32_t TASKS_SAMPLE;
	uint32_t TASKS_STOP;
	uint32_t TASKS_CALIBRATEOFFSET;
} SAADC_TASKS_struct;

typedef struct {
	uint32_t EVENTS_STARTED;
	uint32_t EVENTS_END;
	uint32_t EVENTS_DONE;
	uint32_t EVENTS_RESULTDONE;
	uint32_t EVENTS_CALIBRATEDONE;
	uint32_t EVENTS_STOPPED;
	// individual channel events follow here
} SAADC_EVENTS_struct;

typedef struct {
	uint32_t INTEN;
	uint32_t INTENSET;
	uint32_t INTENCLR;
} SAADC_INTERRUPTS_struct;

typedef struct {
	uint32_t CH0_PSELP;
	uint32_t CH0_PSELN;
	uint32_t CH0_CONFIG;
	uint32_t CH0_LIMIT;
	uint32_t CH1_PSELP;
	uint32_t CH1_PSELN;
	uint32_t CH1_CONFIG;
	uint32_t CH1_LIMIT;
	uint32_t CH2_PSELP;
	uint32_t CH2_PSELN;
	uint32_t CH2_CONFIG;
	uint32_t CH2_LIMIT;
	uint32_t CH3_PSELP;
	uint32_t CH3_PSELN;
	uint32_t CH3_CONFIG;
	uint32_t CH3_LIMIT;
	uint32_t CH4_PSELP;
	uint32_t CH4_PSELN;
	uint32_t CH4_CONFIG;
	uint32_t CH4_LIMIT;
	uint32_t CH5_PSELP;
	uint32_t CH5_PSELN;
	uint32_t CH5_CONFIG;
	uint32_t CH5_LIMIT;
	uint32_t CH6_PSELP;
	uint32_t CH6_PSELN;
	uint32_t CH6_CONFIG;
	uint32_t CH6_LIMIT;
	uint32_t CH7_PSELP;
	uint32_t CH7_PSELN;
	uint32_t CH7_CONFIG;
	uint32_t CH7_LIMIT;
} SAADC_CONFIG_struct;

typedef struct {
	uint32_t RESOLUTION;
	uint32_t OVERSAMPLE;
	uint32_t SAMPLERATE;
} SAADC_SAMPLE_struct;

typedef struct {
	uint32_t RESULT_PTR;
	uint32_t RESULT_MAXCNT;
	uint32_t RESULT_AMOUNT;
} SAADC_RESULT_struct;

void saadc_enable();
void saadc_disable();
void saadc_start();
void saadc_sample();
bool saadc_result_ready();
void saadc_clear_result_ready();
void saadc_set_resolution(uint32_t resolution);
void set_sample_rate(uint32_t cc, uint32_t mode);
void set_result_pointer(uint32_t ptr);
void set_result_maxcnt(uint32_t number_words);
void saadc_set_pin_channel(uint32_t channel, uint32_t analog_pin);
void saadc_configure_channel(uint32_t channel, uint32_t resp, uint32_t resn, uint32_t gain, uint32_t refsel, uint32_t tacq, uint32_t mode, uint32_t burst);

