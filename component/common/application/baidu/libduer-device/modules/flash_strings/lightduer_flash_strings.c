// Copyright (2018) Baidu Inc. All rights reserved.
/**
 * File: lightduer_flash_strings.c
 * Auth: Sijun Li(lisijun@baidu.com)
 * Desc: APIs of store string list to flash.
 */
#include "lightduer_flash_strings.h"
#include <string.h>
#include <stdio.h>
#include "lightduer_log.h"
#include "lightduer_memory.h"

#define PAGE_SIZE                (1 << s_flash_config.page_align_bits)
static duer_flash_config_t s_flash_config = {0};
static duer_bool s_is_flash_config_set = DUER_FALSE;

static inline unsigned int flash_addr_align_floor(unsigned int addr, int bits) {
    addr >>= bits;
    addr <<= bits;
    return addr;
}

static inline unsigned int flash_addr_align_ceil(unsigned int addr, int bits) {              
    unsigned int bitmask = (1 << bits) - 1;
    if (addr & bitmask) {
        addr >>= bits;
        addr += 1;
        addr <<= bits;
    }
    return addr;
}

static inline unsigned int flash_addr_cycle(unsigned int addr, int size) {
    if (addr >= size) {
        return (addr - size);
    } else {
        return addr;
    }
}

void duer_set_flash_config(const duer_flash_config_t *config) {
    if (!s_is_flash_config_set) {
        memcpy(&s_flash_config, config, sizeof(s_flash_config));
        s_is_flash_config_set = DUER_TRUE;
    }
}

static int duer_payload_write_flash(
        duer_flash_context_t *ctx,
        unsigned int addr_start, 
        duer_flash_data_header *p_flash_header, 
        char *payload_string, 
        int payload_len) 
{
    int part1_len = 0;
    int part2_len = 0;
    int i = 0;
    char *page_cache = NULL;

    page_cache = (char *)DUER_MALLOC(PAGE_SIZE);
    memset(page_cache, 0, PAGE_SIZE);

    // Put header into cache.
    if (payload_string) {
        if (PAGE_SIZE - sizeof(duer_flash_data_header) >= payload_len) {
            i = payload_len;
        } else {
            i = PAGE_SIZE - sizeof(duer_flash_data_header);
        }
        memcpy(page_cache + sizeof(duer_flash_data_header), payload_string, i);
    } else {
        duer_flash_read(ctx, addr_start, page_cache, PAGE_SIZE);
    }
    memcpy(page_cache, p_flash_header, sizeof(duer_flash_data_header));
    // write header and some of payload
    duer_flash_write(ctx, addr_start, page_cache, PAGE_SIZE);

    // write rest of payload string.
    if (payload_string && i < payload_len) {
        if (addr_start + sizeof(duer_flash_data_header) + payload_len > ctx->len) {
            part1_len = ctx->len - (addr_start + PAGE_SIZE);
            part2_len = addr_start + sizeof(duer_flash_data_header) + payload_len - ctx->len;
            if (part1_len > 0) {
                duer_flash_write(ctx, addr_start + PAGE_SIZE, payload_string + i, part1_len);
            }
            duer_flash_write(ctx, 0, payload_string + i + part1_len, part2_len);
        } else {
            duer_flash_write(ctx, addr_start + PAGE_SIZE, payload_string + i, payload_len - i);
        }
    }

    DUER_FREE(page_cache);

    return DUER_OK;
}

/*
 * Append data to current flash end.
 * return: 
 *      DUER_OK, successful.
 *      DUER_ERR_FLASH_CORRUPT flash corrupt, erase all data.
 */
static int duer_add_to_flash(
        duer_flash_context_t *ctx,
        void *raw_data, 
        duer_raw2string_func raw2string_func,
        duer_free_string_func free_string_func,
        unsigned int last_ele_addr, 
        unsigned int *new_ele_addr) 
{
    unsigned int addr_start = 0; 
    unsigned int addr_end = 0;
    unsigned int sector_start = 0;
    unsigned int sector_end = 0;
    duer_flash_data_header data_add = {0, 0};
    duer_flash_data_header data = {0, 0};
    int payload_len = 0;
    char *payload_string = NULL;
    int rs = DUER_OK;
    
    if (!raw_data) {
        rs = DUER_ERR_FAILED;
        goto RET;
    }
    if (raw2string_func) {
        payload_string = raw2string_func(raw_data);
    } else {
        payload_string = (char *)raw_data;
    }
    if (!payload_string) {
        DUER_LOGE("Failed to print raw data to string!");
        rs = DUER_ERR_FAILED;
        goto RET;
    }

    data_add.magic = FLASH_MAGIC_BITMASK;
    payload_len = flash_addr_align_ceil(strlen(payload_string) + 1, s_flash_config.word_align_bits);

    if (last_ele_addr != FLASH_INVALID_ADDR) {
        duer_flash_read(ctx, last_ele_addr, (char *)&data, sizeof(data));
        addr_start = last_ele_addr + sizeof(data) + data.len;
        addr_start = flash_addr_align_ceil(addr_start, s_flash_config.page_align_bits);
        addr_start = flash_addr_cycle(addr_start, ctx->len);
    } else {
        DUER_LOGD("Flash is empty.");
        addr_start = 0;
        data_add.magic = FLASH_MAGIC;
    }
    addr_end = addr_start + sizeof(duer_flash_data_header) + payload_len;
    addr_end = flash_addr_cycle(addr_end, ctx->len);

    if (addr_start >= ctx->len) {
        DUER_LOGE("Falsh data corrupt");
        rs = DUER_ERR_FLASH_CORRUPT;
        goto RET;
    }
    if (addr_end > ctx->len) {
        DUER_LOGE("Alert flash space is not enough!");
        rs = DUER_ERR_FAILED;
        goto RET;
    }

    // Erase next used sectors.
    sector_start = flash_addr_align_floor(addr_start - 1, s_flash_config.sector_align_bits);
    sector_start += (1 << s_flash_config.sector_align_bits);
    sector_start = flash_addr_cycle(sector_start, ctx->len);
    sector_end = flash_addr_align_ceil(addr_end, s_flash_config.sector_align_bits);
    sector_end = flash_addr_cycle(sector_end, ctx->len);
    if (sector_start > sector_end) {
        duer_flash_erase(ctx, sector_start, ctx->len - sector_start);
        duer_flash_erase(ctx, 0, sector_end);
    } else if (sector_end > sector_start) {
        duer_flash_erase(ctx, sector_start, sector_end - sector_start);
    }

    // Write data into flash.
    data_add.len = payload_len;
    duer_payload_write_flash(ctx, addr_start, &data_add, payload_string, payload_len);
    DUER_LOGI("write payload to flash[%08x]: %s", addr_start, payload_string);

    if (new_ele_addr) {
        *new_ele_addr = addr_start;
    }

RET:
    if (raw2string_func && free_string_func) {
        free_string_func(payload_string);
    }
    return rs;
}

static inline int duer_flash_get_last_ele(duer_flash_context_t *ctx, unsigned int *addr) {
    int i = 0;

    // find last flash payload addr.
    while (i < ctx->max_ele_count && ctx->ele_list[i] != FLASH_INVALID_ADDR) {
        ++i;
    } 

    if (addr) {
        if (i > 0) {
            *addr = ctx->ele_list[i - 1];
        } else {
            *addr = FLASH_INVALID_ADDR;
        }
    }

    return i;
}

int duer_append_to_flash(
        duer_flash_context_t *ctx,
        void *raw_data,
        duer_raw2string_func raw2string_func,
        duer_free_string_func free_string_func) 
{
    int i = 0;
    unsigned int flash_data_addr = 0;
    int rs = 0;

    if (!s_is_flash_config_set) {
        DUER_LOGE("Please set flash config first");
    }

    if (!ctx) {
        DUER_LOGE("Flash module haven't init, call duer_flash_init first.");
        return DUER_ERR_FAILED;
    }

    i = duer_flash_get_last_ele(ctx, &flash_data_addr);
    
    if (i >= ctx->max_ele_count) {
        DUER_LOGE("Number of payload in Flash is full!");
        return DUER_ERR_FLASH_FULL;
    }

    rs = duer_add_to_flash(
            ctx, 
            raw_data, 
            raw2string_func, 
            free_string_func, 
            flash_data_addr, 
            &flash_data_addr);

    ctx->ele_list[i] = flash_data_addr;
    return rs;
}

void duer_update_to_flash_prepare(
        duer_flash_context_t *ctx,
        unsigned int *p_first_ele_addr,
        unsigned int *p_last_ele_addr) {
    int i = 0;

    if (!s_is_flash_config_set) {
        DUER_LOGE("Please set flash config first");
    }

    if (!ctx) {
        DUER_LOGE("Flash module haven't init, call duer_flash_init first.");
        return;
    }

    if (p_last_ele_addr) {
        duer_flash_get_last_ele(ctx, p_last_ele_addr);
    }

    if (p_first_ele_addr) {
        *p_first_ele_addr = ctx->ele_list[0];
    }

    for (i = 0; i < ctx->max_ele_count; ++i) {
        ctx->ele_list[i] = FLASH_INVALID_ADDR;
    }
}

// remove or change payload value.
int duer_update_to_flash(
        duer_flash_context_t *ctx,
        void *raw_data,
        duer_raw2string_func raw2string_func,
        duer_free_string_func free_string_func,
        unsigned int *p_last_ele_addr) {
    int i = 0;

    if (!s_is_flash_config_set) {
        DUER_LOGE("Please set flash config first");
    }

    if (!ctx) {
        DUER_LOGE("Flash module haven't init, call duer_flash_init first.");
        return DUER_ERR_FAILED;
    }

    if (!p_last_ele_addr) {
        return DUER_ERR_FAILED;
    }

    while (i < ctx->max_ele_count && ctx->ele_list[i] != FLASH_INVALID_ADDR) {
        ++i;
    } 
    
    if (i >= ctx->max_ele_count) {
        DUER_LOGE("Number of payload in Flash is full!");
        return DUER_ERR_FLASH_FULL;
    }

    duer_add_to_flash(
            ctx, 
            raw_data, 
            raw2string_func, 
            free_string_func, 
            *p_last_ele_addr, 
            &ctx->ele_list[i]);

    if (ctx->ele_list[i] != FLASH_INVALID_ADDR) {
        *p_last_ele_addr = ctx->ele_list[i];
    }

    return DUER_OK;
}

void duer_update_flash_header(
        duer_flash_context_t *ctx,
        unsigned int first_ele_addr) {
    duer_flash_data_header flash_data_header = {0, 0};

    if (!s_is_flash_config_set) {
        DUER_LOGE("Please set flash config first");
    }

    if (!ctx) {
        DUER_LOGE("Flash module haven't init, call duer_flash_init first.");
        return;
    }

    if (ctx->ele_list[0] != FLASH_INVALID_ADDR) {
        flash_data_header.magic = FLASH_MAGIC;
        flash_data_header.len = FLASH_LEN_BITMASK;
        duer_payload_write_flash(ctx, ctx->ele_list[0], &flash_data_header, NULL, 0);
    }

    if (first_ele_addr != FLASH_INVALID_ADDR) {
        duer_flash_read(ctx, first_ele_addr, (char *)&flash_data_header, sizeof(flash_data_header));
        if (flash_data_header.magic == FLASH_MAGIC) {
            flash_data_header.magic = 0;
            flash_data_header.len = FLASH_LEN_BITMASK;
            duer_payload_write_flash(ctx, first_ele_addr, &flash_data_header, NULL, 0);
        }
    }
}

void duer_get_all_ele_from_flash(duer_flash_context_t *ctx) {
    duer_flash_data_header data = {0, 0};
    unsigned int addr = 0;
    int i = 0;
    int page_size = (1 << s_flash_config.page_align_bits);

    if (!s_is_flash_config_set) {
        DUER_LOGE("Please set flash config first");
    }

    if (!ctx) {
        DUER_LOGE("Flash module haven't init, call duer_flash_init first.");
        return;
    }

    for (addr = 0; addr < ctx->len; addr += page_size) {
        duer_flash_read(ctx, addr, (char *)&data, sizeof(data));
        if (data.magic == FLASH_MAGIC) {
            ctx->ele_list[0] = addr;
            break;
        }
    }

    if (addr >= ctx->len) {
        DUER_LOGI("Flash have no payload stored.");
        return;
    }

    for (i = 1; i < ctx->max_ele_count; i++) {
        addr += sizeof(duer_flash_data_header) + data.len;
        addr = flash_addr_align_ceil(addr, s_flash_config.page_align_bits);
        if (addr >= ctx->len) { // search around;
            addr = addr - ctx->len;
            if (addr >= (unsigned int)ctx->ele_list[0]) { // exceed payload flash size.
                break;
            }
        }

        duer_flash_read(ctx, addr, (char *)&data, sizeof(data));
        if (~(data.magic) == 0 && ~(data.len) != 0) { // check header valid.
            ctx->ele_list[i] = addr;
        } else {
            break;
        }
    }
    if (ctx->ele_list[i - 1] != FLASH_INVALID_ADDR) {
        DUER_LOGI("Flash stored %d payload(s).", i);
    } else {
        DUER_LOGI("Flash stored %d payload(s).", i - 1);
    }
}

int duer_get_flash_header(
        duer_flash_context_t *ctx,
        unsigned int flash_index,
        duer_flash_data_header *p_header) {
    if (!s_is_flash_config_set) {
        DUER_LOGE("Please set flash config first");
    }

    if (!p_header) {
        DUER_LOGE("Parameter is NULL!");
        return DUER_ERR_FAILED;
    }

    if (!ctx) {
        DUER_LOGE("Flash module haven't init, call duer_flash_init first.");
        return DUER_ERR_FAILED;
    }

    if (ctx->ele_list[flash_index] == FLASH_INVALID_ADDR) {
        DUER_LOGW("Flash have no data!");
        return DUER_ERR_FAILED;
    }

    duer_flash_read(
            ctx,
            ctx->ele_list[flash_index],
            (char *)p_header,
            sizeof(duer_flash_data_header));
    if (p_header->len >= ctx->len) {
        DUER_LOGW("Flash payload length[%d] value exceed max size!", p_header->len);
        return DUER_ERR_FAILED;
    }

    return DUER_OK;
}

void duer_parse_flash_ele_to_string(
        duer_flash_context_t *ctx,
        unsigned int flash_index,
        duer_flash_data_header data,
        char *payload_string) {
    int i = 0;

    if (!s_is_flash_config_set) {
        DUER_LOGE("Please set flash config first");
    }

    if (!ctx) {
        DUER_LOGE("Flash module haven't init, call duer_flash_init first.");
        return;
    }

    if (ctx->ele_list[flash_index] == FLASH_INVALID_ADDR) {
        DUER_LOGE("Flash address invalid.");
        return;
    }

    if (!payload_string) {
        DUER_LOGE("string buffer empty!");
        return;
    }

    DUER_LOGD("Write flash header: {magic: 0x%08x, len: 0x%08x}\n", data.magic, data.len);
    if (ctx->ele_list[flash_index] + sizeof(data) + data.len > ctx->len) {
        i = ctx->ele_list[flash_index] + sizeof(data) + data.len - ctx->len;
        duer_flash_read(ctx, ctx->ele_list[flash_index] + sizeof(data), payload_string, i);
        duer_flash_read(ctx, 0, payload_string + i, data.len - i);
    } else {
        duer_flash_read(ctx, ctx->ele_list[flash_index] + sizeof(data), payload_string, data.len);
    }

    DUER_LOGD("Get flash payload_string :[%s]", payload_string);
}

