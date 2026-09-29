#include "include.h"
#include "Config.h"
#include "lua.h"
#include "lauxlib.h"
#include "lua_mcu_port.h"
#include "lua_script_update.h"
#include "stm32f4xx_flash.h"

char *Lua_Name;
lua_State *g_lua_state = NULL;
#define APP_ERASE_LUA_SLOTS_ON_BOOT  0

static char App_LuaName[] = "Flash_Lua";


 









static void App_Write(const char *msg) {
    UART_M_SendString(msg, 0);
}


 











static GPIO_TypeDef *App_GetLedGpio(int id) {
    return (id == 2) ? LED2_GPIO : LED1_GPIO;
}


 







static uint16_t App_GetLedPin(int id) {
    return (id == 2) ? LED2_PIN : LED1_PIN;
}


static int App_CheckLedId(lua_State *L) {
    int id = (int)luaL_checkinteger(L, 1);

    luaL_argcheck(L, id == 1 || id == 2, 1, "LED id must be 1 or 2");
    return id;
}


 











static int Lua_LedOn(lua_State *L) {
    int id = App_CheckLedId(L);
    LED_ON(App_GetLedGpio(id), App_GetLedPin(id));
    return 0;
}


 










static int Lua_LedOff(lua_State *L) {
    int id = App_CheckLedId(L);
    LED_OFF(App_GetLedGpio(id), App_GetLedPin(id));
    return 0;
}


 










static int Lua_LedToggle(lua_State *L) {
    int id = App_CheckLedId(L);
    LED_T(App_GetLedGpio(id), App_GetLedPin(id));
    return 0;
}


 











static int Lua_KeyRead(lua_State *L) {
    lua_pushinteger(L, Key_Read());
    return 1;
}


 











static int Lua_DelayMs(lua_State *L) {
    return Lua_Run_DelayMs(L);
}


 










static void Lua_RegisterMcuApi(lua_State *L) {
    static const luaL_Reg mcu_api[] = {
        {"led_on", Lua_LedOn},
        {"led_off", Lua_LedOff},
        {"led_toggle", Lua_LedToggle},
        {"key_read", Lua_KeyRead},
        {"delay_ms", Lua_DelayMs},
        {NULL, NULL}
    };

    BSP_LuaRegisterFunctionList(L, mcu_api);
}


 








static void App_EraseLuaSlotsOnBoot(void) {
#if APP_ERASE_LUA_SLOTS_ON_BOOT
    FLASH_Status status_a;
    FLASH_Status status_b;

    App_Write("[INFO] Erase Lua slot A/B start\r\n");

    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                    FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    status_a = FLASH_EraseSector(LUA_SLOT_A_SECTOR, VoltageRange_3);
    status_b = FLASH_EraseSector(LUA_SLOT_B_SECTOR, VoltageRange_3);

    FLASH_Lock();

    if (status_a == FLASH_COMPLETE && status_b == FLASH_COMPLETE) {
        App_Write("[INFO] Erase Lua slot A/B done\r\n");
    }
    else {
        App_Write("[ERR] Erase Lua slot A/B failed\r\n");
    }
#endif
}


static uint8_t App_RecoveryRequested(void) {
    if (GPIO_ReadInputDataBit(KEY1_GPIO, KEY1_PIN) != Bit_RESET) {
        return 0U;
    }

    Timer_DelayMs(20U);
    return (GPIO_ReadInputDataBit(KEY1_GPIO, KEY1_PIN) == Bit_RESET) ? 1U : 0U;
}


 















int main(void) {
    uint8_t bsp_status;
    int run_status;

    bsp_status = BSP_Init();

    App_Write("\r\n===== Lua Hot Update Test =====\r\n");
    if (bsp_status != 0U) {
        App_Write("[WARN] BSP_Init reported an error\r\n");
    }

    App_EraseLuaSlotsOnBoot();

    Lua_Name = App_LuaName;
    g_lua_state = lua_mcu_newstate();
    if (!g_lua_state) {
        App_Write("[ERR] lua_mcu_newstate failed\r\n");
        while (1) {
        }
    }

    Lua_RegisterMcuApi(g_lua_state);
    Lua_Run_Init();
    LuaScript_Init();
    Uart_To_Lua_Init();

    if (App_RecoveryRequested() != 0U) {
        App_Write("[WARN] Recovery key held, Lua autorun skipped\r\n");
    }
    else {
        run_status = LuaScript_RunActive(g_lua_state);
        if (run_status == 0 || Lua_Run_IsRunning() != 0U) {
            App_Write("[INFO] Active Lua script started\r\n");
        }
        else {
            App_Write("[INFO] No valid active Lua script, waiting UART update\r\n");
        }
    }

    while (1) {
        Cmd_Poll(g_lua_state);
        Lua_Run_Poll(g_lua_state);
        Timer_DelayMs(1);
    }
}
