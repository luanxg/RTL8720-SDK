
include $(MAKE_INCLUDE_GEN)

.PHONY: all clean

#*****************************************************************************#
#                               VARIABLES	                              #
#*****************************************************************************#
DIR = $(ROOTDIR)/../src
IGUIDIR =$(BASEDIR)/component/common/ui/ucgui

MODULE_IFLAGS = -I$(DIR)./
MODULE_IFLAGS = -I$(DIR)/demo_code/
MODULE_IFLAGS += -I$(IGUIDIR)./
MODULE_IFLAGS += -I$(IGUIDIR)/Config/
MODULE_IFLAGS += -I$(IGUIDIR)/GUI/Core/
MODULE_IFLAGS += -I$(IGUIDIR)/GUI/JPEG/
MODULE_IFLAGS += -I$(IGUIDIR)/GUI/MultiLayer/
MODULE_IFLAGS += -I$(IGUIDIR)/GUI/Widget/
MODULE_IFLAGS += -I$(IGUIDIR)/GUI/WM/
MODULE_IFLAGS += -I$(BASEDIR)/component/common/graphic/codec/jpeg

vpath %.c $(DIR)
#*****************************************************************************#
#                               Source FILE LIST                              #
#*****************************************************************************#
CSRC = $(DIR)/lcdc/bsp_rgb_lcd.c
CSRC += $(DIR)/lcdc/bsp_lcdc_font.c
CSRC += $(DIR)/lcdc/gt9147.c
CSRC += $(DIR)/lcdc/ctiic.c
CSRC += $(DIR)/lcdc/touch.c
CSRC += $(DIR)/main.c

#*****************************************************************************#
#                               Object FILE LIST                              #
#*****************************************************************************#

OBJS += $(notdir $(CSRC:.c=.o))

#*****************************************************************************#
#                        RULES TO GENERATE TARGETS                            #
#*****************************************************************************#
all: CLEAN_OBJS CORE_TARGETS COPY_RAM_OBJS
		make -C demo_code all
	
#*****************************************************************************#
#              GENERATE OBJECT FILE 
#*****************************************************************************#
CORE_TARGETS: $(OBJS)

#*****************************************************************************#
#                        RULES TO CLEAN TARGETS                               #
#*****************************************************************************#
clean: CLEAN_OBJS
	$(REMOVE) *.o
	$(REMOVE) *.i
	$(REMOVE) *.s
	$(REMOVE) *.d

-include $(DEPS)	
