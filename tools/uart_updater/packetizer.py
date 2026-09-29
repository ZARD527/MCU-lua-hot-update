# -*- coding: utf-8 -*-
"""Lua UART update packet builder.

This module contains the protocol/business logic only.  The GUI imports this
module but the packet format, CRC calculation, and file splitting live here.
"""

from __future__ import annotations

import json
import struct
import zlib
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, List


LUA_PREFIX = b"LUA"

LUA_UART_CMD_BEGIN = 0x01
LUA_UART_CMD_DATA = 0x02
LUA_UART_CMD_END = 0x03
LUA_UART_CMD_RUN = 0x04

HEADER_STRUCT = struct.Struct("<BBHH")
BEGIN_PAYLOAD_STRUCT = struct.Struct("<III")

MAX_PAYLOAD_SIZE = 256
MAX_SCRIPT_SIZE = 32 * 1024
MAX_SCRIPT_VERSION = 0x7FFFFFFF
DEFAULT_CHUNK_SIZE = 64


class PacketError(ValueError):
    """Raised when a packet cannot be built from the supplied options."""


@dataclass(frozen=True)
class LuaPacket:
    cmd: int
    seq: int
    payload: bytes
    crc16: int

    @property
    def length(self) -> int:
        return len(self.payload)

    @property
    def header(self) -> bytes:
        return HEADER_STRUCT.pack(self.cmd, self.seq & 0xFF, self.length, self.crc16)

    @property
    def frame(self) -> bytes:
        return self.header + self.payload

    def to_dict(self) -> dict:
        return {
            "cmd": self.cmd,
            "seq": self.seq,
            "len": self.length,
            "crc16": f"0x{self.crc16:04X}",
        }


@dataclass(frozen=True)
class LuaBuildResult:
    source_path: Path
    version: int
    script_size: int
    script_crc32: int
    chunk_size: int
    packets: List[LuaPacket]

    @property
    def frame_count(self) -> int:
        return len(self.packets)

    def to_stream(self, prefix_once: bool = True, prefix_each_packet: bool = False) -> bytes:
        if prefix_each_packet:
            raise PacketError("prefix_each_packet is not supported by the MCU session protocol")
        stream = bytearray()
        if prefix_once:
            stream.extend(LUA_PREFIX)
        for packet in self.packets:
            if packet.cmd == LUA_UART_CMD_RUN:
                stream.extend(LUA_PREFIX)
            stream.extend(packet.frame)
        return bytes(stream)

    def summary_json(self) -> str:
        data = {
            "source": str(self.source_path),
            "version": self.version,
            "script_size": self.script_size,
            "script_crc32": f"0x{self.script_crc32:08X}",
            "chunk_size": self.chunk_size,
            "frame_count": self.frame_count,
            "packets": [packet.to_dict() for packet in self.packets],
        }
        return json.dumps(data, ensure_ascii=False, indent=2)


def crc32_ieee(data: bytes) -> int:
    """CRC32/IEEE, compatible with zlib and the MCU software CRC32 design."""
    return zlib.crc32(data) & 0xFFFFFFFF


def crc16_ccitt_false(data: bytes) -> int:
    """CRC-16/CCITT-FALSE for per-packet payload checking."""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def _make_packet(cmd: int, seq: int, payload: bytes, use_packet_crc16: bool) -> LuaPacket:
    if not 0 <= seq <= 0xFF:
        raise PacketError("seq must be in range 0..255")
    if len(payload) > MAX_PAYLOAD_SIZE:
        raise PacketError(f"payload too large: {len(payload)} > {MAX_PAYLOAD_SIZE}")
    crc16 = crc16_ccitt_false(payload) if use_packet_crc16 else 0
    return LuaPacket(cmd=cmd, seq=seq, payload=payload, crc16=crc16)


def _chunks(data: bytes, chunk_size: int) -> Iterable[bytes]:
    for offset in range(0, len(data), chunk_size):
        yield data[offset : offset + chunk_size]


def build_lua_update_packets(
    source_path: str | Path,
    version: int,
    chunk_size: int = DEFAULT_CHUNK_SIZE,
    add_run_packet: bool = False,
    use_packet_crc16: bool = True,
) -> LuaBuildResult:
    path = Path(source_path)
    if not path.is_file():
        raise PacketError(f"file not found: {path}")
    if not 1 <= version <= MAX_SCRIPT_VERSION:
        raise PacketError(f"version must be in range 1..{MAX_SCRIPT_VERSION}")
    if not 1 <= chunk_size <= MAX_PAYLOAD_SIZE:
        raise PacketError(f"chunk_size must be in range 1..{MAX_PAYLOAD_SIZE}")

    script = path.read_bytes()
    if not script:
        raise PacketError("Lua script is empty")
    if len(script) > MAX_SCRIPT_SIZE:
        raise PacketError(f"Lua script is too large: {len(script)} > {MAX_SCRIPT_SIZE}")

    script_crc32 = crc32_ieee(script)
    packets: List[LuaPacket] = []
    seq = 0

    begin_payload = BEGIN_PAYLOAD_STRUCT.pack(version, len(script), script_crc32)
    packets.append(_make_packet(LUA_UART_CMD_BEGIN, seq, begin_payload, use_packet_crc16))
    seq = (seq + 1) & 0xFF

    for chunk in _chunks(script, chunk_size):
        packets.append(_make_packet(LUA_UART_CMD_DATA, seq, chunk, use_packet_crc16))
        seq = (seq + 1) & 0xFF

    packets.append(_make_packet(LUA_UART_CMD_END, seq, b"", use_packet_crc16))
    seq = (seq + 1) & 0xFF

    if add_run_packet:
        # RUN is a separate command session (it has its own LUA prefix).
        packets.append(_make_packet(LUA_UART_CMD_RUN, 0, b"", use_packet_crc16))

    return LuaBuildResult(
        source_path=path,
        version=version,
        script_size=len(script),
        script_crc32=script_crc32,
        chunk_size=chunk_size,
        packets=packets,
    )
