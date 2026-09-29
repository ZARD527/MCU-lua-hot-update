# Lua MCU UART Update Tool

这个目录是 PC 侧打包和串口发送工具，GUI 和业务逻辑分离：

- `packetizer.py`: 读取 Lua 文件、计算整文件 CRC32、按协议切包。
- `serial_sender.py`: 串口发送、二进制 ACK 解析和失败重试。
- `gui.py`: PyQt5 图形界面。
- `main.py`: 程序入口。

当前协议按下位机现有设计生成：

```text
LUA 前缀
LuaUartPacketHeader + payload
LuaUartPacketHeader + payload
...
```

包头格式为小端：

```c
typedef struct {
    uint8_t  cmd;
    uint8_t  seq;
    uint16_t len;
    uint16_t crc16;
} LuaUartPacketHeader;
```

BEGIN payload:

```c
typedef struct {
    uint32_t version;
    uint32_t length;
    uint32_t crc32;
} LuaUpdateBeginPayload;
```

运行：

```powershell
pip install -r requirements.txt
python main.py
```

默认 DATA payload 为 64 字节，最大为 256 字节；Lua 脚本默认限制为 32 KiB。
每包 CRC16 为协议必选项，GUI 中不可关闭。RUN 使用新的 `LUA` 命令会话，因此序号重新从 0 开始。

ACK 为固定 8 字节帧：`"AK" + version + seq + int16(ret) + crc16`。接收器可以从混合的文本日志中搜索并校验 ACK；超时会先在当前会话重发，仍失败后再带 `LUA` 前缀重新同步。

运行 PC 端协议回归测试：

```powershell
python -B -m unittest discover -s . -p "test_*.py" -v
```
