#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

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
#include "virtual_timer.h"

//driver for kobuki
#include "buckler.h"
#include "kobukiActuator.h"
#include "kobukiSensorPoll.h"
#include "kobukiSensorTypes.h"
#include "kobukiUtilities.h"


//************ VARIABLES ****************

// State machine states
typedef enum {
  AMBIENT,
  ATTENTION,
  TOUCH,
  SCARED,
} flower_state_t;

typedef enum {
  NEUTRAL,
  ANGRY,
  SAD,
} moods_t;

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} led_t;

// Sensors
light_values_t current_light, previous_light;
touch_values_t touch_struct;

uint32_t current_light_avg, previous_light_avg, change_in_light;
bool motion_yn, facing_motion, touch_state;

// LEDs
uint8_t r, b, g;

// Timing
uint32_t current_time, previous_time, timer_start, timer_counter;

// Mood
moods_t today_mood;

// Motor
uint16_t current_direction = 0, next_direction = 0, previous_direction = 0;
uint8_t max_direction;
uint16_t last_encoder = 0, curr_encoder = 0;
float distance_traveled = 0.0;

// initial state
KobukiSensors_t sensors = {0};
flower_state_t state = AMBIENT;


//************ THRESHOLDS and SETTINGS ****************
uint16_t scared_light_thresh = (MAX_LIGHT - MIN_LIGHT)/2;
// calculate turn_thresh for 180 degrees
uint32_t timer_thresh = 1500000, timer_scared_thresh = 15000000;
float turn_thresh = 0.111125;
float turn_thresh_180 = 0.111125, turn_thresh_90 = 0.0555626, turn_thresh_270 = 0.1666875;
uint8_t touch_threshold = 25;

// Motor config
int8_t motor_speed = 30;

// Touch config
uint8_t touch_index = 0;
uint8_t touch_length = 25;
uint8_t touch_buffer[25] = {0};


// LED baseline values

led_t ANGRY_LED = {255, 0, 0}; 
led_t SAD_LED = {0, 0, 255}; 
led_t NEUTRAL_LED = {155, 40, 10}; 
led_t TOUCH_LED = {148, 0, 211}; 


//************ HELPER FUNCTIONS ****************

uint8_t sumTouchBuffer() {
  uint8_t sum = 0;
  for (uint8_t i = 0; i < touch_length; i++) {
    sum = sum + touch_buffer[i];
  }
  return sum;
}

int32_t modulo(int32_t a, int32_t b) {
  if (b < 0 ) return modulo(-a, -b);
  int32_t result = a % b; 
  return result >= 0 ? result : result + b;
}


// NOTE: Does not handle encoder wrap at maxuint
void face_motion(uint16_t current_direction_var, uint16_t next_direction_var) {
  if (current_direction_var != next_direction_var) {
    int32_t diff = next_direction_var - current_direction_var;

    // change to (-180, 180)
    diff = modulo((diff + 180), 360) - 180;

    // translate angle to ticks
    // int32_t goal_ticks = 480 * diff / 360;
    // int32_t encoder_delta = 0;
    // int32_t goal_encoder_value = curr_encoder + goal_ticks;
    // uint32_t start_encoder = sensors.rightWheelEncoder;

    // printf("goalticks, diff, start_encoder %d, %d, %ld", goal_ticks, diff, start_encoder);

    while (abs(diff) > 10) {
      update_sensor_values();
      kobukiSensorPoll(&sensors);
      curr_encoder = sensors.rightWheelEncoder; 
      int32_t cur_degrees = curr_encoder; 
      diff = next_direction_var - cur_degrees;
      diff = modulo((diff + 180), 360) - 180;

      if (diff < 0) {
        kobukiDriveDirect(0, -1 * motor_speed);
      } else {
        kobukiDriveDirect(0, motor_speed);
      }
      // encoder_delta = abs(curr_encoder - start_encoder);
      nrf_delay_ms(10);
    }
    kobukiDriveDirect(0, 0);
  }
}

void vibrate() {
  uint8_t ms_delay = 100;
  uint8_t vibrate_reps = 5;

  for (uint8_t i = 0; i < vibrate_reps; i++) {
    for (uint8_t j = 0; j < ms_delay / 10; j++) {
        kobukiSensorPoll(&sensors);
        kobukiDriveDirect(0, motor_speed);
        nrf_delay_ms(10);
    }
    for (uint8_t k = 0; k < ms_delay / 10; k++) {
        kobukiSensorPoll(&sensors);
        kobukiDriveDirect(0, -1 * motor_speed);
        nrf_delay_ms(10);
    }
  }
}

int boundInt(int i, int low, int high) {
    int result;
    result = i > high ? high : i;
    result = result < low ? low : result;
    return i;
}

uint8_t max_light_direction() {
  uint8_t max_index = 0;
  int16_t *lights;
  lights = (int16_t*) &current_light;

  for (uint8_t i = 1; i < 4; i++) {
    if (lights[i] > lights[max_index]) {
      max_index = i;
    }
  }

  return max_index + 1;
}

void update_sensor_values() {
  // motor values
  kobukiSensorPoll(&sensors);
  curr_encoder = sensors.rightWheelEncoder;

  // timing
  previous_time = current_time;
  current_time = read_timer();

  // light
  previous_light = current_light;
  current_light = read_light_sensors();
  previous_light_avg = (previous_light.light1 + previous_light.light2 + previous_light.light3 + previous_light.light4)/4;
  current_light_avg = (current_light.light1 + current_light.light2 + current_light.light3 + current_light.light4)/4;
  change_in_light = abs(current_light_avg - previous_light_avg);

  // motion
  motion_yn = read_motion_sensor();

  // touch
  touch_struct = read_touch_sensors();
  touch_buffer[touch_index] = 1;
  // if touch ever reads 0, clear the whole buffer
  if (touch_struct.touch1 && touch_struct.touch3) {
      memset(touch_buffer, 0, sizeof(touch_buffer));
  }
  touch_state = sumTouchBuffer() >= touch_threshold;
  touch_index = (touch_index + 1) % touch_length;

}

//************ STATE MACHINE ****************

void state_machine() {
  // config today's mood:
  led_t AMBIENT_LED = NEUTRAL_LED;
  uint8_t FACE_DIRECTION = 0;

  switch (today_mood) {
    case NEUTRAL: {
      AMBIENT_LED = NEUTRAL_LED;
      FACE_DIRECTION = 0;
      break;
    }
    case ANGRY: {
      AMBIENT_LED = ANGRY_LED;
      FACE_DIRECTION = 180;
      break;
    } 
    case SAD:  {
      AMBIENT_LED = SAD_LED;
      FACE_DIRECTION = 0;
      break;
    }
  }

  while (1) {
    printf("\n\n");
    nrf_delay_ms(10);
    update_sensor_values();

    printf("encoders: %d, %d \n", sensors.leftWheelEncoder, sensors.rightWheelEncoder);
    printf("light: curr:  %ld, prev: %ld, diff: %d, thresh: %d \n ", current_light_avg, previous_light_avg, abs(current_light_avg - previous_light_avg), scared_light_thresh);
    // printf("touch buffer: %d, %d, %d, %d, %d \n", touch_buffer[0], touch_buffer[1], touch_buffer[2], touch_buffer[3], touch_buffer[4]);
    printf("touched? %d \n", touch_state);
    printf("Current direction is: %d, where 0 degrees is front and 180 is back\n", current_direction);

    switch(state) {
      case AMBIENT: {
        // transition logic
        if (touch_state) {
          printf("AMBIENT --> TOUCH\n");
          state = TOUCH;
          timer_start = current_time;
        }
        else if (motion_yn) {
          printf("AMBIENT --> MOTION \n");
          state = ATTENTION;
          // last_encoder = sensors.rightWheelEncoder;
          // distance_traveled = 0.0;
          timer_start = current_time;
          // kobukiDriveDirect(0, motor_speed);
        }
        else if (change_in_light >= scared_light_thresh) {
          printf("AMBIENT --> SCARED \n");
          state = SCARED;
          timer_start = current_time;
          flashLEDs((current_time - timer_start));
        }
        else {
          //set LEDs based on ambient light
          printf("AMBIENT STATE\n");

          float scaler = 1 - (float) current_light_avg / 255.0;
          r = AMBIENT_LED.r * scaler;
          g = AMBIENT_LED.g * scaler;
          b = AMBIENT_LED.b * scaler;

          r = boundInt(r, 0, 255);
          g = boundInt(g, 0, 255);
          b = boundInt(b, 0, 255);

          set_LED_color(r, g, b);
          LEDS_ON();

          switch(max_light_direction()) {
            case 1: {
              next_direction = 0;
              break;
            } case 2: {
              next_direction = 90;
              break;
            } case 3: {
              next_direction = 180;
              break;
            } case 4: {
              next_direction = 270;
              break;
            }
          }

          printf("The direction of max light is: %d\n", next_direction);
          printf("Turning to face max light\n");
          face_motion(current_direction, next_direction);
          printf("Finished turning to face max light\n");
          current_direction = next_direction;
        }
        break;
      }

      case ATTENTION: {
        printf("ATTENTION STATE\n");
        next_direction = FACE_DIRECTION;
        printf("turning towards motion, towards direction: %d\n", next_direction);
        face_motion(current_direction, next_direction);
        printf("done turning towards motion\n");
        current_direction = next_direction;

        if (abs(current_light_avg - previous_light_avg) >= scared_light_thresh) {
          printf("ATTENTION --> SCARED \n");
          state = SCARED;
          timer_start = current_time;
          flashLEDs((current_time - timer_start));
        }
        else { // (timer_counter > timer_thresh) {
           printf("ATTENTION --> AMBIENT \n");
           state = AMBIENT;
        }

        break; // each case needs to end with break!
      }

      case TOUCH: {
        timer_counter = current_time - timer_start;
        if (change_in_light >= scared_light_thresh) {
          printf("TOUCH --> SCARED \n");
          state = SCARED;
          timer_start = current_time;
          flashLEDs((current_time - timer_start));
        }
        else if (timer_counter > timer_thresh) {
          printf("TOUCH --> AMBIENT \n");
          state = AMBIENT;
        }
        else {
          printf("TOUCH STATE \n");
          set_LED_color(TOUCH_LED.r, TOUCH_LED.g, TOUCH_LED.b);
          LEDS_ON();
          vibrate();
          kobukiDriveDirect(0, 0);
        }
        break; // each case needs to end with break!
      }

      case SCARED: {
        // transition logic
        timer_counter = current_time - timer_start;
        if (timer_counter > timer_scared_thresh) {
          printf("SCARED --> AMBIENT \n");
          state = AMBIENT;
        }
        else {
          printf("SCARED STATE \n");
          flashLEDs((current_time - timer_start));
          kobukiDriveDirect(0, 0);
        }
        break; // each case needs to end with break!
      }
    }
  }
}



void initialize_all() {
   ret_code_t error_code = NRF_SUCCESS;

  // initialize RTT library
  error_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(error_code);
  NRF_LOG_DEFAULT_BACKENDS_INIT();
  printf("Log initialized!\n");

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

  // initialize sensors
  initialize_motor();
  initialize_touch_sensors();
  initialize_light_sensors();
  initialize_motion_sensor();
  set_LED_color(190, 0, 90);

  initialize_LED();
  virtual_timer_init(); 

  printf("All sensors initialized!\n");
}

int main(void) {

  // initialize hardware
  initialize_all();

  // set initial values
  current_light = read_light_sensors();
  previous_light = current_light;

  srand(current_time);
  today_mood = rand() % 2;
  printf("Today's mood is: %d \n", today_mood);

  state_machine();

  // while(true) {
  //   nrf_delay_ms(10);
  //   current_light = read_light_sensors();
  //   // printf("lights: %ld, %ld, %ld, %ld \n", current_light.light1, current_light.light2, current_light.light3, current_light.light4);
  //   kobukiSensorPoll(&sensors);
  //   printf("encoders: %d  \n", sensors.rightWheelEncoder);
  //   // kobukiDriveDirect(0, 30);
  //   switch(max_light_direction()) {
  //     case 1: {
  //       next_direction = 0;
  //       break;
  //     } case 2: {
  //       next_direction = 90;
  //       break;
  //     } case 3: {
  //       next_direction = 180;
  //       break;
  //     } case 4: {
  //       next_direction = 270;
  //       break;
  //     }
  //   }
  //   kobukiDriveDirect(0, 0);
  //   printf("The direction of maxx light is: %d\n", next_direction);
  //   // //printf("Turning to face max light\n");
  //   face_motion(current_direction, next_direction);
  //   // //printf("Finished turning to face max light\n");
  //   current_direction = next_direction;
  // }
}
