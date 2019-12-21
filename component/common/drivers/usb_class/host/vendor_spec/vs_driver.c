#include "isoout_test.h"  
#include "vs_intf.h"
#include "usb.h"
#define VC_TASK_PRIORITY       2
#if defined (CONFIG_PLATFORM_8721D)
#define RtlZmalloc rtw_zmalloc
struct usbtest_info vs_info;
#endif

struct usbtest_info {
	const char		*name;
	u8			ep_in;		/* bulk/intr source */
	u8			ep_out;		/* bulk/intr sink */
	unsigned		autoconf:1;
	unsigned		ctrl_out:1;
	unsigned		iso:1;		/* try iso in/out */
	unsigned		intr:1;		/* try interrupt in/out */
	int			alt;
};

struct usbtest_dev {
		struct usb_interface	*intf;
		struct usbtest_info	*info;
		int			in_pipe;
		int			out_pipe;
		int			in_pipe_addr;
		int			out_pipe_addr;
		int			in_iso_pipe;
		int			out_iso_pipe;
                int			in_iso_addr;
		int			out_iso_addr;
		int			in_int_pipe;
		int			out_int_pipe;
		struct usb_endpoint_descriptor	*iso_in, *iso_out;
		struct usb_endpoint_descriptor	*int_in, *int_out;
		//struct mutex		lock;

#define TBUF_SIZE	256
	u8			*buf;
};

static struct usb_device *testdev_to_usbdev(struct usbtest_dev *test)
{
	return interface_to_usbdev(test->intf);
}

static void simple_callback(struct urb *urb)
{
  usb_free_urb(urb);
}

static void simple_callback1(struct urb *urb)
{
  usb_free_urb(urb);
}


static inline void endpoint_update(int edi,
				   struct usb_host_endpoint **in,
				   struct usb_host_endpoint **out,
				   struct usb_host_endpoint *e)
{
	if (edi) {
            if (!*in)
               *in = e;
	} else {
	    if (!*out)
               *out = e;
	}
}



static int get_endpoints(struct usbtest_dev *dev, struct usb_interface *intf)
{     
      int tmp =0;
      struct usb_host_interface *alt;
      struct usb_host_endpoint	*in, *out;
      struct usb_host_endpoint	*iso_in, *iso_out;
      struct usb_host_endpoint	*int_in, *int_out;
      struct usb_device	*udev ;

#if defined (CONFIG_PLATFORM_8721D)
	dev->info = &vs_info;
#endif

      for (tmp = 0; tmp < intf->num_altsetting; tmp++) {
          unsigned ep;
          in = out = NULL;
          iso_in = iso_out = NULL;
          int_in = int_out = NULL;
          alt = intf->altsetting + tmp;
          
          for (ep = 0; ep < alt->desc.bNumEndpoints; ep++) {
              struct usb_host_endpoint *e = &alt->endpoint[ep];
              struct usb_endpoint_descriptor *desc = &e->desc;
              int edi;
              //printf("Endpoint address = 0x%x\n",(desc->bEndpointAddress));
              //e = alt->endpoint + ep;
              edi = usb_endpoint_dir_in(&e->desc);
              switch (usb_endpoint_type(&e->desc)) {
			case USB_ENDPOINT_XFER_BULK:
                                if (edi){
                                    dev->in_pipe_addr = desc->bEndpointAddress;
                                }else{
                                    dev->out_pipe_addr = desc->bEndpointAddress;
                                } 
                                //printf("find bulk endpoint\n");  
				continue;
			case USB_ENDPOINT_XFER_INT:
				if (dev->info->intr)
					endpoint_update(edi, &int_in, &int_out, e);
				continue;
			case USB_ENDPOINT_XFER_ISOC:
                                if (edi){
                                    dev->in_iso_addr = desc->bEndpointAddress;
                                    dev->iso_in = &e->desc;
                                }else{
                                    dev->out_iso_addr = desc->bEndpointAddress;
                                    dev->iso_out = &e->desc;
                                }
                                //if (dev->info->iso)
                                    endpoint_update(edi, &iso_in, &iso_out, e);

			default:
				continue;
			}
          }
        
      }
      
      if ((in && out)  ||  iso_in || iso_out || int_in || int_out)
              goto found;      
found:
      udev = testdev_to_usbdev(dev);
      dev->info->alt = alt->desc.bAlternateSetting;
      //printf("&iso_in->desc->wMaxPacketSize=%d\n",&iso_in->desc->wMaxPacketSize);
      	if (iso_in) {
		dev->iso_in = &iso_in->desc;
		dev->in_iso_pipe = usb_rcvisocpipe(udev,
				iso_in->desc.bEndpointAddress
					& USB_ENDPOINT_NUMBER_MASK);
            printf("iso_in->desc.bEndpointAddress =0x%x\n",iso_in->desc.bEndpointAddress);
	}

	if (iso_out) {
		dev->iso_out = &iso_out->desc;
		dev->out_iso_pipe = usb_sndisocpipe(udev,
				iso_out->desc.bEndpointAddress
					& USB_ENDPOINT_NUMBER_MASK);
             printf("iso_out->desc.bEndpointAddress =0x%x\n",iso_out->desc.bEndpointAddress);
	}
      return 0;
            
}

static int set_altsetting(struct usbtest_dev *dev, int alternate)
{
	struct usb_interface		*iface = dev->intf;
	struct usb_device		*udev;

	if (alternate < 0 || alternate >= 256)
		return -EINVAL;

	udev = interface_to_usbdev(iface);
	return usb_set_interface(udev,
			iface->altsetting[0].desc.bInterfaceNumber,
			alternate);
}

volatile int temp = 0;
unsigned int timer1;
unsigned int timer_old;


int DumpInArray[IN_STORAGE_SIZE];
volatile int in_count = 0;
volatile int in_count_check = 0;
int CallbackBit = 1;
static void complicated_callback(struct urb *urb)
{
  //printf("complicated_callback_in\n");
  in_count_check++;
  if(CallbackBit == 1){ 
      in_count ++;
      //printf("in call urb->status = %d\n",urb->status);
      
      int ret =0;
      int i,j;
      
              u8 *mem;
              for (i = 0; i < urb->number_of_packets; ++i) {
                  mem = (u8 *)urb->transfer_buffer + urb->iso_frame_desc[i].offset;
                  int g = 0;
                  temp = 0;
                  
                  for (g=0;g<IN_BUFFER_SIZE;g++){
                      temp = temp + mem[g];
                  }
                  temp = temp/IN_BUFFER_SIZE;
                  DumpInArray[in_count_check] = temp;
                  //printf("sony cn =%d, temp =%d\n",in_count,temp);
                  if (temp == 255){
                      CallbackBit = 0;
                      printf("in_count=%d\n",in_count);
                      printf("in_count_check=%d\n",in_count_check);
                      in_count = 0;
#if 0
                      for(j=0;j<IN_STORAGE_SIZE;j++){
                         printf("DumpInArray[%d] = %d\n",j,DumpInArray[j]);
                      }
#endif
                      return;
                  }
                  
                  for (g=0;g<IN_BUFFER_SIZE;g++){
                      mem[g] = 0;
                  }
              }
      if ((ret = usb_submit_urb(urb)) < 0) {
              printf("\n\rFailed to resubmit  URB (%d).\n", ret);
      }
  }else{
    return;
  }
    
}

//static u8 *iso_out_buffer ;
volatile int out_number_=0x00;
int isoout_complicated_callback_conter_ = 0;
static void complicated_callback_out(struct urb *urb)
{   
    int ret;     
    int i2;
    int i = 0;
    u8 *iso_out_buf_complete = malloc(OUT_BUFFER_SIZE);
    memset(iso_out_buf_complete, out_number_, OUT_BUFFER_SIZE);
    urb->transfer_buffer = iso_out_buf_complete ;

    isoout_complicated_callback_conter_++;
    if(USB_IN_CNT <= isoout_complicated_callback_conter_){
      printf("end callback out!\n");
      free(iso_out_buf_complete);
      return; //STOP OUT_TRANSACTION
    }

    if ((ret = usb_submit_urb(urb)) < 0) {
            printf("\n\rFailed to resubmit  URB (%d).\n", ret);
            //uvc_printk(KERN_ERR, "Failed to resubmit video URB (%d).\n", ret);
    }
    out_number_++;
    out_number_ = out_number_%256;
    free(iso_out_buf_complete);
}
static unsigned int vs_endpoint_max_bpi(struct usb_device *dev,
                                        struct usb_endpoint_descriptor *desc1)
					 //struct usb_host_endpoint *ep)
{
	u16 psize;

	switch (dev->speed) {
#if 0
	case USB_SPEED_SUPER:
		return ep->ss_ep_comp.wBytesPerInterval;
#endif
	case USB_SPEED_HIGH:
		//psize = usb_endpoint_maxp(&ep->desc);
                psize = desc1->wMaxPacketSize;
		return (psize & 0x07ff) * (1 + ((psize >> 11) & 3));
	default:
                psize = desc1->wMaxPacketSize;
		//psize = usb_endpoint_maxp(&ep->desc);
		return psize & 0x07ff;
	}
}

//========gloaba declaration for free===================
struct usbtest_dev *my_dev;
struct usbtest_dev *dev_ctrl;
struct usb_device *udev_control;
struct usb_interface *intf_for_free;

int usb_connect_state = 0;//define connecting state, 1:connecting 0:disconnect

#ifdef  iso_test_in
struct urb *urb_iso_in[2];
u8 *iso_in_buffer;
#endif
#ifdef  iso_test_out
struct urb *urb_iso_out[2];
u8 *iso_out_buffer;
#endif

//======================================================

int vs_probe(struct usb_interface *intf)	
{
    usb_connect_state = 1;
    struct usb_device *udev = interface_to_usbdev(intf);  
    volatile int j = 0;
    
    if((my_dev = (struct usbtest_dev *)RtlZmalloc(sizeof *my_dev)) == NULL)
            printf("my_device error\n");
    my_dev -> intf = intf;
    usb_set_intfdata(intf, my_dev);

    dev_ctrl = usb_get_intfdata(intf);
    udev_control = testdev_to_usbdev(dev_ctrl);
    
    //set_altsetting(dev_ctrl,0);
    
   /**********************************************
    * Get endpoint address                       *
    **********************************************/    
    get_endpoints(dev_ctrl,intf);    
    
   /********************************************************
    * ctrl setup request: Get device descriptor Request    *
    ********************************************************/ 
#ifdef ctrl_test     
    struct urb *urb0; 
    urb0 = usb_alloc_urb(0);
    u8 *buf0 =  RtlZmalloc(18);
    
    unsigned char *ctrl_buf = RtlZmalloc(8);
    ctrl_buf[0]= 0x80;//0xa1;
    ctrl_buf[1]= 0x06;//0xfe;
    ctrl_buf[2]= 0x00;
    ctrl_buf[3]= 0x01;//0;
    ctrl_buf[4]= 0;
    ctrl_buf[5]= 0;
    ctrl_buf[6]= 0x12;//1;
    ctrl_buf[7]= 0;      

    /****************************************************************
     * allocate the parameter of control URB,                       *
     * call usb_submit_urb to transmit/receive data                 *
     * if the transmission complete, the return value will be zero, *
     * and the callback function will free URB                      *
     * buf0[0] is used to receive the return value by device        *
     ****************************************************************/
    
    buf0[0] = 99; //this buf is used to receive the return value by device
    usb_fill_control_urb(urb0,udev, usb_rcvctrlpipe(udev,0),ctrl_buf,buf0,18,simple_callback1,NULL);
    int length;
    int a0 = usb_submit_urb(urb0);
    free(ctrl_buf);
    free(urb0);
    free(buf0);    
#endif      
    
#ifdef bulk_test
    /****************************************************************
     * allocate the parameter of bulk URB,                          *
     * call usb_submit_urb to transmit data                         *
     * if the transmission complete, the return value will be zero, *
     * and the callback function will free URB                      *
     * buf[0] is the value we try to send to device, and we expect  *
     * device will send this back later.                            *
     ****************************************************************/
    struct urb *urb; 
    struct urb *urb2; 
    
    urb  = usb_alloc_urb(0);  
    urb2 = usb_alloc_urb(0);

    u8 *buf  =  RtlZmalloc(16);
    u8 *buf2 =  RtlZmalloc(16);
    
    buf[0] = 102;
    usb_fill_bulk_urb(urb, udev_control, usb_sndbulkpipe(udev_control,dev_ctrl->out_pipe_addr), buf, 16, simple_callback,NULL);
    int a1 = usb_submit_urb(urb);
#if defined (CONFIG_PLATFORM_8721D)
    vTaskDelay(10);
#endif
    printf("a1 = %d,act_LEN=%d\n",a1,urb->actual_length);
    
   /*********************************************
    * Print the result                           *
    * buf2[0] should be buf[0]+1                 *
    **********************************************/ 

 
     /****************************************************************
     * allocate the parameter of control URB,                        *
     * call usb_submit_urb to receive data                           *
     * if the transmission complete, the return value will be zero,  *
     * and the callback function will free URB                       *           
     * buf2 is the value device try to send back, the value should be*
     * incremented 1                                                 *
     *****************************************************************/

    buf2[0] = 100;
    usb_fill_bulk_urb(urb2, udev_control, usb_rcvbulkpipe(udev_control,dev_ctrl->in_pipe_addr), buf2, 16, simple_callback,NULL);
    int a2 = usb_submit_urb(urb2);
#if defined (CONFIG_PLATFORM_8721D)
    vTaskDelay(10);
#endif
    printf("a2 = %d,act_LEN=%d\n",a2,urb2->actual_length);
    printf("before/after increment =%d,%d\n",buf[0],buf2[0]);
    
    free(urb);
    free(urb2);
    free(buf);
    free(buf2);
#endif 
    vTaskDelay(300);
#ifdef  iso_test_in
    unsigned		i, maxp;
    unsigned int in_npackets = 1 ;
    u16 psize;
    psize = vs_endpoint_max_bpi(udev_control,dev_ctrl->iso_in);
    CallbackBit = 1;
    iso_in_buffer = malloc(in_npackets  * psize);
    for (i = 0 ; i < in_npackets * psize ;i++){
        iso_in_buffer[i] = 0xff;
    }
    
    for (i = 0; i < 1; ++i) {
        urb_iso_in[i] = usb_alloc_urb(in_npackets);
    
        urb_iso_in[i]->dev = udev_control;
        urb_iso_in[i]->pipe = usb_rcvisocpipe(udev_control,dev_ctrl->in_iso_addr);//dev_ctrl->in_iso_pipe;//
        urb_iso_in[i]->transfer_flags = URB_ISO_ASAP;//  | URB_NO_TRANSFER_DMA_MAP;
        urb_iso_in[i]->interval = dev_ctrl->iso_out->bInterval;
        urb_iso_in[i]->transfer_buffer = iso_in_buffer;
            if (!urb_iso_in[i]->transfer_buffer) {
               printf("error in urb->transfer_buffer\n");
            }
        urb_iso_in[i]->complete = complicated_callback;
        urb_iso_in[i]->context = NULL;
        urb_iso_in[i]->number_of_packets =in_npackets ;
        urb_iso_in[i]->transfer_buffer_length = in_npackets * psize;//12240;

        for (j = 0; j < in_npackets; ++j) {
                urb_iso_in[i]->iso_frame_desc[j].offset = j * psize;
                urb_iso_in[i]->iso_frame_desc[j].length = psize;
        }
    }
    
#if 1
    for (i = 0; i < 1; ++i){ 
        int status_iso = usb_submit_urb(urb_iso_in[i]);
    }
#endif

#endif
    vTaskDelay(5);
#ifdef  iso_test_out
    unsigned packets;
    unsigned int npackets = 1 ;
    int i2;
    u16  psize2 = 64;//vs_endpoint_max_bpi(udev_control,dev_ctrl->iso_out);
    iso_out_buffer = malloc(npackets * psize2);
    memset(iso_out_buffer, out_number_++, npackets * psize2);
    
    for (i2 = 0; i2 < 1; ++i2) {
        urb_iso_out[i2] = usb_alloc_urb(npackets);
    
        urb_iso_out[i2]->dev = udev_control;
        urb_iso_out[i2]->pipe =usb_sndisocpipe(udev_control,dev_ctrl->out_iso_addr);
        urb_iso_out[i2]->transfer_flags = URB_ISO_ASAP;//  | URB_NO_TRANSFER_DMA_MAP;
        urb_iso_out[i2]->interval = dev_ctrl->iso_out->bInterval;
        urb_iso_out[i2]->transfer_buffer = iso_out_buffer;
            if (!urb_iso_out[i2]->transfer_buffer) {
               printf("error in urb->transfer_buffer\n");
            }
        urb_iso_out[i2]->complete = complicated_callback_out;
        urb_iso_out[i2]->context = NULL;
        urb_iso_out[i2]->number_of_packets = npackets;
        urb_iso_out[i2]->transfer_buffer_length = npackets * psize2;

        for (j = 0; j < npackets; ++j) {
                urb_iso_out[i2]->iso_frame_desc[j].offset = j * psize2;
                urb_iso_out[i2]->iso_frame_desc[j].length = psize2;
        }
    }

    for (i2 = 0; i2 < 1; ++i2){ 
        //timer_old = xTaskGetTickCount();
        int status_iso = usb_submit_urb(urb_iso_out[i2]);
    } 
#endif
    return 0;
} 


 static u8 vc_probe_owner_cnt = 0;
 static _Mutex vc_probe_owner_lock = NULL;
 void vendor_specific_probe_thread(void *param)
 {
	 struct usb_interface *intf   = (struct usb_interface *)param;
         intf_for_free = (struct usb_interface *)param;
	 if((intf != NULL) && (intf->cur_altsetting->desc.bInterfaceClass == USB_CLASS_VENDOR_SPEC) )//&& (intf->cur_altsetting->desc.bInterfaceSubClass == UVC_VC_HEADER))
#if 0
		 vc_probe_owner_cnt++;
	 else
		 goto exit;
	 if(vc_probe_owner_lock == NULL)
		 RtlMutexInit(&vc_probe_owner_lock);
	 		RtlDownMutex(&vc_probe_owner_lock);
	 			vs_probe(intf);	 
		    RtlUpMutex(&vc_probe_owner_lock);
	 	 vc_probe_owner_cnt--;
	 if((vc_probe_owner_lock != NULL) && (vc_probe_owner_cnt == 0))
		 RtlMutexFree(&vc_probe_owner_lock);
 exit:	 
#endif
   {
         vs_probe(intf);
	 vTaskDelete(NULL);	 
   }
 }


 int vendor_specific_probe_entry(struct usb_interface *intf)
 {
		 if(xTaskCreate(vendor_specific_probe_thread, ((const signed char*)"vs_prb_thread"), 2048, intf, VC_TASK_PRIORITY, NULL) != pdPASS) {
			 printf("\r\n vs_probe_thread: Create Task Error\n");
			 return -1;
		 }
		 return 0;
 }
 
 //==============================ATCMD===========================
#include "at_cmd/log_service.h" 
#include "usb.h"
#include "example_vendor_specific.h"

#if 1
void fATU0(void *arg)
{   
    vs_free();
    printf("\n\r call vs free  %d\n\r", xPortGetFreeHeapSize());
}

void fATU1(void *arg)
{   
    _usb_deinit();
    printf("\n\r call usb deinit  %d\n\r", xPortGetFreeHeapSize());
}

void fATU2(void *arg)
{   
    example_vendor_specific();
    printf("\n\r call example_vendor_specific %d\n\r", xPortGetFreeHeapSize());
}

log_item_t at_usb_items[ ] = {
	{"ATU0", fATU0,}, // vs_free
        {"ATU1", fATU1,},//_usb_deinit
        {"ATU2", fATU2,},//example_vendor_specific
};

void at_usb_host_init(void)
{
        //printf("at_usb_host_init\n");
	log_service_add_table(at_usb_items, sizeof(at_usb_items)/sizeof(at_usb_items[0]));
}

#if SUPPORT_LOG_SERVICE
log_module_init(at_usb_host_init);
#endif

#endif
 
 
