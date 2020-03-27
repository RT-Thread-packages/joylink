#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "joylink.h"
#include "joylink_packets.h"
#include "joylink_json.h"
#include "joylink_auth_crc.h"
#include "joylink_ret_code.h"
#include "joylink_extern.h"

#include <rtthread.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <easyflash.h>

// configure default device information
#define JLP_UUID             "012345"
#define IDT_CLOUD_PUB_KEY    "01234567890123456789012345678901"
#define JLP_MAC              "012345678912"
#define JLP_PRIVATE_KEY      "01234567890123456789012345678901"

typedef struct _light_manage_
{
    int conn_st;    
    JLPInfo_t jlp;
    jl2_d_idt_t idt;
}LightManage_t;

LightManage_t _g_lightMgr = {
    .conn_st = -1,    

    .jlp.version = JLP_VERSION,
    .jlp.joySdkVersion = _VERSION_,
    .jlp.uuid = JLP_UUID,    
    .jlp.devtype = JLP_DEV_TYPE,
    .jlp.lancon = JLP_LAN_CTRL,
    .jlp.cmd_tran_type = JLP_CMD_TYPE,
    .jlp.noSnapshot = JLP_SNAPSHOT,
    .jlp.is_actived = 0,
    .jlp.joylink_server = JLP_SERVER,
    .jlp.server_port = JLP_PORT,

    .idt.type = 0,
    .idt.cloud_pub_key = IDT_CLOUD_PUB_KEY,
    .idt.pub_key = "01234567890123456789012345678901",
    .idt.sig = "01234567890123456789012345678901",
    .idt.f_sig = "01234567890123456789012345678901",
    .idt.f_pub_key = "01234567890123456789012345678901",
};

LightManage_t *_g_pLightMgr = &_g_lightMgr;

JLPInfo_t user_jlp;
jl2_d_idt_t user_idt;

/**
 * brief:
 * Get random number.
 *
 * @Returns: 
 *  random number
 */
int
joylink_dev_get_random(void)
{
    return rand();
}

/**
 * brief:
 * Check device network is ok. 
 *
 * @Returns: 
 *  0: network is not connected
 *  1: network is connected
 */
RT_WEAK int
joylink_dev_is_net_ok(void)
{
    /**
    *FIXME:must to do
     */
    return 1;
}

/**
 * brief:
 * When connecting server st changed,
 * this fun will be called.
 *
 * @Param: st device connect status
 * JL_SERVER_ST_INIT      (0)
 * JL_SERVER_ST_AUTH      (1)
 * JL_SERVER_ST_WORK      (2)
 *
 * @Returns: 
 *  E_RET_OK
 *  E_RET_ERROR
 */
E_JLRetCode_t
joylink_dev_set_connect_st(int st)
{
    char buff[64] = {0};

    sprintf(buff, "{\"conn_status\":\"%d\"}", st);
    log_info("--set_connect_st:%s\n", buff);

    // Save the state.
    _g_pLightMgr->conn_st = st;

    return E_RET_OK;
}

/**
 * brief:
 * Save joylink protocol info in flash.
 *
 * @Param:jlp joylink cloud information
 *
 * @Returns: 
 *  E_RET_OK
 *  E_RET_ERROR
 */
static E_JLRetCode_t
joylink_dev_set_info(const char *jlp_key, const char *jlp_value)
{
    RT_ASSERT(jlp_key);
    RT_ASSERT(jlp_value);

    if (rt_strlen(jlp_value))
    {
        if (ef_set_env(jlp_key, jlp_value) != EF_NO_ERR)
        {
            log_debug("-- set %s(%s) failed\n", jlp_key, jlp_value);
            return E_RET_ERROR;
        }
    }

    return E_RET_OK; 
}

E_JLRetCode_t
joylink_dev_set_attr_jlp(JLPInfo_t *jlp)
{
    if (RT_NULL == jlp)
    {
        return E_RET_ERROR;
    }

    if (joylink_dev_set_info(JLP_ENV_FEEDID, jlp->feedid) == E_RET_ERROR ||
        joylink_dev_set_info(JLP_ENV_ACCESSKEY, jlp->accesskey) == E_RET_ERROR ||
        joylink_dev_set_info(JLP_ENV_LOCALKEY, jlp->localkey) == E_RET_ERROR ||
        joylink_dev_set_info(JLP_ENV_ACTIVATE, "ture") == E_RET_ERROR)
    {
        return E_RET_ERROR;
    }

    if (ef_save_env() != EF_NO_ERR)
    {
        log_debug("-- save environment failed\n");
        return E_RET_ERROR;
    }

    return E_RET_OK;
}

/**
 * brief: 
 *
 * @Param: jlp joylink idt
 *
 * @Returns:
 * E_RET_ERROR
 * E_RET_OK
 */
E_JLRetCode_t
joylink_dev_get_idt(jl2_d_idt_t *pidt)
{
    jl2_d_idt_t *_g_idt = &(_g_pLightMgr->idt);

    if (RT_NULL == pidt)
    {
        return E_RET_ERROR; 
    }

    pidt->type = 0;
    strcpy(pidt->sig, _g_idt->sig);
    strcpy(pidt->pub_key, _g_idt->pub_key);
    strcpy(pidt->f_sig, _g_idt->f_sig);
    strcpy(pidt->f_pub_key, _g_idt->f_pub_key);
    strcpy(pidt->cloud_pub_key, _g_idt->cloud_pub_key);

    return E_RET_OK;
}

/**
 * brief: 
 * Set unactived mode for deivce activated.
 *
 * @Returns:
 * E_RET_OK
 */
E_JLRetCode_t
joylink_set_unactived_mode(void)
{
    _g_pLightMgr->jlp.is_actived = 0;
    log_debug("> set sdk unactived mode!\n");

    return E_RET_OK;
}

extern char getin_config_flag;

/**
 * brief: 
 * Set config mode for deivce smarting config.
 *
 * @Returns:
 * E_RET_OK
 */
E_JLRetCode_t
joylink_set_config_mode(void)
{
    getin_config_flag = 1;
    _g_pLightMgr->jlp.is_actived = 0;
    log_debug("> set sdk config mode!\n");

    return E_RET_OK;
}

/**
 * brief: 
 * Exit current config mode or unactived mode.
 *
 * @Returns:
 * E_RET_OK
 */
E_JLRetCode_t
joylink_exit_current_mode(void)
{
    getin_config_flag = 2;
    _g_pLightMgr->jlp.is_actived = 1;
    log_debug("> exit current mode!\n");

    return E_RET_OK;
}

/**
 * brief: 
 * Get device UUID .
 *
 * @Param: out device UUID
 * 
 * @Returns: 
 * <0: get failed
 *  0: get successfully
 */
RT_WEAK int
joylink_dev_get_uuid(char *out)
{
    rt_memcpy(out, JLP_UUID, rt_strlen(JLP_UUID));
    return 0;
}

/**
 * brief: 
 * Get device public key .
 *
 * @Param: out device public key
 * 
 * @Returns: 
 * <0: get failed
 *  0: get successfully
 */
RT_WEAK int
joylink_dev_get_public_key(char *out)
{
    rt_memcpy(out, IDT_CLOUD_PUB_KEY, rt_strlen(IDT_CLOUD_PUB_KEY));
    return 0;
}

/**
 * brief: 
 * Get device MAC address.
 *
 * @Param: out MAC address buffer
 * 
 * @Returns: 
 * <0: get failed
 *  0: get successfully
 */
RT_WEAK int
joylink_dev_get_user_mac(char *out)
{
    rt_memcpy(out, JLP_MAC, rt_strlen(JLP_MAC));
    return 0;
}

/**
 * brief: 
 * Get device pricate key.
 * 
 * @Param: out private key buffer
 *
 * @Returns:
 * <0: get failed
 *  0: get successfully
 */
RT_WEAK int
joylink_dev_get_private_key(char *out)
{
    rt_memcpy(out, JLP_PRIVATE_KEY, rt_strlen(JLP_PRIVATE_KEY));
    return 0;
}

static E_JLRetCode_t
joylink_dev_sync_info(void)
{
    JLPInfo_t *_g_jlp = &(_g_pLightMgr->jlp);
    jl2_d_idt_t *_g_idt = &(_g_pLightMgr->idt);

    if (joylink_dev_get_uuid(_g_jlp->uuid) < 0)
    {
        log_error("joylink get device uuid failed.\n");
        return E_RET_ERROR;
    }

    if (joylink_dev_get_public_key(_g_idt->cloud_pub_key) < 0)
    {
        log_error("joylink get device public key failed.\n");
        return E_RET_ERROR;
    }

    if (joylink_dev_get_user_mac(_g_jlp->mac) < 0)
    {
        log_error("joylink get device MAC address failed.\n");
        return E_RET_ERROR;
    }

    if (joylink_dev_get_private_key(_g_jlp->prikey) < 0)
    {
        log_error("joylink get device private key failed.\n");
        return E_RET_ERROR;
    }

    return E_RET_OK;
}

static E_JLRetCode_t
joylink_dev_get_info(char *jlp_info, const char *jlp_key, size_t size)
{
    char *info = RT_NULL;

    RT_ASSERT(jlp_info);
    RT_ASSERT(jlp_key);

    info = ef_get_env(jlp_key);
    if (RT_NULL == info)
    {
        log_debug("--get %s failed", jlp_key);
        return E_RET_ERROR;
    }

    rt_memcpy(jlp_info, info, size);

    return E_RET_OK;
}

/**
 * brief: 
 * Get joylink information from flash.
 *
 * @Param: jlp joylink information
 *
 * @Returns: 
 * E_RET_OK
 * E_RET_ERROR
 */
E_JLRetCode_t
joylink_dev_get_jlp_info(JLPInfo_t *jlp)
{
    JLPInfo_t *_g_jlp = &(_g_pLightMgr->jlp);
    char *activate = RT_NULL;

    if (RT_NULL == jlp)
    {
        return E_RET_ERROR;
    }

    // Synchronize the gloabl device information
    if (joylink_dev_sync_info() != E_RET_OK)
    {
        log_error("joylink synchronize device information failed.\n");
        return E_RET_ERROR;
    }

    // get device information from falsh
    activate = ef_get_env(JLP_ENV_ACTIVATE);
    if (activate && strcmp(activate, "ture") == 0)
    {       
        joylink_dev_get_info(_g_jlp->feedid, JLP_ENV_FEEDID, sizeof(_g_jlp->feedid));
        joylink_dev_get_info(_g_jlp->accesskey, JLP_ENV_ACCESSKEY, sizeof(_g_jlp->accesskey));
        joylink_dev_get_info(_g_jlp->localkey, JLP_ENV_LOCALKEY, sizeof(_g_jlp->localkey));

        jlp->is_actived = 1;
    }
    else
    {
#if defined(JOYLINK_USING_SMARTCONFIG) || defined(JOYLINK_USING_THUNDER_SLAVE) || \
    defined(JOYLINK_USING_SOFTAP)
        joylink_set_config_mode();
#else
        joylink_set_unactived_mode();
#endif 
    }

    if (joylink_dev_get_user_mac(jlp->mac) < 0)
    {
        strcpy(jlp->mac, _g_jlp->mac);
    }

    if (joylink_dev_get_private_key(jlp->prikey) < 0)
    {
        strcpy(jlp->prikey, _g_jlp->prikey);
    }

    // set joylink information
    strcpy(jlp->feedid, _g_jlp->feedid);
    strcpy(jlp->accesskey, _g_jlp->accesskey);
    strcpy(jlp->localkey, _g_jlp->localkey);
    strcpy(jlp->joylink_server, _g_jlp->joylink_server);
    jlp->server_port = _g_jlp->server_port;

    jlp->version = _g_jlp->version;
    strcpy(jlp->joySdkVersion, _g_jlp->joySdkVersion);
    strcpy(jlp->uuid, _g_jlp->uuid);
    jlp->devtype = _g_jlp->devtype;
    jlp->lancon = _g_jlp->lancon;
    jlp->cmd_tran_type = _g_jlp->cmd_tran_type;
    jlp->noSnapshot = _g_jlp->noSnapshot;

    // Set global user information
    user_jlp = _g_pLightMgr->jlp;
    user_idt = _g_pLightMgr->idt; 

    return E_RET_OK;
}

/**
 * brief: 
 * Splicing model_code format json data transmission.
 *
 * @Param: out_modelcode output data
 * @Param: out_max the maximum length for output data
 *
 * @Returns: 
 * Output data length
 */
RT_WEAK int
joylink_dev_get_modelcode(char *out_modelcode, int32_t out_max)
{
    if (RT_NULL == out_modelcode || out_max < 0)
    {
        return 0;
    }
    /**
    *FIXME:must to do
    */
    return 0;
}

/**
 * brief: 
 *
 * @Param: out_snap
 * @Param: out_max
 *
 * @Returns: 
 */
RT_WEAK int
joylink_dev_get_snap_shot_with_retcode(int32_t ret_code, char *out_snap, int32_t out_max)
{
    if (RT_NULL == out_snap || out_max < 0)
    {
        return 0;
    }
    /**
    *FIXME:must to do
    */
    return 0;
}

/**
 * brief: 
 *
 * @Param: out_snap
 * @Param: out_max
 *
 * @Returns: 
 */
int
joylink_dev_get_snap_shot(char *out_snap, int32_t out_max)
{
    return joylink_dev_get_snap_shot_with_retcode(0, out_snap, out_max); 
}

/**
 * brief: 
 *
 * @Param: out_snap
 * @Param: out_max
 * @Param: code
 * @Param: feedid
 *
 * @Returns: 
 */
int
joylink_dev_get_json_snap_shot(char *out_snap, int32_t out_max, int code, char *feedid)
{
    /**
     *FIXME:must to do
     */
    sprintf(out_snap, "{\"code\":%d, \"feedid\":\"%s\"}", code, feedid);

    return strlen(out_snap);
}

/**
 * brief: 
 *
 * @Param: json_cmd
 *
 * @Returns: 
 */
E_JLRetCode_t 
joylink_dev_lan_json_ctrl(const char *json_cmd)
{
    /**
     *FIXME:must to do
     */
    log_debug("json ctrl:%s", json_cmd);

    return E_RET_OK;
}

/**
 * brief: 
 *
 * @Param: src
 * @Param: src_len
 * @Param: ctr
 * @Param: from_server
 *
 * @Returns: 
 */
RT_WEAK E_JLRetCode_t 
joylink_dev_script_ctrl(const char *src, int src_len, JLContrl_t *ctr, int from_server)
{
    if (RT_NULL == src || NULL == ctr)
    {
        return E_RET_ERROR;
    }

    /**
     *FIXME:must to do
    */

    return E_RET_OK;
}

/**
 * brief: 
 *
 * @Param: otaOrder
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_dev_ota(JLOtaOrder_t *otaOrder)
{
#ifdef JOYLINK_USING_OTA
    rt_thread_t tid;

    extern void joylink_ota_task(void *data);
    tid = rt_thread_create("jl_ota", joylink_ota_task, (void *)otaOrder, 1024 * 6, 10, 10);
    if (tid)
    {
        rt_thread_startup(tid);
    }
#else
    log_error("Not support OTA update feature, please enable it in the menuconfig!\n");
#endif /* JOYLINK_USING_OTA */
    return E_RET_OK;
}


/**
 * brief: 
 */
void
joylink_dev_ota_status_upload(void)
{
    /**
     *FIXME:must to do
     */
    return;    
}

void
joylink_dev_ota_finsh(void)
{
    extern void rt_hw_cpu_reset(void);
    rt_hw_cpu_reset();
}


int joylink_dev_https_post( char* host, char* query, char *revbuf, int buflen)
{   
#if _IS_DEV_REQUEST_ACTIVE_SUPPORTED_
#define RECV_DATA_SIZE  1024 * 4
	char *recv_data = RT_NULL, *recv_body = RT_NULL;
	struct hostent *he;
    struct sockaddr_in server_addr; 
    int socket_fd = -1;
    int send_len = 0, recv_len = 0, ret_len = 0;
    struct timeval timeout;

	recv_data = rt_calloc(1, RECV_DATA_SIZE);
	if (recv_data == RT_NULL)
	{
		return -1;
	}

	if ((he = gethostbyname(host)) == RT_NULL)
	{
		log_error("host(%s) gethostbyname failed\n", host);
		return -1;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(80);
	server_addr.sin_addr = *((struct in_addr *)he->h_addr);

	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		log_error("socket create failed\n");
		return -1;
	}

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    /* set send and recv timeout option */
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout,
               sizeof(timeout));
    setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, (void *)&timeout,
               sizeof(timeout));

    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
	{
		log_error("socket connect failed\n");
		return -1;
	}

	send_len = send(socket_fd, query, rt_strlen(query), 0);
	if (send_len != rt_strlen(query))
	{
		log_error("send query data failed\n");
		return -1;
	}

    while (1)
    {
        ret_len = recv(socket_fd, recv_data + recv_len, RECV_DATA_SIZE - recv_len, 0);
        if (ret_len <= 0)
        {
            break;
        }
        recv_len += ret_len;
    }
    
	recv_body = strstr(recv_data, "\r\n\r\n");
	if (recv_body)
	{
		rt_memcpy(revbuf, recv_body + 4, 
            rt_strlen(recv_body + 4) > buflen ? buflen : rt_strlen(recv_body + 4));
	}
	else
	{
		log_error("prase recv data failed, recv_len: %d", recv_len);
		return -1;
	}
    
    if (socket_fd >= 0)
    {
        closesocket(socket_fd);
    }
    if (recv_data)
    {
        rt_free(recv_data);
    }

#endif
	return 0;
}


int joylink_dev_run_status(JLRunStatus_t status)
{
    int ret = 0;
    /**
         *FIXME:must to do
    */
    return ret;
}

