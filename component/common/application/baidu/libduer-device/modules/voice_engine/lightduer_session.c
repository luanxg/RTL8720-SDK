// Copyright (2017) Baidu Inc. All rights reserveed.
//
// File: lightduer_session.h
// Auth: Su Hao (suhao@baidu.com)
// Desc: Manage the talk session.

#include "lightduer_session.h"
#include "lightduer_log.h"
#include "lightduer_mutex.h"
#include "lightduer_random.h"

#define DUER_SESSION_START      (10000)
#define DUER_SESSION_TATAL      (DUER_SESSION_START * 10)

#define DUER_SESSION_INACTIVE   (0x80000000)

static duer_u32_t   g_session;
static duer_mutex_t g_mutex;

static void local_mutex_lock(duer_bool status)
{
    duer_status_t rs;
    if (status) {
        rs = duer_mutex_lock(g_mutex);
    } else {
        rs = duer_mutex_unlock(g_mutex);
    }

    if (rs != DUER_OK) {
        DUER_LOGE("lock failed: rs = %d, status = %d", rs, status);
    }
}

void duer_session_initialize(void)
{
    if (g_mutex == NULL) {
        g_mutex = duer_mutex_create();
    }

    local_mutex_lock(DUER_TRUE);

    g_session = DUER_SESSION_START + (duer_u32_t)duer_random() % DUER_SESSION_TATAL;

    DUER_LOGI("random = %d", g_session);

    local_mutex_lock(DUER_FALSE);
}

duer_u32_t duer_session_generate(void)
{
    duer_u32_t rs;

    local_mutex_lock(DUER_TRUE);

    g_session &= (~DUER_SESSION_INACTIVE);

    rs = ++g_session;

    local_mutex_lock(DUER_FALSE);

    return rs;
}

duer_bool duer_session_consume(duer_u32_t id)
{
    duer_bool rs = DUER_FALSE;

    local_mutex_lock(DUER_TRUE);

    if (id == g_session) {
        g_session |= DUER_SESSION_INACTIVE;
        rs = DUER_TRUE;
    }

    local_mutex_lock(DUER_FALSE);

    return rs;
}

duer_bool duer_session_is_matched(duer_u32_t id)
{
    duer_bool rs;

    local_mutex_lock(DUER_TRUE);

    rs = (g_session == id) ? DUER_TRUE : DUER_FALSE;

    local_mutex_lock(DUER_FALSE);

    return rs;
}

void duer_session_finalize(void)
{
    if (g_mutex != NULL) {
        duer_mutex_destroy(g_mutex);
        g_mutex = NULL;
    }
}