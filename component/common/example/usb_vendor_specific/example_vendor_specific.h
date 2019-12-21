#ifndef EXAMPLE_MEDIA_VS_H
#define EXAMPLE_MEDIA_VS_H

#include <platform/platform_stdlib.h>
#include "platform_opts.h"
#if !defined(CONFIG_PLATFORM_8721D)
#include "osdep_api.h"
#endif

void example_vendor_specific(void);

#endif 