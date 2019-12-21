
include $(MAKE_INCLUDE_GEN)


.PHONY: all clean

#*****************************************************************************#
#                               VARIABLES	                              #
#*****************************************************************************#
LWIPDIR = $(BASEDIR)/component/common/network/lwip/lwip_v1.4.1
NETWORK = $(BASEDIR)/component/common/network

IFLAGS               += -I$(BASEDIR)/component/common/network/coap/include
#*****************************************************************************#
#                               Source FILE LIST                              #
#*****************************************************************************#
OBJS += \
		$(LWIPDIR)/src/core/tcp_in.o \
		$(LWIPDIR)/src/core/tcp_out.o \
		$(LWIPDIR)/src/core/def.o \
		$(LWIPDIR)/src/core/ipv4/igmp.o \
		$(LWIPDIR)/src/core/pbuf.o \
		$(LWIPDIR)/src/core/ipv4/inet_chksum.o \
		$(LWIPDIR)/src/api/api_msg.o \
		$(LWIPDIR)/src/api/netbuf.o \
		$(LWIPDIR)/port/realtek/freertos/sys_arch.o \
		$(LWIPDIR)/port/realtek/freertos/ethernetif.o \

OBJS +=		\
		$(LWIPDIR)/src/core/lwip_timers.o \
		$(LWIPDIR)/src/core/raw.o \
		$(LWIPDIR)/src/core/mem.o \
		$(LWIPDIR)/src/core/memp.o \
		$(LWIPDIR)/src/core/netif.o \
		$(LWIPDIR)/src/core/ipv4/ip.o \
		$(LWIPDIR)/src/core/ipv4/ip_addr.o \
		$(LWIPDIR)/src/core/ipv4/ip_frag.o \
		$(LWIPDIR)/src/core/tcp.o \
		$(LWIPDIR)/src/core/udp.o \
		$(LWIPDIR)/src/api/sockets.o \
		$(LWIPDIR)/src/api/tcpip.o \
		$(LWIPDIR)/src/netif/etharp.o \
		$(LWIPDIR)/src/api/netdb.o \
		$(LWIPDIR)/src/api/api_lib.o \
		$(LWIPDIR)/src/api/netifapi.o \

#ifeq ($(CONFIG_HIGH_TP),y)
#OBJS += $(OBJS_CONFIG) 
#endif
#*****************************************************************************#
#                               Include Dependency                            #
#*****************************************************************************#
-include $(OBJS:.o=.d)


#*****************************************************************************#
#                        RULES TO GENERATE TARGETS                            #
#*****************************************************************************#
all: CORE_TARGETS RENAME_CODE2SRAM COPY_RAM_OBJS
#*****************************************************************************#
#              GENERATE OBJECT FILE 
#*****************************************************************************#
CORE_TARGETS: $(OBJS)

%.o:%.c
	$(CC) $(GLOBAL_CFLAGS) $(MODULE_IFLAGS) $<  -o $@

#*****************************************************************************#
#              CLEAN GENERATED FILES                                          #
#*****************************************************************************#
clean: CLEAN_OBJS 
	$(REMOVE) *.o
	$(REMOVE) *.i
	$(REMOVE) *.s
	$(REMOVE) *.d

-include $(DEPS)
