# -*- coding: utf-8 -*-
"""Host-side regression tests for the Lua UART protocol."""

from __future__ import annotations

import struct
import tempfile
import unittest
from pathlib import Path

from packetizer import (
    BEGIN_PAYLOAD_STRUCT,
    HEADER_STRUCT,
    LUA_UART_CMD_BEGIN,
    LUA_UART_CMD_RUN,
    MAX_SCRIPT_SIZE,
    PacketError,
    build_lua_update_packets,
    crc16_ccitt_false,
    crc32_ieee,
)
from serial_sender import build_ack_frame, extract_ack


class ProtocolTests(unittest.TestCase):
    def _script(self, content: bytes = b"print('ok')\n"):
        directory = tempfile.TemporaryDirectory()
        path = Path(directory.name) / "script.lua"
        path.write_bytes(content)
        return directory, path

    def test_crc_golden_vectors(self) -> None:
        data = b"123456789"
        self.assertEqual(crc16_ccitt_false(data), 0x29B1)
        self.assertEqual(crc32_ieee(data), 0xCBF43926)

    def test_begin_packet_is_little_endian(self) -> None:
        directory, path = self._script()
        with directory:
            result = build_lua_update_packets(path, version=7)
        begin = result.packets[0]
        self.assertEqual(begin.cmd, LUA_UART_CMD_BEGIN)
        self.assertEqual(len(begin.header), HEADER_STRUCT.size)
        version, length, crc32 = BEGIN_PAYLOAD_STRUCT.unpack(begin.payload)
        self.assertEqual(version, 7)
        self.assertEqual(length, result.script_size)
        self.assertEqual(crc32, result.script_crc32)

    def test_run_starts_a_new_sequence(self) -> None:
        directory, path = self._script()
        with directory:
            result = build_lua_update_packets(path, version=1, add_run_packet=True)
        run = result.packets[-1]
        self.assertEqual(run.cmd, LUA_UART_CMD_RUN)
        self.assertEqual(run.seq, 0)
        self.assertEqual(run.crc16, 0xFFFF)

    def test_script_size_limit(self) -> None:
        directory, path = self._script(b"x" * (MAX_SCRIPT_SIZE + 1))
        with directory:
            with self.assertRaises(PacketError):
                build_lua_update_packets(path, version=1)

    def test_ack_parser_ignores_logs_and_bad_crc(self) -> None:
        bad = bytearray(build_ack_frame(3, 0))
        bad[-1] ^= 0x01
        stream = bytearray(b"debug line\r\n" + bad + build_ack_frame(3, -103))
        self.assertEqual(extract_ack(stream, 3), -103)

    def test_ack_layout_is_eight_bytes(self) -> None:
        frame = build_ack_frame(255, -5)
        self.assertEqual(len(frame), 8)
        self.assertEqual(struct.unpack("<2sBBh", frame[:6]), (b"AK", 1, 255, -5))


if __name__ == "__main__":
    unittest.main()
