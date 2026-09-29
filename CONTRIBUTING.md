# Contributing

Contributions should keep the firmware build reproducible and preserve third-party license boundaries.

## Development checks

Before submitting a change:

1. Run the PC protocol tests:

   ```powershell
   cd tools/uart_updater
   python -B -m unittest discover -s . -p "test_*.py" -v
   ```

2. Import the firmware integration package, including the complete bundled Lua MCU directory, into a compatible hardware base project and rebuild it with the documented ARMCC and device-pack versions. Platform libraries remain external.
3. Test relevant behavior on an STM32F411 target.
4. Do not commit `Objects/`, `Listings/`, IDE user state, Python caches, virtual environments, generated packets, logs, device dumps, credentials or local absolute paths.
5. Update protocol and architecture documentation when wire formats, Flash layout or safety behavior changes.

## Source and license rules

- Keep project code separate from vendor and upstream code.
- Do not remove copyright or license notices.
- Record the origin and license of every new third-party component.
- Do not add STM32 SPL or other complete vendor dependency trees to this patch repository. If dependencies are distributed elsewhere, their original license continues to apply.
- Do not commit secrets, production signing keys, access tokens or customer data.

By contributing project-owned code, you agree that it may be distributed under the project MIT license unless the file clearly states another compatible license.

