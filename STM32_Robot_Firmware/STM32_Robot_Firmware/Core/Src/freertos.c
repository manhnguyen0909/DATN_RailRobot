/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usart.h"
#include "MPU6050.h"
#include "VL53L1X_api.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "secret.h"

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
/* USER CODE BEGIN Variables */
/* Sensor data structure */
typedef struct
{
  uint16_t distance_left;
  uint16_t distance_right;
  float pitch;
  float roll;
  float battery_voltage;
  float encoder_rpm_left;
  float encoder_rpm_right;
} SensorData_t;

extern PID_Controller pid_L, pid_R;
extern float filtered_rpm_L, filtered_rpm_R;
extern const float encoder_filter_alpha;
extern Struct_MPU6050 MPU6050;
extern TIM_HandleTypeDef htim2, htim3;
extern uint16_t adc_value;
extern float battery_voltage;
extern ADC_HandleTypeDef hadc1;

float Compute_PID(PID_Controller *pid, float current_rpm, float dt_s);
void Motor_Drive(int speedL, int speedR);

/* SensorQueue configuration: queue depth = 1 (required for xQueueOverwrite) */
static QueueHandle_t SensorQueueHandle_native = NULL;

/* Debug variables for monitoring PID and motor output */
volatile int debug_target_L = 0, debug_target_R = 0;

/* Error counters for VL53L1X self-healing mechanism */
static int error_count_L = 0;
static int error_count_R = 0;

/* USER CODE END Variables */
osThreadId MotorTaskHandle;
osThreadId CommTaskHandle;
osThreadId SensorTaskHandle;
osMessageQId MotorQueueHandle;
osMutexId EncoderMutexHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartMotorTask(void const * argument);
void StartCommTask(void const * argument);
void StartSensorTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* definition and creation of EncoderMutex */
  osMutexDef(EncoderMutex);
  EncoderMutexHandle = osMutexCreate(osMutex(EncoderMutex));

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* definition and creation of MotorQueue */
  osMessageQDef(MotorQueue, 10, uint32_t);
  MotorQueueHandle = osMessageCreate(osMessageQ(MotorQueue), NULL);

  /* USER CODE BEGIN RTOS_QUEUES */
  SensorQueueHandle_native = xQueueCreate(1, sizeof(SensorData_t));
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of MotorTask */
  osThreadDef(MotorTask, StartMotorTask, osPriorityHigh, 0, 512);
  MotorTaskHandle = osThreadCreate(osThread(MotorTask), NULL);

  /* definition and creation of CommTask */
  osThreadDef(CommTask, StartCommTask, osPriorityNormal, 0, 512);
  CommTaskHandle = osThreadCreate(osThread(CommTask), NULL);

  /* definition and creation of SensorTask */
  osThreadDef(SensorTask, StartSensorTask, osPriorityLow, 0, 512);
  SensorTaskHandle = osThreadCreate(osThread(SensorTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartMotorTask */
/**
  * @brief  Function implementing the MotorTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartMotorTask */
void StartMotorTask(void const * argument)
{
  /* USER CODE BEGIN StartMotorTask */
  osEvent event;
  int32_t last_count_L = (int32_t)__HAL_TIM_GET_COUNTER(&htim2);
  int32_t last_count_R = (int32_t)__HAL_TIM_GET_COUNTER(&htim3);
  uint32_t last_time = osKernelSysTick();
  float dt_s;
  int pwm_L = 0, pwm_R = 0;
  /* Infinite loop */
  for(;;)
  {
    /* Receive setpoint from CommTask */
    event = osMessageGet(MotorQueueHandle, 0);
    if (event.status == osEventMessage)
    {
      uint32_t combined = event.value.v;
      pid_L.setpoint = (float)((int16_t)((combined >> 16) & 0xFFFF));
      pid_R.setpoint = (float)((int16_t)(combined & 0xFFFF));
      debug_target_L = (int)pid_L.setpoint;
      debug_target_R = (int)pid_R.setpoint;
    }
    if (pid_L.setpoint == 0.0f && pid_R.setpoint == 0.0f)
    {
      Motor_Drive(0, 0);

      /* Reset filter */
      filtered_rpm_L = 0.0f;
      filtered_rpm_R = 0.0f;

      /* Clear PID integral and derivative */
      pid_L.integral = 0.0f;
      pid_L.last_error = 0.0f;
      pid_R.integral = 0.0f;
      pid_R.last_error = 0.0f;

      /* Reset timing and counter */
      last_time = osKernelSysTick();
      last_count_L = (int32_t)__HAL_TIM_GET_COUNTER(&htim2);
      last_count_R = (int32_t)__HAL_TIM_GET_COUNTER(&htim3);

      osDelay(100);
      continue;
    }
    /* Calculate actual time interval */
    uint32_t current_time = osKernelSysTick();
    dt_s = (float)(current_time - last_time) / 1000.0f;
    last_time = current_time;
    if (dt_s < 0.001f)
      dt_s = 0.001f;

    /* Read encoder pulse delta */
    uint32_t raw_L = __HAL_TIM_GET_COUNTER(&htim2);
    uint32_t raw_R = __HAL_TIM_GET_COUNTER(&htim3);
    int32_t delta_L = (int32_t)(raw_L - (uint32_t)last_count_L);
    int32_t delta_R = (int32_t)(int16_t)(raw_R - (uint16_t)last_count_R);
    last_count_L = (int32_t)raw_L;
    last_count_R = (int32_t)raw_R;

    /* Calculate RPM from encoder pulses */
    float rpm_L_raw = ((float)delta_L / 937.2f) / dt_s * 60.0f;
    float rpm_R_raw = ((float)delta_R / 937.2f) / dt_s * 60.0f;

    if (osMutexWait(EncoderMutexHandle, 10) == osOK)
    {
      /* Apply exponential moving average filter */
      if (filtered_rpm_L == 0.0f && rpm_L_raw != 0.0f)
        filtered_rpm_L = rpm_L_raw;
      else
        filtered_rpm_L += encoder_filter_alpha * (rpm_L_raw - filtered_rpm_L);

      if (filtered_rpm_R == 0.0f && rpm_R_raw != 0.0f)
        filtered_rpm_R = rpm_R_raw;
      else
        filtered_rpm_R += encoder_filter_alpha * (rpm_R_raw - filtered_rpm_R);

      /* Compute PID output */
      pwm_L = (int)Compute_PID(&pid_L, filtered_rpm_L, dt_s);
      pwm_R = (int)Compute_PID(&pid_R, filtered_rpm_R, dt_s);
      osMutexRelease(EncoderMutexHandle);
    }
    Motor_Drive(pwm_L, pwm_R);
    osDelay(100);
  }
  /* USER CODE END StartMotorTask */
}

/* USER CODE BEGIN Header_StartCommTask */
/**
* @brief Function implementing the CommTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCommTask */
void StartCommTask(void const * argument)
{
  /* USER CODE BEGIN StartCommTask */
   SensorData_t sensorData;
  memset(&sensorData, 0, sizeof(sensorData));

  int speedL = 0, speedR = 0;
  char my_token[] = ROBOT_TOKEN;
  char pi_data[50] = "NORMAL";
  float real_lat = 0.0f;
  float real_lon = 0.0f;
  int real_sats = 0;
  uint32_t last_tx_time = osKernelSysTick();
  uint32_t last_gps_time = osKernelSysTick();
  uint32_t last_pi_time = osKernelSysTick();
  /* Infinite loop */
  for(;;)
  {
    /* Process LoRa commands */
    if (lora_flag == 1)
    {
      char temp_buffer[100];
      strncpy(temp_buffer, lora_buffer, sizeof(temp_buffer));

      memset(lora_buffer, 0, sizeof(lora_buffer));
      lora_index = 0;
      lora_flag = 0;

      char *cmd_pointer = strstr(temp_buffer, ROBOT_TOKEN "$SET:");
      if (cmd_pointer != NULL)
      {
        if (sscanf(cmd_pointer, "%s $SET:%d,%d", my_token, &speedL, &speedR) == 3)
        {
          uint32_t combined_speed = ((uint32_t)((int16_t)speedL) << 16) |
                                    ((uint32_t)((int16_t)speedR) & 0xFFFF);
          osMessagePut(MotorQueueHandle, combined_speed, 0);
        }
      }
    }

    /* Process Raspberry Pi commands */
    if (pi_flag == 1)
    {
      strncpy(pi_data, pi_buffer, sizeof(pi_data) - 1);
      pi_data[sizeof(pi_data) - 1] = '\0';
      pi_data[strcspn(pi_data, "\r\n")] = '\0';

      memset(pi_buffer, 0, sizeof(pi_buffer));
      pi_index = 0;
      pi_flag = 0;
      last_pi_time = osKernelSysTick();
    }

    /* Process GPS data - extract coordinates from NMEA $GPGGA sentence */
    if (osKernelSysTick() - last_gps_time >= 2000)
    {
      last_gps_time = osKernelSysTick();

      char temp_gps[513];
      memcpy(temp_gps, gps_buffer, 512);
      temp_gps[512] = '\0';

      char *gpgga_ptr = strstr(temp_gps, "$GPGGA");
      if (gpgga_ptr != NULL)
      {
        float raw_lat = 0, raw_lon = 0;
        char lat_dir, lon_dir;
        int fix_quality = 0, satellites = 0;

        if (sscanf(gpgga_ptr, "$GPGGA,%*[^,],%f,%c,%f,%c,%d,%d",
                   &raw_lat, &lat_dir, &raw_lon, &lon_dir, &fix_quality, &satellites) >= 6)
        {
          real_sats = satellites;

          if (fix_quality > 0)
          {
            /* Convert NMEA format to decimal degrees */
            int lat_deg = (int)(raw_lat / 100);
            float lat_min = raw_lat - (lat_deg * 100);
            real_lat = lat_deg + (lat_min / 60.0f);
            if (lat_dir == 'S')
              real_lat = -real_lat;

            int lon_deg = (int)(raw_lon / 100);
            float lon_min = raw_lon - (lon_deg * 100);
            real_lon = lon_deg + (lon_min / 60.0f);
            if (lon_dir == 'W')
              real_lon = -real_lon;
          }
        }
      }
    }

    /*Receive sensor data from the sensor queue*/
    SensorData_t rxSensor;
    if (xQueueReceive(SensorQueueHandle_native, &rxSensor, 0) == pdTRUE)
    {
      sensorData = rxSensor;
    }

    /* Send telemetry data via LoRa */
    if (osKernelSysTick() - last_tx_time >= 1000)
    {
      // if GPS data is stale for more than 10 seconds, set to "NONE"
      if (osKernelSysTick() - last_pi_time > 10000)
      {
        strncpy(pi_data, "NONE", sizeof(pi_data));
      }
      static char buffer[260];
      // - Convert lat/lon to integer format (microdegrees) for compact transmission
      int32_t lat_int = (int32_t)(real_lat * 1000000.0f);
      int32_t lon_int = (int32_t)(real_lon * 1000000.0f);
      snprintf(buffer, sizeof(buffer),
               "%s L%d R%d E%.1f,%.1f P%.2f Ro%.2f V%.2f PI:%s debug%d,%d GPS:%d,%d S:%d!",
               my_token,
               sensorData.distance_left,
               sensorData.distance_right,
               sensorData.encoder_rpm_left,
               sensorData.encoder_rpm_right,
               sensorData.roll,
               sensorData.pitch,
               sensorData.battery_voltage,
               pi_data,
               debug_target_L, debug_target_R, // Nhét chuỗi Pi vào đây
               lat_int, lon_int, real_sats);
      // Wait until previous transmission is complete before sending new data
      while (huart2.gState != HAL_UART_STATE_READY)
      {
        osDelay(1);
      }
      HAL_StatusTypeDef tx_status = HAL_UART_Transmit_IT(&huart2, (uint8_t *)buffer, strlen(buffer));
      if (tx_status != HAL_OK)
      {
        HAL_UART_AbortTransmit(&huart2);
      }
      last_tx_time = osKernelSysTick();
    }
    osDelay(10);
  }
  /* USER CODE END StartCommTask */
}

/* USER CODE BEGIN Header_StartSensorTask */
/**
* @brief Function implementing the SensorTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSensorTask */
void StartSensorTask(void const * argument)
{
  /* USER CODE BEGIN StartSensorTask */
  SensorData_t sensorData;
  memset(&sensorData, 0, sizeof(sensorData));
  uint16_t dist_L = 0, dist_R = 0;
  /* Infinite loop */
  for(;;)
  {
    /* Read battery voltage via ADC */
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
    {
      adc_value = HAL_ADC_GetValue(&hadc1);
      const float he_so_bu = 0.9586f;
      battery_voltage = ((float)adc_value / 4095.0f) * 3.3f * ((R1 + R2) / R2) * he_so_bu;
    }
    sensorData.battery_voltage = battery_voltage;

    /* Read encoder RPM */
    if (osMutexWait(EncoderMutexHandle, 20) == osOK)
    {
      sensorData.encoder_rpm_left = filtered_rpm_L;
      sensorData.encoder_rpm_right = filtered_rpm_R;
      osMutexRelease(EncoderMutexHandle);
    }

    /* Read IMU data and compute pitch/roll with low-pass filtering */
    MPU6050_ProcessData(&MPU6050);
    float acc_x = (float)MPU6050.acc_x_raw;
    float acc_y = (float)MPU6050.acc_y_raw;
    float acc_z = (float)MPU6050.acc_z_raw;

    //Calculate raw angles from accelerometer
    float raw_pitch = atan2f(acc_x, sqrtf(acc_y * acc_y + acc_z * acc_z)) * RAD_TO_DEG;
    float raw_roll = atan2f(acc_y, sqrtf(acc_x * acc_x + acc_z * acc_z)) * RAD_TO_DEG;

    // Low-Pass Filter
    static float filtered_pitch = 0.0f;
    static float filtered_roll = 0.0f;
    const float imu_alpha = 0.05f; // Chỉnh từ 0.05 (mượt nhưng trễ) đến 0.2 (nhạy nhưng hơi gai)

    if (filtered_pitch == 0.0f && filtered_roll == 0.0f)
    {
      filtered_pitch = raw_pitch;
      filtered_roll = raw_roll;
    }
    else
    {
      // Update filtered angles
      filtered_pitch += imu_alpha * (raw_pitch - filtered_pitch);
      filtered_roll += imu_alpha * (raw_roll - filtered_roll);
    }
    sensorData.pitch = filtered_pitch;
    sensorData.roll = filtered_roll;

    /*Read VL53L1X data with error handling */
    uint8_t dataReady;
    uint8_t rangeStatus_L, rangeStatus_R;
    uint16_t timeout;

    /* Read data from left sensor */
    dataReady = 0;
    timeout = 0;

    while (dataReady == 0 && timeout < 60)
    {
      VL53L1X_CheckForDataReady(DEV_LEFT, &dataReady);
      if (dataReady == 0)
      {
        osDelay(1);
        timeout++;
      }
    }
    if (dataReady == 1)
    {
      VL53L1X_GetRangeStatus(DEV_LEFT, &rangeStatus_L);
      VL53L1X_GetDistance(DEV_LEFT, &dist_L);
      VL53L1X_ClearInterrupt(DEV_LEFT);
    }
    else
    {
      rangeStatus_L = 255;
    }
    /* Read data from right sensor */
    dataReady = 0;
    timeout = 0;
    while (dataReady == 0 && timeout < 60)
    {
      VL53L1X_CheckForDataReady(DEV_RIGHT, &dataReady);
      if (dataReady == 0)
      {
        osDelay(1);
        timeout++;
      }
    }
    if (dataReady == 1)
    {
      VL53L1X_GetRangeStatus(DEV_RIGHT, &rangeStatus_R);
      VL53L1X_GetDistance(DEV_RIGHT, &dist_R);
      VL53L1X_ClearInterrupt(DEV_RIGHT);
    }
    else
    {
      rangeStatus_R = 255;
    }

    sensorData.distance_left = (rangeStatus_L == 0) ? dist_L : 9999;
    sensorData.distance_right = (rangeStatus_R == 0) ? dist_R : 9999;

    
    if (sensorData.distance_left == 9999)
    {
      error_count_L++;
      if (error_count_L >= 3)
      {
        printf("VL53L1X Left crashed! Resetting...\n");
      
        HAL_GPIO_WritePin(SHUT_L_GPIO_Port, SHUT_L_Pin, GPIO_PIN_RESET);
        osDelay(10);
        HAL_GPIO_WritePin(SHUT_L_GPIO_Port, SHUT_L_Pin, GPIO_PIN_SET);
        osDelay(20); // Chờ sensor boot

        // Re-init cảm biến trái
        uint8_t state = 0;
        int boot_timeout = 0;
        while (state == 0 && boot_timeout < 50)
        {
          VL53L1X_BootState(0x52, &state);
          osDelay(2);
          boot_timeout++;
        }
        if (state != 0)
        {
          VL53L1X_SetI2CAddress(0x52, DEV_LEFT);
          VL53L1X_SensorInit(DEV_LEFT);
          VL53L1X_SetDistanceMode(DEV_LEFT, 2);
          VL53L1X_SetTimingBudgetInMs(DEV_LEFT, 50);
          VL53L1X_SetInterMeasurementInMs(DEV_LEFT, 55);
          VL53L1X_StartRanging(DEV_LEFT);
          printf("VL53L1X Left reset complete!\n");
          error_count_L = 0;
        }
      }
    }
    else
    {
      error_count_L = 0;
    }

    /* Reset cảm biến phải nếu lỗi liên tiếp 3 lần */
    if (sensorData.distance_right == 9999)
    {
      error_count_R++;
      if (error_count_R >= 3)
      {
        printf("VL53L1X Right crashed! Resetting...\n");
        // Kéo XSHUT_R xuống LOW trong 10ms
        HAL_GPIO_WritePin(SHUT_R_GPIO_Port, SHUT_R_Pin, GPIO_PIN_RESET);
        osDelay(10);
        HAL_GPIO_WritePin(SHUT_R_GPIO_Port, SHUT_R_Pin, GPIO_PIN_SET);
        osDelay(20); // Chờ sensor boot

        // Re-init cảm biến phải
        uint8_t state = 0;
        int boot_timeout = 0; // Thêm biến đếm

        // Chờ tối đa 50 vòng (tương đương 100ms)
        while (state == 0 && boot_timeout < 50)
        {
          VL53L1X_BootState(DEV_RIGHT, &state);
          osDelay(2);
          boot_timeout++;
        }

        if (state != 0)
        {
          VL53L1X_SensorInit(DEV_RIGHT);
          VL53L1X_SetDistanceMode(DEV_RIGHT, 2);
          VL53L1X_SetTimingBudgetInMs(DEV_RIGHT, 50);
          VL53L1X_SetInterMeasurementInMs(DEV_RIGHT, 55);
          VL53L1X_StartRanging(DEV_RIGHT);
          printf("VL53L1X Right reset complete!\n");
          error_count_R = 0;
        }
        else
        {
          printf("VL53L1X Right hardware dead!\n");
        }
      }
    }
    else
    {
      error_count_R = 0;
    }
    xQueueOverwrite(SensorQueueHandle_native, &sensorData);
    osDelay(100);
  }
  /* USER CODE END StartSensorTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
