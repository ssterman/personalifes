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

#include "buckler.h"
#include "gpio.h"

void SWI1_EGU1_IRQHandler(void) {
    NRF_EGU1->EVENTS_TRIGGERED[0] = 0;
    printf("Software interrupt occured!\n");
    nrf_delay_ms(5000);
    printf("Done with software interrupt!\n");
}

void GPIOTE_IRQHandler(void) {
    NRF_GPIOTE->EVENTS_IN[0] = 0;
    gpio_set(25);
    printf("interrupt value: %08x\n", NRF_GPIOTE->EVENTS_IN[0]);
    printf("GPIO interrupt occured!\n");
    nrf_delay_ms(500);
    gpio_clear(25);
    //nrf_delay_ms(1000);
    printf("Done with GPIO interrupt\n");
}

int main(void) {
  ret_code_t error_code = NRF_SUCCESS;

  // initialize RTT library
  error_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(error_code);
  NRF_LOG_DEFAULT_BACKENDS_INIT();
  printf("Log initialized!\n");
  NRF_GPIOTE->CONFIG[0] = 1 | (28 << 8) | (2 << 16);
  NRF_GPIOTE->INTENSET = 1;
  NVIC_EnableIRQ(GPIOTE_IRQn);
  gpio_config(25, OUTPUT);
  gpio_clear(25);
  // loop forever

  //checkoff 1
  while (1) {
    printf("Looping\n");
    //printf("interrupt value: %08x\n", NRF_GPIOTE->EVENTS_IN[0]);
    //if (NRF_GPIOTE->EVENTS_IN[0] != 0){
    //  GPIOTE_IRQHandler();
    //}
    nrf_delay_ms(1000);
  }

  //checkoff 2
  // gpio_config(22, INPUT);
  // while (1) {
  //   printf("Looping\n");
  //   bool switch_b = gpio_read(22);
  //   if (switch_b) {
  //   //printf("interrupt value: %08x\n", NRF_GPIOTE->EVENTS_IN[0]);
  //     __WFI();
  //     //if (NRF_GPIOTE->EVENTS_IN[0] != 0){
  //     //  GPIOTE_IRQHandler();
  //     //}
  //   }
  //   else {
  //     //if (NRF_GPIOTE->EVENTS_IN[0] != 0){
  //     //  GPIOTE_IRQHandler();
  //     //}
  //   }
  //   nrf_delay_ms(1000);
  // }
    
  //checkoff 3
  // gpio_config(22, INPUT);
  // software_interrupt_init();
  // NVIC_SetPriority(GPIOTE_IRQn, 0);
  // NVIC_SetPriority(SWI1_EGU1_IRQn, 0);
  // while (1) {
  //   printf("Looping\n");
  //   bool switch_b = gpio_read(22);
  //   if (switch_b) {
  //   //printf("interrupt value: %08x\n", NRF_GPIOTE->EVENTS_IN[0]);
  //     software_interrupt_generate();
  //   }
  //   //if (NRF_GPIOTE->EVENTS_IN[0] != 0){
  //   //    GPIOTE_IRQHandler();
  //   //}
  //   nrf_delay_ms(1000);
  // }

}


