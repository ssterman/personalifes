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
  TURNING,
  REVERSING,
} robot_state_t;

typedef enum {
  CLOCKWISE,
  COUNTERCLOCKWISE,
} rotation_direction_t;

typedef enum {
  LEFT,
  CENTER,
  RIGHT,
} direction_t;

void lcdPrintLine2Float(float value) {
  char buf [16];
  snprintf ( buf , 16 , "%f", value );
  display_write ( buf , DISPLAY_LINE_1 ) ;
}

static float measure_distance(uint16_t current_encoder, uint16_t previous_encoder, bool reversing) {
  const float CONVERSION = 0.0006108; // ticks to meters

  uint16_t ticks = 0;
  float distance = 0;
 
  if (current_encoder == previous_encoder) {
    return distance;
  }

  if (!reversing) {
    if (current_encoder > previous_encoder) {
      ticks = (current_encoder - previous_encoder);
    } else {
      ticks = UINT16_MAX - previous_encoder + current_encoder;
    }
    distance = ticks * CONVERSION;
  } else {
    if (current_encoder > previous_encoder) {
      printf("reversing at wrap: %i, %i\n", current_encoder, previous_encoder);
      ticks = ( (previous_encoder) + (UINT16_MAX - current_encoder) );
    } else {
      printf("reversing in middle: %i, %i\n", current_encoder, previous_encoder);
      ticks = previous_encoder - current_encoder;
    }
    distance = (-1.0) * ticks * CONVERSION;
    printf("increment, ticks: %f, %i\n", distance, ticks);
  }

  // it should never be this high, indicates a holdover from previous direction
  if (ticks > 60000) { return 0; }

  return distance;
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

  // configure initial state
  robot_state_t state = OFF;
  KobukiSensors_t sensors = {0};


  float total_distance = 0;
  float angle = 0;
  uint16_t previous_encoder = sensors.rightWheelEncoder;

  direction_t obstaclePosition = CENTER;

  float GOAL_ANGLE = 90;
  rotation_direction_t TURN_DIRECTION = COUNTERCLOCKWISE;

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
                   // reset distance
          total_distance = 0;
          previous_encoder = sensors.rightWheelEncoder;
          state = DRIVING;
        } else {
          // perform state-specific actions here
          display_write("OFF", DISPLAY_LINE_0);
          kobukiDriveDirect(0, 0);
          state = OFF;
        }
        break; // each case needs to end with break!
      }

      case DRIVING: {
        // transition logic
        if (is_button_pressed(&sensors)) {
          lsm9ds1_stop_gyro_integration();
          state = OFF;
        } else if (sensors.bumps_wheelDrops.bumpLeft 
           || sensors.bumps_wheelDrops.bumpCenter
           || sensors.bumps_wheelDrops.bumpRight
        ){
          if (sensors.bumps_wheelDrops.bumpLeft) {
            obstaclePosition = LEFT;
          } else if (sensors.bumps_wheelDrops.bumpRight) {
            obstaclePosition = RIGHT;
          } else {
            obstaclePosition = CENTER;
          }

          previous_encoder = sensors.rightWheelEncoder;
          total_distance = 0;
          printf("(go to REVERSING)\n");
          state = REVERSING;
        } else if (total_distance >= 0.5) {
          printf("transition to turning from DRIVING");
          angle = 0;
          GOAL_ANGLE = 90;
          TURN_DIRECTION = COUNTERCLOCKWISE;
          //turn on gyro
          lsm9ds1_start_gyro_integration();
          state = TURNING;
        } else {
          // perform state-specific actions here
          display_write("DRIVING", DISPLAY_LINE_0);
          kobukiDriveDirect(75, 75);
          
          //measure distance
          uint16_t current_encoder = sensors.rightWheelEncoder;
          float increment = measure_distance(current_encoder, previous_encoder, false);
          printf("(increment) %f\n", increment);
          //update distance
          total_distance = total_distance + increment;
          printf("total distance %f \n", total_distance);
          //update the previous encoder value
          previous_encoder = current_encoder;

          lcdPrintLine2Float(total_distance);

          state = DRIVING;
        }
        break; // each case needs to end with break!
      } case TURNING: {
        if (is_button_pressed(&sensors)) {
          lsm9ds1_stop_gyro_integration();          
          state = OFF;
        } else if (sensors.bumps_wheelDrops.bumpLeft 
           || sensors.bumps_wheelDrops.bumpCenter
           || sensors.bumps_wheelDrops.bumpRight
        ){
          if (sensors.bumps_wheelDrops.bumpLeft) {
            obstaclePosition = LEFT;
          } else if (sensors.bumps_wheelDrops.bumpRight) {
            obstaclePosition = RIGHT;
          } else {
            obstaclePosition = CENTER;
          }
          lsm9ds1_stop_gyro_integration();
          previous_encoder = sensors.rightWheelEncoder;
          total_distance = 0;
          printf("go to reversing\n");
          state = REVERSING;
        } else if (angle >= GOAL_ANGLE) {
          total_distance = 0;
          //turn off gyro
          lsm9ds1_stop_gyro_integration();
          previous_encoder = sensors.rightWheelEncoder;
          state = DRIVING;
          printf("(transition to driving from turning) %f\n", total_distance );
        } else {
          angle = fabs(lsm9ds1_read_gyro_integration().z_axis);
          display_write("TURNING", DISPLAY_LINE_0);
          if (TURN_DIRECTION == CLOCKWISE) {
            kobukiDriveDirect(50, -50);
          } else {
            kobukiDriveDirect(-50, 50);
          }

          lcdPrintLine2Float(angle);

          state = TURNING;
        }
        break;
      } case REVERSING: {
          if (is_button_pressed(&sensors)) {
            state = OFF;
          } else if (total_distance <= -0.1) {
            printf("leaving reversing: %f\n", total_distance);
            angle = 0;
            GOAL_ANGLE = 45;
            if (obstaclePosition == RIGHT) {
              TURN_DIRECTION = COUNTERCLOCKWISE;
            } else {
              TURN_DIRECTION = CLOCKWISE;
            }
            //turn on gyro
            lsm9ds1_start_gyro_integration();
            state = TURNING;
          } else {
            display_write("REVERSING", DISPLAY_LINE_0);
            kobukiDriveDirect(-75, -75);
            
            //measure distance
            uint16_t current_encoder = sensors.rightWheelEncoder;
            float increment = measure_distance(current_encoder, previous_encoder, true);
            printf("(increment) %f\n", increment);
            //update distance
            total_distance = total_distance + increment;
            printf("total distance %f \n", total_distance);
            //update the previous encoder value
            previous_encoder = current_encoder;

            lcdPrintLine2Float(total_distance);

            state = REVERSING;
          }
        break;
      }

      // add other cases here

    }
  }
}

