/* --------------------------------------------------
 * @brief: 
 *
 * @version: 1.0
 * --------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "joylink.h"
#include "joylink_packets.h"
#include "joylink_json.h"
#include "joylink_extern_sub_dev.h"
#include "joylink_utils.h"

#define JL_MAX_SUB 30

int sub_dev_numb = 4;

JLSubDevData_t _g_sub_dev[JL_MAX_SUB] = {
#if 1
        {.mac = "AA0011223344", .type = E_JLDEV_TYPE_SUB, .uuid = "3C939C", .lancon = 1, .cmd_tran_type = 1, .state = 1, .protocol = 1, .subNoSnapshot = E_SNAPSHOT_NO, .devAuthValue = "96f730bd1a6f9fcc2d40cfad0c0d8b9c"},
    {.mac = "AA0011223355", .type = E_JLDEV_TYPE_SUB, .uuid = "3C939C", .lancon = 1, .cmd_tran_type = 1, .state = 1, .protocol = 1, .subNoSnapshot = E_SNAPSHOT_NO, .devAuthValue = "9401a7005788efd93c25b46fdaf169ad"},
    {.mac = "AA0011223399", .type = E_JLDEV_TYPE_SUB, .uuid = "3C939C", .lancon = 1, .cmd_tran_type = 1, .state = 1, .protocol = 1, .subNoSnapshot = E_SNAPSHOT_NO, .devAuthValue = "880c86bcd07b1648367c332cb2a1463a"},
    {.mac = "AA00112233AA", .type = E_JLDEV_TYPE_SUB, .uuid = "3C939C", .lancon = 1, .cmd_tran_type = 1, .state = 1, .protocol = 1, .subNoSnapshot = E_SNAPSHOT_NO, .devAuthValue = "4445c58c13659307f298ec51444a7201"}
#endif
#if 0
        {.mac = "AA0011223344", .type = E_JLDEV_TYPE_SUB, .uuid = "4E4638", .lancon = 1, .cmd_tran_type = 1, .state = 1, .protocol = 1, .subNoSnapshot = E_SNAPSHOT_NO, .devAuthValue = "96f730bd1a6f9fcc2d40cfad0c0d8b9c"},
    {.mac = "AA0011223355", .type = E_JLDEV_TYPE_SUB, .uuid = "4E4638", .lancon = 1, .cmd_tran_type = 1, .state = 1, .protocol = 1, .subNoSnapshot = E_SNAPSHOT_NO, .devAuthValue = "9401a7005788efd93c25b46fdaf169ad"},
    {.mac = "AA0011223366", .type = E_JLDEV_TYPE_SUB, .uuid = "4E4638", .lancon = 1, .cmd_tran_type = 1, .state = 1, .protocol = 1, .subNoSnapshot = E_SNAPSHOT_NO, .devAuthValue = "844cd9ab1f28140d8d60bd389b660154"},
    {.mac = "AA0011223377", .type = E_JLDEV_TYPE_SUB, .uuid = "4E4638", .lancon = 1, .cmd_tran_type = 1, .state = 1, .protocol = 1, .subNoSnapshot = E_SNAPSHOT_NO, .devAuthValue = "006b728b172e3e204e16380398ffe9da"},
    {.mac = "AA0011223399", .type = E_JLDEV_TYPE_SUB, .uuid = "514C0E", .lancon = 1, .cmd_tran_type = 1, .state = 1, .protocol = 1, .subNoSnapshot = E_SNAPSHOT_NO, .devAuthValue = "880c86bcd07b1648367c332cb2a1463a"},
    {.mac = "AA00112233AA", .type = E_JLDEV_TYPE_SUB, .uuid = "514C0E", .lancon = 1, .cmd_tran_type = 1, .state = 1, .protocol = 1, .subNoSnapshot = E_SNAPSHOT_NO, .devAuthValue = "4445c58c13659307f298ec51444a7201"},
    {.mac = "AA00112233BB", .type = E_JLDEV_TYPE_SUB, .uuid = "514C0E", .lancon = 1, .cmd_tran_type = 1, .state = 1, .protocol = 1, .subNoSnapshot = E_SNAPSHOT_NO, .devAuthValue = "a2e3c63557d94575015ac713dff54b3d"},
    {.mac = "AA00112233DD", .type = E_JLDEV_TYPE_SUB, .uuid = "514C0E", .lancon = 1, .cmd_tran_type = 1, .state = 1, .protocol = 1, .subNoSnapshot = E_SNAPSHOT_NO, .devAuthValue = "dbc499de2517fe99e7246a5f9b32ef54"}
#endif
};

#ifdef _SAVE_FILE_
char *sub_file = "joylink_subdev_info.txt";

void joylink_dev_sub_data_save(void)
{
    FILE *outfile;
    int i;
    
    if(sub_dev_numb <= 0){
        system("rm joylink_subdev_info.txt");
        sub_dev_numb = 0;
        return;
    }
        
    outfile = fopen(sub_file, "wb+" );
    for(i = 0; i < JL_MAX_SUB; i++){
        if(strlen(_g_sub_dev[i].uuid) > 0)
            fwrite(&_g_sub_dev[i], sizeof(JLSubDevData_t), 1, outfile);
    }
    fclose(outfile);
}

void joylink_dev_sub_data_read(void)
{
    int ret;
    FILE *infile;

    infile = fopen(sub_file, "rb+");
    if(infile > 0){
        sub_dev_numb = 0;
        memset(_g_sub_dev, 0, sizeof(JLSubDevData_t) * JL_MAX_SUB);

        while(1){
            ret = fread(&_g_sub_dev[sub_dev_numb], sizeof(JLSubDevData_t), 1, infile);
            if(ret <= 0){
                fclose(infile);
                break;
            }
            sub_dev_numb++;
        }
    }
}
#endif

/**
 * brief: 
 *
 * @Param: dev
 * @Param: num
 *
 * @Returns: 
 */

int error_auth_flag = 0;

E_JLRetCode_t
joylink_dev_sub_save_auth_value(char *uuid, char *mac, char *auth_value)
{
    E_JLRetCode_t ret = E_RET_ERROR;
    int i; 

    for(i = 0; i < JL_MAX_SUB; i++){
        if(!strcmp(uuid, _g_sub_dev[i].uuid) && !strcmp(mac, _g_sub_dev[i].mac)){            
        memset(_g_sub_dev[i].devAuthValue, 0, 33);

        joylink_util_byte2hexstr((uint8_t *)auth_value, 16, (uint8_t *)_g_sub_dev[i].devAuthValue, 33);
        printf("SubDev value: %s %s\n", mac, _g_sub_dev[i].devAuthValue);
            ret = E_RET_OK;
        break;
        }
    }
#ifdef _SAVE_FILE_
    joylink_dev_sub_data_save();
#endif

    return ret;
}

/**
 * brief: 
 *
 * @Param: dev
 * @Param: num
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_dev_sub_add(JLSubDevData_t *dev, int num)
{
    /**
     *FIXME: todo
     */
    int i = 0;
    E_JLRetCode_t ret = E_RET_OK;

    for(i = 0; i < JL_MAX_SUB; i++){
        if(!strcmp(dev->uuid, _g_sub_dev[i].uuid) && !strcmp(dev->mac, _g_sub_dev[i].mac))
            return ret;
    }
    for(i = 0; i < JL_MAX_SUB; i++){
        if(strlen(_g_sub_dev[i].uuid) == 0){
            memcpy(&_g_sub_dev[i], dev, sizeof(JLSubDevData_t));
        sub_dev_numb++;
        break;
        }
    }
    return ret;
}

/**
 * brief: 
 *
 * @Param: dev
 * @Param: num
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_sub_dev_del(char *feedid)
{
    /**
     *FIXME: todo
     */
    E_JLRetCode_t ret = E_RET_OK;
    int i = 0;
    
    for(i = 0; i < JL_MAX_SUB; i++){
        if(!strcmp(feedid, _g_sub_dev[i].feedid)){
            memset(&_g_sub_dev[i], 0, sizeof(JLSubDevData_t));
        sub_dev_numb--;
        break;
        }
    }
    return ret;
}

/**
 * brief: 
 *
 * @Param: feedid
 * @Param: dev
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_dev_sub_get_by_feedid(char *feedid, JLSubDevData_t *dev)
{
    /**
     *FIXME: todo
     */
    E_JLRetCode_t ret = E_RET_ERROR;
    int i; 

    for(i = 0; i < JL_MAX_SUB; i++){
        if(!strcmp(feedid, _g_sub_dev[i].feedid)){
            memcpy(dev, &_g_sub_dev[i], sizeof(JLSubDevData_t));
            ret = E_RET_OK;
        break;
        }
    }

    return ret;
}

/**
 * brief: 
 *
 * @Param: uuid
 * @Param: mac
 * @Param: dev
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_sub_dev_get_by_uuid_mac(char *uuid, char *mac, JLSubDevData_t *dev)
{
    /**
     *FIXME: todo
     */
    E_JLRetCode_t ret = E_RET_OK;
    int i; 

    for(i = 0; i < JL_MAX_SUB; i++){
        if(!strcmp(uuid, _g_sub_dev[i].uuid) && !strcmp(mac, _g_sub_dev[i].mac)){
            memcpy(dev, &_g_sub_dev[i], sizeof(JLSubDevData_t));
            ret = E_RET_OK;
        break;
        }
    }

    return ret;
}

/**
 * brief: 
 *
 * @Param: uuid
 * @Param: mac
 * @Param: dev
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_dev_sub_update_keys_by_uuid_mac(char *uuid, char *mac, JLSubDevData_t *dev)
{
    /**
     *FIXME: todo
     */
    E_JLRetCode_t ret = E_RET_ERROR;
    int i; 

    for(i = 0; i < JL_MAX_SUB; i++){
        if(!strcmp(uuid, _g_sub_dev[i].uuid) && !strcmp(mac, _g_sub_dev[i].mac)){
            memcpy(_g_sub_dev[i].accesskey, dev->accesskey, sizeof(dev->accesskey));
            memcpy(_g_sub_dev[i].localkey, dev->localkey, sizeof(dev->localkey));
            memcpy(_g_sub_dev[i].feedid, dev->feedid, sizeof(dev->feedid));

        _g_sub_dev[i].lancon = dev->lancon;
            _g_sub_dev[i].cmd_tran_type = dev->cmd_tran_type;
    
            _g_sub_dev[i].state = E_JLDEV_ONLINE;
            ret = E_RET_OK;
        break;
        }
    }
#ifdef _SAVE_FILE_
    joylink_dev_sub_data_save();
#endif

    return ret;
}

/**
 * brief: 
 *
 * @Param: feedid
 * @Param: version
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_dev_sub_version_update(char *feedid, int version)
{
    /**
     *FIXME: todo
     */
    E_JLRetCode_t ret = E_RET_ERROR;
    int i; 

    for(i = 0; i < JL_MAX_SUB; i++){
        if(!strcmp(feedid, _g_sub_dev[i].feedid)){
            _g_sub_dev[i].version = version;
            ret = E_RET_OK;
        break;
        }
    }

    return ret;
}

/**
 * brief: 
 *
 * @Param: count
 * @Param: scan_type
 *
 * @Returns: 
 */

static char read_flag = 0;

JLSubDevData_t *
joylink_dev_sub_devs_get(int *count)
{
    /**
     *FIXME: todo must lock
     */
    int i = 0, j = 0, sum = 0; 
    JLSubDevData_t *devs = NULL;

    #ifdef _SAVE_FILE_
    if(read_flag == 0){
    read_flag = 1;
    joylink_dev_sub_data_read();
    }
    #endif
    
    sum = sub_dev_numb;
    devs = (JLSubDevData_t *)malloc(sizeof(JLSubDevData_t) * sum);
    bzero(devs, sizeof(JLSubDevData_t) * sum);
    if(devs != NULL){
        for(i = 0; i < JL_MAX_SUB; i++){
        if(strlen(_g_sub_dev[i].uuid) != 0){            
                    memcpy(&devs[j], &_g_sub_dev[i], sizeof(JLSubDevData_t));
            j++;
        }
        }
    }
   
    *count = sum;
    return devs;
}

/**
 * brief: 
 *
 * @Param: cmd
 * @Param: cmd_len
 * @Param: feedid
 *
 * @Returns: 
 */

static int sub_power = 0;

E_JLRetCode_t
joylink_dev_sub_ctrl(const char* cmd, int cmd_len, char* feedid)
{
    /**
     *FIXME: todo must lock
     */
    log_debug("cmd:%s:feedid:%s\n", cmd, feedid);
    E_JLRetCode_t ret = E_RET_OK;

    if(sub_power == 0)
    sub_power = 1;
    else if(sub_power == 1)
    sub_power = 0;

    return ret;
}

/**
 * brief: 
 *
 * @Param: feedid
 * @Param: out_len
 *
 * @Returns: 
 */


char *
joylink_dev_sub_get_snap_shot(char *feedid, int *out_len)
{
    /**
     *FIXME: todo must lock
     */
    char on[] = "{\"code\": 0,  \"streams\": [{ \"current_value\": \"1\", \"stream_id\": \"power\" }]}";
    char off[] = "{\"code\": 0,  \"streams\": [{ \"current_value\": \"0\", \"stream_id\": \"power\" }]}";

    char *tp =     NULL;

    char *ss = (char*)malloc(100);

    if(sub_power == 0)
    tp = off;
    else if(sub_power == 1)
    tp = on;

    if(ss != NULL){
        memset(ss, 0, 100);
        memcpy(ss, tp, strlen(tp)); 
        *out_len = strlen(tp);
    }

    return ss;
}

/**
 * brief: 
 *
 * @Param: feedid
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_dev_sub_unbind(const char *feedid)
{
    /**
     *FIXME: todo must lock
     */
    E_JLRetCode_t ret = E_RET_OK;
    int i = 0;
    
    log_debug("feedid:%s\n", feedid);
    
    for(i = 0; i < JL_MAX_SUB; i++){
        if(!strcmp(feedid, _g_sub_dev[i].feedid)){
            //_g_sub_dev[i].state = 2;//E_JLDEV_UNBIND;
            memset(&_g_sub_dev[i], 0, sizeof(JLSubDevData_t));
            sub_dev_numb--;
            break;
        }
    }
#ifdef _SAVE_FILE_
    joylink_dev_sub_data_save();
#endif

    return ret;
}
