#ifndef _JOYLINK_DEV_H_
#define _JOYLINK_DEV_H_

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "joylink.h"

#define LIGHT_CMD_NONE            (-1)
#define LIGHT_CMD_POWER            (1)

#define LIGHT_CTRL_NONE         (-1)
#define LIGHT_CTRL_ON           (1)
#define LIGHT_CTRL_OFF          (0)

typedef struct __light_ctrl{
    char cmd;
    int para_power;
    int para_state;
    int para_look;
    int para_move;
}LightCtrl_t;

/**
 * brief:
 * Check device network is ok. 
 *
 * @Returns: 
 *  0: network is not connected
 *  1: network is connected
 */
int
joylink_dev_is_net_ok(void);

/**
 * brief: 
 *
 * @Param: st
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_dev_set_connect_st(int st);

/**
 * brief: 
 *
 * @Param: jlp
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_dev_set_attr_jlp(JLPInfo_t *jlp);

/**
 * brief: 
 *
 * @Param: jlp
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_dev_get_jlp_info(JLPInfo_t *jlp);

/**
 * brief: 
 * Set unactived mode for deivce activated.
 *
 * @Returns:
 * E_RET_OK
 */
E_JLRetCode_t
joylink_set_unactived_mode(void);

/**
 * brief: 
 * Set config mode for deivce smarting config.
 *
 * @Returns:
 * E_RET_OK
 */
E_JLRetCode_t
joylink_set_config_mode(void);

/**
 * brief: 
 * Exit current config mode or unactived mode.
 *
 * @Returns:
 * E_RET_OK
 */
E_JLRetCode_t
joylink_exit_current_mode(void);

/**
 * brief: 
 *
 * @Param: out_modelcode
 * @Param: out_max
 *
 * @Returns: 
 */
int
joylink_dev_get_modelcode(char *out_modelcode, int32_t out_max);

/**
 * brief: 
 *
 * @Param: out_snap
 * @Param: out_max
 *
 * @Returns: 
 */
int
joylink_dev_get_snap_shot(char *out_snap, int32_t out_max);

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
joylink_dev_get_json_snap_shot(char *out_snap, int32_t out_max, int code, char *feedid);

/**
 * brief: 
 *
 * @Param: json_cmd
 *
 * @Returns: 
 */
E_JLRetCode_t 
joylink_dev_lan_json_ctrl(const char *json_cmd);

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
E_JLRetCode_t 
joylink_dev_script_ctrl(const char *src, int src_len, JLContrl_t *ctr, int from_server);

/**
 * brief: 
 *
 * @Param: otaOrder
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_dev_ota(JLOtaOrder_t *otaOrder);

/**
 * brief: 
 */
void
joylink_dev_ota_status_upload(void);

/**
 * brief: 
 */
int 
joylink_test_ota_crc(void);


/**
 * brief: 
 *
 * @Param: pidt
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_dev_get_idt(jl2_d_idt_t *pidt);

/**
 * brief: 
 *
 * @Param: 
 *
 * @Returns: 
 */
int
joylink_dev_get_random(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
