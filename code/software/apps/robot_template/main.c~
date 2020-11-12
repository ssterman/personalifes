// Robot Template app
//
// Framework for creating applications that control the Kobuki robot

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "app_error.h"
#include "app_timer.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_drv_spi.h"

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
  OFF,
  DRIVING,
  TURN_CW,
  BACK_UP_LEFT,
  BACK_UP_RIGHT,
  TURN_AWAY_LEFT,
  TURN_AWAY_RIGHT,
} robot_state_t;

void back_up_state(bool left);
void pre_dir_change();
void display_float(float v);

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

static bool is_bump(KobukiSensors_t *sensors) {
  return sensors->bumps_wheelDrops.bumpLeft
    || sensors->bumps_wheelDrops.bumpCenter
    || sensors->bumps_wheelDrops.bumpRight;
}

static uint16_t last_encoder = 0;
static float distance_traveled = 0.0;

// configure initial state
static KobukiSensors_t sensors = {0};
static robot_state_t state = OFF;

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

  // initialize Kobuki
  kobukiInit();
  printf("Kobuki initialized!\n");

  // loop forever, running state machine
  while (1) {
    // read sensors from robot
    kobukiSensorPoll(&sensors);

    // delay before continuing
    // Note: removing this delay will make responses quicker, but will result
    //  in printf's in this loop breaking JTAG
    nrf_delay_ms(1);


    // handle states
    switch(state) {
      case OFF: {
        // transition logic
        if (is_button_pressed(&sensors)) {
          last_encoder = sensors.leftWheelEncoder;
          distance_traveled = 0.0;
          state = DRIVING;
        } else {
          // perform state-specific actions here
          display_write("OFF", DISPLAY_LINE_0);
          display_write("", DISPLAY_LINE_1);
          kobukiDriveDirect(0, 0);
          state = OFF;
        }
        break; // each case needs to end with break!
      }

      case DRIVING: {
        // transition logic
        if (is_button_pressed(&sensors)) {
          state = OFF;
        } else if (sensors.bumps_wheelDrops.bumpRight
            || sensors.bumps_wheelDrops.bumpCenter) {
          pre_dir_change();
          state = BACK_UP_LEFT;
        } else if (sensors.bumps_wheelDrops.bumpLeft) {
          pre_dir_change();
          state = BACK_UP_RIGHT;
        } else {
          // perform state-specific actions here
          display_write("DRIVING", DISPLAY_LINE_0);
          uint16_t curr_encoder = sensors.leftWheelEncoder;
          float value = measure_distance(curr_encoder, last_encoder);
          distance_traveled += value;
          last_encoder = curr_encoder;
          display_float(distance_traveled);
          if (distance_traveled >= 0.5) {
            lsm9ds1_start_gyro_integration();
            state = TURN_CW;
          } else {
            kobukiDriveDirect(75, 75);
            state = DRIVING;
          }
        }
        break; // each case needs to end with break!
      }

      // add other cases here
      case TURN_CW: {
        if (is_button_pressed(&sensors)) {
          lsm9ds1_stop_gyro_integration();
          state = OFF;
        } else if (sensors.bumps_wheelDrops.bumpRight
            || sensors.bumps_wheelDrops.bumpCenter) {
          lsm9ds1_stop_gyro_integration();
          pre_dir_change();
          state = BACK_UP_LEFT;
        } else if (sensors.bumps_wheelDrops.bumpLeft) {
          lsm9ds1_stop_gyro_integration();
          pre_dir_change();
          state = BACK_UP_RIGHT;
        } else {
          // returns angles in degrees, cw is negative
          float angle_turned = fabs(lsm9ds1_read_gyro_integration().z_axis);
          if (angle_turned >= 90) {
            lsm9ds1_stop_gyro_integration();
            pre_dir_change();
            state = DRIVING;
          } else {
            display_write("TURNING", DISPLAY_LINE_0);
            kobukiDriveDirect(40, -40);
            display_float(angle_turned);
            state = TURN_CW;
          }
        }
        break;
      }

      case BACK_UP_LEFT: {
        display_write("BACK UP LEFT", DISPLAY_LINE_0);
        back_up_state(true);
        break;
      }
      case BACK_UP_RIGHT: {
        display_write("BACK UP RIGHT", DISPLAY_LINE_0);
        back_up_state(false);
        break;
      }
      case TURN_AWAY_LEFT: {
        if (is_button_pressed(&sensors)) {
          lsm9ds1_stop_gyro_integration();
          state = OFF;
        } else {
          float angle_turned = fabs(lsm9ds1_read_gyro_integration().z_axis);
          if (angle_turned >= 45) {
            lsm9ds1_stop_gyro_integration();
            pre_dir_change();
            state = DRIVING;
          } else {
            display_write("EVADING LEFT", DISPLAY_LINE_0);
            display_float(angle_turned);
            kobukiDriveDirect(-40, 40);
          }
        }
        break;
      }
      case TURN_AWAY_RIGHT: {
        if (is_button_pressed(&sensors)) {
          lsm9ds1_stop_gyro_integration();
          state = OFF;
        } else {
          float angle_turned = fabs(lsm9ds1_read_gyro_integration().z_axis);
          if (angle_turned >= 45) {
            lsm9ds1_stop_gyro_integration();
            pre_dir_change();
            state = DRIVING;
          } else {
            display_write("EVADING RIGHT", DISPLAY_LINE_0);
            display_float(angle_turned);
            kobukiDriveDirect(40, -40);
          }
        }
        break;
      }
    }
  }
}

void display_float(float v) {
  char buf [16];
  snprintf(buf, 16, "%f", v);
  display_write(buf, DISPLAY_LINE_1);
}

void pre_dir_change() {
  // allow encoder momentum to stop
  display_write("PAUSING", DISPLAY_LINE_0);
  kobukiDriveDirect(0, 0);
  for (int i = 0; i < 30; i++) {
    kobukiSensorPoll(&sensors);
    nrf_delay_ms(1);
  }
  distance_traveled = 0;
  last_encoder = sensors.leftWheelEncoder;
}

void back_up_state(bool left) {
  if (is_button_pressed(&sensors)) {
    state = OFF;
  } else {
    uint16_t curr_encoder = sensors.leftWheelEncoder;
    float value = measure_distance_reversed(curr_encoder, last_encoder);
    distance_traveled += value;
    last_encoder = curr_encoder;
    display_float(distance_traveled);
    
    if (distance_traveled <= -0.1) {
      lsm9ds1_start_gyro_integration();
      state = left ? TURN_AWAY_LEFT : TURN_AWAY_RIGHT;
    } else {
      kobukiDriveDirect(-50, -50);
      state = left ? BACK_UP_LEFT : BACK_UP_RIGHT;
    }
  }
}
