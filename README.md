# STM32F411 Lua Script Runtime and Hot-Update Framework

> STM32F411 Lua 脚本运行时与串口热更新移植包

一个面向 STM32F411CEU6 的 Lua 5.4.8 MCU 移植项目。它提供精简 Lua 运行时、固定内存池、协程调度、Flash A/B 脚本存储、UART 更新协议，以及配套的 PyQt5 上位机。

## 当前状态

| 项目 | 状态 |
| --- | --- |
| MCU | STM32F411CEU6 / STM32F411CEUx |
| Lua | 5.4.8，64 KiB 固定内存池 |
| 脚本存储 | 内部 Flash A/B Slot |
| 更新入口 | PB6/PB7 软件串口或 USART1 PA9/PA10 |
| PC 工具 | Python、PyQt5、pyserial |
| 协议测试 | 6项单元测试通过 |
| 参考构建 | ARMCC 5.06 update 5：0 errors，2 warnings |

## 主要功能

- Lua 5.4.8 解析器、虚拟机、垃圾回收器和 MCU 适配层。
- 64 KiB 静态 Lua 内存池，8字节对齐，不依赖系统堆。
- base、table、string、math 精简标准库。
- LED、按键、非阻塞延时等 Lua/C 交互接口。
- 协程式 Lua 调度，`delay_ms()` 只暂停 Lua 任务，不阻塞 MCU 主循环。
- 20 ms 连续执行保护，避免不主动 yield 的 Lua 代码长期占用主循环。
- Flash A/B 脚本区、写入状态、CRC32校验和 Lua 语法预检。
- 每包 CRC16、连续序号、12秒收包超时和重复包幂等应答。
- 固定8字节二进制 ACK，可从混合文本日志中识别。
- 运行/加载错误时标记当前 Slot 为 BAD，并尝试启动另一个有效 Slot。
- PyQt5 图形工具、分包器、串口发送器、ACK重试和协议测试。
- 启动时按住 PA0 可跳过 Lua 自动运行，保留恢复入口。

## 仓库结构

```text
.
├─ firmware/
│  ├─ User/                         修改后的应用、BSP、UART和Lua调度文件
│  ├─ Libraries/lua/                当前工程使用的完整Lua MCU源码集合
│  │  ├─ code/                      Lua核心、精简库和MCU扩展
│  │  ├─ LICENSE.txt                Lua MIT许可证
│  │  ├─ README.md                  English
│  │  └─ README.zh-CN.md            中文说明
│  ├─ README.md                     English integration guide
│  └─ README.zh-CN.md               中文移植说明
├─ tools/uart_updater/              PyQt5串口更新工具和协议测试
├─ examples/lua/                    Lua示例脚本
├─ docs/                            架构、构建、协议和硬件资料
├─ .github/workflows/               PC协议测试工作流
├─ SECURITY.md                      安全边界和漏洞报告方式
├─ THIRD_PARTY_NOTICES.md           第三方依赖及许可范围
├─ CONTRIBUTING.md                  贡献规范
└─ LICENSE                          项目原创代码的MIT许可证
```

中文文档入口：[docs/README.zh-CN.md](docs/README.zh-CN.md)。

## 移植包边界

仓库已经包含：

- `firmware/Libraries/lua`：Lua核心、头文件、精简标准库和MCU扩展。
- `firmware/User`：本项目修改或新增的应用层文件。
- PC串口工具、Lua示例、协议与移植文档。

用户仍需从自己的 STM32F411 基础工程提供：

- STM32F411 CMSIS Device头文件。
- 对应编译器的启动文件和 `system_stm32f4xx.c`。
- STM32F4 Standard Peripheral Library，参考版本 V1.8.1。
- `User/stm32f4xx_conf.h` 和 `User/stm32f4xx_it.h`。
- Keil工程文件或其他构建系统。

## 硬件配置

| 功能 | 引脚/资源 | 配置 |
| --- | --- | --- |
| LED1 | PC13 | 低电平点亮 |
| LED2 | PB9 | 低电平点亮 |
| KEY1/恢复键 | PA0 | 上拉输入，低电平触发，20 ms消抖 |
| 软件串口 TX/RX | PB6/PB7 | 9600 8N1，TIM5采样 |
| 日志/硬件串口 TX/RX | PA9/PA10 | USART1，115200 8N1 |
| 系统时间基准 | SysTick | 默认不初始化外部 LSE |

两个串口接收入口最终写入同一个协议环形缓冲区，因此不要同时从两个端口发送相互独立的数据流。

## Lua运行环境

当前启用的标准库：

```text
base / table / string / math
```

为了控制 Flash、RAM 和攻击面，当前没有加入文件 I/O、OS、package/动态加载、debug 和 UTF-8标准库。协程调度使用 Lua 核心 API，不依赖 `coroutine` 标准库表。

向 Lua 注册的 MCU API：

```lua
led_on(id)       -- id: 1或2
led_off(id)
led_toggle(id)
key_read()       -- 检测到按键按下事件时返回1
delay_ms(ms)     -- 协程yield，不阻塞C主循环
```

Lua脚本中的长循环必须周期性调用 `delay_ms()` 或其他 yield 入口，否则连续执行保护会终止任务。

示例：

```lua
print("blink")

while true do
    led_toggle(1)
    delay_ms(500)
end
```

更多示例见 [examples/lua](examples/lua)。

## 导入到STM32工程

1. 备份目标 STM32F411 工程。
2. 将 `firmware/Libraries/lua` 整个目录复制到目标工程的 `Libraries/lua`。
3. 将 `firmware/User` 中的文件复制到目标工程对应位置，并检查 `Config.h` 中的 GPIO、串口和定时器定义。
4. 在区分大小写的平台上只保留 `User/Config.h`，不要同时保留旧的 `User/config.h`。
5. 将 `Libraries/lua/code` 下全部28个 `.c` 文件加入 Lua 编译分组。
6. 将 `firmware/User` 中13个 `.c` 文件加入应用分组，并使用基础工程提供的 `stm32f4xx_conf.h`、`stm32f4xx_it.h`。
7. 添加以下预处理宏：

   ```text
   STM32F411xE
   USE_STDPERIPH_DRIVER
   LUA_USE_C89
   LUA_USE_JUMPTABLE=0
   LUA_MCU_NO_FILE
   ```

8. 添加头文件搜索路径：

   ```text
   ./User
   ./Libraries/lua/code
   ./RTE/Device/STM32F411CEUx
   ./Libraries/CMSIS/Include
   ./Libraries/CMSIS/Device/ST/STM32F4xx/Include
   ./Libraries/STM32F4xx_StdPeriph_Driver/inc
   ```

9. 将固件 IROM限制为 `0x08000000` 起始、`0x00040000` 大小，避免链接器占用 Lua Slot。
10. 完整 Rebuild，并完成[硬件验证清单](docs/building.zh-CN.md#硬件验证清单)。

更详细的移植说明见 [firmware/README.zh-CN.md](firmware/README.zh-CN.md)。

## Flash布局

| 区域 | 起始地址 | 大小 | STM32F411扇区 |
| --- | --- | --- | --- |
| 固件 IROM | `0x08000000` | 256 KiB | Sector 0–5 |
| Lua Slot A | `0x08040000` | 128 KiB | Sector 6 |
| Lua Slot B | `0x08060000` | 128 KiB | Sector 7 |

每个 Lua Slot 以24字节 Header 开始，记录 magic、版本、长度、CRC32、更新序号和状态。当前脚本最大长度限制为32 KiB，版本范围为 `1..0x7FFFFFFF`，新版本必须大于现有有效版本。

正常发布配置：

```c
#define APP_ERASE_LUA_SLOTS_ON_BOOT  0
```

下载固件时仍需避免 **Erase Full Chip**，否则下载器会清除两个 Lua Slot。

## UART更新协议

一次更新由以下会话组成：

```text
"LUA"
  -> BEGIN(version, length, crc32), seq=0
  -> DATA, seq=1..n
  -> END

"LUA"
  -> RUN, seq=0
```

6字节小端包头：

| 偏移 | 长度 | 字段 |
| ---: | ---: | --- |
| 0 | 1 | `cmd` |
| 1 | 1 | `seq` |
| 2 | 2 | `payload length` |
| 4 | 2 | `payload CRC16` |

命令值：`BEGIN=0x01`、`DATA=0x02`、`END=0x03`、`RUN=0x04`。CRC16使用 CCITT-FALSE（初值 `0xFFFF`，多项式 `0x1021`）；END阶段还会检查完整脚本的 CRC32/IEEE并执行 Lua文本语法预检。

ACK/ERR 是固定8字节二进制帧：

```text
"AK" + protocol_version + seq + int16(status) + crc16
```

`status == 0` 表示成功。协议错误码：

| 状态码 | 含义 |
| ---: | --- |
| `-100` | 参数错误 |
| `-101` | 包头错误 |
| `-102` | 长度错误 |
| `-103` | CRC16错误 |
| `-104` | 序号错误 |
| `-105` | 接收超时 |
| `-106` | 不支持的命令 |

完整协议见 [docs/protocol.zh-CN.md](docs/protocol.zh-CN.md)。

## PC串口更新工具

要求 Python 3.11或兼容版本。建议使用虚拟环境：

```powershell
cd tools/uart_updater
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
python main.py
```

默认参数：

| 参数 | 默认值 |
| --- | --- |
| 波特率 | 9600 |
| DATA payload | 64字节，最大256字节 |
| 单字节间隔 | 1.5 ms |
| ACK超时 | 8 s |
| 最大重试 | 2次 |
| 单脚本上限 | 32 KiB |
| 每包 CRC16 | 强制启用 |

工具支持串口监视、文本/HEX显示、包清单、二进制 ACK解析、丢 ACK重发和超时重新同步。详细说明见 [tools/uart_updater/README.md](tools/uart_updater/README.md)。

## 测试

运行 PC 协议回归测试：

```powershell
cd tools/uart_updater
python -B -m unittest discover -s . -p "test_*.py" -v
```

测试覆盖 CRC向量、BEGIN小端布局、8字节 ACK、混合日志中的 ACK识别、RUN新会话序号和脚本大小限制。GitHub Actions也会对上位机和协议相关修改运行这组测试。

硬件发布前还应验证：错误 CRC、序号跳变、ACK丢失、接收超时、更新中断、更新时断电、复位保留、恢复键和长时间运行。

## 文档

- [中文文档索引](docs/README.zh-CN.md)
- [系统架构](docs/architecture.zh-CN.md)
- [移植、构建与测试](docs/building.zh-CN.md)
- [UART更新协议](docs/protocol.zh-CN.md)
- [固件移植说明](firmware/README.zh-CN.md)
- [Lua MCU源码说明](firmware/Libraries/lua/README.zh-CN.md)
- [安全策略](SECURITY.md)
- [第三方声明](THIRD_PARTY_NOTICES.md)
- [贡献指南](CONTRIBUTING.md)

## 安全边界与限制

- CRC16/CRC32只能检查传输错误，不能证明脚本来源可信。
- 当前没有数字签名、安全启动、设备认证、加密或完整防重放机制。
- 能访问更新串口的一方可以替换设备上的 Lua脚本。
- A/B回退能够处理可检测的 Lua加载和运行错误，但没有完整的 `PENDING_TEST/CONFIRMED`、看门狗复位确认流程。
- 软件串口吞吐和抗干扰能力低于硬件 USART。
- 版本号只能递增，不支持同版本强制覆盖。
- Lua脚本应视为具有设备控制权限的代码。

## 许可证

- 项目原创代码和文档：根目录 [MIT License](LICENSE)。
- Lua 5.4.8 原始源码：[Lua MIT License](firmware/Libraries/lua/LICENSE.txt)。
- PyQt5：GPLv3或 Riverbank商业许可证，依赖未随仓库分发。
- CMSIS、STM32 SPL、Keil和设备包：未随仓库分发，使用者应遵循各自许可证。

完整范围见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
