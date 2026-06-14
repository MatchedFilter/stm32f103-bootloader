#include "main.h"

#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <threads.h>
#include <unistd.h>

static const uint32_t gpio_a = 01;
static const uint32_t gpio_b = 02;
static const uint32_t gpio_c = 03;

const uint32_t *GPIOA = &gpio_a;
const uint32_t *GPIOB = &gpio_b;
const uint32_t *GPIOC = &gpio_c;

extern uint8_t usb_rx_buffer[64];
extern volatile uint32_t usb_rx_len;
extern volatile uint8_t usb_data_ready;

static SysTick_Type sys_tick_stub = {0};
SysTick_Type *SysTick             = &sys_tick_stub;

static NVIC_Type nvic_stub = {0};
NVIC_Type *NVIC            = &nvic_stub;

USBD_HandleTypeDef hUsbDeviceFS;

static void *serial_rx_thread(void *arg);
static int configure_serial_port(const char *port_name);

typedef struct
{
  int fd;
  volatile bool keep_running;
} thread_config_t;

void HAL_Init(void)
{
}

void MX_USB_DEVICE_Init(void)
{
  const char *port_name = "ttyUSB0";

  int fd = configure_serial_port(port_name);
  if (fd < 0)
  {
    fprintf(stderr, "Unable to open serial port: %s\n", port_name);
    exit(127);
  }

  thread_config_t config;
  config.fd           = fd;
  config.keep_running = true;

  pthread_t rx_thread_id;
  if (pthread_create(&rx_thread_id, NULL, serial_rx_thread, &config) != 0)
  {
    fprintf(stderr, "Failed to create receiver thread\n");
    exit(127);
  }
}

void __NOP(void)
{
}

uint8_t CDC_Transmit_FS(uint8_t *Buf, uint16_t Len)
{
  (void) Len;
  return (uint8_t) fprintf(stdout, "%s", (char *) Buf);
}

HAL_StatusTypeDef HAL_FLASH_Unlock(void)
{
  return HAL_OK;
}

HAL_StatusTypeDef HAL_FLASH_Program(uint32_t TypeProgram, uint32_t Address, uint64_t Data)
{
  (void) TypeProgram;
  (void) Address;
  (void) Data;
  return HAL_OK;
}

HAL_StatusTypeDef HAL_FLASH_Lock(void)
{
  return HAL_OK;
}

void HAL_Delay(uint32_t Delay)
{
  struct timespec duration;
  duration.tv_sec  = Delay / 1000U;
  duration.tv_nsec = (Delay % 1000U) * 1000000U;
  thrd_sleep(&duration, NULL);
}

void __HAL_RCC_GPIOA_CLK_ENABLE(void)
{
}

void HAL_GPIO_Init(const uint32_t *GPIOx, GPIO_InitTypeDef *GPIO_Init)
{
  (void) GPIOx;
  (void) GPIO_Init;
}

void HAL_GPIO_WritePin(const uint32_t *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState)
{
  (void) GPIOx;
  (void) GPIO_Pin;
  (void) PinState;
}

void HAL_GPIO_DeInit(const uint32_t *GPIOx, uint32_t GPIO_Pin)
{
  (void) GPIOx;
  (void) GPIO_Pin;
}

USBD_StatusTypeDef USBD_DeInit(USBD_HandleTypeDef *pdev)
{
  (void) pdev;
  return USBD_OK;
}

HAL_StatusTypeDef HAL_RCC_DeInit(void)
{
  return HAL_OK;
}

HAL_StatusTypeDef HAL_DeInit(void)
{
  return HAL_OK;
}

void __set_MSP(uint32_t topOfMainStack)
{
  (void) topOfMainStack;
}

void __disable_irq(void)
{
}

HAL_StatusTypeDef HAL_RCC_OscConfig(RCC_OscInitTypeDef *RCC_OscInitStruct)
{
  (void) RCC_OscInitStruct;
  return HAL_OK;
}

HAL_StatusTypeDef HAL_RCC_ClockConfig(RCC_ClkInitTypeDef *RCC_ClkInitStruct, uint32_t FLatency)
{
  (void) RCC_ClkInitStruct;
  (void) FLatency;
  return HAL_OK;
}

HAL_StatusTypeDef HAL_RCCEx_PeriphCLKConfig(RCC_PeriphCLKInitTypeDef *PeriphClkInit)
{
  (void) PeriphClkInit;
  return HAL_OK;
}

// This function runs entirely in a separate thread
static void *serial_rx_thread(void *arg)
{
  thread_config_t *config = (thread_config_t *) arg;

  printf("[RX Thread] Started listening...\n");

  while (config->keep_running)
  {
    // read() blocks here until at least 1 byte arrives (configured via VMIN/VTIME)
    ssize_t bytes_read = read(config->fd, usb_rx_buffer, sizeof(usb_rx_buffer) - 1);

    if (bytes_read > 0)
    {
      usb_rx_len                = (uint32_t) bytes_read;
      usb_rx_buffer[usb_rx_len] = '\0'; // Null-terminate string safely
      printf("[RX Thread] Received %d bytes: %s\n", usb_rx_len, usb_rx_buffer);
      usb_data_ready = 1U;
    }
  }

  printf("[RX Thread] Exiting...\n");
  return NULL;
}

static int configure_serial_port(const char *port_name)
{
  // Open the serial port in Read/Write mode
  // O_NOCTTY: This program doesn't want to become the port's "controlling terminal"
  int fd = open(port_name, O_RDWR | O_NOCTTY);
  if (fd == -1)
  {
    perror("Unable to open serial port");
    return -1;
  }

  struct termios tty;
  if (tcgetattr(fd, &tty) != 0)
  {
    perror("Error from tcgetattr");
    close(fd);
    return -1;
  }

  // --- 1. Set Baud Rate ---
  cfsetospeed(&tty, B115200);
  cfsetispeed(&tty, B115200);

  // --- 2. Set 8N1 (8 data bits, No parity, 1 stop bit) ---
  tty.c_cflag &= ~PARENB; // Clear parity bit -> No parity
  tty.c_cflag &= ~CSTOPB; // Clear stop bit -> 1 stop bit
  tty.c_cflag &= ~CSIZE;  // Clear the mask for data size
  tty.c_cflag |= CS8;     // Set 8 data bits

  // --- 3. Disable Hardware/Software Flow Control ---
  tty.c_cflag &= ~CRTSCTS;                // No hardware flow control (RTS/CTS)
  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // No software flow control (XON/XOFF)

  // --- 4. Turn off Local & Input Processing (Raw Mode) ---
  // This stops Linux from doing things like echoing characters back, or waiting for a newline (\n)
  tty.c_cflag |= (CLOCAL | CREAD); // Turn on READ & ignore ctrl lines
  tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes

  // --- 5. Set Blocking/Timeout Behavior ---
  tty.c_cc[VMIN]  = 1; // Block until at least 1 byte arrives
  tty.c_cc[VTIME] = 0; // No timeout (wait indefinitely)

  // Apply the configurations immediately
  if (tcsetattr(fd, TCSANOW, &tty) != 0)
  {
    perror("Error from tcsetattr");
    close(fd);
    return -1;
  }

  return fd;
}
