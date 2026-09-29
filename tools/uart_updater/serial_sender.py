# -*- coding: utf-8 -*-
"""Serial transport helper for Lua UART update packets."""

from __future__ import annotations

import struct
import time
from dataclasses import dataclass
from typing import Callable, Iterable, Optional

from packetizer import LUA_PREFIX, LUA_UART_CMD_RUN, LuaPacket, crc16_ccitt_false


ACK_MAGIC = b"AK"
ACK_VERSION = 1
ACK_STRUCT = struct.Struct("<2sBBhH")

MCU_ERR_CRC16 = -103


@dataclass(frozen=True)
class SerialOptions:
    port: str
    baudrate: int = 9600
    timeout: float = 1.0
    ack_timeout: float = 8.0
    inter_packet_delay: float = 0.05
    inter_byte_delay: float = 0.0015
    open_settle_delay: float = 0.30
    prefix_delay: float = 0.20
    prefix_once: bool = True
    prefix_each_packet: bool = False
    wait_ack: bool = True
    max_retries: int = 2


class SerialDependencyError(RuntimeError):
    pass


def list_serial_ports() -> list[str]:
    try:
        from serial.tools import list_ports
    except ImportError as exc:
        raise SerialDependencyError("pyserial is not installed") from exc
    return [port.device for port in list_ports.comports()]


def build_ack_frame(seq: int, status: int) -> bytes:
    """Build an ACK frame for tests and protocol diagnostics."""
    prefix = struct.pack("<2sBBh", ACK_MAGIC, ACK_VERSION, seq & 0xFF, status)
    return prefix + struct.pack("<H", crc16_ccitt_false(prefix))


def extract_ack(buffer: bytearray, expected_seq: int) -> Optional[int]:
    """Extract one valid ACK for *expected_seq* from a mixed log/binary stream."""
    while True:
        magic_at = buffer.find(ACK_MAGIC)
        if magic_at < 0:
            if len(buffer) > 1:
                del buffer[:-1]
            return None
        if magic_at > 0:
            del buffer[:magic_at]
        if len(buffer) < ACK_STRUCT.size:
            return None

        frame = bytes(buffer[: ACK_STRUCT.size])
        magic, version, seq, status, received_crc = ACK_STRUCT.unpack(frame)
        if (
            magic != ACK_MAGIC
            or version != ACK_VERSION
            or crc16_ccitt_false(frame[:6]) != received_crc
        ):
            del buffer[0]
            continue

        del buffer[: ACK_STRUCT.size]
        if seq != (expected_seq & 0xFF):
            continue
        return status


def send_packets(
    packets: Iterable[LuaPacket],
    options: SerialOptions,
    on_log: Optional[Callable[[str], None]] = None,
    on_rx: Optional[Callable[[bytes], None]] = None,
    on_tx: Optional[Callable[[bytes], None]] = None,
) -> None:
    try:
        import serial
    except ImportError as exc:
        raise SerialDependencyError("pyserial is not installed") from exc

    packet_list = list(packets)
    if options.max_retries < 0:
        raise ValueError("max_retries must be >= 0")
    if options.prefix_each_packet:
        raise ValueError("prefix_each_packet is not supported by the MCU session protocol")

    def log(message: str) -> None:
        if on_log:
            on_log(message)

    def write_bytes(ser, data: bytes) -> None:
        if options.inter_byte_delay <= 0:
            ser.write(data)
            ser.flush()
            if on_tx:
                on_tx(data)
            return
        for byte in data:
            ser.write(bytes((byte,)))
            ser.flush()
            time.sleep(options.inter_byte_delay)
        if on_tx:
            on_tx(data)

    def send_prefix(ser) -> None:
        write_bytes(ser, LUA_PREFIX)
        log("sent prefix LUA")
        time.sleep(options.prefix_delay)

    with serial.Serial(options.port, options.baudrate, timeout=options.timeout) as ser:
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        time.sleep(options.open_settle_delay)

        for packet_index, packet in enumerate(packet_list):
            needs_initial_prefix = (
                options.prefix_each_packet
                or packet.cmd == LUA_UART_CMD_RUN
                or (packet_index == 0 and options.prefix_once)
            )
            retry_needs_prefix = False

            for attempt in range(options.max_retries + 1):
                if attempt == 0 and needs_initial_prefix:
                    send_prefix(ser)
                elif attempt > 0 and (options.prefix_each_packet or retry_needs_prefix):
                    send_prefix(ser)

                write_bytes(ser, packet.frame)
                log(
                    f"sent seq={packet.seq} cmd=0x{packet.cmd:02X} "
                    f"len={packet.length} attempt={attempt + 1}"
                )

                if not options.wait_ack:
                    break

                deadline = time.monotonic() + options.ack_timeout
                receive_buffer = bytearray()
                status: Optional[int] = None

                while time.monotonic() < deadline:
                    waiting = ser.in_waiting
                    chunk = ser.read(waiting or 1)
                    if chunk:
                        receive_buffer.extend(chunk)
                        if on_rx:
                            on_rx(chunk)
                        status = extract_ack(receive_buffer, packet.seq)
                        if status is not None:
                            break
                    else:
                        time.sleep(0.01)

                if status == 0:
                    log(f"ack ok: seq={packet.seq} ret=0")
                    break

                if status is None:
                    # First retry stays in the current MCU session (typical lost ACK).
                    # A later retry sends a fresh prefix so a reset/timed-out MCU can resync.
                    retry_needs_prefix = attempt >= 1
                    reason = f"ACK timeout at seq={packet.seq}"
                else:
                    retry_needs_prefix = False
                    reason = f"MCU rejected packet seq={packet.seq}, ret={status}"

                can_retry = attempt < options.max_retries and (
                    status is None or status == MCU_ERR_CRC16
                )
                if not can_retry:
                    if status is None:
                        raise TimeoutError(reason)
                    raise RuntimeError(reason)

                log(f"retry: {reason}")
                ser.reset_input_buffer()
                time.sleep(options.inter_packet_delay)
            else:  # pragma: no cover - defensive; the loop exits via break/raise.
                raise RuntimeError(f"send loop exhausted at seq={packet.seq}")

            time.sleep(options.inter_packet_delay)
