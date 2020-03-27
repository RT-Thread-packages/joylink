#include "joylink_log.h"
#include "joylink_thunder.h"

#include "joylink_extern.h"
#include "joylink_utils.h"
#include "joylink_auth_md5.h"

#include "joylink_config_handle.h"
#include "joylink_porting_layer.h"

#include <rtdevice.h>

#define CYCLE_HANDLE_TIME  50*1000  // 50ms

#define JOY_CONFIG_STAY_COUNT 5

uint8_t config_stop_flag = 1;
static struct rt_wlan_device *wlan_dev = RT_NULL;

typedef struct _Result
{
    uint8_t ssid_len;
    uint8_t pass_len;

    char ssid[33];
    char pass[33];
} Result_t;

Result_t config_result;

int joylink_get_random(void)
{
    return rand();
}

void joylink_change_hannel(int ch)
{
    if (wlan_dev)
    {
        rt_wlan_dev_set_channel(wlan_dev, ch);
    }
    return;
}

void joylink_80211_recv(uint8_t *buf, int buflen)
{
#ifdef JOYLINK_THUNDER_SLAVE
#define PHEADER_HEADER_LEN 64
    if (buflen > 70)
    {
        /* Check recv 802.11 data magic "JOY" */
        if ((buf[PHEADER_HEADER_LEN] == 0x4A) && (buf[PHEADER_HEADER_LEN + 1] == 0x4F) &&\
            (buf[PHEADER_HEADER_LEN + 2] == 0x59))
        {
            joyThunderSlaveProbeH(buf, buflen);
        }
    }

#endif
#ifdef JOYLINK_SMART_CONFIG
    joylink_smnt_datahandler((PHEADER_802_11)buf, buflen);
#endif
}

int joylink_80211_send(uint8_t *buf, int buflen)
{
    int ret = 0;

    ret = rt_wlan_dev_send_raw_frame(wlan_dev, buf, buflen + 4);
    if (ret != 0)
    {
        log_error("send faild: ret = %d", ret);
    }

    return ret;
}

#ifdef JOYLINK_THUNDER_SLAVE
int joylink_delete_mark(uint8_t *str)
{
    int for_i = 0;
    int for_n = 0;

    uint8_t temp[64] = {0};

    if(str == NULL)    return -1;

    for(for_i = 0; for_i < strlen((char *)str); for_i++)
    {
        if((str[for_i] >= '0' && str[for_i] <= '9') \
        || (str[for_i] >= 'a' && str[for_i] <= 'f') \
        || (str[for_i] >= 'A' && str[for_i] <= 'F')){
            temp[for_n] = str[for_i];
            for_n++;
        }
    }
    memset(str, 0, 64);
    memcpy(str, temp, strlen((char *)temp));

    return 0;
}

/**
 * brief:
 *
 * @Param: thunder slave init and finish
 *
 * @Returns:
 */
extern JLPInfo_t user_jlp;
extern jl2_d_idt_t user_idt;

int
joylink_thunder_slave_init(void)
{
    tc_slave_func_param_t thunder_param;

    log_info("init thunder slave!\r\n");

    memset(&thunder_param,0,sizeof(tc_slave_func_param_t));

    memcpy(thunder_param.uuid, user_jlp.uuid, 6);

    joylink_delete_mark((uint8_t *) user_jlp.mac);
    joylink_util_hexStr2bytes(user_jlp.mac, thunder_param.mac_dev, JOY_MAC_ADDRESS_LEN);

    joylink_util_hexStr2bytes(user_jlp.prikey, thunder_param.prikey_d, JOY_ECC_PRIKEY_LEN);
    joylink_util_hexStr2bytes(user_idt.cloud_pub_key, thunder_param.pubkey_c, JOY_ECC_PUBKEY_LEN);

    thunder_param.deviceid.length = strlen((char *)thunder_param.mac_dev);
    thunder_param.deviceid.value = joylink_util_malloc(thunder_param.deviceid.length);
    memcpy(thunder_param.deviceid.value, thunder_param.mac_dev, thunder_param.deviceid.length);

    thunder_param.switch_channel = (switch_channel_cb_t)joylink_change_hannel;
    thunder_param.get_random     =     (get_random_cb_t)joylink_get_random;
    thunder_param.result_notify_cb = (thunder_finish_cb_t)joylink_thunder_slave_finish;
    thunder_param.packet_80211_send_cb = (packet_80211_send_cb_t)joylink_80211_send;
    joyThunderSlaveInit(&thunder_param);

    joyThunderSlaveStart();

    joylink_util_free(thunder_param.deviceid.value);
    thunder_param.deviceid.value = NULL;
    return 0;
}

extern E_JLRetCode_t joylink_dev_set_attr_jlp(JLPInfo_t *jlp);
extern void joylink_connect_wifi(void);

int
joylink_thunder_slave_finish(tc_slave_result_t *result)
{
    JLPInfo_t jlp;

    uint8_t temp[32] = {0};
    uint8_t localkey[33] = {0};
    MD5_CTX md5buf;

    log_info("joylink thunder slave finish");

    memset(&jlp, 0, sizeof(JLPInfo_t));
    memcpy(jlp.feedid, result->cloud_feedid.value, result->cloud_feedid.length);
    memcpy(jlp.accesskey, result->cloud_ackey.value, result->cloud_ackey.length);

    memset(&md5buf, 0, sizeof(MD5_CTX));
    JDMD5Init(&md5buf);
    JDMD5Update(&md5buf, result->cloud_ackey.value, strlen((char *)result->cloud_ackey.value));
    JDMD5Final(&md5buf, temp);
    joylink_util_byte2hexstr(temp, 16, localkey, 32);

    memcpy(jlp.localkey, localkey, strlen((char *) localkey));

    log_info("feedid:%s, accesskey:%s, localkey: %s, serverurl:%s", result->cloud_feedid.value, result->cloud_ackey.value, localkey, result->cloud_server.value);

    joylink_dev_set_attr_jlp(&jlp);

    extern E_JLRetCode_t joylink_dev_get_jlp_info(JLPInfo_t *jlp);
    joylink_dev_get_jlp_info(&_g_pdev->jlp);

    memset(&config_result, 0, sizeof(Result_t));

    if(result->ap_ssid.length <= 32)
    {
        memcpy(config_result.ssid, result->ap_ssid.value, result->ap_ssid.length);
        config_result.ssid_len = result->ap_ssid.length;
    }
    else
    {
        log_info("passwd length too large\n");
    }

    if(result->ap_password.length <= 32)
    {
        memcpy(config_result.pass, result->ap_password.value, result->ap_password.length);
        config_result.pass_len = result->ap_password.length;

    }
    else
    {
        log_info("ssid length too large\n");
    }

    log_info("ssid:%s, passwd:%s\n", config_result.ssid, config_result.pass);

    if(strlen(config_result.ssid))
    {
        extern void joylink_config_wifi(uint8_t * ssid, uint8_t ssid_len, uint8_t * pwd, uint8_t pwd_len);
        joylink_config_wifi((uint8_t *)config_result.ssid, config_result.ssid_len, (uint8_t *)config_result.pass, config_result.pass_len);
    }

    joylink_config_stop();
    joylink_connect_wifi();

    return 0;
}

#endif /* JOYLINK_THUNDER_SLAVE */

#ifdef JOYLINK_USING_SMARTCONFIG
/**
 * brief:
 *
 * @Param: smartConfig init and finish
 *
 * @Returns:
 */

extern void joylinke_smart_config_finish(joylink_smnt_result_t* presult);

#ifndef JOYLINK_SMARTCONFIG_KEY
#define SMARTCONFIG_KEY "TJZ9M8SXE8IE9B5W"
#else
#define SMARTCONFIG_KEY JOYLINK_SMARTCONFIG_KEY
#endif

joylink_smnt_param_t smart_conf_param;

RT_WEAK int joylink_dev_get_samrtconfig_key(char *out)
{
    rt_memcpy(out, SMARTCONFIG_KEY, rt_strlen(SMARTCONFIG_KEY));
    return RT_EOK;
}

int
joylink_smart_config_init(void)
{
    char samrtconfig_key[20] = {0};
    memset(&smart_conf_param, 0, sizeof(joylink_smnt_param_t));

    joylink_dev_get_samrtconfig_key(samrtconfig_key);
    memcpy(smart_conf_param.secretkey, samrtconfig_key, strlen(samrtconfig_key));
    smart_conf_param.get_result_callback = &joylinke_smart_config_finish;

    log_info("init smart config!");
    joylink_smnt_init(smart_conf_param);
    return 0;
}

void
joylinke_smart_config_finish(joylink_smnt_result_t* presult)
{


    if(presult->smnt_result_status != smnt_result_ok)
    {
        log_error("joylink smartconfig error, please try it again!");
        joylink_config_stop();
        return 0;
    }

    memset(&config_result, 0, sizeof(Result_t));

    if(presult->jd_password_len <= 32)
    {
        memcpy(config_result.pass, presult->jd_password, presult->jd_password_len);
        config_result.pass_len = presult->jd_password_len;
    }
    else
    {
        log_info("passwd length too large:%d", presult->jd_password_len);
        joylink_config_stop();
        return 0;
    }

    if(presult->jd_ssid_len <= 32)
    {
        memcpy(config_result.ssid, presult->jd_ssid, presult->jd_ssid_len);
        config_result.ssid_len = presult->jd_ssid_len;
    }
    else
    {
        log_info("ssid length too large:%d", presult->jd_ssid_len);
        joylink_config_stop();
        return 0;
    }

    log_info("ssid:%s, passwd:%s", config_result.ssid, config_result.pass);

    if(strlen(config_result.ssid))
    {
        extern void joylink_config_wifi(uint8_t * ssid, uint8_t ssid_len, uint8_t * pwd, uint8_t pwd_len);
        joylink_config_wifi(config_result.ssid, config_result.ssid_len, config_result.pass, config_result.pass_len);
    }

    joylink_config_stop();
    joylink_connect_wifi();

    return;
}
#endif /* JOYLINK_USING_SMARTCONFIG */

/**
 * brief:
 *
 * @Param: change channel
 *
 * @Returns:
 */
static uint8_t config_count = 0;
static uint8_t config_channel = 0;

extern joylinkSmnt_t* pSmnt;
extern tc_slave_ctl_t tc_slave_ctl;

void
joylink_config_change_channel(void)
{
#ifdef JOYLINK_THUNDER_SLAVE
    if(tc_slave_ctl.thunder_state > sReqChannel)
        return;
#endif
#ifdef JOYLINK_SMART_CONFIG
    if(pSmnt->state > SMART_CH_LOCKING)
        return;
#endif
    config_count++;
    if(config_count == JOY_CONFIG_STAY_COUNT){
        config_count = 0;

        joylink_change_hannel(config_channel+1);
        // log_info("-->switch channel to:%d", config_channel+1);

#ifdef JOYLINK_THUNDER_SLAVE
        tc_slave_ctl.current_channel = config_channel + 1;
#endif
#ifdef JOYLINK_SMART_CONFIG
        pSmnt->chCurrentIndex = config_channel;

        pSmnt->state = SMART_CH_LOCKING;
        pSmnt->syncStepPoint = 0;
        pSmnt->syncCount = 0;
        pSmnt->chCurrentProbability = 0;
#endif
        config_channel++;
        if(config_channel == 13)
            config_channel = 0;
    }
}
/**
 * brief:
 *
 * @Param: config loop handle
 *
 * @Returns:
 */
void
joylink_config_loop_handle(void *data)
{
    time_t *time_out = (time_t *)data;
    time_t time_start = time(NULL);
    double time_old = 0;
    double time_now = 0;

    while(1){
        time_now = clock();
        if(time_now - time_old >= CYCLE_HANDLE_TIME){
            time_old = time_now;
            #ifdef JOYLINK_THUNDER_SLAVE
            joyThunderSlave50mCycle();
            #endif
            #ifdef JOYLINK_SMART_CONFIG
            joylink_smnt_cyclecall();
            #endif
            joylink_config_change_channel();
        }
        #ifdef JOYLINK_THUNDER_SLAVE
        if(config_stop_flag)
            break;
        #endif
        if(time_now - time_start > *time_out){
            joylink_config_stop();
            break;
        }
    }
}

static void joylink_wifi_recv_cb(struct rt_wlan_device *device, void *data, int len)
{
    joylink_80211_recv(data, len);
}

static void joylink_wifi_recv_init(void)
{
    if (rt_wlan_is_connected())
    {
        rt_wlan_disconnect();
    }

    rt_wlan_dev_set_promisc_callback(wlan_dev, joylink_wifi_recv_cb);
    rt_wlan_dev_enter_promisc(wlan_dev);
}

static void joylink_timer_task_entry(void *param)
{
    int ret = 0;
    rt_bool_t  config_finish = RT_FALSE;
    uint32_t start_time = 0, timeout = 0;

    timeout = (uint32_t)param;
    start_time = rt_tick_from_millisecond(rt_tick_get());
    while ((rt_tick_from_millisecond(rt_tick_get()) - start_time) < timeout)
    {
#ifdef JOYLINK_THUNDER_SLAVE
        ret = joyThunderSlave50mCycle();
#else
        ret = joylink_smnt_cyclecall();
#endif
        if (ret == 0)
        {
            config_finish = RT_TRUE;
            break;
        }
        joylink_config_change_channel();

        rt_thread_mdelay(50);
    }

    if (config_finish == RT_FALSE)
    {
        log_info("joylink_config_stop, timeout");
    }
    joylink_config_stop();
}

static int joylink_config_timer_start(uint32_t time_out)
{
    rt_thread_t joy_timer_task = RT_NULL;

    joy_timer_task = rt_thread_create("jl_timer", joylink_timer_task_entry, (void *)time_out, 4096, 20, 10);
    if (joy_timer_task)
    {
        rt_thread_startup(joy_timer_task);
    }

    return 0;
}

int joylink_config_start(uint32_t time_out)
{
    char check_flag = 0;

    if (wlan_dev == RT_NULL)
    {
        wlan_dev = (struct rt_wlan_device *)rt_device_find(RT_WLAN_DEVICE_STA_NAME);
        if (wlan_dev == RT_NULL)
        {
            LOG_E("Find wlan device(%s) failed.", RT_WLAN_DEVICE_STA_NAME);
            return RT_NULL;
        }
    }

    joylink_wifi_recv_init();
#ifdef JOYLINK_THUNDER_SLAVE
    config_stop_flag = 0;
    check_flag = 1;
    joylink_thunder_slave_init();
#endif
#ifdef JOYLINK_SMART_CONFIG
    check_flag = 1;
    joylink_smart_config_init();
#endif
    if(check_flag == 0){
        log_info("joylink config do not open!");
        return -1;
    }
    joylink_config_timer_start(time_out);
    return 0;
}

int joylink_config_stop(void)
{
#if defined(JOYLINK_SMART_CONFIG) || defined(JOYLINK_THUNDER_SLAVE)
#ifdef JOYLINK_THUNDER_SLAVE
    config_stop_flag = 1;
    joyThunderSlaveStop();
#endif
    rt_wlan_dev_set_promisc_callback(wlan_dev, RT_NULL);
    rt_wlan_dev_exit_promisc(wlan_dev);
#endif

    return 0;
}

int joylink_config_is_start(void)
{
    return config_stop_flag ? 0 : 1;
}

static uint8_t joylink_get_config_rst = 0;
static char joylink_ssid[33] = {0};
static char joylink_pwd[33] = {0};

/* config wifi ssid and password */
void joylink_config_wifi(uint8_t * ssid, uint8_t ssid_len, uint8_t * pwd, uint8_t pwd_len)
{
    rt_memset(joylink_ssid, 0x0, 33);
    rt_memset(joylink_pwd, 0x0, 33);

    if((ssid_len != 0) && (ssid_len <= 32))
    {
        rt_memcpy(joylink_ssid, ssid, ssid_len);
    }
    else
    {
        joylink_get_config_rst = 0;
        return;
    }

    if((pwd_len != 0) && (pwd_len <= 32))
    {
       rt_memcpy(joylink_pwd, pwd, pwd_len);
    }

    joylink_get_config_rst = 1;
}

static void joylink_connect_entry(void *param)
{
    if (rt_strlen(joylink_pwd) > 0)
    {
        rt_wlan_connect(joylink_ssid, joylink_pwd);
    }
    else
    {
        rt_wlan_connect(joylink_ssid, RT_NULL);
    }

    joylink_get_config_rst = 0;
}

/* connect wifi after samrtconfig get wifi ssid and password */
void joylink_connect_wifi(void)
{
    if (joylink_get_config_rst)
    {
        rt_thread_t tid;

        joylink_get_config_rst = 0;
        tid = rt_thread_create("jl_conn", joylink_connect_entry, RT_NULL, 2048, 20, 10);
        if (tid)
        {
            rt_thread_startup(tid);
        }
    }

}
