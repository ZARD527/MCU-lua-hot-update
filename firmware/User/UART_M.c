#include "Config.h"
#include "include.h"


static volatile uint8_t Rx_Data;
static volatile uint32_t BitTicks;
static volatile uint8_t Rx_BitIndex;
static volatile UART_RX_State Rx_State;

uint8_t str[521];
uint8_t SoftUART_RxState = 0;
static uint32_t SoftUART_BitCycles;
static uint32_t SoftUART_TxCycles;
static volatile uint16_t UART_M_Push_Id;
static uint8_t SoftUART_RxRingBuffer[512];
static volatile uint16_t SoftUART_RxRingHead;
static volatile uint16_t SoftUART_RxRingTail;
static volatile uint16_t SoftUART_RxRingCount;


 







void SoftUART_RingBufferClear(void) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    SoftUART_RxRingHead = 0;
    SoftUART_RxRingTail = 0;
    SoftUART_RxRingCount = 0;
    __set_PRIMASK(primask);
}


 





uint8_t SoftUART_RingBufferIsEmpty(void) {
    return (SoftUART_RingBufferSize() == 0U) ? 1U : 0U;
}


 









uint8_t SoftUART_RingBufferIsFull(void) {
    return (SoftUART_RingBufferSize() >= 512U) ? 1U : 0U;
}


 





uint16_t SoftUART_RingBufferSize(void) {
    uint32_t primask = __get_PRIMASK();
    uint16_t count;

    __disable_irq();
    count = SoftUART_RxRingCount;
    __set_PRIMASK(primask);
    return count;
}


 







void SoftUART_RingBufferInit(void) {
    SoftUART_RxRingHead = 0;
    SoftUART_RxRingTail = 0;
    SoftUART_RxRingCount = 0;
    for (uint16_t i = 0; i < 512; i++) {
        SoftUART_RxRingBuffer[i] = 0;
        str[i] = 0;
    }
}


 











uint8_t SoftUART_RingBufferPut(uint8_t data) {
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    if (SoftUART_RxRingCount >= 512U) {
        __set_PRIMASK(primask);
        return 0U;
    }
    SoftUART_RxRingBuffer[SoftUART_RxRingHead] = data;
    SoftUART_RxRingHead = (SoftUART_RxRingHead + 1) % 512;
    SoftUART_RxRingCount++;
    __set_PRIMASK(primask);
    return 1U;
}


 










uint8_t SoftUART_RingBufferGet(uint8_t *data) {
    uint32_t primask;

    if (!data) {
        return 0U;
    }

    primask = __get_PRIMASK();
    __disable_irq();
    if (SoftUART_RxRingCount == 0U) {
        __set_PRIMASK(primask);
        return 0U;
    }

    *data = SoftUART_RxRingBuffer[SoftUART_RxRingTail];
    SoftUART_RxRingTail = (SoftUART_RxRingTail + 1) % 512;
    SoftUART_RxRingCount--;
    __set_PRIMASK(primask);
    return 1U;
    
    
}


 












uint8_t SoftUART_RingBufferPeek(uint16_t len, uint8_t *data) {
    uint32_t primask;
    uint16_t tail;
    uint16_t i;

    if (!data) {
        return 0U;
    }

    primask = __get_PRIMASK();
    __disable_irq();
    if (SoftUART_RxRingCount < len) {
        __set_PRIMASK(primask);
        return 0U;
    }

    tail = SoftUART_RxRingTail;

    for (i = 0U; i < len; i++) {
        data[i] = SoftUART_RxRingBuffer[tail];
        tail = (tail + 1) % 512;
    }

    data[len] = '\0';
    __set_PRIMASK(primask);

    return 1U;
}


 







static void UART_M_DWTInit(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}


 









static void SoftUART_DelayCycles(uint32_t tick) {
    uint32_t temp = DWT->CYCCNT;
    while ((DWT->CYCCNT - temp) < tick) {
        __NOP();
    }
}


 









static void SoftUART_Start(uint32_t tick) {
    TIM_SetCounter(SOFT_UART_TIM, 0);
    TIM_SetAutoreload(SOFT_UART_TIM, (tick - 1));
    TIM_ClearITPendingBit(SOFT_UART_TIM, TIM_IT_Update);
    TIM_Cmd(SOFT_UART_TIM, ENABLE);
}


 







static void SoftUART_Stop(void) {
    TIM_Cmd(SOFT_UART_TIM, DISABLE);
    TIM_SetCounter(SOFT_UART_TIM, 0);
    TIM_ClearITPendingBit(SOFT_UART_TIM, TIM_IT_Update);
}


 









static uint32_t UART_M_GetTIM6Clock(void)
{
    uint32_t pclk1 = SystemCoreClock;
    uint32_t ppre1 = RCC->CFGR & RCC_CFGR_PPRE1;

    if (ppre1 >= RCC_CFGR_PPRE1_DIV2) {
        pclk1 /= (1U << (((ppre1 >> 10) & 0x7U) - 3U));
        return pclk1 * 2U;
    }

    return pclk1;
}


 







static void SoftUART_TIMInit(void) {
    TIM_TimeBaseInitTypeDef TIM_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(SOFT_UART_TIM_RCC, ENABLE);

    TIM_InitStructure.TIM_Prescaler = 0;
    TIM_InitStructure.TIM_Period = 0xFFFF;
    TIM_InitStructure.TIM_RepetitionCounter = 0;
    TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(SOFT_UART_TIM, &TIM_InitStructure);

    TIM_ClearFlag(SOFT_UART_TIM, TIM_FLAG_Update);
    TIM_ITConfig(SOFT_UART_TIM, TIM_IT_Update, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannel = SOFT_UART_TIM_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_Init(&NVIC_InitStructure);

}


 







void UART_M_EXTIInit(void) {
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOB, EXTI_PinSource7);

    EXTI_InitStructure.EXTI_Line = EXTI_Line7;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  
    EXTI_ClearITPendingBit(EXTI_Line7);
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}


 







void UART_M_GPIO_Init (void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(SOFT_UART_GPIO_CLK, ENABLE);

    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = SOFT_UART_TX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SOFT_UART_TX_GPIO, &GPIO_InitStructure);

    GPIO_SetBits(SOFT_UART_TX_GPIO, SOFT_UART_TX_PIN);

    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = SOFT_UART_RX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SOFT_UART_RX_GPIO, &GPIO_InitStructure);
}


 








void UART_M_Init(void) {

    Rx_Data = 0U;
    BitTicks = 0U;
    Rx_BitIndex = 0U;
    UART_M_Push_Id = 0;
    Rx_State = UART_RX_IDLE;

    
    SoftUART_BitCycles = (UART_M_GetTIM6Clock() + SOFT_UART_BAUDRATE / 2U) / SOFT_UART_BAUDRATE;
    SoftUART_TxCycles = (SystemCoreClock + SOFT_UART_BAUDRATE / 2U) / SOFT_UART_BAUDRATE;

    UART_M_DWTInit();
    UART_M_GPIO_Init();
    SoftUART_TIMInit();
    SoftUART_RingBufferInit();
    EXTI_ClearITPendingBit(EXTI_Line7);
    UART_M_EXTIInit();

}


 










void UART_M_SendByte(uint8_t data) {
    uint32_t primask = __get_PRIMASK();

    __disable_irq();

    GPIO_ResetBits(SOFT_UART_TX_GPIO, SOFT_UART_TX_PIN);
    SoftUART_DelayCycles(SoftUART_TxCycles);

    for (uint8_t i = 0; i < 8; i++) {
        if ((data & 0x01)) {
            GPIO_SetBits(SOFT_UART_TX_GPIO, SOFT_UART_TX_PIN);
        }
        else {
            GPIO_ResetBits(SOFT_UART_TX_GPIO, SOFT_UART_TX_PIN);
        }
        data >>= 1;
        SoftUART_DelayCycles(SoftUART_TxCycles);
    }

    GPIO_SetBits(SOFT_UART_TX_GPIO, SOFT_UART_TX_PIN);
    SoftUART_DelayCycles(SoftUART_TxCycles);

    __set_PRIMASK(primask);
}


 










void UART_M_SendString (const char *str, uint16_t len) {
    if (!len) {
        while (str && *str != '\0') {
            UART_M_SendByte((uint8_t)*str);
            str++;
        }
    }
    else {
        for (int i = 0; i < len; i++) {
            UART_M_SendByte(str[i]);
        }
    }
}


 











uint8_t UART_M_ReceiveByte(uint8_t *data) {

    if (!data) {
        return 0;
    }
    
    uint32_t primask;
    uint8_t value = 0U;

    primask = __get_PRIMASK();
    __disable_irq();
    
    SoftUART_DelayCycles(SoftUART_BitCycles / 2U);
    if (GPIO_ReadInputDataBit(SOFT_UART_RX_GPIO, SOFT_UART_RX_PIN)) {
        __set_PRIMASK(primask);
        return 0;
    }

    SoftUART_DelayCycles(SoftUART_BitCycles);

    for (uint8_t i = 0 ; i < 8U; i++) {
        if (GPIO_ReadInputDataBit(SOFT_UART_RX_GPIO, SOFT_UART_RX_PIN)) {
            value |= (uint8_t)(1U << i);
        }
        SoftUART_DelayCycles(SoftUART_BitCycles);
    }

    if (!(GPIO_ReadInputDataBit(SOFT_UART_RX_GPIO, SOFT_UART_RX_PIN))) {
        __set_PRIMASK(primask);
        return 0;
    }

    __set_PRIMASK(primask);
    *data = value;

    return 1;

}


 







void UART_M_ReceiveByte_TIM (void) {

    if ((GPIO_ReadInputDataBit(SOFT_UART_RX_GPIO, SOFT_UART_RX_PIN))) {
        Rx_Data |= (uint8_t)(1U << Rx_BitIndex);
    }

    Rx_BitIndex++;

    if (Rx_BitIndex >= 8U) {
        Rx_State = UART_RX_STOP;
        TIM_SetAutoreload(SOFT_UART_TIM, SoftUART_BitCycles - 1);
    }

}


 








void EXTI9_5_IRQHandler(void) {
    if (EXTI_GetITStatus(EXTI_Line7) != RESET) {
        EXTI_ClearITPendingBit(EXTI_Line7);

        if (Rx_State != UART_RX_IDLE) {
            return;
        }

        if (GPIO_ReadInputDataBit(SOFT_UART_RX_GPIO, SOFT_UART_RX_PIN) == Bit_RESET) {
                
            EXTI->IMR &= ~(EXTI_Line7);

            Rx_Data = 0U;
            Rx_BitIndex = 0U;
            Rx_State = UART_RX_DATA;

            uint32_t firstSampleCycles = SoftUART_BitCycles + (SoftUART_BitCycles / 2U);
            SoftUART_Start(firstSampleCycles);
        }
        
    }
}


 








void SOFT_UART_TIM_IRQHandler(void) {
    if (TIM_GetITStatus(SOFT_UART_TIM, TIM_IT_Update)) {
        TIM_ClearITPendingBit(SOFT_UART_TIM, TIM_IT_Update);
        switch (Rx_State) {
        case UART_RX_DATA:
            UART_M_ReceiveByte_TIM();
                if (Rx_BitIndex == 1U) {
                    TIM_SetAutoreload(SOFT_UART_TIM, SoftUART_BitCycles - 1U);
                }
            break;

        case UART_RX_STOP:
            if (GPIO_ReadInputDataBit(SOFT_UART_RX_GPIO, SOFT_UART_RX_PIN) != Bit_RESET) {
                
                if (!SoftUART_RingBufferIsFull()) {
                    SoftUART_RingBufferPut(Rx_Data);
                }

                if (UART_M_Push_Id < 512) {
                    str[UART_M_Push_Id] = Rx_Data;;
                    UART_M_Push_Id = (UART_M_Push_Id += 1) % 512;
                }

            }

            Rx_Data = 0U;
            Rx_BitIndex = 0U;
            Rx_State = UART_RX_IDLE;
            SoftUART_Stop();
            EXTI->IMR |= EXTI_Line7;

            break;
        
        default:

            Rx_Data = 0U;
            Rx_BitIndex = 0U;
            Rx_State = UART_RX_IDLE;
            SoftUART_Stop();
            EXTI->IMR |= EXTI_Line7;

            break;
        }
    }
}
