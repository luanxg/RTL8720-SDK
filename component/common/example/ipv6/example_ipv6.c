#include "FreeRTOS.h"
#include "task.h"
#include <platform/platform_stdlib.h>
#include <lwip/sockets.h>
#include <ipv6/example_ipv6.h>
#include <lwip_netconf.h>
#include <lwip/netif.h>
#include <wifi/wifi_ind.h>

#if CONFIG_EXAMPLE_IPV6

extern struct netif xnetif[];
xSemaphoreHandle ipv6_semaphore;

static void example_ipv6_udp_server(void)
{
    int                   server_fd;
    struct sockaddr_in6   ser_addr , client_addr;

    int addrlen = sizeof(struct sockaddr_in6);

    char send_data[MAX_SEND_SIZE] = "Hi client!";
    char recv_data[MAX_RECV_SIZE];

    //create socket
    if((server_fd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        printf("\n\r[ERROR] Create socket failed\n");
        return;
    }
    printf("\n\r[INFO] Create socket successfully\n");

    //initialize structure dest
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin6_family = AF_INET6;
    ser_addr.sin6_port = htons(UDP_SERVER_PORT);
    ser_addr.sin6_addr = (struct in6_addr) IN6ADDR_ANY_INIT;

    //Assign a port number to socket
    if(bind(server_fd, (struct sockaddr*)&ser_addr, sizeof(ser_addr)) != 0){
        printf("\n\r[ERROR] Bind socket failed\n");
        closesocket(server_fd);
        return;
    }
    printf("\n\r[INFO] Bind socket successfully\n");

    while(1){
        memset(recv_data, 0, MAX_RECV_SIZE);
        if(recvfrom(server_fd, recv_data, MAX_RECV_SIZE, 0, (struct sockaddr *)&client_addr, &addrlen) > 0){
            printf("\n\r[INFO] Receive data : %s\n",recv_data);
            //Send Response
            if(sendto(server_fd, send_data, MAX_SEND_SIZE, 0, (struct sockaddr *)&client_addr, addrlen) == -1){
                printf("\n\r[ERROR] Send data failed\n");
            }
            else{
                printf("\n\r[INFO] Send data successfully\n");
            }
        }
    }

    closesocket(server_fd);

    return;
}

static void example_ipv6_udp_client(void)
{
    int                 client_fd;
    struct sockaddr_in6 ser_addr;

    int addrlen = sizeof(struct sockaddr_in6);

    char recv_data[MAX_RECV_SIZE];
    char send_data[MAX_SEND_SIZE] = "Hi Server!!";

    //create socket
    if((client_fd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        printf("\n\r[ERROR] Create socket failed\n");
        return;
    }
    printf("\n\r[INFO] Create socket successfully\n");

    //initialize value in dest
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin6_family = AF_INET6;
    ser_addr.sin6_port = htons(UDP_SERVER_PORT);
    inet_pton(AF_INET6, UDP_SERVER_IP, &(ser_addr.sin6_addr));

    while(1){
        //Send data to server
        if(sendto(client_fd, send_data, MAX_SEND_SIZE, 0, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) == -1){
            printf("\n\r[ERROR] Send data failed\n");
        }
        else{
            printf("\n\r[INFO] Send data to server successfully\n");
        }

        //Receive data from server response
        memset(recv_data, 0, MAX_RECV_SIZE);
        if(recvfrom(client_fd, recv_data, MAX_RECV_SIZE, 0, (struct sockaddr *)&ser_addr, &addrlen) <= 0){
            //printf("\n\r[ERROR] Receive data failed\n");
        }
        else{
            printf("\n\r[INFO] Receive from server: %s\n", recv_data);
        }
        vTaskDelay(1000);
    }

    closesocket(client_fd);

    return;
}

static void example_ipv6_tcp_server(void)
{
    int                   server_fd, client_fd;
    struct sockaddr_in6   ser_addr , client_addr;

    int addrlen = sizeof(struct sockaddr_in6);

    char send_data[MAX_SEND_SIZE] = "Hi client!!";
    char recv_data[MAX_RECV_SIZE];

    //create socket
    if((server_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) == -1){
        printf("\n\r[ERROR] Create socket failed\n");
        return;
    }
    printf("\n\r[INFO] Create socket successfully\n");

    //initialize structure dest
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin6_family = AF_INET6;
    ser_addr.sin6_port = htons(TCP_SERVER_PORT);
    ser_addr.sin6_addr = (struct in6_addr) IN6ADDR_ANY_INIT;

    //Assign a port number to socket
    if(bind(server_fd, (struct sockaddr*)&ser_addr, sizeof(ser_addr)) != 0){
        printf("\n\r[ERROR] Bind socket failed\n");
        closesocket(server_fd);
        return;
    }
    printf("\n\r[INFO] Bind socket successfully\n");

    //Make it listen to socket with max 20 connections
    if(listen(server_fd, 20) != 0){
        printf("\n\r[ERROR] Listen socket failed\n");
        closesocket(server_fd);
        return;
    }
    printf("\n\r[INFO] Listen socket successfully\n");

    //Accept
    if((client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addrlen)) == -1){
        printf("\n\r[ERROR] Accept connection failed\n");
        closesocket(server_fd);
        closesocket(client_fd);
        return;
    }
    printf("\n\r[INFO] Accept connection successfully\n");

    while(1){
        memset(recv_data, 0, MAX_RECV_SIZE);
        if(recv(client_fd, recv_data, MAX_RECV_SIZE, 0) > 0){
            printf("\n\r[INFO] Receive data : %s\n",recv_data);
            //Send Response
            if(send(client_fd, send_data, MAX_SEND_SIZE, 0) == -1){
                printf("\n\r[ERROR] Send data failed\n");
            }
            else{
                printf("\n\r[INFO] Send data successfully\n");
            }
        }
    }
    closesocket(client_fd);
    closesocket(server_fd);
    return;
}

static void example_ipv6_tcp_client(void)
{
    int                 client_fd;
    struct sockaddr_in6 ser_addr;

    int addrlen = sizeof(struct sockaddr_in6);

    char recv_data[MAX_RECV_SIZE];
    char send_data[MAX_SEND_SIZE] = "Hi Server!!";

    //create socket
    if((client_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) ==  -1){
        printf("\n\r[ERROR] Create socket failed\n");
        return;
    }
    printf("\n\r[INFO] Create socket successfully\n");

    //initialize value in dest
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin6_family = AF_INET6;
    ser_addr.sin6_port = htons(TCP_SERVER_PORT);
    inet_pton(AF_INET6, TCP_SERVER_IP, &(ser_addr.sin6_addr));

    //Connecting to server
    if(connect(client_fd, (struct sockaddr*)&ser_addr, sizeof(ser_addr)) == -1){
        printf("\n\r[ERROR] Connect to server failed\n");
    }
    printf("[INFO] Connect to server successfully\n");

    while(1){
        //Send data to server
        if(send(client_fd, send_data, MAX_SEND_SIZE,0) == -1){
            printf("\n\r[ERROR] Send data failed\n");
        }
        else{
            printf("\n\r[INFO] Send data to server successfully\n");
        }

        //Receive data from server response
        if(recv(client_fd, recv_data, MAX_RECV_SIZE, 0) <= 0){
            //printf("\n\r[ERROR] Receive data failed\n");
        }
        else{
            printf("\n\r[INFO] Receive from server: %s\n", recv_data);
        }
        vTaskDelay(1000);
    }
    closesocket(client_fd);
    return;
}

static void example_ipv6_mcast_server()
{
    int                   server_fd;
    struct sockaddr_in6   ser_addr , client_addr;

    int addrlen = sizeof(struct sockaddr_in6);

    char send_data[MAX_SEND_SIZE] = "Hi client!";
    char recv_data[MAX_RECV_SIZE];

    //Register to multicast group membership
    ip6_addr_t mcast_addr;
    inet_pton(AF_INET6, MCAST_GROUP_IP, &(mcast_addr.addr));
    if(mld6_joingroup(IP6_ADDR_ANY , &mcast_addr) != 0){
        printf("\n\r[ERROR] Register to ipv6 multicast group failed\n");
    }

    //create socket
    if((server_fd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        printf("\n\r[ERROR] Create socket failed\n");
        return;
    }
    printf("\n\r[INFO] Create socket successfully\n");

    //initialize structure dest
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin6_family = AF_INET6;
    ser_addr.sin6_port = htons(MCAST_GROUP_PORT);
    ser_addr.sin6_addr = (struct in6_addr) IN6ADDR_ANY_INIT;

    //Assign a port number to socket
    if(bind(server_fd, (struct sockaddr*)&ser_addr, sizeof(ser_addr)) != 0){
        printf("\n\r[ERROR] Bind socket failed\n");
        closesocket(server_fd);
        return;
    }
    printf("\n\r[INFO] Bind socket successfully\n");

    while(1){
        memset(recv_data, 0, MAX_RECV_SIZE);
        if(recvfrom(server_fd, recv_data, MAX_RECV_SIZE, 0, (struct sockaddr *)&client_addr, &addrlen) > 0){
            printf("\n\r[INFO] Receive data : %s\n",recv_data);
            //Send Response
            if(sendto(server_fd, send_data, MAX_SEND_SIZE, 0, (struct sockaddr *)&client_addr, addrlen) == -1){
                printf("\n\r[ERROR] Send data failed\n");
            }
            else{
                printf("\n\r[INFO] Send data successfully\n");
            }
        }
    }
    closesocket(server_fd);
    return;
}

static void example_ipv6_mcast_client(void)
{
    int                 client_fd;
    struct sockaddr_in6 ser_addr;

    int addrlen = sizeof(struct sockaddr_in6);

    char recv_data[MAX_RECV_SIZE];
    char send_data[MAX_SEND_SIZE] = "Hi Server!!";

    //create socket
    if((client_fd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) ==  -1){
        printf("\n\r[ERROR] Create socket failed\n");
        return;
    }
    printf("\n\r[INFO] Create socket successfully\n");

    //initialize value in dest
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin6_family = AF_INET6;
    ser_addr.sin6_port = htons(MCAST_GROUP_PORT);
    inet_pton(AF_INET6, MCAST_GROUP_IP, &(ser_addr.sin6_addr));

    while(1){
        //Send data to server
        if(sendto(client_fd, send_data, MAX_SEND_SIZE, 0, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) == -1){
            printf("\n\r[ERROR] Send data failed\n");
        }
        else{
            printf("\n\r[INFO] Send data to server successfully\n");
        }

        //Receive data from server response
        memset(recv_data, 0, MAX_RECV_SIZE);
        if(recvfrom(client_fd, recv_data, MAX_RECV_SIZE, 0, (struct sockaddr *)&ser_addr, &addrlen) <= 0){
            //printf("\n\r[ERROR] Receive data failed\n");
        }
        else{
            printf("\n\r[INFO] Receive from server: %s\n", recv_data);
        }
        vTaskDelay(1000);
    }
    closesocket(client_fd);
    return;
}

static void example_ipv6_thread(void *param)
{
    wifi_reg_event_handler(WIFI_EVENT_CONNECT, example_ipv6_callback, NULL);
    rtw_init_sema(&ipv6_semaphore, 0);
    if(!ipv6_semaphore){
        vTaskDelete(NULL);
        return;
    }
    if(rtw_down_timeout_sema(&ipv6_semaphore, IPV6_SEMA_TIMEOUT) == RTW_FALSE) {
        rtw_free_sema(&ipv6_semaphore);
    }
    LwIP_AUTOIP_IPv6(&xnetif[0]);
    //Wait for ipv6 addr process conflict done
    while(!ip6_addr_isvalid(netif_ip6_addr_state(&xnetif[0],0)));

/***---open a example service once!!---***/
    //example_ipv6_udp_server();
    //example_ipv6_tcp_server();
    //example_ipv6_mcast_server();

    //example_ipv6_udp_client();
    //example_ipv6_tcp_client();
    example_ipv6_mcast_client();

    vTaskDelete(NULL);
}

//Wait for wifi association done callback
void example_ipv6_callback(char* buf, int buf_len, int flags, void* userdata)
{
    rtw_up_sema(&ipv6_semaphore);
}

void example_ipv6(void)
{
    if(xTaskCreate(example_ipv6_thread, ((const char*)"example_ipv6_thread"), 1024, NULL, tskIDLE_PRIORITY + 1 + PRIORITIE_OFFSET, NULL) != pdPASS)
        printf("\n\r%s xTaskCreate(example_ipv6_thread) failed", __FUNCTION__);
}

#endif // #if CONFIG_EXAMPLE_IPV6
