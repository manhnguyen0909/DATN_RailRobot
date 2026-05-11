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
#include "cmsis_os.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "MPU6050.h"
#include "VL53L1X_api.h"
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
uint16_t adc_value = 0;
float v_pin = 0.0f;
float battery_voltage = 0.0f;
volatile uint8_t mpu_data_ready_flag = 0;

/* PID Controllers instantiation */
PID_Controller pid_L = {0.5f, 7.2f, 0.08f, 3.0f, 0.0f, 0.0f, 0.0f, 1000.0f, -1000.0f};
PID_Controller pid_R = {0.5f, 7.5f, 0.08f, 3.0f, 0.0f, 0.0f, 0.0f, 1000.0f, -1000.0f};

/* Exponential Moving Average (EMA) filters for Encoders */
float filtered_rpm_L = 0.0f;
float filtered_rpm_R = 0.0f;
const float encoder_filter_alpha = 0.15f;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */
void I2C_Scan(void);
void Motor_Drive(int speedL, int speedR);
float Compute_PID(PID_Controller *pid, float current_rpm, float dt_s);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
 * @brief Scans the I2C bus and prints connected device addresses.
 */
void I2C_Scan(void)
{
  printf("Scanning I2C bus...\n");
  uint8_t count = 0;
  for (uint8_t i = 1; i < 128; i++)
  {
    if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(i << 1), 3, 5) == HAL_OK)
    {
      printf("Found device at 0x%02X\n", i);
      count++;
    }
  }
  printf("Total I2C devices found: %d\n", count);
}

/**
 * @brief Drives dual DC motors using PWM signals.
 */
void Motor_Drive(int speedL, int speedR)
{
  /* Control Left Motor - TIM4 CH3 */
  if (speedL > 0)
  {
    HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(AIN2_GPIO_Port, AIN2_Pin, GPIO_PIN_RESET);
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, speedL);
  }
  else if (speedL < 0)
  {
    HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AIN2_GPIO_Port, AIN2_Pin, GPIO_PIN_SET);
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, -speedL);
  }
  else
  {
    HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AIN2_GPIO_Port, AIN2_Pin, GPIO_PIN_RESET);
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, 0);
  }

  /* Control Right Motor - TIM4 CH4 */
  if (speedR > 0)
  {
    HAL_GPIO_WritePin(BIN1_GPIO_Port, BIN1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(BIN2_GPIO_Port, BIN2_Pin, GPIO_PIN_RESET);
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, speedR);
  }
  else if (speedR < 0)
  {
    HAL_GPIO_WritePin(BIN1_GPIO_Port, BIN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BIN2_GPIO_Port, BIN2_Pin, GPIO_PIN_SET);
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, -speedR);
  }
  else
  {
    HAL_GPIO_WritePin(BIN1_GPIO_Port, BIN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BIN2_GPIO_Port, BIN2_Pin, GPIO_PIN_RESET);
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, 0);
  }
}

/**
 * @brief Computes PID controller output with anti-windup mechanism.
 */
float Compute_PID(PID_Controller *pid, float current_rpm, float dt_s)
{
  float error = pid->setpoint - current_rpm;
  
  pid->integral += error * dt_s;
  if (pid->integral > 8000.0f) pid->integral = 8000.0f;
  else if (pid->integral < -8000.0f) pid->integral = -8000.0f;

  float derivative = 0.0f;
  if (dt_s > 0.0f) derivative = (error - pid->last_error) / dt_s;
  pid->last_error = error;

  float output = (pid->Kp * error) + (pid->Ki * pid->integral) + (pid->Kd * derivative);
  output += pid->Kff * pid->setpoint;

  if (output > pid->out_max) output = pid->out_max;
  else if (output < pid->out_min) output = pid->out_min;

  return output;
}
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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART6_UART_Init();
  /* USER CODE BEGIN 2 */
/* --- I2C Devices Initialization --- */
  HAL_GPIO_WritePin(SHUT_L_GPIO_Port, SHUT_L_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(SHUT_R_GPIO_Port, SHUT_R_Pin, GPIO_PIN_RESET);
  HAL_Delay(20);
  
  I2C_Scan();
  HAL_Delay(1000);
  MPU6050_Initialization();

  /* Initialize VL53L1X Left Sensor */
  uint8_t state = 0;
  HAL_GPIO_WritePin(SHUT_L_GPIO_Port, SHUT_L_Pin, GPIO_PIN_SET);
  HAL_Delay(20);
  while (state == 0)
  {
    VL53L1X_BootState(0x52, &state);
    HAL_Delay(2);
  }
  VL53L1X_SetI2CAddress(0x52, DEV_LEFT);
  VL53L1X_SensorInit(DEV_LEFT);
  VL53L1X_SetDistanceMode(DEV_LEFT, 2);
  VL53L1X_SetTimingBudgetInMs(DEV_LEFT, 50);
  VL53L1X_SetInterMeasurementInMs(DEV_LEFT, 55);
  VL53L1X_StartRanging(DEV_LEFT);

  /* Initialize VL53L1X Right Sensor */
  HAL_GPIO_WritePin(SHUT_R_GPIO_Port, SHUT_R_Pin, GPIO_PIN_SET);
  HAL_Delay(20);
  state = 0;
  while (state == 0)
  {
    VL53L1X_BootState(DEV_RIGHT, &state);
    HAL_Delay(2);
  }
  VL53L1X_SensorInit(DEV_RIGHT);
  VL53L1X_SetDistanceMode(DEV_RIGHT, 2);
  VL53L1X_SetTimingBudgetInMs(DEV_RIGHT, 50);
  VL53L1X_SetInterMeasurementInMs(DEV_RIGHT, 55);
  VL53L1X_StartRanging(DEV_RIGHT);

  /* --- Actuators & Feedback Initialization --- */
  HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
  HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);

  /* Communication variables reset */
  lora_flag = 0; lora_index = 0;
  memset(lora_buffer, 0, sizeof(lora_buffer));
  pi_flag = 0; pi_index = 0;
  memset(pi_buffer, 0, sizeof(pi_buffer));

  HAL_UART_Receive_IT(&huart1, &rx_byte_pi, 1);
  HAL_UART_Receive_IT(&huart2, &rx_byte_lora, 1);
  HAL_UART_Receive_DMA(&huart6, (uint8_t *)gps_buffer, 512);
  /* USER CODE END 2 */

  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

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

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM9 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM9)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
