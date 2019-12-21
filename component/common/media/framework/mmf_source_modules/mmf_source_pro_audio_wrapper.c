#include "mmf_source.h"
#include "mmf_source_pro_audio_wrapper.h"

msrc_module_t audio_wrapper_module =
{
	.create = audio_wrapper_open,
	.destroy = audio_wrapper_close,
	.set_param = audio_wrapper_set_param,
	.handle = audio_wrapper_source_mod_handle,
};
