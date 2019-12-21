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
// Author: Su Hao (suhao@baidu.com)
//
// Description: The hashcode algorithm

#include "lightduer_hashcode.h"

/* constants for duer_hashcode() */
#define DUER__STRHASH_SHORTSTRING   4096L
#define DUER__STRHASH_MEDIUMSTRING  (256L * 1024L)
#define DUER__STRHASH_BLOCKSIZE     256L

#define DUER__SEED                  ((duer_u32_t) 0x738b3f34)

/* 'magic' constants for Murmurhash2 */
#define DUER__MAGIC_M  ((duer_u32_t) 0x5bd1e995UL)
#define DUER__MAGIC_R  24

DUER_LOC_IMPL duer_u32_t duer_hashcode_compute(const char* data, duer_size_t len, duer_u32_t seed) {
    duer_u32_t hash = seed ^ ((duer_u32_t)len);

    while (len >= 4) {
        /* Portability workaround is required for platforms without
        * unaligned access.  The replacement code emulates little
        * endian access even on big endian architectures, which is
        * OK as long as it is consistent for a build.
        */
        duer_u32_t k = ((duer_u32_t)data[0]) |
                      (((duer_u32_t)data[1]) << 8) |
                      (((duer_u32_t)data[2]) << 16) |
                      (((duer_u32_t)data[3]) << 24);
        k *= DUER__MAGIC_M;
        k ^= k >> DUER__MAGIC_R;
        k *= DUER__MAGIC_M;
        hash *= DUER__MAGIC_M;
        hash ^= k;
        data += 4;
        len -= 4;
    }

    if (len == 3) {
        hash ^= data[2] << 16;
        --len;
    }
    if (len == 2) {
        hash ^= data[1] << 8;
        --len;
    }
    if (len == 1) {
        hash ^= data[0];
        hash *= DUER__MAGIC_M;
    }

    hash ^= hash >> 13;
    hash *= DUER__MAGIC_M;
    hash ^= hash >> 15;
    return hash;
}

DUER_INT_IMPL duer_u32_t duer_hashcode(const char* data, duer_size_t size,
                                    duer_u32_t seed) {
    duer_u32_t hash = 0;
    /*
     *  Sampling long strings by byte skipping (like Lua does) is potentially
     *  a cache problem.  Here we do 'block skipping' instead for long strings:
     *  hash an initial part, and then sample the rest of the string with
     *  reasonably sized chunks.
     *
     *  Skip should depend on length and bound the total time to roughly
     *  logarithmic.
     *
     *  With current values:
     *
     *    1M string => 256 * 241 = 61696 bytes (0.06M) of hashing
     *    1G string => 256 * 16321 = 4178176 bytes (3.98M) of hashing
     *
     *  After an initial part has been hashed, an offset is applied before
     *  starting the sampling.  The initial offset is computed from the
     *  hash of the initial part of the string.  The idea is to avoid the
     *  case that all long strings have certain offset ranges that are never
     *  sampled.
     */
    /* note: mixing len into seed improves hashing when skipping */
    duer_u32_t str_seed = seed ? seed : DUER__SEED;
    str_seed ^= ((duer_u32_t)size);

    if (size <= DUER__STRHASH_SHORTSTRING) {
        hash = duer_hashcode_compute(data, size, str_seed);
    } else {
        duer_size_t off;
        duer_size_t skip;

        if (size <= DUER__STRHASH_MEDIUMSTRING) {
            skip = (duer_size_t)(16 * DUER__STRHASH_BLOCKSIZE + DUER__STRHASH_BLOCKSIZE);
        } else {
            skip = (duer_size_t)(256 * DUER__STRHASH_BLOCKSIZE + DUER__STRHASH_BLOCKSIZE);
        }

        hash = duer_hashcode_compute(data, (duer_size_t)DUER__STRHASH_SHORTSTRING,
                                    str_seed);
        off = DUER__STRHASH_SHORTSTRING + (skip * (hash % 256)) / 256;

        /* XXX: inefficient loop */
        while (off < size) {
            duer_size_t left = size - off;
            duer_size_t now =
                (duer_size_t)(left > DUER__STRHASH_BLOCKSIZE ? DUER__STRHASH_BLOCKSIZE : left);
            hash ^= duer_hashcode_compute(data + off, now, str_seed);
            off += skip;
        }
    }

    return hash;
}

