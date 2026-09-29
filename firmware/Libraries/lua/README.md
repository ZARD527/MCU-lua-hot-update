# Lua MCU source bundle

This directory contains the complete Lua source set used by the STM32F411 firmware port.

Version: Lua 5.4.8.

Enabled embedded libraries:

- base;
- table;
- string;
- math.

Project-specific integration:

- `code/linit_mcu.c`: opens the selected embedded libraries;
- `code/lua_mcu_port.c/.h`: MCU allocator, output and Lua-state creation;
- `code/lua_script_update.c/.h`: Flash A/B script update and active-slot handling.

Desktop-oriented standard-library modules such as file I/O, OS access, package/dynamic loading, debug and UTF-8 are not part of this MCU source set. Add such modules only after reviewing their memory, platform and security implications.

Lua-originated source remains under the Lua MIT license in `LICENSE.txt`. Project-specific integration follows the repository root license.

