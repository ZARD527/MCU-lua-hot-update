/*
** $Id: lopcodes.c $
** Opcodes for Lua virtual machine
** See Copyright Notice in lua.h
*/

#define lopcodes_c
#define LUA_CORE

#include "lprefix.h"


#include "lopcodes.h"


 

LUAI_DDEF const lu_byte luaP_opmodes[NUM_OPCODES] = {
 
  opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iAsBx)		 
 ,opmode(0, 0, 0, 0, 1, iAsBx)		 
 ,opmode(0, 0, 0, 0, 1, iABx)		 
 ,opmode(0, 0, 0, 0, 1, iABx)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 0, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 0, iABC)		 
 ,opmode(0, 0, 0, 0, 0, iABC)		 
 ,opmode(0, 0, 0, 0, 0, iABC)		 
 ,opmode(0, 0, 0, 0, 0, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(1, 0, 0, 0, 0, iABC)		 
 ,opmode(1, 0, 0, 0, 0, iABC)		 
 ,opmode(1, 0, 0, 0, 0, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 0, iABC)		 
 ,opmode(0, 0, 0, 0, 0, iABC)		 
 ,opmode(0, 0, 0, 0, 0, isJ)		 
 ,opmode(0, 0, 0, 1, 0, iABC)		 
 ,opmode(0, 0, 0, 1, 0, iABC)		 
 ,opmode(0, 0, 0, 1, 0, iABC)		 
 ,opmode(0, 0, 0, 1, 0, iABC)		 
 ,opmode(0, 0, 0, 1, 0, iABC)		 
 ,opmode(0, 0, 0, 1, 0, iABC)		 
 ,opmode(0, 0, 0, 1, 0, iABC)		 
 ,opmode(0, 0, 0, 1, 0, iABC)		 
 ,opmode(0, 0, 0, 1, 0, iABC)		 
 ,opmode(0, 0, 0, 1, 0, iABC)		 
 ,opmode(0, 0, 0, 1, 1, iABC)		 
 ,opmode(0, 1, 1, 0, 1, iABC)		 
 ,opmode(0, 1, 1, 0, 1, iABC)		 
 ,opmode(0, 0, 1, 0, 0, iABC)		 
 ,opmode(0, 0, 0, 0, 0, iABC)		 
 ,opmode(0, 0, 0, 0, 0, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABx)		 
 ,opmode(0, 0, 0, 0, 1, iABx)		 
 ,opmode(0, 0, 0, 0, 0, iABx)		 
 ,opmode(0, 0, 0, 0, 0, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABx)		 
 ,opmode(0, 0, 1, 0, 0, iABC)		 
 ,opmode(0, 0, 0, 0, 1, iABx)		 
 ,opmode(0, 1, 0, 0, 1, iABC)		 
 ,opmode(0, 0, 1, 0, 1, iABC)		 
 ,opmode(0, 0, 0, 0, 0, iAx)		 
};

