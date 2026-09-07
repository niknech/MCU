/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

uint8_t ParseRxBuffer();
uint8_t GetBytes(uint32_t addr, uint8_t length, char* bytesBuf);
void ConvertToIntelHex(uint32_t addr, uint8_t readLength);
uint8_t calcCrc(char* str, uint8_t length);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
char rx_buffer [128];
volatile uint8_t rx_complete = 0;

char cmd[10];
uint32_t addr = 0;
uint8_t length= 0;

static const char* hex = "0123456789ABCDEF";

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
    if(rx_complete)
    {
      if (!ParseRxBuffer())
      {
	ConvertToIntelHex(addr, length);
      }

      rx_complete = 0;
      memset(rx_buffer, '\0', sizeof(rx_buffer));

    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLRCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
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

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
uint8_t ParseRxBuffer()
{
	uint8_t parsed = sscanf(rx_buffer, "%7s %x %hhu", cmd, &addr, &length);

	if (parsed != 3)
		return -1;

	if (strcmp("get_hex", cmd))
		return -1;

	return 0;
}

uint8_t GetBytes(uint32_t addr, uint8_t length, char* bytesBuf)
{

    if (length > 16)
    	return -1;

    uint8_t* ptrAddr = (uint8_t*)addr;

    // Разбиваем Hex число на два символа ASCII
    for (int i=0; i<length; i++)
    {
        bytesBuf[i*2] = hex [ptrAddr[i] >> 4];
        bytesBuf[i*2+1] = hex [ptrAddr[i] & 0x0F];
    }

    return 1;
}

void ConvertToIntelHex(uint32_t addr, uint8_t length)
{
    char IntelStr[45] = {'\0'};
    char bytesBuf[33] = {'\0'};
    uint16_t lowAddr = addr & 0x0000FFFF;

    uint8_t usb_ts_status = 0;


    // Печатаем заголовок
    snprintf(IntelStr, sizeof(IntelStr), ":%8s%04x", "02000004", addr>>16);
    snprintf(IntelStr, sizeof(IntelStr), "%12s%02x\n", IntelStr, calcCrc(IntelStr, strlen(IntelStr)));
    CDC_Transmit_FS((uint8_t*)IntelStr, strlen(IntelStr));
    memset(IntelStr, '\0', sizeof(IntelStr));

    while (length)
    {
    	if (length >= 16)
    	{
    	    // Получаем число по адресу и переводим в ASCII
    	    GetBytes(addr, 16, bytesBuf);
    	    // Собираем Intel Hex без CRC
    	    snprintf(IntelStr, sizeof(IntelStr), ":%02x%04x00%s", 16, lowAddr, bytesBuf);
    	    // Добавялем CRC
    	    snprintf(IntelStr, sizeof(IntelStr), "%s%02x\n", IntelStr, calcCrc(IntelStr, strlen(IntelStr)));
    	    // Двигаемся по адресу дальше
    	    addr = addr + 16;
    	    lowAddr = addr & 0x0000FFFF;
    	    length = length - 16;
    	}
    	else
    	{
    	    // Получаем число по адресу и переводим в ASCII
    	    GetBytes(addr, length, bytesBuf);
    	    // Собираем Intel Hex без CRC
    	    snprintf(IntelStr, sizeof(IntelStr), ":%02x%04x00%s", length, lowAddr, bytesBuf);
    	    // Добавялем CRC
    	    snprintf(IntelStr, sizeof(IntelStr), "%s%02x\n", IntelStr, calcCrc(IntelStr, strlen(IntelStr)));
    	    // Двигаемся по адресу дальше
    	    addr = addr + length;
    	    lowAddr = addr & 0x0000FFFF;
    	    length = 0;
    	}

    	    do
    	    {
    		usb_ts_status = CDC_Transmit_FS((uint8_t*)IntelStr, strlen(IntelStr));
    	    } while (usb_ts_status == USBD_BUSY);

	    memset(bytesBuf, '\0', sizeof(bytesBuf));
	    memset(IntelStr, '\0', sizeof(IntelStr));
    }
    do
    {
	usb_ts_status = CDC_Transmit_FS((uint8_t*)":00000001FF", 11);
    } while (usb_ts_status == USBD_BUSY);
}

uint8_t calcCrc(char* str, uint8_t length)
{
    char tmp[3];
    tmp [2] = '\0';
    uint32_t sum = 0;
    uint8_t hex = 0;

    for (int i = 1; i < length; i+=2)
    {
        tmp[0] = str[i];
        tmp[1] = str[i+1];

        sscanf(tmp, "%2x", &hex);
        sum += hex;
    }

    return (uint8_t)(0-sum);
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
