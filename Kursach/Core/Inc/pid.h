/*
 * pid.h
 *
 *  Created on: 21 окт. 2026 г.
 *      Author: nick
 */

#ifndef INC_PID_H_
#define INC_PID_H_

#include "inttypes.h"

typedef struct
{
  float kp;
  float ki;
  float kd;
  float* input;
  float setpoint;
} PID_Settings_t;

typedef struct
{
  float dT;
  float error;
  float prev_error;
  float integral;
  float derivative;
  float output;
  float scale_output;
} PID_Control_t;

void PID_Init(PID_Settings_t* pid_settings, PID_Control_t* pid_control, float* input_parameter);
void PID_Update(PID_Control_t* pid_control, PID_Settings_t pid_settings);

#endif /* INC_PID_H_ */
