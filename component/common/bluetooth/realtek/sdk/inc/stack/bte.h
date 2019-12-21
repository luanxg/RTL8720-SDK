/**
 * Copyright (c) 2015, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BTE_H_
#define _BTE_H_

#include <stdbool.h>
#include <bt_flags.h>

#ifdef __cplusplus
extern "C" {
#endif

bool bte_init(void);
#if F_BT_DEINIT
void bte_deinit(void);
#endif

#ifdef __cplusplus
}
#endif

#endif  /* _BTE_H_ */
