# UART update protocol

All multi-byte integers are little-endian.

## Session prefix

Each update or standalone RUN session starts with the three ASCII bytes:

```text
4C 55 41    "LUA"
```

## Packet header

The packed wire layout is six bytes:

| Offset | Size | Field |
| ---: | ---: | --- |
| 0 | 1 | command |
| 1 | 1 | sequence number |
| 2 | 2 | payload length |
| 4 | 2 | payload CRC16 |

CRC16 uses CRC-16/CCITT-FALSE: initial value `0xFFFF`, polynomial `0x1021`, no reflection and no final XOR. The CRC of an empty payload is `0xFFFF`.

## Commands

| Value | Name | Payload |
| ---: | --- | --- |
| `0x01` | BEGIN | `version:u32, length:u32, crc32:u32` |
| `0x02` | DATA | 1–256 Lua source bytes |
| `0x03` | END | empty |
| `0x04` | RUN | empty |

BEGIN starts at sequence 0. Each successfully processed packet increments the expected sequence. A standalone RUN opens a new `LUA` session and uses sequence 0.

The full Lua source CRC is CRC32/IEEE. The maximum accepted script size is 32 KiB. Versions must increase relative to existing valid slots.

## ACK/ERR frame

Every result uses the same eight-byte binary frame:

| Offset | Size | Field |
| ---: | ---: | --- |
| 0 | 2 | ASCII `AK` |
| 2 | 1 | protocol version (`1`) |
| 3 | 1 | sequence number |
| 4 | 2 | signed status (`int16`) |
| 6 | 2 | CRC16 of bytes 0–5 |

Status 0 means success. Defined protocol errors include:

| Status | Meaning |
| ---: | --- |
| `-100` | invalid argument |
| `-101` | invalid header |
| `-102` | invalid length |
| `-103` | CRC16 mismatch |
| `-104` | unexpected sequence |
| `-105` | receive timeout |
| `-106` | unsupported command |

Flash/update functions can return additional nonzero application results. The sender must stop the current transaction after a nonzero result unless it is intentionally retransmitting the identical last packet to recover a lost ACK.

## Recovery behavior

- Header or payload inactivity for 12 seconds aborts the transaction.
- An invalid header clears pending receive data and aborts the update.
- An exact duplicate of the last processed packet returns the cached result without repeating the Flash operation.
- Text logs and binary ACK bytes can share an output stream; the PC parser searches for valid `AK` frames and verifies their CRC.

