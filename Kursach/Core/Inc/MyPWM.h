/*
 * MyPWM.h
 *
 *  Created on: Sep 11, 2025
 *      Author: Nikita
 */

#ifndef INC_MYPWM_H_
#define INC_MYPWM_H_

#include "stdbool.h"
#include "main.h"
#include "math.h"

#define PWM_DEFAULT_FREQUENCY_HZ (1000U)
#define PWM_DEFAULT_DUTY_CYCLE (50.0f)


typedef struct
{
	TIM_HandleTypeDef* timer_handler;
	uint32_t channel;
	uint32_t frequency_hz;
	float duty_cycle_percent;
	bool is_initialized;
	bool is_running;
} PWM_Handle_t;

typedef enum
{
	PWM_STATUS_OK,
	PWM_STATUS_ERROR_NULL_POINTER,
	PWM_STATUS_ERROR_INVALID_CHANNEL,
	PWM_STATUS_ERROR_INVALID_INITIALIZATION,
	PWM_STATUS_ERROR_CALCULATION_OVERFLOW,
	PWM_STATUS_ERROR_HAL_START_ERROR,
	PWM_STATUS_ERROR_HAL_STOP_ERROR,
	PWM_STATUS_ERROR_HAL_START_IT_ERROR,
	PWM_STATUS_ERROR_HAL_STOP_IT_ERROR,
	PWM_STATUS_ERROR_ALSO_RUNNING,
	PWM_STATUS_ERROR_ALSO_STOPPED,
	PWM_STATUS_ERROR_HAL_START_DMA_ERROR
} PWM_Status_t;

/*prototypes of public functions*/
PWM_Status_t PWM_Init(PWM_Handle_t* pwm_handler, TIM_HandleTypeDef* timer_handler, uint32_t channel);
PWM_Status_t PWM_Configure(PWM_Handle_t* pwm_handler);
PWM_Status_t PWM_Start (PWM_Handle_t* pwm_handler);
PWM_Status_t PWM_Stop (PWM_Handle_t* pwm_handler);
bool PWM_IsRunning (PWM_Handle_t* pwm_handler);
PWM_Status_t PWM_SetDutyCyclePercent(PWM_Handle_t* pwm_handler, float duty_cycle_percent);
PWM_Status_t PWM_SetFrequency (PWM_Handle_t* pwm_handler, uint32_t frequency);
PWM_Status_t PWM_GetFrequency (PWM_Handle_t* pwm_handler, uint32_t* frequency_hz);
PWM_Status_t PWM_GetDutyCyclePercent (PWM_Handle_t* pwm_handler, float* duty_cycle_percent);
PWM_Status_t PWM_Start_It(PWM_Handle_t* pwm_handler);
PWM_Status_t PWM_Stop_It(PWM_Handle_t* pwm_handler);
PWM_Status_t PWM_Start_DMA (PWM_Handle_t* pwm_handler);

#endif /* INC_MYPWM_H_ */
