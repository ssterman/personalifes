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

light_values_t current_light, previous_light;
uint16_t = scared_light_thresh = ;
uint32_t timer_thresh = , turn_thresh = , timer_scared_thresh = ;
uint8_t r, b, g;
bool motion_yn, facing_motion;
touch_values_t touch_state;
uint32_t current_time, previous_time, timer_start, timer_counter;

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
  nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
  i2c_config.scl = BUCKLER_SENSORS_SCL;
  i2c_config.sda = BUCKLER_SENSORS_SDA;
  i2c_config.frequency = NRF_TWIM_FREQ_100K;
  error_code = nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);
  APP_ERROR_CHECK(error_code);
  lsm9ds1_init(&twi_mngr_instance);
  printf("IMU initialized!\n");

  // initialize sensors
  initialize_motor()
  initialize_touch_sensors();
  initialize_light_sensors();
  initialize_motion_sensor();
  initialize_LED();
  printf("All sensors initialized!\n");
  virtual_timer_init();

  // gpio_config(23, OUTPUT);
  // gpio_config(24, OUTPUT);
  // gpio_config(25, OUTPUT);
  // gpio_config(28, INPUT);
  // gpio_config(22, INPUT);
  // uint32_t *gpio_out = (uint32_t *) GPIO_OUT_addr;
  // uint32_t gpio_out_value;

    kobukiSensorPoll(&sensors);
    previous_time = current_time;
    current_time = read_timer();
    previous_light = current_light;
    previous_light_val = 
    printf("previous light value: %d\n", previous_light_val);
    current_light = read_light_sensors();
    current_light_val = 
    printf("current light value: %d\n", current_light_val);
    motion_yn = read_motion_sensor();
    printf("Motion?: %d\n", motion_yn);
    touch_state = read_touch_sensors();
    printf("touch?: %d\n", touch_state);
    nrf_delay_ms(1);
    printf("turning SMA on\n");
    turn_SMA_on();
    nrf_delay_ms(60000);
    printf("turning SMA off\n");
    turn_SMA_off();
    nrf_delay_ms(60000);
    printf("turning motors on\n");
    last_encoder = sensors.leftWheelEncoder;
    distance_traveled = 0.0;
    timer_start = current_time;
    kobukiDriveDirect(5, 0);
    nrf_delay_ms(5000);
    printf("turning motors off\n");
    kobukiDriveDirect(0, 0);
    float value = measure_distance(curr_encoder, last_encoder);
    distance_traveled += value;
    printf("distance traveled was: %d\n", distance_traveled);
    printf("turning LEDs on for two different colors\n");
    r = 
    g = 
    b = 
    set_LED_color(r, g, b);
    nrf_delay_ms(5000);
    r = 
    g = 
    b = 
    set_LED_color(r, g, b);
    nrf_delay_ms(5000);
    printf("Flashing LEDs\n");
    flashLEDs();
    printf("Finished test!\n");

}
