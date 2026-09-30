#include "include.h"
#include "Config.h"


static volatile uint64_t g_timerTicks;
static volatile uint8_t g_timerRtcReady;


uint8_t Timer_Init(void) {
    g_timerTicks = 0;
    g_timerRtcReady = 0U;

    if (SysTick_Config(SystemCoreClock / 1000U) != 0U) {
        return 0U;
    }

#if TIMER_USE_LSE
    {
    uint32_t start;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    PWR_BackupAccessCmd(ENABLE);

    RCC_LSEConfig(RCC_LSE_ON);
    start = SysTick_GetMs();

    while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) {
        if ((SysTick_GetMs() - start) >= TIMER_LSE_STARTUP_TIMEOUT_MS) {
            return 2U;
        }
    }

    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
    RCC_RTCCLKCmd(ENABLE);
    RTC_WaitForSynchro();

    RTC_InitTypeDef RTC_InitStructure;

    RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
    RTC_InitStructure.RTC_AsynchPrediv = 0;
    RTC_InitStructure.RTC_SynchPrediv = 32767;
    
    RTC_Init(&RTC_InitStructure);
    
    RTC_WakeUpCmd(DISABLE);
    start = SysTick_GetMs();
    while (RTC_GetFlagStatus(RTC_FLAG_WUTWF) == RESET) {
        if ((SysTick_GetMs() - start) >= 1000U) {
            return 2U;
        }
    }
    RTC_WakeUpClockConfig(RTC_WakeUpClock_RTCCLK_Div16);
    RTC_SetWakeUpCounter(TIMER_RTC_WAKEUP_RELOAD);

    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line = EXTI_Line22;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;       
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;    
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;                 
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = RTC_WKUP_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_Init(&NVIC_InitStructure);

    RTC_ClearFlag(RTC_FLAG_WUTF);
    RTC_ClearITPendingBit(RTC_IT_WUT);
    EXTI_ClearITPendingBit(EXTI_Line22);
    RTC_ITConfig(RTC_IT_WUT, ENABLE);
    RTC_WakeUpCmd(ENABLE);
    g_timerRtcReady = 1U;

    return 1U;
    }
#else
    return 2U;
#endif

}


uint64_t Timer_GetTicks(void) {
    if (g_timerRtcReady == 0U) {
        return ((uint64_t)SysTick_GetMs() * TIMER_TICK_HZ) / 1000ULL;
    }

    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    uint64_t ticks = g_timerTicks;
    __set_PRIMASK(primask);
    return ticks;
}


uint32_t Timer_GetMs(void) {
    return (uint32_t)(Timer_GetTicks() * 1000ULL / TIMER_TICK_HZ);
}


static inline uint32_t BCD2BIN(uint32_t bcd) {
    return ((bcd >> 4) * 10 + (bcd & 0x0F));
}


uint64_t Timer_GetUs(void) {
    return Timer_GetTicks() * 1000000ULL / TIMER_TICK_HZ;
}


uint64_t Timer_GetUs_High(void) {
    uint32_t TR, SSR;

    if (g_timerRtcReady == 0U) {
        return (uint64_t)SysTick_GetMs() * 1000ULL;
    }

    TR = 0xFFFFFFFF;
    SSR = 0xFFFFFFFF;
    while(TR != RTC->TR) {
        TR  = RTC->TR;
        SSR = RTC->SSR;
    }

    uint32_t sec = BCD2BIN((TR >> 0) & 0x7F);
    uint32_t min = BCD2BIN((TR >> 8) & 0x7F);
    uint32_t hour = BCD2BIN((TR >> 16) & 0x3F);

    uint64_t totalUs = ((uint64_t)hour * 3600 + min * 60 + sec) * 1000000ULL;
    uint64_t subUs = (32767U - SSR) * 1000000ULL / 32768U;
    return totalUs + subUs;
}


void Timer_DelayMs(uint32_t ms) {
    uint64_t start = Timer_GetTicks();
    uint64_t timeout = ((uint64_t)ms * TIMER_TICK_HZ + 999ULL) / 1000ULL;
    while (!Timer_Expired(start, timeout)) {
        __NOP();
    }
    
}


uint8_t Timer_Expired(uint64_t startTick, uint64_t timeoutTick) {
    return ((Timer_GetTicks() - startTick) >= timeoutTick) ? 1U : 0U;
}


uint64_t Timer_MsToTicks(uint32_t ms) {
    return ((uint64_t)ms * TIMER_TICK_HZ + 999ULL) / 1000ULL;
}


void Timer_Start(TimerTask_t *timer, uint32_t timeoutMs) {
    if (!timer || !timeoutMs) {
        return;
    }

    timer->startTick = Timer_GetTicks();
    timer->timeoutTick = Timer_MsToTicks(timeoutMs);

}


uint8_t Timer_IsExpired(const TimerTask_t *timer) {
    if (!timer) {
        return 0U;
    }

    return Timer_Expired(timer->startTick, timer->timeoutTick);
}


void Timer_Restart(TimerTask_t *timer) {
    if (timer == 0) {
        return;
    }

    timer->startTick = Timer_GetTicks();
}


void Timer_RTCWakeupIRQHandler(void) {
    if (g_timerRtcReady != 0U && RTC_GetITStatus(RTC_IT_WUT) != RESET) {
        RTC_ClearITPendingBit(RTC_IT_WUT);
        EXTI_ClearITPendingBit(EXTI_Line22);
        g_timerTicks++;
    }
}
