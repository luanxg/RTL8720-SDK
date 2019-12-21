#include "mmf_sink.h"
#include "mmf_sink_pro_audio_wrapper.h"

msink_module_t audio_wrapper_sink_module = 
{
	.create = audio_wrapper_open,
	.destroy = audio_wrapper_close,
	.set_param = audio_wrapper_set_param,
	.handle = audio_wrapper_sink_mod_handle,
};

