#include "FreeRTOS.h"
#include "task.h"
#include "platform_stdlib.h"
#include "osdep_service.h"
#include "google_nest.h"

#if (GOOGLENEST_USE_TLS == GOOGLENEST_TLS_POLARSSL)
#include <polarssl/net.h>
#include <polarssl/ssl.h>
#include <polarssl/error.h>
#include <polarssl/memory.h>

struct gn_tls{
	ssl_context ctx; 
};

#elif (GOOGLENEST_USE_TLS == GOOGLENEST_TLS_MBEDTLS)
#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/error.h"

struct gn_tls{
	mbedtls_ssl_context ctx; 
	mbedtls_ssl_config conf;
	mbedtls_net_context socket;	
};

static void* _calloc_func(size_t nmemb, size_t size) {
	size_t mem_size;
	void *ptr = NULL;

	mem_size = nmemb * size;
	ptr = pvPortMalloc(mem_size);

	if(ptr)
		memset(ptr, 0, mem_size);

	return ptr;
}

static char *gn_itoa(int value){
	char *val_str;
	int tmp = value, len = 1;

	while((tmp /= 10) > 0)
		len ++;

	val_str = (char *) pvPortMalloc(len + 1);
	sprintf(val_str, "%d", value);

	return val_str;
}
#endif /* GOOGLENEST_USE_TLS */


static int _random_func(void *p_rng, unsigned char *output, unsigned int output_len) {
	rtw_get_random_bytes(output, output_len);
	return 0;
}

extern int mbedtls_platform_set_calloc_free( void * (*calloc_func)( size_t, size_t ), void (*free_func)( void * ) );
void *gn_tls_connect(int *sock , char *host, int port){
#if (GOOGLENEST_USE_TLS == GOOGLENEST_TLS_POLARSSL)
	int ret;
	struct gn_tls *tls =NULL;

	memory_set_own(pvPortMalloc, vPortFree);
	tls = (struct gn_tls *) malloc(sizeof(struct gn_tls));

	if(tls){
		ssl_context *ssl = &tls->ctx;
		memset(tls, 0, sizeof(struct gn_tls));

		if((ret = net_connect(sock, host, port)) != 0) {
			printf("\n[GOOGLENEST] ERROR: net_connect %d\n", ret);
			goto exit;
		}

		if((ret = ssl_init(ssl)) != 0) {
			printf("\n[GOOGLENEST] ERROR: ssl_init %d\n", ret);
			goto exit;
		}

		ssl_set_endpoint(ssl, SSL_IS_CLIENT);
		ssl_set_authmode(ssl, SSL_VERIFY_NONE);
		ssl_set_rng(ssl, _random_func, NULL);
		ssl_set_bio(ssl, net_recv, sock, net_send, sock);
		
		if((ret = ssl_handshake(ssl)) != 0) {
			printf("\n[GOOGLENEST] ERROR: ssl_handshake -0x%x\n", -ret);
			goto exit;
		}
	}
	else{
		printf("\n[GOOGLENEST] ERROR: malloc\n");
		ret = -1;
		goto exit;
	}
exit:
	if(ret && tls) {
		net_close(*sock);
		ssl_free(&tls->ctx);
		free(tls);
		tls = NULL;
	}
	return (void *) tls;

#elif (GOOGLENEST_USE_TLS == GOOGLENEST_TLS_MBEDTLS)
	int ret;
	struct gn_tls *tls =NULL;

	mbedtls_platform_set_calloc_free(_calloc_func, vPortFree);
	tls = (struct gn_tls *) malloc(sizeof(struct gn_tls));

	if(tls){
		mbedtls_ssl_context *ssl = &tls->ctx;
		mbedtls_ssl_config *conf = &tls->conf;
		mbedtls_net_context *server_fd = &tls->socket;
		memset(tls, 0, sizeof(struct gn_tls));

		server_fd->fd = *sock;
		char *port_str = gn_itoa(port);

		if((ret = mbedtls_net_connect(server_fd, host, port_str, MBEDTLS_NET_PROTO_TCP)) != 0) {
			printf("\n[GOOGLENEST] ERROR: net_connect %d\n", ret);
			goto exit;
		}

		free(port_str);
		*sock = server_fd->fd;
		mbedtls_ssl_init(ssl);
		mbedtls_ssl_config_init(conf);
		mbedtls_ssl_set_bio(ssl, server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

		if((ret = mbedtls_ssl_config_defaults(conf,
				MBEDTLS_SSL_IS_CLIENT,
				MBEDTLS_SSL_TRANSPORT_STREAM,
				MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {

			printf("\n[GOOGLENEST] ERROR: ssl_config %d\n", ret);
			goto exit;
		}

		mbedtls_ssl_conf_authmode(conf, MBEDTLS_SSL_VERIFY_NONE);
		mbedtls_ssl_conf_rng(conf, _random_func, NULL);

		if((ret = mbedtls_ssl_setup(ssl, conf)) != 0) {
			printf("\n[GOOGLENEST] ERROR: ssl_setup %d\n", ret);
			goto exit;
		}
		if((ret = mbedtls_ssl_handshake(ssl)) != 0) {
			printf("\n[GOOGLENEST] ERROR: ssl_handshake -0x%x\n", -ret);
			goto exit;
		}
	}
	else {
		printf("\n[GOOGLENEST] ERROR: malloc\n");
		ret = -1;
		goto exit;
	}
exit:
	if(ret && tls){
		mbedtls_net_free(&tls->socket);
		mbedtls_ssl_free(&tls->ctx);
		mbedtls_ssl_config_free(&tls->conf);
		free(tls);
		tls = NULL;
	}
	return (void *) tls;
#endif /* GOOGLENEST_USE_TLS */
}

void gn_tls_close(void *tls_in,int *sock){
	struct gn_tls *tls = (struct gn_tls *)tls_in;

#if (GOOGLENEST_USE_TLS == GOOGLENEST_TLS_POLARSSL)
	if(tls)
 		ssl_close_notify(&tls->ctx);

	if(*sock != -1){
		net_close(*sock);
		*sock = -1;
	}
	ssl_free(&tls->ctx);  
	free(tls);
	tls = NULL;
#elif (GOOGLENEST_USE_TLS == GOOGLENEST_TLS_MBEDTLS)
	if(tls)
		mbedtls_ssl_close_notify(&tls->ctx);
        
	if(*sock != -1){
		mbedtls_net_free(&tls->socket);
		*sock = -1;
	}
	mbedtls_ssl_free(&tls->ctx);
	mbedtls_ssl_config_free(&tls->conf);	
	free(tls);
	tls = NULL;	
#endif /* GOOGLENEST_USE_TLS */
}

int gn_tls_write(void *tls_in, char *request, int request_len){
	struct gn_tls *tls = (struct gn_tls *)tls_in;
#if (GOOGLENEST_USE_TLS == GOOGLENEST_TLS_POLARSSL)
	return ssl_write(&tls->ctx, request, request_len);	
#elif (GOOGLENEST_USE_TLS == GOOGLENEST_TLS_MBEDTLS)	
	return mbedtls_ssl_write(&tls->ctx, (unsigned char const*)request, request_len);
#endif /* GOOGLENEST_USE_TLS */
}

int gn_tls_read(void *tls_in, char *buffer, int buf_len){
	struct gn_tls *tls = (struct gn_tls *)tls_in; 
#if (GOOGLENEST_USE_TLS == GOOGLENEST_TLS_POLARSSL)
	return ssl_read(&tls->ctx, buffer, buf_len);
#elif (GOOGLENEST_USE_TLS == GOOGLENEST_TLS_MBEDTLS)	
	return mbedtls_ssl_read(&tls->ctx, (unsigned char*)buffer, buf_len);
#endif /* GOOGLENEST_USE_TLS */
}