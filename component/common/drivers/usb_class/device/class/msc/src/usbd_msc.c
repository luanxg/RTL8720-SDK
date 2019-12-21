#include "msc/inc/usbd_msc_desc.h"
#include "msc/inc/usbd_msc.h"
#include "msc/inc/usbd_scsi.h"
#include "core/inc/usb_config.h"
#include "core/inc/usb_composite.h"
#include "osdep_service.h"

// chose physical hard disk type
#include "sd.h"

// Config use thread to write data
#define CONGIF_USE_DATA_THREAD	0

/* MSC thread priority*/
#define MSC_BULK_OUT_CMD_THREAD_PRIORITY	0
#define MSC_BULK_DATA_THREAD_PRIORITY	0


// define global variables
struct msc_dev gMSC_DEVICE;
struct msc_common gMSC_COMMON;

// physical disk access methods
struct msc_opts disk_operation;

int _N_bh = 0;
int _Size_bh = 0;

/* maxpacket and other transfer characteristics vary by speed. */
#define ep_desc(g,hs,fs) (((g)->speed==USB_SPEED_HIGH)?(hs):(fs))
int usbd_msc_lun_read(struct msc_lun *curlun, u32 sector,u8 *buffer,u32 count);
int usbd_msc_lun_write(struct msc_lun *curlun, u32 sector, u8 *buffer,u32 count);
#if (defined(CONFIG_PLATFORM_8195BHP) || defined (CONFIG_PLATFORM_8721D))
#define CONTAINER_OF container_of
#endif 


static u32 min(
    IN  u32 value1,
    IN  u32 value2
)
{
    u32 retval = 0;
    retval = ((value1>=value2) ? value2:value1);
    return retval;
}

int usbd_msc_halt_bulk_in_endpoint(struct msc_dev *mscdev)
{
	int	rc;

	rc = usb_ep_set_halt(mscdev->in_ep);
	if (rc == -EAGAIN)
		USBD_ERROR("delayed bulk-in endpoint halt\n");
	while (rc != 0) {
		if (rc != -EAGAIN) {
			USBD_WARN("usb_ep_set_halt -> %d\n", rc);
			rc = 0;
			break;
		}

		/* Wait for a short time and then try again */
		rtw_mdelay_os(10);
		rc = usb_ep_set_halt(mscdev->in_ep);;
	}
	return rc;
}

int usbd_msc_bulk_in_transfer(struct msc_dev *mscdev, struct usb_request *req){
	int result;
	
	req->dma = (dma_addr_t)req->buf;

	result = usb_ep_queue(mscdev->in_ep, req, 1);
	
	if (result != 0){
		USBD_ERROR("usbd_msc_bulk_in_transfer fail(%d)\n", result);	
		return result;
	}
	return 0;
}

int usbd_msc_bulk_out_transfer(struct msc_dev *mscdev, struct usb_request *req){
	int result;

	req->dma = (dma_addr_t)req->buf;
	
	result = usb_ep_queue(mscdev->out_ep, req, 1);
	
	if (result != 0){
		USBD_ERROR("usbd_msc_bulk_out_transfer fail(%d)\n", result);	
		return result;
	}
	return 0;
}


void usbd_msc_bulk_in_complete (struct usb_ep *ep, struct usb_request *req){
	struct msc_bufhd* bufhd = (struct msc_bufhd*)req->context;
	struct msc_dev* mscdev = (struct msc_dev*)ep->driver_data;
	struct msc_common* common = mscdev->common;
	int status = req->status;

	/*check request status*/
	switch (status) {
		case 0: 			/* tx seccussfuly*/
			break;
		/* FALLTHROUGH */
		case -ECONNRESET:		/* unlink */
			USBD_ERROR("ECONNRESET status = %d\n", status);
		case -ESHUTDOWN:		/* disconnect etc */
			USBD_ERROR("ESHUTDOWN status = %d\n", status);
			break;
		default:
			USBD_ERROR("bulk_in_complete ERROR(%d)\n", status);
	}

	if(bufhd->type == BUFHD_DATA){
		//USBD_ERROR("bulk_in_complete free bufhd (0X%X) %d\n" ,bufhd, common->nbufhd_a);
		usbd_msc_put_bufhd(common, bufhd);
	}		
}

#if (CONGIF_USE_DATA_THREAD==1)
thread_return usbd_msc_bulk_data_handler(thread_context context){
	struct msc_dev *mscdev = (struct msc_dev *)context;
	struct msc_common* common = mscdev->common;
	struct msc_bufhd* bufhd = NULL;
	struct usb_request* req = NULL;
	struct msc_lun	*curlun;
	int result;
	int blkNbr;
	int lba;

#if defined (CONFIG_PLATFORM_8721D)
	//rtw_create_task in Ameba1 has semaphore and initial inside, but AmebaD didn't have it, so we create semaphore by ourselves
	rtw_init_sema(&mscdev->msc_outDataTask.wakeup_sema, 0);  
	rtw_init_sema(&mscdev->msc_outDataTask.terminate_sema, 0); 
#endif
	
	while(1){
		result = rtw_down_sema(&(mscdev->msc_outDataTask.wakeup_sema));
		if (_FAIL == result) {
			USBD_ERROR("rtw down sema fail!\n");
			break;
		}

		if(mscdev->msc_outDataTask.blocked)
			break;
		
		// handle bulk out request
		rtw_mutex_get(&common->bod_mutex);
		result = rtw_is_list_empty(&common->bod_list);
		rtw_mutex_put(&common->bod_mutex);
		if(result == _TRUE)
			continue;
next:
		rtw_mutex_get(&common->bod_mutex);
		bufhd = list_first_entry(&common->bod_list, struct msc_bufhd, list);
		rtw_mutex_put(&common->bod_mutex);

		req = bufhd->reqout;
		curlun = common->curlun;
		blkNbr = req->actual >> curlun->blkbits;
		lba = curlun->lba;
		
		if(blkNbr >= 1){
			if(usbd_msc_lun_write(common->curlun, lba, bufhd->buf, blkNbr)){
			USBD_ERROR("error in file write:sector(%d),counts(%d)\n", lba, blkNbr);
			goto next;
		}
			//USBD_ERROR("write %d: sector %d(%d)", usb_to_sdio_cnt, lba, blkNbr);
		}else{
			USBD_ERROR("invalid parameter:sector(%d),counts(%d)\n", lba, blkNbr);
			USBD_ERROR("req:actural(%d),bits(%d)\n", req->actual, curlun->blkbits);
		}
		
		curlun->lba += blkNbr;
		common->residue -= req->actual;

		rtw_mutex_get(&common->bod_mutex);
		rtw_list_delete(&bufhd->list);
		result = rtw_is_list_empty(&common->bod_list);
		rtw_mutex_put(&common->bod_mutex);

		usbd_msc_put_bufhd(common, bufhd);

		if(result == _FALSE)
			goto next;
	}
	rtw_up_sema(&(mscdev->msc_outDataTask.terminate_sema));
	rtw_thread_exit();
}
#endif

thread_return usbd_msc_bulk_cmd_handler(thread_context context){
	struct msc_dev *mscdev = (struct msc_dev *)context;
	struct msc_common* common = mscdev->common;
	struct msc_bufhd* bufhd = NULL;
	struct usb_request* req = NULL;
	int result;
#if (defined(CONFIG_PLATFORM_8195BHP) || defined (CONFIG_PLATFORM_8721D))
	//rtw_create_task in Ameba1 has semaphore and initial inside, but AmebaPro(AmebaD) didn't have it, so we create semaphore by ourselves
	rtw_init_sema(&mscdev->msc_outCmdTask.wakeup_sema, 0);  
	rtw_init_sema(&mscdev->msc_outCmdTask.terminate_sema, 0); 
#endif
	while(1){
		result = rtw_down_sema(&(mscdev->msc_outCmdTask.wakeup_sema));
		if (_FAIL == result) {
			USBD_ERROR("rtw down sema fail!\n");
			break;
		}
		
		if(mscdev->msc_outCmdTask.blocked)
			break;

		// handle bulk out request
		rtw_mutex_get(&common->boc_mutex);
		result = rtw_is_list_empty(&common->boc_list);
		rtw_mutex_put(&common->boc_mutex);
		if(result == _TRUE)
			continue;
next:
		rtw_mutex_get(&common->boc_mutex);
		bufhd = list_first_entry(&common->boc_list, struct msc_bufhd, list);
		rtw_mutex_put(&common->boc_mutex);

		req = bufhd->reqout;
#if defined(CONFIG_PLATFORM_8721D)
		DCache_Invalidate(((u32)(req->buf) & CACHE_LINE_ADDR_MSK), (req->actual+ CACHE_LINE_SIZE));
#endif
		// receive CBW
		if(bufhd->type== BUFHD_CBW){
			result = usbd_msc_receive_cbw(mscdev, req);
		}
		
		if(result == 0) {//response successfully
			rtw_list_delete(&bufhd->list);
		}
		rtw_mutex_get(&common->boc_mutex);
		result = rtw_is_list_empty(&common->boc_list);
		rtw_mutex_put(&common->boc_mutex);

		if(result == _FALSE)
			goto next;
		/* submit bulk out request agains*/
		else{
			if(bufhd->type== BUFHD_CBW){
				req->length = US_BULK_CB_WRAP_LEN;
				result = usb_ep_queue(mscdev->out_ep, req, 1);
				if (result)
					USBD_ERROR("%s queue req --> %d\n",mscdev->out_ep->name, result);
			}
		}
	}
	rtw_up_sema(&(mscdev->msc_outCmdTask.terminate_sema));
	USBD_ERROR("TASK DELETE\n");
	rtw_thread_exit();
}

void usbd_msc_bulk_out_complete (struct usb_ep *ep, struct usb_request *req){
    struct msc_dev *mscdev = ep->driver_data;  
	struct msc_common* common = mscdev->common;
	struct msc_bufhd* bufhd = (struct msc_bufhd*)req->context;

	int status = req->status;

//	USBD_PRINTF("usbd_msc_bulk_out_complete acture = %d\n", req->actual);
	/*check request status*/
	switch(status){
		/* normal complete */
		case 0:	
			if(req->actual > 0) {
				if(bufhd->type== BUFHD_CBW && req->actual == US_BULK_CB_WRAP_LEN){
					rtw_mutex_get(&common->boc_mutex);
					rtw_list_insert_tail(&bufhd->list, &common->boc_list);
					rtw_mutex_put(&common->boc_mutex);
					/*release sema to wake up bulk out handler*/
					rtw_up_sema(&(mscdev->msc_outCmdTask.wakeup_sema));
				}else if(bufhd->type == BUFHD_DATA && req->actual>=common->curlun->blksize){
					rtw_mutex_get(&common->bod_mutex);
					rtw_list_insert_tail(&bufhd->list, &common->bod_list);
					rtw_mutex_put(&common->bod_mutex);
#if (CONGIF_USE_DATA_THREAD==1)
					/*release sema to wake up bulk out handler*/
					rtw_up_sema(&(mscdev->msc_outDataTask.wakeup_sema));
#endif
				}else{
					USBD_ERROR("Invalid package received (%d)\n",req->actual);
					if(req->actual == US_BULK_CB_WRAP_LEN){
						USBD_ERROR("CBW received\n");
						struct msc_bufhd* bhd = common->cbw_bh;
						memcpy(bhd->reqout->buf, req->buf, req->actual);
						bhd->reqout->actual = req->actual;
						rtw_mutex_get(&common->boc_mutex);
						rtw_list_insert_tail(&bhd->list, &common->boc_list);
						rtw_mutex_put(&common->boc_mutex);
						/*release sema to wake up bulk out handler*/
						rtw_up_sema(&(mscdev->msc_outCmdTask.wakeup_sema));
						if(bufhd->type == BUFHD_DATA){
							usbd_msc_put_bufhd(common, bufhd);
							USBD_ERROR("Put data buffer back to pool\n");
						}
					}
				}
			}
			break;
		/* software-driven interface shutdown */
		case -ECONNRESET:		/* unlink */
		case -ESHUTDOWN:		/* disconnect etc */
			USBD_ERROR("tx endpoint shutdown, code %d\n", status);
			break;

		/* for hardware automagic (such as pxa) */
		case -ECONNABORTED: 	/* endpoint reset */
			USBD_ERROR("tx %s reset\n", ep->name);
			break;

		/* data overrun */
		case -EOVERFLOW:
			/* FALLTHROUGH */

		default:
			USBD_ERROR("tx status %d\n", status);
			break;
	}
	return;
}

int usbd_msc_lun_write(struct msc_lun *curlun, u32 sector, u8 *buffer,u32 count){
	int ret = 0;
	if(curlun == NULL)
		return -1;

	rtw_mutex_get(&curlun->lun_mutex);
	if(curlun->lun_opts->disk_write){
		ret = curlun->lun_opts->disk_write(sector, buffer, count);
	}
	rtw_mutex_put(&curlun->lun_mutex);

	return ret;
}

int usbd_msc_lun_read(struct msc_lun *curlun, u32 sector,u8 *buffer,u32 count){
	int ret = 0;
	if(curlun == NULL)
		return -1;

	rtw_mutex_get(&curlun->lun_mutex);
	if(curlun->lun_opts->disk_read){
		ret = curlun->lun_opts->disk_read(sector, buffer, count);
	}
	rtw_mutex_put(&curlun->lun_mutex);

	return ret;
}


void usbd_msc_lun_close(struct msc_lun *curlun){
	if(curlun->is_open){
		if(curlun->lun_opts->disk_deinit)
			curlun->lun_opts->disk_deinit();
		curlun->is_open = 0;
	}
}

int usbd_msc_lun_open(struct msc_lun *curlun){
	int		ro = 0;
	int		rc = -EINVAL;
	u64		size;
	u32		num_sectors = 0;
	u32		blkbits;
	u32		blksize;

	/* init hard disk */
	if(curlun->lun_opts->disk_init){
		if(curlun->lun_opts->disk_init()){
			rc = -ENOMEDIUM;
			goto err;
		}
		curlun->is_open = 1;
	}else{
		rc = -ENOMEDIUM;
		goto err;
	}

	/* get disk capacity */
	if(curlun->lun_opts->disk_getcapacity)
		curlun->lun_opts->disk_getcapacity(&num_sectors);
	
	blksize = 512;
	blkbits = 9;

	size = (u64)num_sectors<<blkbits;

	curlun->blksize = blksize;
	curlun->blkbits = blkbits;
	curlun->ro = ro;
	curlun->file_length = size;
	curlun->num_sectors = num_sectors;

	return 0;
err:
	return rc;
}

int usbd_msc_create_lun(struct msc_common *common, unsigned int id){
	struct msc_lun *lun;
	int status;
	
	lun = (struct msc_lun *)rtw_zmalloc(sizeof(struct msc_lun));
	if (!lun)
		return -ENOMEM;

	lun->cdrom = 0;
	lun->ro = 0;
	lun->initially_ro = lun->ro;
	lun->removable = 1; // set device removeable

	common->luns[id] = lun;

	lun->lun_opts = &disk_operation;
	
	rtw_mutex_init(&lun->lun_mutex);
	status = usbd_msc_lun_open(lun);
	if(status)
		goto err;

	return 0;
err:
	return status;
}

void usbd_msc_remove_luns(struct msc_common *common, unsigned int n){
	int i;

	for (i = 0; i < n; ++i){
		if (common->luns[i]) {
			usbd_msc_lun_close(common->luns[i]);
			rtw_mfree((u8*)common->luns[i], sizeof(struct msc_lun));
			common->luns[i] = NULL;
		}
	}
	
	if(common->luns)
		rtw_mfree((u8*)common->luns, common->nluns*sizeof(struct msc_lun*));
}

int usbd_msc_create_luns(struct msc_common *common){
	struct msc_lun ** luns;
	int i;
	int status;
	
	luns = (struct msc_lun **)rtw_zmalloc(common->nluns*sizeof(struct msc_lun*));
	if(!luns){
		USBD_ERROR("Malloc luns fail\n");
		return -ENOMEM;
	}

	common->luns = luns;
	
	for (i = 0; i < common->nluns; ++i) {
		status = usbd_msc_create_lun(common, i);
		if(status)
			goto err;
	}

	return 0;
err:
	usbd_msc_remove_luns(common, i);
	return status;
}

void usbd_msc_put_bufhd(struct msc_common *common, struct msc_bufhd* bufhd){
	rtw_mutex_get(&common->bufhd_mutex);
	rtw_list_insert_tail(&bufhd->list, &common->bufhd_pool);
	common->nbufhd_a++;
	rtw_mutex_put(&common->bufhd_mutex);
}

struct msc_bufhd* usbd_msc_get_bufhd(struct msc_common *common){
	struct msc_bufhd *bufhd = NULL;

	rtw_mutex_get(&common->bufhd_mutex);
	while(rtw_is_list_empty(&common->bufhd_pool)==_TRUE){
		rtw_mutex_put(&common->bufhd_mutex);
		return NULL;
	}
	bufhd = list_first_entry(&common->bufhd_pool, struct msc_bufhd, list);
	rtw_list_delete(&bufhd->list);
	common->nbufhd_a--;
	rtw_mutex_put(&common->bufhd_mutex);
	
	return bufhd; 
}

struct msc_bufhd *usbd_msc_alloc_bufhd(struct msc_common *common,int bufsize, bufhd_type type){
	struct msc_bufhd *bufhd = NULL;

	bufhd = (struct msc_bufhd *)rtw_zmalloc(sizeof(struct msc_bufhd));
	if(bufhd){
#if defined(CONFIG_PLATFORM_8721D)
		if(type != BUFHD_DATA) {
			bufhd->buf = rtw_zmalloc(bufsize);
			if(bufhd->buf == NULL){
				rtw_mfree((void*)bufhd, sizeof(struct msc_bufhd));
				USBD_ERROR("malloc bufhd->buf fail, size %d\n", bufsize);
				goto fail;
			}
		} else {
		//for disk_write/read, buf addr should be 32byte align to increase write/read speed
			bufhd->prebuf= rtw_zmalloc(bufsize+0x1F);
			if(bufhd->prebuf == NULL){
				rtw_mfree((void*)bufhd, sizeof(struct msc_bufhd));
				USBD_ERROR("malloc bufhd->prebuf fail, size %d\n", bufsize);
				goto fail;
			}
			bufhd->buf = (u8*)_RND((u32)bufhd->prebuf, 32);//buf 32 byte align
		}
#else
		bufhd->buf = rtw_zmalloc(bufsize);
		if(bufhd->buf == NULL){
			rtw_mfree((void*)bufhd, sizeof(struct msc_bufhd));
			USBD_ERROR("malloc bufhd->buf fail, size %d\n", bufsize);
			goto fail;
		}
#endif
		bufhd->buf_size = bufsize;
	}
	
	bufhd->type = type;
	return bufhd;
fail:
	return NULL;
}

void usbd_msc_free_bufhds(struct msc_common *common){
	struct msc_bufhd *bufhd = NULL;
	/* free bufhd for cwb*/
	if(common->cbw_bh){
		bufhd = common->cbw_bh;
		if(bufhd->reqout){
			usb_ep_free_request(common->mscdev->out_ep, bufhd->reqout);
			bufhd->reqout = NULL;
		}

		rtw_mfree(bufhd->buf, bufhd->buf_size);
		rtw_mfree((void*)bufhd, sizeof(struct msc_bufhd));
	}
	/* free bufhd for csb*/
	if(common->csw_bh){
		bufhd = common->csw_bh;
		if(bufhd->reqin){
			usb_ep_free_request(common->mscdev->in_ep, bufhd->reqin);
			bufhd->reqin = NULL;
		}

		rtw_mfree(bufhd->buf, bufhd->buf_size);
		rtw_mfree((void*)bufhd, sizeof(struct msc_bufhd));
	}
	/* free bufhd for data*/
	while(!rtw_is_list_empty(&common->bufhd_pool)){
		rtw_mutex_get(&common->bufhd_mutex);
		bufhd = list_first_entry(&common->bufhd_pool, struct msc_bufhd, list);
		rtw_list_delete(&bufhd->list);
		rtw_mutex_put(&common->bufhd_mutex);
		
		if(bufhd->reqin){
			usb_ep_free_request(common->mscdev->in_ep, bufhd->reqin);
			bufhd->reqin = NULL;
		}

		if(bufhd->reqout){
			usb_ep_free_request(common->mscdev->out_ep, bufhd->reqout);
			bufhd->reqout = NULL;
		}
#if defined(CONFIG_PLATFORM_8721D)
		rtw_mfree(bufhd->prebuf, bufhd->buf_size+0x1F);
#else
		rtw_mfree(bufhd->buf, bufhd->buf_size);
#endif
		rtw_mfree((void*)bufhd, sizeof(struct msc_bufhd));
		common->nbufhd_a--;
	}
}

int usbd_msc_alloc_bufhds(struct msc_common *common){
	struct msc_bufhd *bufhd = NULL;
	int i,rc;

	/* malloc bufhd for cwb*/
	bufhd = usbd_msc_alloc_bufhd(common, US_BULK_CB_WRAP_LEN, BUFHD_CBW);
	if(bufhd){
		common->cbw_bh = bufhd;
	}else{
		USBD_ERROR("malloc bufhd cbw fail\n");
		rc = -ENOMEM;
		goto fail;
	}
	/* malloc bufhd for csb*/
	bufhd = usbd_msc_alloc_bufhd(common, US_BULK_CS_WRAP_LEN, BUFHD_CSW);
	if(bufhd){
		common->csw_bh = bufhd;
	}else{
		USBD_ERROR("malloc bufhd csw fail\n");
		rc = -ENOMEM;
		goto fail;
	}
	
	/* malloc bufhd for bulk data stage*/
	for (i = 0; i < common->nbufhd; i++) {
		bufhd = usbd_msc_alloc_bufhd(common, _Size_bh, BUFHD_DATA);
		if(bufhd){
			/* add bufhd to list*/
			rtw_init_listhead(&bufhd->list);
			rtw_list_insert_tail(&bufhd->list, &common->bufhd_pool);
		}else{
			USBD_ERROR("malloc bufhd(No.%d)fail\n", i+1);
			rc = -ENOMEM;
			goto fail;
		}
		//USBD_PRINTF("malloc bufhd(No.%d) 0x%X\n", i+1, bufhd);
	}
	
	common->nbufhd_a = common->nbufhd;
	return 0;
fail:
	usbd_msc_free_bufhds(common);
	return rc;
}

static int fun_set_alt(struct usb_function *f, unsigned interface, unsigned alt){
	struct msc_dev	*mscdev = CONTAINER_OF(f,struct msc_dev, func);
	struct msc_common *common = mscdev->common;
	struct msc_bufhd *bufhd;
	struct usb_request	*req;
	int status = 0;

FUN_ENTER;
	USBD_PRINTF("config #%d set_alt interface #%d \n",f->config->bConfigurationValue, interface);

	if(common->running){
		USBD_PRINTF("Reset interface\n");
		/* deallocate the request */
		list_for_each_entry(bufhd, &common->bufhd_pool, list, struct msc_bufhd) {
			if(bufhd->reqin){
				usb_ep_free_request(mscdev->in_ep, bufhd->reqin);
				bufhd->reqin = NULL;
			}
			if(bufhd->reqout){
				usb_ep_free_request(mscdev->out_ep, bufhd->reqout);
				bufhd->reqout = NULL;
			}
		}
		/* Disable the endpoints */
		if (mscdev->bulk_in_enabled) {
			usb_ep_disable(mscdev->in_ep);
			mscdev->in_ep->driver_data = NULL;
			mscdev->bulk_in_enabled = 0;
		}
		if (mscdev->bulk_out_enabled) {
			usb_ep_disable(mscdev->out_ep);
			mscdev->out_ep->driver_data = NULL;
			mscdev->bulk_out_enabled = 0;
		}

		common->mscdev = NULL;
		common->running = 0;
	}
	
	common->mscdev = mscdev;

//1 enable bulk in endpoint
	if (mscdev->in_ep != NULL) {
		/* restart endpoint */
		if (mscdev->in_ep->driver_data != NULL)
			usb_ep_disable(mscdev->in_ep);

		/*
		 * assign an corresponding endpoint descriptor to an endpoint
		 * according to the gadget speed
		 */
		mscdev->in_ep->desc = ep_desc (common->gadget, &usbd_msc_source_desc_HS, &usbd_msc_source_desc_FS);
		status = usb_ep_enable(mscdev->in_ep, mscdev->in_ep->desc);
		if (status < 0) {
			USBD_ERROR("Enable IN endpoint FAILED!\n");
			goto fail;
		}
		mscdev->in_ep->driver_data = mscdev;
		mscdev->bulk_in_enabled = 1;
		/*malloc request for CSW*/
		req = usb_ep_alloc_request(mscdev->in_ep, 1);
		if (!req){
			USBD_ERROR("malloc bufhd->reqin fail\n");
			goto fail;
		}
		bufhd = common->csw_bh;
		bufhd->reqin = req;
		bufhd->type = BUFHD_CSW;
		req->buf = bufhd->buf;
		req->dma = (dma_addr_t)bufhd->buf;
		req->context = bufhd;
		req->complete = usbd_msc_bulk_in_complete;
	}
	
//2 enable bulk out endpoint and alloc requset to recieve data from host
	if (mscdev->out_ep != NULL) {
		/* restart endpoint */
		if (mscdev->out_ep->driver_data != NULL)
			usb_ep_disable(mscdev->out_ep);
		/*
		* assign an corresponding endpoint descriptor to an endpoint
		* according to the gadget speed
		*/
		mscdev->out_ep->desc = ep_desc (common->gadget, &usbd_msc_sink_desc_HS, &usbd_msc_sink_desc_FS);
		status = usb_ep_enable(mscdev->out_ep, mscdev->out_ep->desc);
		if (status < 0) {
			USBD_ERROR("Enable out endpoint FAILED!\n");
			goto fail;
		}
		mscdev->out_ep->driver_data = mscdev;
		mscdev->bulk_out_enabled = 1;
		/*malloc request for CBW*/
		req = usb_ep_alloc_request(mscdev->out_ep, 1);
		if (!req){
			USBD_ERROR("malloc bufhd->reqin fail\n");
			goto fail;
		}
		bufhd = common->cbw_bh;
		bufhd->reqout = req;
		bufhd->type = BUFHD_CBW;
		req->buf = bufhd->buf;
		req->dma = (dma_addr_t)bufhd->buf;
		req->context = bufhd;
		req->complete = usbd_msc_bulk_out_complete;
	}

	/* malloc request for data*/
	list_for_each_entry(bufhd, &common->bufhd_pool, list, struct msc_bufhd) {
		bufhd->reqin = usb_ep_alloc_request(mscdev->in_ep, 1);
		if (!bufhd->reqin){
			USBD_ERROR("malloc bufhd->reqin fail\n");
			goto fail;
		}

		bufhd->reqout = usb_ep_alloc_request(mscdev->out_ep, 1);
		if (!bufhd->reqout){
			USBD_ERROR("malloc bufhd->reqout fail\n");
			goto fail;
		}
		bufhd->reqin->buf = bufhd->reqout->buf = bufhd->buf;
		bufhd->reqin->dma= bufhd->reqout->dma= (dma_addr_t)bufhd->buf;
		bufhd->reqin->context= bufhd->reqout->context= bufhd;
		bufhd->reqin->complete = usbd_msc_bulk_in_complete;
		bufhd->reqout->complete = usbd_msc_bulk_out_complete;
		bufhd->type = BUFHD_DATA;
		//USBD_PRINTF("malloced bufhd 0x%X (%d)\n", bufhd, common->nbufhd_a);
	}

	/* enqueue CBW request to receive SCSI command from host*/
	req = common->cbw_bh->reqout;
	req->length = US_BULK_CB_WRAP_LEN;
	status = usb_ep_queue(mscdev->out_ep, req, 1);
	if(status){
		USBD_ERROR("enqueue out endpoint FAILED!\n");
		goto fail;
	}

	common->running = 1;
FUN_EXIT;
	return 0;
FUN_EXIT;	
fail:
	if(status){
	// free allocated resource here
	// todo
	}
	return status;
}

void fun_disable (struct usb_function *f){
	struct msc_dev	*mscdev = CONTAINER_OF(f,struct msc_dev, func);
	struct msc_common *common = mscdev->common;
FUN_ENTER;
	if(common){
	// TODO:
	}
FUN_EXIT;
}

static int fun_bind(struct usb_configuration *c, struct usb_function *f){
	struct msc_dev	*mscdev = CONTAINER_OF(f,struct msc_dev, func);
	struct usb_ep *ep;

	int status = -ENODEV;
	int id;

FUN_ENTER;

	if(rtw_create_task(&mscdev->msc_outCmdTask, (const char *)"MSC_BULK_CMD",
		512, MSC_BULK_OUT_CMD_THREAD_PRIORITY, usbd_msc_bulk_cmd_handler, (void *)mscdev)!= 1){
		USBD_ERROR("Create thread MSC_BULK_CMD fail\n");
		goto fail;
	}

#if(CONGIF_USE_DATA_THREAD == 1)
	if(rtw_create_task(&mscdev->msc_outDataTask, (const char *)"MSC_BULK_DATA",
		512, MSC_BULK_DATA_THREAD_PRIORITY, usbd_msc_bulk_data_handler, (void *)mscdev)!= 1){
		USBD_ERROR("Create thread MSC_BULK_DATA fail\n");
		goto fail;
	}
#endif

	/* allocate instance-specific interface IDs, and patch descriptors */
	id = usb_interface_id(c, f); // this will return the allocated interface id 
	if (id < 0){
		status = id;
		goto fail;
	}
	usbd_msc_intf_desc.bInterfaceNumber = id;
	mscdev->interface_number = id; // record interface number

	/* search endpoint according to endpoint descriptor and modify endpoint address*/
	ep = usb_ep_autoconfig(c->cdev->gadget, &usbd_msc_source_desc_FS);
	if (!ep)
		goto fail;
	ep->driver_data = mscdev;/* claim */
	mscdev->in_ep = ep;

	ep = usb_ep_autoconfig(c->cdev->gadget, &usbd_msc_sink_desc_FS);
	if (!ep)
		goto fail;
	ep->driver_data = mscdev;/* claim */
	mscdev->out_ep = ep;

	usbd_msc_source_desc_HS.bEndpointAddress =
			usbd_msc_source_desc_FS.bEndpointAddress;// modified after calling usb_ep_autoconfig()
	usbd_msc_sink_desc_HS.bEndpointAddress =
			usbd_msc_sink_desc_FS.bEndpointAddress;

	status = usb_assign_descriptors(f, usbd_msc_descriptors_FS,
			usbd_msc_descriptors_HS, NULL);
	if (status)
		goto fail;
FUN_EXIT;
	return 0;
FUN_EXIT;
fail:
	usb_free_all_descriptors(f);
	if(mscdev->msc_outCmdTask.task)
		rtw_delete_task(&mscdev->msc_outCmdTask);
#if(CONGIF_USE_DATA_THREAD == 1)
	if(mscdev->msc_outDataTask.task)
		rtw_delete_task(&mscdev->msc_outDataTask);
#endif
	return status;
}

void fun_unbind_msc(struct usb_configuration *c, struct usb_function *f){
	struct msc_dev *mscdev = CONTAINER_OF(f,struct msc_dev, func);
FUN_ENTER;
	usb_free_all_descriptors(f);
		/* free task */
	if(mscdev->msc_outCmdTask.task)
		rtw_delete_task(&mscdev->msc_outCmdTask);
#if(CONGIF_USE_DATA_THREAD == 1)
	if(mscdev->msc_outDataTask.task)
		rtw_delete_task(&mscdev->msc_outDataTask);
#endif 

FUN_EXIT;
}


int fun_setup_msc(struct usb_function *fun, const struct usb_ctrlrequest *ctrl){
	struct msc_dev *mscdev = CONTAINER_OF(fun,struct msc_dev, func);
	struct usb_ep *ep0 = mscdev->common->ep0;
	struct usb_request	*req0 = mscdev->common->req0;
FUN_ENTER;
	u16 wLength = ctrl->wLength;
	u16 wIndex = ctrl->wIndex;
	u16	wValue = ctrl->wValue;

	int status;

	req0->context = NULL;
	req0->length = 0;
	
	switch(ctrl->bRequest){
		case MSC_REQUEST_RESET:
			USBD_PRINTF("ep0-interface-setup: MSC_REQUEST_RESET\n");
			if (ctrl->bRequestType !=
		    (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE))
			break;
			if (wIndex != mscdev->interface_number || wValue != 0 ||
				wLength != 0)
			return -EDOM;
			/* stop the current transction */
			if (!mscdev->common->running)
				break;
			/*
			if (test_and_clear_bit(IGNORE_BULK_OUT,
					       &common->fsg->atomic_bitflags))
				usb_ep_clear_halt(common->fsg->bulk_in);

			if (common->ep0_req_tag == exception_req_tag)
				ep0_queue(common);*/	/* Complete the status stage */

			return USB_GADGET_DELAYED_STATUS;
			break;
        case MSC_REQUEST_GET_MAX_LUN:
			if (ctrl->bRequestType !=
		    (USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE))
				break;

			if (wIndex != mscdev->interface_number || wValue != 0 ||
				wLength != 1)
			return -EDOM;

			*(u8 *)req0->buf = mscdev->common->nluns - 1;

			req0->length = min((u16)1, wLength);
			status = usb_ep_queue (ep0, req0, 1);
			
			if (status != 0 && status != -ESHUTDOWN) {
				/* We can't do much more than wait for a reset */
				USBD_WARN("error in submission: %s --> %d\n",
					ep0->name, status);
			}
			return status;
			break;
		default:
			USBD_PRINTF("ep0-interface-setup: UNKNOWN\n");
	}
FUN_EXIT;
	return -EOPNOTSUPP;
}

void fun_free(struct usb_function *f){
	//
}

__weak
void fun_suspend(struct usb_function *f){
	//printf("fun_suspend\n\r");
}

static int config_bind(struct usb_configuration *c){
	struct msc_dev *mscdev;
	struct msc_common *common;
	int status = 0;
FUN_ENTER;

	mscdev =(struct msc_dev *)c->cdev->gadget->device;
	common = mscdev->common;

	common->gadget = c->cdev->gadget;
	
	mscdev->func.name = "msc_function_sets";
	mscdev->func.bind = fun_bind;
	mscdev->func.unbind = fun_unbind_msc;
	mscdev->func.setup = fun_setup_msc;
	mscdev->func.set_alt = fun_set_alt;
	mscdev->func.disable = fun_disable;
	mscdev->func.suspend = fun_suspend;
	mscdev->func.free_func = fun_free;
	
	status = usb_add_function(c, &mscdev->func);
	if(status)
		goto free_fun;

FUN_EXIT;
	return status;

free_fun:
FUN_EXIT;
	usb_put_function(&mscdev->func);
	return status;
}

void config_unbind_msc(struct usb_configuration *c){
//	struct msc_dev *mscdev;
//	struct usb_request* req;
FUN_ENTER;
	// TODO:
FUN_EXIT;
}


static struct usb_configuration usbd_msc_cfg_driver = {
	.label			= "usbd-msc",
	.unbind			= config_unbind_msc,
	.bConfigurationValue	= DEV_CONFIG_VALUE,
	.bmAttributes		= USB_CONFIG_ATT_ONE,// | USB_CONFIG_ATT_SELFPOWER,
};

int usbd_msc_bind(
    struct usb_composite_dev *cdev
)
{
	int status = 0;
	struct msc_dev *mscdev;
	struct msc_common *common;
	
FUN_ENTER;

	mscdev = &gMSC_DEVICE;
	common = &gMSC_COMMON;

	cdev->gadget->device = mscdev;

	/* link msc_dev and msc_common */
	mscdev->common = common;

	common->gadget = cdev->gadget;
	common->ep0 = cdev->gadget->ep0;
	common->req0 = cdev->req;
	common->nluns = MSC_MAX_LOGIC_UNIT_NUMBER;
	common->nbufhd = _N_bh;
	common->can_stall = 0;
	
	rtw_init_listhead(&common->bufhd_pool);
	rtw_mutex_init(&common->bufhd_mutex);

	rtw_init_listhead(&common->boc_list);
	rtw_mutex_init(&common->boc_mutex);

	rtw_init_listhead(&common->bod_list);
	rtw_mutex_init(&common->bod_mutex);

	/* alloc buffer header */
	status = usbd_msc_alloc_bufhds(common);
	if(status){
		USBD_ERROR("Malloc buffer header fail\n");
		goto fail_alloc_buf;
	}

	/* create logic unit */
	status = usbd_msc_create_luns(common);
	if(status < 0)
		goto fail_alloc_buf;

    /* register configuration(s)*/
	status = usb_add_config(cdev, &usbd_msc_cfg_driver,
			config_bind);
	if (status < 0)
		goto fail_set_luns;

FUN_EXIT;
	return 0;

fail_set_luns:
	usbd_msc_remove_luns(common, common->nluns);
	
fail_alloc_buf:
	usbd_msc_free_bufhds(mscdev->common);
	rtw_mutex_free(&common->bod_mutex);
	rtw_mutex_free(&common->boc_mutex);
	rtw_mutex_free(&common->bufhd_mutex);
FUN_EXIT;

    return status;
}

int usbd_msc_unbind(
    struct usb_composite_dev *cdev
)
{
	struct msc_dev *mscdev = (struct msc_dev*)cdev->gadget->device;
	struct msc_common *common;
FUN_ENTER;

	common = mscdev->common;

	common->running = 0; // indicate driver stop

	usb_put_function(&mscdev->func);

	usbd_msc_remove_luns(common, common->nluns);

	usbd_msc_free_bufhds(common);

	rtw_mutex_free(&common->bod_mutex);
	rtw_mutex_free(&common->boc_mutex);
	rtw_mutex_free(&common->bufhd_mutex);
FUN_EXIT;
	return 0;
}

void usbd_msc_disconnect(struct usb_composite_dev *cdev){
	USBD_PRINTF("usbd_msc_disconnect\n");
}

__weak
void usbd_msc_suspend(struct usb_composite_dev *cdev){
	//printf("usbd_msc_suspend\n");
}

struct usb_composite_driver     usbd_msc_driver = {
	.name 	    = "usbd_msc_driver",
	.dev		= &usbd_msc_device_desc,
	.strings 	= dev_msc_strings,
	.bind       = usbd_msc_bind,
	.unbind     = usbd_msc_unbind,
	.disconnect = usbd_msc_disconnect,
	.suspend    = usbd_msc_suspend,
	.max_speed  = USB_SPEED_HIGH,
};


int usbd_msc_init(int N_bh, int Size_bh, disk_type type){
	int status = 0;
	_N_bh = N_bh;
	_Size_bh = Size_bh;

	switch(type){
		case DISK_SDCARD:
			disk_operation.disk_init = SD_Init;
			disk_operation.disk_deinit = SD_DeInit;
			disk_operation.disk_getcapacity = SD_GetCapacity;
			disk_operation.disk_read = SD_ReadBlocks;
			disk_operation.disk_write = SD_WriteBlocks;
			break;
		case DISK_FLASH:
			USBD_ERROR("DISK Type of Flash is not supported now\n");
			status = -1;
			break;
		default:
			USBD_ERROR("Unknown DISK type\n");
			status = -1;
			break;
	}
	if(status != 0)
		return status;
	return usb_composite_probe(&usbd_msc_driver);
}


void usbd_msc_deinit(void){
	usb_composite_unregister(&usbd_msc_driver);
}

