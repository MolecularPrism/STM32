/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "stm32f0xx_hal.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>

#define DELIMITER 0
#define SPACE 1
#define PERIOD 2
#define JUNK 3

UART_HandleTypeDef huart2;

void UART_Init(void){
	huart2.Instance = USART2;
	huart2.Init.BaudRate = 9600;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	HAL_UART_Init(&huart2);
}

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int binaryToDecimal(char* binary)
{
	int num;
	sscanf(binary, "%d", &num);
    int dec_value = 0;

    // Initializing base value to 1, i.e 2^0
    int base = 1;

    int temp = num;
    while (temp) {
        int last_digit = temp % 10;
        temp = temp / 10;

        dec_value += last_digit * base;

        base = base * 2;
    }

    return dec_value;
}

//if result is between 48 - 57 then value's decimal
int is_Decimal(char* buf){
	int value;
	int is_dec;

	value = binaryToDecimal(buf); //decimal value of binary

	if(value >= 48 && value <= 57){
		is_dec = 1;
	} else{
		is_dec = 0;
	}

	return is_dec;
}

// if the value is not decimal, we need to check whether
//it's a . or " " or \0 or \n
int check_nondec_symbol(char* buf){
	int symbol_identifier;
	//first check if it's a delimiter byte
	if(strcmp(buf,"00000000") == 0 || strcmp(buf,"00001010") == 0){ //if NULL or New line
		symbol_identifier = DELIMITER; //0 means delimiter
	}
	else if(strcmp(buf,"00100000") == 0){ //if space
		symbol_identifier = SPACE; //1 means space
	}
	else if(strcmp(buf,"00101110") == 0){ //if .
		symbol_identifier = PERIOD; //2 means .
	}
	else{ //if anything else
		symbol_identifier = JUNK; //3 means JUNK (later parse them out)
	}

	return symbol_identifier;

}

char ascii_to_decimal(char* buf){
	char decimal;

	if(strcmp(buf,"00110000") == 0){
		decimal = '0';
	}
	else if(strcmp(buf,"00110001") == 0){
		decimal = '1';
	}
	else if(strcmp(buf, "00110010") == 0){
		decimal = '2';
	}
	else if(strcmp(buf, "00110011") == 0){
		decimal = '3';
	}
	else if(strcmp(buf, "00110100") == 0){
		decimal = '4';
	}
	else if(strcmp(buf, "00110101") == 0){
		decimal = '5';
	}
	else if(strcmp(buf, "00110110") == 0){
		decimal = '6';
	}
	else if(strcmp(buf, "00110111") == 0){
		decimal = '7';
	}
	else if(strcmp(buf, "00111000") == 0){
		decimal = '8';
	}
	else if(strcmp(buf, "00111001") == 0){
		decimal = '9';
	}

	return decimal;
}



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
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
    /* USER CODE END WHILE */
	char data_value[16];
	char buf[8];
	char decimal_val;
	int is_decimal;
	int check_symbol;
	int pos = 0;
	HAL_UART_Receive(&huart2, (uint8_t*)buf, 8, 100);

	//check if decimal or not
	is_decimal = is_Decimal(buf);
	//check what non-decimal symbol it is
	check_symbol = check_nondec_symbol(buf);

	//if it's not decimal, then check what kind of character it is
	if(is_decimal == 0){
		//if delimiter byte, then we clear data_value
		if(check_symbol == DELIMITER){
			data_value[0] = '\0';

		//if it's a period then we concatenate to data_value
		if(check_symbol == PERIOD){
			strcat(data_value, '.');
		}

		//if it's a white space or some junk value, we just do nothing
	}else{
		//if it's decimal then convert ascii to decimal val
		decimal_val = ascii_to_decimal(buf);
		strcat(data_value, decimal_val); //concatenate decimal digit to data arr

	}

	//clear buf just in case
	buf[0] = '\0';
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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

#ifdef  USE_FULL_ASSERT
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
