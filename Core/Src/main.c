/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
#include "EEPROM.h";
#include "SIM800H.h";
const unsigned char ACK[2] = {	0x06, 0x00 };
extern unsigned char U2Rx_data[];
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
//extern unsigned char RXTimeout;
unsigned char RXTimeout;
extern unsigned char aRxBuffer[];
extern unsigned char aTxBuffer[];

unsigned char RXBUF[];
unsigned char RXP;
unsigned char RXCP;
extern unsigned char SPIPageBUF[];




extern unsigned char Reload_Device_Flag;
extern unsigned char LearnMode;
extern unsigned char LearnTimeout;
extern unsigned char creg_timer;
extern float Bat_Low_V;
extern float Bat_Ok_V;
extern unsigned char prev_id;
extern unsigned int prev_id_timer;
extern unsigned char Health_TX_Hour;
extern unsigned char Health_TX_Minute;
extern unsigned char Health_TX_days;

extern unsigned int no_data_Timer;

extern unsigned char NO_SIM;

extern unsigned char PrevSerialNum[4];
extern unsigned char match;

unsigned char minute_flag = 0;
unsigned char sec = 0;

unsigned char Erase_RF_Devices = 0;

signed int ADCValue = 0;

unsigned char bat_sample = 0;
unsigned char Sample_Battery = 0;

unsigned char Battery_Low = 0;
unsigned char Battery_Ok = 0;
unsigned char BatLow_Debounce = 0;
unsigned char BatOk_Debounce = 0;

unsigned char Prev_Mains = 1;
unsigned char Mains_Debounce = 0;


unsigned char Message[200];
extern unsigned char Out_Message[]; // 200
unsigned char Loopcount;
unsigned char Check_GSM_Registered = 0;
unsigned char Reset_GSM_Flag = 0;
unsigned char GPRS_Enabled = 0;
unsigned char GPRS_Barear_Started = 0;
unsigned long running_seconds = 0;
unsigned char Update_Mem_Flag = 0;

unsigned int year = 0;
unsigned char month = 0;
unsigned char day = 0;
unsigned char hours = 0;
unsigned char minutes = 0;
unsigned char seconds = 0;

unsigned char Disable_LED = 0;

unsigned char Airtime_Req_Timer = 0;

unsigned char TX_days_counter = 0;

unsigned char Panic_Out_Time = 0;
unsigned char Alarm_Out_Time = 0;
unsigned int Buzzer_Time = 0;
unsigned int Siren_Time = 0;

unsigned char Arm_Status = 0;
unsigned char Learn_Button_Time = 0;

extern unsigned int MesQue_Head;
extern unsigned int MesQue_Tail;

unsigned char Send_Health_Message = 0;

extern float BatteryVoltage;

#define TIMESYNC_SMS					100
#define PASSWORDS						101
#define SMSC_NUMBERS					102
#define TIME_24H_MESSAGE				103
#define INPUT_STATUS1_2					104
#define OUTPUTS_TIMER_VALUES			105
#define SIGNAL_STRENGTH					106
#define SOFTWARE_VERSION				107
#define AIRTIME							108
#define DATETIME						109
#define CELLREQ							110
#define OUTPUTS_STATUS					111
#define GPRS_SETTINGS_MESSAGE			112
#define GPRS_STATUS_MESSAGE				113
#define INPUT_STATUS5_6					114
#define CELL_LOCATION					115
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc;

I2C_HandleTypeDef hi2c1;

IWDG_HandleTypeDef hiwdg;

RTC_HandleTypeDef hrtc;

TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim7;

UART_HandleTypeDef huart2;

osThreadId defaultTaskHandle;
uint32_t defaultTaskBuffer[ 256 ];
osStaticThreadDef_t defaultTaskControlBlock;
osThreadId GSMTaskHandle;
uint32_t GSMTaskBuffer[ 256 ];
osStaticThreadDef_t GSMTaskControlBlock;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC_Init(void);
static void MX_I2C1_Init(void);
static void MX_RTC_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_IWDG_Init(void);
static void MX_TIM6_Init(void);
static void MX_TIM7_Init(void);
void StartDefaultTask(void const * argument);
void StartGSMTask(void const * argument);

static void MX_NVIC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART2)	//current UART
	{
		SIM800H_RX();
	}
}

/*
void Clear_RX_Buf(void)
{ unsigned int t = 0;
  for(t = 0;t < 256;t++) {
    RXBUF[t]=0;
  }
  RXP=0;
  RXCP=0;
}

unsigned char Search_RXBUF(void)
//   Search String = BUF
{ 	unsigned char x, y, Match, Pos, tc;
	unsigned char write_failed = 0;
	unsigned long addr;
	unsigned char * end;

	Pos=0xFF;
	y=0;
	Match=0;
	x=RXCP;
	while(x!=RXP) {
		if((RXBUF[x]>0)&&(RXBUF[x]<8)) {
			if(RXP!=(x-1)) {
				if((RXBUF[x] == 2)||(RXBUF[x] == 6))  {  // Write 24LC256 or 24LC512
					//smalleeprom = 0;
					write_failed = 0;
				    Match=x;
					Match++;
					if(Match!=RXP) {
						addr = RXBUF[Match] << 8;
						Match++;
						if(Match!=RXP) { addr |= RXBUF[Match];  Match++; }
						  else {
							  RXCP=Match;
							  break;
						  }  //  exit if not valid
						memcpy(&aTxBuffer, &RXBUF[3], RXP - 3);
						if(IIC_EEPROM_Write_Page(addr, RXP-3) == 0) {
							CDC_Transmit_FS(ACK, 1);  // Send ACK to PC
						} else {
							Clear_RX_Buf();
							break;
						}
					}
					Clear_RX_Buf();
					break;
				} else if((RXBUF[x] == 4)||(RXBUF[x] == 7))  {  // Read 24LC256 or 24LC512
					  //smalleeprom = 0;
					  Match=x;
					  Match++;
					  if(Match!=RXP) {
						  addr = RXBUF[Match] << 8;
						  Match++;
						  if(Match!=RXP) { addr |= RXBUF[Match];  Match++; }
							  else { RXCP=Match;  break; }  //  exit if not valid
						  tc = RXBUF[Match];
						  IIC_EEPROM_Read_Page(addr, 128);
						  end = memchr(aRxBuffer, tc, 128);
						  if(end == NULL) {
							  CDC_Transmit_FS(aRxBuffer, 64);
						  } else {
							  CDC_Transmit_FS(aRxBuffer, end - &aRxBuffer[0] + 1);
						  }

					  }
					  Clear_RX_Buf();
					  break;
			    } else if(RXBUF[x] == 5)  {  // Check Hardeware version
			    	Match=x;
			    	if(Match!=RXP) {
						CDC_Transmit_FS(ACK, 1);  // Send ACK to PC
					}
					Clear_RX_Buf();
					break;
			    } else {
					Clear_RX_Buf();
					break;
				}
			} else { // end if RXP!=
				Clear_RX_Buf();
				break;
			}
		}
		x++;
	}

	return(1);
}
*/
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
  MX_ADC_Init();
  MX_I2C1_Init();
  MX_RTC_Init();
  MX_USART2_UART_Init();
  MX_IWDG_Init();
  MX_TIM6_Init();
  MX_TIM7_Init();

  /* Initialize interrupts */
  MX_NVIC_Init();
  /* USER CODE BEGIN 2 */

   HAL_UART_Receive_IT(&huart2, U2Rx_data, 1);  // enable UART2 RX interrupt on RX
  /* USER CODE END 2 */

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
  /* definition and creation of defaultTask */
  osThreadStaticDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 256, defaultTaskBuffer, &defaultTaskControlBlock);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of GSMTask */
  osThreadStaticDef(GSMTask, StartGSMTask, osPriorityIdle, 0, 256, GSMTaskBuffer, &GSMTaskControlBlock);
  GSMTaskHandle = osThreadCreate(osThread(GSMTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

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

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_HIGH);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI14
                              |RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI14CalibrationValue = 16;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1|RCC_PERIPHCLK_RTC;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief NVIC Configuration.
  * @retval None
  */
static void MX_NVIC_Init(void)
{
  /* I2C1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(I2C1_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(I2C1_IRQn);
  /* RTC_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(RTC_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(RTC_IRQn);
  /* RCC_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(RCC_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(RCC_IRQn);
  /* ADC1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(ADC1_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(ADC1_IRQn);
  /* TIM6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(TIM6_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(TIM6_IRQn);
  /* TIM7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(TIM7_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(TIM7_IRQn);
  /* EXTI4_15_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
  /* USART2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(USART2_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(USART2_IRQn);
}

/**
  * @brief ADC Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC_Init(void)
{

  /* USER CODE BEGIN ADC_Init 0 */

  /* USER CODE END ADC_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC_Init 1 */

  /* USER CODE END ADC_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc.Instance = ADC1;
  hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc.Init.LowPowerAutoWait = DISABLE;
  hadc.Init.LowPowerAutoPowerOff = DISABLE;
  hadc.Init.ContinuousConvMode = DISABLE;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.DMAContinuousRequests = DISABLE;
  hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_8;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC_Init 2 */

  /* USER CODE END ADC_Init 2 */

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
  hi2c1.Init.Timing = 0x2000090E;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief IWDG Initialization Function
  * @param None
  * @retval None
  */
static void MX_IWDG_Init(void)
{

  /* USER CODE BEGIN IWDG_Init 0 */

  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */

  /* USER CODE END IWDG_Init 1 */
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
  hiwdg.Init.Window = 4095;
  hiwdg.Init.Reload = 4095;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG_Init 2 */

  /* USER CODE END IWDG_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};
  RTC_AlarmTypeDef sAlarm = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0;
  sTime.Minutes = 0;
  sTime.Seconds = 0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 1;
  sDate.Year = 0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable the Alarm A
  */
  sAlarm.AlarmTime.Hours = 0;
  sAlarm.AlarmTime.Minutes = 0;
  sAlarm.AlarmTime.Seconds = 0;
  sAlarm.AlarmTime.SubSeconds = 0;
  sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
  sAlarm.AlarmMask = RTC_ALARMMASK_ALL;
  sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_NONE;
  sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
  sAlarm.AlarmDateWeekDay = 1;
  sAlarm.Alarm = RTC_ALARM_A;
  if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 100;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 10000;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */
  HAL_TIM_Base_Start_IT(&htim6);
  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief TIM7 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM7_Init(void)
{

  /* USER CODE BEGIN TIM7_Init 0 */

  /* USER CODE END TIM7_Init 0 */

  /* USER CODE BEGIN TIM7_Init 1 */

  /* USER CODE END TIM7_Init 1 */
  htim7.Instance = TIM7;
  htim7.Init.Prescaler = 2000;
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim7.Init.Period = 1000;
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM7_Init 2 */
  HAL_TIM_Base_Start_IT(&htim7);
  /* USER CODE END TIM7_Init 2 */

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
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
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

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GSM_ONOFF_Pin|GSM_PWR_Pin|OUT_ONOFF_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, RELAY2_Pin|RELAY1_Pin|RFRX_LED_Pin|OUT_ALARM_Pin
                          |OUT_PANIC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : GSM_ONOFF_Pin GSM_PWR_Pin OUT_ONOFF_Pin */
  GPIO_InitStruct.Pin = GSM_ONOFF_Pin|GSM_PWR_Pin|OUT_ONOFF_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : GSM_RI_Pin */
  GPIO_InitStruct.Pin = GSM_RI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GSM_RI_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PA5 PA6 PA7 PA9
                           PA10 PA11 PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_9
                          |GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : MAINS_ONOFF_Pin */
  GPIO_InitStruct.Pin = MAINS_ONOFF_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MAINS_ONOFF_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PB2 PB10 PB11 PB5
                           PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_5
                          |GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : RELAY2_Pin RELAY1_Pin RFRX_LED_Pin OUT_ALARM_Pin
                           OUT_PANIC_Pin */
  GPIO_InitStruct.Pin = RELAY2_Pin|RELAY1_Pin|RFRX_LED_Pin|OUT_ALARM_Pin
                          |OUT_PANIC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : RFRX_LEARN_BUTTON_Pin */
  GPIO_InitStruct.Pin = RFRX_LEARN_BUTTON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(RFRX_LEARN_BUTTON_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : RFRX_IN_Pin */
  GPIO_InitStruct.Pin = RFRX_IN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(RFRX_IN_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{	// 1 Second Interrupt

	if(Siren_Time > 0) {
		Siren_Time--;
		if(Siren_Time == 0) {
			HAL_GPIO_WritePin(GPIOB, RELAY1_Pin, GPIO_PIN_RESET);
		} else {
			HAL_GPIO_WritePin(GPIOB, RELAY1_Pin, GPIO_PIN_SET);
		}
	}

	if(Buzzer_Time > 0) {
		Buzzer_Time--;
		if(Buzzer_Time == 0) {
			HAL_GPIO_WritePin(GPIOB, RELAY2_Pin, GPIO_PIN_RESET);
		} else {
			HAL_GPIO_WritePin(GPIOB, RELAY2_Pin, GPIO_PIN_SET);
		}
	}

	if(Alarm_Out_Time > 0) {
		Alarm_Out_Time--;
		if(Alarm_Out_Time == 0) {
			HAL_GPIO_WritePin(GPIOB, OUT_ALARM_Pin, GPIO_PIN_RESET);
		} else {
			HAL_GPIO_WritePin(GPIOB, OUT_ALARM_Pin, GPIO_PIN_SET);
		}
	}

	if(Panic_Out_Time > 0) {
		Panic_Out_Time--;
		if(Panic_Out_Time == 0) {
			HAL_GPIO_WritePin(GPIOB, OUT_PANIC_Pin, GPIO_PIN_RESET);
		} else {
			HAL_GPIO_WritePin(GPIOB, OUT_PANIC_Pin, GPIO_PIN_SET);
		}
	}

	if(HAL_GPIO_ReadPin(RFRX_LEARN_BUTTON_GPIO_Port, RFRX_LEARN_BUTTON_Pin) == 0) {
		Learn_Button_Time++;
		if(Learn_Button_Time >= 5) {
			Erase_RF_Devices = 1;
			Learn_Button_Time = 0;
		}
	} else {
		Learn_Button_Time = 0;
	}

	if(LearnTimeout > 0) {
		LearnTimeout--;
		if(LearnTimeout == 0) {
			LearnMode = 0;
			HAL_GPIO_WritePin(GPIOB, RFRX_LED_Pin, GPIO_PIN_RESET);
		}
	}

	if(creg_timer > 0) {
		creg_timer--;
	}

	sec++;
	if(sec > 59) {
		minute_flag = 1;
		sec = 0;
	}

	if(bat_sample > 10) {
		bat_sample = 0;
		Sample_Battery = 1;

	} else {
		bat_sample++;
	}

	no_data_Timer++;
	if(no_data_Timer == 3) {
		memset(PrevSerialNum, 0, 4);
		match = 0;

	}

	if(Airtime_Req_Timer > 0) {
		Airtime_Req_Timer--;
	}
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN 5 */
  unsigned char t = 0;
  /* Infinite loop */
//  Message[0] = TIMESYNC_SMS;
//    Message[1] = 0;
//    Add_Message_to_Que(Message, 1, EE_OWN_NUMBER, 0, TRUE);

//	Message[0] = AIRTIME;
//	Message[1] = 0;
//	Add_Message_to_Que(Message, 0, 0, 0, TRUE);

    Reload_Device_Flag = 1;
    Update_Mem_Flag = TRUE;


    for(;;)
    {
  	if(t > 5) {
  		HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
  		t = 0;
  	}
  	t++;

  	if(Update_Mem_Flag == TRUE) {
  		Update_Mem_Flag = FALSE;
  		Update_Mem_Values();
  	}

  	Check_RF_Flags();

  //	if(RXTimeout == 2) {
 // 		Search_RXBUF();
 // 	} else if(RXTimeout < 10){
 // 		RXTimeout++;
 // 	}

  	if(minute_flag == 1) {
  			minute_flag = 0;

  			RTC_TimeTypeDef sTime;
  			RTC_DateTypeDef sDate;

  			HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
  			HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

  			if(((unsigned char)sTime.Hours == (unsigned char)Health_TX_Hour)&&((unsigned char)sTime.Minutes == (unsigned char)Health_TX_Minute)) {
  				TX_days_counter++;
  				if(TX_days_counter >= Health_TX_days) {
  					TX_days_counter = 0;

  					Send_Health_Message = TRUE;
  				}
  			}

  		}

  		if(Send_Health_Message == TRUE) {
  			Send_Health_Message = FALSE;
  			Read_Settings(EE_PREPAID_USSD);
  			if((SPIPageBUF[0] == '*')&&(strlen(SPIPageBUF) > 2)) {
  				Message[0] = AIRTIME;
  				Message[1] = 0;
  				Message[2] = 0;
  			} else {
  				Message[0] = 114;
  				Message[1] = EE_AT_MESSAGE;
  				Message[2] = 0;
  			}
  			Add_Message_to_Que(Message, 0, 0, 0, TRUE);

  			Message[0] = TIMESYNC_SMS;
  			Message[1] = 0;
  			Add_Message_to_Que(Message, 1, EE_OWN_NUMBER, 0, TRUE);
  		}

  		if(prev_id_timer > 0) {
  			prev_id_timer--;
  			if(prev_id_timer == 0) {
  				prev_id = 0;
  			}
  		}

  		if(Sample_Battery == 1) {
  			Sample_Battery = 0;
  			if (HAL_ADC_Start(&hadc) != HAL_OK)
  			{
  				/* Start Conversation Error */
  			}
  			if (HAL_ADC_PollForConversion(&hadc, 500) != HAL_OK)
  			{
  				/* End Of Conversion flag not set on time */
  				ADCValue=-1;
  			}
  			else
  			{
  				/* ADC conversion completed */
  				/*##-5- Get the converted value of regular channel ########################*/
  				ADCValue = HAL_ADC_GetValue(&hadc);
  				BatteryVoltage = (ADCValue * 0.00388);
  			}
  			HAL_ADC_Stop(&hadc);

  			if((BatteryVoltage <  Bat_Low_V)&&(Battery_Low == 0)) {
  				if(BatLow_Debounce > 10) {
  					Battery_Low = 1;
  					Battery_Ok = 0;

  					Message[0] = EE_BATTERY_LOW;
  					Message[1] = EE_AT_MESSAGE;
  					Message[2] = 0;
  					Add_Message_to_Que(Message, 0, 0, 0, TRUE);

  				} else {
  					BatLow_Debounce++;
  				}
  			} else if((BatteryVoltage >  Bat_Ok_V)&&(Battery_Ok == 0)) {
  				if(BatOk_Debounce > 10) {
  					Battery_Low = 0;
  					Battery_Ok = 1;

  					Message[0] = EE_BATTERY_OK;
  					Message[1] = EE_AT_MESSAGE;
  					Message[2] = 0;
  					Add_Message_to_Que(Message, 0, 0, 0, TRUE);

  				} else {
  					BatOk_Debounce++;
  				}
  			}

  			if(HAL_GPIO_ReadPin(GPIOB, MAINS_ONOFF_Pin) != Prev_Mains) {
  				if(Mains_Debounce < 200) {
  					Mains_Debounce++;
  					if(Mains_Debounce == 20) {
  						Prev_Mains = HAL_GPIO_ReadPin(GPIOB, MAINS_ONOFF_Pin);
  						if(Prev_Mains == 1) {
  							Message[0] = EE_MAINS_ON;
  							Message[1] = EE_AT_MESSAGE;
  							Message[2] = 0;
  							Add_Message_to_Que(Message, 0, 0, 0, TRUE);
  						} else {
  							Message[0] = EE_MAINS_OFF;
  							Message[1] = EE_AT_MESSAGE;
  							Message[2] = 0;
  							Add_Message_to_Que(Message, 0, 0, 0, TRUE);
  						}
  					}
  				}
  			} else {
  				Mains_Debounce = 0;
  			}
  		}

  		if(Erase_RF_Devices == 1) {
  			Disable_LED = 1;
  			Clear_RF_Device_List();
  			for(unsigned char u = 0; u < 20; u++) {

  				HAL_GPIO_WritePin(GPIOB, RFRX_LED_Pin, GPIO_PIN_SET);
  				Delay_LED();   Delay_LED();  	Delay_LED();   Delay_LED();

  				HAL_GPIO_WritePin(GPIOB, RFRX_LED_Pin, GPIO_PIN_RESET);
  				Delay_LED();  Delay_LED();  	Delay_LED();   Delay_LED();
  			}
  			Disable_LED = 0;
  			Erase_RF_Devices = 0;
  			LearnMode = 0;
  		}

  	    osDelay(20);
  	  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartGSMTask */
/**
* @brief Function implementing the GSMTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartGSMTask */
void StartGSMTask(void const * argument)
{
  /* USER CODE BEGIN StartGSMTask */
  /* Infinite loop */
	 Reset_GSM_Flag = 1;

	  /* Infinite loop */
	  for(;;)
	  {
		  HAL_IWDG_Refresh(&hiwdg);
		  	HAL_UART_Receive_IT(&huart2, U2Rx_data, 1);  // enable UART2 RX interrupt on RX

		  	if(Loopcount < 40) {
		  		Loopcount++;
		  	} else {
		  		if(Check_GSM_Registered) {
		  			Check_GSM_Registered = FALSE;
		  			if(!SIM800H_Register_Status()) {
		  				SIM800H_Restart();
		  			}
		  		}
		  		if(NO_SIM == 0 && Check_for_Messages()) {
		  			Loopcount = 30;
		  		} else {
		  			Loopcount = 0;
		  		}

		  	}

		  	if(Reset_GSM_Flag == TRUE) {
		  		Reset_GSM_Flag = FALSE;

		  		SIM800H_Restart();


		  	}

		  	if(Check_MessageQue() == TRUE) {   // Check if there is message to be send
		  		HAL_IWDG_Refresh(&hiwdg);
		  		if(GPRS_Enabled == TRUE) {
		  			if(GPRS_Barear_Started != TRUE) {
		  				SIM800H_Restart();
		  			}
		  		}
		  		running_seconds = 0;  // clear watchdog counter
		  		Send_Message(Out_Message);
		  	}

		  	osDelay(50);  // 100

	  }
  /* USER CODE END StartGSMTask */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM17 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM17) {
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
