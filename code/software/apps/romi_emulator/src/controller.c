#include "controller.h"
#include <math.h>
#include <stdio.h>

#define SIMULATION true

#if SIMULATION
#include "simulatorFunctions.h"
#else
#include "kobukiSensorTypes.h"
#include "display.h"
#endif

// Configure initial state
static KobukiSensors_t sensors = {0};
static robot_state_t state = OFF;
static uint16_t last_encoder = 0;
static float distance_traveled = 0.0;

// You may need to add additional variables to keep track of state here

// Return distance traveled between two encoder values
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

// Return true if a cliff has been seen
// Save information about which cliff
static bool check_and_save_bump(KobukiSensors_t* sensors, bool* obstacle_is_right) {
  // Your code here
}

bool cliff_left, cliff_center, cliff_right;

// Return true if a cliff has been seen
// Save information about which cliff
static bool check_cliff(KobukiSensors_t* sensors) {
  cliff_left = sensors->cliffLeft;
  cliff_center = sensors->cliffCenter;
  cliff_right = sensors->cliffRight;
  if (cliff_left || cliff_center || cliff_right) {
    return 1;
  }
  else {
    return 0;
  }
}

// Read accelerometer value and calculate and return tilt (along axis corresponding to climbing the hill)
static float read_tilt() {
  lsm9ds1_measurement_t accelerometer_values = lsm9ds1_read_accelerometer();
  float  x_g = accelerometer_values.x_axis;
  float  y_g = accelerometer_values.y_axis;
  float  z_g = accelerometer_values.z_axis;
  float  theta = atan(x_g/(sqrt(pow(y_g,2) + pow(z_g,2))))*180/3.14;
  float  psi = atan(y_g/(sqrt(pow(x_g,2) + pow(z_g,2))))*180/3.14;
  float  phi = atan((sqrt(pow(x_g,2) + pow(y_g,2))/z_g))*180/3.14;
  return phi;
}

static float read_psi_y() {
  lsm9ds1_measurement_t accelerometer_values = lsm9ds1_read_accelerometer();
  float  x_g = accelerometer_values.x_axis;
  float  y_g = accelerometer_values.y_axis;
  float  z_g = accelerometer_values.z_axis;
  //float  theta = atan(x_g/(sqrt(pow(y_g,2) + pow(z_g,2))))*180/3.14;
  float  psi = atan(y_g/(sqrt(pow(x_g,2) + pow(z_g,2))))*180/3.14;
  //float  phi = atan((sqrt(pow(x_g,2) + pow(y_g,2))/z_g))*180/3.14;
  return psi;
}

static float read_theta_x() {
  lsm9ds1_measurement_t accelerometer_values = lsm9ds1_read_accelerometer();
  float  x_g = accelerometer_values.x_axis;
  float  y_g = accelerometer_values.y_axis;
  float  z_g = accelerometer_values.z_axis;
  float  theta = atan(x_g/(sqrt(pow(y_g,2) + pow(z_g,2))))*180/3.14;
  //float  psi = atan(y_g/(sqrt(pow(x_g,2) + pow(z_g,2))))*180/3.14;
  //float  phi = atan((sqrt(pow(x_g,2) + pow(y_g,2))/z_g))*180/3.14;
  return theta;
}

int i;
float min_theta;
float current_theta, previous_theta;
float current_psi;
float curr_tilt, prev_tilt;
bool up0_down1;

// Robot controller
// State machine running on robot_state_t state
// This is called in a while loop
robot_state_t controller(robot_state_t state) {

  kobukiSensorPoll(&sensors);
  nrf_delay_ms(1);
  float tilt = read_tilt();
  bool is_cliff = check_cliff(&sensors);
  float tilt_accel = read_tilt();
  //display_float(tilt_accel);
    // handle states
    switch(state) {
      display_float((int)state);
      case OFF: {
        // transition logic
        if (is_button_pressed(&sensors)) {
          last_encoder = sensors.leftWheelEncoder;
          distance_traveled = 0.0;
          state = TURN_UPHILL;
          kobukiDriveDirect(20, -20);
          min_theta = read_theta_x();
          display_float(min_theta);
          display_float(tilt_accel);
        } else {
          // perform state-specific actions here
          display_write("OFF", DISPLAY_LINE_0);
          display_write("", DISPLAY_LINE_1);
          kobukiDriveDirect(0, 0);
          state = OFF;
        }
        break; // each case needs to end with break!
      }

      case TURN_UPHILL: {
        // transition logic
        if (is_button_pressed(&sensors)) {
          state = OFF; 
        } else if (cliff_right || cliff_center) {
          lsm9ds1_stop_gyro_integration();
          // allow encoder momentum to stop
          display_write("PAUSING", DISPLAY_LINE_0);
          kobukiDriveDirect(0, 0);
          for (i = 0; i < 30; i++) {
            kobukiSensorPoll(&sensors);
            nrf_delay_ms(1);
          }
          distance_traveled = 0;
          last_encoder = sensors.leftWheelEncoder;
          state = BACK_UP_LEFT;
        } else if (cliff_left) {
          lsm9ds1_stop_gyro_integration();
          // allow encoder momentum to stop
          display_write("PAUSING", DISPLAY_LINE_0);
          kobukiDriveDirect(0, 0);
          for (i = 0; i < 30; i++) {
            kobukiSensorPoll(&sensors);
            nrf_delay_ms(1);
          }
          distance_traveled = 0;
          last_encoder = sensors.leftWheelEncoder;
          state = BACK_UP_RIGHT;
        } else {
          display_write("TURN_UPHILL", DISPLAY_LINE_0);
          current_theta = read_theta_x();
          current_psi = read_psi_y();
          //display_float(current_theta);
          display_write(" ",DISPLAY_LINE_0);
          //display_float(current_psi);
          //display_float(min_theta);
          //display_float(tilt_accel);
          if ((((previous_theta > 0) && (current_theta <= 0)) || ((previous_theta < 0) && (current_theta >=0))) && (current_psi <= 0)) {
            state = DRIVING_UP;
            kobukiDriveDirect(100,100);
          }
          else {
            previous_theta = current_theta;
            //display_float(min_theta);
            kobukiDriveDirect(50,-50);
            state = TURN_UPHILL;
          }
        }
        break; // each case needs to end with break!
      }

      case DRIVING_UP: {
        // transition logic
        display_float(tilt_accel);
        display_write(" ", DISPLAY_LINE_0);
        if (is_button_pressed(&sensors)) {
          state = OFF;
        } else if (cliff_right || cliff_center) {
            display_write("PAUSING", DISPLAY_LINE_0);
            kobukiDriveDirect(0, 0);
            for (i = 0; i < 30; i++) {
              kobukiSensorPoll(&sensors);
              nrf_delay_ms(1);
            }
            distance_traveled = 0;
            display_float(distance_traveled);
            last_encoder = sensors.leftWheelEncoder;
            //display_float(last_encoder);
            //kobukiDriveDirect(-100, -100);
            up0_down1 = 0;
          state = BACK_UP_LEFT;
        } else if (cliff_left) {
            display_write("PAUSING", DISPLAY_LINE_0);
            kobukiDriveDirect(0, 0);
            for (i = 0; i < 30; i++) {
              kobukiSensorPoll(&sensors);
              nrf_delay_ms(1);
            }
            distance_traveled = 0;
            display_float(distance_traveled);
            last_encoder = sensors.leftWheelEncoder;
            up0_down1 = 0;
            state = BACK_UP_RIGHT;
        } else {
          // perform state-specific actions here
          display_write("DRIVING_UP", DISPLAY_LINE_0);
          //uint16_t curr_encoder = sensors.leftWheelEncoder;
          //float value = measure_distance(curr_encoder, last_encoder);
          //distance_traveled += value;
          //last_encoder = curr_encoder;
          //display_float(distance_traveled);
          if (tilt_accel <= 3) {
            //lsm9ds1_start_gyro_integration();
            distance_traveled = 0;
            last_encoder = sensors.leftWheelEncoder;
            state = DRIVE_FURTHER;
            kobukiDriveDirect(100, 100);
          } else {
            kobukiDriveDirect(100, 100);
            state = DRIVING_UP;
          }
        }
        break; // each case needs to end with break!
      }

      case DRIVE_FURTHER: {
        // transition logic
        display_float(tilt_accel);
        display_write(" ", DISPLAY_LINE_0);
        if (is_button_pressed(&sensors)) {
          state = OFF;
        } else if (cliff_right || cliff_center) {
            display_write("PAUSING", DISPLAY_LINE_0);
            kobukiDriveDirect(0, 0);
            for (i = 0; i < 30; i++) {
              kobukiSensorPoll(&sensors);
              nrf_delay_ms(1);
            }
            distance_traveled = 0;
            display_float(distance_traveled);
            last_encoder = sensors.leftWheelEncoder;
            //display_float(last_encoder);
            //kobukiDriveDirect(-100, -100);
            up0_down1 = 0;
            state = BACK_UP_LEFT;
        } else if (cliff_left) {
            display_write("PAUSING", DISPLAY_LINE_0);
            kobukiDriveDirect(0, 0);
            for (i = 0; i < 30; i++) {
              kobukiSensorPoll(&sensors);
              nrf_delay_ms(1);
            }
            distance_traveled = 0;
            display_float(distance_traveled);
            last_encoder = sensors.leftWheelEncoder;
            up0_down1 = 0;
            state = BACK_UP_RIGHT;
        } else {
          // perform state-specific actions here
          display_write("DRIVING_UP", DISPLAY_LINE_0);
          uint16_t curr_encoder = sensors.leftWheelEncoder;
          float value = measure_distance(curr_encoder, last_encoder);
          distance_traveled += value;
          last_encoder = curr_encoder;
          display_float(distance_traveled);
          if (distance_traveled < 0.1) {
            //distance_traveled = 0;
            //last_encoder = sensors.leftWheelEncoder;
            state = DRIVE_FURTHER;
            kobukiDriveDirect(100, 100);
          } else {
            lsm9ds1_start_gyro_integration();
            state = TURN_CW;
          }
        }
        break; // each case needs to end with break!
      }

      // add other cases here
      case TURN_CW: {
        if (is_button_pressed(&sensors)) {
          lsm9ds1_stop_gyro_integration();
          state = OFF;
        // } else if (cliff_right || cliff_center) {
        //   lsm9ds1_stop_gyro_integration();
        //   // allow encoder momentum to stop
        //   display_write("PAUSING, cliff R or C ", DISPLAY_LINE_0);
        //   kobukiDriveDirect(0, 0);
        //   for (i = 0; i < 30; i++) {
        //     kobukiSensorPoll(&sensors);
        //     nrf_delay_ms(1);
        //   }
        //   distance_traveled = 0;
        //   last_encoder = sensors.leftWheelEncoder;
        //   up0_down1 = 1;
        //   state = BACK_UP_LEFT;
        // } else if (cliff_left) {
        //   lsm9ds1_stop_gyro_integration();
        //   // allow encoder momentum to stop
        //   display_write("PAUSING, cliff L", DISPLAY_LINE_0);
        //   kobukiDriveDirect(0, 0);
        //   for (i = 0; i < 30; i++) {
        //     kobukiSensorPoll(&sensors);
        //     nrf_delay_ms(1);
        //   }
        //   distance_traveled = 0;
        //   last_encoder = sensors.leftWheelEncoder;
        //   up0_down1 = 1;
        //   state = BACK_UP_RIGHT;
        } else {
          // returns angles in degrees, cw is negative
          float angle_turned = fabs(lsm9ds1_read_gyro_integration().z_axis);
          display_write(" ", DISPLAY_LINE_0);
          display_float(angle_turned);
          if (angle_turned >= 160) {
            lsm9ds1_stop_gyro_integration();
            // allow encoder momentum to stop
            display_write("PAUSING", DISPLAY_LINE_0);
            kobukiDriveDirect(0, 0);
            for (i = 0; i < 30; i++) {
              kobukiSensorPoll(&sensors);
              nrf_delay_ms(1);
            }
            distance_traveled = 0;
            last_encoder = sensors.leftWheelEncoder;
            curr_tilt = tilt_accel;
            state = DRIVING_DOWN;
            kobukiDriveDirect(100,100);
          } else {
            display_write("TURNING", DISPLAY_LINE_0);
            kobukiDriveDirect(40, -40);
            display_float(angle_turned);
            state = TURN_CW;
          }
        }
        break;
      }

      case DRIVING_DOWN: {
        // transition logic
        display_float(tilt_accel);
        display_write(" ", DISPLAY_LINE_0);
        if (is_button_pressed(&sensors)) {
          state = OFF;
        } else if ((cliff_right || cliff_center) && (distance_traveled > 0.15)) {
            display_write("PAUSING, cliff_left", DISPLAY_LINE_0);
            kobukiDriveDirect(0, 0);
            for (i = 0; i < 30; i++) {
              kobukiSensorPoll(&sensors);
              nrf_delay_ms(1);
            }
            distance_traveled = 0;
            display_float(distance_traveled);
            last_encoder = sensors.leftWheelEncoder;
            //display_float(last_encoder);
            //kobukiDriveDirect(-100, -100);
            up0_down1 = 1;
            state = BACK_UP_LEFT;
        } else if (cliff_left && (distance_traveled > 0.15)) {
            if (cliff_right) {
              display_write("PAUSING, CLIFF RIGHT", DISPLAY_LINE_0);
            }
            else {
              display_write("PAUSING, CLIFF CENTER", DISPLAY_LINE_0);
            }
            kobukiDriveDirect(0, 0);
            for (i = 0; i < 30; i++) {
              kobukiSensorPoll(&sensors);
              nrf_delay_ms(1);
            }
            distance_traveled = 0;
            display_float(distance_traveled);
            last_encoder = sensors.leftWheelEncoder;
            curr_tilt = tilt_accel;
            up0_down1 = 1;
            state = BACK_UP_RIGHT;
        } else {
          // perform state-specific actions here
          display_write("DRIVING_DOWN ", DISPLAY_LINE_0);
          uint16_t curr_encoder = sensors.leftWheelEncoder;
          float value = measure_distance(curr_encoder, last_encoder);
          distance_traveled += value;
          last_encoder = curr_encoder;
          display_float(distance_traveled);
          display_write(" ", DISPLAY_LINE_0);
          prev_tilt = curr_tilt;
          curr_tilt = tilt_accel;
          if ((prev_tilt > 1) && (curr_tilt <= 1) && (distance_traveled > 0.3)) {
            //lsm9ds1_start_gyro_integration();
            state = OFF;
          } else {
            kobukiDriveDirect(100, 100);
            state = DRIVING_DOWN;
          }
        }
        break; // each case needs to end with break!
      }

      case BACK_UP_LEFT: {
        display_write("BACK UP LEFT", DISPLAY_LINE_0);
        if (is_button_pressed(&sensors)) {
          state = OFF;
        } 
        else {
          uint16_t curr_encoder = sensors.leftWheelEncoder;
          display_float(curr_encoder);
          float value = measure_distance(curr_encoder, last_encoder);
          display_float(value);
          distance_traveled -= value;
          last_encoder = curr_encoder;
          display_float(distance_traveled);
    
          if (distance_traveled <= -0.1) {
            //display_write("distance less than -1", DISPLAY_LINE_0);
            lsm9ds1_start_gyro_integration();
            state = TURN_AWAY_LEFT;
            //if (state == TURN_AWAY_LEFT) {
               //display_write("going to TURN_AWAY_LEFT", DISPLAY_LINE_0);
            //}
          } 
          else {
            //display_write("still backing up",DISPLAY_LINE_0);
             kobukiDriveDirect(-50, -50);
             state = BACK_UP_LEFT;
          }
        }
        break;
      }

      case BACK_UP_RIGHT: {
        display_write("BACK UP RIGHT", DISPLAY_LINE_0);
        if (is_button_pressed(&sensors)) {
          state = OFF;
        } 
        else {
          uint16_t curr_encoder = sensors.leftWheelEncoder;
          float value = measure_distance(curr_encoder, last_encoder);
          distance_traveled -= value;
          last_encoder = curr_encoder;
          display_float(distance_traveled);
    
          if (distance_traveled <= -0.1) {
            //display_write("distance less than -80", DISPLAY_LINE_0);
            lsm9ds1_start_gyro_integration();
            state = TURN_AWAY_RIGHT;
            //curr_tilt = tilt_accel;
            //if (state == TURN_AWAY_RIGHT) {
            //   display_write("TURN_AWAY_LEFT", DISPLAY_LINE_0);
            //} 
          }
          else {
            kobukiDriveDirect(-50, -50);
            state = BACK_UP_RIGHT;
          }
        }
        break;
      }

      case TURN_AWAY_LEFT: {
        if (is_button_pressed(&sensors)) {
          lsm9ds1_stop_gyro_integration();
          state = OFF;
        } else {
          float angle_turned = fabs(lsm9ds1_read_gyro_integration().z_axis);
          if (angle_turned >= 30) {
            lsm9ds1_stop_gyro_integration();
            // allow encoder momentum to stop
            display_write("PAUSING", DISPLAY_LINE_0);
            kobukiDriveDirect(0, 0);
            for (i = 0; i < 30; i++) {
              kobukiSensorPoll(&sensors);
              nrf_delay_ms(1);
            }
            distance_traveled = 0;
            last_encoder = sensors.leftWheelEncoder;
            if (up0_down1 == 0) {
              state = DRIVING_UP;
            }
            else {
              state = DRIVING_DOWN;
            }
          } else {
            display_write("EVADING LEFT", DISPLAY_LINE_0);
            display_float(angle_turned);
            kobukiDriveDirect(-40, 40);
            state = TURN_AWAY_LEFT;
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
          if (angle_turned >= 30) {
            lsm9ds1_stop_gyro_integration();
            // allow encoder momentum to stop
            display_write("PAUSING", DISPLAY_LINE_0);
            kobukiDriveDirect(0, 0);
            for (i = 0; i < 30; i++) {
              kobukiSensorPoll(&sensors);
              nrf_delay_ms(1);
            }
            distance_traveled = 0;
            last_encoder = sensors.leftWheelEncoder;
            if (up0_down1 == 0) {
              state = DRIVING_UP;
            }
            else {
              state = DRIVING_DOWN;
            }
          } else {
            display_write("EVADING RIGHT", DISPLAY_LINE_0);
            display_float(angle_turned);
            kobukiDriveDirect(40, -40);
            state = TURN_AWAY_RIGHT;
          }
        }
        break;
      }
    }
    //if (is_cliff) {
    //  state = OFF;
    //  display_write("Sensed Cliff", DISPLAY_LINE_0);
    //}
    return state;
}

void display_float(float v) {
  char buf [16];
  snprintf(buf, 16, "%f", v);
  display_write(buf, DISPLAY_LINE_1);
}

//int i; 

void pre_dir_change() {
  // allow encoder momentum to stop
  display_write("PAUSING", DISPLAY_LINE_0);
  kobukiDriveDirect(0, 0);
  for (i = 0; i < 30; i++) {
    kobukiSensorPoll(&sensors);
    nrf_delay_ms(1);
  }
  distance_traveled = 0;
  last_encoder = sensors.leftWheelEncoder;
}


// void back_up_state(bool left) {
//   if (is_button_pressed(&sensors)) {
//     state = OFF;
//   } else {
//     uint16_t curr_encoder = sensors.leftWheelEncoder;
//     float value = measure_distance_reversed(curr_encoder, last_encoder);
//     distance_traveled += value;
//     last_encoder = curr_encoder;
//     display_float(distance_traveled);
    
//     if (distance_traveled <= -1) {
//       display_write("distance less than -1", DISPLAY_LINE_0);
//       lsm9ds1_start_gyro_integration();
//       state = left ? TURN_AWAY_LEFT : TURN_AWAY_RIGHT;
//       if (state == TURN_AWAY_LEFT) {
//  display_write("TURN_AWAY_LEFT", DISPLAY_LINE_0);
//       }
//       else if (state == TURN_AWAY_RIGHT) {
//  display_write("TURN_AWAY_RIGHT", DISPLAY_LINE_0);
//       }
//     } else {
//       kobukiDriveDirect(-50, -50);
//       state = left ? BACK_UP_LEFT : BACK_UP_RIGHT;
//     }
//   }
// }
