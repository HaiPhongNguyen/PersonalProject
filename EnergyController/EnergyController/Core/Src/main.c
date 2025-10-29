/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ADC.h"
#include "ADS1115.h"
#include "LCD2004.h"
#include "Display.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
DateTime_t globalTime;
/* Debounce bằng thời gian (không delay trong ISR) */
static volatile uint32_t s_last_irq_tick = 0U;
#define KEYPAD_DEBOUNCE_MS   30U   // lockout ngắn để tránh lặp
char keyPress = 0;
#define ROWS		4U
#define COLS		4U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
uint8_t    g_portActive[5]   = {0};

static GPIO_TypeDef* ROW_PORT[ROWS] = {R1_GPIO_Port, R2_GPIO_Port, R3_GPIO_Port, R4_GPIO_Port};
static uint16_t      ROW_PIN [ROWS] = {R1_Pin,       R2_Pin,       R3_Pin,       R4_Pin      };
static GPIO_TypeDef* COL_PORT[COLS] = {C1_GPIO_Port, C2_GPIO_Port, C3_GPIO_Port, C4_GPIO_Port};
static uint16_t      COL_PIN [COLS] = {C1_Pin,       C2_Pin,       C3_Pin,       C4_Pin      };

static const char keymap[ROWS][COLS] = {
    {'D','#','0','*'},
    {'C','9','8','7'},
    {'B','6','5','4'},
    {'A','3','2','1'}
};

static inline char Keypad_ScanFast(void)
{
    for (int r = 0; r < ROWS; r++)
    {
        // set tất cả hàng HIGH
        for (int i = 0; i < ROWS; i++) {
            HAL_GPIO_WritePin(ROW_PORT[i], ROW_PIN[i], GPIO_PIN_SET);
        }
        // kéo một hàng xuống LOW
        HAL_GPIO_WritePin(ROW_PORT[r], ROW_PIN[r], GPIO_PIN_RESET);

        // đọc các cột
        for (int c = 0; c < COLS; c++)
        {
            if (HAL_GPIO_ReadPin(COL_PORT[c], COL_PIN[c]) == GPIO_PIN_RESET)
            {
                // trả hàng về HIGH trước khi return
                HAL_GPIO_WritePin(ROW_PORT[r], ROW_PIN[r], GPIO_PIN_SET);
                return keymap[r][c];
            }
        }
    }
    return 0;
}

static inline void Keypad_Init(void)
{
    // Tạo queue: 8 phần tử, mỗi phần tử 1 char (tăng depth cho bền)

    // Kéo tất cả hàng về LOW (mặc định)
    for (int r = 0; r < ROWS; r++) {
        HAL_GPIO_WritePin(ROW_PORT[r], ROW_PIN[r], GPIO_PIN_RESET);
    }
}
/* ==== DateTime helpers: cộng phút vào DateTime_t ==== */
static int is_leap(uint16_t y) { return ((y%4==0 && y%100!=0) || (y%400==0)); }

static uint8_t days_in_month(uint8_t m, uint16_t y)
{
    static const uint8_t d[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (m == 0 || m > 12) return 31;
    if (m == 2) return d[1] + (is_leap(y) ? 1 : 0);
    return d[m-1];
}

/* base->year là 2 chữ số (00..99) => hiểu là 2000+year */
static void DateTime_AddMinutes(const DateTime_t *base, uint32_t addMin, DateTime_t *out)
{
    if (!base || !out) return;

    uint16_t fullY = 2000u + base->year;
    uint8_t  mon   = base->month  ? base->month  : 1;
    uint8_t  day   = base->day    ? base->day    : 1;
    uint8_t  hour  = base->hour;
    uint8_t  min   = base->min;
    uint8_t  sec   = base->sec;   // giữ nguyên giây

    /* cộng phút -> tách phần ngày & phần trong ngày */
    uint32_t totalMin = (uint32_t)hour * 60u + (uint32_t)min + addMin;
    uint32_t daysAdd  = totalMin / (24u * 60u);
    uint32_t minInDay = totalMin % (24u * 60u);

    hour = (uint8_t)(minInDay / 60u);
    min  = (uint8_t)(minInDay % 60u);

    /* cộng ngày, cuốn theo tháng/năm */
    while (daysAdd--)
    {
        uint8_t dim = days_in_month(mon, fullY);
        if (day < dim) {
            day++;
        } else {
            day = 1;
            if (mon < 12) mon++;
            else { mon = 1; fullY++; }
        }
    }

    out->sec   = sec;
    out->min   = min;
    out->hour  = hour;
    out->day   = day;
    out->month = mon;
    out->year  = (uint8_t)(fullY >= 2000 ? (fullY - 2000) : 0);  // 2 chữ số
}

/**************************************/
/*****  Port Service Function *********/
/**************************************/
static void open_port(const uint8_t port_number)
{
	switch(port_number)
	{
		case 0:
			HAL_GPIO_WritePin(SP1_GPIO_Port, SP1_Pin, 0U);
			break;
		case 1:
			HAL_GPIO_WritePin(SP2_GPIO_Port, SP2_Pin, 0U);
			break;
		case 2:
			HAL_GPIO_WritePin(SP3_GPIO_Port, SP3_Pin, 0U);
			break;
		case 3:
			HAL_GPIO_WritePin(SP4_GPIO_Port, SP4_Pin, 0U);
			break;
		case 4:
			HAL_GPIO_WritePin(SIV_GPIO_Port, SIV_Pin, 0U);
			break;
		default:
			HAL_GPIO_WritePin(SP1_GPIO_Port, SP1_Pin, 1U);
			HAL_GPIO_WritePin(SP2_GPIO_Port, SP2_Pin, 1U);
			HAL_GPIO_WritePin(SP3_GPIO_Port, SP3_Pin, 1U);
			HAL_GPIO_WritePin(SP4_GPIO_Port, SP4_Pin, 1U);
			HAL_GPIO_WritePin(SIV_GPIO_Port, SIV_Pin, 1U);
			break;
	}
}
static void close_port(const uint8_t port_number)
{
	switch(port_number)
	{
		case 0:
			HAL_GPIO_WritePin(SP1_GPIO_Port, SP1_Pin, 1U);
			break;
		case 1:
			HAL_GPIO_WritePin(SP2_GPIO_Port, SP2_Pin, 1U);
			break;
		case 2:
			HAL_GPIO_WritePin(SP3_GPIO_Port, SP3_Pin, 1U);
			break;
		case 3:
			HAL_GPIO_WritePin(SP4_GPIO_Port, SP4_Pin, 1U);
			break;
		case 4:
			HAL_GPIO_WritePin(SIV_GPIO_Port, SIV_Pin, 1U);
			break;
		default:
			HAL_GPIO_WritePin(SP1_GPIO_Port, SP1_Pin, 1U);
			HAL_GPIO_WritePin(SP2_GPIO_Port, SP2_Pin, 1U);
			HAL_GPIO_WritePin(SP3_GPIO_Port, SP3_Pin, 1U);
			HAL_GPIO_WritePin(SP4_GPIO_Port, SP4_Pin, 1U);
			HAL_GPIO_WritePin(SIV_GPIO_Port, SIV_Pin, 1U);
			break;
	}
}
static void PortControl_OnSelectFromPage2(uint8_t port)
{
    if (port < 1 || port > 5) return;
    uint8_t idx = (uint8_t)(port - 1);

    if (!g_portActive[idx]) {
        open_port(port-1);       // mở phần cứng
        g_portActive[idx] = 1;
        // chưa có endTime, đợi đặt từ Page3
        g_endTime_set[idx] = 0;
    }
}
static void PortControl_Service(void)
{
    static uint32_t lastTick = 0;
    uint32_t nowTick = osKernelGetTickCount();
    if ((nowTick - lastTick) < 1000U) return;  // 1 Hz
    lastTick = nowTick;

    // cập nhật thời gian hiện tại
    DS1307_ReadDateTime(&globalTime);

    for (uint8_t i = 0; i < 5; ++i) {
        if (g_portActive[i] && g_endTime_set[i]) {
            // nếu now >= endTime -> đóng cổng
            if (DateTime_Compare(&globalTime, &g_endTime[i]) >= 0) {
            	close_port(i);   // đóng phần cứng
                g_portActive[i]  = 0;
                g_endTime_set[i] = 0;
            }
        }
    }
}

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* Definitions for defaultTask */

/* USER CODE BEGIN PV */

osThreadId_t ReadSourceTaskHandle;
const osThreadAttr_t ReadInputTask_attributes = {
  .name = "Read_Input_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal2,
};

osThreadId_t DisplayTaskHandle;
const osThreadAttr_t Display_attributes = {
  .name = "DisplayTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
void StartDefaultTask(void *argument);

/* USER CODE BEGIN PFP */
void ReadSource(void *pvParameter);
void DisplayTask(void *pvParameter);
void Keypad_EXTI_Callback(uint16_t GPIO_Pin);


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**************** Global Variables ************/
SensorData_t InputSourceData;
SourceOutputData_t OutputSourceData;
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
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  ReadSourceTaskHandle = osThreadNew(ReadSource, NULL, &ReadInputTask_attributes);
  if (ReadSourceTaskHandle == NULL) {
    // Không đủ heap, sai priority mapping, hoặc header/RTOS mismatch
    Error_Handler();
  }

  DisplayTaskHandle = osThreadNew(DisplayTask, NULL, &Display_attributes);
  if (DisplayTaskHandle == NULL) {
    Error_Handler();
  }
//  /* USER CODE BEGIN RTOS_THREADS */
  DS1307_Init(&hi2c2);
  Display_Init(&hi2c2, 0x27); // hoặc 0x3F tùy module
  Sensor_Init(&hi2c1, &hadc1);
  Sensor_Calibrate(10U);
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL8;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV4;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_7CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 100000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

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
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BZ_GPIO_Port, BZ_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, R4_Pin|R3_Pin|R2_Pin|R1_Pin
                          |SP4_Pin|SP2_Pin|SP1_Pin|SIV_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SP3_GPIO_Port, SP3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : BZ_Pin */
  GPIO_InitStruct.Pin = BZ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BZ_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : C4_Pin C3_Pin C2_Pin C1_Pin */
  GPIO_InitStruct.Pin = C4_Pin|C3_Pin|C2_Pin|C1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : R4_Pin R3_Pin R2_Pin R1_Pin
                           SP4_Pin SP2_Pin SP1_Pin SIV_Pin */
  GPIO_InitStruct.Pin = R4_Pin|R3_Pin|R2_Pin|R1_Pin
                          |SP4_Pin|SP2_Pin|SP1_Pin|SIV_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : SP3_Pin */
  GPIO_InitStruct.Pin = SP3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SP3_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_IRQn, 7, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 7, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    Keypad_EXTI_Callback(GPIO_Pin);
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
void ReadSource(void *pvParameter)
{
	while(1)
	{
		Sensor_ReadAll(&InputSourceData);
		SourceOutput_ReadAll(&OutputSourceData);
		osDelay(1000U);
	}

}

void DisplayTask(void *pvParameter)
{
    DS1307_ReadDateTime(&globalTime);
    typedef enum {
        PAGE1 = 1,
        PAGE2,
        PAGE3,
        PAGE4,
        PAGE5,
        PAGE5_1, PAGE5_2, PAGE5_3, PAGE5_4, PAGE5_5
    } AppPage_t;

    AppPage_t page = PAGE1;
    AppPage_t lastPage = 0;          // theo dõi chuyển trang để chỉ vẽ khi cần

    uint8_t   selectedPort   = 1;    // 1..5 (được chọn ở Page2)
    int8_t    timeOption     = -1;   // -1 = chưa chọn; 1..5 (được chọn ở Page3)
    int8_t    lastTimeOption = -2;   // giá trị đệm để phát hiện thay đổi ở Page3

    uint8_t needRedrawPage3 = 0;     // cờ yêu cầu vẽ lại Page3

    const uint32_t UI_REFRESH_MS = 200;
    uint32_t lastRefreshTick = osKernelGetTickCount();

    // lần đầu render Page1
    Display_UpdatePage1(&InputSourceData, &globalTime);

    for (;;)
    {
        // 1) đọc phím (non-blocking)
        char key = keyPress;

        // 2) xử lý theo trang hiện tại
        if (key != 0)
        {
            switch (page)
            {
            case PAGE1:
                if (key == '*')
                {
                    page = PAGE2;
                    if (lastPage != page) { Display_Page2(); }
                }
                else if (key >= '1' && key <= '5')
                {
                    uint8_t idx = (uint8_t)(key - '0');
                    page = (AppPage_t)(PAGE5_1 + (idx - 1));
                    if (lastPage != page) { Display_Page5_Select(idx, &OutputSourceData); }
                }
                break;

            case PAGE2:
                if (key >= '1' && key <= '5')
                {
                    selectedPort = (uint8_t)(key - '0');
                    PortControl_OnSelectFromPage2(selectedPort);

                    page = PAGE3;
                    timeOption = -1;
                    needRedrawPage3 = 1;
                }
                else if (key == '0')
                {
                    page = PAGE1;
                    if (lastPage != page) { Display_UpdatePage1(&InputSourceData, &globalTime); }
                }
                break;

            case PAGE3:
                if (key >= '1' && key <= '5')
                {
                    timeOption = (int8_t)(key - '0');
                    if (timeOption != lastTimeOption)
                        needRedrawPage3 = 1;
                }
                else if (key == '#')
                {
                    if (timeOption == -1)
                    {
                        // chưa chọn -> hiển thị nhắc
                    }
                    else if (timeOption >= 1 && timeOption <= 4)
                    {
                        uint32_t addMin = (timeOption == 1) ? 30u :
                                          (timeOption == 2) ? 60u :
                                          (timeOption == 3) ? 120u : 240u;

                        DS1307_ReadDateTime(&globalTime);
                        DateTime_t et;
                        DateTime_AddMinutes(&globalTime, addMin, &et);

                        if (selectedPort >= 1 && selectedPort <= 5)
                        {
                            uint8_t idx = (uint8_t)(selectedPort - 1);
                            g_endTime[idx] = et;
                            g_endTime_set[idx] = 1;
                        }

                        page = PAGE5;
                        if (lastPage != page) { Display_Page5_Select(selectedPort, &OutputSourceData); }
                    }
                    else // timeOption == 5 -> custom
                    {
                        page = PAGE4;
                        if (lastPage != page)
                        {
                            key = 0;
                            Display_Page4();
                        }
                    }
                }
                else if (key == '*')
                {
                    page = PAGE2;
                    if (lastPage != page) { Display_Page2(); }
                }
                else if (key == '0')
                {
                    page = PAGE1;
                    if (lastPage != page) { Display_UpdatePage1(&InputSourceData, &globalTime); }
                }
                break;

            case PAGE4:
                if (key >= '0' && key <= '9')
                {
                    Display_Page4_HandleKey(key);
                }
                else if (key == '#')
                {
                    uint16_t chargeMinutes = Display_GetInputMinutes();
                    if (chargeMinutes == 0) chargeMinutes = 1;

                    DS1307_ReadDateTime(&globalTime);
                    DateTime_t et;
                    DateTime_AddMinutes(&globalTime, chargeMinutes, &et);

                    if (selectedPort >= 1 && selectedPort <= 5)
                    {
                        uint8_t idx = (uint8_t)(selectedPort - 1);
                        g_endTime[idx] = et;
                        g_endTime_set[idx] = 1;
                    }

                    page = PAGE5;
                    if (lastPage != page) { Display_Page5_Select(selectedPort, &OutputSourceData); }
                    Display_ClearInputBuffer();
                }
                else if (key == '*')
                {
                    Display_ClearInputBuffer();
                    page = PAGE3;
                    needRedrawPage3 = 1;
                }
                break;

            case PAGE5:
                if (key == '0')
                {
                    page = PAGE1;
                    if (lastPage != page) { Display_UpdatePage1(&InputSourceData, &globalTime); }
                }
                else if (key >= '1' && key <= '5')
                {
                    uint8_t idx = (uint8_t)(key - '0');
                    page = (AppPage_t)(PAGE5_1 + (idx - 1));
                    if (lastPage != page) { Display_Page5_Select(idx, &OutputSourceData); }
                }
                else if (key == '*')
                {
                    page = PAGE3;
                    needRedrawPage3 = 1;
                }
                break;

            case PAGE5_1:
            case PAGE5_2:
            case PAGE5_3:
            case PAGE5_4:
            case PAGE5_5:
                if (key == '0')
                {
                    page = PAGE1;
                    if (lastPage != page) { Display_UpdatePage1(&InputSourceData, &globalTime); }
                }
                else if (key == '*')
                {
                    page = PAGE2;
                    if (lastPage != page) { Display_Page2(); }
                }
                break;

            default:
                page = PAGE1;
                if (lastPage != page) { Display_UpdatePage1(&InputSourceData, &globalTime); }
                break;
            }

            // ✅ Reset phím sau khi xử lý xong
            keyPress = 0;
        }

        // 2.5) Vẽ khi phát hiện CHUYỂN TRANG hoặc cần vẽ Page3
        if (lastPage != page)
        {
            if (page == PAGE3)
            {
                lastTimeOption = -2;
                needRedrawPage3 = 1;
                Display_Page3();
            }
            lastPage = page;
        }

        if (page == PAGE3 && needRedrawPage3)
        {
            Display_Page3();
            lastTimeOption = timeOption;
            needRedrawPage3 = 0;
        }

        // 3) refresh định kỳ
        uint32_t now = osKernelGetTickCount();
        if ((now - lastRefreshTick) >= UI_REFRESH_MS)
        {
            lastRefreshTick = now;
            switch (page)
            {
                case PAGE1:
                    Display_UpdatePage1(&InputSourceData, &globalTime);
                    break;
                case PAGE2:
                    break;
                case PAGE3:
                    break;
                case PAGE4:
                    break;
                case PAGE5:
                    break;
                case PAGE5_1: Display_Page5_Select(1, &OutputSourceData); break;
                case PAGE5_2: Display_Page5_Select(2, &OutputSourceData); break;
                case PAGE5_3: Display_Page5_Select(3, &OutputSourceData); break;
                case PAGE5_4: Display_Page5_Select(4, &OutputSourceData); break;
                case PAGE5_5: Display_Page5_Select(5, &OutputSourceData); break;
                default: break;
            }
        }

        PortControl_Service();
    }
}




void Keypad_EXTI_Callback(uint16_t GPIO_Pin)
{
    (void)GPIO_Pin;

    // Tạm thời vô hiệu hoá ngắt (tránh ngắt lặp)

    // Gửi tín hiệu cho task keypad (CMSIS v2)
    // osSemaphoreRelease có thể gọi trong ISR trên RTX5; context switch sẽ được xử lý khi thoát ISR.
    // ví dụ: bật LED báo lỗi ISR priority
    HAL_GPIO_WritePin(BZ_GPIO_Port, BZ_Pin, 1U);
    osDelay(250);
    HAL_GPIO_WritePin(BZ_GPIO_Port, BZ_Pin, 0U);

    (void)GPIO_Pin;

    // Debounce bằng thời gian – KHÔNG dùng osDelay trong ISR
    uint32_t now = HAL_GetTick();
    if ((now - s_last_irq_tick) < KEYPAD_DEBOUNCE_MS) {
        return; // bỏ qua các ngắt quá sát nhau
    }
    s_last_irq_tick = now;

    // Quét ma trận nhanh
    char key = Keypad_ScanFast();

    // Đẩy phím vào queue cho DisplayTask đọc
    if (key != 0) {
        keyPress = key;
    }

    // Trả các hàng về LOW (trạng thái idle)
    for (int r = 0; r < ROWS; r++) {
        HAL_GPIO_WritePin(ROW_PORT[r], ROW_PIN[r], GPIO_PIN_RESET);
    }
}


/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1)
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
