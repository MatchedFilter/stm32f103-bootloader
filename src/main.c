#include "main.h"

#include "usb_device.h"
#include "usbd_cdc_if.h"

#include <string.h>

void SystemClock_Config(void);
void Boot_Application(void);
void Force_USB_Reenumeration(void);

// Shared variables for USB interrupt handling
extern USBD_HandleTypeDef hUsbDeviceFS;
uint8_t usb_rx_buffer[64]       = {0};
volatile uint32_t usb_rx_len    = 0;
volatile uint8_t usb_data_ready = 0;

#define JUMP_ADDRESS 0x08002000
#define MAGIC_WORD "UPDATE"
#define MAGIC_WORD_LEN 6
#define TIMEOUT_LOOP 2000000 // Software timeout loop counter approx ~200ms

int main(void)
{
  // 1. Force the host PC to re-enumerate the USB device before anything else
  Force_USB_Reenumeration();

  HAL_Init();
  SystemClock_Config();

  // 2. Initialize the USB Virtual Com Port Stack
  MX_USB_DEVICE_Init();

  // 3. Wait for the magic word with a timeout window
  uint32_t timeout    = TIMEOUT_LOOP;
  uint8_t update_mode = 0;

  while (timeout > 0)
  {
    if (usb_data_ready)
    {
      usb_data_ready = 0;
      if (usb_rx_len >= MAGIC_WORD_LEN && memcmp(usb_rx_buffer, MAGIC_WORD, MAGIC_WORD_LEN) == 0)
      {
        update_mode = 1;
        break;
      }
    }
    timeout--;
    __NOP(); // No-operation to prevent compiler optimizing the empty loop away
  }

  if (update_mode)
  {
    // Match! Stay in bootloader loop to accept binary chunks via USB CDC
    uint8_t ack[] = "BOOTLOADER_READY\r\n";
    CDC_Transmit_FS(ack, strlen((char *) ack));

    while (1)
    {
      if (usb_data_ready)
      {
        usb_data_ready = 0;
        // Parse your hex/binary application chunks here
        // and write them to flash starting at 0x08002000
      }
    }
  }
  else
  {
    // Timeout -> Jump straight to closed-up Incubator App
    Boot_Application();
  }
}

void Force_USB_Reenumeration(void)
{
  // Explicitly pull PA12 (USB D+) low manually to mimic a physical disconnect
  __HAL_RCC_GPIOA_CLK_ENABLE();
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin              = GPIO_PIN_12;
  GPIO_InitStruct.Mode             = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed            = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
  for (volatile uint32_t i = 0; i < 100000; i++)
    ; // Short delay block

  // De-initialize pin so the native USB peripheral can take full structural
  // control over it
  HAL_GPIO_DeInit(GPIOA, GPIO_PIN_12);
}

void Boot_Application(void)
{
  // Clean down USB controller completely before switching vectors
  USBD_DeInit(&hUsbDeviceFS);
  HAL_RCC_DeInit();
  HAL_DeInit();

  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL  = 0;

  for (int i = 0; i < 8; i++)
  {
    NVIC->ICER[i] = 0xFFFFFFFF;
    NVIC->ICPR[i] = 0xFFFFFFFF;
  }

  typedef void (*pFunction)(void);
  uint32_t JumpAddress        = *(__IO uint32_t *) (JUMP_ADDRESS + 4);
  pFunction JumpToApplication = (pFunction) JumpAddress;

  __set_MSP(*(__IO uint32_t *) JUMP_ADDRESS);
  JumpToApplication();
}

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

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct   = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct   = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState       = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL6;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType =
    RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection    = RCC_USBCLKSOURCE_PLL;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}
