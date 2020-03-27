#ifndef _JOYLINK_EXTERN_H_
#define _JOYLINK_EXTERN_H_

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#ifdef __RT_THREAD__
#include <rtthread.h>
#endif

#include "joylink.h"

#define JOYLINK_CLOUD_AUTH
#define JOYLINK_DEVICE_AUTH

#ifdef JOYLINK_USING_THUNDER_SLAVE
#define JOYLINK_THUNDER_SLAVE
#endif
#ifdef JOYLINK_USING_SMARTCONFIG
#define JOYLINK_SMART_CONFIG
#endif
#define JLP_VERSION  1

// server configution
#define JLP_SERVER           "sbdevicegw.jd.com"
#define JLP_PORT             2002

#define JLP_ENV_FEEDID       "jlp_feedid"
#define JLP_ENV_ACCESSKEY    "jlp_accesskey"
#define JLP_ENV_LOCALKEY     "jlp_localkey"
#define JLP_ENV_ACTIVATE     "jlp_activate"

// Create dev and get the index from developer center
#define JLP_DEV_TYPE         E_JLDEV_TYPE_NORMAL
#define JLP_LAN_CTRL         E_LAN_CTRL_ENABLE
#define JLP_CMD_TYPE         E_CMD_TYPE_LUA_SCRIPT
#define JLP_SNAPSHOT         E_SNAPSHOT_NO

// bzero/sleep function support
#define bzero(buff, bufsz)   rt_memset(buff, 0, bufsz)
#define sleep(time)          rt_thread_mdelay(time * 1000)

// for rt-thread cJSON package
#define cJSON_Print          cJSON_PrintUnformatted

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
int
joylink_dev_get_uuid(char *out);

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
int
joylink_dev_get_public_key(char *out);

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
int
joylink_dev_get_user_mac(char *out);

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
int
joylink_dev_get_private_key(char *out);


#ifdef __cplusplus
}
#endif

#endif

