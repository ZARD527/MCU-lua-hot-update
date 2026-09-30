#include "include.h"
#include "Config.h"


#define UART_H_RX_BUFFER_SIZE  512U


static uint8_t UART_H_RxRingBuffer[UART_H_RX_BUFFER_SIZE];
static volatile uint16_t UART_H_RxRingHead;
static volatile uint16_t UART_H_RxRingTail;
static volatile uint16_t UART_H_RxRingCount;


void UART_H_RingBufferClear(void) {
    UART_H_RxRingHead = 0U;
    UART_H_RxRingTail = 0U;
    UART_H_RxRingCount = 0U;
}


uint8_t UART_H_RingBufferIsEmpty(void) {
    return (UART_H_RxRingCount == 0U) ? 1U : 0U;
}


uint8_t UART_H_RingBufferIsFull(void) {
    return (UART_H_RxRingCount >= UART_H_RX_BUFFER_SIZE) ? 1U : 0U;
}


uint16_t UART_H_RingBufferSize(void) {
    return UART_H_RxRingCount;
}


void UART_H_RingBufferInit(void) {
    UART_H_RingBufferClear();

    for (uint16_t i = 0; i < UART_H_RX_BUFFER_SIZE; i++) {
        UART_H_RxRingBuffer[i] = 0U;
    }
}


uint8_t UART_H_RingBufferPut(uint8_t data) {
    if (UART_H_RingBufferIsFull()) {
        return 0U;
    }

    UART_H_RxRingBuffer[UART_H_RxRingHead] = data;
    UART_H_RxRingHead = (UART_H_RxRingHead + 1U) % UART_H_RX_BUFFER_SIZE;
    UART_H_RxRingCount++;

    return 1U;
}


uint8_t UART_H_RingBufferGet(uint8_t *data) {
    if (!data || UART_H_RingBufferIsEmpty()) {
        return 0U;
    }

    *data = UART_H_RxRingBuffer[UART_H_RxRingTail];
    UART_H_RxRingTail = (UART_H_RxRingTail + 1U) % UART_H_RX_BUFFER_SIZE;
    UART_H_RxRingCount--;

    return 1U;
}


uint8_t UART_H_RingBufferPeek(uint16_t len, uint8_t *data) {
    uint16_t tail;

    if (!data || UART_H_RingBufferIsEmpty() || UART_H_RxRingCount < len) {
        return 0U;
    }

    tail = UART_H_RxRingTail;

    for (uint16_t i = 0; i < len; i++) {
        data[i] = UART_H_RxRingBuffer[tail];
        tail = (tail + 1U) % UART_H_RX_BUFFER_SIZE;
    }

    data[len] = '\0';

    return 1U;
}


void UART_H_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(LOG_UART_GPIO_CLK, ENABLE);

    GPIO_PinAFConfig(LOG_UART_GPIO, LOG_UART_TX_SOURCE, LOG_UART_GPIO_AF);
    GPIO_PinAFConfig(LOG_UART_GPIO, LOG_UART_RX_SOURCE, LOG_UART_GPIO_AF);

    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = LOG_UART_TX_PIN | LOG_UART_RX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LOG_UART_GPIO, &GPIO_InitStructure);
}


void UART_H_NVIC_Init(void) {
    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}


void UART_H_Init(void) {
    USART_InitTypeDef USART_InitStructure;

    UART_H_RingBufferInit();
    UART_H_GPIO_Init();

    RCC_APB2PeriphClockCmd(LOG_UART_CLK, ENABLE);

    USART_StructInit(&USART_InitStructure);
    USART_InitStructure.USART_BaudRate = LOG_UART_BAUDRATE;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(LOG_UART, &USART_InitStructure);

    USART_ITConfig(LOG_UART, USART_IT_RXNE, ENABLE);
    UART_H_NVIC_Init();

    USART_Cmd(LOG_UART, ENABLE);
}


void UART_H_SendByte(uint8_t data) {
    while (USART_GetFlagStatus(LOG_UART, USART_FLAG_TXE) == RESET) {
    }

    USART_SendData(LOG_UART, (uint16_t)data);
}


void UART_H_SendString(const char *str, uint16_t len) {
    if (!str) {
        return;
    }

    if (!len) {
        while (*str != '\0') {
            UART_H_SendByte((uint8_t)*str);
            str++;
        }
    }
    else {
        for (uint16_t i = 0; i < len; i++) {
            UART_H_SendByte((uint8_t)str[i]);
        }
    }
}


uint8_t UART_H_ReceiveByte(uint8_t *data) {
    if (!data) {
        return 0U;
    }

    if (USART_GetFlagStatus(LOG_UART, USART_FLAG_RXNE) == RESET) {
        return 0U;
    }

    *data = (uint8_t)USART_ReceiveData(LOG_UART);

    return 1U;
}


void UART_H_IRQHandler(void) {
    uint8_t data;

    if (USART_GetITStatus(LOG_UART, USART_IT_RXNE) != RESET) {
        data = (uint8_t)USART_ReceiveData(LOG_UART);

        (void)UART_H_RingBufferPut(data);
        (void)SoftUART_RingBufferPut(data);

        USART_ClearITPendingBit(LOG_UART, USART_IT_RXNE);
    }
}
