#include "Config.h"
#include "include.h"

#include <stdarg.h>
#include <stdio.h>

#pragma import(__use_no_semihosting)

struct __FILE {
    int handle;
};

FILE __stdout;

static uint8_t g_logSoftReady;

static void Log_PrintClockInfo(void);

static void Log_PrintClockInfo(void) {
    RCC_ClocksTypeDef clocks;

    RCC_GetClocksFreq(&clocks);

    log_info("Chip: %s", LOG_CHIP_NAME);
    log_info("Firmware: %s", LOG_FW_VERSION);
    log_info("Build: %s %s", __DATE__, __TIME__);
    log_info("SYSCLK: %lu Hz", clocks.SYSCLK_Frequency);
    log_info("HCLK: %lu Hz", clocks.HCLK_Frequency);
    log_info("PCLK1: %lu Hz", clocks.PCLK1_Frequency);
    log_info("PCLK2: %lu Hz", clocks.PCLK2_Frequency);
}

void Log_Init(void) {
    SystemCoreClockUpdate();
    UART_H_Init();
    g_logSoftReady = 1U;

    printf("\r\n");
    log_info("Log soft: PB6=TX PB7=RX %lu 8N1", (uint32_t)SOFT_UART_BAUDRATE);
    log_info("Log mirror: USART1 PA9=TX PA10=RX %lu 8N1", (uint32_t)LOG_UART_BAUDRATE);
    Log_PrintClockInfo();
}

void log_info(const char *fmt, ...) {
    va_list args;

    printf("[INFO] ");
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\r\n");
}

void log_error(const char *fmt, ...) {
    va_list args;

    printf("[ERROR] ");
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\r\n");
}

int fputc(int ch, FILE *f) {
    (void)f;

    if (g_logSoftReady != 0U) {
        UART_M_SendByte((uint8_t)ch);
    }

    UART_H_SendByte((uint8_t)ch);

    return ch;
}

void _ttywrch(int ch) {
    (void)fputc(ch, &__stdout);
}

void _sys_exit(int return_code) {
    (void)return_code;

    while (1) {
    }
}
