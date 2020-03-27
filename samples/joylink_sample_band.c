#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rtthread.h>
#include <arpa/inet.h>
#include <netdev.h>

#include <cJSON.h>

#include <joylink.h>
#include <joylink_log.h>
#include <joylink_dev.h>
#include <joylink_extern.h>
#include <joylink_extern.h>
#include <joylink_porting_layer.h>

#define USER_DATA_SPORT      "Sport"
#define USER_DATA_SLEEP      "Sleep"
#define USER_DATA_PHYSIQUE   "Physique"

#define USER_DATA_CMD_SPORT      0x01
#define USER_DATA_CMD_SLEEP      0x02
#define USER_DATA_CMD_PHYSIQUE   0x03

#define USER_DATT_LEN            128
typedef struct _user_dev_status_t {
    char Sport[USER_DATT_LEN];
    char Sleep[USER_DATT_LEN];
    char Physique[USER_DATT_LEN];
} user_dev_status_t;

static user_dev_status_t user_dev;
extern struct netdev *netdev_default;

/* Check device network is connected */
int joylink_dev_is_net_ok(void)
{
    return (netdev_is_link_up(netdev_default) && (!ip_addr_isany(&(netdev_default->ip_addr)))) ? 1 : 0;
}

/* Get device UUID */
int joylink_dev_get_uuid(char *out)
{
    rt_memcpy(out, JOYLINK_SAMPLE_UUID, rt_strlen(JOYLINK_SAMPLE_UUID));
    return RT_EOK;
}

/* Get devuce public key */
int joylink_dev_get_public_key(char *out)
{
    rt_memcpy(out, JOYLINK_SAMPLE_PUB_KEY, rt_strlen(JOYLINK_SAMPLE_PUB_KEY));
    return RT_EOK;
}

/* Get device MAC address */
int joylink_dev_get_user_mac(char *out)
{
    int idx = 0;

    while(!netdev_is_up(netdev_default))
    {
        // delay for network interface device
        rt_thread_mdelay(500);
    }

    for (idx = 0; idx < netdev_default->hwaddr_len; idx++)
    {
        rt_snprintf(out + 2 * idx, 16 - 2 * idx, "%02X", netdev_default->hwaddr[idx]);
    }
    return RT_EOK;
}

/* Get device pricate key */
int joylink_dev_get_private_key(char *out)
{
    rt_memcpy(out, JOYLINK_SAMPLE_PRIVATE_KEY, rt_strlen(JOYLINK_SAMPLE_PRIVATE_KEY));
    return RT_EOK;
}

/* Get device usert data */
user_dev_status_t *joylink_dev_user_data_get(void)
{
    return (user_dev_status_t *) &user_dev;
}

/* Set device user data information */
int joylink_dev_user_data_set(int cmd, void *value)
{
    user_dev_status_t *status = &user_dev;
    char *data = (void *)value;

    switch (cmd)
    {
    case USER_DATA_CMD_SPORT:
        rt_memset(status->Sport, 0, USER_DATT_LEN);
        rt_memcpy(status->Sport, data, rt_strlen(data));
        log_debug("set wristband device sport status(%s).", status->Sport);
        break;

    case USER_DATA_CMD_SLEEP:
        rt_memset(status->Sleep, 0, USER_DATT_LEN);
        rt_memcpy(status->Sleep, data, rt_strlen(data));
        log_debug("set wristband device sleep status(%s).", status->Sleep);
        break;

    case USER_DATA_CMD_PHYSIQUE:
        rt_memset(status->Physique, 0, USER_DATT_LEN);
        rt_memcpy(status->Physique, data, rt_strlen(data));
        log_debug("set wristband device physique status(%s).", status->Physique);
        break;

    default:
        log_error("input cmd(%d) error\n", cmd);
        break;
    }

    return 0;
}

int joylink_band_set(int argc, char **argv)
{
    if (argc != 3)
    {
        rt_kprintf("joylink_band_set <sport/sleep/physique> <value>   -- set joylink wristband wristband device information\n");
        return -1;
    }

    if (rt_strcmp(argv[1], "sport") == 0)
    {
        joylink_dev_user_data_set(USER_DATA_CMD_SPORT, (void *)argv[2]);
    }
    else if (rt_strcmp(argv[1], "sleep") == 0)
    {
        joylink_dev_user_data_set(USER_DATA_CMD_SLEEP, (void *)argv[2]);
    }
    else if (rt_strcmp(argv[1], "physique") == 0)
    {
        joylink_dev_user_data_set(USER_DATA_CMD_PHYSIQUE, (void *)argv[2]);
    }
    else
    {
        rt_kprintf("joylink_band_set <sport/sleep/physique> <value>   -- set joylink wristband device information\n");
        return -1;
    }

    return 0;
}

#ifdef FINSH_USING_MSH
#include <finsh.h>
MSH_CMD_EXPORT(joylink_band_set, set joylink wristband device information);
#endif /* FINSH_USING_MSH */

/* prase the json device information */
static int joylink_dev_parse_ctrl(const char *pMsg, user_dev_status_t *userDev)
{
    int ret = -1;
    cJSON *pSub, *pJson, *pStreams;

    if (NULL == pMsg || NULL == userDev)
    {
        goto RET;
    }
    log_debug("json_org:%s", pMsg);

    pJson = cJSON_Parse(pMsg);
    if (RT_NULL == pJson)
    {
        log_error("--->:ERROR: pMsg is NULL\n");
        goto RET;
    }

    pStreams = cJSON_GetObjectItem(pJson, "streams");
    if (RT_NULL != pStreams)
    {
        int iSize = cJSON_GetArraySize(pStreams);
        int iCnt;

        for( iCnt = 0; iCnt < iSize; iCnt++)
        {
            char *dout = RT_NULL;

            pSub = cJSON_GetArrayItem(pStreams, iCnt);
            if (NULL == pSub)
                continue;

            cJSON *pSId = cJSON_GetObjectItem(pSub, "stream_id");
            if (NULL == pSId)
                break;

            cJSON *pV = cJSON_GetObjectItem(pSub, "current_value");
            if (NULL == pV)
                continue;

            if (!strcmp(USER_DATA_SPORT, pSId->valuestring))
            {
                memset(userDev->Sport, 0, sizeof(userDev->Sport));
                strcpy(userDev->Sport, pV->valuestring);
            }

            if (!strcmp(USER_DATA_SLEEP, pSId->valuestring))
            {
                memset(userDev->Sleep, 0, sizeof(userDev->Sleep));
                strcpy(userDev->Sleep, pV->valuestring);
            }

            if (!strcmp(USER_DATA_PHYSIQUE, pSId->valuestring))
            {
                memset(userDev->Physique, 0, sizeof(userDev->Physique));
                strcpy(userDev->Physique, pV->valuestring);
            }

            dout = cJSON_Print(pSub);
            if (RT_NULL != dout)
            {
                log_debug("org streams:%s", dout);
                rt_free(dout);
            }
        }
    }

    cJSON_Delete(pJson);
RET:
    return ret;
}

/* Generate json format data by user device information */
static char *joylink_dev_package_info(const int retCode, user_dev_status_t *userDev)
{
    cJSON *root, *arrary;
    cJSON *data_Sport, *data_Sleep, *data_Physique;
    char *out;

    if (RT_NULL == userDev)
    {
        return RT_NULL;
    }

    root = cJSON_CreateObject();
    if (RT_NULL == root)
    {
        goto RET;
    }

    arrary = cJSON_CreateArray();
    if (RT_NULL == arrary)
    {
        cJSON_Delete(root);
        goto RET;
    }

    cJSON_AddNumberToObject(root, "code", retCode);
    cJSON_AddItemToObject(root, "streams", arrary);

    data_Sport = cJSON_CreateObject();
    cJSON_AddItemToArray(arrary, data_Sport);
    cJSON_AddStringToObject(data_Sport, "stream_id", "Sport");
    cJSON_AddStringToObject(data_Sport, "current_value", userDev->Sport);
    data_Sleep = cJSON_CreateObject();
    cJSON_AddItemToArray(arrary, data_Sleep);
    cJSON_AddStringToObject(data_Sleep, "stream_id", "Sleep");
    cJSON_AddStringToObject(data_Sleep, "current_value", userDev->Sleep);
    data_Physique = cJSON_CreateObject();
    cJSON_AddItemToArray(arrary, data_Physique);
    cJSON_AddStringToObject(data_Physique, "stream_id", "Physique");
    cJSON_AddStringToObject(data_Physique, "current_value", userDev->Physique);

    out = cJSON_Print(root);
    cJSON_Delete(root);
RET:
    return out;
}

/* Get device information */
static char * joylink_dev_modelcode_info(const int retCode, user_dev_status_t *userDev)
{
    cJSON *root, *arrary, *element;
    char *out  = RT_NULL;
    char i2str[32];

    if (RT_NULL == userDev)
    {
        return RT_NULL;
    }

    root = cJSON_CreateObject();
    if (RT_NULL == root)
    {
        goto RET;
    }

    arrary = cJSON_CreateArray();
    if (RT_NULL == arrary)
    {
        cJSON_Delete(root);
        goto RET;
    }
    cJSON_AddItemToObject(root, "model_codes", arrary);

    rt_memset(i2str, 0, sizeof(i2str));
    element = cJSON_CreateObject();
    cJSON_AddItemToArray(arrary, element);
    cJSON_AddStringToObject(element, "feedid", "247828880060773075");
    cJSON_AddStringToObject(element, "model_code", "12345678123456781234567812345678");

    out = cJSON_Print(root);
    cJSON_Delete(root);
RET:
    return out;
}

/* device get model_code information data for the JSON format */
int joylink_dev_get_modelcode(char *out_modelcode, int32_t out_max)
{
    int len = 0;
    char *packet_data = RT_NULL;

    if (RT_NULL == out_modelcode || out_max < 0)
    {
        return 0;
    }

    packet_data = joylink_dev_modelcode_info(0, &user_dev);
    if (RT_NULL != packet_data)
    {
        len = rt_strlen(packet_data);
        log_debug("------>%s:len:%d\n", packet_data, len);
        if (len < out_max)
        {
            rt_memcpy(out_modelcode, packet_data, len);
        }
    }

    if (RT_NULL !=  packet_data)
    {
        rt_free(packet_data);
    }

    return len < out_max ? len : 0;
}

/* device get current device information data for the JSON foramt */
int joylink_dev_get_snap_shot_with_retcode(int32_t ret_code, char *out_snap, int32_t out_max)
{
    int len = 0;
    char *packet_data = RT_NULL;

    if (RT_NULL == out_snap || out_max < 0)
    {
        return 0;
    }

    packet_data = joylink_dev_package_info(ret_code, &user_dev);
    if (RT_NULL != packet_data)
    {
        len = rt_strlen(packet_data);
        log_debug("------>%s:len:%d\n", packet_data, len);
        if (len < out_max)
        {
            rt_memcpy(out_snap, packet_data, len);
        }
    }

    if (RT_NULL != packet_data)
    {
        rt_free(packet_data);
    }

    return len < out_max ? len : 0;
}

E_JLRetCode_t joylink_dev_script_ctrl(const char *src, int src_len, JLContrl_t *ctr, int from_server)
{
    E_JLRetCode_t ret = E_RET_ERROR;
    time_t tt;

    if (RT_NULL == src || RT_NULL == ctr)
    {
        return E_RET_ERROR;
    }

    ctr->biz_code = (int)(*((int *)(src + 4)));
    ctr->serial = (int)(*((int *)(src +8)));

    tt = time(NULL);
    log_info("bcode:%d:server:%d:time:%ld", ctr->biz_code, from_server, (long)tt);

    if (ctr->biz_code == JL_BZCODE_GET_SNAPSHOT)
    {
        ret = E_RET_OK;
    }
    else if (ctr->biz_code == JL_BZCODE_CTRL)
    {
        joylink_dev_parse_ctrl(src + 12, &user_dev);
        return E_RET_OK;
    }
    else
    {
        log_error("unKown biz_code:%d", ctr->biz_code);
    }

    return ret;
}

#ifdef JOYLINK_USING_AUTO_INIT
INIT_APP_EXPORT(joylink_start);
#endif

#ifdef FINSH_USING_MSH
#include <finsh.h>
MSH_CMD_EXPORT(joylink_start, joylink cloud work start);
MSH_CMD_EXPORT(joylink_stop, joylink cloud work stop);
MSH_CMD_EXPORT(joylink_config_reset, joylink wifi configuration reset);
MSH_CMD_EXPORT(joylink_mode_change, joylink network configuration mode change);
#endif /* FINSH_USING_MSH */
