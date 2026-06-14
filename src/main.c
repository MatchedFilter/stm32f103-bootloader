#include "main.h"

#include "usb_device.h"
#include "usbd_cdc_if.h"

#include <stdlib.h>
#include <string.h>

typedef void (*pFunction)(void);

void SystemClock_Config(void);
void Boot_Application(void);
void Force_USB_Reenumeration(void);

// Helper functions for parsing SREC lines
static uint8_t parse_hex_nibble(char c);
static uint8_t parse_hex_byte(const char *ptr);
static void process_line(char *line);
static void process_srec_line(char *line);

static void blocking_usb_transmit(uint8_t *buffer, uint16_t length);

// Shared variables for USB interrupt handling
extern USBD_HandleTypeDef hUsbDeviceFS;
uint8_t usb_rx_buffer[64]       = {0};
volatile uint32_t usb_rx_len    = 0;
volatile uint8_t usb_data_ready = 0;

#define JUMP_ADDRESS   0x08002000
#define MAGIC_WORD     "enter bootloader"
#define MAGIC_WORD_LEN 16

// Streaming character line buffer configuration
#define SREC_MAX_LINE_LEN 128
char line_buffer[SREC_MAX_LINE_LEN];
uint16_t line_index = 0;

int main(void)
{
  // 1. Force the host PC to re-enumerate the USB device before anything else
  Force_USB_Reenumeration();

  HAL_Init();
  SystemClock_Config();

  // 2. Initialize the USB Virtual Com Port Stack
  MX_USB_DEVICE_Init();

  // 3. Announce boot sequence tracking immediately for the Python app phase 1
  uint8_t booting_message[] = "booting\n";
  blocking_usb_transmit(booting_message, strlen((char *) booting_message));

  // 4. Wait for the magic word with a timeout window
  uint32_t timeout    = TIMEOUT_LOOP_COUNT;
  uint8_t update_mode = 0;

  while (timeout > 0)
  {
    if (usb_data_ready)
    {
      usb_data_ready = 0;

      // If Python sends a reset command during this window instead, process it
      if (usb_rx_len >= 5 && memcmp(usb_rx_buffer, "reset", 5) == 0)
      {
        static uint8_t reset_ack[] = "reset ack\n";
        blocking_usb_transmit(reset_ack, (uint16_t) strlen((char *) reset_ack));
        HAL_Delay(100); // Small cushion to allow USB transmission to finish
        NVIC_SystemReset();
      }

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
    // Match! Notify host machine that bootloader is accepting streaming SREC files
    uint8_t ack[] = "enter bootloader ack\n";
    blocking_usb_transmit(ack, strlen((char *) ack));

    // Unlock internal microcontroller flash memory storage sectors once globally
    HAL_FLASH_Unlock();

    while (1)
    {
      if (usb_data_ready)
      {
        usb_data_ready = 0;

        // Loop over individual streaming characters inside the incoming USB chunk
        for (uint32_t i = 0; i < usb_rx_len; i++)
        {
          char incoming_char = (char) usb_rx_buffer[i];

          // Look for line termination carriage boundaries
          if (incoming_char == '\n' || incoming_char == '\r')
          {
            if (line_index > 0)
            {
              line_buffer[line_index] = '\0'; // Null-terminate string array
              process_line(line_buffer);      // Parse line block
              line_index = 0;                 // Reset line processing tracker
            }
          }
          else if (line_index < (SREC_MAX_LINE_LEN - 1))
          {
            line_buffer[line_index++] = incoming_char;
          }
        }
      }
    }
  }
  else
  {
    // Timeout -> Jump straight to application
    Boot_Application();
  }
}

// Low-level helper parsing utilities
static uint8_t parse_hex_nibble(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  return 0;
}

static uint8_t parse_hex_byte(const char *ptr)
{
  return (parse_hex_nibble(ptr[0]) << 4) | parse_hex_nibble(ptr[1]);
}

static void process_line(char *line)
{
  // Note: Newline characters are stripped by main's parser loop before hitting here
  if (strcmp(line, "reset") == 0)
  {
    static uint8_t reset_ack[] = "reset ack\n";
    blocking_usb_transmit(reset_ack, (uint16_t) strlen((char *) reset_ack));
    HAL_Delay(100);
    NVIC_SystemReset();
  }
  else if (strcmp(line, "enter bootloader") == 0)
  {
    static uint8_t enter_bootloader_ack[] = "enter bootloader ack\n";
    blocking_usb_transmit(enter_bootloader_ack, (uint16_t) strlen((char *) enter_bootloader_ack));
  }
  else
  {
    process_srec_line(line);
  }
}

static void process_srec_line(char *line)
{
  // Verify line frame validity
  if (line[0] != 'S')
    return;

  char record_type = line[1];

  // Unpack Byte Count parameter
  uint8_t byte_count = parse_hex_byte(&line[2]);

  // We track running checksum calculation validation
  uint32_t running_checksum = byte_count;

  // Track scanning references inside our string text indices
  uint16_t ptr_idx = 4;

  // ---------------------------------------------------------------------------
  // CASE A: DATA RECORDS (S1, S2, S3)
  // ---------------------------------------------------------------------------
  if (record_type == '1' || record_type == '2' || record_type == '3')
  {
    uint8_t addr_bytes      = (record_type == '1') ? 2 : ((record_type == '2') ? 3 : 4);
    uint32_t target_address = 0;

    // Decode target memory destination address bytes
    for (uint8_t i = 0; i < addr_bytes; i++)
    {
      uint8_t b         = parse_hex_byte(&line[ptr_idx]);
      target_address    = (target_address << 8) | b;
      running_checksum += b;
      ptr_idx          += 2;
    }

    // Safety guard filter to ensure we are only writing inside designated app space boundaries
    if (target_address < JUMP_ADDRESS)
    {
      uint8_t error_tick[] = "?"; // Send error flag for bad address mapping
      blocking_usb_transmit(error_tick, 1);
      return;
    }

    // Unpack data chunks and commit them down to flash storage
    uint8_t data_bytes_count = byte_count - addr_bytes - 1;

    for (uint8_t i = 0; i < data_bytes_count; i += 2)
    {
      uint8_t byte1     = parse_hex_byte(&line[ptr_idx]);
      running_checksum += byte1;
      ptr_idx          += 2;

      uint8_t byte2 = 0xFF; // Pad if odd data length
      if (i + 1 < data_bytes_count)
      {
        byte2             = parse_hex_byte(&line[ptr_idx]);
        running_checksum += byte2;
        ptr_idx          += 2;
      }

      uint16_t flash_halfword = (byte2 << 8) | byte1;

      // Commit halfword to flash memory address location
      HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, target_address + i, flash_halfword);
    }

    // Send confirmation tick for data line processing completion
    uint8_t tick[] = ".";
    blocking_usb_transmit(tick, 1);
  }
  // ---------------------------------------------------------------------------
  // CASE B: HEADER RECORD (S0)
  // ---------------------------------------------------------------------------
  else if (record_type == '0')
  {
    // Acknowledge the metadata line immediately so Python can safely send the next line
    uint8_t tick[] = ".";
    blocking_usb_transmit(tick, 1);
  }
  // ---------------------------------------------------------------------------
  // CASE C: TERMINATION RECORDS (S7, S8, S9)
  // ---------------------------------------------------------------------------
  else if (record_type == '7' || record_type == '8' || record_type == '9')
  {
    // First, send the final tick so Python knows the line was consumed safely
    uint8_t tick[] = ".";
    blocking_usb_transmit(tick, 1);

    // Lock down memory and transmit the final completion flag string
    HAL_FLASH_Lock();
    uint8_t complete_msg[] = "FLASH_SUCCESS\n";
    blocking_usb_transmit(complete_msg, strlen((char *) complete_msg));

    HAL_Delay(500); // Give the host machine time to read the buffer before jumping
    Boot_Application();
  }
}

void Force_USB_Reenumeration(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin              = GPIO_PIN_12;
  GPIO_InitStruct.Mode             = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed            = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
  for (volatile uint32_t i = 0; i < 100000; i++)
    ;

  HAL_GPIO_DeInit(GPIOA, GPIO_PIN_12);
}

void Boot_Application(void)
{
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

  uint32_t JumpAddress        = *(__IO uint32_t *) (JUMP_ADDRESS + 4);
  pFunction JumpToApplication = (pFunction) JumpAddress;

  __set_MSP(*(__IO uint32_t *) JUMP_ADDRESS);
  JumpToApplication();
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct   = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct   = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

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

static void blocking_usb_transmit(uint8_t *buffer, uint16_t length)
{
  extern USBD_HandleTypeDef hUsbDeviceFS;
  USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *) hUsbDeviceFS.pClassData;

  // Wait until the prior packet transmission state drops to 0 (Idle)
  while (hcdc->TxState != 0)
  {
  }

  CDC_Transmit_FS(buffer, length);
}
