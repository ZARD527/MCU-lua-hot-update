#ifndef lua_mcu_port_h
#define lua_mcu_port_h


#include "lua.h"
#include <stddef.h>
#include <stdint.h>


#ifndef LUA_MCU_HEAP_SIZE
#define LUA_MCU_HEAP_SIZE  (64u * 1024u)
#endif


#define LUA_MCU_ALIGN_SIZE  8u


typedef struct LuaMcuHeap {
  uint8_t *buffer;
  size_t size;
  size_t used;
  size_t peak;
  size_t failed;
  void *first;
} LuaMcuHeap;

typedef struct LuaMcuBlock {
    size_t size;
    uint8_t used;
    struct LuaMcuBlock *prev;
    struct LuaMcuBlock *next;
} LuaMcuBlock;


void lua_mcu_writeline(void);
uint64_t lua_mcu_get_tick(void);
uint32_t lua_mcu_get_seed(void);
lua_State *lua_mcu_newstate(void);
void lua_mcu_write(const char *s, size_t len);
int lua_mcu_dostring(lua_State *L, const char *script);
void lua_mcu_write_error(const char *msg);
void lua_mcu_heap_init(void);
void *lua_mcu_alloc(void *ud, void *ptr, size_t osize, size_t nsize);
int lua_mcu_dobuffer(lua_State *L, const char *name, const char *buff, size_t size);


#endif
