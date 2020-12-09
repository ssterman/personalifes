// Blink app
//
// Blinks the LEDs on Buckler

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


typedef enum {
  AMBIENT,
  ATTENTION,
  TOUCH,
  SCARED,
} flower_state_t;

light_values_t current_light, previous_light;
// put into a separate file 
uint16_t scared_light_thresh = (MAX_LIGHT - MIN_LIGHT)/2;
uint32_t current_light_avg, previous_light_avg, ambient_light;
// calculate turn_thresh for 180 degrees
uint32_t timer_thresh = 1500000, timer_scared_thresh = 15000000, average_light;
float turn_thresh = 0.111125;
float turn_thresh_180 = 0.111125, turn_thresh_90 = 0.0555626, turn_thresh_270 = 0.1666875;
uint8_t r, b, g;
bool motion_yn, facing_motion, touch_state;
touch_values_t touch_struct;
uint32_t current_time, previous_time, timer_start, timer_counter;
uint8_t touch_index = 0;
uint8_t touch_threshold = 25;
uint8_t touch_length = 25;
uint8_t touch_buffer[25] = {0};
uint8_t max_direction;
uint8_t today_mood;
uint16_t current_direction = 0, next_direction = 0, previous_direction = 0;

static float measure_distance(uint16_t current_encoder,
                              uint16_t previous_encoder) {
  const float CONVERSION = 0.0006108;
  uint16_t ticks = current_encoder >= previous_encoder
    ? current_encoder - previous_encoder
    : current_encoder + (UINT16_MAX - previous_encoder);
  return CONVERSION * ticks;
}

// static float measure_distance_reversed(uint16_t current_encoder,
//                                        uint16_t previous_encoder) {
//   // take the absolute value of ticks traveled before negating
//   const float CONVERSION = 0.0006108;
//   uint16_t ticks = current_encoder <= previous_encoder
//     ? previous_encoder - current_encoder
//     : previous_encoder + (UINT16_MAX - current_encoder);
//   return -CONVERSION * ticks;
// }

static uint16_t last_encoder = 0;
static float distance_traveled = 0.0;

// configure initial state
static KobukiSensors_t sensors = {0};
static flower_state_t state = AMBIENT;

uint8_t sumTouchBuffer() {
  uint8_t sum = 0;
  for (uint8_t i = 0; i < touch_length; i++) {
    sum = sum + touch_buffer[i];
  }
  return sum;
}

void face_motion(uint16_t current_direction_var, uint16_t next_direction_var) {
  if (current_direction_var != next_direction_var) {
    uint16_t amount_turn = abs(next_direction_var - current_direction_var);
    if (amount_turn > 180) {
      amount_turn = 90;
    }
    if (amount_turn == 90){
      turn_thresh = turn_thresh_90;
    }
    else if (amount_turn == 180){
      turn_thresh = turn_thresh_180;
    }
    last_encoder = sensors.rightWheelEncoder;
    distance_traveled = 0.0;
    while (distance_traveled < turn_thresh) {
      kobukiSensorPoll(&sensors);
      uint16_t curr_encoder = sensors.rightWheelEncoder;
      float value = measure_distance(curr_encoder, last_encoder);
      printf("value, %d, %f \n", curr_encoder, value);
      distance_traveled += value;
      last_encoder = curr_encoder;
      if (((next_direction_var<current_direction_var) && (abs(next_direction_var-current_direction_var)<=180)) || (current_direction_var - next_direction_var > 180)) {
        kobukiDriveDirect(0, -20);
        //turn left
      }
      else {
        kobukiDriveDirect(0, 20);
        //turn right
      }
    }
    kobukiDriveDirect(0, 0);
  }
}

void vibrate() {
  kobukiDriveDirect(0, 20);
  nrf_delay_ms(1);
  kobukiDriveDirect(0, -20);
  nrf_delay_ms(1);
  kobukiDriveDirect(0, 20);
  nrf_delay_ms(1);
  kobukiDriveDirect(0, -20);
  nrf_delay_ms(1);
  kobukiDriveDirect(0, 20);
  nrf_delay_ms(1);
  kobukiDriveDirect(0, -20);
  nrf_delay_ms(1);
  kobukiDriveDirect(0, 20);
  nrf_delay_ms(1);
  kobukiDriveDirect(0, -20);
  nrf_delay_ms(1);
  kobukiDriveDirect(0, 20);
  nrf_delay_ms(1);
  kobukiDriveDirect(0, -20);
  nrf_delay_ms(1);
  kobukiDriveDirect(0, 20);
  nrf_delay_ms(1);
  kobukiDriveDirect(0, -20);
  nrf_delay_ms(1);
  kobukiDriveDirect(0, 20);
  nrf_delay_ms(1);
  kobukiDriveDirect(0, -20);
  nrf_delay_ms(1);
  kobukiDriveDirect(0, 20);
  nrf_delay_ms(1);
  kobukiDriveDirect(0, -20);
  nrf_delay_ms(1);
  kobukiDriveDirect(0, 0);
}

/*
0 -> 90 turn right
0 -> 180 turn right
0 -> 270 turn left

90 -> 180 turn right
90 -> 0 turn left
90 -> 270 turn right

180 -> 90 turn left
180 -> 0 turn left
180 -> 270 turn right

270 -> 0 turn right
270 -> 90 turn left
270 -> 180 turn left
*/

void state_machine() {
   while (1) {
    printf("\n\n");
    kobukiSensorPoll(&sensors);
    printf("sensors: %d, %d \n", sensors.rightWheelEncoder, sensors.leftWheelEncoder);
    previous_time = current_time;
    current_time = read_timer();
    previous_light = current_light;
    current_light = read_light_sensors();
    previous_light_avg = (previous_light.light1 + previous_light.light2 + previous_light.light3 + previous_light.light4)/4;
    current_light_avg = (current_light.light1 + current_light.light2 + current_light.light3 + current_light.light4)/4;
    motion_yn = read_motion_sensor();
    touch_struct = read_touch_sensors();
    // touch_state = (!touch_struct.touch1 || !touch_struct.touch3); // || touch_struct.touch2 || touch_struct.touch3 || touch_struct.touch4);
    touch_buffer[touch_index] = 1;

    //if it ever reads 0, clear the whole thing
    if (touch_struct.touch1 && touch_struct.touch3) {
        memset(touch_buffer, 0, sizeof(touch_buffer));
    }
    touch_state = sumTouchBuffer() >= touch_threshold;
    touch_index = (touch_index + 1) % touch_length;
    printf("touch buffer: %d, %d, %d, %d, %d \n", touch_buffer[0], touch_buffer[1], touch_buffer[2], touch_buffer[3], touch_buffer[4]);
    nrf_delay_ms(1);

    printf("current, prev, thresh: %d, %d, %d, %d \n ", current_light_avg, previous_light_avg, abs(current_light_avg - previous_light_avg), scared_light_thresh);
    printf("touched? %d \n", touch_state);

    switch(state) {
      case AMBIENT: {
        //set LEDs based on ambient light
        // transition logic
        if (touch_state) {
          printf("AMBIENT --> TOUCH\n");
          state = TOUCH;
          timer_start = current_time;
        }
        else if (motion_yn) {
          printf("AMBIENT --> MOTION \n");
          state = ATTENTION;
          last_encoder = sensors.rightWheelEncoder;
          distance_traveled = 0.0;
          timer_start = current_time;
          // to turn, need to operate at 20
          kobukiDriveDirect(0, 20);
        }
        // average the 4 light sensors 
        else if (abs(current_light_avg - previous_light_avg) >= scared_light_thresh) {
          printf("AMBIENT --> SCARED \n");
          state = SCARED;
          timer_start = current_time;
          flashLEDs((current_time - timer_start));
        }
        else {
          printf("AMBIENT STATE\n");

          float scaler = 1 - (float) current_light_avg / 255.0;
          r = 155 * scaler; //- ambient_light;
          g = 40 * scaler; //- ambient_light;
          b = 10 * scaler;

          r = boundInt(r, 0, 255);
          g = boundInt(g, 0, 255);
          b = boundInt(b, 0, 255);

          set_LED_color(r, g, b);
          LEDS_ON();
          kobukiDriveDirect(0, 0);

        }
        break; // each case needs to end with break!
      }

      case ATTENTION: {
        //
        timer_counter = current_time - timer_start;
        uint16_t curr_encoder = sensors.rightWheelEncoder;
        float value = measure_distance(curr_encoder, last_encoder);
        printf("value, %d, %f \n", curr_encoder, value);
        distance_traveled += value;
        last_encoder = curr_encoder;
        //facing_motion = distance_traveled;
        // if (abs(current_light_avg - previous_light_avg) >= scared_light_thresh) {
        //   printf("ATTENTION --> SCARED \n");
        //   state = SCARED;
        //   timer_start = current_time;
        //   flashLEDs((current_time - timer_start));
        // }
        // else if (timer_counter > timer_thresh) {
        //   printf("ATTENTION --> AMBIENT \n");
        //   state = AMBIENT;
        // }
        if (distance_traveled > turn_thresh) {
          printf("ATTENTION --> TOUCH \n");
          state = TOUCH;
        }
        else {
          printf("ATTENTION STATE\n");
          kobukiDriveDirect(0, 20);
          printf("     distance_traveled, %f, %f\n", distance_traveled, turn_thresh);
        }
        break; // each case needs to end with break!
      }

      case TOUCH: {
        timer_counter = current_time - timer_start;
        if (abs(current_light_avg - previous_light_avg) >= scared_light_thresh) {
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

int boundInt(int i, int low, int high) {
    int result;
    result = i > high ? high : i;
    result = result < low ? low : result;
    return i;
}

void state_machine_extended_sad() {
   while (1) {
    printf("\n\n");
    kobukiSensorPoll(&sensors);
    printf("sensors: %d, %d \n", sensors.rightWheelEncoder, sensors.leftWheelEncoder);
    printf("Current direction is: %d, where 0 degrees is front and 180 is back\n", current_direction);
    previous_time = current_time;
    current_time = read_timer();
    previous_light = current_light;
    current_light = read_light_sensors();
    previous_light_avg = (previous_light.light1 + previous_light.light2 + previous_light.light3 + previous_light.light4)/4;
    current_light_avg = (current_light.light1 + current_light.light2 + current_light.light3 + current_light.light4)/4;
    motion_yn = read_motion_sensor();
    touch_struct = read_touch_sensors();
    // touch_state = (!touch_struct.touch1 || !touch_struct.touch3); // || touch_struct.touch2 || touch_struct.touch3 || touch_struct.touch4);
    touch_buffer[touch_index] = 1;

    //if it ever reads 0, clear the whole thing
    if (touch_struct.touch1 && touch_struct.touch3) {
        memset(touch_buffer, 0, sizeof(touch_buffer));
    }
    touch_state = sumTouchBuffer() >= touch_threshold;
    touch_index = (touch_index + 1) % touch_length;
    printf("touch buffer: %d, %d, %d, %d, %d \n", touch_buffer[0], touch_buffer[1], touch_buffer[2], touch_buffer[3], touch_buffer[4]);
    nrf_delay_ms(1);

    printf("current, prev, thresh: %d, %d, %d, %d \n ", current_light_avg, previous_light_avg, abs(current_light_avg - previous_light_avg), scared_light_thresh);
    printf("touched? %d \n", touch_state);

    switch(state) {
      case AMBIENT: {
        //set LEDs based on ambient light
        // transition logic
        if (touch_state) {
          printf("AMBIENT --> TOUCH\n");
          state = TOUCH;
          timer_start = current_time;
          printf("going to vibrate now\n");
          vibrate();
          printf("ended vibration\n");
        }
        else if (motion_yn) {
          printf("AMBIENT --> MOTION \n");
          state = AMBIENT;
          //last_encoder = sensors.rightWheelEncoder;
          //distance_traveled = 0.0;
          //timer_start = current_time;
          printf("going to vibrate now\n");
          vibrate();
          printf("ended vibration\n");
          // to turn, need to operate at 20
          //next_direction = 180;
          //facing_motion(current_direction, next_direction);
          //current_direction = next_direction;
        }
        // average the 4 light sensors 
        else if (abs(current_light_avg - previous_light_avg) >= scared_light_thresh) {
          printf("AMBIENT --> SCARED \n");
          state = SCARED;
          timer_start = current_time;
          flashLEDs((current_time - timer_start));
        }
        else {
          printf("AMBIENT STATE\n");

          float scaler = 1 - (float) current_light_avg / 255.0;
          r = 0 * scaler; //- ambient_light;
          g = 0 * scaler; //- ambient_light;
          b = 255 * scaler;

          r = boundInt(r, 0, 255);
          g = boundInt(g, 0, 255);
          b = boundInt(b, 0, 255);

          set_LED_color(r, g, b);
          LEDS_ON();
          if ((current_light.light1 <= current_light.light2) && (current_light.light1 <= current_light.light3) && (current_light.light1 <= current_light.light4)) {
            max_direction = LIGHT0_PIN;
          }
          else if ((current_light.light2 <= current_light.light1) && (current_light.light2 <= current_light.light3) && (current_light.light2 <= current_light.light4)) {
            max_direction = LIGHT1_PIN;
          }
          else if ((current_light.light3 <= current_light.light1) && (current_light.light3 <= current_light.light4) && (current_light.light3 <= current_light.light2)) {
            max_direction = LIGHT2_PIN;
          }
          else if ((current_light.light4 <= current_light.light1) && (current_light.light4 <= current_light.light3) && (current_light.light4 <= current_light.light2)) {
            max_direction = LIGHT3_PIN;
          }
          printf("Max light came from LED %d\n", max_direction);
          if (max_direction == FRONT) {
            next_direction = 0;
          }
          else if (max_direction == RIGHT) {
            next_direction = 90;
          }
          else if (max_direction == BACK) {
            next_direction = 180;
          }
          else if (max_direction == LEFT) {
            next_direction = 270;
          }
          printf("The direction of max light is: %d\n", next_direction);
          printf("Turning to face max light\n");
          face_motion(current_direction, next_direction);
          printf("Finished turning to face max light\n");
          current_direction = next_direction;
          //kobukiDriveDirect(0, 0);
        }
        break; // each case needs to end with break!
      }

      case ATTENTION: {
        //
        timer_counter = current_time - timer_start;
        //uint16_t curr_encoder = sensors.rightWheelEncoder;
        //float value = measure_distance(curr_encoder, last_encoder);
        //printf("value, %d, %f \n", curr_encoder, value);
        //distance_traveled += value;
        //last_encoder = curr_encoder;
        //facing_motion = distance_traveled;
        if (abs(current_light_avg - previous_light_avg) >= scared_light_thresh) {
          printf("ATTENTION --> SCARED \n");
          state = SCARED;
          timer_start = current_time;
          flashLEDs((current_time - timer_start));
        }
        else if (timer_counter > timer_thresh) {
           printf("ATTENTION --> AMBIENT \n");
           state = AMBIENT;
         }
        //else if (distance_traveled > turn_thresh) {
        //  printf("ATTENTION --> TOUCH \n");
        //  state = TOUCH;
        //}
        else {
          printf("ATTENTION STATE\n");
          //kobukiDriveDirect(0, 20);
          //printf("     distance_traveled, %f, %f\n", distance_traveled, turn_thresh);
        }
        break; // each case needs to end with break!
      }

      case TOUCH: {
        timer_counter = current_time - timer_start;
        if (abs(current_light_avg - previous_light_avg) >= scared_light_thresh) {
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
          //vibrate();
          set_LED_color(148, 0, 211); // purple!
          LEDS_ON();
          //kobukiDriveDirect(0, 0);
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

void state_machine_extended_angry() {
   while (1) {
    printf("\n\n");
    kobukiSensorPoll(&sensors);
    printf("sensors: %d, %d \n", sensors.rightWheelEncoder, sensors.leftWheelEncoder);
    printf("Current direction is: %d, where 0 degrees is front and 180 is back\n", current_direction);
    previous_time = current_time;
    current_time = read_timer();
    previous_light = current_light;
    current_light = read_light_sensors();
    previous_light_avg = (previous_light.light1 + previous_light.light2 + previous_light.light3 + previous_light.light4)/4;
    current_light_avg = (current_light.light1 + current_light.light2 + current_light.light3 + current_light.light4)/4;
    motion_yn = read_motion_sensor();
    touch_struct = read_touch_sensors();
    // touch_state = (!touch_struct.touch1 || !touch_struct.touch3); // || touch_struct.touch2 || touch_struct.touch3 || touch_struct.touch4);
    touch_buffer[touch_index] = 1;

    //if it ever reads 0, clear the whole thing
    if (touch_struct.touch1 && touch_struct.touch3) {
        memset(touch_buffer, 0, sizeof(touch_buffer));
    }
    touch_state = sumTouchBuffer() >= touch_threshold;
    touch_index = (touch_index + 1) % touch_length;
    printf("touch buffer: %d, %d, %d, %d, %d \n", touch_buffer[0], touch_buffer[1], touch_buffer[2], touch_buffer[3], touch_buffer[4]);
    nrf_delay_ms(1);

    printf("current, prev, thresh: %d, %d, %d, %d \n ", current_light_avg, previous_light_avg, abs(current_light_avg - previous_light_avg), scared_light_thresh);
    printf("touched? %d \n", touch_state);

    switch(state) {
      case AMBIENT: {
        //set LEDs based on ambient light
        // transition logic
        if (touch_state) {
          printf("AMBIENT --> TOUCH\n");
          state = TOUCH;
          timer_start = current_time;
          printf("going to vibrate now\n");
          vibrate();
          printf("ended vibration\n");
        }
        else if (motion_yn) {
          printf("AMBIENT --> MOTION \n");
          state = ATTENTION;
          //last_encoder = sensors.rightWheelEncoder;
          //distance_traveled = 0.0;
          timer_start = current_time;
          // to turn, need to operate at 20
          next_direction = 0;
          printf("turning towards motion, towards direction: %d\n", next_direction);
          facing_motion(current_direction, next_direction);
          printf("done turning towards motion\n");
          current_direction = next_direction;
        }
        // average the 4 light sensors 
        else if (abs(current_light_avg - previous_light_avg) >= scared_light_thresh) {
          printf("AMBIENT --> SCARED \n");
          state = SCARED;
          timer_start = current_time;
          flashLEDs((current_time - timer_start));
        }
        else {
          printf("AMBIENT STATE\n");

          float scaler = 1 - (float) current_light_avg / 255.0;
          r = 255 * scaler; //- ambient_light;
          g = 0 * scaler; //- ambient_light;
          b = 0 * scaler;

          r = boundInt(r, 0, 255);
          g = boundInt(g, 0, 255);
          b = boundInt(b, 0, 255);

          set_LED_color(r, g, b);
          LEDS_ON();
          if ((current_light.light1 >= current_light.light2) && (current_light.light1 >= current_light.light3) && (current_light.light1 >= current_light.light4)) {
            max_direction = LIGHT0_PIN;
          }
          else if ((current_light.light2 >= current_light.light1) && (current_light.light2 >= current_light.light3) && (current_light.light2 >= current_light.light4)) {
            max_direction = LIGHT1_PIN;
          }
          else if ((current_light.light3 >= current_light.light1) && (current_light.light3 >= current_light.light4) && (current_light.light3 >= current_light.light2)) {
            max_direction = LIGHT2_PIN;
          }
          else if ((current_light.light4 >= current_light.light1) && (current_light.light4 >= current_light.light3) && (current_light.light4 >= current_light.light2)) {
            max_direction = LIGHT3_PIN;
          }
          printf("Max light came from LED %d\n", max_direction);
          if (max_direction == FRONT) {
            next_direction = 0;
          }
          else if (max_direction == RIGHT) {
            next_direction = 90;
          }
          else if (max_direction == BACK) {
            next_direction = 180;
          }
          else if (max_direction == LEFT) {
            next_direction = 270;
          }
          printf("The direction of max light is: %d\n", next_direction);
          printf("Turning to face max light\n");
          face_motion(current_direction, next_direction);
          printf("Finished turning to face max light\n");
          current_direction = next_direction;
          //kobukiDriveDirect(0, 0);
        }
        break; // each case needs to end with break!
      }

      case ATTENTION: {
        //
        timer_counter = current_time - timer_start;
        //uint16_t curr_encoder = sensors.rightWheelEncoder;
        //float value = measure_distance(curr_encoder, last_encoder);
        //printf("value, %d, %f \n", curr_encoder, value);
        //distance_traveled += value;
        //last_encoder = curr_encoder;
        //facing_motion = distance_traveled;
        if (abs(current_light_avg - previous_light_avg) >= scared_light_thresh) {
          printf("ATTENTION --> SCARED \n");
          state = SCARED;
          timer_start = current_time;
          flashLEDs((current_time - timer_start));
        }
        else if (timer_counter > timer_thresh) {
           printf("ATTENTION --> AMBIENT \n");
           state = AMBIENT;
         }
        //else if (distance_traveled > turn_thresh) {
        //  printf("ATTENTION --> TOUCH \n");
        //  state = TOUCH;
        //}
        else {
          printf("ATTENTION STATE\n");
          //kobukiDriveDirect(0, 20);
          //printf("     distance_traveled, %f, %f\n", distance_traveled, turn_thresh);
        }
        break; // each case needs to end with break!
      }

      case TOUCH: {
        timer_counter = current_time - timer_start;
        if (abs(current_light_avg - previous_light_avg) >= scared_light_thresh) {
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
          //vibrate();
          set_LED_color(148, 0, 211); // purple!
          LEDS_ON();
          //kobukiDriveDirect(0, 0);
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

void state_machine_extended() {
   while (1) {
    printf("\n\n");
    kobukiSensorPoll(&sensors);
    printf("sensors: %d, %d \n", sensors.rightWheelEncoder, sensors.leftWheelEncoder);
    printf("Current direction is: %d, where 0 degrees is front and 180 is back\n", current_direction);
    previous_time = current_time;
    current_time = read_timer();
    previous_light = current_light;
    current_light = read_light_sensors();
    previous_light_avg = (previous_light.light1 + previous_light.light2 + previous_light.light3 + previous_light.light4)/4;
    current_light_avg = (current_light.light1 + current_light.light2 + current_light.light3 + current_light.light4)/4;
    motion_yn = read_motion_sensor();
    touch_struct = read_touch_sensors();
    // touch_state = (!touch_struct.touch1 || !touch_struct.touch3); // || touch_struct.touch2 || touch_struct.touch3 || touch_struct.touch4);
    touch_buffer[touch_index] = 1;

    //if it ever reads 0, clear the whole thing
    if (touch_struct.touch1 && touch_struct.touch3) {
        memset(touch_buffer, 0, sizeof(touch_buffer));
    }
    touch_state = sumTouchBuffer() >= touch_threshold;
    touch_index = (touch_index + 1) % touch_length;
    printf("touch buffer: %d, %d, %d, %d, %d \n", touch_buffer[0], touch_buffer[1], touch_buffer[2], touch_buffer[3], touch_buffer[4]);
    nrf_delay_ms(1);

    printf("current, prev, thresh: %d, %d, %d, %d \n ", current_light_avg, previous_light_avg, abs(current_light_avg - previous_light_avg), scared_light_thresh);
    printf("touched? %d \n", touch_state);

    switch(state) {
      case AMBIENT: {
        //set LEDs based on ambient light
        // transition logic
        if (touch_state) {
          printf("AMBIENT --> TOUCH\n");
          state = TOUCH;
          timer_start = current_time;
          printf("going to vibrate now\n");
          vibrate();
          printf("ended vibration\n");
        }
        else if (motion_yn) {
          printf("AMBIENT --> MOTION \n");
          state = ATTENTION;
          //last_encoder = sensors.rightWheelEncoder;
          //distance_traveled = 0.0;
          timer_start = current_time;
          // to turn, need to operate at 20
          next_direction = 0;
          printf("turning towards motion, towards direction: %d\n", next_direction);
          facing_motion(current_direction, next_direction);
          printf("done turning towards motion\n");
          current_direction = next_direction;
        }
        // average the 4 light sensors 
        else if (abs(current_light_avg - previous_light_avg) >= scared_light_thresh) {
          printf("AMBIENT --> SCARED \n");
          state = SCARED;
          timer_start = current_time;
          flashLEDs((current_time - timer_start));
        }
        else {
          printf("AMBIENT STATE\n");

          float scaler = 1 - (float) current_light_avg / 255.0;
          r = 155 * scaler; //- ambient_light;
          g = 40 * scaler; //- ambient_light;
          b = 10 * scaler;

          r = boundInt(r, 0, 255);
          g = boundInt(g, 0, 255);
          b = boundInt(b, 0, 255);

          set_LED_color(r, g, b);
          LEDS_ON();
          if ((current_light.light1 >= current_light.light2) && (current_light.light1 >= current_light.light3) && (current_light.light1 >= current_light.light4)) {
            max_direction = LIGHT0_PIN;
          }
          else if ((current_light.light2 >= current_light.light1) && (current_light.light2 >= current_light.light3) && (current_light.light2 >= current_light.light4)) {
            max_direction = LIGHT1_PIN;
          }
          else if ((current_light.light3 >= current_light.light1) && (current_light.light3 >= current_light.light4) && (current_light.light3 >= current_light.light2)) {
            max_direction = LIGHT2_PIN;
          }
          else if ((current_light.light4 >= current_light.light1) && (current_light.light4 >= current_light.light3) && (current_light.light4 >= current_light.light2)) {
            max_direction = LIGHT3_PIN;
          }
          printf("Max light came from LED %d\n", max_direction);
          if (max_direction == FRONT) {
            next_direction = 0;
          }
          else if (max_direction == RIGHT) {
            next_direction = 90;
          }
          else if (max_direction == BACK) {
            next_direction = 180;
          }
          else if (max_direction == LEFT) {
            next_direction = 270;
          }
          printf("The direction of max light is: %d\n", next_direction);
          printf("Turning to face max light\n");
          face_motion(current_direction, next_direction);
          printf("Finished turning to face max light\n");
          current_direction = next_direction;
          //kobukiDriveDirect(0, 0);
        }
        break; // each case needs to end with break!
      }

      case ATTENTION: {
        //
        timer_counter = current_time - timer_start;
        //uint16_t curr_encoder = sensors.rightWheelEncoder;
        //float value = measure_distance(curr_encoder, last_encoder);
        //printf("value, %d, %f \n", curr_encoder, value);
        //distance_traveled += value;
        //last_encoder = curr_encoder;
        //facing_motion = distance_traveled;
        if (abs(current_light_avg - previous_light_avg) >= scared_light_thresh) {
          printf("ATTENTION --> SCARED \n");
          state = SCARED;
          timer_start = current_time;
          flashLEDs((current_time - timer_start));
        }
        else if (timer_counter > timer_thresh) {
           printf("ATTENTION --> AMBIENT \n");
           state = AMBIENT;
         }
        //else if (distance_traveled > turn_thresh) {
        //  printf("ATTENTION --> TOUCH \n");
        //  state = TOUCH;
        //}
        else {
          printf("ATTENTION STATE\n");
          //kobukiDriveDirect(0, 20);
          //printf("     distance_traveled, %f, %f\n", distance_traveled, turn_thresh);
        }
        break; // each case needs to end with break!
      }

      case TOUCH: {
        timer_counter = current_time - timer_start;
        if (abs(current_light_avg - previous_light_avg) >= scared_light_thresh) {
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
          //vibrate();
          set_LED_color(148, 0, 211); // purple!
          LEDS_ON();
          //kobukiDriveDirect(0, 0);
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

int main(void) {
  ret_code_t error_code = NRF_SUCCESS;

  // initialize RTT library
  error_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(error_code);
  NRF_LOG_DEFAULT_BACKENDS_INIT();
  printf("Log initialized!\n");

  // initialize LEDs
  // nrf_gpio_pin_dir_set(23, NRF_GPIO_PIN_DIR_OUTPUT);
  // nrf_gpio_pin_dir_set(24, NRF_GPIO_PIN_DIR_OUTPUT);
  // nrf_gpio_pin_dir_set(25, NRF_GPIO_PIN_DIR_OUTPUT);

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
  printf("All sensors initialized!\n");
  virtual_timer_init();

  current_light = read_light_sensors();
  previous_light = current_light;
  srand(current_time);
  today_mood = rand() % 2;
  printf("Today's mood is: %d \n", today_mood);
  //state_machine();

  //uncomment for moods:
  if (today_mood == 0) {
    state_machine_extended();
  }
  else if (today_mood == 1) {
    state_machine_extended_angry();
    //led's red in ambient
    //turns away from you for motion 
  }
  else if (today_mood == 2) {
    state_machine_extended_sad();
    //led's blue in ambient
    //face in direction of least light, only vibrate for motion or touch
  }


  //uint32_t r = 0;
  //uint32_t g = 0;
  //uint32_t b = 0;

  //bool redup = true;
  //bool greenup = true;
  //bool blueup = true;

  // loop forever
  // while (1) {
    // printf("\n\n");
    // current_light = read_light_sensors();
    // kobukiSensorPoll(&sensors);
    // previous_time = current_time;
    // current_time = read_timer();

    // //printf("current time is: %d \n", current_time);
    // previous_light = current_light;
    // previous_light_avg = (previous_light.light1 + previous_light.light2 + previous_light.light3 + previous_light.light4)/4;
    // current_light_avg = (current_light.light1 + current_light.light2 + current_light.light3 + current_light.light4)/4;
    // printf("Avg light is %d \n", current_light_avg);
    // printf("lights: %d, %d, %d, %d \n", current_light.light1, current_light.light2, current_light.light3, current_light.light4);
    // current_light_avg = boundInt(current_light_avg, 1, 255);

    // float scaler = 1 - (float) current_light_avg / 255.0;
    // r = 155 * scaler; //- ambient_light;
    // g = 40 * scaler; //- ambient_light;
    // b = 10 * scaler;

    // r = boundInt(r, 0, 255);
    // g = boundInt(g, 0, 255);
    // b = boundInt(b, 0, 255);

    // set_LED_color(r, g, b);
    // LEDS_ON();

    // printf("The set light is r = %d, g = %d, b = %d \n", r, g, b);
    // motion_yn = read_motion_sensor();
    // printf("is there motion %d \n", motion_yn);
    // touch_struct = read_touch_sensors();
    // touch_state = (!touch_struct.touch1 || !touch_struct.touch2); //(!touch_struct.touch0 || !touch_struct.touch1 || !touch_struct.touch2 || !touch_struct.touch4);
    // // printf("is there touch %d \n", touch_state);
    // printf("touch: %d, %d, %d, %d, %d\n", touch_struct.touch0, touch_struct.touch1, touch_struct.touch2, touch_struct.touch3, touch_struct.touch4);
    // nrf_delay_ms(100);
    
    // kobukiDriveDirect(20,20);





    // printf("************loop\n");
    // light_values_t light_values = read_light_sensors();
    // printf("Light Values: %d, %d, %d, %d\n", light_values.light1, light_values.light2, light_values.light3, light_values.light4);
  
    // printf("%d, %d %d \n", redup, (!greenup) && (g > 0), blueup);

    // if (redup && r < 255) {
    //   r = (r + 5);
    // } else if (redup && r >= 255) {
    //   redup = false;
    // } else if ((!redup) && (r > 0)) {
    //   r = r - 5;
    // } else {
    //   redup = true;
    // }

    // if (greenup && g < 255) {
    //   g = (g + 10);
    // } else if (greenup && g >= 255) {
    //   greenup = false;
    // } else if ((!greenup) && (g > 0)) {
    //   g = (g - 10);
    // } else {
    //   greenup = true;
    // }

    // if (blueup && b < 255) {
    //   b = (b + 15);
    // } else if (blueup && b >= 255) {
    //   blueup = false;
    // } else if ((!blueup) && (b > 0)) {
    //   b = b - 15;
    // } else {
    //   blueup = true;
    // }

    // printf("rgb %u %u %u \n",r, g, b);
    // // g = (g + 10) % 255;
    // // b = (b + 20) % 255;

    // set_LED_color(r, g, b);
    // LEDS_ON();

  // }

}






