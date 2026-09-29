#include "lua_script_update.h"
#include "include.h"
#include "Config.h"
#include "string.h"
#include "stdio.h"


static LuaUartPacketHeader g_header;
static LuaUartPacketHeader g_last_header;
static uint8_t g_payload[LUA_UART_MAX_PAYLOAD];
static uint8_t g_expected_seq;
static uint8_t g_last_response_valid;
static int g_last_result;
static uint32_t g_last_activity_ms;

uint16_t Lua_Kid = LUA_UART_WAIT;


static uint16_t Uart_To_Lua_Crc16(const uint8_t *data, uint16_t len) {
    uint16_t crc = 0xFFFFU;
    uint16_t index;

    for (index = 0U; index < len; index++) {
        uint8_t bit;

        crc ^= (uint16_t)data[index] << 8;
        for (bit = 0U; bit < 8U; bit++) {
            crc = (crc & 0x8000U) ? (uint16_t)((crc << 1) ^ 0x1021U)
                                  : (uint16_t)(crc << 1);
        }
    }

    return crc;
}


static uint8_t Uart_To_Lua_HeaderEquals(const LuaUartPacketHeader *left,
                                        const LuaUartPacketHeader *right) {
    return (left->cmd == right->cmd &&
            left->seq == right->seq &&
            left->len == right->len &&
            left->crc16 == right->crc16) ? 1U : 0U;
}


static int Uart_To_Lua_ValidateHeader(const LuaUartPacketHeader *header) {
    if (header->cmd == LUA_UART_CMD_BEGIN) {
        return (header->len == sizeof(LuaUpdateBeginPayload)) ? 0 : LUA_UART_ERR_LENGTH;
    }
    if (header->cmd == LUA_UART_CMD_DATA) {
        return (header->len > 0U && header->len <= LUA_UART_MAX_PAYLOAD) ?
               0 : LUA_UART_ERR_LENGTH;
    }
    if (header->cmd == LUA_UART_CMD_END || header->cmd == LUA_UART_CMD_RUN) {
        return (header->len == 0U) ? 0 : LUA_UART_ERR_LENGTH;
    }
    return LUA_UART_ERR_COMMAND;
}


static void Uart_To_Lua_ResetParser(uint8_t abort_update) {
    Lua_Kid = LUA_UART_WAIT;
    memset(&g_header, 0, sizeof(g_header));
    memset(g_payload, 0, sizeof(g_payload));
    if (abort_update != 0U) {
        LuaScript_AbortUpdate();
    }
}


static uint8_t Uart_To_Lua_HasTimedOut(void) {
    return ((uint32_t)(Timer_GetMs() - g_last_activity_ms) >=
            LUA_UART_PACKET_TIMEOUT_MS) ? 1U : 0U;
}


static int Uart_To_Lua_Dispatch(lua_State *L) {
    if (g_header.cmd == LUA_UART_CMD_BEGIN) {
        LuaUpdateBeginPayload begin;

        memcpy(&begin, g_payload, sizeof(begin));
        return LuaScript_BeginUpdate(begin.version, begin.length, begin.crc32);
    }
    if (g_header.cmd == LUA_UART_CMD_DATA) {
        return LuaScript_WriteChunk(g_payload, g_header.len);
    }
    if (g_header.cmd == LUA_UART_CMD_END) {
        return LuaScript_EndUpdate(L);
    }
    if (g_header.cmd == LUA_UART_CMD_RUN) {
        return LuaScript_RunActive(L);
    }
    return LUA_UART_ERR_COMMAND;
}


static uint16_t Uart_To_Lua_ProcessCompletePacket(lua_State *L) {
    uint16_t calculated_crc = Uart_To_Lua_Crc16(g_payload, g_header.len);
    int result;

    if (calculated_crc != g_header.crc16) {
        Uart_To_Lua_SendErr(g_header.seq, LUA_UART_ERR_CRC16);
        Lua_Kid = LUA_UART_READ_HEADER;
        g_last_activity_ms = Timer_GetMs();
        return LUA_UART_BUSY;
    }

    if (g_last_response_valid != 0U && g_header.seq == g_last_header.seq &&
        Uart_To_Lua_HeaderEquals(&g_header, &g_last_header) != 0U) {
        Uart_To_Lua_SendAck(g_header.seq, g_last_result);
        if (g_header.cmd == LUA_UART_CMD_END || g_header.cmd == LUA_UART_CMD_RUN) {
            Uart_To_Lua_ResetParser(0U);
            return LUA_UART_DONE;
        }
        Lua_Kid = LUA_UART_READ_HEADER;
        g_last_activity_ms = Timer_GetMs();
        return LUA_UART_BUSY;
    }

    if ((g_header.cmd == LUA_UART_CMD_BEGIN || g_header.cmd == LUA_UART_CMD_RUN) &&
        g_header.seq == 0U) {
        g_expected_seq = 0U;
        g_last_response_valid = 0U;
        if (g_header.cmd == LUA_UART_CMD_BEGIN) {
            LuaScript_AbortUpdate();
        }
    }

    if (g_header.seq != g_expected_seq) {
        Uart_To_Lua_SendErr(g_header.seq, LUA_UART_ERR_SEQUENCE);
        Lua_Kid = LUA_UART_READ_HEADER;
        g_last_activity_ms = Timer_GetMs();
        return LUA_UART_BUSY;
    }

    result = Uart_To_Lua_Dispatch(L);
    g_last_header = g_header;
    g_last_result = result;
    g_last_response_valid = 1U;
    Uart_To_Lua_SendAck(g_header.seq, result);

#if LUA_UART_DEBUG
    printf("[LUA] cmd=0x%02X seq=%u len=%u ret=%d\r\n",
           g_header.cmd, g_header.seq, g_header.len, result);
#endif

    if (result == 0) {
        g_expected_seq++;
    }

    if (g_header.cmd == LUA_UART_CMD_END || g_header.cmd == LUA_UART_CMD_RUN) {
        Uart_To_Lua_ResetParser((result == 0) ? 0U : 1U);
        return (result == 0) ? LUA_UART_DONE : LUA_UART_ERR;
    }
    if (result != 0) {
        Uart_To_Lua_ResetParser(1U);
        return LUA_UART_ERR;
    }

    Lua_Kid = LUA_UART_READ_HEADER;
    g_last_activity_ms = Timer_GetMs();
    memset(g_payload, 0, sizeof(g_payload));
    return LUA_UART_BUSY;
}


void Uart_To_Lua_Init(void) {
    memset(&g_header, 0, sizeof(g_header));
    memset(&g_last_header, 0, sizeof(g_last_header));
    memset(g_payload, 0, sizeof(g_payload));
    g_expected_seq = 0U;
    g_last_response_valid = 0U;
    g_last_result = 0;
    g_last_activity_ms = Timer_GetMs();
    Lua_Kid = LUA_UART_WAIT;
}


uint16_t Uart_To_Lua_Poll(lua_State *L) {
    uint16_t index;
    int header_result;

    if (L == NULL) {
        return LUA_UART_ERR;
    }

    if (Lua_Kid == LUA_UART_WAIT) {
        Lua_Kid = LUA_UART_READ_HEADER;
        g_last_activity_ms = Timer_GetMs();
    }

    if (Lua_Kid == LUA_UART_READ_HEADER) {
        uint8_t raw_header[sizeof(LuaUartPacketHeader)];
        uint8_t prefix[4];

        if (SoftUART_RingBufferPeek(3U, prefix) != 0U &&
            prefix[0] == 'L' && prefix[1] == 'U' && prefix[2] == 'A') {
            for (index = 0U; index < 3U; index++) {
                (void)SoftUART_RingBufferGet(&prefix[index]);
            }
            g_last_activity_ms = Timer_GetMs();
            return LUA_UART_BUSY;
        }

        if (SoftUART_RingBufferSize() < sizeof(raw_header)) {
            if (Uart_To_Lua_HasTimedOut() != 0U) {
                Uart_To_Lua_SendErr(g_expected_seq, LUA_UART_ERR_TIMEOUT);
                g_last_response_valid = 0U;
                SoftUART_RingBufferClear();
                Uart_To_Lua_ResetParser(1U);
                return LUA_UART_ERR;
            }
            return LUA_UART_BUSY;
        }

        for (index = 0U; index < sizeof(raw_header); index++) {
            (void)SoftUART_RingBufferGet(&raw_header[index]);
        }
        memcpy(&g_header, raw_header, sizeof(g_header));
        g_last_activity_ms = Timer_GetMs();

        header_result = Uart_To_Lua_ValidateHeader(&g_header);
        if (header_result != 0) {
            Uart_To_Lua_SendErr(g_header.seq, header_result);
            SoftUART_RingBufferClear();
            Uart_To_Lua_ResetParser(1U);
            return LUA_UART_ERR;
        }

        if (g_header.len == 0U) {
            memset(g_payload, 0, sizeof(g_payload));
            return Uart_To_Lua_ProcessCompletePacket(L);
        }

        Lua_Kid = LUA_UART_READ_PAYLOAD;
    }

    if (Lua_Kid == LUA_UART_READ_PAYLOAD) {
        if (SoftUART_RingBufferSize() < g_header.len) {
            if (Uart_To_Lua_HasTimedOut() != 0U) {
                Uart_To_Lua_SendErr(g_header.seq, LUA_UART_ERR_TIMEOUT);
                g_last_response_valid = 0U;
                SoftUART_RingBufferClear();
                Uart_To_Lua_ResetParser(1U);
                return LUA_UART_ERR;
            }
            return LUA_UART_BUSY;
        }

        for (index = 0U; index < g_header.len; index++) {
            (void)SoftUART_RingBufferGet(&g_payload[index]);
        }
        g_last_activity_ms = Timer_GetMs();
        return Uart_To_Lua_ProcessCompletePacket(L);
    }

    Uart_To_Lua_ResetParser(1U);
    return LUA_UART_ERR;
}


void Uart_To_Lua_SendAck(uint8_t seq, int ret) {
    uint8_t frame[LUA_UART_ACK_SIZE];
    uint16_t crc;
    int16_t status = (int16_t)ret;

    frame[0] = LUA_UART_ACK_MAGIC0;
    frame[1] = LUA_UART_ACK_MAGIC1;
    frame[2] = LUA_UART_ACK_VERSION;
    frame[3] = seq;
    frame[4] = (uint8_t)((uint16_t)status & 0xFFU);
    frame[5] = (uint8_t)(((uint16_t)status >> 8) & 0xFFU);
    crc = Uart_To_Lua_Crc16(frame, 6U);
    frame[6] = (uint8_t)(crc & 0xFFU);
    frame[7] = (uint8_t)((crc >> 8) & 0xFFU);

    UART_H_SendString((const char *)frame, sizeof(frame));
    UART_M_SendString((const char *)frame, sizeof(frame));
}


void Uart_To_Lua_SendErr(uint8_t seq, int ret) {
    Uart_To_Lua_SendAck(seq, ret);
}
