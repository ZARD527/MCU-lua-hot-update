# Firmware integration package

This directory contains the changed STM32 application files and the complete Lua MCU source directory used by the tested project. It is designed to be imported into a compatible STM32F411 hardware base project.

It is not a standalone board project because CMSIS, STM32 SPL, startup/system files and unchanged board-base headers are intentionally not duplicated.

## Included application/project changes

```text
Hello_World.uvprojx
User/bsp.c
User/cmd.c
User/Config.h
User/include.h
User/Key.c
User/LED.c
User/log.c
User/Lua_Run.c
User/main.c
User/stm32f4xx_it.c
User/timer.c
User/UART.c
User/UART_H.c
User/UART_M.c
User/UART_To_Lua.c
```

## Included Lua MCU directory

`Libraries/lua/` is copied as a complete unit from the tested firmware and contains:

- Lua 5.4.8 parser, VM, garbage collector and core API;
- auxiliary library and the base, table, string and math libraries;
- `linit_mcu.c`, which registers the embedded library subset;
- `lua_mcu_port.c/.h`, including the fixed MCU memory pool and output/error adaptation;
- `lua_script_update.c/.h`, including Flash A/B script storage and validation;
- public/internal Lua headers and the Lua MIT license;
- the port Makefile retained for source provenance/reference.

This is the complete Lua source set used by this MCU project, not the full desktop Lua distribution. File I/O, OS, package/dynamic loading, debug and UTF-8 standard-library modules are intentionally absent to reduce Flash/RAM usage and attack surface. Coroutine execution used by `Lua_Run.c` relies on the Lua core API and remains supported.

## Required hardware base components

Prepare a complete STM32F411 project containing:

- STM32F411CEUx CMSIS headers and startup/system source;
- STM32F4 Standard Peripheral Library compatible with V1.8.1;
- `User/stm32f4xx_conf.h` and `User/stm32f4xx_it.h`;
- the normal Keil build environment.

Lua does not need to be imported from another location; use the bundled `Libraries/lua` directory.

## Import procedure

1. Back up the destination hardware project.
2. Copy `Libraries/lua` as one complete directory to the destination `Libraries/lua` path.
3. Copy the included `User` files to the same relative destination paths.
4. Review hardware definitions in `User/Config.h` before replacing an existing board configuration. The capital `C` is intentional; on a case-sensitive system, rename/remove an older `User/config.h` so only `User/Config.h` remains.
5. Add every `.c` file under `Libraries/lua/code` to a Keil group such as `Lua`.
6. Add or confirm all included `User/*.c` files in the application group.
7. Use `Hello_World.uvprojx` as a configuration reference. Replace a destination project file only when it shares the same base and has been backed up.

## Reference Keil configuration

Target device:

```text
STM32F411CEUx
```

Compiler symbols:

```text
STM32F411xE
USE_STDPERIPH_DRIVER
LUA_USE_C89
LUA_USE_JUMPTABLE=0
LUA_MCU_NO_FILE
```

Reference include paths:

```text
./User
./Libraries/lua/code
./RTE/Device/STM32F411CEUx
./Libraries/CMSIS/Include
./Libraries/CMSIS/Device/ST/STM32F4xx/Include
./Libraries/STM32F4xx_StdPeriph_Driver/inc
```

Application IROM must stop before the Lua slots:

```text
start: 0x08000000
size:  0x00040000
```

Lua storage remains:

```text
Slot A: 0x08040000, 128 KiB, Sector 6
Slot B: 0x08060000, 128 KiB, Sector 7
```

## Important checks after import

- `APP_ERASE_LUA_SLOTS_ON_BOOT` should remain `0` for normal use.
- `TIMER_USE_LSE` is `0`; the default board does not require an external 32.768 kHz crystal.
- Confirm PB6/PB7 9600 and PA9/PA10 115200 match the physical connection.
- Avoid full-chip erase when preserving Lua slots.
- Run a complete Keil rebuild and the hardware checklist in `../docs/building.md`.

