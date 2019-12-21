
include $(MAKE_INCLUDE_GEN)

.PHONY: all clean

DIR = $(BASEDIR)/component/common/drivers/usb_class/device/core/src
FW_DIR = $(BASEDIR)/component/soc/realtek/amebad/fwlib/usb_otg/src
DWC_DIR = $(BASEDIR)/component/soc/realtek/amebad/fwlib/usb_otg/src/dwc_common_port
MSC_DIR = $(BASEDIR)/component/common/drivers/usb_class/device/class

MODULE_IFLAGS = -I$(FW_DIR)/../include/
MODULE_IFLAGS += -I$(DIR)/../inc
MODULE_IFLAGS += -I$(BASEDIR)/component/common/drivers/sdio/realtek/sdio_host/inc

vpath %.c $(DIR) $(FW_DIR) $(DWC_DIR) $(MSC_DIR) $(shell find $(MSC_DIR) -type d)

#*****************************************************************************#
#                               Source FILE LIST                              #
#*****************************************************************************#
CSRC = $(DIR)/usb_config_rom.c
CSRC += $(DIR)/usb_composite_rom.c

CSRC += $(FW_DIR)/hal_otg.c \
	$(FW_DIR)/dwc_otg_hcd_os.c \
	$(FW_DIR)/dwc_otg_driver_rom.c \
	$(FW_DIR)/dwc_otg_driver.c \
	$(FW_DIR)/dwc_otg_common.c \
	$(FW_DIR)/dwc_otg_common_rom.c \
	$(FW_DIR)/dwc_otg_hcd.c \
	$(FW_DIR)/dwc_otg_hcd_intr.c \
	$(FW_DIR)/dwc_otg_hcd_queue.c \
	$(FW_DIR)/dwc_otg_cil.c \
	$(FW_DIR)/dwc_otg_cil_intr.c \
	$(FW_DIR)/dwc_otg_pcd.c \
	$(FW_DIR)/dwc_otg_pcd_intr.c \
	$(FW_DIR)/rtl8721d_otg_epautoconf.c \
	$(FW_DIR)/dwc_otg_pcd_linux.c \
	$(FW_DIR)/core_otg.c \
	$(FW_DIR)/dwc_os.c \
	$(FW_DIR)/dwc_otg_dbg.c \
	$(FW_DIR)/usb_gadget.c \
	$(FW_DIR)/usb.c \
	
CSRC += $(DWC_DIR)/dwc_common_os.c



#*****************************************************************************#
#                               Object FILE LIST                              #
#*****************************************************************************#
OBJS = $(notdir $(CSRC:.c=.o))

#*****************************************************************************#
#                        RULES TO GENERATE TARGETS                            #
#*****************************************************************************#

# Define the Rules to build the core targets
all: CORE_TARGETS
	$(AR) rvs lib_usbd.a $(OBJS)
	$(MOVE) -f lib_usbd.a $(ROOTDIR)/lib/application
	
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

-include $(DEPS)	