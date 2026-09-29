# Lua MCU 源码目录说明

本目录包含 STM32F411 固件移植当前使用的完整 Lua 源码集合。

Lua 版本：5.4.8。

当前启用的 MCU 标准库：

- base
- table
- string
- math

工程专用移植文件：

- `code/linit_mcu.c`：打开当前启用的 MCU 标准库。
- `code/lua_mcu_port.c/.h`：实现固定内存池、输出接口和 Lua 状态机创建。
- `code/lua_script_update.c/.h`：实现 Flash A/B 脚本更新和活动 Slot 管理。

本 MCU 源码集合没有包含面向桌面的文件 I/O、OS、package/动态加载、debug 和 UTF-8 标准库模块。如需增加这些模块，必须先评估 Flash/RAM 占用、底层平台接口和安全影响。

Lua 原始源码继续遵循本目录 [LICENSE.txt](LICENSE.txt) 中的 Lua MIT 许可证。工程专用移植代码遵循仓库根目录的 [LICENSE](../../../LICENSE)。

