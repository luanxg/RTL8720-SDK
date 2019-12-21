
include $(MAKE_INCLUDE_GEN)

.PHONY: all clean

#*****************************************************************************#
#                               VARIABLES	                              #
#*****************************************************************************#
DIR = $(BASEDIR)/component/soc/realtek/amebad/cmsis-dsp/Source

vpath %.c $(DIR) $(shell find $(DIR) -type d)
vpath %.S $(DIR) $(shell find $(DIR) -type d)
#*****************************************************************************#
#                               Source FILE LIST                              #
#*****************************************************************************#
CSRC = $(DIR)/BasicMathFunctions/arm_abs_f32.c \
	$(DIR)/BasicMathFunctions/arm_abs_q7.c \
	$(DIR)/BasicMathFunctions/arm_abs_q15.c \
	$(DIR)/BasicMathFunctions/arm_abs_q31.c \
	$(DIR)/BasicMathFunctions/arm_add_f32.c \
	$(DIR)/BasicMathFunctions/arm_add_q7.c \
	$(DIR)/BasicMathFunctions/arm_add_q15.c \
	$(DIR)/BasicMathFunctions/arm_add_q31.c \
	$(DIR)/BasicMathFunctions/arm_dot_prod_f32.c \
	$(DIR)/BasicMathFunctions/arm_dot_prod_q7.c \
	$(DIR)/BasicMathFunctions/arm_dot_prod_q15.c \
	$(DIR)/BasicMathFunctions/arm_dot_prod_q31.c \
	$(DIR)/BasicMathFunctions/arm_mult_f32.c \
	$(DIR)/BasicMathFunctions/arm_mult_q7.c \
	$(DIR)/BasicMathFunctions/arm_mult_q15.c \
	$(DIR)/BasicMathFunctions/arm_mult_q31.c \
	$(DIR)/BasicMathFunctions/arm_negate_f32.c \
	$(DIR)/BasicMathFunctions/arm_negate_q7.c \
	$(DIR)/BasicMathFunctions/arm_negate_q15.c \
	$(DIR)/BasicMathFunctions/arm_negate_q31.c \
	$(DIR)/BasicMathFunctions/arm_offset_f32.c \
	$(DIR)/BasicMathFunctions/arm_offset_q7.c \
	$(DIR)/BasicMathFunctions/arm_offset_q15.c \
	$(DIR)/BasicMathFunctions/arm_offset_q31.c \
	$(DIR)/BasicMathFunctions/arm_scale_f32.c \
	$(DIR)/BasicMathFunctions/arm_scale_q7.c \
	$(DIR)/BasicMathFunctions/arm_scale_q15.c \
	$(DIR)/BasicMathFunctions/arm_scale_q31.c \
	$(DIR)/BasicMathFunctions/arm_shift_q7.c \
	$(DIR)/BasicMathFunctions/arm_shift_q15.c \
	$(DIR)/BasicMathFunctions/arm_shift_q31.c \
	$(DIR)/BasicMathFunctions/arm_sub_f32.c \
	$(DIR)/BasicMathFunctions/arm_sub_q7.c \
	$(DIR)/BasicMathFunctions/arm_sub_q15.c \
	$(DIR)/BasicMathFunctions/arm_sub_q31.c \
	$(DIR)/CommonTables/arm_common_tables.c \
	$(DIR)/CommonTables/arm_const_structs.c \
	$(DIR)/ComplexMathFunctions/arm_cmplx_conj_f32.c \
	$(DIR)/ComplexMathFunctions/arm_cmplx_conj_q15.c \
	$(DIR)/ComplexMathFunctions/arm_cmplx_conj_q31.c \
	$(DIR)/ComplexMathFunctions/arm_cmplx_dot_prod_f32.c \
	$(DIR)/ComplexMathFunctions/arm_cmplx_dot_prod_q15.c \
	$(DIR)/ComplexMathFunctions/arm_cmplx_dot_prod_q31.c \
	$(DIR)/ComplexMathFunctions/arm_cmplx_mag_f32.c \
	$(DIR)/ComplexMathFunctions/arm_cmplx_mag_q15.c \
	$(DIR)/ComplexMathFunctions/arm_cmplx_mag_q31.c \
	$(DIR)/ComplexMathFunctions/arm_cmplx_mag_squared_f32.c \
	$(DIR)/ComplexMathFunctions/arm_cmplx_mag_squared_q15.c \
	$(DIR)/ComplexMathFunctions/arm_cmplx_mag_squared_q31.c \
	$(DIR)/ComplexMathFunctions/arm_cmplx_mult_cmplx_f32.c \
	$(DIR)/ComplexMathFunctions/arm_cmplx_mult_cmplx_q15.c \
	$(DIR)/ComplexMathFunctions/arm_cmplx_mult_cmplx_q31.c \
	$(DIR)/ComplexMathFunctions/arm_cmplx_mult_real_f32.c \
	$(DIR)/ComplexMathFunctions/arm_cmplx_mult_real_q15.c \
	$(DIR)/ComplexMathFunctions/arm_cmplx_mult_real_q31.c \
	$(DIR)/FastMathFunctions/arm_cos_f32.c \
	$(DIR)/FastMathFunctions/arm_cos_q15.c \
	$(DIR)/FastMathFunctions/arm_cos_q31.c \
	$(DIR)/FastMathFunctions/arm_sin_f32.c \
	$(DIR)/FastMathFunctions/arm_sin_q15.c \
	$(DIR)/FastMathFunctions/arm_sin_q31.c \
	$(DIR)/FastMathFunctions/arm_sqrt_q15.c \
	$(DIR)/FastMathFunctions/arm_sqrt_q31.c \
	$(DIR)/TransformFunctions/arm_bitreversal.c \
	$(DIR)/TransformFunctions/arm_cfft_f32.c \
	$(DIR)/TransformFunctions/arm_cfft_q15.c \
	$(DIR)/TransformFunctions/arm_cfft_q31.c \
	$(DIR)/TransformFunctions/arm_cfft_radix2_f32.c \
	$(DIR)/TransformFunctions/arm_cfft_radix2_init_f32.c \
	$(DIR)/TransformFunctions/arm_cfft_radix2_init_q15.c \
	$(DIR)/TransformFunctions/arm_cfft_radix2_init_q31.c \
	$(DIR)/TransformFunctions/arm_cfft_radix2_q15.c \
	$(DIR)/TransformFunctions/arm_cfft_radix2_q31.c \
	$(DIR)/TransformFunctions/arm_cfft_radix4_f32.c \
	$(DIR)/TransformFunctions/arm_cfft_radix4_init_f32.c \
	$(DIR)/TransformFunctions/arm_cfft_radix4_init_q15.c \
	$(DIR)/TransformFunctions/arm_cfft_radix4_init_q31.c \
	$(DIR)/TransformFunctions/arm_cfft_radix4_q15.c \
	$(DIR)/TransformFunctions/arm_cfft_radix4_q31.c \
	$(DIR)/TransformFunctions/arm_cfft_radix8_f32.c \
	$(DIR)/TransformFunctions/arm_dct4_f32.c \
	$(DIR)/TransformFunctions/arm_dct4_init_f32.c \
	$(DIR)/TransformFunctions/arm_dct4_init_q15.c \
	$(DIR)/TransformFunctions/arm_dct4_init_q31.c \
	$(DIR)/TransformFunctions/arm_dct4_q15.c \
	$(DIR)/TransformFunctions/arm_dct4_q31.c \
	$(DIR)/TransformFunctions/arm_rfft_f32.c \
	$(DIR)/TransformFunctions/arm_rfft_fast_f32.c \
	$(DIR)/TransformFunctions/arm_rfft_fast_init_f32.c \
	$(DIR)/TransformFunctions/arm_rfft_init_f32.c \
	$(DIR)/TransformFunctions/arm_rfft_init_q15.c \
	$(DIR)/TransformFunctions/arm_rfft_init_q31.c
	
	
ASMSRC = $(DIR)/TransformFunctions/arm_bitreversal2.S
	
	
	

#*****************************************************************************#
#                               Object FILE LIST                              #
#*****************************************************************************#
OBJS = $(notdir $(CSRC:.c=.o))
ASM_OBJS = $(notdir $(ASMSRC:.S=.o))
#*****************************************************************************#
#                        RULES TO GENERATE TARGETS                            #
#*****************************************************************************#

# Define the Rules to build the core targets
#all: CORE_TARGETS
all: CORE_TARGETS COPY_RAM_OBJS
	$(AR) rvs lib_cmsis_dsp.a $(OBJS)
	$(MOVE) -f lib_cmsis_dsp.a $(ROOTDIR)/lib/application
#*****************************************************************************#
#              GENERATE OBJECT FILE 
#*****************************************************************************#
CORE_TARGETS: $(OBJS) $(ASM_OBJS)


#*****************************************************************************#
#                        RULES TO CLEAN TARGETS                               #
#*****************************************************************************#
clean: CLEAN_OBJS 
	$(REMOVE) *.o
	$(REMOVE) *.i
	$(REMOVE) *.s
	$(REMOVE) *.d

-include $(DEPS)
