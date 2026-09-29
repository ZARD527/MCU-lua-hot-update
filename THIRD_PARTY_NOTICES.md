# Third-party notices

This repository distributes a Lua MCU integration package, examples and a PC updater. The root MIT license applies only to project-owned code and documentation; it does not replace or override a third-party license.

## Lua 5.4.8

- Bundled location: `firmware/Libraries/lua/code/`
- Upstream: <https://www.lua.org/>
- License: MIT
- Copyright: 1994–2025 Lua.org, PUC-Rio
- License copy: `firmware/Libraries/lua/LICENSE.txt`

The bundle contains the complete Lua source subset used by this MCU project, plus project-owned port/update files. It is intentionally configured for embedded use and does not include every desktop standard-library source module from the official Lua distribution. Lua-originated files remain under the Lua MIT license; project-owned integration files remain under the root project license.

## ARM CMSIS Core

- Required by the hardware base project; not bundled here.
- Upstream/vendor source must retain its own copyright and license notices.
- The tested base used CMSIS Cortex-M4 headers compatible with `core_cm4.h` V4.10.

## STM32F4 CMSIS device and startup files

- Required by the hardware base project; not bundled here.
- Tested device: STM32F411CEUx.
- Tested Keil device pack: `Keil.STM32F4xx_DFP.2.16.0`.
- Obtain through Keil/ST under the applicable package terms.

## STM32F4 Standard Peripheral Library

- Required by the tested hardware base project; not bundled here.
- Tested source headers identify STM32F4 SPL V1.8.1.
- Vendor: STMicroelectronics.
- The tested package contains the ST `SLA0044` license.

Important: `SLA0044` restricts covered components to use on, or in combination with, microcontrollers or microprocessors manufactured by or for STMicroelectronics. Users must retain vendor notices and verify the license shipped with the exact package they obtain.

## Python runtime dependencies

The packages below are declared in `tools/uart_updater/requirements.txt` and are installed separately; their source or wheels are not bundled here.

- PyQt5: GPLv3 or Riverbank Commercial License. See <https://www.riverbankcomputing.com/software/pyqt>.
- pyserial: BSD-3-Clause. See <https://github.com/pyserial/pyserial>.

Review dependency licenses before redistributing a packaged executable. In particular, a proprietary distribution using PyQt5 generally requires an appropriate Riverbank commercial license or compliance with the GPL version.

## Development tools

Keil MDK, ARM Compiler 5 and the STM32F4 device pack are not distributed by this repository. They must be obtained and used under their own terms.

