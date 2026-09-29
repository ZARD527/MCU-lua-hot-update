# 系统架构

本文中的路径表示移植包合并到完整硬件基础工程后的目录。发布包包含完整的 Lua MCU 目录和修改后的应用文件，但不会重复提供未修改的 CMSIS、STM32 SPL、启动文件及其他平台文件。

## 运行时数据流

```text
PB7 软件串口 RX ─┐
                  ├─> 共用接收环形缓冲区 -> Cmd_Poll()
PA10 USART1 RX ───┘                         |
                                            v
                                 Lua UART 协议解析器
                                 CRC / 长度 / 序号检查
                                            |
                         +------------------+------------------+
                         |                                     |
                    Flash A/B 更新                        Lua RUN
                         |                                     |
                    CRC32 + 语法检查                     协程轮询
                         |                                     |
                    选择活动 Slot                    LED / 按键 / 延时

ACK 输出：PB6 软件串口 TX，同时镜像到 PA9 USART1 TX
```

两个接收通道当前都会写入同一个协议解析缓冲区。不能同时从两个串口发送相互独立的数据流，否则字节可能交叉，造成协议包损坏。

## 固件分层

- `User/main.c`：系统启动顺序、Lua 状态机创建、C API 注册和主循环。
- `User/bsp.c`：系统时钟和板级外设初始化。
- `User/UART_M.c`：基于中断的软件串口及共用环形缓冲区。
- `User/UART_H.c`：USART1 日志输出和更新数据输入。
- `User/cmd.c`：ASCII 命令前缀识别与同步。
- `User/UART_To_Lua.c`：Lua 二进制更新协议状态机。
- `User/Lua_Run.c`：Lua 协程调度和运行限制。
- `Libraries/lua/code/lua_mcu_port.c`：固定内存池分配器和 MCU Lua 适配层。
- `Libraries/lua/code/lua_script_update.c`：Flash A/B 存储、校验和活动 Slot 选择。

## 更新事务

```text
LUA 前缀
  -> BEGIN(version, length, crc32)
  -> DATA(seq=1..n)
  -> END

LUA 前缀
  -> RUN(seq=0)
```

数据包在分发前会完成长度、CRC 和序号检查。如果收到与上一个已处理包完全相同的重发包，设备会重新发送缓存的处理结果，不会再次执行 Flash 写入。

## Flash 边界

Keil 工程将应用程序限制在 Flash 前 256 KiB，Sector 6 和 Sector 7 专门保留给 Lua A/B Slot。整片擦除仍然会清除两个 Lua Slot，因此下载器和调试器的擦除配置也是运行安全的一部分。

## 信任边界

当前协议只能检查传输错误和协议一致性，不能验证发送者身份。Lua 脚本能够调用硬件接口，应当视为具有设备控制权限的输入。用于产品前请阅读根目录的 [SECURITY.md](../SECURITY.md)。

