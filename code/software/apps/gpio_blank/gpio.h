#pragma once

#include "nrf.h"
#include "stdbool.h"

typedef enum {
    INPUT = 0,
    OUTPUT,
} gpio_direction_t;

#define GPIO_OUT_addr (volatile uint32_t *) 0x50000504;
#define PIN_CNF_addr (volatile uint32_t *) 0x50000700;
//uint32_t *GPIO_DIR = (uint32_t*) 0x50000514;

typedef struct {
	uint32_t GPIO_OUT;
	uint32_t GPIO_OUTSET;
	uint32_t GPIO_OUTCLR;
	uint32_t GPIO_IN;
	uint32_t GPIO_DIR;
	uint32_t GPIO_DIRSET;
	uint32_t GPIO_LATCH;
	uint32_t GPIO_DETECTMODE;
} GPIO_struct;

typedef struct {
	uint32_t PIN_CNF[32];
} PIN_CNF_struct;



// Inputs: 
//  gpio_num - gpio number 0-31
//  dir - gpio direction (INPUT, OUTPUT)
void gpio_config(uint8_t gpio_num, gpio_direction_t dir);

// Inputs: 
//  gpio_num - gpio number 0-31
void gpio_set(uint8_t gpio_num);

// Inputs: 
//  gpio_num - gpio number 0-31
void gpio_clear(uint8_t gpio_num);

// Inputs: 
//  gpio_num - gpio number 0-31
// Returns:
//  current state of the specified gpio pin
bool gpio_read(uint8_t gpio_num);
