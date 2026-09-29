#include "include.h"
#include "Config.h"
#include "stdio.h"
#include "string.h"
#include "lua.h"


uint16_t Kid_Cmd = WAIT_CMD;
static uint8_t Temp_Cmd[4];


uint8_t Which_Cmd(uint8_t *cmd) {
    if (!cmd) {
        return 1;
    }
    if (!strcmp((char *)cmd, "LUA")) {
        Kid_Cmd = LUA;
        return 0;
    }
    else if (!strcmp((char *)cmd, "CON")) {
        Kid_Cmd = Con_Cmd;
        return 0;
    }
    else {
        uint8_t dummy;
        SoftUART_RingBufferGet(&dummy);
        return 1;
    }
}


void Cmd_Poll(lua_State *L) {
    switch (Kid_Cmd) {
    case WAIT_CMD:
        if(SoftUART_RingBufferPeek(3, Temp_Cmd)) {
            if (!Which_Cmd(Temp_Cmd)) {
                uint8_t dummy;
                SoftUART_RingBufferGet(&dummy);
                SoftUART_RingBufferGet(&dummy);
                SoftUART_RingBufferGet(&dummy);
                if (Kid_Cmd == LUA && LUA_UART_DEBUG != 0U) {
                    printf("[LUA] cmd enter\r\n");
                }
                break;   
            }
        }
        break;
    case LUA:
        {
            uint16_t result = Uart_To_Lua_Poll(L);
            if (result == LUA_UART_DONE || result == LUA_UART_ERR) {
                Kid_Cmd = WAIT_CMD;
            }
        }
        break;
    case Con_Cmd:
        __NOP();
        Kid_Cmd = WAIT_CMD;
        break;
    default:
        printf("Error Cmd!\n");
        Kid_Cmd = WAIT_CMD;
        return;
    }
}
