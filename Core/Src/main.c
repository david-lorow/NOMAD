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
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/*--------------------------------------------------------------------------------------*/
typedef struct Data {
	float d_target;
	float d_current;
} Data;
/*--------------------------------------------------------------------------------------*/

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/*--------------------------------------------------------------------------------------*/
//Right
#define r_max 1999
//Center
#define center 1555
//Left
#define l_max 1111

//Forward - Medium speed
#define f_med 1545
//Fast speed
#define f_max 1605
//Stop
#define stop 1515
//Reverse < 1515
#define rev 1455
/*--------------------------------------------------------------------------------------*/

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */



/*--------------------------------------------------------------------------------------*/
static uint8_t tx[4];
static uint8_t rx[4];
static volatile uint8_t spi_done = 0;

static Data s_data = {(float)stop, (float)stop};
/*--------------------------------------------------------------------------------------*/


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
/*--------------------------------------------------------------------------------------*/


void SystemClock_Config(void);
void P_DIddy(volatile Data *session_data);
void Servo_SetPWM(float pulse_us);
void ESC_SetPWM(float pulse_us);


/*--------------------------------------------------------------------------------------*/
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */


  /*--------------------------------------------------------------------------------------*/

  //Start timer and clear PWM cycle flag, set up SPI to active via interrupts, expect 4 bytes
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
  float initial_reply = s_data.d_current;
  memcpy(tx, &initial_reply, sizeof initial_reply);
  HAL_SPI_TransmitReceive_IT(&hspi1, tx, rx, 4);

  /*--------------------------------------------------------------------------------------*/


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
  /*--------------------------------------------------------------------------------------*/
	  if(spi_done) {
		  spi_done = 0;
		  float new_target;
		  memcpy(&new_target, rx, sizeof new_target);

		  if (new_target == rev || new_target == stop || new_target == f_med || new_target == f_max) s_data.d_target = new_target;
		  float reply = s_data.d_current;
		  memcpy(tx, &reply, sizeof reply);

		  HAL_SPI_TransmitReceive_IT(&hspi1, tx, rx, 4);
	  }

	  if (__HAL_TIM_GET_FLAG(&htim3, TIM_FLAG_UPDATE)) {
	  	  __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
	  	  P_DIddy(&s_data);  // Exactly one call per observed period
	  }

  /*--------------------------------------------------------------------------------------*/

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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

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
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */


  /*--------------------------------------------------------------------------------------*/
  //Invoked by the HAL after a 4 byte interrupt completes, needs to be global
  void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
  {
      if (hspi->Instance == SPI1) {
          spi_done = 1;  // All four bytes have been exchanged.
      }
  }

  //Pure motor example
  void P_DIddy(volatile Data *session_data) {
	  float error = session_data->d_target - session_data->d_current;
	  if (error == 0) return;
	  if(error > 0) {
		  session_data->d_current+=30;
		  ESC_SetPWM(session_data->d_current);
	  }
	  else {
		  session_data->d_current-=30;
		  ESC_SetPWM(session_data->d_current);
	  }
  }

  void Servo_SetPWM(float pulse_us)
  {
      // Limit to measured servo range
      if (pulse_us < 1111)
          pulse_us = 1111;

      if (pulse_us > 1999)
          pulse_us = 1999;

      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, pulse_us);
  }

  void ESC_SetPWM(float pulse_us)
  {
      // Clamp to measured ESC range
      if (pulse_us < 999)
          pulse_us = 999;

      if (pulse_us > 1999)
          pulse_us = 1999;

      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse_us);
  }
  /*--------------------------------------------------------------------------------------*/


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
