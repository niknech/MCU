#include "app.h"

#define RX_BUFFER_SIZE 256

uint32_t last_timestamp = 0;
volatile uint32_t adcDataReady = 0;
extern char rxBuffer[RX_BUFFER_SIZE];
extern int rxIndex;

volatile uint16_t display_print_count = 0;

char display_text_buf[64];
char display_val_buf[32];

RC_Struct_t rc = {0,};
PWM_Handle_t pwm;
PID_Settings_t pid_settings;
PID_Control_t pid_control;

static GMT020 display = {
    .spi = &hspi1,
    .cs_port = GPIOA, .cs_pin = GPIO_PIN_6,
    .dc_port = GPIOB, .dc_pin = GPIO_PIN_6,
    .rst_port = GPIOC, .rst_pin = GPIO_PIN_7,
    .rotation = 3, .bgr = 0, .inversion = 1
};

void main_app(void)
{
  GMT_Init(&display);
  PID_Init(&pid_settings, &pid_control, &(rc.voltage));
  HAL_ADCEx_InjectedStart_IT(&hadc1);
  PWM_Init(&pwm, &htim1, TIM_CHANNEL_1);
  PWM_Start(&pwm);

  HAL_UART_Receive_IT(&huart2, (uint8_t*) &rxBuffer[rxIndex], 1);

  while (1)
  {
    if (display_print_count >= 500)
    {
      GMT_PrintPIDValues(&display, rc.voltage, rc.current, pid_control.output, pid_settings.setpoint, pid_control.error);
      display_print_count = 0;
    }
    if (adcDataReady)
    {
      RC_Update(&rc);
      PID_Update(&pid_control, pid_settings);
      PWM_SetDutyCyclePercent(&pwm, pid_control.output);

      adcDataReady = 0;
    }
  }

}

void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
      display_print_count++;

      if (!adcDataReady)
      {
        rc.voltage_raw =
            HAL_ADCEx_InjectedGetValue(hadc, ADC_INJECTED_RANK_1);

        rc.current_raw =
            HAL_ADCEx_InjectedGetValue(hadc, ADC_INJECTED_RANK_2);
        pid_control.dT = (float)(HAL_GetTick() - last_timestamp) / 1000.0f;
        last_timestamp = HAL_GetTick();
        adcDataReady = 1;
      }
    }
}


