
include $(MAKE_INCLUDE_GEN)

.PHONY: all clean

#*****************************************************************************#
#                               VARIABLES	                              #
#*****************************************************************************#
DIR = $(BASEDIR)/component/common/audio/a52dec
MODULE_IFLAGS = -I$(DIR)

vpath %.c $(DIR) $(shell find $(DIR) -type d)

#*****************************************************************************#
#                          Header FILE Directory LIST                         #
#*****************************************************************************#
MODULE_IFLAGS += -I$(BASEDIR)/component/common/audio/a52dec/include
MODULE_IFLAGS += -I$(BASEDIR)/component/common/audio/a52dec/src
MODULE_IFLAGS += -I$(BASEDIR)/component/common/audio/a52dec/libao
MODULE_IFLAGS += -I$(BASEDIR)/component/common/audio/a52dec/liba52

#*****************************************************************************#
#                               Source FILE LIST                              #
#*****************************************************************************#
CSRC = $(DIR)/src/a52dec.c
#CSRC += $(DIR)/src/getopt.c
CSRC += $(DIR)/src/gettimeofday.c
CSRC += $(DIR)/lib52/bit_allocate.c
CSRC += $(DIR)/lib52/bitstream.c
CSRC += $(DIR)/lib52/downmix.c
CSRC += $(DIR)/lib52/imdct.c
CSRC += $(DIR)/lib52/parse.c
CSRC += $(DIR)/libao/audio_out.c
CSRC += $(DIR)/libao/audio_out_sport.c
CSRC += $(DIR)/libao/float2s16.c

 



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