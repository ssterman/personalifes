#include "gpio.h"

volatile GPIO_struct* GPIO_reg_addr = (volatile GPIO_struct *) GPIO_OUT_addr;
volatile PIN_CNF_struct* PIN_CNF_reg_addr = (volatile PIN_CNF_struct *) PIN_CNF_addr;
// Inputs: 
//  gpio_num - gpio number 0-31
//  dir - gpio direction (INPUT, OUTPUT)
void gpio_config(uint8_t gpio_num, gpio_direction_t dir) {
	if (dir == INPUT) {
		PIN_CNF_reg_addr->PIN_CNF[gpio_num] = 0 | (0<< 1);
	}
	else {
		PIN_CNF_reg_addr->PIN_CNF[gpio_num] = 1 | (0<< 1);
	}
}

// Set gpio_num high
// Inputs: 
//  gpio_num - gpio number 0-31
void gpio_set(uint8_t gpio_num) {
	//GPIO_reg_addr->GPIO_OUT = (GPIO_reg_addr->GPIO_OUT | (1 << gpio_num));
	GPIO_reg_addr->GPIO_OUT = (GPIO_reg_addr->GPIO_OUT & !(1 << gpio_num));
}

// Set gpio_num low
// Inputs: 
//  gpio_num - gpio number 0-31
void gpio_clear(uint8_t gpio_num) {
	GPIO_reg_addr->GPIO_OUT = (GPIO_reg_addr->GPIO_OUT | (1 << gpio_num));
	//GPIO_reg_addr->GPIO_OUT = (GPIO_reg_addr->GPIO_OUT & !(1 << gpio_num));
}

// Inputs: 
//  gpio_num - gpio number 0-31
bool gpio_read(uint8_t gpio_num) {
    // should return pin state
    uint32_t Reading = GPIO_reg_addr->GPIO_IN;
  	uint32_t mask = 1<<gpio_num;
    return (Reading & mask)>>gpio_num;
}
