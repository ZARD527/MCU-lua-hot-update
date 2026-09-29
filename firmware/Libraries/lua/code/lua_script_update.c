#include "lua_script_update.h"
#include "stm32f4xx_flash.h"
#include "include.h"
#include "Config.h"


static LuaScriptHeader update_header;
static uint32_t update_slot_addr;
static uint8_t update_in_progress;
static uint32_t update_write_offset;
static uint32_t update_programmed_offset;
static uint8_t update_word_buffer[4];
static uint8_t update_word_fill;

static uint32_t active_slot_addr;


static uint32_t LuaScript_SlotSize(uint32_t slot_addr) {
    if (slot_addr == LUA_SLOT_A_ADDR) {
        return LUA_SLOT_A_SIZE;
    }
    if (slot_addr == LUA_SLOT_B_ADDR) {
        return LUA_SLOT_B_SIZE;
    }
    return 0U;
}


static uint32_t LuaScript_CalcCrc32(uint32_t addr, uint32_t len) {
    const uint8_t *p = (const uint8_t *)addr;
    uint32_t crc = 0xFFFFFFFFU;

    while (len-- != 0U) {
        uint8_t bit;

        crc ^= *p++;
        for (bit = 0U; bit < 8U; bit++) {
            crc = (crc & 1U) ? ((crc >> 1) ^ 0xEDB88320U) : (crc >> 1);
        }
    }

    return ~crc;
}


static uint8_t LuaScript_IsSlotValid(uint32_t slot_addr) {
    const LuaScriptHeader *header;
    uint32_t slot_size = LuaScript_SlotSize(slot_addr);
    uint32_t script_addr;

    if (slot_size == 0U || (slot_addr & 3U) != 0U) {
        return 0U;
    }

    header = (const LuaScriptHeader *)slot_addr;
    if (header->magic != LUA_SCRIPT_MAGIC ||
        header->state != LUA_SCRIPT_STATE_VALID ||
        header->version == 0U ||
        header->version > LUA_SCRIPT_MAX_VERSION ||
        header->length == 0U ||
        header->length > LUA_SCRIPT_MAX_SIZE ||
        header->length > (slot_size - sizeof(LuaScriptHeader))) {
        return 0U;
    }

    script_addr = slot_addr + sizeof(LuaScriptHeader);
    return (header->crc32 == LuaScript_CalcCrc32(script_addr, header->length)) ? 1U : 0U;
}


static int LuaScript_EraseSlot(uint32_t slot_addr) {
    FLASH_Status status;
    uint32_t sector;

    if (slot_addr == LUA_SLOT_A_ADDR) {
        sector = LUA_SLOT_A_SECTOR;
    }
    else if (slot_addr == LUA_SLOT_B_ADDR) {
        sector = LUA_SLOT_B_SECTOR;
    }
    else {
        return -1;
    }

    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                    FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
    status = FLASH_EraseSector(sector, VoltageRange_3);
    FLASH_Lock();

    return (status == FLASH_COMPLETE) ? 0 : -2;
}


static int LuaScript_ProgramWordUnlocked(uint32_t addr, uint32_t value) {
    FLASH_Status status = FLASH_ProgramWord(addr, value);

    if (status != FLASH_COMPLETE) {
        return -1;
    }
    if (*(volatile const uint32_t *)addr != value) {
        return -2;
    }
    return 0;
}


static int LuaScript_WriteHeader(void) {
    const uint32_t *words = (const uint32_t *)&update_header;
    uint32_t offset;
    int result = 0;

    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                    FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    for (offset = 0U; offset < sizeof(LuaScriptHeader); offset += 4U) {
        result = LuaScript_ProgramWordUnlocked(update_slot_addr + offset, words[offset / 4U]);
        if (result != 0) {
            break;
        }
    }

    FLASH_Lock();
    return result;
}


static int LuaScript_MarkSlotBad(uint32_t slot_addr) {
    const LuaScriptHeader *header;
    uint32_t state_addr;
    int result;

    if (LuaScript_SlotSize(slot_addr) == 0U) {
        return -1;
    }

    header = (const LuaScriptHeader *)slot_addr;
    if (header->magic != LUA_SCRIPT_MAGIC || header->state != LUA_SCRIPT_STATE_VALID) {
        return -2;
    }

    state_addr = slot_addr + (uint32_t)offsetof(LuaScriptHeader, state);
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                    FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
    result = LuaScript_ProgramWordUnlocked(state_addr, LUA_SCRIPT_STATE_BAD);
    FLASH_Lock();

    return result;
}


static int LuaScript_ValidateSyntax(lua_State *L, uint32_t script_addr, uint32_t length) {
    int status;

    if (L == NULL) {
        return -1;
    }

    status = luaL_loadbufferx(L, (const char *)script_addr, length, "update_preflight", "t");
    if (status != LUA_OK) {
        const char *message = lua_tostring(L, -1);
        lua_mcu_write_error(message != NULL ? message : "Lua syntax validation failed");
        lua_pop(L, 1);
        return -2;
    }

    lua_pop(L, 1);
    return 0;
}


static int LuaScript_StartSelectedSlot(void) {
    const LuaScriptHeader *header;
    uint32_t script_addr;

    if (g_lua_state == NULL || active_slot_addr == 0U ||
        !LuaScript_IsSlotValid(active_slot_addr)) {
        return -1;
    }

    header = (const LuaScriptHeader *)active_slot_addr;
    script_addr = active_slot_addr + sizeof(LuaScriptHeader);
    return Lua_Run_StartBuffer(Lua_Name, (const char *)script_addr, header->length);
}


uint32_t LuaScript_SelectValidSlot(void) {
    uint8_t valid_a = LuaScript_IsSlotValid(LUA_SLOT_A_ADDR);
    uint8_t valid_b = LuaScript_IsSlotValid(LUA_SLOT_B_ADDR);

    if (valid_a == 0U && valid_b == 0U) {
        return 0U;
    }
    if (valid_a != 0U && valid_b == 0U) {
        return LUA_SLOT_A_ADDR;
    }
    if (valid_a == 0U && valid_b != 0U) {
        return LUA_SLOT_B_ADDR;
    }

    return (((const LuaScriptHeader *)LUA_SLOT_A_ADDR)->seq >=
            ((const LuaScriptHeader *)LUA_SLOT_B_ADDR)->seq) ?
           LUA_SLOT_A_ADDR : LUA_SLOT_B_ADDR;
}


void LuaScript_Init(void) {
    update_in_progress = 0U;
    update_write_offset = 0U;
    update_programmed_offset = 0U;
    update_word_fill = 0U;
    active_slot_addr = LuaScript_SelectValidSlot();
}


void LuaScript_AbortUpdate(void) {
    update_in_progress = 0U;
    update_write_offset = 0U;
    update_programmed_offset = 0U;
    update_word_fill = 0U;
    memset(update_word_buffer, 0xFF, sizeof(update_word_buffer));
}


int LuaScript_BeginUpdate(uint32_t version, uint32_t length, uint32_t crc32) {
    const LuaScriptHeader *current = NULL;
    uint32_t old_seq = 0U;
    uint32_t old_version = 0U;
    uint32_t target_slot;

    if (version == 0U || version > LUA_SCRIPT_MAX_VERSION ||
        length == 0U || length > LUA_SCRIPT_MAX_SIZE) {
        return -1;
    }

    active_slot_addr = LuaScript_SelectValidSlot();
    if (active_slot_addr != 0U) {
        current = (const LuaScriptHeader *)active_slot_addr;
        old_seq = current->seq;
        old_version = current->version;
    }

    if (version <= old_version || old_seq == 0xFFFFFFFFU) {
        return -2;
    }

    target_slot = (active_slot_addr == LUA_SLOT_A_ADDR) ? LUA_SLOT_B_ADDR : LUA_SLOT_A_ADDR;
    if (length > (LuaScript_SlotSize(target_slot) - sizeof(LuaScriptHeader))) {
        return -3;
    }

    Lua_Run_Stop();
    LuaScript_AbortUpdate();
    if (LuaScript_EraseSlot(target_slot) != 0) {
        lua_mcu_write_error("Lua slot erase failed");
        return -4;
    }

    update_header.magic = LUA_SCRIPT_MAGIC;
    update_header.version = version;
    update_header.length = length;
    update_header.crc32 = crc32;
    update_header.seq = old_seq + 1U;
    update_header.state = LUA_SCRIPT_STATE_WRITING;

    update_slot_addr = target_slot;
    update_in_progress = 1U;
    memset(update_word_buffer, 0xFF, sizeof(update_word_buffer));
    return 0;
}


int LuaScript_WriteChunk(const uint8_t *data, uint32_t len) {
    uint32_t chunk_start = update_write_offset;
    uint32_t index;
    int result = 0;

    if (data == NULL || len == 0U) {
        return -1;
    }
    if (update_in_progress == 0U) {
        return -2;
    }
    if (len > (update_header.length - update_write_offset)) {
        LuaScript_AbortUpdate();
        return -3;
    }

    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                    FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    for (index = 0U; index < len; index++) {
        uint32_t word;

        update_word_buffer[update_word_fill++] = data[index];
        update_write_offset++;

        if (update_word_fill == 4U || update_write_offset == update_header.length) {
            while (update_word_fill < 4U) {
                update_word_buffer[update_word_fill++] = 0xFFU;
            }

            word = (uint32_t)update_word_buffer[0] |
                   ((uint32_t)update_word_buffer[1] << 8) |
                   ((uint32_t)update_word_buffer[2] << 16) |
                   ((uint32_t)update_word_buffer[3] << 24);
            result = LuaScript_ProgramWordUnlocked(
                update_slot_addr + sizeof(LuaScriptHeader) + update_programmed_offset,
                word);
            if (result != 0) {
                break;
            }

            update_programmed_offset += 4U;
            update_word_fill = 0U;
            memset(update_word_buffer, 0xFF, sizeof(update_word_buffer));
        }
    }

    FLASH_Lock();

    if (result != 0) {
        LuaScript_AbortUpdate();
        lua_mcu_write_error("Lua flash word program/readback failed");
        return -4;
    }

    for (index = chunk_start;
         index < update_write_offset && index < update_programmed_offset;
         index++) {
        const uint8_t *flash = (const uint8_t *)(update_slot_addr + sizeof(LuaScriptHeader));
        if (flash[index] != data[index - chunk_start]) {
            LuaScript_AbortUpdate();
            lua_mcu_write_error("Lua flash chunk readback failed");
            return -5;
        }
    }

    return 0;
}


int LuaScript_EndUpdate(lua_State *L) {
    uint32_t script_addr;

    if (update_in_progress == 0U) {
        return -1;
    }
    if (update_write_offset != update_header.length || update_word_fill != 0U) {
        LuaScript_AbortUpdate();
        return -2;
    }

    script_addr = update_slot_addr + sizeof(LuaScriptHeader);
    if (LuaScript_CalcCrc32(script_addr, update_header.length) != update_header.crc32) {
        LuaScript_AbortUpdate();
        lua_mcu_write_error("Lua script CRC32 mismatch");
        return -3;
    }
    if (LuaScript_ValidateSyntax(L, script_addr, update_header.length) != 0) {
        LuaScript_AbortUpdate();
        return -4;
    }

    update_header.state = LUA_SCRIPT_STATE_VALID;
    if (LuaScript_WriteHeader() != 0) {
        LuaScript_AbortUpdate();
        lua_mcu_write_error("Lua header program/readback failed");
        return -5;
    }

    LuaScript_AbortUpdate();
    LuaScript_Init();
    return 0;
}


int LuaScript_RunActive(lua_State *L) {
    uint32_t failed_slot;
    int result;

    if (L == NULL) {
        return -1;
    }

    active_slot_addr = LuaScript_SelectValidSlot();
    if (active_slot_addr == 0U) {
        return -2;
    }

    result = LuaScript_StartSelectedSlot();
    if (result == 0) {
        return 0;
    }

    failed_slot = active_slot_addr;
    (void)LuaScript_MarkSlotBad(failed_slot);
    active_slot_addr = LuaScript_SelectValidSlot();
    if (active_slot_addr != 0U && active_slot_addr != failed_slot) {
        (void)LuaScript_StartSelectedSlot();
    }
    return -3;
}


int LuaScript_HandleRunFailure(lua_State *L) {
    uint32_t failed_slot = active_slot_addr;

    if (L == NULL || failed_slot == 0U) {
        return -1;
    }
    if (LuaScript_MarkSlotBad(failed_slot) != 0) {
        return -2;
    }

    active_slot_addr = LuaScript_SelectValidSlot();
    if (active_slot_addr == 0U || active_slot_addr == failed_slot) {
        return -3;
    }

    return LuaScript_StartSelectedSlot();
}


uint32_t Get_Lua_Size(void) {
    return update_write_offset;
}
