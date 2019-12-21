
include $(MAKE_INCLUDE_GEN)

.PHONY: all clean

DIR = $(BASEDIR)/component/common/drivers/usb_class/host/storage/src
MODULE_IFLAGS += -I$(DIR)/../inc

vpath %.c $(DIR) $(shell find $(DIR) -type d)

#*****************************************************************************#
#                               Source FILE LIST                              #
#*****************************************************************************#
CSRC = $(DIR)/us.c
CSRC += $(DIR)/us_intf.c
CSRC += $(DIR)/us_scsi.c
CSRC += $(DIR)/us_transport.c
CSRC += $(DIR)/us_protocol.c
CSRC += $(DIR)/us_initializers.c
CSRC += $(DIR)/usual_tables.c
CSRC += $(DIR)/us_debug.c
CSRC += $(DIR)/src/scsi_eh.c
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