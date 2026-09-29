# 移植、构建与测试

## 发布包范围

`firmware/` 包含：

- 本次移植修改过的应用层和 Keil 工程文件；
- 测试工程正在使用的完整 Lua 5.4.8 MCU 源码目录；
- MCU 内存分配、输出适配以及 Flash A/B 更新模块。

发布包不会重复提供未修改的 CMSIS、STM32 SPL、启动文件、系统文件和板级基础头文件。因此，`firmware/Hello_World.uvprojx` 是经过测试的工程配置参考，不是可以在本目录中直接构建的完整工程。

## 硬件基础工程要求

- STM32F411CEU6/STM32F411CEUx 对应的 CMSIS Device 支持和启动、系统文件。
- 与 V1.8.1 兼容的 STM32F4 Standard Peripheral Library。
- 基础工程中的 `User/stm32f4xx_conf.h` 和 `User/stm32f4xx_it.h`。
- Keil MDK-ARM，以及 ARM Compiler 5.06 update 5（build 528）。
- Keil STM32F4xx Device Family Pack 2.16.0。

Lua 源码已经完整包含在 `firmware/Libraries/lua`，不需要另外下载。CMSIS、SPL 等平台依赖应从官方或其他合法渠道取得，并保留其原始许可证。

## 导入并构建固件

1. 备份完整的 STM32F411 硬件基础工程。
2. 将 `firmware/Libraries/lua` 整个目录复制到基础工程中。
3. 检查板卡引脚定义后，将 `firmware/User` 中的文件复制到基础工程对应路径。
4. 将 `firmware/Hello_World.uvprojx` 作为配置参考，不要直接覆盖结构不同的工程文件。
5. 将 `Libraries/lua/code` 下全部 `.c` 文件加入 Keil 的 Lua 源码分组。
6. 按 `firmware/README.zh-CN.md` 设置编译宏、头文件路径和 IROM 边界。
7. 执行 **Rebuild all target files**，并逐条检查编译警告。
8. 如果需要保留已有 Lua A/B Slot，下载时不能使用整片擦除。

参考完整工程已经验证为 `0 Error(s), 2 Warning(s)`。两条警告来自 Lua 核心中的未使用辅助代码：`lbaselib.c` 和 `lfunc.c`。

## PC 上位机

建议使用 Python 虚拟环境：

```powershell
cd tools/uart_updater
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
python main.py
```

PyQt5 使用 GPLv3/商业双重许可证。如果需要重新打包或分发上位机，请先阅读根目录的 `THIRD_PARTY_NOTICES.md`。

## 协议测试

```powershell
cd tools/uart_updater
python -B -m unittest discover -s . -p "test_*.py" -v
```

## 硬件验证清单

- A/B Slot 均为空时能够正常启动并等待更新。
- 发送并运行 `examples/lua/01_led_on.lua`。
- 使用更高版本号发送并运行 `02_blink_key.lua`。
- 复位后活动脚本仍然存在并可运行。
- 注入错误 CRC16，设备应拒绝数据包。
- 丢弃一次 ACK，确认上位机重发不会导致重复 Flash 写入。
- 在 DATA 传输中断后，确认接收超时能够恢复状态。
- 更新过程中断电，确认旧的有效 Slot 仍可被选择。
- 启动时按住 PA0，确认恢复入口会跳过 Lua 自动运行。

