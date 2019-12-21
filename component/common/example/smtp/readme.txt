LWIP/SSL SMTP EXAMPLE

Description:
Send email from lwip/ssl smtp client.

Configuration:

Secure mode  #define SECURE_MODE 1
Modify  server_name, server_port, mode, user_name, user_pwd, mail_from, mail_to, mail_subject, mail_body, force_ciphersuite
accordingly in example_ssl_download.c

Unsecure mode #define SECURE_MODE 0
Modify  server_name, user_name, user_pwd, mail_from, mail_to, mail_subject, mail_body accordingly in example_ssl_download.c

[config_rsa.h]
	#define POLARSSL_CERTS_C
     or
        #define MBEDTLS_CERTS_C  
[platform_opts.h]
	#define CONFIG_EXAMPLE_SSL_SMTP    1

Execution:
Can make automatical Wi-Fi connection when booting by using wlan fast connect example.
A lwip/ssl smtp example thread will be started automatically when booting.

[Supported List]
	Source code not in project :
	    Ameba-1, Ameba-z, Ameba-pro