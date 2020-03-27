#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(__RT_THREAD__)
#include <stdint.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#else
#include <stdarg.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
#endif

#include "joylink.h"
#include "joylink_utils.h"
#include "joylink_packets.h"
#include "joylink_crypt.h"
#include "joylink_json.h"
#include "joylink_dev.h"
#include "joylink_sub_dev.h"
#include "joylink_config_handle.h"
#include "joylink_extern.h"
#ifdef JOYLINK_USING_SOFTAP
#include "joylink_softap_start.h"
#include "joylink_softap.h"
#ifdef _IS_DEV_REQUEST_ACTIVE_SUPPORTED_
#include "joylink_cloud_log.h"
#include "joylink_dev_active.h"
#endif /* _IS_DEV_REQUEST_ACTIVE_SUPPORTED_ */
#endif /* JOYLINK_USING_SOFTAP  */
JLDevice_t  _g_dev = {
    .server_socket = -1,
    .server_st = 0,
    .hb_lost_count = 0,
    .lan_socket = -1,
    .local_port = 80
};

JLDevice_t  *_g_pdev = &_g_dev;

static rt_bool_t is_start = RT_FALSE;
static rt_sem_t jlp_sem = NULL;

extern int
joylink_proc_server_st();

extern void
joylink_proc_lan();

extern void
joylink_proc_server();

extern int
joylink_ecc_contex_init(void);

extern void
joylink_cloud_fd_lock();

extern void
joylink_cloud_fd_unlock();

extern void
joylink_agent_gw_thread_start();

extern int
joylink_softap_is_need_active(void);

extern int
joylink_softap_active_clear(void);

extern void
jl3_uECC_set_rng(uECC_RNG_Function rng_function);

extern int
joylink_is_usr_timestamp_ok(char *usr, int timestamp);

extern void
joylink_agent_req_cloud_proc();

/**
 * brief:
 *
 * @Param: ver
 */
void
joylink_dev_set_ver(short ver)
{
    _g_pdev->jlp.version = ver;
    joylink_dev_set_attr_jlp(&_g_pdev->jlp);
}

/**
 * brief:
 *
 * @Returns:
 */
short
joylink_dev_get_ver()
{
    return _g_pdev->jlp.version;
}

void
joylink_lan_entry(void *param)
{
    int maxFd = 0, ret = 0;
    fd_set  readfds;
    struct timeval selectTimeOut;

    while(is_start)
    {
        if (!joylink_dev_is_net_ok()){
            rt_thread_mdelay(10);
            continue;
        }

        FD_ZERO(&readfds);
        FD_SET(_g_pdev->lan_socket, &readfds);
        maxFd = (int)_g_pdev->lan_socket;

        selectTimeOut.tv_usec = 0L;
        selectTimeOut.tv_sec = (long)1;

        ret = select(maxFd + 1, &readfds, NULL, NULL, &selectTimeOut);
        if (ret < 0){
            log_error("Lan socket select error: %s!\r\n", strerror(errno));
            goto _exit;
        }else if (ret == 0){
        }else{
            if (FD_ISSET(_g_pdev->lan_socket, &readfds)){
                joylink_proc_lan();
                rt_thread_mdelay(10);
            }
        }
    }
_exit:
    if (_g_pdev->lan_socket >= 0) {
        closesocket(_g_pdev->lan_socket);
        _g_pdev->lan_socket = -1;
    }
}

void
joylink_lan_loop(void)
{
    int broadcastEnable = 1;
    struct sockaddr_in sin;
    rt_thread_t lan_tid = RT_NULL;

    bzero(&sin, sizeof(sin));
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(_g_pdev->local_port);
    _g_pdev->lan_socket = socket(AF_INET, SOCK_DGRAM, 0);

    if (_g_pdev->lan_socket < 0){
        log_error("lan socket() failed!");
        return;
    }

    if (setsockopt(_g_pdev->lan_socket,
                SOL_SOCKET, SO_BROADCAST,
                (uint8_t *)&broadcastEnable,
                sizeof(broadcastEnable)) < 0){
        log_error("SO_BROADCAST ERR");
    }

    if(0 > bind(_g_pdev->lan_socket, (SOCKADDR*)&sin, sizeof(SOCKADDR))){
        log_error("Bind lan socket error!");
    }

    lan_tid = rt_thread_create("joy_lan", joylink_lan_entry, RT_NULL, 10 * 1024, 10, 10);
    if (lan_tid)
    {
        rt_thread_startup(lan_tid);
    }
}

extern void
joylink_agent_req_cloud_proc();

char getin_config_flag = 0;

/**
 * brief:
 */
void
joylink_main_loop(void)
{
    int ret;
    struct timeval  selectTimeOut;
    static uint32_t serverTimer;
    static uint32_t runTimer;

    static int interval = 0;
    int active_req_count = 0;

    joylink_util_timer_reset(&serverTimer);
    joylink_util_timer_reset(&runTimer);

    // init joylink semaphore
    if (jlp_sem == NULL) {
        jlp_sem = rt_sem_create("jlp_sem", 0, RT_IPC_FLAG_FIFO);
        if (jlp_sem == RT_NULL)
        {
            log_error("sem create failed!\n");
            return;
        }
    }

    // start work
    is_start = RT_TRUE;

    // start lan socket loop
    joylink_lan_loop();

    while (is_start){
            joylink_cloud_fd_lock();
#if defined(JOYLINK_USING_SOFTAP)
        if (getin_config_flag == 1) {
            getin_config_flag = 0;
            joylink_softap_start();
        }
#ifdef _IS_DEV_REQUEST_ACTIVE_SUPPORTED_

        if(!joylink_dev_is_net_ok())
        {
            rt_thread_mdelay(10);
            continue;
        }

        if(joylink_softap_is_need_active()){
            joylink_cloud_log_post(TAGID_LOG_AP_CONNECTED, RES_LOG_SUCCES,NULL,"[CloudLog]Connect Ap Success","0",NULL);
            log_info("dev active begin");
            ret = joylink_dev_active_req();
            if(ret >= 0){
                log_info("dev active success");
                joylink_softap_active_clear();
            }else{
                log_error("dev active failed try again");
            }
            active_req_count++;
            log_info("active_req_count = %d",active_req_count);
            if(active_req_count > 5){
                joylink_softap_active_clear();
                active_req_count = 0;
                joylink_cloud_log_post(TAGID_LOG_AP_FULL_RES, RES_LOG_FAIL,NULL,"[CloudLog]device active failed,timeout","0",NULL);
            }
        }

#endif /* _IS_DEV_REQUEST_ACTIVE_SUPPORTED_ */
#elif defined(JOYLINK_USING_SMARTCONFIG) || defined(JOYLINK_USING_THUNDER_SLAVE)
            if (getin_config_flag == 1) {
                getin_config_flag = 0;
                joylink_config_start(60 * 1000);
            } else if (getin_config_flag == 2) {
                getin_config_flag = 0;
                joylink_config_stop();
            }
#endif /* JOYLINK_USING_SOFTAP || JOYLINK_USING_SMARTCONFIG */

        if (joylink_util_is_time_out(serverTimer, interval)){
            joylink_util_timer_reset(&serverTimer);
            if(joylink_dev_is_net_ok()){
                interval = joylink_proc_server_st();
            }else{
                interval = 100;
            }
        }

        int maxFd;
        fd_set  readfds;


        FD_ZERO(&readfds);
        if (_g_pdev->server_socket != -1
               && _g_pdev->server_st > 0){
            FD_SET(_g_pdev->server_socket, &readfds);
        }
        maxFd = (int)_g_pdev->server_socket;

        selectTimeOut.tv_usec = 0L;
        selectTimeOut.tv_sec = (long)1;

        ret = select(maxFd + 1, &readfds, NULL, NULL, &selectTimeOut);
        if (ret < 0){
            log_error("Select ERR: %s!\r\n", strerror(errno));
            goto UNLOOK_FD;
        }else if (ret == 0){
            goto UNLOOK_FD;
        }else{
            if((_g_pdev->server_socket != -1) &&
                (FD_ISSET(_g_pdev->server_socket, &readfds))){
                joylink_proc_server();
            }
        }

        UNLOOK_FD:
        joylink_cloud_fd_unlock();
#ifdef _AGENT_GW_
        if(E_RET_TRUE == is_joylink_server_st_work()){
            /*log_debug("proc agent_req_cloud proc");*/
            joylink_agent_req_cloud_proc();
        }
#endif
    }

    // clean interval timer
    interval = 0;

    // close socket and free memory
    if (_g_pdev->server_socket >= 0) {
        closesocket(_g_pdev->server_socket);
        _g_pdev->server_socket = -1;
    }

#ifdef JOYLINK_USING_SOFTAP
    extern int joylink_softap_stop(void);
    joylink_softap_stop();
#elif defined(JOYLINK_USING_THUNDER_SLAVE)
    joylink_config_stop();
#endif

    // release a semaphore for joylink_main_stop()
    rt_sem_release(jlp_sem);
}


/**
 * brief:
 */
int
joylink_check_cpu_mode(void)
{
    union
    {
        int data_int;
        char data_char;
    } temp;

    temp.data_int = 1;

    if(temp.data_char != 1){
    return -1;
    }else{
    return 0;
    }
}

int
joylink_check_mac_capital(char *mac)
{
	int i = 0;

	if(mac == NULL || strlen(mac) > 32){
		return -1;
	}
	for(i = 0; i < strlen(mac); i++){
		if(mac[i] >= 'a' && mac[i] <= 'f'){
			mac[i] = mac[i] - 'a' + 'A';
		}
	}
	//printf("Mac must are written in capitals!\n");
}

/**
 * brief:
 */
static void
joylink_dev_init()
{
    /**
     *NOTE: If your model_code_flag is configed in cloud,
     *Please set _g_pdev->model_code_flag = 1.
     */
    /*_g_pdev->model_code_flag = 1;*/
    char mac[16] = {0};
    char key[68] = {0};
    char uuid[JOY_UUID_LEN] = {0};
    char public_key[IDT_C_PK_LEN] = {0};

    printf("\n/**************info********************/\n\n");
    printf("sdk version: %s\n\n", _VERSION_);
    printf("dev version: %d\n",JLP_VERSION);

    printf("dev type: %d\n", JLP_DEV_TYPE);
    printf("dev lan strl: %d\n", JLP_LAN_CTRL);
    printf("dev cmd type: %d\n\n", JLP_CMD_TYPE);

    if(joylink_dev_get_uuid(uuid) < 0){
        printf("dev uuid: error!\n");
    }else{
        printf("dev uuid: %s\n", uuid);
    }

    if(joylink_dev_get_public_key(public_key) < 0){
        printf("dev public key: error!\n\n");
    }else{
        printf("dev public key: %s\n\n", public_key);
    }

    if(joylink_dev_get_user_mac(mac) < 0){
            printf("dev mac: error!\n");
    }else{
		joylink_check_mac_capital(mac);
        printf("dev mac: %s\n", mac);
    }
    if(joylink_dev_get_private_key(key) < 0){
            printf("dev private key: error!\n\n");
    }else{
        printf("dev private key: %s\n\n", key);
    }
    if(joylink_check_cpu_mode() == 0){
            printf("plt mode: --ok! little-endian--\n\n");
    }else{
        printf("plt mode: --error! big-endian--\n\n");
    }
    printf("/*************************************/\n\n");

    joylink_dev_get_jlp_info(&_g_pdev->jlp);
    joylink_dev_get_idt(&_g_pdev->idt);
    log_debug("--->feedid:%s", _g_pdev->jlp.feedid);
    log_debug("--->uuid:%s", _g_pdev->jlp.uuid);
    log_debug("--->accesskey:%s", _g_pdev->jlp.accesskey);
    log_debug("--->localkey:%s", _g_pdev->jlp.localkey);
}

/**
 * brief:
 *
 * @Returns:
 */

static int default_random(uint8_t *dest, unsigned size) {

    char *ptr = (char *)dest;
    size_t left = size;
    while (left > 0) {
        *ptr = (uint8_t)joylink_dev_get_random();
        left--;
        ptr++;
    }
    return 1;
}

int
joylink_main_start(void)
{
    jl3_uECC_set_rng((uECC_RNG_Function) &default_random);
    joylink_ecc_contex_init();
    joylink_dev_init();

#ifdef _AGENT_GW_
    joylink_agent_devs_load();
    joylink_agent_gw_thread_start();
#endif
    joylink_main_loop();
    return 0;
}

int
joylink_main_stop(void)
{
#define JOYLINK_SEM_TIMO   (5 * RT_TICK_PER_SECOND)
    if (is_start == RT_TRUE) {
        // stop work
        is_start = RT_FALSE;

        // get close success semaphore
        if (rt_sem_take(jlp_sem, JOYLINK_SEM_TIMO) < 0)
        {
            return -1;
        }

        // clean local server sattus
        _g_pdev->server_st = 0;
        _g_pdev->hb_lost_count = 0;
    }

    return 0;
}

int joylink_sdk_feedid_get(char *buf,char buflen)
{
    if(strlen(_g_pdev->jlp.feedid) == 0){
        return -1;
    }
    if(buflen < (strlen(_g_pdev->jlp.feedid) + 1)){
        log_error("buflen error");
        return -1;
    }

    memset(buf,0,buflen);
    strcpy(buf,_g_pdev->jlp.feedid);
    return 0;

}

