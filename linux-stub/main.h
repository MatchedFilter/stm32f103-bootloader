#ifndef MAIN_H
#define MAIN_H
#include <stdint.h>

#define __IO
#define FLASH_TYPEPROGRAM_HALFWORD                                                                 \
  0x01U                              /*!<Program a half-word (16-bit) at a specified               \
                                        address.*/
#define FLASH_TYPEPROGRAM_WORD 0x02U /*!<Program a word (32-bit) at a specified address.*/
#define FLASH_TYPEPROGRAM_DOUBLEWORD                                                               \
  0x03U /*!<Program a double word (64-bit) at a specified address*/

#define GPIO_SPEED_FREQ_LOW    1
#define RCC_OSCILLATORTYPE_HSE 0x00000001U

#define RCC_HSE_ON              0
#define RCC_HSE_PREDIV_DIV1     0
#define RCC_HSI_ON              0
#define RCC_PLL_ON              0
#define RCC_PLLSOURCE_HSE       0
#define RCC_PLL_MUL6            0
#define RCC_CLOCKTYPE_HCLK      0
#define RCC_CLOCKTYPE_SYSCLK    0
#define RCC_CLOCKTYPE_PCLK1     0
#define RCC_CLOCKTYPE_PCLK2     0
#define RCC_SYSCLKSOURCE_PLLCLK 0
#define RCC_SYSCLK_DIV1         1
#define RCC_HCLK_DIV2           1
#define RCC_HCLK_DIV1           1
#define FLASH_LATENCY_1         1
#define RCC_USBCLKSOURCE_PLL    0

#define RCC_PERIPHCLK_USB 0

#define GPIO_PIN_0   ((uint16_t) 0x0001) /* Pin 0 selected    */
#define GPIO_PIN_1   ((uint16_t) 0x0002) /* Pin 1 selected    */
#define GPIO_PIN_2   ((uint16_t) 0x0004) /* Pin 2 selected    */
#define GPIO_PIN_3   ((uint16_t) 0x0008) /* Pin 3 selected    */
#define GPIO_PIN_4   ((uint16_t) 0x0010) /* Pin 4 selected    */
#define GPIO_PIN_5   ((uint16_t) 0x0020) /* Pin 5 selected    */
#define GPIO_PIN_6   ((uint16_t) 0x0040) /* Pin 6 selected    */
#define GPIO_PIN_7   ((uint16_t) 0x0080) /* Pin 7 selected    */
#define GPIO_PIN_8   ((uint16_t) 0x0100) /* Pin 8 selected    */
#define GPIO_PIN_9   ((uint16_t) 0x0200) /* Pin 9 selected    */
#define GPIO_PIN_10  ((uint16_t) 0x0400) /* Pin 10 selected   */
#define GPIO_PIN_11  ((uint16_t) 0x0800) /* Pin 11 selected   */
#define GPIO_PIN_12  ((uint16_t) 0x1000) /* Pin 12 selected   */
#define GPIO_PIN_13  ((uint16_t) 0x2000) /* Pin 13 selected   */
#define GPIO_PIN_14  ((uint16_t) 0x4000) /* Pin 14 selected   */
#define GPIO_PIN_15  ((uint16_t) 0x8000) /* Pin 15 selected   */
#define GPIO_PIN_All ((uint16_t) 0xFFFF) /* All pins selected */

#define GPIO_MODE_INPUT     0x00000000u     /*!< Input Floating Mode                   */
#define GPIO_MODE_OUTPUT_PP 0x00000001u     /*!< Output Push Pull Mode                 */
#define GPIO_MODE_OUTPUT_OD 0x00000011u     /*!< Output Open Drain Mode                */
#define GPIO_MODE_AF_PP     0x00000002u     /*!< Alternate Function Push Pull Mode     */
#define GPIO_MODE_AF_OD     0x00000012u     /*!< Alternate Function Open Drain Mode    */
#define GPIO_MODE_AF_INPUT  GPIO_MODE_INPUT /*!< Alternate Function Input Mode         */

#define TIMEOUT_LOOP_COUNT 2000000000

/* USB Device handle structure */
typedef struct _USBD_HandleTypeDef
{
  uint8_t id;
  uint32_t dev_config;
  uint32_t dev_default_config;
  uint32_t dev_config_status;
  uint32_t ep0_state;
  uint32_t ep0_data_len;
  uint8_t dev_state;
  uint8_t dev_old_state;
  uint8_t dev_address;
  uint8_t dev_connection_status;
  uint8_t dev_test_mode;
  uint32_t dev_remote_wakeup;

  void *pClassData;
  void *pUserData;
  void *pData;
} USBD_HandleTypeDef;

/* Following USB Device status */
typedef enum
{
  USBD_OK = 0U,
  USBD_BUSY,
  USBD_FAIL,
} USBD_StatusTypeDef;

typedef enum
{
  HAL_OK      = 0x00U,
  HAL_ERROR   = 0x01U,
  HAL_BUSY    = 0x02U,
  HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

typedef struct
{
  uint32_t Pin; /*!< Specifies the GPIO pins to be configured.
                     This parameter can be any value of @ref GPIO_pins_define */

  uint32_t Mode; /*!< Specifies the operating mode for the selected pins.
                      This parameter can be a value of @ref GPIO_mode_define */

  uint32_t Pull; /*!< Specifies the Pull-up or Pull-Down activation for the selected pins.
                      This parameter can be a value of @ref GPIO_pull_define */

  uint32_t Speed; /*!< Specifies the speed for the selected pins.
                       This parameter can be a value of @ref GPIO_speed_define */
} GPIO_InitTypeDef;

typedef enum
{
  GPIO_PIN_RESET = 0u,
  GPIO_PIN_SET
} GPIO_PinState;

typedef struct
{
  uint32_t ClockType;
  uint32_t SYSCLKSource;
  uint32_t AHBCLKDivider;
  uint32_t APB1CLKDivider;
  uint32_t APB2CLKDivider;
} RCC_ClkInitTypeDef;

typedef struct
{
  uint32_t PeriphClockSelection;
  uint32_t RTCClockSelection;
  uint32_t AdcClockSelection;
  uint32_t I2s2ClockSelection;
  uint32_t I2s3ClockSelection;
  uint32_t UsbClockSelection;
} RCC_PeriphCLKInitTypeDef;

typedef struct
{
  uint32_t ISER[16U]; /*!< Offset: 0x000 (R/W)  Interrupt Set Enable Register */
  uint32_t RESERVED0[16U];
  uint32_t ICER[16U]; /*!< Offset: 0x080 (R/W)  Interrupt Clear Enable Register */
  uint32_t RSERVED1[16U];
  uint32_t ISPR[16U]; /*!< Offset: 0x100 (R/W)  Interrupt Set Pending Register */
  uint32_t RESERVED2[16U];
  uint32_t ICPR[16U]; /*!< Offset: 0x180 (R/W)  Interrupt Clear Pending Register */
  uint32_t RESERVED3[16U];
  uint32_t IABR[16U]; /*!< Offset: 0x200 (R/W)  Interrupt Active bit Register */
  uint32_t RESERVED4[16U];
  uint32_t ITNS[16U]; /*!< Offset: 0x280 (R/W)  Interrupt Non-Secure State Register */
  uint32_t RESERVED5[16U];
  uint32_t IPR[124U]; /*!< Offset: 0x300 (R/W)  Interrupt Priority Register */
} NVIC_Type;

typedef struct
{
  uint32_t CTRL;
  uint32_t LOAD;
  uint32_t VAL;
  uint32_t CALIB;
} SysTick_Type;

/**
 * @brief  RCC PLL configuration structure definition
 */
typedef struct
{
  uint32_t PLLState;
  uint32_t PLLSource;
  uint32_t PLLMUL;
} RCC_PLLInitTypeDef;

typedef struct
{
  uint32_t PLL2State; /*!< The new state of the PLL2.
                          This parameter can be a value of @ref RCCEx_PLL2_Config */

  uint32_t PLL2MUL; /*!< PLL2MUL: Multiplication factor for PLL2 VCO input clock
                      This parameter must be a value of @ref RCCEx_PLL2_Multiplication_Factor*/

  uint32_t HSEPrediv2Value; /*!<  The Prediv2 factor value.
                                 This parameter can be a value of @ref RCCEx_Prediv2_Factor */

} RCC_PLL2InitTypeDef;

typedef struct
{
  uint32_t OscillatorType;
  uint32_t Prediv1Source;
  uint32_t HSEState;
  uint32_t HSEPredivValue;
  uint32_t LSEState;
  uint32_t HSIState;
  uint32_t HSICalibrationValue;
  uint32_t LSIState;
  RCC_PLLInitTypeDef PLL;
  RCC_PLL2InitTypeDef PLL2;
} RCC_OscInitTypeDef;

extern void HAL_Init(void);
extern void MX_USB_DEVICE_Init(void);
extern void __NOP(void);
extern uint8_t CDC_Transmit_FS(uint8_t *Buf, uint16_t Len);

extern HAL_StatusTypeDef HAL_FLASH_Unlock(void);
extern HAL_StatusTypeDef HAL_FLASH_Program(uint32_t TypeProgram, uint32_t Address, uint64_t Data);
extern HAL_StatusTypeDef HAL_FLASH_Lock(void);
extern void HAL_Delay(uint32_t Delay);
extern void __HAL_RCC_GPIOA_CLK_ENABLE(void);
extern void HAL_GPIO_Init(const uint32_t *GPIOx, GPIO_InitTypeDef *GPIO_Init);
extern void HAL_GPIO_WritePin(const uint32_t *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
extern void HAL_GPIO_DeInit(const uint32_t *GPIOx, uint32_t GPIO_Pin);
extern USBD_StatusTypeDef USBD_DeInit(USBD_HandleTypeDef *pdev);
extern HAL_StatusTypeDef HAL_RCC_DeInit(void);
extern HAL_StatusTypeDef HAL_DeInit(void);
extern void __set_MSP(uint32_t topOfMainStack);
extern void __disable_irq(void);
extern HAL_StatusTypeDef HAL_RCC_OscConfig(RCC_OscInitTypeDef *RCC_OscInitStruct);
extern HAL_StatusTypeDef HAL_RCC_ClockConfig(RCC_ClkInitTypeDef *RCC_ClkInitStruct,
                                             uint32_t FLatency);
extern HAL_StatusTypeDef HAL_RCCEx_PeriphCLKConfig(RCC_PeriphCLKInitTypeDef *PeriphClkInit);

extern const uint32_t *GPIOA;
extern const uint32_t *GPIOB;
extern const uint32_t *GPIOC;

extern SysTick_Type *SysTick;
extern NVIC_Type *NVIC;

#endif /* MAIN_H */
