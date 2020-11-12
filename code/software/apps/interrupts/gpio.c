#include "gpio.h"

GPIO_REGISTERS* gpio_registers = (GPIO_REGISTERS*) 0x50000504;
PIN_CONFIGURATIONS* pin_configs = (PIN_CONFIGURATIONS*) 0x50000700;

// Inputs: 
//  gpio_num - gpio number 0-31
//  dir - gpio direction (INPUT, OUTPUT)
void gpio_config(uint8_t gpio_num, gpio_direction_t dir) {
	//printf("dir, %d\n", dir);
	if (dir) {
		pin_configs->PIN_CNF[gpio_num] |= 3;
		//printf("output");
	} else {
		pin_configs->PIN_CNF[gpio_num] &= (0xFFFFFFFC);
		//printf("input");
	}
}

// Set gpio_num high
// Inputs: 
//  gpio_num - gpio number 0-31
void gpio_set(uint8_t gpio_num) {
	gpio_registers->OUT |= 1 << gpio_num; 
	//printf("set %lx\n", gpio_registers->OUT);
}

// Set gpio_num low
// Inputs: 
//  gpio_num - gpio number 0-31
void gpio_clear(uint8_t gpio_num) {
	gpio_registers->OUT &= ~(1 << gpio_num);
	//printf("clear %lx\n", gpio_registers->OUT);

}

// Inputs: 
//  gpio_num - gpio number 0-31
bool gpio_read(uint8_t gpio_num) {
    // should return pin state
    return gpio_registers->IN << (31-gpio_num) >> 31;
}
