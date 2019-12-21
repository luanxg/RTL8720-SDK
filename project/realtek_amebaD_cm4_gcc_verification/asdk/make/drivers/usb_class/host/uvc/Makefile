
include $(MAKE_INCLUDE_GEN)

.PHONY: all clean

DIR = $(BASEDIR)/component/common/drivers/usb_class/host/uvc/src
MODULE_IFLAGS += -I$(DIR)/../inc

vpath %.c $(DIR)

#*****************************************************************************#
#                               Source FILE LIST                              #
#*****************************************************************************#
CSRC = 
#$(DIR)/uvc_ctrl.c
# CSRC += $(DIR)/uvc_driver.c
# CSRC += $(DIR)/uvc_intf.c
# CSRC += $(DIR)/uvc_queue.c
# CSRC += $(DIR)/uvc_status.c
# CSRC += $(DIR)/uvc_v4l2.c
# CSRC += $(DIR)/uvc_video.c

#*****************************************************************************#
#                               Object FILE LIST                              #
#*****************************************************************************#
OBJS = $(notdir $(CSRC:.c=.o))

#*****************************************************************************#
#                        RULES TO GENERATE TARGETS                            #
#*****************************************************************************#

# Define the Rules to build the core targets
all: CORE_TARGETS COPY_RAM_OBJS
	
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