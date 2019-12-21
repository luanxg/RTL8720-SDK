/**
 * Copyright (2017) Baidu Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
// Author: Zhang Leliang(zhangleliang@baidu.com)
//
// Description: bitmap cache for fix-sized objects.

#include "stdio.h"
#include "stdlib.h"

#include "lightduer_bitmap.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"

static duer_mutex_t s_bitmap_lock = NULL;

int dump_bitmap(bitmap_objects_t *bop, const char *tag)
{
    DUER_LOGI("=========%s", tag);
    DUER_LOGI("num_of_objs:%d, num_of_bitmaps:%d\n", bop->num_of_objs, bop->num_of_bitmaps);
    for (int i = 0; i < bop->num_of_bitmaps; ++i) {
        DUER_LOGI("%x ", bop->bitmaps[i]);
    }
    DUER_LOGI("\n");
    DUER_LOGI("=========%s", tag);
    return 0;
}

int clear_index(bitmap_objects_t *bop, int index)
{
    if (index >= bop->num_of_objs) {
        DUER_LOGE("index(%d) bigger than max(%d)", index, bop->num_of_objs);
        return -1;
    }

    //int i_of_bitmap = index / (sizeof(unisgned char) * 8);
    //int offset_of_bitmap = index % (sizeof(unsigned char) * 8);
    int i_of_bitmap = index >> 3;
    int offset_of_bitmap = index & 0x07;

    unsigned char mask = 0x1 << offset_of_bitmap;
    //printf("test index:%d, i:%d, offset:%d, 0x%x\n", index, i_of_bitmap, offset_of_bitmap, mask);
    if (bop->bitmaps[i_of_bitmap] & mask) {
        bop->bitmaps[i_of_bitmap] &= ~mask;
        return 0;
    } else {
        DUER_LOGE("index(%d) already clear!", index);
        return -1;
    }
}

int set_index(bitmap_objects_t *bop, int index)
{
    if (index >= bop->num_of_objs) {
        DUER_LOGE("index(%d) bigger than max(%d)", index, bop->num_of_objs);
        return -1;
    }

    //int i_of_bitmap = index / (sizeof(unsigned char) * 8);
    //int offset_of_bitmap = index % (sizeof(unsigned char) * 8);
    int i_of_bitmap = index >> 3;
    int offset_of_bitmap = index & 0x07;


    unsigned char mask = 0x1 << offset_of_bitmap;
    //printf("test index:%d, i:%d, offset:%d, 0x%x\n", index, i_of_bitmap, offset_of_bitmap, mask);
    if (!(bop->bitmaps[i_of_bitmap] & mask)) {
        bop->bitmaps[i_of_bitmap] |= mask;
        return 0;
    } else {
        DUER_LOGE("index(%d) already set!", index);
        return -1;
    }
}

int first_free_index(bitmap_objects_t *bop)
{
    int index = -1;
    int current_index = 0;
    int found = 0;

    for (int i = 0; i < bop->num_of_bitmaps; ++i) {
        if (found || current_index >= bop->num_of_objs) {
            break;
        }
        unsigned char b = bop->bitmaps[i];

        //printf("test, %x, current_index:%d \n", b, current_index);
        for (int j = 0; j < sizeof(unsigned char) * 8; ++j) {
            if (!(b & 0x1)) {
                found = 1;
                break;
            }
            b >>= 1;
            if(++current_index >= bop->num_of_objs) {
                break;
            }
        }
    }

    if (current_index < bop->num_of_objs) {
        if (set_index(bop, current_index) < 0) {//Note: whether it is perfect to set here
            return index;
        }
        index = current_index;
    }
    return index;
}

int first_set_index(bitmap_objects_t *bop)
{
    int index = -1;
    int current_index = 0;
    int found = 0;

    for (int i = 0; i < bop->num_of_bitmaps; ++i) {
        if (found || current_index >= bop->num_of_objs) {
            break;
        }
        unsigned char b = bop->bitmaps[i];
        for (int j = 0; j < sizeof(unsigned char) * 8; ++j) {
            if ((b & 0x1)) {
                found = 1;
                break;
            }
            b >>= 1;
            if(++current_index >= bop->num_of_objs) {
                break;
            }
        }
    }

    if (current_index < bop->num_of_objs) {
        index = current_index;
    }
    return index;
}

int init_bitmap(int object_num, bitmap_objects_t *bop, int element_size)
{
    if (bop == NULL || object_num <= 0) {
        DUER_LOGW("invalid params!");
        return -1;
    }
    if (uninit_bitmap(bop) != 0) {
        DUER_LOGW("invalid status!");
        return -1;
    }
    int num_of_objs_in_one_type = sizeof(unsigned char) * 8;
    int bitmap_num = (object_num + num_of_objs_in_one_type - 1) / (num_of_objs_in_one_type);

    int all_size = object_num * element_size + bitmap_num * sizeof(unsigned char);

    DUER_LOGI("test obj_num:%d, bitmap_num:%d,all_size:%d, nooiot:%d",
                    object_num, bitmap_num, all_size, num_of_objs_in_one_type);

    bop->num_of_objs = object_num;
    bop->num_of_bitmaps = bitmap_num;
    bop->bitmap_lock = NULL; // not used now
    bop->element_size = element_size;
    if (!s_bitmap_lock) {
        s_bitmap_lock = duer_mutex_create(); // maybe fail, beccause the mutex not initialized.
    }
    if (!bop->bitmaps) {
        bop->bitmaps = DUER_CALLOC(bitmap_num * sizeof(unsigned char), 1);
    }
    if (!bop->objects) {
        bop->objects = DUER_MALLOC(object_num * element_size);
    }

    return 0;
}

int uninit_bitmap(bitmap_objects_t *bop)
{
    //check if all bitmap is free
    if (bop->bitmaps) {
        int f_s_index = first_set_index(bop);
        if(f_s_index != -1) {
            DUER_LOGE("the first set index is %d!", f_s_index);
            return -1;
        }
    }

    bop->num_of_objs = 0;
    bop->num_of_bitmaps = 0;
    // NOTE, keep the mutex
    // if (s_bitmap_lock) {
    //     duer_mutex_destroy(s_bitmap_lock);
    //     s_bitmap_lock = NULL;
    // }
    if (bop->bitmaps) {
        DUER_FREE(bop->bitmaps);
        bop->bitmaps = NULL;
    }
    if (bop->objects) {
        DUER_FREE(bop->objects);
        bop->objects = NULL;
    }
    return 0;
}

void *alloc_obj(bitmap_objects_t *bop)
{
    if (!s_bitmap_lock) {
        //Note if null will create because the mutex maybe craete fail in init func
        s_bitmap_lock = duer_mutex_create();
        if (!s_bitmap_lock) {
            DUER_LOGE("mutex is not initialized!");
            return NULL;
        }
    }
    // if (!s_bitmap_lock) {
    //     printf("mutex create fail!\n");
    // }
    // int r = 0;
    duer_mutex_lock(s_bitmap_lock);
    // if (r != 0) {
    //     printf("fail to lock\n");
    // }
    // dump_bitmap(&bitmap_objects, "before alloc");
    int index = first_free_index(bop);
    duer_mutex_unlock(s_bitmap_lock);
    if (index >= 0) {
        // dump_bitmap(&bitmap_objects, "after alloc");
        void *obj = (char*)bop->objects + (bop->element_size * index);

        return obj;
    }
    return NULL;
}

int free_obj(bitmap_objects_t *bop, void *obj)
{
    if (obj < bop->objects || (char*)obj >= ((char*)bop->objects + bop->num_of_objs * bop->element_size)) {
        //DUER_LOGI("obj not in the bitmap buffer:%p", obj); // maybe print too much
        return -2;
    }
    if (!s_bitmap_lock) {
        DUER_LOGE("mutex is not initialized!");
        return -1;
    }
    int index = ((char*)obj - (char*)bop->objects) / bop->element_size;
    //printf("index:%d, obj:%p, base:%p\n", index, obj, bop->objects);
    // int r = 0;
    duer_mutex_lock(s_bitmap_lock);
    //dump_bitmap(&bitmap_objects, "before free");
    // if (r != 0) {
    //     printf("fail to lock\n");
    // }
    int r = clear_index(bop, index);
    //dump_bitmap(&bitmap_objects, "after free");
    duer_mutex_unlock(s_bitmap_lock);
    return r;
}
