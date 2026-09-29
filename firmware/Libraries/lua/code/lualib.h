/*
** $Id: lualib.h $
** Lua standard libraries
** See Copyright Notice in lua.h
*/


#ifndef lualib_h
#define lualib_h

#include "lua.h"


 
#define LUA_VERSUFFIX          "_" LUA_VERSION_MAJOR "_" LUA_VERSION_MINOR


LUAMOD_API int (luaopen_base) (lua_State *L);


#define LUA_TABLIBNAME	"table"
LUAMOD_API int (luaopen_table) (lua_State *L);


#define LUA_STRLIBNAME	"string"
LUAMOD_API int (luaopen_string) (lua_State *L);


#define LUA_MATHLIBNAME	"math"
LUAMOD_API int (luaopen_math) (lua_State *L);


LUALIB_API void (luaL_openlibs_mcu) (lua_State *L);


#endif
