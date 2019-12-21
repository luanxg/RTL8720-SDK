
include $(MAKE_INCLUDE_GEN)

.PHONY: all clean

#*****************************************************************************#
#                               VARIABLES	                                  #
#*****************************************************************************#

#MODULE_IFLAGS = -I../../../../../../include
#MODULE_IFLAGS += -I../../../../../../include/rtl8195a
#MODULE_IFLAGS += -I../../../../../../../../../../../../freertos/infra/include

DIR = $(BASEDIR)/component/soc/realtek/amebad/fwlib/usb_otg/src/dwc_common_port
MODULE_IFLAGS = -I$(DIR)/../../include/

vpath %.c $(DIR)

#*****************************************************************************#
#                               Source FILE LIST                              #
#*****************************************************************************#
ifeq ($(CONFIG_USB_BOOT_SIM),y)
CSRC = 
else
CSRC = $(DIR)/dwc_common_os.c \
#	$(DIR)/dwc_mem.c
endif

#*****************************************************************************#
#                               Object FILE LIST                              #
#*****************************************************************************#
OBJS = $(notdir $(CSRC:.c=.o))

#*****************************************************************************#
#                        RULES TO GENERATE TARGETS                            #
#*****************************************************************************#

# Define the Rules to build the core targets
all: CLEAN_OBJS CORE_TARGETS COPY_RAM_OBJS

#*****************************************************************************#
#              GENERATE OBJECT FILE 
#*****************************************************************************#
CORE_TARGETS:	$(OBJS)

#*****************************************************************************#
#                        RULES TO CLEAN TARGETS                               #
#*****************************************************************************#
clean: CLEAN_OBJS
	$(REMOVE) *.o
	$(REMOVE) *.i
	$(REMOVE) *.s
	$(REMOVE) *.d

