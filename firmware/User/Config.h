#ifndef __CONFIG_H__
#define __CONFIG_H__


#include "lua.h"
#include "stdint.h"



extern char *Lua_Name;
extern lua_State *g_lua_state;



typedef enum {
    UART_RX_IDLE = 0,
    UART_RX_DATA,
    UART_RX_STOP
} UART_RX_State;

#define SOFT_UART_TX_GPIO        GPIOB
#define SOFT_UART_TX_PIN         GPIO_Pin_6
#define SOFT_UART_RX_GPIO        GPIOB
#define SOFT_UART_RX_PIN         GPIO_Pin_7
#define SOFT_UART_GPIO_CLK       RCC_AHB1Periph_GPIOB
#define SOFT_UART_BAUDRATE       9600U

#define SOFT_UART_TIM           TIM5
#define SOFT_UART_TIM_IRQn       TIM5_IRQn
#define SOFT_UART_TIM_IRQHandler TIM5_IRQHandler
#define SOFT_UART_TIM_RCC       RCC_APB1Periph_TIM5


#define LOG_FW_VERSION          "0.1.0"
#define LOG_CHIP_NAME           "STM32F411CEUx"

#define LOG_UART                USART1
#define LOG_UART_BAUDRATE       115200U
#define LOG_UART_GPIO           GPIOA
#define LOG_UART_TX_PIN         GPIO_Pin_9
#define LOG_UART_RX_PIN         GPIO_Pin_10
#define LOG_UART_TX_SOURCE      GPIO_PinSource9
#define LOG_UART_RX_SOURCE      GPIO_PinSource10
#define LOG_UART_GPIO_AF        GPIO_AF_USART1
#define LOG_UART_GPIO_CLK       RCC_AHB1Periph_GPIOA
#define LOG_UART_CLK            RCC_APB2Periph_USART1



#define LUA_UART_MAX_PAYLOAD          256U
#define LUA_UART_PACKET_TIMEOUT_MS    12000U
#define LUA_UART_REQUIRE_CRC16        1U
#define LUA_UART_DEBUG                0U

#define LUA_UART_ACK_MAGIC0           0x41U   
#define LUA_UART_ACK_MAGIC1           0x4BU   
#define LUA_UART_ACK_VERSION          1U
#define LUA_UART_ACK_SIZE             8U

#define LUA_UART_RET_OK               0
#define LUA_UART_ERR_ARGUMENT        -100
#define LUA_UART_ERR_HEADER          -101
#define LUA_UART_ERR_LENGTH          -102
#define LUA_UART_ERR_CRC16           -103
#define LUA_UART_ERR_SEQUENCE        -104
#define LUA_UART_ERR_TIMEOUT         -105
#define LUA_UART_ERR_COMMAND         -106



#define TIMER_USE_LSE            0U
#define TIMER_RTC_WAKEUP_DIV     16U
#define TIMER_TICK_HZ            1024U
#define TIMER_LSE_FREQ_HZ        32768U
#define TIMER_LSE_STARTUP_TIMEOUT_MS  5000U
#define TIMER_RTC_WAKEUP_RELOAD  ((TIMER_LSE_FREQ_HZ / TIMER_RTC_WAKEUP_DIV / TIMER_TICK_HZ) - 1U)



#define LED1_GPIO               GPIOC
#define LED1_PIN                GPIO_Pin_13
#define LED2_GPIO               GPIOB
#define LED2_PIN                GPIO_Pin_9



#define KEY1_GPIO               GPIOA
#define KEY1_PIN                GPIO_Pin_0




typedef struct {
    uint32_t version;
    uint32_t length;
    uint32_t crc32;
} LuaUpdateBeginPayload;

typedef struct {
    uint8_t  cmd;      
    uint8_t  seq;      
    uint16_t len;      
    uint16_t crc16;    
} LuaUartPacketHeader;

typedef char LuaUartPacketHeader_MustBe6Bytes[
    (sizeof(LuaUartPacketHeader) == 6U) ? 1 : -1];

typedef enum {
    LUA_UART_WAIT,
    LUA_UART_READ_HEADER,
    LUA_UART_READ_PAYLOAD,
    LUA_UART_ERROR_RECOVER
} LUA_State;

#define LUA_UART_CMD_BEGIN  0x01
#define LUA_UART_CMD_DATA   0x02
#define LUA_UART_CMD_END    0x03
#define LUA_UART_CMD_RUN    0x04

#define LUA_UART_BUSY 0
#define LUA_UART_DONE 1
#define LUA_UART_ERR  2



typedef struct {
    uint8_t  magic0;
    uint8_t  magic1;
    uint8_t  magic2;
    uint16_t crc16;
    uint8_t  cmd;
    uint8_t  seq;
    uint16_t len;
} Cmd_Packet;

typedef enum {
    WAIT_CMD,
    LUA,
    Con_Cmd,
} Cmd_Kid;



extern lua_State *g_lua_state;

#define LUA_RUN_IDLE      0

#define LUA_RUN_RUNNING   1
#define LUA_RUN_SLEEPING  2
#define LUA_RUN_DONE      3
#define LUA_RUN_ERROR     4


#endif
