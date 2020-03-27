#include "stdio.h"
#include <unistd.h>
// #include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include "joylink_log.h"
#include "joylink_softap.h"
#include "joylink_softap_start.h"
#include <rtconfig.h>
#include <rtdevice.h>

#define JOYLINK_LOCAL_UDP_PORT 4320
#define JOYLINK_REMOTE_UDP_PORT 9999

#define JOYLINK_TCP_PORT 3000

/* set the broadcast address for AP mode(192.168.169.1) */
#define JOYLINK_UDP_BROAD_IP  "192.168.169.255"

static int is_ap_start = 0;
static rt_sem_t ap_sem = RT_NULL;

/**
 * @name:joylink_udp_init 
 *
 * @param: port
 *
 * @returns:   
 */
static int joylink_udp_init(int port)
{
    int socket_fd = 0;

    struct sockaddr_in sin;

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(port);

    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

    int broadcastEnable = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_BROADCAST, (uint8_t *)&broadcastEnable, sizeof(broadcastEnable)) < 0){
        log_error("error: udp set broadcast error!\n");
        close(socket_fd);
        return -1;
    }

    if(0 > bind(socket_fd, (struct sockaddr *)&sin, sizeof(struct sockaddr))){
        log_error("error: udp bind socket error!\n");
        close(socket_fd);
        return -1;
    }
    
    /* for broadcast in AP mode */
    struct in_addr addr;
    addr.s_addr =inet_addr(DHCPD_SERVER_IP);
    log_info("soft ap ip %X\n", addr.s_addr);
    if(setsockopt(socket_fd, IPPROTO_IP, IP_MULTICAST_IF, &addr, sizeof(struct in_addr)))
    {
        close(socket_fd);
        return -1;      
    }
    
    log_info("udp init is ok\n");

    return socket_fd;
}

/**
 * @name:joylink_tcp_init 
 *
 * @param: port
 *
 * @returns:   
 */
static int joylink_tcp_init(int port)
{
    int socket_fd;

    struct sockaddr_in sin;

    memset(&sin, 0, sizeof(sin));

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(port);

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(0 > bind(socket_fd, (struct sockaddr *)&sin, sizeof(struct sockaddr))){
        log_error("error: tcp bind socket error!\n");
        close(socket_fd);
        return -1;
    }

    if(0 > listen(socket_fd, 0))
    {
        log_error("error: tcp listen error!\n");
        close(socket_fd);
        return -1;
    }
    log_info("tcp init is ok\n");

    return socket_fd;
}

static int udp_fd = -1;
static int tcp_fd = -1;
static int client_fd = -1;

int joylink_udp_broad_send(int socket_fd, char *buf, int len)
{
    int ret_len = 0;

    struct sockaddr_in temp_addr;
    socklen_t temp_size = sizeof(temp_addr);

    if(socket_fd != udp_fd){
        printf("joylink udp broad send error!\n");        
        return -1;
    }
    
    memset(&temp_addr, 0, sizeof(temp_addr));

    temp_addr.sin_family = AF_INET;
    temp_addr.sin_port = htons(JOYLINK_REMOTE_UDP_PORT);
    temp_addr.sin_addr.s_addr = inet_addr(JOYLINK_UDP_BROAD_IP);

    ret_len = sendto(socket_fd, buf, len, 0, (struct sockaddr *)&temp_addr, temp_size);
    log_info("joylink udp broad send data! fd: %d, len: %d\n", socket_fd, len);

    return ret_len;
}

static struct sockaddr_in udp_recv;
socklen_t udp_size = sizeof(udp_recv);

/**
 * @name:joylink_softap_socket_send 
 *
 * @param: socket_fd
 * @param: buf
 * @param: len
 *
 * @returns:   
 */
int joylink_softap_socket_send(int socket_fd, char *buf, int len)
{
    int ret_len = -1;

    if(socket_fd < 0 || buf == NULL || len <= 0){
        log_error("joylink socket send error!\n");
        return -1;
    }
    if(socket_fd == client_fd){
        ret_len = send(client_fd, buf, len, 0);
        log_info("joylink tcp send len: %d ret: %d \n", len, ret_len);
    }else if(socket_fd == udp_fd){
        ret_len = sendto(udp_fd, buf, len, 0, (struct sockaddr *)&udp_recv, udp_size);
        log_info("joylink tcp send len: %d ret: %d \n", len, ret_len);
    }else{
        log_error("joylink socket send fd error!\n");    
    }
    return ret_len;
}

static void joylink_softap_free(void)
{
    rt_wlan_ap_stop();

    /* close socket */
    if (udp_fd >= 0) {
        close(udp_fd);
        udp_fd = -1;
    }
    if (client_fd >= 0) {
        close(client_fd);
        client_fd = -1;
    }
    if (tcp_fd >= 0) {
        close(tcp_fd);
        tcp_fd = -1;
    }
    if (ap_sem)
    {
        rt_sem_delete(ap_sem);
        ap_sem = RT_NULL;
    }

}

/**
 * @name:joylink_softap_stop
 *
 * @returns:   
 */
int joylink_softap_stop(void)
{
#define JOYLINK_AP_SEM_TIMO   (5 * RT_TICK_PER_SECOND)

    if (is_ap_start) {
        /* clear status */
        is_ap_start = 0;

        /* get close success semaphore */
        if (rt_sem_take(ap_sem, JOYLINK_AP_SEM_TIMO) < 0)
        {
            return -1;
        }

        joylink_softap_free();
    }

    return 0;
}

/**
 * @name:joylink_softap_entry 
 *
 * @returns:   
 */
static void joylink_softap_entry(void *param)
{
    int ret_len = 0;
    char recBuffer[1024] = {0};

    int ret = 0;
    int i = 0;

    int udp_broad_disable = 0;
    int data_ret = 0;

    int max_fd = -1;
    fd_set  readfds;
    struct timeval time_out;

    time_t time_start = 0;
    time_t time_now = 0;

    joylinkSoftAP_Result_t softap_res;
    memset(&softap_res,0,sizeof(joylinkSoftAP_Result_t));
    
    joylink_softap_init();
    
    udp_fd = joylink_udp_init(JOYLINK_LOCAL_UDP_PORT);
    tcp_fd = joylink_tcp_init(JOYLINK_TCP_PORT);

    while(is_ap_start){
        max_fd = -1;
        FD_ZERO(&readfds);

        if(udp_fd >= 0){
            FD_SET(udp_fd, &readfds);
        }
        if(tcp_fd >= 0){
            FD_SET(tcp_fd, &readfds);
        }
        if(client_fd >= 0){
            FD_SET(client_fd, &readfds);
        }
        if(max_fd < udp_fd){
            max_fd = udp_fd;
        }
        if(max_fd < tcp_fd){
            max_fd = tcp_fd;
        }
        if(max_fd < client_fd){
            max_fd = client_fd;
        }

        time_out.tv_usec = 0L;
        time_out.tv_sec = (long)2;

        time_now = time(NULL);
        if(udp_broad_disable == 0 && udp_fd >= 0 && (time_now - time_start) >= 2){
            time_start = time_now;
            log_info("joylink softap udp broad\n");
            joylink_softap_udpbroad(udp_fd);
        }
        log_info("\njoylink wait data!\n\n");

        ret = select(max_fd+1, &readfds, NULL, NULL, &time_out);
        if (ret <= 0){
            continue;
        }
        for(i = 0; i < ret; i++){
            if(FD_ISSET(tcp_fd, &readfds)){
                int new_fd = -1;
                struct sockaddr_in client_addr;
                socklen_t client_size = sizeof(client_addr);

                memset(&client_addr, 0, sizeof(client_addr));
                new_fd = accept(tcp_fd, (struct sockaddr *)(&client_addr), &client_size);
                if(new_fd >= 0 && client_fd < 0){
                    client_fd = new_fd;
                    log_error("accept a client!\n");
                    continue;
                }else{
                    close(new_fd);
                    log_info("error: tcp have a client Already!\n");
                    continue;
                }
            }
            if(FD_ISSET(client_fd, &readfds)){
                memset(recBuffer, 0, 1024);
                ret_len =recv(client_fd, recBuffer, 1024, 0);
                if(ret_len <= 0)
                { 
                     close(client_fd);
                    client_fd = -1;
                    log_error("tcp recv data error!,tcp close\n");
                    continue;
                }
                log_info("tcp recv data: %s len: %d\n", recBuffer, ret_len);
                data_ret = joylink_softap_data_packet_handle(client_fd, (uint8_t *)recBuffer, ret_len);
                if(data_ret > 0){
                    udp_broad_disable = 1;
                }else{
                    udp_broad_disable = 0;
                }
            }
            if(FD_ISSET(udp_fd, &readfds)){
                memset(recBuffer, 0, 1024);
                ret_len = recvfrom(udp_fd, recBuffer, sizeof(recBuffer), 0, (struct sockaddr *)&udp_recv, &udp_size);
                if(ret_len <= 0){
                    close(udp_fd);
                    udp_fd = -1;
                    log_error("udp recv data error!,UDP close\n");
                    continue;
                }
                log_info("udp recv data: %s len: %d\n", recBuffer, ret_len);
                data_ret = joylink_softap_data_packet_handle(udp_fd, (uint8_t *)recBuffer, ret_len);
                if(data_ret > 0){
                    udp_broad_disable = 1;
                }else{
                    udp_broad_disable = 0;
                }
            }
        }
        if(joylink_softap_result(&softap_res) != -1){
            log_info("softap finsh,ssid->%s,pass->%s\n", softap_res.ssid, softap_res.pass);

            /* stop AP mode and connect wifi by the ssid and password */
            is_ap_start = 0;
            joylink_softap_free();

            extern void wlan_autoconnect_init(void);
            wlan_autoconnect_init();
            rt_wlan_config_autoreconnect(RT_TRUE);
            rt_wlan_connect((const char *)softap_res.ssid, (const char *)softap_res.pass);

            return;
        }
    }

    /* release a semaphore for joylink_softap_stop */
    if (ap_sem)
    {
        rt_sem_release(ap_sem);
    }
}

/**
 * @name:joylink_softap_start 
 *
 * @returns:   
 */
int joylink_softap_start(void)
{
    rt_thread_t tid;

    if (ap_sem == NULL) {
        ap_sem = rt_sem_create("ap_sem", 0, RT_IPC_FLAG_FIFO);
        if (ap_sem == RT_NULL)
        {
            log_error("ap sem create failed!\n");
            return -1;
        }
    }

    tid = rt_thread_create("jp_ap", joylink_softap_entry, RT_NULL, 6 * 1024, 12, 10);
    if (tid)
    {
        is_ap_start = 1;
        rt_thread_startup(tid);
    }

    return 0;
}

int joylink_softap_is_start(void)
{
    return is_ap_start;
}
