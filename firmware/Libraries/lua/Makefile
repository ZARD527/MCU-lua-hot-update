# Lua 5.4.8 MCU core build
# This folder intentionally excludes the PC command-line tools and heavy host libs:
#   lua.c, luac.c, liolib.c, loslib.c, loadlib.c, ldblib.c, linit.c, lcorolib.c, lutf8lib.c
# Add optional libraries back only when the MCU port has matching platform support.

BUILD_DIR ?= build
CODE_DIR  ?= code
TARGET    ?= liblua_mcu.lib

CC ?= armcc
AR ?= armar

CFLAGS ?= --c99 -c --cpu Cortex-M4.fp -O2 --apcs=interwork --split_sections
CFLAGS += -I$(CODE_DIR) -I../../User -DLUA_USE_C89 -DLUA_USE_JUMPTABLE=0 -DLUA_MCU_NO_FILE

CORE_SRCS = \
  $(CODE_DIR)/lapi.c \
  $(CODE_DIR)/lauxlib.c \
  $(CODE_DIR)/lbaselib.c \
  $(CODE_DIR)/lcode.c \
  $(CODE_DIR)/lctype.c \
  $(CODE_DIR)/ldebug.c \
  $(CODE_DIR)/ldo.c \
  $(CODE_DIR)/ldump.c \
  $(CODE_DIR)/lfunc.c \
  $(CODE_DIR)/lgc.c \
  $(CODE_DIR)/llex.c \
  $(CODE_DIR)/lmathlib.c \
  $(CODE_DIR)/lmem.c \
  $(CODE_DIR)/lobject.c \
  $(CODE_DIR)/lopcodes.c \
  $(CODE_DIR)/lparser.c \
  $(CODE_DIR)/lstate.c \
  $(CODE_DIR)/lstring.c \
  $(CODE_DIR)/lstrlib.c \
  $(CODE_DIR)/ltable.c \
  $(CODE_DIR)/ltablib.c \
  $(CODE_DIR)/ltm.c \
  $(CODE_DIR)/lundump.c \
  $(CODE_DIR)/lvm.c \
  $(CODE_DIR)/lzio.c \
  $(CODE_DIR)/lua_mcu_port.c \
  $(CODE_DIR)/linit_mcu.c \

OBJS = $(CORE_SRCS:$(CODE_DIR)/%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean list

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(CODE_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/$(TARGET): $(OBJS)
	$(AR) --create $@ $(OBJS)

list:
	@echo $(CORE_SRCS)

clean:
	del /Q $(BUILD_DIR)\*.o $(BUILD_DIR)\$(TARGET) 2>NUL
