# -*- coding: utf-8 -*-
"""PyQt GUI for Lua UART update packet building and sending."""

from __future__ import annotations

import html
from pathlib import Path

from PyQt5.QtCore import Qt, QThread, QTimer, pyqtSignal
from PyQt5.QtGui import QFont
from PyQt5.QtWidgets import (
    QCheckBox,
    QComboBox,
    QFileDialog,
    QFormLayout,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMainWindow,
    QMessageBox,
    QPushButton,
    QPlainTextEdit,
    QSpinBox,
    QSplitter,
    QTextEdit,
    QVBoxLayout,
    QWidget,
)

from packetizer import (
    DEFAULT_CHUNK_SIZE,
    MAX_PAYLOAD_SIZE,
    LuaBuildResult,
    PacketError,
    build_lua_update_packets,
)
from serial_sender import SerialDependencyError, SerialOptions, list_serial_ports, send_packets


class SendWorker(QThread):
    log = pyqtSignal(str, str)
    rx = pyqtSignal(bytes)
    tx = pyqtSignal(bytes)
    done = pyqtSignal()
    failed = pyqtSignal(str)

    def __init__(self, build: LuaBuildResult, options: SerialOptions):
        super().__init__()
        self._build = build
        self._options = options

    def run(self) -> None:
        try:
            send_packets(self._build.packets, self._options, self._log, self.rx.emit, self.tx.emit)
            self.done.emit()
        except Exception as exc:  # GUI boundary: show all transport errors.
            self.failed.emit(str(exc))

    def _log(self, message: str) -> None:
        if message.startswith("sent "):
            kind = "tx"
        elif message.startswith("send failed"):
            kind = "err"
        else:
            kind = "info"
        self.log.emit(kind, message)


class MonitorWorker(QThread):
    rx = pyqtSignal(bytes)
    log = pyqtSignal(str, str)
    failed = pyqtSignal(str)

    def __init__(self, port: str, baudrate: int):
        super().__init__()
        self._port = port
        self._baudrate = baudrate
        self._running = True

    def stop(self) -> None:
        self._running = False

    def run(self) -> None:
        try:
            import serial
        except ImportError:
            self.failed.emit("pyserial is not installed")
            return

        try:
            with serial.Serial(self._port, self._baudrate, timeout=0.1) as ser:
                ser.reset_input_buffer()
                ser.reset_output_buffer()
                self.log.emit("info", f"monitor open {self._port} @ {self._baudrate} 8N1")
                while self._running:
                    waiting = ser.in_waiting
                    data = ser.read(waiting or 1)
                    if data:
                        time_to_collect = 0.01
                        self.msleep(int(time_to_collect * 1000))
                        waiting = ser.in_waiting
                        if waiting:
                            data += ser.read(waiting)
                        self.rx.emit(data)
                self.log.emit("info", "monitor closed")
        except Exception as exc:
            self.failed.emit(str(exc))


class MainWindow(QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("Lua MCU UART Update Tool")
        self.setFont(QFont("KaiTi", 11, QFont.Bold))
        self._build: LuaBuildResult | None = None
        self._worker: SendWorker | None = None
        self._monitor: MonitorWorker | None = None
        self._serial_entries: list[tuple[str, str | bytes]] = []
        self._serial_hex_mode = False
        self._rx_buffer = bytearray()
        self._rx_flush_timer = QTimer(self)
        self._rx_flush_timer.setSingleShot(True)
        self._rx_flush_timer.timeout.connect(self._flush_rx_buffer)

        self.file_edit = QLineEdit()
        self.file_edit.setReadOnly(True)
        self.pick_button = QPushButton("选择 Lua 文件")
        self.pick_button.clicked.connect(self.pick_file)

        self.version_spin = QSpinBox()
        self.version_spin.setRange(1, 0x7FFFFFFF)
        self.version_spin.setValue(1)

        self.chunk_spin = QSpinBox()
        self.chunk_spin.setRange(1, MAX_PAYLOAD_SIZE)
        self.chunk_spin.setValue(DEFAULT_CHUNK_SIZE)

        self.run_check = QCheckBox("END 后追加 RUN 包")
        self.crc16_check = QCheckBox("启用每包 CRC16")
        self.crc16_check.setChecked(True)
        self.crc16_check.setEnabled(False)
        self.crc16_check.setToolTip("工程协议要求每个包都携带 CRC16")

        self.prefix_once_check = QCheckBox("发送流开头加 LUA 前缀")
        self.prefix_once_check.setChecked(True)
        self.prefix_each_check = QCheckBox("每包都加 LUA 前缀")
        self.prefix_each_check.setEnabled(False)
        self.prefix_each_check.setToolTip("当前协议只在 BEGIN 会话和独立 RUN 会话前发送前缀")

        self.port_combo = QComboBox()
        self.refresh_ports_button = QPushButton("刷新串口")
        self.refresh_ports_button.clicked.connect(self.refresh_ports)

        self.baud_combo = QComboBox()
        self.baud_combo.setEditable(True)
        self.baud_combo.addItems(["9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"])
        self.baud_combo.setCurrentText("9600")

        self.monitor_check = QCheckBox("开启串口监听")
        self.monitor_check.toggled.connect(self.toggle_monitor)

        self.ack_check = QCheckBox("等待 ACK")
        self.ack_check.setChecked(True)

        self.build_button = QPushButton("生成包")
        self.build_button.clicked.connect(self.build_packets)
        self.save_bin_button = QPushButton("保存 bin")
        self.save_bin_button.clicked.connect(self.save_binary)
        self.save_json_button = QPushButton("保存清单")
        self.save_json_button.clicked.connect(self.save_manifest)
        self.send_button = QPushButton("串口发送")
        self.send_button.clicked.connect(self.send_serial)

        self.summary = QPlainTextEdit()
        self.summary.setReadOnly(True)
        self.serial_output = QTextEdit()
        self.serial_output.setReadOnly(True)
        self.serial_clear_button = QPushButton("清空")
        self.serial_clear_button.clicked.connect(self.clear_serial_output)
        self.serial_format_button = QPushButton("文本")
        self.serial_format_button.setCheckable(True)
        self.serial_format_button.clicked.connect(self.toggle_serial_format)

        self._layout()
        self._style()
        self.refresh_ports()

    def _layout(self) -> None:
        file_row = QHBoxLayout()
        file_row.addWidget(self.file_edit, 1)
        file_row.addWidget(self.pick_button)

        build_box = QGroupBox("打包")
        build_form = QFormLayout(build_box)
        build_form.addRow("Lua 文件", file_row)
        build_form.addRow("版本号", self.version_spin)
        build_form.addRow("DATA payload 字节", self.chunk_spin)
        build_form.addRow(self.run_check)
        build_form.addRow(self.crc16_check)
        build_form.addRow(self.prefix_once_check)
        build_form.addRow(self.prefix_each_check)

        serial_box = QGroupBox("串口")
        serial_grid = QGridLayout(serial_box)
        serial_grid.addWidget(QLabel("端口"), 0, 0)
        serial_grid.addWidget(self.port_combo, 0, 1)
        serial_grid.addWidget(self.refresh_ports_button, 0, 2)
        serial_grid.addWidget(QLabel("波特率"), 1, 0)
        serial_grid.addWidget(self.baud_combo, 1, 1)
        serial_grid.addWidget(self.monitor_check, 2, 0, 1, 2)
        serial_grid.addWidget(self.ack_check, 3, 0, 1, 2)

        button_row = QHBoxLayout()
        button_row.addWidget(self.build_button)
        button_row.addWidget(self.save_bin_button)
        button_row.addWidget(self.save_json_button)
        button_row.addWidget(self.send_button)

        controls_layout = QVBoxLayout()
        controls_layout.addWidget(build_box)
        controls_layout.addWidget(serial_box)
        controls_layout.addLayout(button_row)
        controls_layout.addStretch(1)

        controls_widget = QWidget()
        controls_widget.setLayout(controls_layout)

        summary_box = QGroupBox("包清单")
        summary_layout = QVBoxLayout(summary_box)
        summary_layout.addWidget(self.summary)

        serial_output_box = QGroupBox("串口输出")
        serial_output_layout = QVBoxLayout(serial_output_box)
        serial_tools = QHBoxLayout()
        serial_tools.addStretch(1)
        serial_tools.addWidget(self.serial_clear_button)
        serial_tools.addWidget(self.serial_format_button)
        serial_output_layout.addLayout(serial_tools)
        serial_output_layout.addWidget(self.serial_output)

        output_splitter = QSplitter(Qt.Horizontal)
        output_splitter.addWidget(summary_box)
        output_splitter.addWidget(serial_output_box)
        output_splitter.setStretchFactor(0, 1)
        output_splitter.setStretchFactor(1, 1)

        main_splitter = QSplitter(Qt.Vertical)
        main_splitter.addWidget(controls_widget)
        main_splitter.addWidget(output_splitter)
        main_splitter.setStretchFactor(0, 0)
        main_splitter.setStretchFactor(1, 1)
        main_splitter.setSizes([360, 360])

        root = QVBoxLayout()
        root.addWidget(main_splitter)

        widget = QWidget()
        widget.setLayout(root)
        self.setCentralWidget(widget)
        self.resize(940, 760)

    def _style(self) -> None:
        self.setStyleSheet("""
            QMainWindow {
                background: #EEF3EF;
            }
            QWidget {
                color: #3F4A45;
                font-family: "KaiTi", "STKaiti", "Microsoft YaHei";
                font-size: 14px;
                font-weight: 700;
            }
            QGroupBox {
                background: #F8FAF7;
                border: 1px solid #C8D6CD;
                border-radius: 8px;
                margin-top: 12px;
                padding: 14px 12px 12px 12px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 14px;
                padding: 0 8px;
                color: #5E7469;
                background: #EEF3EF;
                font-size: 16px;
            }
            QLabel {
                color: #55645E;
                min-height: 24px;
            }
            QLineEdit, QComboBox, QSpinBox {
                min-height: 30px;
                padding: 4px 8px;
                border: 1px solid #B7C8BD;
                border-radius: 6px;
                background: #FFFFFF;
                selection-background-color: #A8BFAE;
            }
            QLineEdit:focus, QComboBox:focus, QSpinBox:focus {
                border: 2px solid #8CA796;
                background: #FBFDFB;
            }
            QComboBox::drop-down {
                width: 28px;
                border-left: 1px solid #C8D6CD;
                border-top-right-radius: 6px;
                border-bottom-right-radius: 6px;
                background: #EDF4EF;
            }
            QComboBox::down-arrow {
                width: 0;
                height: 0;
                border-left: 5px solid transparent;
                border-right: 5px solid transparent;
                border-top: 6px solid #6F8E7A;
                margin-right: 8px;
            }
            QComboBox QAbstractItemView {
                border: 1px solid #B7C8BD;
                border-radius: 6px;
                background: #FFFFFF;
                selection-background-color: #D8E3DA;
                selection-color: #34443B;
                padding: 4px;
            }
            QSpinBox::up-button, QSpinBox::down-button {
                width: 24px;
                border-left: 1px solid #C8D6CD;
                background: #EDF4EF;
            }
            QSpinBox::up-button {
                border-top-right-radius: 6px;
            }
            QSpinBox::down-button {
                border-bottom-right-radius: 6px;
            }
            QSpinBox::up-arrow {
                width: 0;
                height: 0;
                border-left: 4px solid transparent;
                border-right: 4px solid transparent;
                border-bottom: 5px solid #6F8E7A;
            }
            QSpinBox::down-arrow {
                width: 0;
                height: 0;
                border-left: 4px solid transparent;
                border-right: 4px solid transparent;
                border-top: 5px solid #6F8E7A;
            }
            QPushButton {
                min-height: 32px;
                padding: 6px 14px;
                border: 1px solid #9DB2A6;
                border-radius: 6px;
                background: #D8E3DA;
                color: #34443B;
            }
            QPushButton:hover {
                background: #CADBCF;
                border-color: #829A8E;
            }
            QPushButton:pressed {
                background: #B9CEC0;
            }
            QPushButton:disabled {
                color: #8D9993;
                background: #E2E7E3;
                border-color: #CDD6D0;
            }
            QCheckBox {
                min-height: 28px;
                spacing: 8px;
            }
            QCheckBox::indicator {
                width: 18px;
                height: 18px;
                border: 1px solid #9DB2A6;
                border-radius: 4px;
                background: #FFFFFF;
            }
            QCheckBox::indicator:checked {
                background: #8FAE99;
                border-color: #6F8E7A;
            }
            QPlainTextEdit, QTextEdit {
                border: 1px solid #C1D0C7;
                border-radius: 8px;
                background: #FCF7F1;
                color: #45514B;
                padding: 8px;
                font-family: "KaiTi", "STKaiti", "Consolas";
                font-size: 14px;
                font-weight: 700;
            }
            QSplitter::handle {
                background: #D4E0D7;
            }
            QSplitter::handle:hover {
                background: #AFC4B6;
            }
        """)

    def _baudrate(self) -> int:
        try:
            return int(self.baud_combo.currentText().strip())
        except ValueError:
            raise ValueError("波特率必须是数字")

    def _selected_port(self) -> str:
        port = self.port_combo.currentText().strip()
        if not port:
            raise ValueError("没有选择串口")
        return port

    def _format_bytes(self, data: bytes) -> str:
        return " ".join(f"{byte:02X}" for byte in data)

    def _append_serial_entry(self, kind: str, payload: str | bytes) -> None:
        self._serial_entries.append((kind, payload))
        self._render_serial_entry(kind, payload)

    def _render_serial_entry(self, kind: str, payload: str | bytes) -> None:
        colors = {
            "tx": "#7A6A2F",
            "tx_raw": "#9A7B22",
            "rx": "#2F6F73",
            "info": "#55645E",
            "err": "#B35F5A",
        }
        labels = {
            "tx": "TX",
            "tx_raw": "TX",
            "rx": "RX",
            "info": "INFO",
            "err": "ERR",
        }
        color = colors.get(kind, "#45514B")
        label = labels.get(kind, "LOG")

        if isinstance(payload, bytes):
            if self._serial_hex_mode:
                body = self._format_bytes(payload)
            elif kind == "tx_raw":
                body = f"{len(payload)} bytes"
            else:
                body = payload.decode(errors="replace").rstrip("\r\n")
        else:
            body = payload

        body = html.escape(body)
        if not self._serial_hex_mode:
            body = body.replace("\r\n", "\n").replace("\r", "\n").replace("\n", "<br>")
        self.serial_output.append(
            f'<span style="color:{color};">[{label}] {body}</span>'
        )

    def _rerender_serial_output(self) -> None:
        self._flush_rx_buffer()
        self.serial_output.clear()
        for kind, payload in self._serial_entries:
            self._render_serial_entry(kind, payload)

    def log_serial(self, kind: str, message: str) -> None:
        self._flush_rx_buffer()
        self._append_serial_entry(kind, message)

    def log_serial_rx(self, data: bytes) -> None:
        self._rx_buffer.extend(data)
        while True:
            try:
                newline_index = self._rx_buffer.index(0x0A)
            except ValueError:
                break
            line = bytes(self._rx_buffer[: newline_index + 1])
            del self._rx_buffer[: newline_index + 1]
            self._append_serial_entry("rx", line)
        if self._rx_buffer:
            self._rx_flush_timer.start(80)

    def _flush_rx_buffer(self) -> None:
        if self._rx_flush_timer.isActive():
            self._rx_flush_timer.stop()
        if self._rx_buffer:
            self._append_serial_entry("rx", bytes(self._rx_buffer))
            self._rx_buffer.clear()

    def log_serial_tx(self, data: bytes) -> None:
        self._flush_rx_buffer()
        self._append_serial_entry("tx_raw", data)

    def toggle_serial_format(self) -> None:
        self._serial_hex_mode = self.serial_format_button.isChecked()
        self.serial_format_button.setText("HEX" if self._serial_hex_mode else "文本")
        self._rerender_serial_output()

    def clear_serial_output(self) -> None:
        if self._rx_flush_timer.isActive():
            self._rx_flush_timer.stop()
        self._rx_buffer.clear()
        self._serial_entries.clear()
        self.serial_output.clear()

    def toggle_monitor(self, checked: bool) -> None:
        if checked:
            self._start_monitor()
        else:
            self._stop_monitor()

    def _start_monitor(self) -> None:
        if self._monitor is not None:
            return
        try:
            port = self._selected_port()
            baudrate = self._baudrate()
        except ValueError as exc:
            QMessageBox.warning(self, "串口错误", str(exc))
            self.monitor_check.blockSignals(True)
            self.monitor_check.setChecked(False)
            self.monitor_check.blockSignals(False)
            return

        self._monitor = MonitorWorker(port, baudrate)
        self._monitor.rx.connect(self.log_serial_rx)
        self._monitor.log.connect(self.log_serial)
        self._monitor.failed.connect(self._monitor_failed)
        self._monitor.start()

    def _stop_monitor(self) -> None:
        if self._monitor is None:
            return
        monitor = self._monitor
        self._monitor = None
        monitor.stop()
        monitor.wait(1500)

    def _monitor_failed(self, message: str) -> None:
        self._monitor = None
        self.log_serial("err", f"monitor failed: {message}")
        self.monitor_check.blockSignals(True)
        self.monitor_check.setChecked(False)
        self.monitor_check.blockSignals(False)

    def pick_file(self) -> None:
        path, _ = QFileDialog.getOpenFileName(self, "选择 Lua 脚本", "", "Lua (*.lua);;All Files (*)")
        if path:
            self.file_edit.setText(path)

    def refresh_ports(self) -> None:
        self.port_combo.clear()
        try:
            ports = list_serial_ports()
        except SerialDependencyError:
            ports = []
        self.port_combo.addItems(ports)

    def build_packets(self) -> None:
        try:
            self._build = build_lua_update_packets(
                self.file_edit.text(),
                version=self.version_spin.value(),
                chunk_size=self.chunk_spin.value(),
                add_run_packet=self.run_check.isChecked(),
                use_packet_crc16=self.crc16_check.isChecked(),
            )
        except PacketError as exc:
            QMessageBox.warning(self, "打包失败", str(exc))
            return

        self.summary.setPlainText(self._build.summary_json())

    def _require_build(self) -> LuaBuildResult | None:
        if self._build is None:
            self.build_packets()
        return self._build

    def save_binary(self) -> None:
        build = self._require_build()
        if build is None:
            return
        default = str(build.source_path.with_suffix(".lua_uart.bin"))
        path, _ = QFileDialog.getSaveFileName(self, "保存二进制流", default, "Binary (*.bin);;All Files (*)")
        if not path:
            return
        data = build.to_stream(
            prefix_once=self.prefix_once_check.isChecked(),
            prefix_each_packet=self.prefix_each_check.isChecked(),
        )
        Path(path).write_bytes(data)
        self.log_serial("info", f"saved {path}, {len(data)} bytes")

    def save_manifest(self) -> None:
        build = self._require_build()
        if build is None:
            return
        default = str(build.source_path.with_suffix(".lua_uart.json"))
        path, _ = QFileDialog.getSaveFileName(self, "保存包清单", default, "JSON (*.json);;All Files (*)")
        if not path:
            return
        Path(path).write_text(build.summary_json(), encoding="utf-8")
        self.log_serial("info", f"saved {path}")

    def send_serial(self) -> None:
        build = self._require_build()
        if build is None:
            return
        try:
            port = self._selected_port()
            baudrate = self._baudrate()
        except ValueError as exc:
            QMessageBox.warning(self, "串口错误", str(exc))
            return

        self._stop_monitor()
        self.monitor_check.blockSignals(True)
        self.monitor_check.setChecked(True)
        self.monitor_check.blockSignals(False)

        options = SerialOptions(
            port=port,
            baudrate=baudrate,
            prefix_once=self.prefix_once_check.isChecked(),
            prefix_each_packet=self.prefix_each_check.isChecked(),
            wait_ack=self.ack_check.isChecked(),
        )
        self._serial_entries.clear()
        self._rx_buffer.clear()
        if self._rx_flush_timer.isActive():
            self._rx_flush_timer.stop()
        self.serial_output.clear()
        self.log_serial("info", f"open {port} @ {baudrate} 8N1")
        self.send_button.setEnabled(False)
        self._worker = SendWorker(build, options)
        self._worker.log.connect(self.log_serial)
        self._worker.rx.connect(self.log_serial_rx)
        self._worker.tx.connect(self.log_serial_tx)
        self._worker.done.connect(self._send_done)
        self._worker.failed.connect(self._send_failed)
        self._worker.start()

    def _send_done(self) -> None:
        self.send_button.setEnabled(True)
        self.log_serial("info", "send done")
        self.version_spin.setValue(self.version_spin.value() + 1)
        if self.monitor_check.isChecked():
            self._start_monitor()

    def _send_failed(self, message: str) -> None:
        self.send_button.setEnabled(True)
        QMessageBox.warning(self, "发送失败", message)
        self.log_serial("err", f"send failed: {message}")
        if self.monitor_check.isChecked():
            self._start_monitor()

    def closeEvent(self, event) -> None:
        self._stop_monitor()
        super().closeEvent(event)
