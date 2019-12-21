#include "mmf_source.h"
#include "mmf_source_h264_unit.h"

void* h264_unit_open(void)
{
	struct h264_context *h264_ctx = (struct h264_context *) malloc(sizeof(struct h264_context));
	if (h264_ctx == NULL) {
		return NULL;
	}
	memset(h264_ctx, 0, sizeof(struct h264_context));

	h264_stream_init(h264_ctx);
	h264_enc_init(h264_ctx);

	//voe_init(h264_ctx);
	//isp_init(h264_ctx);

	return h264_ctx;
}

void h264_unit_close(void* ctx)
{
	free((struct h264_context *)ctx);
}

int h264_unit_set_param(void* ctx, int cmd, int arg)
{
	switch(cmd){
		default:
			break;
	}
	return 0;
}

int h264_unit_handle(void* ctx, void* b)
{
	int ret = 0;
	exch_buf_t *exbuf = (exch_buf_t*)b;

	ret = h264_stream_get_frame(ctx, b);
	
	mmf_source_add_exbuf_sending_list_all(exbuf);
	
	return 0;
}

msrc_module_t h264_unit_module =
{
	.create = h264_unit_open,
	.destroy = h264_unit_close,
	.set_param = h264_unit_set_param,
	.handle = h264_unit_handle,
};
