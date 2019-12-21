
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
#all: CORE_TARGETS
all: CORE_TARGETS 
	make -C uvc all
#ifeq ($(CONFIG_USB_HOST_STORAGE),y)
#	make -C storage all
#endif

ifeq ($(CONFIG_USB_HOST_VENDOR),y)
	make -C vendor_spec all
endif

#*****************************************************************************#
#              GENERATE OBJECT FILE 
#*****************************************************************************#
CORE_TARGETS: $(OBJS)


#*****************************************************************************#
#                        RULES TO CLEAN TARGETS                               #
#*****************************************************************************#
clean: 
	make -C uvc clean
	make -C storage clean
	make -C vendor_spec clean
	make -C lib clean
-include $(DEPS)