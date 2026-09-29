#include "include.h"
#include "Config.h"
#include "lauxlib.h"
#include "stdio.h"


uint8_t BSP_Init(void) {
    
    SystemCoreClockUpdate();
    LED_Init();
    UART_M_Init();
    Log_Init();
    Key_Init();
    uint8_t flag = Timer_Init();
    if(!flag) {
        printf("Timer Init Error!\n");
        return 1;
    }
    if (flag == 2U) {
#if TIMER_USE_LSE
        printf("[WARN] LSE unavailable, timer uses SysTick fallback\r\n");
#else
        printf("[INFO] LSE/RTC disabled, timer uses SysTick\r\n");
#endif
    }
    GPIO_SetBits(SOFT_UART_TX_GPIO, SOFT_UART_TX_PIN);

    return 0;
}


 














uint8_t BSP_LuaRegisterFunction(lua_State *L, const char *name, lua_CFunction func) {
    if (!L || !name || !func) {
        return 1U;
    }

    lua_register(L, name, func);

    return 0U;
}


 





















uint8_t BSP_LuaRegisterFunctionList(lua_State *L, const luaL_Reg *funcs) {
    const luaL_Reg *item;

    if (!L || !funcs) {
        return 1U;
    }

    for (item = funcs; item->name != NULL; item++) {
        if (!item->func) {
            return 1U;
        }
        lua_register(L, item->name, item->func);
    }

    return 0U;
}
