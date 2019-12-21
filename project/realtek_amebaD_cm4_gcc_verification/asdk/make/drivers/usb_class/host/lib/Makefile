
include $(MAKE_INCLUDE_GEN)

.PHONY: all clean

STORAGE_DIR = $(BASEDIR)/component/common/drivers/usb_class/host/storage/src
FW_DIR = $(BASEDIR)/component/soc/realtek/amebad/fwlib/usb_otg/src
DWC_DIR = $(BASEDIR)/component/soc/realtek/amebad/fwlib/usb_otg/src/dwc_common_port

MODULE_IFLAGS = -I$(FW_DIR)/../include/
MODULE_IFLAGS += -I$(STORAGE_DIR)/../inc

vpath %.c $(STORAGE_DIR) $(shell find $(STORAGE_DIR) -type d) $(FW_DIR) $(DWC_DIR)

#*****************************************************************************#
#                               Source FILE LIST                              #
#*****************************************************************************#
CSRC = $(STORAGE_DIR)/us.c
CSRC += $(STORAGE_DIR)/us_intf.c
CSRC += $(STORAGE_DIR)/us_scsi.c
CSRC += $(STORAGE_DIR)/us_transport.c
CSRC += $(STORAGE_DIR)/us_protocol.c
CSRC += $(STORAGE_DIR)/us_initializers.c
CSRC += $(STORAGE_DIR)/usual_tables.c
CSRC += $(STORAGE_DIR)/us_debug.c
CSRC += $(STORAGE_DIR)/src/scsi_eh.c

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
all: CORE_TARGETS COPY_RAM_OBJS
	$(AR) rvs lib_usbh.a $(OBJS)
	$(MOVE) -f lib_usbh.a $(ROOTDIR)/lib/application
	
#*****************************************************************************#
#              GENERATE OBJECT FILE 
#*****************************************************************************#
CORE_TARGETS:	$(OBJS)


#*****************************************************************************#
#                        RULES TO CLEAN TARGETS                               #
#*****************************************************************************#
clean: CLEAN_OBJS 

-include $(DEPS)