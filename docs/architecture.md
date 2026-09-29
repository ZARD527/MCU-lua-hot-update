# Architecture

Paths in this document describe the firmware after the integration package has been merged into a complete hardware base project. The release includes the complete Lua MCU directory and changed application files; unmodified CMSIS, STM32 SPL and startup/platform files are intentionally absent.

## Runtime data flow

```text
PB7 software UART RX ─┐
                      ├─> shared RX ring buffer -> Cmd_Poll()
PA10 USART1 RX ───────┘                           |
                                                  v
                                      UART Lua protocol parser
                                      CRC / length / sequence
                                                  |
                              +-------------------+------------------+
                              |                                      |
                         Flash A/B update                       Lua RUN
                              |                                      |
                         CRC32 + syntax                       coroutine poll
                              |                                      |
                         active slot                         LED / key / delay

ACK output: PB6 software UART TX and PA9 USART1 TX
```

Both receive paths currently feed one parser buffer. Simultaneously transmitting independent byte streams into both ports is unsupported because the bytes can be interleaved.

## Firmware layers

- `User/main.c`: startup order, Lua state creation, C API registration and main loop.
- `User/bsp.c`: clocks and board peripherals.
- `User/UART_M.c`: interrupt-driven software UART and shared ring buffer.
- `User/UART_H.c`: USART1 logging/update input.
- `User/cmd.c`: ASCII command-prefix synchronization.
- `User/UART_To_Lua.c`: binary update protocol state machine.
- `User/Lua_Run.c`: cooperative Lua coroutine scheduler and execution limits.
- `Libraries/lua/code/lua_mcu_port.c`: fixed-pool allocator and MCU Lua port.
- `Libraries/lua/code/lua_script_update.c`: Flash A/B storage and active-slot selection.

## Update transaction

```text
LUA prefix
  -> BEGIN(version, length, crc32)
  -> DATA(seq=1..n)
  -> END

LUA prefix
  -> RUN(seq=0)
```

Packets are validated before dispatch. A duplicate of the most recently processed packet receives the cached result and is not written to Flash again.

## Flash boundaries

The Keil target limits application code to the first 256 KiB. Sectors 6 and 7 are reserved for Lua slots. A full-chip erase still removes both slots, so debugger/download configuration remains part of the operational safety model.

## Trust boundaries

The parser checks corruption and protocol consistency, not sender identity. Lua scripts should be considered privileged input. See `SECURITY.md` before adapting this design to a product.

