#include "lua_mcu_port.h"
#include "lauxlib.h"
#include "include.h"
#include "lualib.h"
#include "string.h"
#include "stdio.h"


__align(8) static uint8_t Lua_Heap[LUA_MCU_HEAP_SIZE];
static LuaMcuHeap lua_heap;

static int lua_mcu_report_error(lua_State *L);
static int panic(lua_State *L);


 







void lua_mcu_writeline(void) {
    lua_mcu_write("\r\n", 2);
}


 









uint64_t lua_mcu_get_tick(void) {
    return Timer_GetTicks();
}


 









uint32_t lua_mcu_get_seed(void) {
    
    uint32_t unique_id = *(volatile uint32_t *)0x1FFF7A10; 
    
    
    uint64_t tick = lua_mcu_get_tick();
    uint32_t tick_mix = (uint32_t)(tick ^ (tick >> 32));
    
    
    uint32_t stack_addr = (uint32_t)(uintptr_t)&tick;
    
    
    return unique_id ^ tick_mix ^ stack_addr; 
}


 







lua_State *lua_mcu_newstate(void) {
    lua_mcu_heap_init();
    lua_State * L = lua_newstate(lua_mcu_alloc, &lua_heap);
    if (!L) {
        lua_mcu_write_error("L Error!");
        return NULL;
    }
    lua_atpanic(L, panic);
    luaL_openlibs_mcu(L);
    return L;
}


static void lua_mcu_heap_add_used(LuaMcuHeap *heap, size_t size) {
    heap->used += size;

    if (heap->used > heap->peak) {
        heap->peak = heap->used;
    }
}

static void lua_mcu_heap_sub_used(LuaMcuHeap *heap, size_t size) {
    if (heap->used >= size) {
        heap->used -= size;
    } else {
        heap->used = 0;
    }
}


 










void lua_mcu_write(const char *s, size_t len) {
    UART_M_SendString(s, len);
}


static LuaMcuBlock *lua_mcu_find_block(LuaMcuHeap *heap, void *ptr) {
    uintptr_t start;
    uintptr_t end;
    uintptr_t user;
    LuaMcuBlock *block;
    LuaMcuBlock *cur;

    if (!heap || !heap->buffer || !heap->first || !ptr) {
        return NULL;
    }

    start = (uintptr_t)heap->buffer;
    end = start + heap->size;
    user = (uintptr_t)ptr;

    if (user < start + sizeof(LuaMcuBlock) || user >= end) {
        return NULL;
    }

    if ((user & (LUA_MCU_ALIGN_SIZE - 1u)) != 0u) {
        return NULL;
    }

    block = (LuaMcuBlock *)(user - sizeof(LuaMcuBlock));

    for (cur = (LuaMcuBlock *)heap->first; cur; cur = cur->next) {
        if (cur == block) {
            return block;
        }
    }

    return NULL;
}


 









int lua_mcu_dostring(lua_State *L, const char *script) {
    int status =  luaL_loadstring(L, script);
    if (status != LUA_OK) {
        lua_mcu_report_error(L);
        return status;
    }
    status = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (status != LUA_OK) {
        lua_mcu_report_error(L);
    }
    return status;
}


 









void lua_mcu_write_error(const char *msg) {
    if (!msg) {
        return;
    }

    UART_M_SendString(msg, 0);

}


 






 
void lua_mcu_heap_init(void) {

    lua_heap.size = sizeof(Lua_Heap);
    lua_heap.buffer = Lua_Heap;
    lua_heap.failed = 0;
    lua_heap.peak = 0;
    lua_heap.used = 0;

    LuaMcuBlock *first = (LuaMcuBlock *)lua_heap.buffer;
    first->size = lua_heap.size - sizeof(LuaMcuBlock);
    first->used = 0;
    first->next = NULL;
    first->prev = NULL;

    lua_heap.first = first;

}  


static void *lua_mcu_pool_malloc(LuaMcuHeap *heap, size_t size) {
    uint8_t flage = 0;

    LuaMcuBlock *temp = (LuaMcuBlock *)heap->first;
    if (!temp->used && temp->size >= size) {
        flage = 1; 
    }

    while (temp->next && !flage) {
        temp = temp->next;
        if (!temp->used && temp->size >= size) {
            flage = 1; 
        }
    }
    
    if(flage) {
        temp->used = 1;
        if ((temp->size - size) >= sizeof(LuaMcuBlock) + LUA_MCU_ALIGN_SIZE) {
            LuaMcuBlock *new = (LuaMcuBlock *)((uint8_t *)temp + sizeof(LuaMcuBlock) + size);
            new->next = temp->next;
            temp->next = new;
            new->prev = temp;
            if (new->next) {
                new->next->prev = new;
            }
            new->size = (temp->size - size - sizeof(LuaMcuBlock));
            new->used = 0;
            temp->size = size;
            temp->used = 1;
            lua_mcu_heap_add_used(heap, temp->size);
            return (uint8_t *)temp + sizeof(LuaMcuBlock);
        }
        else {
            temp->used = 1;
            temp->size = temp->size;
            lua_mcu_heap_add_used(heap, temp->size);
            return (uint8_t *)temp + sizeof(LuaMcuBlock);
        }
    }
    else {
        if (heap)
            heap->failed++;
        return NULL;
    }

}


static size_t lua_mcu_align(size_t n) {
    return (n + LUA_MCU_ALIGN_SIZE - 1u) & ~(LUA_MCU_ALIGN_SIZE - 1u);
}



    

    



















static void lua_mcu_pool_free(LuaMcuHeap *heap, void *ptr) {
    LuaMcuBlock *block;

    block = lua_mcu_find_block(heap, ptr);
    if (!block) {
        if (heap) {
            heap->failed++;
        }
        lua_mcu_write_error("Invalid free pointer");
        return;
    }

    if (!block->used) {
        heap->failed++;
        lua_mcu_write_error("Double free");
        return;
    }

    lua_mcu_heap_sub_used(heap, block->size);
    block->used = 0;


    if (!heap || !ptr) {
        return;
    }

    if (block->next && !block->next->used) {
        LuaMcuBlock *next = block->next;

        block->size += sizeof(LuaMcuBlock) + next->size;
        block->next = next->next;

        if (block->next) {
            block->next->prev = block;
        }
    }

    if (block->prev && !block->prev->used) {
        LuaMcuBlock *prev = block->prev;

        prev->size += sizeof(LuaMcuBlock) + block->size;
        prev->next = block->next;

        if (prev->next) {
            prev->next->prev = prev;
        }
    }
}


static void *lua_mcu_pool_realloc(LuaMcuHeap *heap, void *ptr, size_t osize, size_t nsize) {

    nsize = lua_mcu_align(nsize);

    size_t copy_size;
    void *newptr = NULL;
    LuaMcuBlock *temp = lua_mcu_find_block(heap, ptr);

    if (!temp) {
        if (heap) {
            heap->failed++;
        }
        lua_mcu_write_error("Invalid realloc pointer");
        return NULL;
    }

    if (temp->size >= nsize) {
        return ptr;
    }

    newptr = lua_mcu_pool_malloc(heap, nsize);

    if (!newptr) {
        lua_mcu_write_error("Not enough memory");
        return NULL;
    }

    copy_size = (temp->size < nsize) ? temp->size : nsize;

    memcpy(newptr, ptr, copy_size);
    lua_mcu_pool_free(heap, ptr);

    return newptr;
}


 
















void *lua_mcu_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    
    nsize = lua_mcu_align(nsize);

    if (!ud) {
        lua_mcu_write_error("Invalid memory address!");
        return NULL;
    }

    LuaMcuHeap *temp = (LuaMcuHeap *)ud;

    if(!nsize) {
        if (ptr) {
            lua_mcu_pool_free(temp, ptr);
        }
        return NULL;
    }

    if (!ptr) {
        return lua_mcu_pool_malloc(temp, nsize);
    }
    else {
        return lua_mcu_pool_realloc((LuaMcuHeap*)ud, ptr, osize, nsize);
    }
}


 











int lua_mcu_dobuffer(lua_State *L, const char *name, const char *buff, size_t size) {
    int status = luaL_loadbufferx(L, buff, size, name, "t");
    if (status != LUA_OK) {
        lua_mcu_report_error(L);
        return status;
    }
    status = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (status != LUA_OK) {
        lua_mcu_report_error(L);
    }
    return status;
}


static int panic(lua_State *L) {
    
    const char *msg = lua_tostring(L, -1);

    if (msg == NULL) {
        msg = "Unknown Lua panic error";
    }

    lua_mcu_write_error(msg);

    lua_pop(L, 1);

    return 0;
}


static int lua_mcu_report_error(lua_State *L) {
    const char *msg = lua_tostring(L, -1);

    if (msg == NULL) {
        msg = "Unknown Lua panic error";
    }

    lua_mcu_write_error(msg);

    lua_pop(L, 1);

    return 0;
}
