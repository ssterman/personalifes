// Blink app
//
// Blinks the LEDs on Buckler

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "app_error.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_serial.h"

#include "buckler.h"
#include "gpio.h"

//base address for GPIO
//0x50000000

//offsets 
//OUT = 0x504
//DIR = 0x514

// typedef struct {
// 	uint32_t OUT;
// 	uint32_t OUTSET;
// 	uint32_t OUTCLR;
// 	uint32_t IN;
// 	uint32_t DIR;
// 	uint32_t DIRSET;
// 	uint32_t DIRCLR;
// 	uint32_t LATCH;
// 	uint32_t DETECTMODE;
// } GPIO_REGISTERS;

// typedef struct {
// 	uint32_t PIN_CNF[32];
// } PIN_CONFIGURATIONS; 


int main(void) {
  ret_code_t error_code = NRF_SUCCESS;

  // initialize RTT library
  error_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(error_code);
  NRF_LOG_DEFAULT_BACKENDS_INIT();
  printf("Log initialized!\n");

  printf("4.2.2 print values and addresses\n");

  // GPIO_REGISTERS* gpio_registers = (GPIO_REGISTERS*) 0x50000504;
  // PIN_CONFIGURATIONS* pin_configs = (PIN_CONFIGURATIONS*) 0x50000700;

  // uint32_t* OUT = & (gpio_registers->OUT); //(uint32_t*)0x50000504;
  // uint32_t* DIR = & (gpio_registers->DIR); //(uint32_t*)0x50000514;

  // printf("OUT: address - %p, value - %lx\n", OUT, *OUT);
  // printf("DIR: address - %p, value - %lx\n", DIR, *DIR);

  //make button0 and switch0 into inputs
  //button0 is 28
  // uint32_t* BUTTON0 = & (pin_configs->PIN_CNF[28]);
  // uint32_t* SWITCH0 = & (pin_configs->PIN_CNF[22]);

  //set pin direction to input (0) -> make position 1 into 0
  //connect input buffer to pin -> make position 2 into 0
  // *BUTTON0 = *BUTTON0 & 0xFFFFFFFC;
  // *SWITCH0 = *SWITCH0 & 0xFFFFFFFC;

  //set up for blinking LED
  // *DIR = 1 << 24;
  // uint32_t mask = 1 << 24;
  
  uint8_t LED0 = 25;
  uint8_t LED1 = 24;
  uint8_t BUTTON0 = 28;
  uint8_t SWITCH0 = 22;


  printf("before config\n");
  //set LED0,1 as output
  gpio_config(LED0, 1);
  gpio_config(LED1, 1);

  //set BUTTON0, SWITCH0 as input
  gpio_config(BUTTON0, 0);
  gpio_config(SWITCH0, 0);

  printf("before while\n");

  while (1) {
  	//make the LED blink
  	// nrf_delay_ms(250);
  	// *OUT = *OUT ^ mask;
  	nrf_delay_ms(250);
  	// *OUT = *OUT ^ mask;

   //printf("BUTTON0: address - %p, value - %lx\n", BUTTON0, gpio_registers->IN << 3 >> 31);
   // printf("SWITCH0: address - %p, value - %lx\n", SWITCH0, gpio_registers->IN << 9 >> 31);
 	
 	printf("BUTTON0: %lx\n", gpio_read(BUTTON0));

 	if (gpio_read(BUTTON0)) {
	 	gpio_set(LED0);
	} else {
	 	gpio_clear(LED0);
	}

	if (gpio_read(SWITCH0)) {
	 	gpio_set(LED1);
	} else {
	 	gpio_clear(LED1);
	}
  }
}

