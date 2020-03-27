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
#include <joylink_porting_layer.h>

#define USER_DATA_POWER   "Power"
#define USER_DATA_MARK   "Mark"
#define USER_DATA_SETTEMPERATURE   "SetTemperature"
#define USER_DATA_CURRENTTEMPERATURE   "CurrentTemperature"
#define USER_DATA_CURRENTHUMIDITY   "CurrentHumidity"
#define USER_DATA_BABYLOCK   "BabyLock"
#define USER_DATA_STATE   "State"
#define USER_DATA_ERROR   "Error"
#define USER_DATA_MODE   "Mode"

typedef struct _user_dev_status_t {
    int Power;
    int Mark;
    float SetTemperature;
    float CurrentTemperature;
    float CurrentHumidity;
    int BabyLock;
    int State;
    int Error;
    int Mode;
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
int joylink_dev_user_data_set(char *cmd, user_dev_status_t *user_data)
{
    rt_memcpy(&user_dev, user_data, sizeof(user_dev_status_t));
    return 0;
}

int joylink_heater_set(int argc, char **argv)
{
    user_dev_status_t *status = (user_dev_status_t *) &user_dev;

    if (argc != 3)
    {
        rt_kprintf("joylink_heater_set <temperature/humidity> <value>   -- set joylink heater device information\n");
        rt_kprintf("joylink_heater_set <state/error/mode> <value>   -- set joylink heater device status\n");
        return -1;
    }

    if (rt_strcmp(argv[1], "temperature") == 0)
    {
        status->CurrentTemperature = (float)atoi(argv[2]);
        joylink_dev_user_data_set(USER_DATA_CURRENTTEMPERATURE, status);
    }
    else if (rt_strcmp(argv[1], "humidity") == 0)
    {
        status->CurrentHumidity = (float)atoi(argv[2]);
        joylink_dev_user_data_set(USER_DATA_CURRENTHUMIDITY, status);
    }
    else if (rt_strcmp(argv[1], "state") == 0)
    {
        status->State = atoi(argv[2]);
        joylink_dev_user_data_set(USER_DATA_STATE, status);
    }
    else if (rt_strcmp(argv[1], "error") == 0)
    {
        status->Error = atoi(argv[2]);
        joylink_dev_user_data_set(USER_DATA_ERROR, status);
    }
    else if (rt_strcmp(argv[1], "mode") == 0)
    {
        status->Mode = atoi(argv[2]);
        joylink_dev_user_data_set(USER_DATA_MODE, status);
    }
    else
    {
        rt_kprintf("joylink_heater_set <temperature/humidity> <value>   -- set joylink heater device information\n");
        rt_kprintf("joylink_heater_set <state/error/mode> <value>   -- set joylink heater device status\n");
        return -1;
    }

    return 0;
}

#ifdef FINSH_USING_MSH
#include <finsh.h>
MSH_CMD_EXPORT(joylink_heater_set, set joylink heater device information);
#endif /* FINSH_USING_MSH */

/* prase the json device information */
int
joylink_dev_parse_ctrl(const char *pMsg, user_dev_status_t *userDev)
{
    cJSON *pSub, *pJson;
    cJSON *pStreams;
    char tmp_str[64];
    int ret = -1;

    if (RT_NULL == pMsg || RT_NULL == userDev)
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
        for (iCnt = 0; iCnt < iSize; iCnt++)
        {
            pSub = cJSON_GetArrayItem(pStreams, iCnt);
            if (RT_NULL == pSub)
            {
                continue;
            }

            cJSON *pSId = cJSON_GetObjectItem(pSub, "stream_id");
            if (RT_NULL == pSId)
            {
                break;
            }
            cJSON *pV = cJSON_GetObjectItem(pSub, "current_value");
            if (RT_NULL == pV)
            {
                continue;
            }

            if (!strcmp(USER_DATA_POWER, pSId->valuestring))
            {
                if (pV->type == cJSON_String)
                {
                    memset(tmp_str, 0, sizeof(tmp_str));
                    strcpy(tmp_str, pV->valuestring);
                    userDev->Power = atoi(tmp_str);
                }
                else if (pV->type == cJSON_Number)
                {
                    userDev->Power = pV->valueint;
                }
                joylink_dev_user_data_set(USER_DATA_POWER, userDev);
            }
            if (!strcmp(USER_DATA_MARK, pSId->valuestring))
            {
                if (pV->type == cJSON_String)
                {
                    memset(tmp_str, 0, sizeof(tmp_str));
                    strcpy(tmp_str, pV->valuestring);
                    userDev->Mark = atoi(tmp_str);
                }
                else if (pV->type == cJSON_Number)
                {
                    userDev->Mark = pV->valueint;
                }
                joylink_dev_user_data_set(USER_DATA_MARK, userDev);
            }
            if (!strcmp(USER_DATA_SETTEMPERATURE, pSId->valuestring))
            {
                if (pV->type == cJSON_String)
                {
                    memset(tmp_str, 0, sizeof(tmp_str));
                    strcpy(tmp_str, pV->valuestring);
                    userDev->SetTemperature = atof(tmp_str);
                }
                else if (pV->type == cJSON_Number)
                {
                    userDev->SetTemperature = pV->valuedouble;
                }
                joylink_dev_user_data_set(USER_DATA_SETTEMPERATURE, userDev);
            }
            if (!strcmp(USER_DATA_CURRENTTEMPERATURE, pSId->valuestring))
            {
                if (pV->type == cJSON_String)
                {
                    memset(tmp_str, 0, sizeof(tmp_str));
                    strcpy(tmp_str, pV->valuestring);
                    userDev->CurrentTemperature = atof(tmp_str);
                }
                else if (pV->type == cJSON_Number)
                {
                    userDev->CurrentTemperature = pV->valuedouble;
                }
                joylink_dev_user_data_set(USER_DATA_CURRENTTEMPERATURE, userDev);
            }
            if (!strcmp(USER_DATA_CURRENTHUMIDITY, pSId->valuestring))
            {
                if (pV->type == cJSON_String)
                {
                    memset(tmp_str, 0, sizeof(tmp_str));
                    strcpy(tmp_str, pV->valuestring);
                    userDev->CurrentHumidity = atof(tmp_str);
                }
                else if (pV->type == cJSON_Number)
                {
                    userDev->CurrentHumidity = pV->valuedouble;
                }
                joylink_dev_user_data_set(USER_DATA_CURRENTHUMIDITY, userDev);
            }
            if (!strcmp(USER_DATA_BABYLOCK, pSId->valuestring))
            {
                if (pV->type == cJSON_String)
                {
                    memset(tmp_str, 0, sizeof(tmp_str));
                    strcpy(tmp_str, pV->valuestring);
                    userDev->BabyLock = atoi(tmp_str);
                }
                else if (pV->type == cJSON_Number)
                {
                    userDev->BabyLock = pV->valueint;
                }
                joylink_dev_user_data_set(USER_DATA_BABYLOCK, userDev);
            }
            if (!strcmp(USER_DATA_STATE, pSId->valuestring))
            {
                if (pV->type == cJSON_String)
                {
                    memset(tmp_str, 0, sizeof(tmp_str));
                    strcpy(tmp_str, pV->valuestring);
                    userDev->State = atoi(tmp_str);
                }
                else if (pV->type == cJSON_Number)
                {
                    userDev->State = pV->valueint;
                }
                joylink_dev_user_data_set(USER_DATA_STATE, userDev);
            }
            if (!strcmp(USER_DATA_ERROR, pSId->valuestring))
            {
                if (pV->type == cJSON_String)
                {
                    memset(tmp_str, 0, sizeof(tmp_str));
                    strcpy(tmp_str, pV->valuestring);
                    userDev->Error = atoi(tmp_str);
                }
                else if (pV->type == cJSON_Number)
                {
                    userDev->Error = pV->valueint;
                }
                joylink_dev_user_data_set(USER_DATA_ERROR, userDev);
            }
            if (!strcmp(USER_DATA_MODE, pSId->valuestring))
            {
                if (pV->type == cJSON_String)
                {
                    memset(tmp_str, 0, sizeof(tmp_str));
                    strcpy(tmp_str, pV->valuestring);
                    userDev->Mode = atoi(tmp_str);
                }
                else if (pV->type == cJSON_Number)
                {
                    userDev->Mode = pV->valueint;
                }
                joylink_dev_user_data_set(USER_DATA_MODE, userDev);
            }

            char *dout = cJSON_Print(pSub);
            if (RT_NULL != dout)
            {
                log_debug("org streams:%s", dout);
                free(dout);
            }
        }
    }
    cJSON_Delete(pJson);
RET:
    return ret;
}

/* Generate json format data by user device information */
char *
joylink_dev_package_info(const int retCode, user_dev_status_t *userDev)
{
    cJSON *root, *arrary;
    cJSON *data_Power, *data_Mark;
    cJSON *data_SetTemperature, *data_CurrentTemperature, *data_CurrentHumidity;
    cJSON *data_BabyLock, *data_State, *data_Error, *data_Mode;
    char *out = RT_NULL;
    char i2str[64];

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

    data_Power = cJSON_CreateObject();
    cJSON_AddItemToArray(arrary, data_Power);
    cJSON_AddStringToObject(data_Power, "stream_id", "Power");
    memset(i2str, 0, sizeof(i2str));
    sprintf(i2str, "%d", userDev->Power);
    cJSON_AddStringToObject(data_Power, "current_value", i2str);

    data_Mark = cJSON_CreateObject();
    cJSON_AddItemToArray(arrary, data_Mark);
    cJSON_AddStringToObject(data_Mark, "stream_id", "Mark");
    memset(i2str, 0, sizeof(i2str));
    sprintf(i2str, "%d", userDev->Mark);
    cJSON_AddStringToObject(data_Mark, "current_value", i2str);

    data_SetTemperature = cJSON_CreateObject();
    cJSON_AddItemToArray(arrary, data_SetTemperature);
    cJSON_AddStringToObject(data_SetTemperature, "stream_id", "SetTemperature");
    memset(i2str, 0, sizeof(i2str));
    sprintf(i2str, "%f", userDev->SetTemperature);
    cJSON_AddStringToObject(data_SetTemperature, "current_value", i2str);

    data_CurrentTemperature = cJSON_CreateObject();
    cJSON_AddItemToArray(arrary, data_CurrentTemperature);
    cJSON_AddStringToObject(data_CurrentTemperature, "stream_id", "CurrentTemperature");
    memset(i2str, 0, sizeof(i2str));
    sprintf(i2str, "%f", userDev->CurrentTemperature);
    cJSON_AddStringToObject(data_CurrentTemperature, "current_value", i2str);

    data_CurrentHumidity = cJSON_CreateObject();
    cJSON_AddItemToArray(arrary, data_CurrentHumidity);
    cJSON_AddStringToObject(data_CurrentHumidity, "stream_id", "CurrentHumidity");
    memset(i2str, 0, sizeof(i2str));
    sprintf(i2str, "%f", userDev->CurrentHumidity);
    cJSON_AddStringToObject(data_CurrentHumidity, "current_value", i2str);

    data_BabyLock = cJSON_CreateObject();
    cJSON_AddItemToArray(arrary, data_BabyLock);
    cJSON_AddStringToObject(data_BabyLock, "stream_id", "BabyLock");
    memset(i2str, 0, sizeof(i2str));
    sprintf(i2str, "%d", userDev->BabyLock);
    cJSON_AddStringToObject(data_BabyLock, "current_value", i2str);

    data_State = cJSON_CreateObject();
    cJSON_AddItemToArray(arrary, data_State);
    cJSON_AddStringToObject(data_State, "stream_id", "State");
    memset(i2str, 0, sizeof(i2str));
    sprintf(i2str, "%d", userDev->State);
    cJSON_AddStringToObject(data_State, "current_value", i2str);

    data_Error = cJSON_CreateObject();
    cJSON_AddItemToArray(arrary, data_Error);
    cJSON_AddStringToObject(data_Error, "stream_id", "Error");
    memset(i2str, 0, sizeof(i2str));
    sprintf(i2str, "%d", userDev->Error);
    cJSON_AddStringToObject(data_Error, "current_value", i2str);

    data_Mode = cJSON_CreateObject();
    cJSON_AddItemToArray(arrary, data_Mode);
    cJSON_AddStringToObject(data_Mode, "stream_id", "Mode");
    memset(i2str, 0, sizeof(i2str));
    sprintf(i2str, "%d", userDev->Mode);
    cJSON_AddStringToObject(data_Mode, "current_value", i2str);

    out=cJSON_Print(root);
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
