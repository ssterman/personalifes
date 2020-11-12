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
#include "gpio.h"// Blink app
//
// Blinks the LEDs on Buckler





int main(void) {
  ret_code_t error_code = NRF_SUCCESS;

  // initialize RTT library
  error_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(error_code);
  NRF_LOG_DEFAULT_BACKENDS_INIT();
  printf("Log initialized!\n");
  //uint32_t GPIO_OUT_value = GPIO_reg_addr->GPIO_OUT;
  //uint32_t GPIO_DIR_value = GPIO_reg_addr->GPIO_DIR;
  //set LED23 to 1 and then 0
  //printf("GPIO OUT address: %p \n", *GPIO_reg_addr->GPIO_OUT);
  //printf("GPIO DIR address: %p \n", *GPIO_reg_addr->GPIO_DIR);
  //printf("GPIO OUT value: %ld \n", GPIO_OUT_value);
  //printf("GPIO DIR value: %ld \n", GPIO_DIR_value);
  gpio_config(23, OUTPUT);
  gpio_config(24, OUTPUT);
  gpio_config(25, OUTPUT);
  gpio_config(28, INPUT);
  gpio_config(22, INPUT);
  uint32_t *gpio_out = (uint32_t *) GPIO_OUT_addr;
  uint32_t gpio_out_value;

  //GPIO_reg_addr->GPIO_DIR = (1 << 23) | (1<<24) | (1<<25);
  //PIN_CNF_reg_addr->PIN_CNF[28] = 0 | (0<< 1);

  //PIN_CNF_reg_addr->PIN_CNF[22] = 0 | (0<< 1);
  // loop forever
  while (1) {
    bool button_b = gpio_read(28);
  	bool switch_b = gpio_read(22);
    gpio_out_value = *gpio_out;
  	//printf("GPIO button value: %d\n", button_b);
  	//printf("GPIO switch value: %d\n", switch_b);
    printf("GPIO_OUT button: %08x\n", gpio_out_value);

   // nrf_delay_ms(100);
    //printf("GPIO_OUT button: %x\n", gpio_out_value);
    if(switch_b){
    	gpio_set(24);
    }
    else{
    	gpio_clear(24);
    }
    if (button_b) {
      gpio_set(25);
      //printf("%d\n", button_b);
    }
    else{
      gpio_clear(25);
      //printf("%d\n", button_b);
    }
    //printf("GPIO_OUT switch: %x\n", gpio_out_value);
   //nrf_delay_ms(100);
   //  gpio_set(23);
   //  gpio_set(24);
   //  gpio_set(25);
  	// nrf_delay_ms(200);
  	// gpio_clear(23);
   //  gpio_clear(24);
   //  gpio_clear(25);
  	// nrf_delay_ms(200);
   //  gpio_set(23);
   //  gpio_set(24);
   //  gpio_set(25);
  	// nrf_delay_ms(200);
  	// gpio_set(23);
   //  gpio_set(24);
   //  gpio_set(25);
  	// nrf_delay_ms(200);

  }
}
