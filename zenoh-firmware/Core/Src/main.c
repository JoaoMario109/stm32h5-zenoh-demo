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
#include "cmsis_os2.h"
#include "icache.h"
#include "memorymap.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_ARP.h"
#include "task.h"

#ifndef ZENOH_FREERTOS_PLUS_TCP
  #define ZENOH_FREERTOS_PLUS_TCP
#endif

#include <zenoh-pico.h>

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
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define APP_TASK_STACK_DEPTH configMINIMAL_STACK_SIZE
#define APP_TASK_PRIORITY 10

#define MODE "client"
#define LOCATOR ""  // If empty, it will scout
#define KEYEXPR "demo/example/**"

static const uint8_t ucIPAddress[] = {192, 168, 1, 80};
static const uint8_t ucNetMask[] = {255, 255, 255, 0};
static const uint8_t ucGatewayAddress[] = {192, 168, 1, 1};
static const uint8_t ucDNSServerAddress[] = {192, 168, 1, 1};
static const uint8_t ucMACAddress[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

NetworkInterface_t *pxLibslirp_FillInterfaceDescriptor(BaseType_t xEMACIndex, NetworkInterface_t *pxInterface);

static NetworkInterface_t xInterface;
static NetworkEndPoint_t xEndPoint;

static TaskHandle_t xAppTaskHandle;
static StaticTask_t xAppTaskTCB;
static StackType_t uxAppTaskStack[APP_TASK_STACK_DEPTH];

void data_handler(z_loaned_sample_t *sample, void *ctx) {
  (void)(ctx);
  z_view_string_t keystr;
  z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
  z_owned_string_t value;
  z_bytes_to_string(z_sample_payload(sample), &value);
  printf(">> [Subscriber] Received ('%.*s': '%.*s')\n", (int)z_string_len(z_loan(keystr)),
         z_string_data(z_loan(keystr)), (int)z_string_len(z_loan(value)), z_string_data(z_loan(value)));
  z_drop(z_move(value));
}

void app_main(void) {
  z_owned_config_t config;
  z_config_default(&config);
  zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, MODE);
  if (strcmp(LOCATOR, "") != 0) {
      if (strcmp(MODE, "client") == 0) {
          zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, LOCATOR);
      } else {
          zp_config_insert(z_loan_mut(config), Z_CONFIG_LISTEN_KEY, LOCATOR);
      }
  }

  printf("Opening session...\n");
  z_owned_session_t s;
  if (z_open(&s, z_move(config), NULL) < 0) {
      printf("Unable to open session!\n");
      return;
  }

  // Start read and lease tasks for zenoh-pico
  if (zp_start_read_task(z_loan_mut(s), NULL) < 0 || zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
      printf("Unable to start read and lease tasks\n");
      z_session_drop(z_session_move(&s));
      return;
  }

  z_owned_closure_sample_t callback;
  z_closure(&callback, data_handler, NULL, NULL);
  printf("Declaring Subscriber on '%s'...\n", KEYEXPR);
  z_view_keyexpr_t ke;
  z_view_keyexpr_from_str_unchecked(&ke, KEYEXPR);
  z_owned_subscriber_t sub;
  if (z_declare_subscriber(z_loan(s), &sub, z_loan(ke), z_move(callback), NULL) < 0) {
      printf("Unable to declare subscriber.\n");
      return;
  }

  while (1) {
      z_sleep_s(1);
  }

  z_drop(z_move(sub));

  z_drop(z_move(s));
}

void app_task(void *argument)
{
  printf("Waiting for network...\n");

  uint32_t ulNotificationValue = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(5000));

  if (ulNotificationValue == 0) {
      printf("Timed out waiting for network.\n");
  } else {
      printf("Starting Zenoh App...\n");
      app_main();
  }

  vTaskDelete(NULL);
}
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer,
  uint32_t *pulIdleTaskStackSize) {
  static StaticTask_t xIdleTaskTCB;
  static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

  *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
  *ppxIdleTaskStackBuffer = uxIdleTaskStack;
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

BaseType_t xApplicationGetRandomNumber(uint32_t *pulNumber) {
  *pulNumber = (uint32_t)rand();

  return pdTRUE;
}

uint32_t ulApplicationGetNextSequenceNumber(uint32_t /*ulSourceAddress*/, uint16_t /*usSourcePort*/,
           uint32_t /*ulDestinationAddress*/, uint16_t /*usDestinationPort*/) {
  uint32_t ulNext = 0;
  xApplicationGetRandomNumber(&ulNext);

  return ulNext;
}

const char *pcApplicationHostnameHook(void) { return "FreeRTOS-simulator"; }

void vApplicationIPNetworkEventHook_Multi(eIPCallbackEvent_t eNetworkEvent, struct xNetworkEndPoint * /*xEndPoint*/) {
  if (eNetworkEvent == eNetworkUp) {
    printf("Network is up!\n");

    uint32_t ulIPAddress = 0;
    uint32_t ulNetMask = 0;
    uint32_t ulGatewayAddress = 0;
    uint32_t ulDNSServerAddress = 0;
    char cBuf[50];

    // The network is up and configured. Print out the configuration obtained
    // from the DHCP server.
    FreeRTOS_GetEndPointConfiguration(&ulIPAddress, &ulNetMask, &ulGatewayAddress, &ulDNSServerAddress, &xEndPoint);

    // Convert the IP address to a string then print it out.
    FreeRTOS_inet_ntoa(ulIPAddress, cBuf);
    printf("IP Address: %s\n", cBuf);

    // Convert the net mask to a string then print it out.
    FreeRTOS_inet_ntoa(ulNetMask, cBuf);
    printf("Subnet Mask: %s\n", cBuf);

    // Convert the IP address of the gateway to a string then print it out.
    FreeRTOS_inet_ntoa(ulGatewayAddress, cBuf);
    printf("Gateway IP Address: %s\n", cBuf);

    // Convert the IP address of the DNS server to a string then print it out.
    FreeRTOS_inet_ntoa(ulDNSServerAddress, cBuf);
    printf("DNS server IP Address: %s\n", cBuf);

    // Make sure MAC address of the gateway is known
    if (xARPWaitResolution(ulGatewayAddress, pdMS_TO_TICKS(3000)) < 0) {
      xTaskNotifyGive(xAppTaskHandle);
    } else {
      printf("Failed to obtain the MAC address of the gateway!\n");
    }
  } else if (eNetworkEvent == eNetworkDown) {
    printf("IPv4 End Point is down!\n");
  }
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
  MX_ICACHE_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */

  memcpy(ipLOCAL_MAC_ADDRESS, ucMACAddress, sizeof(ucMACAddress));

  pxLibslirp_FillInterfaceDescriptor(0, &xInterface);

  FreeRTOS_FillEndPoint(&xInterface, &xEndPoint, ucIPAddress, ucNetMask, ucGatewayAddress, ucDNSServerAddress,
                        ucMACAddress);
  xEndPoint.bits.bWantDHCP = 1;

  FreeRTOS_IPInit_Multi();

  xAppTaskHandle = xTaskCreateStatic(app_task, "", APP_TASK_STACK_DEPTH, NULL, APP_TASK_PRIORITY, uxAppTaskStack, &xAppTaskTCB);

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* Call init function for freertos objects (in app_freertos.c) */
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLL1_SOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 62;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1_VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1_VCORANGE_WIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 4096;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the programming delay
  */
  __HAL_FLASH_SET_PROGRAM_DELAY(FLASH_PROGRAMMING_DELAY_2);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM12 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM12) {
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
