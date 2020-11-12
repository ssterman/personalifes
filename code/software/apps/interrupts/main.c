// Blink app
//
// Blinks the LEDs on Buckler

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "app_error.h"
#include "app_timer.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_gpio.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_serial.h"
#include "software_interrupt.h"

#include "gpio.h"

#include "buckler.h"

uint8_t LED0 = 25;
uint8_t SWITCH0 = 22;

void SWI1_EGU1_IRQHandler(void) {
    NRF_EGU1->EVENTS_TRIGGERED[0] = 0;
    uint8_t t = 0;
    printf("start software_interrupt\n");
    while(t < 20) {
      printf("loop inside software interrupt\n");
      nrf_delay_ms(100);
      t++;
    }
    printf("end software interrupt\n");
}

void GPIOTE_IRQHandler(void) {
    NRF_GPIOTE->EVENTS_IN[0] = 0;
    printf("button interrupt\n");
    gpio_clear(LED0);
    nrf_delay_ms(250);
    gpio_set(LED0);
}

int main(void) {
  ret_code_t error_code = NRF_SUCCESS;

  // initialize RTT library
  error_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(error_code);
  NRF_LOG_DEFAULT_BACKENDS_INIT();
  printf("Log initialized!\n");

  //set up interrupt for falling button edge
  //gpiote config[0] - set up event for hi to lo transition
  NRF_GPIOTE->CONFIG[0] |= 0b100001110000000001;
  // enable gpiote interrupt using intenset register
  NRF_GPIOTE->INTENSET |= 1; 
  //enable gpiote interrupt in nvic
  NVIC_EnableIRQ(GPIOTE_IRQn);
  NVIC_SetPriority(GPIOTE_IRQn, 0);
  NVIC_SetPriority(SWI1_EGU1_IRQn, 1);

  software_interrupt_init();

  //set LED0,1 as output
  gpio_config(LED0, 1);
  gpio_set(LED0);

  gpio_config(SWITCH0, 0);

  while (1) {
    printf("Looping\n");
    software_interrupt_generate();
    nrf_delay_ms(1000);
    if (gpio_read(SWITCH0)) {
      __WFI();
    } 
  }
}

