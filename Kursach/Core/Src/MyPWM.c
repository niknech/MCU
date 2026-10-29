/*
 * PWM_manager.c
 *
 *  Created on: Sep 11, 2025
 *      Author: Nikita Nechitaylov
 */

#include "MyPWM.h"

/*prototypes of private functions*/
static PWM_Status_t PWM_ValidateTimer(TIM_HandleTypeDef* timer_handler);
static PWM_Status_t PWM_ValidateChannel(uint32_t channel);
static PWM_Status_t PWM_ValidateHandle(PWM_Handle_t* pwm_handler);
static uint32_t PWM_GetTimerClock(TIM_HandleTypeDef* timer);
static PWM_Status_t PWM_CalculateTimerValues(uint16_t* psc, uint32_t* arr, uint32_t timer_clock, uint32_t frequency, TIM_HandleTypeDef* timer_handle);
static uint32_t PWM_CalculateCCR (PWM_Handle_t* pwm_handler);


/*=============PWM INITIALIZATION=============*/
PWM_Status_t PWM_Init(PWM_Handle_t* pwm_handler, TIM_HandleTypeDef* timer_handler, uint32_t channel)
{
	/*input validation*/
	if (pwm_handler == NULL)
		return PWM_STATUS_ERROR_NULL_POINTER;

	PWM_Status_t status = PWM_ValidateTimer(timer_handler);
	if(status != PWM_STATUS_OK)
		return status;

	status = PWM_ValidateChannel(channel);
	if(status != PWM_STATUS_OK)
		return status;

	/*initialization*/
	pwm_handler->timer_handler = timer_handler;
	pwm_handler->channel = channel;
	pwm_handler->frequency_hz = PWM_DEFAULT_FREQUENCY_HZ;
	pwm_handler->duty_cycle_percent = PWM_DEFAULT_DUTY_CYCLE;
	pwm_handler->is_initialized = true;
	pwm_handler->is_running = false;

	/*configuration*/
	return PWM_Configure(pwm_handler);
}

/*=============VALIDATE TIMER HANDLE=============*/
static PWM_Status_t PWM_ValidateTimer(TIM_HandleTypeDef* timer_handler)
{
	if (timer_handler == NULL || timer_handler->Instance == NULL)
		return PWM_STATUS_ERROR_NULL_POINTER;
	return PWM_STATUS_OK;
}

/*=============VALIDATE CHANNEL=============*/
static PWM_Status_t PWM_ValidateChannel(uint32_t channel)
{
	switch (channel)
	{
	case (TIM_CHANNEL_1): return PWM_STATUS_OK;
	case (TIM_CHANNEL_2): return PWM_STATUS_OK;
	case (TIM_CHANNEL_3): return PWM_STATUS_OK;
	case (TIM_CHANNEL_4): return PWM_STATUS_OK;
	default: return PWM_STATUS_ERROR_INVALID_CHANNEL;
	}
}

/*=============VALIDATE PWM HANDLE=============*/
static PWM_Status_t PWM_ValidateHandle(PWM_Handle_t* pwm_handler)
{
	if (!pwm_handler->is_initialized)
		return PWM_STATUS_ERROR_INVALID_INITIALIZATION;

	return PWM_STATUS_OK;
}

/*=============CONFIGURE PWM=============*/
PWM_Status_t PWM_Configure(PWM_Handle_t* pwm_handler)
{
	PWM_Status_t status = PWM_ValidateHandle(pwm_handler);
	if (status != PWM_STATUS_OK)
		return status;

	uint16_t psc;
	uint32_t arr;
	status = PWM_CalculateTimerValues(&psc, &arr, PWM_GetTimerClock(pwm_handler->timer_handler), pwm_handler->frequency_hz, pwm_handler->timer_handler);
	if (status != PWM_STATUS_OK)
		return status;

	pwm_handler->timer_handler->Instance->ARR = arr - 1;
	pwm_handler->timer_handler->Instance->PSC = psc - 1;

	uint32_t ccr = (uint32_t) roundf((arr * pwm_handler->duty_cycle_percent) / 100.0f);
	if (ccr > arr)
		ccr = arr;

	__HAL_TIM_SET_COMPARE(pwm_handler->timer_handler, pwm_handler->channel, ccr);

	return PWM_STATUS_OK;
}

/*=============PWM START=============*/
PWM_Status_t PWM_Start (PWM_Handle_t* pwm_handler)
{
	PWM_Status_t status = PWM_ValidateHandle(pwm_handler);
	if (status != PWM_STATUS_OK)
		return status;

	if(pwm_handler->is_running)
		return PWM_STATUS_ERROR_ALSO_RUNNING; //also running

	HAL_StatusTypeDef HAL_status = HAL_TIM_PWM_Start(pwm_handler->timer_handler, pwm_handler->channel);
	if (HAL_status != HAL_OK)
		return PWM_STATUS_ERROR_HAL_START_ERROR;

	pwm_handler->is_running = true;
	return PWM_STATUS_OK;
}

/*=============PWM STOP=============*/
PWM_Status_t PWM_Stop (PWM_Handle_t* pwm_handler)
{
	PWM_Status_t status = PWM_ValidateHandle(pwm_handler);
	if (status != PWM_STATUS_OK)
		return status;

	if(!pwm_handler->is_running)
		return PWM_STATUS_ERROR_ALSO_STOPPED; //also was stopped

	HAL_StatusTypeDef HAL_status = HAL_TIM_PWM_Stop(pwm_handler->timer_handler, pwm_handler->channel);
	if (HAL_status != HAL_OK)
		return PWM_STATUS_ERROR_HAL_STOP_ERROR;

	pwm_handler->is_running = false;
	return PWM_STATUS_OK;
}

/*=============GET TIMER CLOCK=============*/
static uint32_t PWM_GetTimerClock(TIM_HandleTypeDef* timer)
{
	uint32_t timer_clock = 0;

	/*check APB2 timers*/
	if(timer->Instance == TIM11 || timer->Instance == TIM10 || timer->Instance == TIM9 || timer->Instance == TIM8 || timer->Instance == TIM1)
	{
		timer_clock = HAL_RCC_GetPCLK2Freq();
		if ((RCC->CFGR & RCC_CFGR_PPRE2) != 0)
		{
			timer_clock *= 2; /* APB2 prescaler > 1, timer clock x2 */
		}
	}
	else
	{
		timer_clock = HAL_RCC_GetPCLK1Freq(); //APB1 timers
		if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0)
		{
			timer_clock *= 2; /* APB1 prescaler > 1, timer clock x2 */
		}
	}
	return timer_clock;
}

/*=============CALCULATE BEST PSC AND ARR=============*/
static PWM_Status_t PWM_CalculateTimerValues(uint16_t* psc, uint32_t* arr, uint32_t timer_clock, uint32_t frequency, TIM_HandleTypeDef* timer_handler)
{
	bool is_32bit = false;
	if (PWM_ValidateTimer(timer_handler) != PWM_STATUS_OK)
		return PWM_STATUS_ERROR_NULL_POINTER;
	if (psc == NULL || arr == NULL)
		return PWM_STATUS_ERROR_NULL_POINTER;

	uint64_t total_divide = (uint64_t) timer_clock / frequency;
	if (timer_handler->Instance == TIM2 || timer_handler->Instance == TIM5)
	{
		is_32bit = true; //32bit timer => ARR = 32bit
		if (total_divide > (uint64_t) UINT16_MAX * UINT32_MAX)
			return PWM_STATUS_ERROR_CALCULATION_OVERFLOW;
	}
	else
	{
		is_32bit = false; //16bit timer => ARR = 16bit
		if (total_divide > (uint64_t) UINT16_MAX * UINT16_MAX)
			return PWM_STATUS_ERROR_CALCULATION_OVERFLOW;
	}

	uint32_t best_psc = 1;
	uint32_t best_arr = total_divide;
	for (uint32_t prescaler = 1; prescaler <= UINT16_MAX && prescaler <= total_divide; prescaler++)
	{
		if (total_divide % prescaler == 0)
		{
			uint32_t period = total_divide / prescaler;
			if (is_32bit)
			{
				if (period > best_arr || period <= UINT32_MAX) //check overflow 32bit register
				{
					best_arr = period;
					best_psc = prescaler;
					break;
				}
			}
			else
			{
				if (period > best_arr || period <= UINT16_MAX) //check overflow 16bit register
				{
					best_arr = period;
					best_psc = prescaler;
					break;
				}
			}
		}
	}

	*psc = best_psc;
	*arr = best_arr;

	return PWM_STATUS_OK;

}

/*=============PWM SET FREQUENCY=============*/
PWM_Status_t PWM_SetFrequency (PWM_Handle_t* pwm_handler, uint32_t frequency)
{
	PWM_Status_t status = PWM_ValidateHandle(pwm_handler);
	if (status != PWM_STATUS_OK)
		return status;

	pwm_handler->frequency_hz = frequency;

	return PWM_Configure(pwm_handler);
}

/*=============PWM SET DUTY CYCLE PERCENT=============*/
PWM_Status_t PWM_SetDutyCyclePercent(PWM_Handle_t* pwm_handler, float duty_cycle_percent)
{
	PWM_Status_t status = PWM_ValidateHandle(pwm_handler);
	if (status != PWM_STATUS_OK)
		return status;

	uint32_t arr = pwm_handler->timer_handler->Instance->ARR + 1;
	uint32_t ccr = (uint32_t) roundf((arr * duty_cycle_percent) / 100.0f);
	if (ccr > arr)
		ccr = arr;

	__HAL_TIM_SET_COMPARE(pwm_handler->timer_handler, pwm_handler->channel, ccr);
	pwm_handler->duty_cycle_percent = duty_cycle_percent;
	return PWM_STATUS_OK;
}

/*=============PWM CHECK RUNNING=============*/
bool PWM_IsRunning (PWM_Handle_t* pwm_handler)
{
	PWM_Status_t status = PWM_ValidateHandle(pwm_handler);
	if (status != PWM_STATUS_OK)
		return false;

	return pwm_handler->is_running;
}

/*=============PWM GET FREQUENCY=============*/
PWM_Status_t PWM_GetFrequency (PWM_Handle_t* pwm_handler, uint32_t* frequency_hz)
{
	PWM_Status_t status = PWM_ValidateHandle(pwm_handler);
	if (status != PWM_STATUS_OK)
		return status;

	if (frequency_hz == NULL)
		return PWM_STATUS_ERROR_NULL_POINTER;

	*frequency_hz  = pwm_handler->frequency_hz;

	return PWM_STATUS_OK;
}

/*=============PWM GET DUTY CYCLE PERCENT=============*/
PWM_Status_t PWM_GetDutyCyclePercent (PWM_Handle_t* pwm_handler, float* duty_cycle_percent)
{
	PWM_Status_t status = PWM_ValidateHandle(pwm_handler);
	if (status != PWM_STATUS_OK)
		return status;

	if (duty_cycle_percent == NULL)
		return PWM_STATUS_ERROR_NULL_POINTER;

	*duty_cycle_percent  = pwm_handler->duty_cycle_percent;

	return PWM_STATUS_OK;
}

/*=============PWM START WITH INTERRUPT=============*/
PWM_Status_t PWM_Start_It(PWM_Handle_t* pwm_handler)
{
	PWM_Status_t status = PWM_ValidateHandle(pwm_handler);
	if (status != PWM_STATUS_OK)
		return status;

	if(pwm_handler->is_running)
		return PWM_STATUS_ERROR_ALSO_RUNNING; //also running

	if (HAL_TIM_PWM_Start_IT(pwm_handler->timer_handler, pwm_handler->channel) != HAL_OK)
		return PWM_STATUS_ERROR_HAL_START_IT_ERROR;
	__HAL_TIM_ENABLE_IT(pwm_handler->timer_handler, TIM_IT_UPDATE);

	pwm_handler->is_running = true;
	return PWM_STATUS_OK;
}

/*=============PWM STOP WITH INTERRUPT=============*/
PWM_Status_t PWM_Stop_It(PWM_Handle_t* pwm_handler)
{
	PWM_Status_t status = PWM_ValidateHandle(pwm_handler);
	if (status != PWM_STATUS_OK)
		return status;

	if(!pwm_handler->is_running)
		return PWM_STATUS_ERROR_ALSO_STOPPED; //also stopped

	if (HAL_TIM_PWM_Stop_IT(pwm_handler->timer_handler, pwm_handler->channel) != HAL_OK)
		return PWM_STATUS_ERROR_HAL_STOP_IT_ERROR;

	pwm_handler->is_running = true;
	return PWM_STATUS_OK;
}

/*=============CALCULATE CCR FROM DUTY CYCLE PERCENT=============*/
static uint32_t PWM_CalculateCCR (PWM_Handle_t* pwm_handler)
{
	PWM_Status_t status = PWM_ValidateHandle(pwm_handler);
		if (status != PWM_STATUS_OK)
			return status;

		uint32_t arr = pwm_handler->timer_handler->Instance->ARR + 1;
		uint32_t ccr = (uint32_t) roundf((arr * pwm_handler->duty_cycle_percent) / 100.0f);
		if (ccr > arr)
			ccr = arr;

		return ccr;
}

/*=============PWM START DMA=============*/
PWM_Status_t PWM_Start_DMA (PWM_Handle_t* pwm_handler)
{
	PWM_Status_t status = PWM_ValidateHandle(pwm_handler);
	if (status != PWM_STATUS_OK)
		return status;

	if(pwm_handler->is_running)
		return PWM_STATUS_ERROR_ALSO_RUNNING; //also running

	uint32_t ccr = PWM_CalculateCCR(pwm_handler)-1;
	HAL_StatusTypeDef HAL_status = HAL_TIM_PWM_Start_DMA(pwm_handler->timer_handler, pwm_handler->channel, &ccr, sizeof(ccr));
	if (HAL_status != HAL_OK)
		return PWM_STATUS_ERROR_HAL_START_DMA_ERROR;

	pwm_handler->is_running = true;
	return PWM_STATUS_OK;
}
