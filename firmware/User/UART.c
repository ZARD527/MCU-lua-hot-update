#include "Config.h"
#include "include.h"

static uint32_t SoftUART_BitCycles;

static void SoftUART_DWTInit(void);
static void SoftUART_DelayCycles(uint32_t cycles);
static uint32_t SoftUART_ElapsedCycles(uint32_t start);

void SoftUART_Init(void) {
  GPIO_InitTypeDef GPIO_InitStructure;

  SoftUART_DWTInit();
  SoftUART_BitCycles = SystemCoreClock / SOFT_UART_BAUDRATE;

  RCC_AHB1PeriphClockCmd(SOFT_UART_GPIO_CLK, ENABLE);

  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = SOFT_UART_TX_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(SOFT_UART_TX_GPIO, &GPIO_InitStructure);

  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = SOFT_UART_RX_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(SOFT_UART_RX_GPIO, &GPIO_InitStructure);

  GPIO_SetBits(SOFT_UART_TX_GPIO, SOFT_UART_TX_PIN);
}

void SoftUART_SendByte(uint8_t data) {
  uint8_t i;
  uint32_t primask = __get_PRIMASK();

  __disable_irq();

  GPIO_ResetBits(SOFT_UART_TX_GPIO, SOFT_UART_TX_PIN);
  SoftUART_DelayCycles(SoftUART_BitCycles);

  for (i = 0U; i < 8U; i++) {
    if ((data & 0x01U) != 0U) {
      GPIO_SetBits(SOFT_UART_TX_GPIO, SOFT_UART_TX_PIN);
    } else {
      GPIO_ResetBits(SOFT_UART_TX_GPIO, SOFT_UART_TX_PIN);
    }

    data >>= 1U;
    SoftUART_DelayCycles(SoftUART_BitCycles);
  }

  GPIO_SetBits(SOFT_UART_TX_GPIO, SOFT_UART_TX_PIN);
  SoftUART_DelayCycles(SoftUART_BitCycles);

  __set_PRIMASK(primask);
}

void SoftUART_SendString(const char *str) {
  while ((str != 0) && (*str != '\0')) {
    SoftUART_SendByte((uint8_t)*str);
    str++;
  }
}

uint8_t SoftUART_ReceiveByte(uint8_t *data, uint32_t timeoutMs) {
  uint8_t i;
  uint8_t value = 0U;
  uint32_t primask;
  uint32_t start;
  uint32_t timeoutCycles;

  if (data == 0) {
    return 0U;
  }

  start = DWT->CYCCNT;
  timeoutCycles = (SystemCoreClock / 1000U) * timeoutMs;

  while (GPIO_ReadInputDataBit(SOFT_UART_RX_GPIO, SOFT_UART_RX_PIN) != Bit_RESET) {
    if ((timeoutMs != 0U) && (SoftUART_ElapsedCycles(start) >= timeoutCycles)) {
      return 0U;
    }
  }

  primask = __get_PRIMASK();
  __disable_irq();

  SoftUART_DelayCycles(SoftUART_BitCycles / 2U);
  if (GPIO_ReadInputDataBit(SOFT_UART_RX_GPIO, SOFT_UART_RX_PIN) != Bit_RESET) {
    __set_PRIMASK(primask);
    return 0U;
  }

  SoftUART_DelayCycles(SoftUART_BitCycles);

  for (i = 0U; i < 8U; i++) {
    if (GPIO_ReadInputDataBit(SOFT_UART_RX_GPIO, SOFT_UART_RX_PIN) != Bit_RESET) {
      value |= (uint8_t)(1U << i);
    }
    SoftUART_DelayCycles(SoftUART_BitCycles);
  }

  if (GPIO_ReadInputDataBit(SOFT_UART_RX_GPIO, SOFT_UART_RX_PIN) == Bit_RESET) {
    __set_PRIMASK(primask);
    return 0U;
  }

  __set_PRIMASK(primask);

  *data = value;
  return 1U;
}

static void SoftUART_DWTInit(void) {
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0U;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void SoftUART_DelayCycles(uint32_t cycles) {
  uint32_t start = DWT->CYCCNT;

  while ((DWT->CYCCNT - start) < cycles) {
  }
}

static uint32_t SoftUART_ElapsedCycles(uint32_t start) {
  return DWT->CYCCNT - start;
}
