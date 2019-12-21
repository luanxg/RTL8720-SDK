
include $(MAKE_INCLUDE_GEN)

.PHONY: all clean

#*****************************************************************************#
#                               VARIABLES	                              #
#*****************************************************************************#
DIR = $(BASEDIR)/component/common/audio
AAC_DIR = $(DIR)/fdk-aac
AV_DIR = $(DIR)/libav


vpath %.cpp $(DIR) $(shell find $(DIR) -type d)
vpath %.c $(AV_DIR) $(shell find $(AV_DIR) -type d)


#*****************************************************************************#
#                          Header FILE Directory LIST                         #
#*****************************************************************************#
MODULE_IFLAGS = -I$(AAC_DIR)/libAACdec/include
MODULE_IFLAGS += -I$(AAC_DIR)/libFDK/include
MODULE_IFLAGS += -I$(AAC_DIR)/libMpegTPDec/include
MODULE_IFLAGS += -I$(AAC_DIR)/libMpegTPEnc/include
MODULE_IFLAGS += -I$(AAC_DIR)/libPCMutils/include
MODULE_IFLAGS += -I$(AAC_DIR)/libSBRdec/include
MODULE_IFLAGS += -I$(AAC_DIR)/libSBRenc/include

MODULE_IFLAGS += -I$(AV_DIR)/compat/atomics/dummy
#MODULE_IFLAGS += -I$(AV_DIR)/libavcodec
#MODULE_IFLAGS += -I$(AV_DIR)/libavformat
#MODULE_IFLAGS += -I$(AV_DIR)/libavutil


#*****************************************************************************#
#                               Source FILE LIST                              #
#*****************************************************************************#
CPPSRC = $(AAC_DIR)/libAACdec/src/aac_ram.cpp
CPPSRC += $(AAC_DIR)/libAACdec/src/aac_rom.cpp
CPPSRC += $(AAC_DIR)/libAACdec/src/aacdec_drc.cpp
CPPSRC += $(AAC_DIR)/libAACdec/src/aacdec_hcr.cpp
CPPSRC += $(AAC_DIR)/libAACdec/src/aacdec_hcr_bit.cpp
CPPSRC += $(AAC_DIR)/libAACdec/src/aacdec_hcrs.cpp
CPPSRC += $(AAC_DIR)/libAACdec/src/aacdec_pns.cpp
CPPSRC += $(AAC_DIR)/libAACdec/src/aacdec_tns.cpp
CPPSRC += $(AAC_DIR)/libAACdec/src/aacdecoder.cpp
CPPSRC += $(AAC_DIR)/libAACdec/src/aacdecoder_lib.cpp
CPPSRC += $(AAC_DIR)/libAACdec/src/block.cpp
CPPSRC += $(AAC_DIR)/libAACdec/src/channel.cpp
CPPSRC += $(AAC_DIR)/libAACdec/src/channelinfo.cpp
CPPSRC += $(AAC_DIR)/libAACdec/src/conceal.cpp
CPPSRC += $(AAC_DIR)/libAACdec/src/ldfiltbank.cpp
CPPSRC += $(AAC_DIR)/libAACdec/src/pulsedata.cpp
CPPSRC += $(AAC_DIR)/libAACdec/src/rvlc.cpp
CPPSRC += $(AAC_DIR)/libAACdec/src/rvlcbit.cpp
CPPSRC += $(AAC_DIR)/libAACdec/src/rvlcconceal.cpp
CPPSRC += $(AAC_DIR)/libAACdec/src/stereo.cpp

CPPSRC += $(AAC_DIR)/libFDK/src/autocorr2nd.cpp
CPPSRC += $(AAC_DIR)/libFDK/src/dct.cpp
CPPSRC += $(AAC_DIR)/libFDK/src/FDK_bitbuffer.cpp
CPPSRC += $(AAC_DIR)/libFDK/src/FDK_core.cpp
CPPSRC += $(AAC_DIR)/libFDK/src/FDK_crc.cpp
CPPSRC += $(AAC_DIR)/libFDK/src/FDK_hybrid.cpp
CPPSRC += $(AAC_DIR)/libFDK/src/FDK_tools_rom.cpp
CPPSRC += $(AAC_DIR)/libFDK/src/FDK_trigFcts.cpp
CPPSRC += $(AAC_DIR)/libFDK/src/fft.cpp
CPPSRC += $(AAC_DIR)/libFDK/src/fft_rad2.cpp
CPPSRC += $(AAC_DIR)/libFDK/src/fixpoint_math.cpp
CPPSRC += $(AAC_DIR)/libFDK/src/mdct.cpp
CPPSRC += $(AAC_DIR)/libFDK/src/qmf.cpp
CPPSRC += $(AAC_DIR)/libFDK/src/scale.cpp

CPPSRC += $(AAC_DIR)/libMpegTPDec/src/tpdec_adif.cpp
CPPSRC += $(AAC_DIR)/libMpegTPDec/src/tpdec_adts.cpp
CPPSRC += $(AAC_DIR)/libMpegTPDec/src/tpdec_asc.cpp
CPPSRC += $(AAC_DIR)/libMpegTPDec/src/tpdec_drm.cpp
CPPSRC += $(AAC_DIR)/libMpegTPDec/src/tpdec_latm.cpp
CPPSRC += $(AAC_DIR)/libMpegTPDec/src/tpdec_lib.cpp

CPPSRC += $(AAC_DIR)/libMpegTPEnc/src/tpenc_adif.cpp
CPPSRC += $(AAC_DIR)/libMpegTPEnc/src/tpenc_adts.cpp
CPPSRC += $(AAC_DIR)/libMpegTPEnc/src/tpenc_asc.cpp
CPPSRC += $(AAC_DIR)/libMpegTPEnc/src/tpenc_latm.cpp
CPPSRC += $(AAC_DIR)/libMpegTPEnc/src/tpenc_lib.cpp

CPPSRC += $(AAC_DIR)/libPCMutils/src/limiter.cpp
CPPSRC += $(AAC_DIR)/libPCMutils/src/pcmutils_lib.cpp

CPPSRC += $(AAC_DIR)/libSBRdec/src/env_calc.cpp
CPPSRC += $(AAC_DIR)/libSBRdec/src/env_dec.cpp
CPPSRC += $(AAC_DIR)/libSBRdec/src/env_extr.cpp
CPPSRC += $(AAC_DIR)/libSBRdec/src/huff_dec.cpp
CPPSRC += $(AAC_DIR)/libSBRdec/src/lpp_tran.cpp
CPPSRC += $(AAC_DIR)/libSBRdec/src/psbitdec.cpp
CPPSRC += $(AAC_DIR)/libSBRdec/src/psdec.cpp
CPPSRC += $(AAC_DIR)/libSBRdec/src/psdec_hybrid.cpp
CPPSRC += $(AAC_DIR)/libSBRdec/src/sbr_crc.cpp
CPPSRC += $(AAC_DIR)/libSBRdec/src/sbr_deb.cpp
CPPSRC += $(AAC_DIR)/libSBRdec/src/sbr_dec.cpp
CPPSRC += $(AAC_DIR)/libSBRdec/src/sbr_ram.cpp
CPPSRC += $(AAC_DIR)/libSBRdec/src/sbr_rom.cpp
CPPSRC += $(AAC_DIR)/libSBRdec/src/sbrdec_drc.cpp
CPPSRC += $(AAC_DIR)/libSBRdec/src/sbrdec_freq_sca.cpp
CPPSRC += $(AAC_DIR)/libSBRdec/src/sbrdecoder.cpp

CPPSRC += $(AAC_DIR)/libSBRenc/src/bit_sbr.cpp
CPPSRC += $(AAC_DIR)/libSBRenc/src/code_env.cpp
CPPSRC += $(AAC_DIR)/libSBRenc/src/env_bit.cpp
CPPSRC += $(AAC_DIR)/libSBRenc/src/env_est.cpp
CPPSRC += $(AAC_DIR)/libSBRenc/src/fram_gen.cpp
CPPSRC += $(AAC_DIR)/libSBRenc/src/invf_est.cpp
CPPSRC += $(AAC_DIR)/libSBRenc/src/mh_det.cpp
CPPSRC += $(AAC_DIR)/libSBRenc/src/nf_est.cpp
CPPSRC += $(AAC_DIR)/libSBRenc/src/ps_bitenc.cpp
CPPSRC += $(AAC_DIR)/libSBRenc/src/ps_encode.cpp
CPPSRC += $(AAC_DIR)/libSBRenc/src/ps_main.cpp
CPPSRC += $(AAC_DIR)/libSBRenc/src/resampler.cpp
CPPSRC += $(AAC_DIR)/libSBRenc/src/sbr_encoder.cpp
CPPSRC += $(AAC_DIR)/libSBRenc/src/sbr_misc.cpp
CPPSRC += $(AAC_DIR)/libSBRenc/src/sbr_ram.cpp
CPPSRC += $(AAC_DIR)/libSBRenc/src/sbr_rom.cpp
CPPSRC += $(AAC_DIR)/libSBRenc/src/sbrenc_freq_sca.cpp
CPPSRC += $(AAC_DIR)/libSBRenc/src/ton_corr.cpp
CPPSRC += $(AAC_DIR)/libSBRenc/src/tran_det.cpp

CPPSRC += $(AAC_DIR)/libSYS/src/cmdl_parser.cpp
CPPSRC += $(AAC_DIR)/libSYS/src/conv_string.cpp
CPPSRC += $(AAC_DIR)/libSYS/src/genericStds.cpp
CPPSRC += $(AAC_DIR)/libSYS/src/wav_file.cpp


# CSRC += $(AV_DIR)/libavcodec/ac3tab.c
# CSRC += $(AV_DIR)/libavcodec/avpacket.c
# CSRC += $(AV_DIR)/libavcodec/bitstream_filters.c
# CSRC += $(AV_DIR)/libavcodec/bsf.c
# CSRC += $(AV_DIR)/libavcodec/codec_desc.c
# CSRC += $(AV_DIR)/libavcodec/decode.c
# CSRC += $(AV_DIR)/libavcodec/mpeg4audio.c
# CSRC += $(AV_DIR)/libavcodec/mpegaudiodata.c
# CSRC += $(AV_DIR)/libavcodec/null_bsf.c
# #CSRC += $(AV_DIR)/libavcodec/options.c
# CSRC += $(AV_DIR)/libavcodec/options_libavcodec.c
# CSRC += $(AV_DIR)/libavcodec/parser.c
# CSRC += $(AV_DIR)/libavcodec/utils_libavcodec.c

# CSRC += $(AV_DIR)/libavformat/allformats.c
# CSRC += $(AV_DIR)/libavformat/avio.c
# CSRC += $(AV_DIR)/libavformat/aviobuf.c
# CSRC += $(AV_DIR)/libavformat/avtcp.c
# CSRC += $(AV_DIR)/libavformat/cutils.c
# CSRC += $(AV_DIR)/libavformat/file.c
# CSRC += $(AV_DIR)/libavformat/format.c
# CSRC += $(AV_DIR)/libavformat/hls.c
# CSRC += $(AV_DIR)/libavformat/http.c
# CSRC += $(AV_DIR)/libavformat/id3v1.c
# CSRC += $(AV_DIR)/libavformat/id3v2.c
# CSRC += $(AV_DIR)/libavformat/isom.c
# CSRC += $(AV_DIR)/libavformat/metadata.c
# CSRC += $(AV_DIR)/libavformat/mov.c
# CSRC += $(AV_DIR)/libavformat/mov_chan.c
# CSRC += $(AV_DIR)/libavformat/mov_esds.c
# CSRC += $(AV_DIR)/libavformat/mpegts.c
# CSRC += $(AV_DIR)/libavformat/network.c
# CSRC += $(AV_DIR)/libavformat/options.c
# CSRC += $(AV_DIR)/libavformat/os_support.c
# # CSRC += $(AV_DIR)/libavformat/protocol_list.c
# CSRC += $(AV_DIR)/libavformat/protocols.c
# CSRC += $(AV_DIR)/libavformat/replaygain.c
# CSRC += $(AV_DIR)/libavformat/riff.c
# CSRC += $(AV_DIR)/libavformat/riffdec.c
# CSRC += $(AV_DIR)/libavformat/url.c
# CSRC += $(AV_DIR)/libavformat/utils_libavformat.c

# CSRC += $(AV_DIR)/libavutil/averror.c
# CSRC += $(AV_DIR)/libavutil/avstring.c
# CSRC += $(AV_DIR)/libavutil/buffer.c
# CSRC += $(AV_DIR)/libavutil/channel_layout.c
# CSRC += $(AV_DIR)/libavutil/crc.c
# CSRC += $(AV_DIR)/libavutil/dict.c
# CSRC += $(AV_DIR)/libavutil/eval.c
# CSRC += $(AV_DIR)/libavutil/frame.c
# CSRC += $(AV_DIR)/libavutil/imgutils.c
# CSRC += $(AV_DIR)/libavutil/intmath.c
# CSRC += $(AV_DIR)/libavutil/log.c
# CSRC += $(AV_DIR)/libavutil/log2_tab.c
# CSRC += $(AV_DIR)/libavutil/mathematics.c
# #CSRC += $(AV_DIR)/libavutil/mem.c
# CSRC += $(AV_DIR)/libavutil/mem_libavutil.c
# CSRC += $(AV_DIR)/libavutil/opt.c
# CSRC += $(AV_DIR)/libavutil/parseutils.c
# CSRC += $(AV_DIR)/libavutil/pixdesc.c
# CSRC += $(AV_DIR)/libavutil/rational.c
# CSRC += $(AV_DIR)/libavutil/samplefmt.c
# CSRC += $(AV_DIR)/libavutil/time.c



#*****************************************************************************#
#                               Object FILE LIST                              #
#*****************************************************************************#
CPPOBJS = $(notdir $(CPPSRC:.cpp=.o))
COBJS = $(notdir $(CSRC:.c=.o))
OBJS = $(CPPOBJS)
OBJS += $(COBJS)

#*****************************************************************************#
#                        RULES TO GENERATE TARGETS                            #
#*****************************************************************************#

# Define the Rules to build the core targets
all: CORE_TARGETS COPY_RAM_OBJS
	
#*****************************************************************************#
#              GENERATE OBJECT FILE 
#*****************************************************************************#
CORE_TARGETS:	$(CPPOBJS) $(COBJS)


#*****************************************************************************#
#                        RULES TO CLEAN TARGETS                               #
#*****************************************************************************#
clean: CLEAN_OBJS 
	$(REMOVE) *.o
	$(REMOVE) *.i
	$(REMOVE) *.s
	$(REMOVE) *.d

-include $(DEPS)	
