include $(MAKE_INCLUDE_GEN)

.PHONY: all clean

#*****************************************************************************#
#                               VARIABLES	                              #
#*****************************************************************************#

#*****************************************************************************#
#                        RULES TO GENERATE TARGETS                            #
#*****************************************************************************#
all: mk_copy
	@make -C xip all
	@make -C sram all
	@make -C library all
	@make -C src all

mk_copy:
	$(REMOVE) -rf ./src
	$(COPY) -r $(ROOTDIR)/../src ./

#*****************************************************************************#
#              CLEAN GENERATED FILES                                          #
#*****************************************************************************#
clean: mk_copy
	@make -C xip clean
	@make -C sram clean
	@make -C library clean
	@make -C src clean

-include $(DEPS)
