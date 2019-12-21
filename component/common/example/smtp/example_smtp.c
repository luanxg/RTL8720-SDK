#include "FreeRTOS.h"
#include "task.h"
#include <platform_stdlib.h>
#include <lwip_netconf.h>

#include "platform_opts.h"

//Reminder: Please define POLARSSL_CERTS_C or MBEDTLS_CERTS_C in congig_rsa.h before running the example.

#define SECURE_MODE 1

static void example_smtp_thread()
{
#if SECURE_MODE
    /*=================================== Secure mode ===================================*/
  char *argv[13];

  argv[1] = "server_name=smtp.gmail.com"; //e.g. smtp.gmail.com, smtp.mail.yahoo.com
  argv[2] = "server_port=587"; //check the port according to smtp sever, usually 587(STARTTLS) or 465(SSL/TLS)
  argv[3] = "debug_level=1";
  argv[4] = "authentication=1";
  argv[5] = "mode=1"; //STARTTLS: 1  SSL/TLS: 0
  argv[6] = "user_name=sender@gmail.com";
  argv[7] = "user_pwd=password";
  argv[8] = "mail_from=sender@gmail.com";
  argv[9] = "mail_to=receiver@gmail.com";
  argv[10] = "mail_subject=Test mail";
  argv[11] = "mail_body=Hello, this is written by user!\r\nEnjoy!";
  argv[12] = "force_ciphersuite=";//argv[12] = "force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA"; 
  
                                    /*TLS-RSA-WITH-AES-256-CBC-SHA256
                                      TLS-RSA-WITH-AES-256-CBC-SHA
                                      TLS-RSA-WITH-AES-128-CBC-SHA256
                                      TLS-RSA-WITH-AES-128-CBC-SHA
                                      TLS-RSA-WITH-RC4-128-SHA
                                      TLS-RSA-WITH-RC4-128-MD5
                                    */

  printf("Wait 5 seconds for Wifi initinitalised \r\n");
  
  vTaskDelay(5000);
  
  printf("ssl smtp example starts\r\n");
  
  ssl_mail_client(13,argv);

  printf("ssl smtp example done\r\n");
  
#else
  /*=================================== Unsecure mode ===================================*/
  
  //For unsecure mode debug, please turn on LWIP_DEBUG.
  char *argv[9];

  argv[1] = "server_name=smtp.sina.com";
  argv[2] = "user_name=sender@sina.com";
  argv[3] = "user_pwd=password";
  argv[4] = "mail_from=sender@sina.com";
  argv[5] = "mail_to=receiver@sina.com";
  argv[6] = "mail_subject=Test mail";
  argv[7] = "mail_body=Hello, this is written by user!\r\nEnjoy!";
     
  printf("Wait 5 seconds for Wifi initinitalised \r\n");
  
  vTaskDelay(5000);
  
  printf("smtp example starts\r\n");
  mail_client(8,argv);
#endif
  
exit:
  vTaskDelete(NULL);
}

void example_smtp(void)
{
  if(xTaskCreate(example_smtp_thread, ((const char*)"example_smtp_thread"), 2048, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
    printf("\n\r%s xTaskCreate(init_thread) failed", __FUNCTION__);
}
