# Security policy

## Project status

This is an educational embedded project, not a secure or production-grade OTA system. Only the latest repository state is maintained; no long-term security support or response deadline is promised.

## Threat model

The current design assumes that the physical UART link and the person operating the updater are trusted. CRC16 and CRC32 detect accidental corruption only. They do not provide origin authentication, anti-tamper protection or confidentiality.

Known security limitations include:

- no signed manifest or public-key signature verification;
- no secure boot or firmware authenticity chain;
- no encrypted or authenticated transport;
- no device identity, authorization or replay-resistant monotonic counter;
- only a partial A/B fallback model, without a complete confirmed-boot state machine;
- Lua scripts are privileged device logic and can call the registered hardware APIs;
- denial of service remains possible through physical serial access or resource-heavy scripts.

Do not expose the update UART to an untrusted network or connector without an authenticated gateway. Do not rely on CRC values as a security control.

## Reporting a vulnerability

Use the repository host's private security-advisory function when available. Include the affected revision, hardware, reproduction steps, impact and any proposed mitigation. Avoid publishing working exploits or device-specific secrets before maintainers have had a reasonable opportunity to assess the report.

If private reporting is unavailable, open a minimal public issue requesting a private contact channel and omit exploit details.

## Before production use

At minimum, add signed manifests, anti-rollback policy, secure boot integration, authenticated transport, watchdog-backed trial boot/confirmation, strict script resource limits and hardware fault-injection/power-loss testing. Obtain an independent security review for the final product.

