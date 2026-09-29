#ifndef __LUA_SCRIPT_UPDATE_H_
#define __LUA_SCRIPT_UPDATE_H_


#include "lua.h"
#include "stdio.h"
#include "stddef.h"
#include "stdint.h"
#include "string.h"
#include "lualib.h"
#include "lauxlib.h"
#include "include.h"
#include "lua_mcu_port.h"


#define LUA_SCRIPT_MAGIC          0x4C554153u
#define LUA_SCRIPT_STATE_EMPTY    0xFFFFFFFFu
#define LUA_SCRIPT_STATE_WRITING  0xAAAA5555u
#define LUA_SCRIPT_STATE_VALID    0xFFFFFFFEu
#define LUA_SCRIPT_STATE_BAD      0xFFFFFFFCu

#define LUA_ALL_SIZE   0x00020000u
#define LUA_SCRIPT_MAX_SIZE  0x00008000u
#define LUA_SCRIPT_MAX_VERSION 0x7FFFFFFFu

#define LUA_SLOT_A_ADDR   0x08040000u
#define LUA_SLOT_A_SIZE   0x00020000u

#define LUA_SLOT_B_ADDR   0x08060000u
#define LUA_SLOT_B_SIZE   0x00020000u

#define LUA_SLOT_A_SECTOR FLASH_Sector_6
#define LUA_SLOT_B_SECTOR FLASH_Sector_7


typedef struct {
    uint32_t magic;      
    uint32_t version;    
    uint32_t length;     
    uint32_t crc32;      
    uint32_t seq;        
    uint32_t state;      
} LuaScriptHeader;

typedef char LuaScriptHeader_MustBe24Bytes[
    (sizeof(LuaScriptHeader) == 24U) ? 1 : -1];


void LuaScript_Init(void);
uint32_t Get_Lua_Size(void);
int LuaScript_EndUpdate(lua_State *L);
int LuaScript_RunActive(lua_State *L);
int LuaScript_WriteChunk(const uint8_t *data, uint32_t len);
int LuaScript_BeginUpdate(uint32_t version, uint32_t length, uint32_t crc32);
void LuaScript_AbortUpdate(void);
int LuaScript_HandleRunFailure(lua_State *L);


#endif
