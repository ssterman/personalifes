// Blink app
//
// Blinks the LEDs on Buckler

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "app_error.h"
#include "app_timer.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_serial.h"
#include "nrf_drv_spi.h"

#include "buckler.h"
#include "gpio.h"// Blink app
#include "sensors_and_actuators.h"

//driver for kobuki
#include "buckler.h"
#include "display.h"
#include "kobukiActuator.h"
#include "kobukiSensorPoll.h"
#include "kobukiSensorTypes.h"
#include "kobukiUtilities.h"
#include "lsm9ds1.h"

// I2C manager
NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

typedef enum {
  AMBIENT,
  ATTENTION,
  TOUCH,
  SCARED,
} flower_state_t;

// light_values_t current_light, previous_light;
// uint16_t = scared_light_thresh = ;
// uint32_t timer_thresh = , turn_thresh = , timer_scared_thresh = ;
// uint8_t r, b, g;
// bool motion_yn, facing_motion;
// touch_values_t touch_state;
// uint32_t current_time, previous_time, timer_start, timer_counter;

static float measure_distance(uint16_t current_encoder,
                              uint16_t previous_encoder) {
  const float CONVERSION = 0.0006108;
  uint16_t ticks = current_encoder >= previous_encoder
    ? current_encoder - previous_encoder
    : current_encoder + (UINT16_MAX - previous_encoder);
  return CONVERSION * ticks;
}

static float measure_distance_reversed(uint16_t current_encoder,
                                       uint16_t previous_encoder) {
  // take the absolute value of ticks traveled before negating
  const float CONVERSION = 0.0006108;
  uint16_t ticks = current_encoder <= previous_encoder
    ? previous_encoder - current_encoder
    : previous_encoder + (UINT16_MAX - current_encoder);
  return -CONVERSION * ticks;
}

static uint16_t last_encoder = 0;
static float distance_traveled = 0.0;

// configure initial state
static KobukiSensors_t sensors = {0};
static flower_state_t state = AMBIENT;


void state_machine() {
  //  while (1) {
  //   kobukiSensorPoll(&sensors);
  //   previous_time = current_time;
  //   current_time = read_timer();
  //   previous_light = current_light;
  //   previous_light_val = 
  //   current_light = read_light_sensors();
  //   current_light_val = 
  //   motion_yn = read_motion_sensor();
  //   touch_state = read_touch_sensors();
  //   nrf_delay_ms(1);

  //   switch(state) {
  //     case AMBIENT: {
  //       //set LEDs based on ambient light
  //       // transition logic
  //       if (touch_state) {
  //         state = TOUCH;
  //         timer_start = current_time;
  //         turn_SMA_on();
  //       }
  //       else if (motion_yn) {
  //         state = ATTENTION;
  //         last_encoder = sensors.leftWheelEncoder;
  //         distance_traveled = 0.0;
  //         timer_start = current_time;
  //         kobukiDriveDirect(5, 0);
  //         turn_SMA_on();
  //       }
  //       else if (fabs(current_light_val - previous_light_val) >= scared_light_thresh) {
  //         state = SCARED;
  //         timer_start = current_time;
  //         flashLEDs();
  //         turn_SMA_on();
  //       }
  //       else {
  //         r = 
  //         g = 
  //         b = 
  //         set_LED_color(r, g, b);
  //       }
  //       break; // each case needs to end with break!
  //     }

  //     case ATTENTION: {
  //       //
  //       timer_counter = current_time - timer_start;
  //       uint16_t curr_encoder = sensors.leftWheelEncoder;
  //       float value = measure_distance(curr_encoder, last_encoder);
  //       distance_traveled += value;
  //       last_encoder = curr_encoder;
  //       facing motion = distance_traveled;
  //       if (fabs(current_light_val - previous_light_val) >= scared_light_thresh) {
  //         state = SCARED;
  //         timer_start = current_time;
  //         turn_SMA_on();
  //         flashLEDs();
  //       }
  //       else if (timer_counter > timer_thresh) {
  //         state = AMBIENT;
  //       }
  //       else if (distance_traveled > turn_thresh) {
  //         state = TOUCH;
  //         turn_SMA_on();
  //       }
  //       else {
  //         kobukiDriveDirect(5, 0);
  //         turn_SMA_on();
  //       }
  //       break; // each case needs to end with break!
  //     }

  //     case TOUCH: {
  //       timer_counter = current_time - timer_start;
  //       if (fabs(current_light_val - previous_light_val) >= scared_light_thresh) {
  //         state = SCARED;
  //         timer_start = current_time;
  //         turn_SMA_on();
  //         flashLEDs();
  //       }
  //       else if (timer_counter > timer_thresh) {
  //         state = AMBIENT;
  //       }
  //       else {
  //         turn_SMA_on();
  //       }
  //       break; // each case needs to end with break!
  //     }

  //     case SCARED: {
  //       // transition logic
  //       timer_counter = current_time - timer_start;
  //       if (timer_counter > timer_scared_thresh) {
  //         state = AMBIENT;
  //       }
  //       else {
  //         flashLEDs();
  //         turn_SMA_on();
  //       }
  //       break; // each case needs to end with break!
  //     }
  //   }

  // }
}


int main(void) {
  ret_code_t error_code = NRF_SUCCESS;

  // initialize RTT library
  error_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(error_code);
  NRF_LOG_DEFAULT_BACKENDS_INIT();
  printf("Log initialized!\n");

  // initialize LEDs
  nrf_gpio_pin_dir_set(23, NRF_GPIO_PIN_DIR_OUTPUT);
  nrf_gpio_pin_dir_set(24, NRF_GPIO_PIN_DIR_OUTPUT);
  nrf_gpio_pin_dir_set(25, NRF_GPIO_PIN_DIR_OUTPUT);

  // initialize display
  nrf_drv_spi_t spi_instance = NRF_DRV_SPI_INSTANCE(1);
  nrf_drv_spi_config_t spi_config = {
    .sck_pin = BUCKLER_LCD_SCLK,
    .mosi_pin = BUCKLER_LCD_MOSI,
    .miso_pin = BUCKLER_LCD_MISO,
    .ss_pin = BUCKLER_LCD_CS,
    .irq_priority = NRFX_SPI_DEFAULT_CONFIG_IRQ_PRIORITY,
    .orc = 0,
    .frequency = NRF_DRV_SPI_FREQ_4M,
    .mode = NRF_DRV_SPI_MODE_2,
    .bit_order = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST
  };

  error_code = nrf_drv_spi_init(&spi_instance, &spi_config, NULL, NULL);
  APP_ERROR_CHECK(error_code);
  display_init(&spi_instance);
  display_write("Hello, Human!", DISPLAY_LINE_0);
  printf("Display initialized!\n");

  // initialize i2c master (two wire interface)
  // nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
  // i2c_config.scl = BUCKLER_SENSORS_SCL;
  // i2c_config.sda = BUCKLER_SENSORS_SDA;
  // i2c_config.frequency = NRF_TWIM_FREQ_100K;
  // error_code = nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);
  // APP_ERROR_CHECK(error_code);
  // lsm9ds1_init(&twi_mngr_instance);
  // printf("IMU initialized!\n");

  // initialize sensors
  initialize_motor();
  initialize_touch_sensors();
  initialize_light_sensors();
  initialize_motion_sensor();
  initialize_LED();
  printf("All sensors initialized!\n");
  virtual_timer_init();

  set_LED_color(1,1,1);
  // loop forever
  // while (1) {
  //   nrf_delay_ms(4000);
  //   printf("************loop\n");
  //   light_values_t light_values = read_light_sensors();
  //   printf("Light Values: %d, %d, %d, %d\n", light_values.light1, light_values.light2, light_values.light3, light_values.light4);
  // }

}
