#include "usb.h"
#include "vs_intf.h"

int vendor_specific_probe_entry(struct usb_interface *intf);
extern int isoout_complicated_callback_conter_;
extern int out_number_;
extern int in_count_check;
extern int in_count;
extern int CallbackBit;
extern int DumpInArray[IN_STORAGE_SIZE];
extern int usb_connect_state;//define connecting state, 1:connecting 0:disconnect
extern struct usbtest_dev *my_dev;
extern struct usbtest_dev *dev_ctrl;
extern struct usb_device *udev_control;
extern struct usb_interface *intf_for_free;

#ifdef  iso_test_in
extern struct urb *urb_iso_in[2];
extern u8 *iso_in_buffer;
#endif
#ifdef  iso_test_out
extern struct urb *urb_iso_out[2];
extern u8 *iso_out_buffer;
#endif

static int free_endpoint(struct usb_device *udev, struct usb_interface *intf)
{
      unsigned ep;
      struct usb_host_interface *alt;
      alt = intf->altsetting;
      for (ep = 0; ep < alt->desc.bNumEndpoints; ep++) {
          struct usb_host_endpoint *e = &alt->endpoint[ep];
          usb_hcd_disable_endpoint(udev,e);
      }
      return 0;
}

void vendor_specific_disconnect(struct usb_interface *intf)
{

if(usb_connect_state == 1){
usb_connect_state = 0;//set usb_connect_state to disconnect state 0
#if 1
    isoout_complicated_callback_conter_ = 0;
    out_number_ = 0x00;
    in_count    = 0;
    CallbackBit = 1;
    in_count_check = 0;
//#if 0
    if(my_dev != NULL){
       free(my_dev);
    }
    if(dev_ctrl != NULL){
       free(dev_ctrl);
    }
    //printf("before free_endpoint\n");
    if(udev_control != NULL){
       free_endpoint(udev_control,intf_for_free);
    }
    //printf("after free_endpoint\n");
    if(udev_control != NULL){
       free(udev_control);
    }
#ifdef  iso_test_in
    free(iso_in_buffer);
    usb_free_urb(urb_iso_in[0]);
#endif
#ifdef  iso_test_out
    //printf("---4 disconnect ---\n");
    if (iso_out_buffer != NULL){
        free(iso_out_buffer);
    }
    //printf("---4.1 disconnect ---\n");
    if(urb_iso_out[0] != NULL){
        usb_free_urb(urb_iso_out[0]);
    }
    //printf("---end free\n ---\n");
#endif
#endif
    printf("\n vendor_specific_disconnect().\n");
    printf("\n\rafter disconnect heap %d\n\r", xPortGetFreeHeapSize());
    int j;
#if 0
    for(j=0;j<IN_STORAGE_SIZE;j++){
      printf("DumpInArray[%d] = %d\n",j,DumpInArray[j]);
    }
#endif
    }
}



static struct usb_driver usb_vs_driver =
{
    .name = "vendor_specific probe",
    .probe = vendor_specific_probe_entry,
    .disconnect = vendor_specific_disconnect,
};

void vs_free(void)
{
    printf("\n\rbefore VS free heap %d\n\r", xPortGetFreeHeapSize());
    if(usb_connect_state == 1){
usb_connect_state = 0;//set usb_connect_state to disconnect state 0
#if 1
    isoout_complicated_callback_conter_ = 0;
    out_number_ = 0x00;
    in_count    = 0;
    CallbackBit = 1;
    in_count_check = 0;
//#if 0
    if(my_dev != NULL){
       free(my_dev);
    }
    if(dev_ctrl != NULL){
       free(dev_ctrl);
    }
    //printf("before free_endpoint\n");
    if(udev_control != NULL){
       free_endpoint(udev_control,intf_for_free);
    }
    //printf("after free_endpoint\n");
    if(udev_control != NULL){
       free(udev_control);
    }
#ifdef  iso_test_in
    free(iso_in_buffer);
    usb_free_urb(urb_iso_in[0]);
#endif
#ifdef  iso_test_out
    //printf("---4 disconnect ---\n");
    if (iso_out_buffer != NULL){
        free(iso_out_buffer);
    }
    //printf("---4.1 disconnect ---\n");
    if(urb_iso_out[0] != NULL){
        usb_free_urb(urb_iso_out[0]);
    }
    //printf("---end free\n ---\n");
#endif
#endif

    int j;
#if 0
    for(j=0;j<IN_STORAGE_SIZE;j++){
      printf("DumpInArray[%d] = %d\n",j,DumpInArray[j]);
    }
#endif
    }
    printf("\n end of VC free.\n");
    printf("\n\rafter VC free heap %d\n\r", xPortGetFreeHeapSize());
    usb_unregister_class_driver(&usb_vs_driver);       
}

int vs_init(void)
{
        int ret = 0;
/* vs register & probe here */
        ret = usb_register_class_driver(&usb_vs_driver);
        //printf("register usb VC\n");
        if(ret < 0) 
              goto error;

        return ret;
error:
        vs_free();
        return ret;
}






