
include $(MAKE_INCLUDE_GEN)

.PHONY: all clean

#*****************************************************************************#
#                               VARIABLES	                              #
#*****************************************************************************#
DIR = $(BASEDIR)/component/common/audio/hmp3
MODULE_IFLAGS = -I$(DIR)

vpath %.c $(DIR) $(shell find $(DIR) -type d)

#*****************************************************************************#
#                               Source FILE LIST                              #
#*****************************************************************************#
CSRC = $(DIR)/trigtabs.c
CSRC += $(DIR)/subband.c
CSRC += $(DIR)/stproc.c
CSRC += $(DIR)/scalfact.c
CSRC += $(DIR)/polyphase.c
CSRC += $(DIR)/mp3tabs.c
CSRC += $(DIR)/mp3dec.c
CSRC += $(DIR)/imdct.c
CSRC += $(DIR)/hufftabs.c
CSRC += $(DIR)/huffman.c
CSRC += $(DIR)/dqchan.c
CSRC += $(DIR)/dequant.c
CSRC += $(DIR)/dct32.c
CSRC += $(DIR)/buffers.c
CSRC += $(DIR)/bitstream.c
 



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

-include $(DEPS)