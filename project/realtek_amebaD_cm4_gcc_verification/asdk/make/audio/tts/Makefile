
include $(MAKE_INCLUDE_GEN)

.PHONY: all clean

#*****************************************************************************#
#                               VARIABLES	                              #
#*****************************************************************************#
DIR = $(BASEDIR)/component/common/audio/tts


MODULE_IFLAGS += -I$(DIR)

vpath %.cpp $(DIR) $(shell find $(DIR) -type d)

#*****************************************************************************#
#                          Header FILE Directory LIST                         #
#*****************************************************************************#
MODULE_IFLAGS += -I$(DIR)/../../example/audio_sport/audio_tts

#*****************************************************************************#
#                               Source FILE LIST                              #
#*****************************************************************************#
CSRC = $(DIR)/compiledict.cpp
CSRC += $(DIR)/dictionary.cpp
CSRC += $(DIR)/intonation.cpp
CSRC += $(DIR)/klatt.cpp
CSRC += $(DIR)/mbrowrap.cpp
CSRC += $(DIR)/numbers.cpp
CSRC += $(DIR)/phonemelist.cpp
CSRC += $(DIR)/readclause.cpp
CSRC += $(DIR)/setlengths.cpp
CSRC += $(DIR)/sonic.cpp
CSRC += $(DIR)/speak.cpp
CSRC += $(DIR)/synth_mbrola.cpp
CSRC += $(DIR)/synthdata.cpp
CSRC += $(DIR)/synthesize.cpp
CSRC += $(DIR)/tr_languages.cpp
CSRC += $(DIR)/translate.cpp
CSRC += $(DIR)/voices.cpp
CSRC += $(DIR)/wave.cpp
CSRC += $(DIR)/wavegen.cpp



	
#*****************************************************************************#
#                               Object FILE LIST                              #
#*****************************************************************************#
OBJS = $(notdir $(CSRC:.cpp=.o))

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
