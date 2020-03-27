#ifndef _JOYLINK_EXTERN_H_
#define _JOYLINK_EXTERN_H_

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "joylink.h"

#define JOYLINK_CLOUD_AUTH
#define JOYLINK_DEVICE_AUTH

#define JOYLINK_THUNDER_SLAVE
#define JOYLINK_SMART_CONFIG

/*
 * user set
 */
#define JLP_VERSION  1
/*
 * Create dev and get the index from developer center
 */

#define JLP_DEV_TYPE    E_JLDEV_TYPE_NORMAL
#define JLP_LAN_CTRL	E_LAN_CTRL_ENABLE
#define JLP_CMD_TYPE	E_CMD_TYPE_LUA_SCRIPT
#define JLP_SNAPSHOT    E_SNAPSHOT_NO

#define JLP_UUID "" 
#define IDT_CLOUD_PUB_KEY ""

typedef struct _user_dev_status_t {

} user_dev_status_t;

/**
 * brief:
 *
 * @Returns:
 */
int joylink_dev_get_user_mac(char *out);

/**
 * brief: 
 *
 * @Returns: 
 */
int joylink_dev_get_private_key(char *out);

/**
 * brief: 
 *
 * @Returns: 
 */
int joylink_dev_user_data_set(char *cmd, user_dev_status_t *user_data);

#ifdef __cplusplus
}
#endif

#endif

