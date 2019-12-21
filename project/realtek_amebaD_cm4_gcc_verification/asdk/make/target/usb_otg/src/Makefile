
include $(MAKE_INCLUDE_GEN)

.PHONY: all clean

#*****************************************************************************#
#                               VARIABLES	                                  #
#*****************************************************************************#
DIR = $(BASEDIR)/component/soc/realtek/amebad/fwlib/usb_otg/src
MODULE_IFLAGS = -I$(DIR)/../include/

#MODULE_IFLAGS = -I$(SWLIBDIR)/drivers/usb/device/class
#MODULE_IFLAGS += -I$(SWLIBDIR)/drivers/usb/device/core

vpath %.c $(DIR)

#*****************************************************************************#
#                               Source FILE LIST                              #
#*****************************************************************************#
ifeq ($(CONFIG_USB_BOOT_SIM),y)
CSRC = $(DIR)/core_otg.c \
	$(DIR)/dwc_otg_common.c \
	$(DIR)/dwc_otg_driver.c \
	$(DIR)/dwc_otg_hcd_os.c \
	$(DIR)/hal_otg.c \
	$(DIR)/rtl8721d_otg.c \
	$(DIR)/usb.c
else
CSRC = $(DIR)/hal_otg.c \
	$(DIR)/dwc_otg_hcd_os.c \
	$(DIR)/dwc_otg_driver_rom.c \
	$(DIR)/dwc_otg_driver.c \
	$(DIR)/dwc_otg_common.c \
	$(DIR)/dwc_otg_common_rom.c \
	$(DIR)/dwc_otg_hcd.c \
	$(DIR)/dwc_otg_hcd_intr.c \
	$(DIR)/dwc_otg_hcd_queue.c \
	$(DIR)/dwc_otg_cil.c \
	$(DIR)/dwc_otg_cil_intr.c \
	$(DIR)/dwc_otg_pcd.c \
	$(DIR)/dwc_otg_pcd_intr.c \
	$(DIR)/rtl8721d_otg_epautoconf.c \
	$(DIR)/dwc_otg_pcd_linux.c \
	$(DIR)/core_otg.c \
	$(DIR)/dwc_os.c \
	$(DIR)/dwc_otg_dbg.c \
	$(DIR)/usb_gadget.c \
	$(DIR)/usb.c \
#	$(DIR)/dwc_otg_adp.c \
#	$(DIR)/dwc_otg_attr.c \
#	$(DIR)/dwc_otg_hcd_ddma.c
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
	make -C dwc_common_port all
#*****************************************************************************#
#              GENERATE OBJECT FILE 
#*****************************************************************************#
CORE_TARGETS:	$(OBJS)


#*****************************************************************************#
#                        RULES TO CLEAN TARGETS                               #
#*****************************************************************************#
clean: CLEAN_OBJS
	#make -C src clean
	make -C dwc_common_port clean
	$(REMOVE) *.o
	$(REMOVE) *.i
	$(REMOVE) *.s
	$(REMOVE) *.d

-include $(DEPS)
