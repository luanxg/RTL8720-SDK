
include $(MAKE_INCLUDE_GEN)

.PHONY: all clean


#*****************************************************************************#
#                               Object FILE LIST                              #
#*****************************************************************************#
OBJS = 

#*****************************************************************************#
#                        RULES TO GENERATE TARGETS                            #
#*****************************************************************************#

# Define the Rules to build the core targets
all: CORE_TARGETS
ifeq ($(CONFIG_BAIDU_DUER),y)
	make -C mp3 all
#	make -C ac3 all
#	make -C haac all
#	make -C hmp3 all
#	make -C m4a_selfparse all
#	make -C m4a all
#	make -C amr	all
endif
#	make -C hls all
#	make -C kws all
#	make -C flac all
#	make -C resample all
#*****************************************************************************#
#              GENERATE OBJECT FILE 
#*****************************************************************************#
CORE_TARGETS: $(OBJS)

	


#*****************************************************************************#
#                        RULES TO CLEAN TARGETS                               #
#*****************************************************************************#
clean: 
	make -C mp3 clean
#	make -C ac3 clean
#	make -C haac clean
#	make -C hmp3 clean
#	make -C m4a_selfparse clean
#	make -C m4a clean
#	make -C amr clean
#	make -C hls clean
#	make -C kws clean
#	make -C flac clean
	make -C resample clean

-include $(DEPS)
