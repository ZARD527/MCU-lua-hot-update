#include "lua_script_update.h"
#include "include.h"
#include "Config.h"
#include "lua.h"
#include "stdio.h"


typedef struct {
    int co_ref;
    uint8_t state;
    lua_State *co;
    uint64_t wake_tick;
    uint64_t run_start_tick;
} LuaTask_t;


static int nresults = 0;
TimerTask_t Lua_Task_Timer;
static LuaTask_t g_lua_task;
uint8_t Lua_Task_State = LUA_RUN_IDLE;


static void Lua_Run_ReportError(LuaTask_t *co) {

    if (!co || !co->co) {
        return;
    }

    const char *str = lua_tostring(co->co, -1);
    if (!str){
        str = "Unknown Lua Error!";
    }

    lua_mcu_write_error(str);
    lua_pop(co->co, 1);

}


static void Lua_Run_Hook(lua_State *L, lua_Debug *ar) {
    (void) ar;
    if((Timer_GetTicks() - g_lua_task.run_start_tick) >= Timer_MsToTicks(20)) {
        luaL_error(L, "Lua timeout, missing delay_ms or yield");
    }
}


static void lua_task_resume(lua_State *L) {

    if (!g_lua_task.co || Lua_Task_State != LUA_RUN_RUNNING) {
        printf("Lua Resum Error!\n");
        g_lua_task.state = LUA_RUN_ERROR;
        Lua_Task_State = LUA_RUN_ERROR;
        return;
    }

    g_lua_task.run_start_tick = Timer_GetTicks();
    int temp = lua_resume(g_lua_task.co, L, 0, &nresults);

    if (temp == LUA_OK) {
        Lua_Task_State = LUA_RUN_DONE;
        g_lua_task.state = LUA_RUN_DONE;
    }
    else if (temp == LUA_YIELD) {
        Lua_Task_State = LUA_RUN_SLEEPING;
        g_lua_task.state = LUA_RUN_SLEEPING;
    }
    else {

        Lua_Task_State = LUA_RUN_ERROR;
        g_lua_task.state = LUA_RUN_ERROR;
        printf("Lua Task Resum Error!\n");
        return;

    }

}


void Lua_Run_Init(void) {

    g_lua_task.co = NULL;
    g_lua_task.co_ref = LUA_NOREF;
    g_lua_task.state = LUA_RUN_IDLE;

    Lua_Task_State = LUA_RUN_IDLE;

    Lua_Task_Timer.startTick = 0;
    Lua_Task_Timer.timeoutTick = 0;

    Lua_Task_State = LUA_RUN_IDLE;
   
}


int Lua_Run_StartBuffer(const char *name, const char *buff, size_t size) {
    
    

    if (!name || !buff || size <= 0) {
        printf("StartBuffer Error!\n");
        return -1;
    }

    if (g_lua_task.co_ref != LUA_NOREF) {
        Lua_Run_Stop();
    }

    if(!g_lua_state) {
        printf("Lua Newstate Error!\n");
        return -1;
    }

    g_lua_task.co = lua_newthread(g_lua_state);
    if (!g_lua_task.co) {
        printf("Lua Co Error\n");
        return -1;
    }
    g_lua_task.co_ref = luaL_ref(g_lua_state, LUA_REGISTRYINDEX);
    g_lua_task.state = LUA_RUN_RUNNING;
    g_lua_task.run_start_tick = 0;
    g_lua_task.wake_tick = 0;


    if(luaL_loadbufferx(g_lua_task.co, buff, size, name, "t") == LUA_OK) {
        lua_sethook(g_lua_task.co, Lua_Run_Hook, LUA_MASKCOUNT, 1000);
        Lua_Task_State = LUA_RUN_RUNNING;
        g_lua_task.state = LUA_RUN_RUNNING;
        return 0;
    }

    g_lua_task.state = LUA_RUN_ERROR;
    Lua_Task_State = LUA_RUN_ERROR;
    Lua_Run_ReportError(&g_lua_task);
    Lua_Run_Stop();
    return -1;
}


void Lua_Run_Poll(lua_State *L) {

    switch (Lua_Task_State) {
    case LUA_RUN_IDLE:
        
        break;

    case LUA_RUN_RUNNING:
        lua_task_resume(L);
        break;

    case LUA_RUN_SLEEPING:
        if(Timer_IsExpired(&Lua_Task_Timer)) {
            Lua_Task_State = LUA_RUN_RUNNING;
            g_lua_task.state = LUA_RUN_RUNNING;
        }
        break;

    case LUA_RUN_DONE:
        Lua_Run_Stop();
        break;

    case LUA_RUN_ERROR:
        Lua_Run_ReportError(&g_lua_task);
        Lua_Run_Stop();
        if (LuaScript_HandleRunFailure(L) == 0) {
            printf("[WARN] Lua runtime failed, fallback script started\r\n");
        }
        else {
            printf("[ERROR] Lua runtime failed, no fallback script\r\n");
        }
        break;

    }

}


void Lua_Run_Stop(void) {

    if (g_lua_task.co_ref != LUA_NOREF) {

        if (!g_lua_state) {
            printf("Stop Lua NULL Error!\n");

            g_lua_task.co_ref = LUA_NOREF;
        
            Lua_Task_Timer.startTick = 0;
            Lua_Task_Timer.timeoutTick = 0;
            g_lua_task.co = NULL;

            Lua_Task_State = LUA_RUN_IDLE;    
            g_lua_task.state = LUA_RUN_IDLE;

            return;
        }
        
        luaL_unref(g_lua_state, LUA_REGISTRYINDEX, g_lua_task.co_ref);
        
        g_lua_task.co_ref = LUA_NOREF;
        
        Lua_Task_Timer.startTick = 0;
        Lua_Task_Timer.timeoutTick = 0;
        g_lua_task.co = NULL;

        Lua_Task_State = LUA_RUN_IDLE;
        g_lua_task.state = LUA_RUN_IDLE;
    }
    else {
        
        g_lua_task.co_ref = LUA_NOREF;
        
        Lua_Task_Timer.startTick = 0;
        Lua_Task_Timer.timeoutTick = 0;
        g_lua_task.co = NULL;

        Lua_Task_State = LUA_RUN_IDLE;
        g_lua_task.state = LUA_RUN_IDLE;
    }

}


uint8_t Lua_Run_IsRunning(void) {
    if (g_lua_task.state == LUA_RUN_RUNNING ||
        g_lua_task.state == LUA_RUN_SLEEPING) {
        return 1;
    }
    return 0;
}


int Lua_Run_DelayMs(lua_State *L) {
    lua_Integer requested = luaL_checkinteger(L, 1);
    uint32_t ms;

    luaL_argcheck(L, requested > 0 && requested <= 60000, 1,
                  "delay must be in range 1..60000 ms");
    ms = (uint32_t)requested;
    Timer_Start(&Lua_Task_Timer, ms);
    Lua_Task_State = LUA_RUN_SLEEPING;
    g_lua_task.state = LUA_RUN_SLEEPING;
    return lua_yield(L, 0);

}
