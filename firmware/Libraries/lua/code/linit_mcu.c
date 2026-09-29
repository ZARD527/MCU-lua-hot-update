#define linit_mcu_c
#define LUA_LIB

#include "lua.h"
#include "lualib.h"
#include <stddef.h>
#include "lauxlib.h"
#include "lprefix.h"

static const luaL_Reg mcu_loadedlibs[] = {
  {LUA_GNAME, luaopen_base},
  {LUA_TABLIBNAME, luaopen_table},
  {LUA_STRLIBNAME, luaopen_string},
  {LUA_MATHLIBNAME, luaopen_math},
  {NULL, NULL}
};

LUALIB_API void luaL_openlibs_mcu(lua_State *L) {
  const luaL_Reg *lib;

  for (lib = mcu_loadedlibs; lib->func; lib++) {
    luaL_requiref(L, lib->name, lib->func, 1);
    lua_pop(L, 1);
  }
}
