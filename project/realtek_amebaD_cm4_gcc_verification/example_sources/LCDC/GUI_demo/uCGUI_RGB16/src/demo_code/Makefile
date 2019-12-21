
include $(MAKE_INCLUDE_GEN)

.PHONY: all clean

#*****************************************************************************#
#                               VARIABLES	                              #
#*****************************************************************************#
DIR = $(ROOTDIR)/../src
IGUIDIR =$(BASEDIR)/component/common/ui/ucgui

MODULE_IFLAGS = -I$(DIR)/
MODULE_IFLAGS += -I$(DIR)/demo_code/
MODULE_IFLAGS+= -I$(IGUIDIR)./
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
CSRC = $(DIR)/lcdc/demo_code/clock_menu.c
CSRC += $(DIR)/lcdc/demo_code/grad_menu.c
CSRC += $(DIR)/lcdc/demo_code/gui_cus_parts.c
CSRC += $(DIR)/lcdc/demo_code/light_menu.c
CSRC += $(DIR)/lcdc/demo_code/main_menu.c
CSRC += $(DIR)/lcdc/demo_code/realtek_demo.c
CSRC += $(DIR)/lcdc/demo_code/start_menu.c
CSRC += $(DIR)/lcdc/demo_code/tempre_menu.c
CSRC += $(DIR)/lcdc/demo_code/wave_menu.c
CSRC += $(DIR)/lcdc/demo_code/clock_pic.c
CSRC += $(DIR)/lcdc/demo_code/gradienter_pic.c
CSRC += $(DIR)/lcdc/demo_code/light_off_pic.c
CSRC += $(DIR)/lcdc/demo_code/light_on_pic.c
CSRC += $(DIR)/lcdc/demo_code/light_pic.c
CSRC += $(DIR)/lcdc/demo_code/realtek_logo.c
CSRC += $(DIR)/lcdc/demo_code/return_btn_pic.c
CSRC += $(DIR)/lcdc/demo_code/tempre_pic.c
CSRC += $(DIR)/lcdc/demo_code/wave_pic.c

#*****************************************************************************#
#                               Object FILE LIST                              #
#*****************************************************************************#


OBJS = $(notdir $(CSRC:.c=.o))


#*****************************************************************************#
#                        RULES TO GENERATE TARGETS                            #
#*****************************************************************************#
all: CLEAN_OBJS CORE_TARGETS COPY_RAM_OBJS
	
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
