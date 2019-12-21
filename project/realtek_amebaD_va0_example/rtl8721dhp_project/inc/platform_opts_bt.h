#ifndef __PLATFORM_OPTS_BT_H__
#define __PLATFORM_OPTS_BT_H__

#ifdef CONFIG_BT_EN
#define CONFIG_BT			1
#else
#define CONFIG_BT			0
#endif

#if CONFIG_BT
#define CONFIG_BT_CONFIG		0
#define CONFIG_BT_PERIPHERAL	1
#define CONFIG_BT_BEACON		0
#define CONFIG_BT_CENTRAL		0

#if CONFIG_BT_CENTRAL
#define CONFIG_BT_USER_COMMAND	0
#endif
#endif // CONFIG_BT

#endif // __PLATFORM_OPTS_BT_H__

