/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */
#include "string.h"
#include <stdlib.h>

#define RX_BUFFER_SIZE 256

char rxBuffer[RX_BUFFER_SIZE] = {0};
int rxIndex = 0;
extern PID_Settings_t pid_settings;
extern PID_Control_t pid_control;

void my_ftoa(float value, char *buf, uint8_t precision);
/* USER CODE END 0 */

UART_HandleTypeDef huart2;

/* USART2 init function */

void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspInit 0 */

  /* USER CODE END USART2_MspInit 0 */
    /* USART2 clock enable */
    __HAL_RCC_USART2_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART2 interrupt Init */
    HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspInit 1 */

  /* USER CODE END USART2_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspDeInit 0 */

  /* USER CODE END USART2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART2_CLK_DISABLE();

    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);

    /* USART2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspDeInit 1 */

  /* USER CODE END USART2_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
void UART_SendString(char* str, UART_HandleTypeDef* huart)
{
  HAL_UART_Transmit(huart, (uint8_t*)str, strlen(str), 1000);
}

void UART_SendFloat(float number, UART_HandleTypeDef* huart)
{
    char numberStr[32];
    my_ftoa(number, numberStr, 5);

    UART_SendString(numberStr, huart);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    rxIndex++;
    if((rxBuffer[rxIndex-2]==0x0D)&&(rxBuffer[rxIndex-1]==0x0A)){
      if (!strncmp(rxBuffer, "kp", 2))
      {
        pid_settings.kp = strtof(rxBuffer + 2, NULL);
        UART_SendString("###################\r\n", huart);
        UART_SendString("Kp is set: ", huart);
        UART_SendFloat(pid_settings.kp, huart);
        UART_SendString("\r\n###################\r\n", huart);
      }
      if (!strncmp(rxBuffer, "ki", 2))
      {
        pid_settings.ki = strtof(rxBuffer + 2, NULL);
        UART_SendString("###################\r\n", huart);
        UART_SendString("Ki is set: ", huart);
        UART_SendFloat(pid_settings.ki, huart);
        UART_SendString("\r\n###################\r\n", huart);
      }
      if (!strncmp(rxBuffer, "kd", 2))
      {
        pid_settings.kd = strtof(rxBuffer + 2, NULL);
        UART_SendString("###################\r\n", huart);
        UART_SendString("Kd is set: ", huart);
        UART_SendFloat(pid_settings.kd, huart);
        UART_SendString("\r\n###################\r\n", huart);
      }
      if (!strncmp(rxBuffer, "s", 1))
      {
        pid_settings.setpoint = strtof(rxBuffer + 1, NULL);
        UART_SendString("###################\r\n", huart);
        UART_SendString("Setpoint is set: ", huart);
        UART_SendFloat(pid_settings.setpoint, huart);
        UART_SendString("\r\n###################\r\n", huart);
      }
          memset(rxBuffer,0,sizeof(rxBuffer));
          rxIndex=0;
      }
        HAL_UART_Receive_IT(huart, (uint8_t*) &rxBuffer[rxIndex], 1);
  }
}

void reverse_str(char *str, uint16_t len)
{
    uint16_t i = 0, j = len - 1;
    while (i < j) {
        char tmp = str[i];
        str[i++] = str[j];
        str[j--] = tmp;
    }
}

void my_ftoa(float value, char *buf, uint8_t precision)
{
    if (buf == NULL) return;
    if (precision > 10) precision = 10; // Ограничиваем точность

    char *p = buf;

    // Обработка знака
    if (value < 0) {
        *p++ = '-';
        value = -value;
    }

    // Целая часть
    uint32_t int_part = (uint32_t)value;
    float frac_part = value - (float)int_part;

    // Конвертация целой части
    char int_buf[12];
    uint16_t int_len = 0;
    if (int_part == 0) {
        int_buf[int_len++] = '0';
    } else {
        while (int_part > 0) {
            int_buf[int_len++] = '0' + (int_part % 10);
            int_part /= 10;
        }
    }
    reverse_str(int_buf, int_len);
    memcpy(p, int_buf, int_len);
    p += int_len;

    // Десятичная точка и дробная часть
    if (precision > 0) {
        *p++ = '.';
        for (uint8_t i = 0; i < precision; i++) {
            frac_part *= 10.0;
            uint8_t digit = (uint8_t)frac_part;
            *p++ = '0' + digit;
            frac_part -= (float)digit;
        }
    }

    *p = '\0';
}

/* USER CODE END 1 */

