# Integrating, building and testing

## Package boundary

`firmware/` includes:

- all application/project files changed by this port;
- the complete Lua 5.4.8 MCU source directory used by the tested project;
- MCU allocator/output integration and Flash A/B update modules.

It intentionally omits unmodified CMSIS, STM32 SPL, startup/system and board-base files. Therefore `firmware/Hello_World.uvprojx` is a tested configuration reference rather than a standalone project in this repository.

## Hardware base prerequisites

- STM32F411CEU6/STM32F411CEUx CMSIS device support and startup/system files.
- STM32F4 Standard Peripheral Library compatible with V1.8.1.
- `User/stm32f4xx_conf.h` and `User/stm32f4xx_it.h` from the base project.
- Keil MDK-ARM with ARM Compiler 5.06 update 5 (build 528).
- Keil STM32F4xx Device Family Pack 2.16.0.

Lua source does not need to be acquired separately; use the bundled `firmware/Libraries/lua` directory. Platform dependencies must be obtained through official or otherwise authorized channels with their licenses retained.

## Integration and firmware build

1. Back up the complete hardware base project.
2. Copy `firmware/Libraries/lua` as a whole into the base project.
3. Copy the included `firmware/User` files to the matching paths after reviewing board pin definitions.
4. Review `firmware/Hello_World.uvprojx` instead of blindly replacing a differently structured project file.
5. Add every `.c` file under `Libraries/lua/code` to a Keil Lua source group.
6. Apply the compiler symbols, include paths and IROM boundary listed in `firmware/README.md`.
7. Run **Rebuild all target files** and review every warning.
8. Download without full-chip erase if existing Lua A/B slots must be retained.

The reference project was verified with `0 Error(s), 2 Warning(s)`. Both warnings are unused helpers in the bundled Lua core (`lbaselib.c` and `lfunc.c`).

## PC updater

Use a virtual environment:

```powershell
cd tools/uart_updater
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
python main.py
```

PyQt5 is GPLv3/commercial dual-licensed. Read `THIRD_PARTY_NOTICES.md` before redistributing a packaged updater.

## Protocol tests

```powershell
cd tools/uart_updater
python -B -m unittest discover -s . -p "test_*.py" -v
```

## Hardware verification checklist

- Boot with both Lua slots empty.
- Send and run `examples/lua/01_led_on.lua`.
- Send a higher version and run `02_blink_key.lua`.
- Reset and verify the active script remains present.
- Inject a bad CRC16 and verify rejection.
- Drop an ACK and verify retransmission is idempotent.
- Interrupt a DATA transfer and verify timeout recovery.
- Remove power during update and verify the previous valid slot remains selectable.
- Hold PA0 during boot and verify the recovery path skips automatic script execution.

