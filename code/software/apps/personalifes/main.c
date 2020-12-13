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

int32_t goal_heading_in_degrees = 0;
float setpoint = 0;
bool reached_goal = false;

// initial state
KobukiSensors_t sensors = {0};
flower_state_t state = AMBIENT;


//************ THRESHOLDS and SETTINGS ****************
uint16_t scared_light_thresh = 70;
// calculate turn_thresh for 180 degrees
uint32_t timer_thresh = 1500000, timer_scared_thresh = 15000000;
float turn_thresh = 0.111125;
float turn_thresh_180 = 0.111125, turn_thresh_90 = 0.0555626, turn_thresh_270 = 0.1666875;
uint8_t touch_threshold = 25;

// Motor config
int8_t motor_speed = 30;
float K_p = 15;
float K_d = 1;
float degrees_per_cycle = 1.5;
float MOTOR_MAX = 40;

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

// https://stackoverflow.com/questions/12089514/real-modulo-operator-in-c-c
int32_t modulo(int32_t aval, int32_t bval) {
  if (bval < 0 ) return modulo(-aval, -bval);
  int32_t result = aval % bval; 
  return result >= 0 ? result : result + bval;
}

int8_t motor_command = 0;

void update_position() {    
    float sign = goal_heading_in_degrees - setpoint > 0 ? 1 : -1;
    // if the setpoint isn't within error of the goal, keep going
    if (abs(setpoint - goal_heading_in_degrees) > degrees_per_cycle) {
      // linear increase in setpoint
      setpoint += 1 * sign * degrees_per_cycle; 
    }

    // (-180, 180) difference_between_position_and_setpoint
    float diff = modulo((setpoint - curr_encoder + 180), 360) - 180;    
    float updated_motor_speed = (K_p * diff) - (K_d * (float) (curr_encoder - last_encoder));

    updated_motor_speed = updated_motor_speed > MOTOR_MAX ? MOTOR_MAX : updated_motor_speed;
    updated_motor_speed = updated_motor_speed < -1 * MOTOR_MAX ? -1 * MOTOR_MAX : updated_motor_speed;

    //motor_command +=  motor_command < updated_motor_speed ? 2 : -2; //(int8_t) updated_motor_speed;
    //motor_command = (float) motor_command * .7 + updated_motor_speed * .3;

    if (sign > 0) {
      updated_motor_speed = updated_motor_speed < 15 ? 15 : updated_motor_speed;
    } else {
      updated_motor_speed = updated_motor_speed > -15 ? -15 : updated_motor_speed;
    }

    motor_command = updated_motor_speed;


    printf("setpoint %f, goal %d, diff %f, speed %d \n", setpoint, goal_heading_in_degrees, diff, motor_command);

    if (abs(setpoint - goal_heading_in_degrees) > degrees_per_cycle || abs(diff) > 10) {
      kobukiDriveDirect(0, motor_command);
      reached_goal = false;
    } else {
      kobukiDriveDirect(0,0);
      printf("GOAL!!!");
      reached_goal = true;
    }
}



// void face_motion(uint16_t current_direction_var, uint16_t next_direction_var) {
//   if (current_direction_var != next_direction_var) {
//     int32_t diff = next_direction_var - current_direction_var;

//     // change to (-180, 180)
//     diff = modulo((diff + 180), 360) - 180;

//     while (abs(diff) > 10) {
//       update_sensor_values();
//       kobukiSensorPoll(&sensors);
//       curr_encoder = sensors.rightWheelEncoder; 
//       int32_t cur_degrees = curr_encoder; 
//       diff = next_direction_var - cur_degrees;
//       diff = modulo((diff + 180), 360) - 180;

//       if (diff < 0) {
//         kobukiDriveDirect(0, -1 * motor_speed);
//       } else {
//         kobukiDriveDirect(0, motor_speed);
//       }
//       nrf_delay_ms(10);
//     }
//     kobukiDriveDirect(0, 0);
//   }
// }

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
  last_encoder = curr_encoder;
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
              goal_heading_in_degrees = 0;
              break;
            } case 2: {
              goal_heading_in_degrees = 90;
              break;
            } case 3: {
              goal_heading_in_degrees = 180;
              break;
            } case 4: {
              goal_heading_in_degrees = 270;
              break;
            }
          }

          update_position();
        }
        break;
      }

      case ATTENTION: {   
        // do this first     
        printf("ATTENTION STATE\n");
        goal_heading_in_degrees = FACE_DIRECTION;
        update_position();

        if (abs(current_light_avg - previous_light_avg) >= scared_light_thresh) {
          printf("ATTENTION --> SCARED \n");
          state = SCARED;
          timer_start = current_time;
          flashLEDs((current_time - timer_start));
        } else if (reached_goal) { // (timer_counter > timer_thresh) {
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
          setpoint = curr_encoder;
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
  //initialize_light_sensors();

  // set initial values
  current_light = read_light_sensors();
  previous_light = current_light;

  srand(current_light.light1);
  today_mood = 0; //rand() % 2;
  // printf("Today's mood is: %d \n", today_mood);


  while (true) {
        printf("\n\n");
        nrf_delay_ms(5);
        update_sensor_values();

          // float scaler = 1 - (float) current_light_avg / 255.0;
          // r = AMBIENT_LED.r * scaler;
          // g = AMBIENT_LED.g * scaler;
          // b = AMBIENT_LED.b * scaler;

          // r = boundInt(r, 0, 255);
          // g = boundInt(g, 0, 255);
          // b = boundInt(b, 0, 255);

          // set_LED_color(r, g, b);
          // LEDS_ON();

          switch(max_light_direction()) {
            case 1: {
              goal_heading_in_degrees = 0;
              break;
            } case 2: {
              goal_heading_in_degrees = 90;
              break;
            } case 3: {
              goal_heading_in_degrees = 180;
              break;
            } case 4: {
              goal_heading_in_degrees = 270;
              break;
            }
          }

          update_position();
      }
  //state_machine();


  //LIGHT SENSOR FORMATTING 
  // while (true) {
  //   nrf_delay_ms(10);
  //   current_light = read_light_sensors();
  //   printf("%d, %d, %d, %d \n", current_light.light1, current_light.light2, current_light.light3, current_light.light4);
  // }
}
