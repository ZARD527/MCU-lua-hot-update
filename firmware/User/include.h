#ifndef __INCLUDE_H_
#define __INCLUDE_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "lua.h"
#include "lauxlib.h"
#include "stm32f4xx.h"
#include "stm32f4xx_it.h"


#ifdef __cplusplus
}
#endif


void LED_Init(void);
void LED_T(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void LED_ON(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void LED_OFF(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);



void Key_Init(void);
uint8_t Key_Read(void);



typedef struct {
    uint64_t startTick;
    uint64_t timeoutTick;
}TimerTask_t;


uint8_t Timer_Init(void);
uint32_t Timer_GetMs(void);
uint64_t Timer_GetUs(void);
uint64_t Timer_GetTicks(void);
uint64_t Timer_GetUs_High(void);

void Timer_DelayMs(uint32_t ms);
uint8_t Timer_Expired(uint64_t startTick, uint64_t timeoutTick);

void RTC_WKUP_IRQHandler(void);
void Timer_RTCWakeupIRQHandler(void);

uint64_t Timer_MsToTicks(uint32_t ms);
void Timer_Restart(TimerTask_t *timer);
uint8_t Timer_IsExpired(const TimerTask_t *timer);
void Timer_Start(TimerTask_t *timer, uint32_t timeoutMs);



void UART_M_Init(void);
void UART_M_SendByte(uint8_t data);
void SoftUART_RingBufferClear(void);
uint16_t SoftUART_RingBufferSize(void);
uint8_t SoftUART_RingBufferIsEmpty(void);
uint8_t SoftUART_RingBufferGet(uint8_t *data);
uint8_t SoftUART_RingBufferPut(uint8_t data);
void UART_M_SendString(const char *str, uint16_t len);
uint8_t SoftUART_RingBufferPeek(uint16_t len, uint8_t *data);



void UART_H_Init(void);
void UART_H_GPIO_Init(void);
void UART_H_NVIC_Init(void);
void UART_H_SendByte(uint8_t data);
void UART_H_SendString(const char *str, uint16_t len);
uint8_t UART_H_ReceiveByte(uint8_t *data);
void UART_H_IRQHandler(void);
void UART_H_RingBufferClear(void);
void UART_H_RingBufferInit(void);
uint16_t UART_H_RingBufferSize(void);
uint8_t UART_H_RingBufferIsEmpty(void);
uint8_t UART_H_RingBufferIsFull(void);
uint8_t UART_H_RingBufferPut(uint8_t data);
uint8_t UART_H_RingBufferGet(uint8_t *data);
uint8_t UART_H_RingBufferPeek(uint16_t len, uint8_t *data);



void Log_Init(void);
void log_info(const char *fmt, ...);
void log_error(const char *fmt, ...);



uint8_t BSP_Init(void);
uint8_t BSP_LuaRegisterFunction(lua_State *L, const char *name, lua_CFunction func);
uint8_t BSP_LuaRegisterFunctionList(lua_State *L, const luaL_Reg *funcs);



void Uart_To_Lua_Init(void);
uint16_t Uart_To_Lua_Poll(lua_State *L);
void Uart_To_Lua_SendAck(uint8_t seq, int ret);
void Uart_To_Lua_SendErr(uint8_t seq, int ret);



uint8_t Which_Cmd(uint8_t *cmd);
void Cmd_Poll(lua_State *L);



void Lua_Run_Init(void);
void Lua_Run_Poll(lua_State *L);
void Lua_Run_Stop(void);
uint8_t Lua_Run_IsRunning(void);
int Lua_Run_DelayMs(lua_State *L);
int Lua_Run_StartBuffer(const char *name, const char *buff, size_t size);
#endif
