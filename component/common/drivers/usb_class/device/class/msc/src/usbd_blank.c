#include "msc/inc/usbd_blank_desc.h"
#include "msc/inc/usbd_blank.h"
#include "msc/inc/usbd_scsi.h"
#include "core/inc/usb_config.h"
#include "core/inc/usb_composite.h"
#include "osdep_service.h"

// chose physical hard disk type
#include "sd.h"

// Config use thread to write data
#define CONGIF_USE_DATA_THREAD	0

// define global variables
struct blank_dev gBLANK_DEVICE;
struct blank_common gBLANK_COMMON;


/* maxpacket and other transfer characteristics vary by speed. */
#define ep_desc(g,hs,fs) (((g)->speed==USB_SPEED_HIGH)?(hs):(fs))
#if defined(CONFIG_PLATFORM_8195BHP)
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




__weak
int fun_set_alt_callback(struct usb_function *f, unsigned interface, unsigned alt){

    printf("USB inside\n");

}

void fun_disable_blank (struct usb_function *f){
	struct blank_dev	*blankdev = CONTAINER_OF(f,struct blank_dev, func);
	struct blank_common *common = blankdev->common;
FUN_ENTER;
	if(common){
	// TODO:
	}
FUN_EXIT;
}

static int fun_bind(struct usb_configuration *c, struct usb_function *f){
	struct blank_dev	*blankdev = CONTAINER_OF(f,struct blank_dev, func);
	struct usb_ep *ep;

	int status = -ENODEV;
	int id;

FUN_ENTER;

	/* allocate instance-specific interface IDs, and patch descriptors */
	id = usb_interface_id(c, f); // this will return the allocated interface id 
	if (id < 0){
		status = id;
		goto fail;
	}
	usbd_blank_intf_desc.bInterfaceNumber = id;
	blankdev->interface_number = id; // record interface number

	/* search endpoint according to endpoint descriptor and modify endpoint address*/
	ep = usb_ep_autoconfig(c->cdev->gadget, &usbd_blank_source_desc_FS);
	if (!ep)
		goto fail;
	ep->driver_data = blankdev;/* claim */
	blankdev->in_ep = ep;

	ep = usb_ep_autoconfig(c->cdev->gadget, &usbd_blank_sink_desc_FS);
	if (!ep)
		goto fail;
	ep->driver_data = blankdev;/* claim */
	blankdev->out_ep = ep;

	usbd_blank_source_desc_HS.bEndpointAddress =
			usbd_blank_source_desc_FS.bEndpointAddress;// modified after calling usb_ep_autoconfig()
	usbd_blank_sink_desc_HS.bEndpointAddress =
			usbd_blank_sink_desc_FS.bEndpointAddress;

	status = usb_assign_descriptors(f, usbd_blank_descriptors_FS,
			usbd_blank_descriptors_HS, NULL);

	if (status)
		goto fail;
FUN_EXIT;
	return 0;
FUN_EXIT;
fail:
	usb_free_all_descriptors(f);

	return status;
}

void fun_unbind_blank(struct usb_configuration *c, struct usb_function *f){
	struct blank_dev *blankdev = CONTAINER_OF(f,struct blank_dev, func);
FUN_ENTER;
	usb_free_all_descriptors(f);



FUN_EXIT;
}


int fun_setup_blank(struct usb_function *fun, const struct usb_ctrlrequest *ctrl){

}

void fun_free_blank(struct usb_function *f){
	
}

static int config_bind(struct usb_configuration *c){
	struct blank_dev *blankdev;
	struct blank_common *common;
	int status = 0;
FUN_ENTER;

	blankdev =(struct blank_dev *)c->cdev->gadget->device;
	common = blankdev->common;

	common->gadget = c->cdev->gadget;
	
	blankdev->func.name = "blank_function_sets";
	blankdev->func.bind = fun_bind;
	blankdev->func.unbind = fun_unbind_blank;
	blankdev->func.setup = fun_setup_blank;
	blankdev->func.set_alt = fun_set_alt_callback;
	blankdev->func.disable = fun_disable_blank;
	blankdev->func.free_func = fun_free_blank;
	
	status = usb_add_function(c, &blankdev->func);
	if(status)
		goto free_fun;

FUN_EXIT;
	return status;

free_fun:
FUN_EXIT;
	usb_put_function(&blankdev->func);
	return status;
}

void config_unbind_blank(struct usb_configuration *c){

FUN_ENTER;
	// TODO:
FUN_EXIT;
}


static struct usb_configuration usbd_blank_cfg_driver = {
	.label			= "usbd-blank",
	.unbind			= config_unbind_blank,
	.bConfigurationValue	= DEV_CONFIG_VALUE,
	.bmAttributes		= USB_CONFIG_ATT_ONE,// | USB_CONFIG_ATT_SELFPOWER,
};

int usbd_blank_bind(
    struct usb_composite_dev *cdev
)
{

    int status = 0;
	struct blank_dev *blankdev;
	struct blank_common *common;
	
FUN_ENTER;

	blankdev = &gBLANK_DEVICE;
	common = &gBLANK_COMMON;

	cdev->gadget->device = blankdev;

	/* link blank_dev and blank_common */
	blankdev->common = common;

	common->gadget = cdev->gadget;
	common->ep0 = cdev->gadget->ep0;
	common->req0 = cdev->req;

    /* register configuration(s)*/
	status = usb_add_config(cdev, &usbd_blank_cfg_driver,
			config_bind);
	if (status < 0)
		goto fail_set_luns;

FUN_EXIT;
	return 0;

fail_set_luns:

	
fail_alloc_buf:

FUN_EXIT;

    return status;
}

int usbd_blank_unbind(
    struct usb_composite_dev *cdev
)
{
	struct blank_dev *blankdev = (struct blank_dev*)cdev->gadget->device;
	struct blank_common *common;
FUN_ENTER;

	usb_put_function(&blankdev->func);


FUN_EXIT;
	return 0;
}

void usbd_blank_disconnect(struct usb_composite_dev *cdev){
	USBD_PRINTF("usbd_blank_disconnect\n");
}


struct usb_composite_driver     usbd_blank_driver = {
	.name 	    = "usbd_blank_driver",
	.dev		= &usbd_blank_device_desc,
	.strings 	= dev_blank_strings,
	.bind       = usbd_blank_bind,
	.unbind     = usbd_blank_unbind,
	.disconnect = usbd_blank_disconnect,
	.max_speed  = USB_SPEED_HIGH,
};


int usbd_blank_init(void){

	return usb_composite_probe(&usbd_blank_driver);
}


void usbd_blank_deinit(void){
	usb_composite_unregister(&usbd_blank_driver);
}

