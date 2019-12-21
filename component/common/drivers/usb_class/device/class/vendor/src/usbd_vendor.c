#include "vendor/inc/usbd_vendor_desc.h"
#include "vendor/inc/usbd_vendor.h"
#include "core/inc/usb_composite.h"
#include "osdep_service.h"
#if defined (CONFIG_PLATFORM_8721D)
#define CONTAINER_OF container_of
#define RtlZmalloc rtw_zmalloc
#endif 

#if 1
#include "isoout_test.h"  //defined USB_ISOOUT_TRANSFER_CNT
#include <timers.h>
#include <gpio_api.h>
#define GPIO_DGB_USBOUTCALLBACK_PIN			PB_3
#define GPIO_DGB_USBINCALLBACK_PIN			PB_4
static gpio_t gDbgGpioInTransaction_;
static gpio_t gDbgGpioOutTransaction_;
static int isoout_complete_conter_ = 0;
typedef struct _IsoOutRecvStatus{
	uint8_t pos;
	uint8_t data;
}IsoOutRecvStatus;
static IsoOutRecvStatus *isoOutRecvStatus_;
static xTimerHandle handleIsoOutTimeout;
static void IsoOutTimerCallback(xTimerHandle *pxTimer);
static void OutputIsoOutRecvResult(void);
#endif 

//u16 iso_out_cnt;
//u8 iso_out_odd_even[64];


// define global variables
// define global variables
struct ven_dev gVEN_DEVICE;
struct ven_common gVEN_COMMON;

/* maxpacket and other transfer characteristics vary by speed. */
#define ep_desc(g,hs,fs) (((g)->speed==USB_SPEED_HIGH)?(hs):(fs))

#define STEP_OUT_RANGE 512
#define STEP_IN_RANGE  512

unsigned int iso_out_count = 0;
unsigned int iso_in_count = 0;

static unsigned int iso_in_value = 0;
struct ven_dev	*vendev_global;

int vendor_check_value(unsigned char *ptr,unsigned int length)
{
        int sum = 0;
        int i = 0;
        printf("ptr[0]=%d\n",ptr[0]);//user can print data here
        for(i=0;i<length;i++)
          sum+=ptr[i];
        return (sum/length);
}


void usbd_bulk_in_complete (struct usb_ep *ep, struct usb_request *req){
  	printf("usbd_bulk_in_complete: complete bulk in!\n");
}

uint8_t *test_buffer_out;
void usbd_bulk_out_complete (struct usb_ep *ep, struct usb_request *req){
  	printf("usbd_bulk_out_complete: complete bulk out!\n");
	for(int i=0;i<16;i++){
		//printf("test_buffer_out = %d\n",test_buffer_out[i]);
	}

	uint8_t *temp_buffer= RtlZmalloc(16);
	for(int i =0;i<16;i++){
		temp_buffer[i]=0x67;
	}

	req->buf = temp_buffer;
	req->dma = (dma_addr_t)req->buf;
	req->complete = usbd_bulk_in_complete;
	int result = usb_ep_queue(vendev_global->in_ep,req,1);
	
}

#if 1
void usbd_vendor_in_complete (struct usb_ep *ep, struct usb_iso_request *req){
	struct ven_dev* vendev = (struct ven_dev*)ep->driver_data;
	struct ven_common* common = vendev->common;
	int status = req->status;
        unsigned long flags;
        int ret = 0;
        unsigned char *mem ;
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
			USBD_ERROR("iso_in_complete ERROR(%d)\n", status);
	}
#if 0
        printf("usbd_vendor_in_complete\r\n");
#else
        iso_in_count++;
        if(iso_in_count>STEP_IN_RANGE){
          printf(".\r\n");
          iso_in_count = 0;
        }
        
        if (req->proc_buf_num == 0){
            mem = (uint8_t *)(req->buf0);
        } else {
            mem = (uint8_t *)(req->buf1);
        }
        memset(mem,iso_in_value,512);
        iso_in_value++;
        iso_in_value%=256;       
#endif
}
void usbd_vendor_out_complete (struct usb_ep *ep, struct usb_iso_request *req)
{
	int i;
	unsigned char *ptr_tmp;
	gpio_write(&gDbgGpioOutTransaction_, 1);
//	for(i=0;i<100;i++){
//		asm volatile ("nop\n\t");
//	}
#if defined (CONFIG_PLATFORM_8721D)
	DCache_Invalidate(((u32)(req->buf0) & CACHE_LINE_ADDR_MSK), (req->data_per_frame + CACHE_LINE_SIZE));
	DCache_Invalidate(((u32)(req->buf1) & CACHE_LINE_ADDR_MSK), (req->data_per_frame1 + CACHE_LINE_SIZE));
#endif
	if(req->proc_buf_num == 0){
		ptr_tmp = (uint8_t *)(req->buf0);
		isoOutRecvStatus_[isoout_complete_conter_].pos = 0;
		isoOutRecvStatus_[isoout_complete_conter_].data = ptr_tmp[0];
	}else{
		ptr_tmp = (uint8_t *)(req->buf1);
		isoOutRecvStatus_[isoout_complete_conter_].pos = 1;
		isoOutRecvStatus_[isoout_complete_conter_].data = ptr_tmp[0];
	}
	if(isoout_complete_conter_== 0){
		xTimerStart(handleIsoOutTimeout, 0);	//Start timer
	}
	isoout_complete_conter_++;

	if(USB_ISOOUT_TRANSFER_CNT == isoout_complete_conter_){
		xTimerStop(handleIsoOutTimeout, 0);
		//OutputIsoOutRecvResult();
	}
	gpio_write(&gDbgGpioOutTransaction_, 0);
	return;
}
#else 
void usbd_vendor_in_complete (struct usb_ep *ep, struct usb_iso_request *req){
	struct ven_dev* vendev = (struct ven_dev*)ep->driver_data;
	struct ven_common* common = vendev->common;
	int status = req->status;
        unsigned long flags;
        int ret = 0;
        unsigned char *mem ;
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
			USBD_ERROR("iso_in_complete ERROR(%d)\n", status);
	}
#if 0
        printf("usbd_vendor_in_complete\r\n");
#else
        iso_in_count++;
        if(iso_in_count>STEP_IN_RANGE){
          printf(".\r\n");
          iso_in_count = 0;
        }
        
        if (req->proc_buf_num == 0){
            mem = (uint8_t *)(req->buf0);
        } else {
            mem = (uint8_t *)(req->buf1);
        }
        memset(mem,iso_in_value,512);
        iso_in_value++;
        iso_in_value%=256;       
#endif
}
void usbd_vendor_out_complete (struct usb_ep *ep, struct usb_iso_request *req){
        struct ven_dev *vendev = ep->driver_data;  
	struct ven_common* common = vendev->common;
        int ret = 0;
	int status = req->status;
        int value = 0;
        unsigned char *ptr_tmp;
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
			USBD_ERROR("iso_in_complete ERROR(%d)\n", status);
	}
#if 0
        printf("usbd_msc_bulk_out_complet = %d 1:%d 2:%d\r\n",status,req->iso_packet_desc0->actual_length,status,req->iso_packet_desc1->actual_length);
#endif
#if 0
        printf("usbd_vendor_out_complete\r\n");
#else
        iso_out_count++;
        if(iso_out_count>STEP_OUT_RANGE){
          printf(".\r\n");
          iso_out_count = 0;
        }
        
    
        if (req->proc_buf_num == 0){
            ptr_tmp = (uint8_t *)(req->buf0);
            //printf("len0=%d\n",req->iso_packet_desc0->actual_length);
            value = vendor_check_value(ptr_tmp,req->iso_packet_desc0->actual_length);
            //DBG_8195A("0: %02x %02x\n", *(ptr_tmp),req->iso_packet_desc0->actual_length);
        } else {
            ptr_tmp = (uint8_t *)(req->buf1);
            //printf("len1=%d\n",req->iso_packet_desc1->actual_length);
            value = vendor_check_value(ptr_tmp,req->iso_packet_desc1->actual_length);
            //DBG_8195A("0: %02x %02x\n", *(ptr_tmp),req->iso_packet_desc1->actual_length);
        }
        printf("value=%d\n",value);
#endif
}
#endif 

#define VERIFY_TYPE_ISO_IN      1
#define VERIFY_TYPE_ISO_OUT     1
#define VERIFY_TYPE_ISO_INOUT	0
#define VERIFY_TYPE_BULK_INOUT  0




static int fun_set_alt(struct usb_function *f, unsigned interface, unsigned alt){
	struct ven_dev	*vendev = CONTAINER_OF(f,struct ven_dev, func);
	struct ven_common *common = vendev->common;
	struct usb_iso_request  *isoinreq;
	struct usb_iso_request  *isooutreq;
	int status = 0;
	
#if VERIFY_TYPE_BULK_INOUT
    struct usb_request	*req;
	struct usb_request	*req2;
	vendev_global = vendev;
	int result = 0;
	int i;

//BULK OUT
if (vendev->out_ep != NULL) {
  
	vendev->out_ep->desc = ep_desc (common->gadget, &usbd_vendor_bulk_sink_desc_HS, &usbd_vendor_bulk_sink_desc_FS); 
	status = usb_ep_enable(vendev->out_ep,vendev->out_ep->desc);
	if (status < 0) {
			USBD_ERROR("Enable out endpoint FAILED!\n");
	}
    vendev->out_ep->driver_data=vendev;
	req2 = usb_ep_alloc_request(vendev->out_ep, 1);
	if (!req2){
			printf("bulk out malloc bufhd->reqin fail\n");
	}
	
	test_buffer_out = RtlZmalloc(16);
	for(i=0;i<16;i++){
		test_buffer_out[i]=0x55;
	}
	
	req2->buf = test_buffer_out;
	req2->length = 16;
	req2->complete = usbd_bulk_out_complete;
	status = usb_ep_queue(vendev->out_ep, req2, 1);
	if(status){
		printf("enqueue out endpoint FAILED!\n");
	}

	
}

//BULK IN
if (vendev->in_ep != NULL) {

	if (vendev->in_ep->driver_data != NULL)
		usb_ep_disable(vendev->in_ep);
	
	vendev->in_ep->desc = ep_desc(common->gadget,&usbd_vendor_bulk_source_desc_HS,&usbd_vendor_bulk_source_desc_FS);       
	status = usb_ep_enable(vendev->in_ep,vendev->in_ep->desc);   
	if (status < 0) {
	  	printf("bulk: Enable IN endpoint FAILED!\n");
	}
	vendev->in_ep->driver_data=vendev;
	
	req=usb_ep_alloc_request(vendev->in_ep,1);
	if (!req){
		printf("bulk: malloc bufhd->reqin fail\n");
	}	
	
	uint8_t *test_in_buffer= RtlZmalloc(16);
	for(i=0;i<16;i++){
		test_in_buffer[i]=0x55;
	}

	req->buf = test_in_buffer;
	req->length = 8;
	req->dma = (dma_addr_t)req->buf;
	req->complete = usbd_bulk_in_complete;
}

#endif

        
#if VERIFY_TYPE_ISO_IN
          if(common->running == 1){
              common->running = 0;
              
              if(vendev->req_in){
                        printf("deinit iso in \r\n");
                        iso_ep_stop(vendev->iso_in_ep,vendev->req_in);
                        free_iso_request(vendev->iso_in_ep,vendev->req_in);
                        vendev->req_in = NULL;
              }
              
              vendev->iso_in_ep->driver_data = NULL;
          }
          if(common->running == 0){
                if (vendev->iso_in_ep != NULL) {
                    /* restart endpoint */
                    if (vendev->iso_in_ep->driver_data != NULL){
                                    usb_ep_disable(vendev->iso_in_ep);
                                    printf("usb_ep_disable\r\n");
                    }
                    /*
                    * assign an corresponding endpoint descriptor to an endpoint
                    * according to the gadget speed
                    */

                    vendev->iso_in_ep->desc = ep_desc (common->gadget, &usbd_vendor_iso_source_desc_HS, &usbd_vendor_iso_source_desc_FS);
                    status = usb_ep_enable(vendev->iso_in_ep, vendev->iso_in_ep->desc);
                    
                    if (status < 0) {
                                    USBD_ERROR("Enable out endpoint FAILED!\n");
                                    //goto fail;
                    }
                    vendev->iso_in_ep->driver_data = vendev;
                     
                    isoinreq = alloc_iso_request(vendev->iso_in_ep, 1, 1);
                    if (!isoinreq){
                                      USBD_ERROR("malloc buffer fail\n");
                    }
                    vendev->req_in = isoinreq;
                    isoinreq->buf0 = vendev->req_buffer_in[0];
                    isoinreq->buf1 = vendev->req_buffer_in[1];
                    isoinreq->dma0 = (dma_addr_t)isoinreq->buf0;
                    isoinreq->dma1 = (dma_addr_t)isoinreq->buf1;
                    //20170822 DBG_8195A("vendev->iso_in_ep->desc->bInterval: %x\n", vendev->iso_in_ep->desc->bInterval);
                    isoinreq->buf_proc_intrvl = (1 << (vendev->iso_in_ep->desc->bInterval - 1));
                    isoinreq->zero = 0;
                    isoinreq->sync_frame = 0;
                    isoinreq->data_pattern_frame = 0;
                    isoinreq->data_per_frame = 128;
                    isoinreq->data_per_frame1 = 128;
                    isoinreq->start_frame = 0;
#if defined(USBD_ENABE_ISOIN_USB_REQ_ISO_ASAP)
                    isoinreq->flags |= USB_REQ_ISO_ASAP;
#endif
                    isoinreq->proc_buf_num = 0;
                    isoinreq->process_buffer = usbd_vendor_in_complete;
                    isoinreq->context = vendev;
                    
                    common->running = 1;
                    iso_ep_start(vendev->iso_in_ep, vendev->req_in, 1);
                    //iso_ep_start(vendev->iso_in_ep, vendev->req_in[1], 1);
                }
            }
#endif
#if VERIFY_TYPE_ISO_OUT
          if(common->running == 1){
              common->running = 0;

              if(vendev->req){
                      printf("deinit iso out \r\n");
                      iso_ep_stop(vendev->iso_out_ep,vendev->req);
                      free_iso_request(vendev->iso_out_ep,vendev->req);
                      vendev->req = NULL;
              }
              
              vendev->iso_out_ep->driver_data = NULL;
          }
          if(common->running == 0){
                if (vendev->iso_out_ep != NULL) {
                    /* restart endpoint */
                    if (vendev->iso_out_ep->driver_data != NULL){
                                    usb_ep_disable(vendev->iso_out_ep);
                                    printf("usb_ep_disable\r\n");
                    }
                    /*
                    * assign an corresponding endpoint descriptor to an endpoint
                    * according to the gadget speed
                    */

                    vendev->iso_out_ep->desc = ep_desc (common->gadget, &usbd_vendor_iso_sink_desc_HS, &usbd_vendor_iso_sink_desc_FS);
                    status = usb_ep_enable(vendev->iso_out_ep, vendev->iso_out_ep->desc);
                    
                    if (status < 0) {
                                    USBD_ERROR("Enable out endpoint FAILED!\n");
                                    //goto fail;
                    }
                    vendev->iso_out_ep->driver_data = vendev;
                    
                    
                    //req = usb_ep_alloc_request(vendev->iso_in_ep, 1);
                    isooutreq = alloc_iso_request(vendev->iso_out_ep, 1, 1);
                    if (!isooutreq){
                                                    USBD_ERROR("malloc buffer fail\n");
                    }
                    vendev->req = isooutreq;
                    isooutreq->buf0 = vendev->req_buffer[0];
                    isooutreq->buf1 = vendev->req_buffer[1];
                    isooutreq->dma0 = (dma_addr_t)isooutreq->buf0;
                    isooutreq->dma1 = (dma_addr_t)isooutreq->buf1;
                    //20170822 DBG_8195A("vendev->iso_in_ep->desc->bInterval: %x\n", vendev->iso_in_ep->desc->bInterval);
                    isooutreq->buf_proc_intrvl = (1 << (vendev->iso_out_ep->desc->bInterval - 1));
                    isooutreq->zero = 0;
                    isooutreq->sync_frame = 0;
                    isooutreq->data_pattern_frame = 0;
                    isooutreq->data_per_frame = 64;
                    isooutreq->data_per_frame1 = 64;
                    isooutreq->start_frame = 0;
#if defined(USBD_ENABE_ISOOUT_USB_REQ_ISO_ASAP)
                    isooutreq->flags |= USB_REQ_ISO_ASAP;
#endif
                    isooutreq->proc_buf_num = 0;
                    isooutreq->process_buffer = usbd_vendor_out_complete;
                    isooutreq->context = vendev; 
                    
                    common->running = 1;
                    iso_ep_start(vendev->iso_out_ep, vendev->req, 1);
                    //iso_ep_start(vendev->iso_in_ep, vendev->req_in[1], 1);
                }
            }
#endif
#if VERIFY_TYPE_ISO_INOUT
          if(common->running == 1){
              common->running = 0;
              // ISO IUT REQUEST
              if(vendev->req){
                  printf("deinit iso out \r\n");
                  iso_ep_stop(vendev->iso_out_ep,vendev->req);
                  free_iso_request(vendev->iso_out_ep,vendev->req);
                  vendev->req = NULL;
              }
              vendev->iso_out_ep->driver_data = NULL;
              // ISO IN REQUEST
              if(vendev->req_in){
                    printf("deinit iso in\r\n");
                    iso_ep_stop(vendev->iso_in_ep,vendev->req_in);
                    free_iso_request(vendev->iso_in_ep,vendev->req_in);
                    vendev->req_in = NULL;
              }
              vendev->iso_in_ep->driver_data = NULL;
              printf("Vendor Reset\r\n");
          }
          if(common->running == 0){
                if (vendev->iso_in_ep != NULL) {
                    /* restart endpoint iso in */
                    if (vendev->iso_in_ep->driver_data != NULL){
                          usb_ep_disable(vendev->iso_in_ep);
                          printf("usb_ep_disable\r\n");
                    }
                    /*
                    * assign an corresponding endpoint descriptor to an endpoint
                    * according to the gadget speed
                    */

                    vendev->iso_in_ep->desc = ep_desc (common->gadget, &usbd_vendor_iso_source_desc_HS, &usbd_vendor_iso_source_desc_FS);
                    status = usb_ep_enable(vendev->iso_in_ep, vendev->iso_in_ep->desc);
                    
                    if (status < 0) {
                          USBD_ERROR("Enable out endpoint FAILED!\n");
                          //goto fail;
                    }
                    vendev->iso_in_ep->driver_data = vendev;
                     
                    isoinreq = alloc_iso_request(vendev->iso_in_ep, 1, 1);
                    if (!isoinreq){
                          USBD_ERROR("malloc buffer fail\n");
                    }
                    vendev->req_in = isoinreq;
                    isoinreq->buf0 = vendev->req_buffer_in[0];
                    isoinreq->buf1 = vendev->req_buffer_in[1];
                    isoinreq->dma0 = (dma_addr_t)isoinreq->buf0;
                    isoinreq->dma1 = (dma_addr_t)isoinreq->buf1;
                    isoinreq->buf_proc_intrvl = (1 << (vendev->iso_in_ep->desc->bInterval - 1));
                    isoinreq->zero = 0;
                    isoinreq->sync_frame = 0;
                    isoinreq->data_pattern_frame = 0;
                    isoinreq->data_per_frame = 512;
                    isoinreq->data_per_frame1 =512;
                    isoinreq->start_frame = 0;
                    isoinreq->proc_buf_num = 0;
                    isoinreq->process_buffer = usbd_vendor_in_complete;
                    isoinreq->context = vendev;
                    
                    common->running = 1;
                    iso_ep_start(vendev->iso_in_ep, vendev->req_in, 1);
                    
                }
                if (vendev->iso_out_ep != NULL) {
                    /* restart endpoint for iso out */
                    if (vendev->iso_out_ep->driver_data != NULL){
                                    usb_ep_disable(vendev->iso_out_ep);
                                    printf("usb_ep_disable\r\n");
                    }
                    /*
                    * assign an corresponding endpoint descriptor to an endpoint
                    * according to the gadget speed
                    */

                    vendev->iso_out_ep->desc = ep_desc (common->gadget, &usbd_vendor_iso_sink_desc_HS, &usbd_vendor_iso_sink_desc_FS);
                    status = usb_ep_enable(vendev->iso_out_ep, vendev->iso_out_ep->desc);
                    
                    if (status < 0) {
                          USBD_ERROR("Enable out endpoint FAILED!\n");
                          //goto fail;
                    }
                    vendev->iso_out_ep->driver_data = vendev;
                    
                    
                    //req = usb_ep_alloc_request(vendev->iso_in_ep, 1);
                    isooutreq = alloc_iso_request(vendev->iso_out_ep, 1, 1);
                    if (!isooutreq){
                            USBD_ERROR("malloc buffer fail\n");
                    }
                    vendev->req = isooutreq;
                    isooutreq->buf0 = vendev->req_buffer[0];
                    isooutreq->buf1 = vendev->req_buffer[1];
                    isooutreq->dma0 = (dma_addr_t)isooutreq->buf0;
                    isooutreq->dma1 = (dma_addr_t)isooutreq->buf1;
                    isooutreq->buf_proc_intrvl = (1 << (vendev->iso_out_ep->desc->bInterval - 1));
                    isooutreq->zero = 0;
                    isooutreq->sync_frame = 0;
                    isooutreq->data_pattern_frame = 0;
                    isooutreq->data_per_frame = 512;
                    isooutreq->data_per_frame1 = 512;
                    isooutreq->start_frame = 0;
                    isooutreq->proc_buf_num = 0;
                    isooutreq->process_buffer = usbd_vendor_out_complete;
                    isooutreq->context = vendev; 
                    common->running = 1;
                    iso_ep_start(vendev->iso_out_ep, vendev->req, 1);

                }
				printf("Vendor on\r\n");
            }
#endif
    return 0;
}

void fun_disable (struct usb_function *f){
}

static int fun_bind(struct usb_configuration *c, struct usb_function *f){
	struct ven_dev	*vendev = CONTAINER_OF(f,struct ven_dev, func);
	struct usb_ep *ep;

	int status = -ENODEV;
	int id;

FUN_ENTER;
        printf("fun_bind\r\n");
	/* allocate instance-specific interface IDs, and patch descriptors */
	id = usb_interface_id(c, f); // this will return the allocated interface id 
	if (id < 0){
		status = id;
		goto fail;
	}
	usbd_vendor_intf0_desc.bInterfaceNumber = id;
        
        //id = usb_interface_id(c, f); // this will return the allocated interface id 
	//if (id < 0){
	//	status = id;
	//	goto fail;
	//}
        usbd_vendor_intf1_desc.bInterfaceNumber = id;
	//vendev->interface_number = id; // record interface number

	/* search endpoint according to endpoint descriptor and modify endpoint address*/
	ep = usb_ep_autoconfig(c->cdev->gadget, &usbd_vendor_bulk_source_desc_FS);
	if (!ep)
		goto fail;
	ep->driver_data = vendev;/* claim */
	vendev->in_ep = ep;

	ep = usb_ep_autoconfig(c->cdev->gadget, &usbd_vendor_bulk_sink_desc_FS);
	if (!ep)
		goto fail;
	ep->driver_data = vendev;/* claim */
	vendev->out_ep = ep;
        
        /* search endpoint according to endpoint descriptor and modify endpoint address*/
	ep = usb_ep_autoconfig(c->cdev->gadget, &usbd_vendor_iso_source_desc_FS);
	if (!ep)
		goto fail;
	ep->driver_data = vendev;/* claim */
	vendev->iso_in_ep = ep;

	ep = usb_ep_autoconfig(c->cdev->gadget, &usbd_vendor_iso_sink_desc_FS);
	if (!ep)
		goto fail;
	ep->driver_data = vendev;/* claim */
	vendev->iso_out_ep = ep;
        
        

	usbd_vendor_bulk_source_desc_HS.bEndpointAddress =
			usbd_vendor_bulk_source_desc_FS.bEndpointAddress;// modified after calling usb_ep_autoconfig()
	usbd_vendor_bulk_sink_desc_HS.bEndpointAddress =
			usbd_vendor_bulk_sink_desc_FS.bEndpointAddress;
        
        usbd_vendor_iso_source_desc_HS.bEndpointAddress =
			usbd_vendor_iso_source_desc_FS.bEndpointAddress;// modified after calling usb_ep_autoconfig()
	usbd_vendor_iso_sink_desc_HS.bEndpointAddress =
			usbd_vendor_iso_sink_desc_FS.bEndpointAddress;
        
        printf("iso address in =%x out = %d\r\n",usbd_vendor_iso_source_desc_HS.bEndpointAddress,usbd_vendor_iso_sink_desc_HS.bEndpointAddress);

	status = usb_assign_descriptors(f, usbd_vendor_descriptors_FS,
			usbd_vendor_descriptors_HS, NULL);
	if (status){
                printf("status = %d\r\n",status);
		goto fail;
        }
FUN_EXIT;
	return 0;
FUN_EXIT;
fail:
	usb_free_all_descriptors(f);
	return status;
}

void fun_unbind(struct usb_configuration *c, struct usb_function *f){
	struct ven_dev *vendev = CONTAINER_OF(f,struct ven_dev, func);
FUN_ENTER;
	usb_free_all_descriptors(f);
		/* free task */

FUN_EXIT;
}


int fun_setup(struct usb_function *fun, const struct usb_ctrlrequest *ctrl){
	struct ven_dev *vendev = CONTAINER_OF(fun,struct ven_dev, func);
	struct usb_ep *ep0 = vendev->common->ep0;
	struct usb_request	*req0 = vendev->common->req0;
FUN_ENTER;
	u16 wLength = ctrl->wLength;
	u16 wIndex = ctrl->wIndex;
	u16	wValue = ctrl->wValue;

	int status;
    
	req0->context = NULL;
	req0->length = ctrl->wLength;
        
        printf("setup request %02x %02x value %04x index %04x %04x\n",
        ctrl->bRequestType, ctrl->bRequest, ctrl->wValue,
        ctrl->wIndex, ctrl->wLength);
        
        memcpy(req0->buf,ctrl,sizeof(struct usb_ctrlrequest ));
        
        status = usb_ep_queue (ep0, req0, 1);
        
        if (status != 0 && status != -ESHUTDOWN) {
              /* We can't do much more than wait for a reset */
              USBD_WARN("error in submission: %s --> %d\n",
                      ep0->name, status);
        }
        
        return status;

FUN_EXIT;
	return -EOPNOTSUPP;
}

void fun_free(struct usb_function *f){

}

static int config_bind(struct usb_configuration *c){
        struct ven_dev *vendev;
	struct ven_common *common;
	int status = 0;
FUN_ENTER;

	vendev =(struct ven_dev *)c->cdev->gadget->device;
	common = vendev->common;

	common->gadget = c->cdev->gadget;
	
	vendev->func.name = "msc_function_sets";
	vendev->func.bind = fun_bind;
	vendev->func.unbind = fun_unbind;
	vendev->func.setup = fun_setup;
	vendev->func.set_alt = fun_set_alt;
	vendev->func.disable = fun_disable;
	vendev->func.free_func = fun_free;
	
	status = usb_add_function(c, &vendev->func);
	if(status)
		goto free_fun;

FUN_EXIT;
	return status;

free_fun:
FUN_EXIT;
	usb_put_function(&vendev->func);
	return status;
}

void config_unbind(struct usb_configuration *c){
        
}


static struct usb_configuration usbd_vendor_cfg_driver = {
	.label			= "usbd-vendor",
	.unbind			= config_unbind,
	.bConfigurationValue	= DEV_CONFIG_VALUE,
	.bmAttributes		= USB_CONFIG_ATT_ONE,// | USB_CONFIG_ATT_SELFPOWER,
};

int usbd_vendor_bind(
    struct usb_composite_dev *cdev
)
{
	int status = 0;
	struct ven_dev *vendev;
	struct ven_common *common;
	
FUN_ENTER;

	vendev = &gVEN_DEVICE;
	common = &gVEN_COMMON;

	cdev->gadget->device = vendev;


	vendev->common = common;

	common->gadget = cdev->gadget;
	common->ep0 = cdev->gadget->ep0;
	common->req0 = cdev->req;
        
        //printf("usbd_vendor_cfg_driver %d %s\r\n",usbd_vendor_cfg_driver.bConfigurationValue,usbd_vendor_cfg_driver.label);
        /* register configuration(s)*/
	status = usb_add_config(cdev, &usbd_vendor_cfg_driver,
			config_bind);
	if (status < 0)
		printf("usbd_vendor_bind set error %d\r\n",status);
        
        

FUN_EXIT;
	return 0;

FUN_EXIT;

    return status;
}

int usbd_vendor_unbind(
    struct usb_composite_dev *cdev
)
{
	struct ven_dev *vendev = (struct ven_dev*)cdev->gadget->device;
	struct ven_common *common;
FUN_ENTER;

	common = vendev->common;

	usb_put_function(&vendev->func);

FUN_EXIT;
	return 0;
}

void usbd_vendor_disconnect(struct usb_composite_dev *cdev){
	USBD_PRINTF("usbd_vendor_disconnect\n");
}


struct usb_composite_driver     usbd_vendor_driver = {
	.name 	    = "usbd_vendor_driver",
	.dev		= &usbd_vendor_device_desc,
	.strings 	= dev_vendor_strings,
	.bind       = usbd_vendor_bind,
	.unbind     = usbd_vendor_unbind,
	.disconnect = usbd_vendor_disconnect,
	.max_speed  = USB_SPEED_HIGH,
};


int usbd_vendor_init(int N_bh, int Size_bh, int type){
        int i = 0;
        printf("usbd_vendor_init\r\n");
#if defined (CONFIG_PLATFORM_8721D)
	DBG_ERR_MSG_ON(MODULE_USB_OTG);
	//DBG_WARN_MSG_ON(MODULE_USB_OTG);
	//DBG_INFO_MSG_ON(MODULE_USB_OTG);
#else
        DBG_ERR_MSG_ON(_DBG_USB_OTG_);
	//DBG_WARN_MSG_ON(_DBG_USB_OTG_);
	//DBG_INFO_MSG_ON(_DBG_USB_OTG_);
#endif
        memset(&gVEN_DEVICE,0,sizeof(struct ven_dev));//ven_dev
	memset(&gVEN_COMMON,0,sizeof(struct ven_common));
        for(i = 0; i < VEN_NUM_REQUESTS; i++){
          gVEN_DEVICE.req_buffer[i]=malloc(512);
          memset(gVEN_DEVICE.req_buffer[i],0,512);
        }
        for(i = 0; i < VEN_NUM_REQUESTS; i++){
          gVEN_DEVICE.req_buffer_in[i]=malloc(512);
          memset(gVEN_DEVICE.req_buffer_in[i],iso_in_value,512);
          iso_in_value++;
          iso_in_value%=256;  
        }
#if 1
	gpio_init(&gDbgGpioInTransaction_, GPIO_DGB_USBINCALLBACK_PIN);
	gpio_dir(&gDbgGpioInTransaction_, PIN_OUTPUT); 
	gpio_mode(&gDbgGpioInTransaction_, PullNone); 
	//--
	gpio_init(&gDbgGpioOutTransaction_, GPIO_DGB_USBOUTCALLBACK_PIN);
	gpio_dir(&gDbgGpioOutTransaction_, PIN_OUTPUT); 
	gpio_mode(&gDbgGpioOutTransaction_, PullNone); 
	//--
	isoOutRecvStatus_ = (IsoOutRecvStatus*)malloc(USB_ISOOUT_TRANSFER_CNT * sizeof(IsoOutRecvStatus));
	//--
	handleIsoOutTimeout = xTimerCreate("IsoOutTimeout",(1000 / portTICK_RATE_MS), pdFALSE, (void *)0, IsoOutTimerCallback);	//one-shot

#endif 
        return usb_composite_probe(&usbd_vendor_driver);
}


void usbd_vendor_deinit(void){
	usb_composite_unregister(&usbd_vendor_driver);
}

static void IsoOutTimerCallback(xTimerHandle *pxTimer)
{
	OutputIsoOutRecvResult();
}
static void OutputIsoOutRecvResult(void)
{
	int i;

	if(isoout_complete_conter_ == USB_ISOOUT_TRANSFER_CNT){
		printf("\n%s() IsoOut CallbackCount=%d[%s]\n", __func__, USB_ISOOUT_TRANSFER_CNT, "\033[32mcorrect\033[39m");
	}else{
		printf("\n%s() TIMEOUT has occurred. Expected count(%d), Receive count(%d) [%s].\n", __func__, USB_ISOOUT_TRANSFER_CNT, isoout_complete_conter_, "\033[31mincorrect\033[39m");
	}
	uint32_t calc=((USB_ISOOUT_TRANSFER_CNT*(USB_ISOOUT_TRANSFER_CNT-1))/2);	//sigma
	uint32_t sum=0;
	for(i=0;i<isoout_complete_conter_;i++){
		sum+=isoOutRecvStatus_[i].data;
	}

	printf("--IsoOutRecvStatus--\nNo\tbufpos\tbufdata\texpect\n");
	for(i=0;i<isoout_complete_conter_;i++){
		printf("%d\t%d\t0x%x\t0x%x\n", i, isoOutRecvStatus_[i].pos, isoOutRecvStatus_[i].data, i);
	}
	printf("\n");

	if(isoout_complete_conter_ == USB_ISOOUT_TRANSFER_CNT){
		printf("--Consistency--\nCalcValue=%d, sumValue=%d, %s.\n",  calc, sum, ((calc==sum)?("\033[32mcorrect\033[39m"):("\033[31mincorrect\033[39m")));
	}
}

//==============================ATCMD===========================
#include "at_cmd/log_service.h" 
#if 1
void fATU0(void *arg)
{   
    OutputIsoOutRecvResult();
}

log_item_t at_usb_items[ ] = {
	{"ATU0", fATU0,}, // Audio Recording
};

void at_usb_init(void)
{
	log_service_add_table(at_usb_items, sizeof(at_usb_items)/sizeof(at_usb_items[0]));
}

#if SUPPORT_LOG_SERVICE
log_module_init(at_usb_init);
#endif

#endif

