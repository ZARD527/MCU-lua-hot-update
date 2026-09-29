# 固件移植包说明

本目录包含修改后的 STM32 应用文件，以及测试工程正在使用的完整 Lua MCU 源码目录，适合导入兼容的 STM32F411 硬件基础工程。

本目录不是独立完整的板级工程，因为没有重复包含 CMSIS、STM32 SPL、启动文件、系统文件和未修改的板级基础头文件。

## 包含的应用和工程修改

```text
Hello_World.uvprojx
User/bsp.c
User/cmd.c
User/Config.h
User/include.h
User/Key.c
User/LED.c
User/log.c
User/Lua_Run.c
User/main.c
User/stm32f4xx_it.c
User/timer.c
User/UART.c
User/UART_H.c
User/UART_M.c
User/UART_To_Lua.c
```

## 包含的 Lua MCU 目录

`Libraries/lua/` 作为一个完整目录从已验证工程中复制，包含：

- Lua 5.4.8 解析器、虚拟机、垃圾回收器和核心 API；
- 辅助库，以及 base、table、string、math 标准库；
- `linit_mcu.c`：注册当前 MCU 启用的精简库；
- `lua_mcu_port.c/.h`：固定内存池、输出、错误处理和 Lua 状态机创建；
- `lua_script_update.c/.h`：Flash A/B 脚本存储、更新和校验；
- Lua 公开头文件、内部头文件和 Lua MIT 许可证；
- 用于来源和移植参考的 Makefile。

这是当前 MCU 工程使用的完整 Lua 源码集合，不是 Lua 官方桌面完整版。为了降低 Flash/RAM 占用和攻击面，没有包含文件 I/O、OS、package/动态加载、debug 和 UTF-8 标准库模块。`Lua_Run.c` 使用的是 Lua 核心协程 API，协程运行仍然受支持。

## 硬件基础工程要求

请准备包含以下内容的 STM32F411 基础工程：

- STM32F411CEUx CMSIS 头文件和启动、系统源码；
- 与 V1.8.1 兼容的 STM32F4 Standard Peripheral Library；
- `User/stm32f4xx_conf.h` 和 `User/stm32f4xx_it.h`；
- 正常可用的 Keil 构建环境。

Lua 不需要从其他位置重新导入，直接使用本包的 `Libraries/lua` 目录。

## 导入步骤

1. 备份目标硬件基础工程。
2. 将 `Libraries/lua` 整个目录复制到目标工程的 `Libraries/lua` 路径。
3. 将本包中的 `User` 文件复制到目标工程相同的相对路径。
4. 覆盖板级配置前检查 `User/Config.h` 中的 GPIO 和串口定义。大写 `C` 是有意的；在区分大小写的系统上，应删除或重命名旧的 `User/config.h`，只保留 `User/Config.h`。
5. 将 `Libraries/lua/code` 下全部 `.c` 文件加入 Keil 的 Lua 分组。
6. 在应用分组中加入或确认本包所有 `User/*.c` 文件。
7. 将 `Hello_World.uvprojx` 作为配置参考。只有目标工程来自相同基础版本且已经备份时，才考虑直接替换工程文件。

## Keil 参考配置

目标芯片：

```text
STM32F411CEUx
```

编译宏：

```text
STM32F411xE
USE_STDPERIPH_DRIVER
LUA_USE_C89
LUA_USE_JUMPTABLE=0
LUA_MCU_NO_FILE
```

参考头文件路径：

```text
./User
./Libraries/lua/code
./RTE/Device/STM32F411CEUx
./Libraries/CMSIS/Include
./Libraries/CMSIS/Device/ST/STM32F4xx/Include
./Libraries/STM32F4xx_StdPeriph_Driver/inc
```

应用程序 IROM 必须在 Lua Slot 之前结束：

```text
起始地址: 0x08000000
大小:     0x00040000
```

Lua 存储区域：

```text
Slot A: 0x08040000，128 KiB，Sector 6
Slot B: 0x08060000，128 KiB，Sector 7
```

## 导入后重点检查

- 正常使用时，`APP_ERASE_LUA_SLOTS_ON_BOOT` 应保持为 `0`。
- `TIMER_USE_LSE` 当前为 `0`，默认板卡不要求外部 32.768 kHz 晶振。
- 确认 PB6/PB7 9600 和 PA9/PA10 115200 与实际接线一致。
- 需要保留 Lua Slot 时，不能使用整片擦除。
- 执行一次完整 Keil Rebuild，并完成 [硬件验证清单](../docs/building.zh-CN.md#硬件验证清单)。

