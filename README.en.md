# STM32F411 Lua Script Runtime and Hot-Update Framework

> A portable Lua runtime and UART hot-update package for STM32F411

[简体中文](README.md) | English

This project ports Lua 5.4.8 to the STM32F411CEU6. It provides a streamlined Lua runtime, a fixed-size memory pool, coroutine-based scheduling, Flash A/B script storage, a UART update protocol, and a companion PyQt5 desktop tool.

## Current Status

| Item | Status |
| --- | --- |
| MCU | STM32F411CEU6 / STM32F411CEUx |
| Lua | 5.4.8 with a 64 KiB fixed memory pool |
| Script storage | Internal Flash A/B slots |
| Update interfaces | PB6/PB7 software UART or USART1 on PA9/PA10 |
| PC tool | Python, PyQt5, and pyserial |
| Protocol tests | 6 unit tests passing |
| Reference build | ARMCC 5.06 update 5: 0 errors, 2 warnings |

## Key Features

- Lua 5.4.8 parser, virtual machine, garbage collector, and MCU adaptation layer.
- A 64 KiB static Lua memory pool with 8-byte alignment and no dependency on the system heap.
- A reduced standard-library set containing base, table, string, and math.
- Lua/C APIs for LEDs, keys, and non-blocking delays.
- Coroutine-based Lua scheduling: `delay_ms()` suspends only the Lua task and does not block the MCU main loop.
- A 20 ms continuous-execution guard prevents Lua code that does not yield voluntarily from monopolizing the main loop.
- Flash A/B script slots with write-state tracking, CRC32 validation, and Lua syntax preflight checks.
- Per-packet CRC16, consecutive sequence numbers, a 12-second receive timeout, and idempotent responses to duplicate packets.
- Fixed 8-byte binary ACK frames that can be detected in a stream containing text logs.
- On load or runtime failure, the active slot is marked `BAD` and the firmware attempts to start the other valid slot.
- A PyQt5 GUI, packet builder, serial sender, ACK retry handling, and protocol tests.
- Holding PA0 during startup skips Lua autorun and preserves a recovery path.

## Repository Layout

```text
.
├─ firmware/
│  ├─ User/                         Modified application, BSP, UART, and Lua scheduler files
│  ├─ Libraries/lua/                Complete Lua MCU source set used by this project
│  │  ├─ code/                      Lua core, reduced libraries, and MCU extensions
│  │  ├─ LICENSE.txt                Lua MIT License
│  │  ├─ README.md                  English
│  │  └─ README.zh-CN.md            Chinese
│  ├─ README.md                     English integration guide
│  └─ README.zh-CN.md               Chinese porting guide
├─ tools/uart_updater/              PyQt5 serial update tool and protocol tests
├─ examples/lua/                    Lua example scripts
├─ docs/                            Architecture, build, protocol, and hardware documentation
├─ .github/workflows/               PC protocol test workflow
├─ SECURITY.md                      Security boundaries and vulnerability reporting
├─ THIRD_PARTY_NOTICES.md           Third-party dependencies and license scope
├─ CONTRIBUTING.md                  Contribution guidelines
└─ LICENSE                          MIT License for original project code
```

## Scope of the Porting Package

The repository includes:

- `firmware/Libraries/lua`: the Lua core, headers, reduced standard libraries, and MCU extensions.
- `firmware/User`: application-layer files modified or added by this project.
- The PC serial tool, Lua examples, protocol documentation, and porting documentation.

You must provide the following from your own STM32F411 base project:

- STM32F411 CMSIS Device headers.
- The startup file for your compiler and `system_stm32f4xx.c`.
- STM32F4 Standard Peripheral Library, with V1.8.1 used as the reference version.
- `User/stm32f4xx_conf.h` and `User/stm32f4xx_it.h`.
- A Keil project file or another build system.

This repository is a porting package, not a standalone, directly buildable Keil project.

## Hardware Configuration

| Function | Pin/resource | Configuration |
| --- | --- | --- |
| LED1 | PC13 | Active low |
| LED2 | PB9 | Active low |
| KEY1/recovery key | PA0 | Pull-up input, active low, 20 ms debounce |
| Software UART TX/RX | PB6/PB7 | 9600 8N1, sampled by TIM5 |
| Log/hardware UART TX/RX | PA9/PA10 | USART1, 115200 8N1 |
| System time base | SysTick | External LSE initialization is disabled by default |

Both receive interfaces ultimately write to the same protocol ring buffer. Do not send independent data streams through both ports at the same time.

## Lua Runtime Environment

Enabled standard libraries:

```text
base / table / string / math
```

To limit Flash usage, RAM usage, and attack surface, the file I/O, OS, package/dynamic-loading, debug, and UTF-8 libraries are not included. Coroutine scheduling uses the Lua core API and does not depend on the `coroutine` standard-library table.

MCU APIs registered with Lua:

```lua
led_on(id)       -- id: 1 or 2
led_off(id)
led_toggle(id)
key_read()       -- returns 1 when a key-press event is detected
delay_ms(ms)     -- yields the coroutine without blocking the C main loop
```

Long-running loops in Lua scripts must periodically call `delay_ms()` or another yield entry point. Otherwise, the continuous-execution guard terminates the task.

Example:

```lua
print("blink")

while true do
    led_toggle(1)
    delay_ms(500)
end
```

More examples are available in [examples/lua](examples/lua).

## Importing into an STM32 Project

1. Back up the target STM32F411 project.
2. Copy the entire `firmware/Libraries/lua` directory to `Libraries/lua` in the target project.
3. Copy the files from `firmware/User` to the corresponding location in the target project, then review the GPIO, UART, and timer definitions in `Config.h`.
4. On case-sensitive platforms, keep only `User/Config.h`; do not retain an older `User/config.h` at the same time.
5. Add all 28 `.c` files under `Libraries/lua/code` to the Lua build group.
6. Add the 13 `.c` files under `firmware/User` to the application build group. Use `stm32f4xx_conf.h` and `stm32f4xx_it.h` from the base project.
7. Add the following preprocessor definitions:

   ```text
   STM32F411xE
   USE_STDPERIPH_DRIVER
   LUA_USE_C89
   LUA_USE_JUMPTABLE=0
   LUA_MCU_NO_FILE
   ```

8. Add the following include paths:

   ```text
   ./User
   ./Libraries/lua/code
   ./RTE/Device/STM32F411CEUx
   ./Libraries/CMSIS/Include
   ./Libraries/CMSIS/Device/ST/STM32F4xx/Include
   ./Libraries/STM32F4xx_StdPeriph_Driver/inc
   ```

9. Limit the firmware IROM region to a start address of `0x08000000` and a size of `0x00040000` so that the linker cannot use the Lua slots.
10. Perform a full rebuild and complete the [hardware verification checklist](docs/building.md#hardware-verification-checklist).

For more detailed porting instructions, see [firmware/README.md](firmware/README.md).

## Flash Layout

| Region | Start address | Size | STM32F411 sectors |
| --- | --- | --- | --- |
| Firmware IROM | `0x08000000` | 256 KiB | Sectors 0–5 |
| Lua Slot A | `0x08040000` | 128 KiB | Sector 6 |
| Lua Slot B | `0x08060000` | 128 KiB | Sector 7 |

Each Lua slot begins with a 24-byte header containing the magic value, version, length, CRC32, update sequence, and state. The current maximum script length is 32 KiB. Valid versions range from `1` to `0x7FFFFFFF`, and a new version must be greater than every existing valid version.

Normal release configuration:

```c
#define APP_ERASE_LUA_SLOTS_ON_BOOT  0
```

Avoid **Erase Full Chip** when flashing firmware, because it erases both Lua slots.

## UART Update Protocol

An update consists of the following sessions:

```text
"LUA"
  -> BEGIN(version, length, crc32), seq=0
  -> DATA, seq=1..n
  -> END

"LUA"
  -> RUN, seq=0
```

The little-endian packet header is 6 bytes:

| Offset | Length | Field |
| ---: | ---: | --- |
| 0 | 1 | `cmd` |
| 1 | 1 | `seq` |
| 2 | 2 | `payload length` |
| 4 | 2 | `payload CRC16` |

Command values are `BEGIN=0x01`, `DATA=0x02`, `END=0x03`, and `RUN=0x04`. CRC16 uses CCITT-FALSE with an initial value of `0xFFFF` and polynomial `0x1021`. During `END`, the firmware also verifies the complete script with CRC32/IEEE and performs a Lua text syntax preflight check.

ACK/ERR uses a fixed 8-byte binary frame:

```text
"AK" + protocol_version + seq + int16(status) + crc16
```

`status == 0` indicates success. Protocol error codes:

| Status | Meaning |
| ---: | --- |
| `-100` | Invalid argument |
| `-101` | Invalid packet header |
| `-102` | Invalid length |
| `-103` | CRC16 mismatch |
| `-104` | Invalid sequence number |
| `-105` | Receive timeout |
| `-106` | Unsupported command |

See [docs/protocol.md](docs/protocol.md) for the complete protocol specification.

## PC Serial Update Tool

Python 3.11 or a compatible version is required. A virtual environment is recommended:

```powershell
cd tools/uart_updater
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
python main.py
```

Default settings:

| Setting | Default |
| --- | --- |
| Baud rate | 9600 |
| DATA payload | 64 bytes, 256 bytes maximum |
| Inter-byte delay | 1.5 ms |
| ACK timeout | 8 s |
| Maximum retries | 2 |
| Maximum script size | 32 KiB |
| Per-packet CRC16 | Always enabled |

The tool supports serial monitoring, text/HEX display, packet manifests, binary ACK parsing, retransmission after a lost ACK, and timeout resynchronization. See [tools/uart_updater/README.md](tools/uart_updater/README.md) for details.

## Testing

Run the PC protocol regression tests:

```powershell
cd tools/uart_updater
python -B -m unittest discover -s . -p "test_*.py" -v
```

The tests cover CRC vectors, the little-endian BEGIN layout, 8-byte ACK frames, ACK detection in mixed log output, RUN session sequence reset, and script-size limits. GitHub Actions also runs this test suite for changes related to the PC tool or protocol.

Before a hardware release, also verify invalid CRC handling, sequence-number jumps, lost ACKs, receive timeouts, interrupted updates, power loss during updates, retention across resets, the recovery key, and long-duration operation.

## Documentation

- [System architecture](docs/architecture.md)
- [Porting, build, and testing](docs/building.md)
- [UART update protocol](docs/protocol.md)
- [Firmware integration guide](firmware/README.md)
- [Lua MCU source guide](firmware/Libraries/lua/README.md)
- [Security policy](SECURITY.md)
- [Third-party notices](THIRD_PARTY_NOTICES.md)
- [Contribution guidelines](CONTRIBUTING.md)

## Security Boundaries and Limitations

- CRC16 and CRC32 detect transmission errors only; they do not authenticate the source of a script.
- The current design does not provide digital signatures, secure boot, device authentication, encryption, or complete replay protection.
- Any party with access to an update UART can replace the Lua script stored on the device.
- A/B fallback handles detectable Lua load and runtime failures, but there is no complete `PENDING_TEST/CONFIRMED` flow with watchdog-reset confirmation.
- The software UART has lower throughput and noise immunity than a hardware USART.
- Version numbers can only increase; forced replacement with the same version is not supported.
- Lua scripts should be treated as code with permission to control the device.

## License

- Original project code and documentation: root [MIT License](LICENSE).
- Original Lua 5.4.8 source: [Lua MIT License](firmware/Libraries/lua/LICENSE.txt).
- PyQt5: GPLv3 or a Riverbank commercial license; the dependency is not distributed with this repository.
- CMSIS, STM32 SPL, Keil, and device packs are not distributed with this repository. Users must comply with their respective licenses.

See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for the complete scope.
