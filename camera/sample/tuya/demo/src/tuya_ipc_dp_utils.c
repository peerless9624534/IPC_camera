/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *FileName: tuya_ipc_dp_utils.c
  *
  * File Description：
  * 1. API implementation of DP point
  *
  * This file code is the basic code, users don't care it
  * Please do not modify any contents of this file at will. 
  * Please contact the Product Manager if you need to modify it.
  *
**********************************************************************************/
#include "tuya_ipc_api.h"
#include "tuya_ipc_dp_utils.h"
#include "tuya_ipc_dp_handler.h"
#include "tuya_cloud_com_defs.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

STATIC VOID respone_dp_value(BYTE_T dp_id, INT_T val);
STATIC VOID respone_dp_bool(BYTE_T dp_id, BOOL_T true_false);
STATIC VOID respone_dp_enum(BYTE_T dp_id, CHAR_T *p_val_enum);
STATIC VOID respone_dp_str(BYTE_T dp_id, CHAR_T *p_val_str);
STATIC VOID respone_dp_raw(BYTE_T dp_id, CHAR_T *p_val_str, INT_T len);
STATIC VOID handle_DP_SD_STORAGE_ONLY_GET(IN TY_OBJ_DP_S *p_obj_dp);

//------------------------------------------
VOID IPC_APP_upload_all_status(VOID)
{
#ifdef TUYA_DP_SLEEP_MODE
    respone_dp_bool(TUYA_DP_SLEEP_MODE, IPC_APP_get_sleep_mode() );
#endif

#ifdef TUYA_DP_LIGHT
    respone_dp_bool(TUYA_DP_LIGHT, IPC_APP_get_light_onoff() );
#endif

#ifdef TUYA_DP_FLIP
    respone_dp_bool(TUYA_DP_FLIP, IPC_APP_get_flip_onoff() );
#endif

#ifdef TUYA_DP_WATERMARK
    respone_dp_bool(TUYA_DP_WATERMARK, IPC_APP_get_watermark_onoff() );
#endif

#ifdef TUYA_DP_WDR
    respone_dp_bool(TUYA_DP_WDR, IPC_APP_get_wdr_onoff() );
#endif

#ifdef TUYA_DP_NIGHT_MODE
    respone_dp_enum(TUYA_DP_NIGHT_MODE, IPC_APP_get_night_mode() );
#endif

#ifdef TUYA_DP_ALARM_FUNCTION
    respone_dp_bool(TUYA_DP_ALARM_FUNCTION, IPC_APP_get_alarm_function_onoff() );
#endif

#ifdef TUYA_DP_ALARM_SENSITIVITY
    respone_dp_enum(TUYA_DP_ALARM_SENSITIVITY, IPC_APP_get_alarm_sensitivity() );
#endif

#ifdef TUYA_DP_ALARM_ZONE_ENABLE
    respone_dp_bool(TUYA_DP_ALARM_ZONE_ENABLE, IPC_APP_get_alarm_zone_onoff() );
#endif

#ifdef TUYA_DP_ALARM_ZONE_DRAW
    respone_dp_str(TUYA_DP_ALARM_ZONE_DRAW, IPC_APP_get_alarm_zone_draw());
#endif

#ifdef TUYA_DP_SD_STATUS_ONLY_GET
    respone_dp_value(TUYA_DP_SD_STATUS_ONLY_GET, IPC_APP_get_sd_status() );
#endif

#ifdef TUYA_DP_SD_STORAGE_ONLY_GET
    handle_DP_SD_STORAGE_ONLY_GET(NULL);
#endif

#ifdef TUYA_DP_SD_RECORD_ENABLE
    respone_dp_bool(TUYA_DP_SD_RECORD_ENABLE, IPC_APP_get_sd_record_onoff() );
#endif

#ifdef TUYA_DP_SD_RECORD_MODE
    CHAR_T sd_mode[4];
    snprintf(sd_mode,4,"%d",IPC_APP_get_sd_record_mode());
    respone_dp_enum(TUYA_DP_SD_RECORD_MODE, sd_mode);
#endif


#ifdef TUYA_DP_SD_FORMAT_STATUS_ONLY_GET
    respone_dp_value(TUYA_DP_SD_FORMAT_STATUS_ONLY_GET, 0);
#endif
#ifdef TUYA_DP_TEMPERATURE
    respone_dp_value(TUYA_DP_TEMPERATURE, 0);
#endif
#ifdef TUYA_DP_HUMIDITY
    respone_dp_value(TUYA_DP_HUMIDITY, 0);
#endif

#ifdef TUYA_DP_BLUB_SWITCH
    respone_dp_bool(TUYA_DP_BLUB_SWITCH, IPC_APP_get_blub_onoff() );
#endif

#ifdef TUYA_DP_POWERMODE
    IPC_APP_update_battery_status();
#endif

#ifdef TUYA_DP_PERSON_TRACKER
    respone_dp_bool(TUYA_DP_PERSON_TRACKER, IPC_APP_get_person_tracker() );
#endif

#ifdef TUYA_DP_CRY_DETECT
    respone_dp_bool(TUYA_DP_CRY_DETECT, IPC_APP_get_cry_onoff());
#endif

#ifdef TUYA_DP_CRY_SENSI
    respone_dp_enum(TUYA_DP_CRY_SENSI, ((IPC_APP_get_cry_sensi()) ? "1" : "0"));
#endif

#ifdef TUYA_DP_ALARM_SOUND
    respone_dp_enum(TUYA_DP_ALARM_SOUND, IPC_APP_get_alarm_sound());
#endif

#ifdef TUYA_DP_ALARM_CRADLESONG
    respone_dp_bool(TUYA_DP_ALARM_CRADLESONG, IPC_APP_get_alarm_cradlesong_onoff());
#endif

#ifdef TUYA_DP_PLAYMUSIC
    respone_dp_value(TUYA_DP_PLAYMUSIC, IPC_APP_get_playmusic_onoff());
#endif

#ifdef TUYA_DP_HUM_FILTER
    respone_dp_bool(TUYA_DP_HUM_FILTER, IPC_APP_get_human_filter());
#endif

#ifdef TUYA_DP_ALARM_INTERVAL
    respone_dp_enum(TUYA_DP_ALARM_INTERVAL, IPC_APP_get_alarm_interval());
#endif
//#ifdef BABY_PRO
#ifdef TUYA_DP_AO_VOLUME
    respone_dp_value(TUYA_DP_AO_VOLUME,IPC_APP_get_AO_VOLUME());
#endif

#ifdef TUYA_DP_ALBUM_ONOFF
    respone_dp_bool(TUYA_DP_ALBUM_ONOFF,IPC_APP_get_Album_onoff());
#endif
//#endif
#ifdef CONFIG_BABY
#ifdef TUYA_DP_SMILE_SHOT_ONOFF
    respone_dp_bool(TUYA_DP_SMILE_SHOT_ONOFF, IPC_APP_get_smile_shot_onoff());
#endif
#ifdef TUYA_DP_COVER_FACE_ONOFF
    respone_dp_bool(TUYA_DP_COVER_FACE_ONOFF, IPC_APP_get_cover_face_onoff());
#endif
#ifdef TUYA_DP_PERM_ALARM_ONOFF
    respone_dp_bool(TUYA_DP_PERM_ALARM_ONOFF, IPC_APP_get_perm_alarm_onoff());
#endif
#ifdef TUYA_DP_TH_ALARM_ONOFF
    respone_dp_bool(TUYA_DP_TH_ALARM_ONOFF, IPC_APP_get_th_alarm_onoff());
#endif
#ifdef TUYA_DP_PERM_AREA
    respone_dp_str(TUYA_DP_PERM_AREA, IPC_APP_get_perm_area_draw());
#endif
#ifdef TUYA_DP_PERM_AREA_2
    respone_dp_str(TUYA_DP_PERM_AREA_2, IPC_APP_get_perm_area_2_draw());
#endif
#ifdef TUYA_DP_SLEEP_DET_ONOFF
    respone_dp_bool(TUYA_DP_SLEEP_DET_ONOFF, IPC_APP_get_sleep_det_onoff());
#endif
#ifdef TUYA_DP_NURSERY_RHYME
    respone_dp_value(TUYA_DP_NURSERY_RHYME, IPC_APP_get_nursery_rhyme());
#endif
#ifdef TUYA_DP_CUSTOMED_VOICE
    respone_dp_enum(TUYA_DP_CUSTOMED_VOICE, IPC_APP_get_customed_voice_action());
#endif

#ifdef TUYA_DP_ASK_CAMERA
    respone_dp_str(TUYA_DP_ASK_CAMERA, "NULL");
#endif

#ifdef TUYA_DP_PUKE_ONOFF
    respone_dp_bool(TUYA_DP_PUKE_ONOFF, IPC_APP_get_puke_alarm_onoff());
#endif

#ifdef TUYA_DP_KICKQUILT_ONOFF
    respone_dp_bool(TUYA_DP_KICKQUILT_ONOFF, IPC_APP_get_kickquilt_alarm_onoff());
#endif

#endif
}
/* 移动侦测 */
#ifdef TUYA_DP_MOTION_DETECTION_ALARM
VOID IPC_APP_report_move_alarm(CHAR_T *p_val_str)
{
    printf("%s:%d IPC_APP_report_move_alarm snap picture!\r\n", __func__, __LINE__);

    respone_dp_str(TUYA_DP_TEMP_HUMI_ALARM, p_val_str); /* baby monitor 的移动侦测报警dp使用185上报 */
}
#endif

/* 声音报警（哭声检测报警） */
#ifdef TUYA_DP_SOUND_ALARM
VOID IPC_APP_report_sound_alarm(CHAR_T *p_val_str)
{
    printf("%s:%d sound alarm snap picture!\r\n", __func__, __LINE__);

    respone_dp_str(TUYA_DP_SOUND_ALARM, p_val_str);
}
#endif

//相册开关状态上报
#ifdef TUYA_DP_ALBUM_ONOFF 
VOID IPC_APP_report_album_onoff(BOOL_T bValue)
{
    printf("%s:%d IPC_APP_report_album_onoff(%d)!\r\n", __func__, __LINE__,bValue);

    respone_dp_bool(TUYA_DP_ALBUM_ONOFF,bValue);
}

VOID IPC_APP_report_album_alarm(CHAR_T *p_val_str)
{
    printf("%s:%d IPC_APP_report_album_alarm snap picture!\r\n", __func__, __LINE__);

    respone_dp_str(TUYA_DP_ALBUM_ONOFF, p_val_str);
}
#endif

#ifdef CONFIG_BABY
#ifdef TUYA_DP_SMILE_SHOT_ONOFF
VOID IPC_APP_report_smile_snap(CHAR_T *p_val_str)
{
    printf("%s:%d IPC_APP_report_smile_snap picture!\r\n", __func__, __LINE__);

    respone_dp_str(TUYA_DP_SMILE_SHOT_ONOFF, p_val_str);
}
#endif
#ifdef TUYA_DP_COVER_FACE_ONOFF
VOID IPC_APP_report_cover_face_alarm(CHAR_T *p_val_str)
{
    printf("%s:%d IPC_APP_report_cover_face_alarm snap picture!\r\n", __func__, __LINE__);

    respone_dp_str(TUYA_DP_COVER_FACE_ONOFF, p_val_str);
}
#endif
#ifdef TUYA_DP_PERM_ALARM_ONOFF
VOID IPC_APP_report_perm_alarm(CHAR_T *p_val_str)
{
    printf("%s:%d IPC_APP_report_perm_alarm snap picture!\r\n", __func__, __LINE__);

    respone_dp_str(TUYA_DP_PERM_ALARM_ONOFF, p_val_str);
}
#endif
/* 温湿度异常报警 */
#ifdef TUYA_DP_TEMP_HUMI_ALARM
VOID IPC_APP_report_temp_humi_alarm(CHAR_T *p_val_str, INT_T len)
{
    printf("%s:%d temp or humi alarm upload picture!\r\n", __func__, __LINE__);

    respone_dp_raw(TUYA_DP_TEMP_HUMI_ALARM, p_val_str, len);
}
#endif

#ifdef TUYA_DP_SLEEP_DET_ONOFF
VOID IPC_APP_report_sleep_det_alarm(CHAR_T *p_val_str)
{
    printf("%s:%d IPC_APP_report_sleep_det_alarm snap picture!\r\n", __func__, __LINE__);

    respone_dp_str(TUYA_DP_SLEEP_DET_ONOFF, p_val_str);
}
#endif

#ifdef TUYA_DP_PUKE_ONOFF
VOID IPC_APP_report_puke_alarm(CHAR_T *p_val_str)
{
    printf("%s:%d IPC_APP_report_puke_alarm snap picture!\r\n", __func__, __LINE__);

    respone_dp_str(TUYA_DP_PUKE_ONOFF, p_val_str);
}
#endif

#ifdef TUYA_DP_KICKQUILT_ONOFF
VOID IPC_APP_report_kickquilt_alarm(CHAR_T *p_val_str)
{
    printf("%s:%d IPC_APP_report_kickquilt_alarm snap picture!\r\n", __func__, __LINE__);

    respone_dp_str(TUYA_DP_KICKQUILT_ONOFF, p_val_str);
}
#endif

#endif

#ifdef TUYA_DP_SD_FORMAT_STATUS_ONLY_GET
VOID IPC_APP_report_sd_format_status(INT_T status)
{
    respone_dp_value(TUYA_DP_SD_FORMAT_STATUS_ONLY_GET, status);
}
#endif
#ifdef TUYA_DP_TEMPERATURE
VOID IPC_APP_report_temperature_value(INT_T value)
{
    respone_dp_value(TUYA_DP_TEMPERATURE, value);
}
#endif
#ifdef TUYA_DP_HUMIDITY
VOID IPC_APP_report_humidity_value(INT_T value)
{
    respone_dp_value(TUYA_DP_HUMIDITY, value);
}
#endif

#ifdef TUYA_DP_SD_STATUS_ONLY_GET
VOID IPC_APP_report_sd_status_changed(INT_T status)
{
    respone_dp_value(TUYA_DP_SD_STATUS_ONLY_GET, status);
}
#endif

#ifdef TUYA_DP_SD_STORAGE_ONLY_GET
VOID IPC_APP_report_sd_storage()
{
    CHAR_T tmp_str[100] = {0};

    UINT_T total = 100;
    UINT_T used = 0;
    UINT_T empty = 100;
    IPC_APP_get_sd_storage(&total, &used, &empty);

    //"total capacity|Current usage|remaining capacity"
    snprintf(tmp_str, 100, "%u|%u|%u", total, used, empty);
    respone_dp_str(TUYA_DP_SD_STORAGE_ONLY_GET, tmp_str);
}
#endif


#ifdef TUYA_DP_POWERMODE
VOID IPC_APP_update_battery_status(VOID)
{
    CHAR_T *power_mode = IPC_APP_get_power_mode();
    INT_T percent = IPC_APP_get_battery_percent();

    printf("current power mode:%s\r\n", power_mode);
    respone_dp_enum(TUYA_DP_POWERMODE, power_mode);
    printf("current battery percent:%d\r\n", percent);
    respone_dp_value(TUYA_DP_ELECTRICITY, percent);
}
#endif


//------------------------------------------
STATIC VOID respone_dp_value(BYTE_T dp_id, INT_T val)
{
    tuya_ipc_dp_report(NULL, dp_id,PROP_VALUE,&val,1);
}

STATIC VOID respone_dp_bool(BYTE_T dp_id, BOOL_T true_false)
{
    tuya_ipc_dp_report(NULL, dp_id,PROP_BOOL,&true_false,1);
}

STATIC VOID respone_dp_enum(BYTE_T dp_id, CHAR_T *p_val_enum)
{
    tuya_ipc_dp_report(NULL, dp_id,PROP_ENUM,p_val_enum,1);
}

STATIC VOID respone_dp_str(BYTE_T dp_id, CHAR_T *p_val_str)
{
    tuya_ipc_dp_report(NULL, dp_id,PROP_STR,p_val_str,1);
}

STATIC VOID respone_dp_raw(BYTE_T dp_id, CHAR_T *p_val_str, INT_T len)
{
    tuya_ipc_dp_report_raw_sync(NULL, dp_id, p_val_str, len, 120); /* 2min */
}

//------------------------------------------
STATIC BOOL_T check_dp_bool_invalid(IN TY_OBJ_DP_S *p_obj_dp)
{
    if(p_obj_dp == NULL)
    {
        printf("error! input is null \r\n");
        return -1;
    }

    if(p_obj_dp->type != PROP_BOOL)
    {
        printf("error! input is not bool %d \r\n", p_obj_dp->type);
        return -2;
    }

    if(p_obj_dp->value.dp_bool == 0)
    {
        return FALSE;
    }
    else if(p_obj_dp->value.dp_bool == 1)
    {
        return TRUE;
    }else
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->value.dp_bool);
        return -2;
    }
}

//------------------------------------------

#ifdef TUYA_DP_SLEEP_MODE
STATIC VOID handle_DP_SLEEP_MODE(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T sleep_mode = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_sleep_mode(sleep_mode);
    sleep_mode = IPC_APP_get_sleep_mode();

    respone_dp_bool(TUYA_DP_SLEEP_MODE, sleep_mode);
}
#endif

#ifdef TUYA_DP_LIGHT
STATIC VOID handle_DP_LIGHT(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T light_on_off = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_light_onoff(light_on_off);
    light_on_off = IPC_APP_get_light_onoff();

    respone_dp_bool(TUYA_DP_LIGHT, light_on_off);
}
#endif

#ifdef TUYA_DP_FLIP
STATIC VOID handle_DP_FLIP(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T flip_on_off = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_flip_onoff(flip_on_off);
    flip_on_off = IPC_APP_get_flip_onoff();

    respone_dp_bool(TUYA_DP_FLIP, flip_on_off);
    /*上报翻转后的报警区域坐标*/
    respone_dp_str(TUYA_DP_ALARM_ZONE_DRAW, IPC_APP_get_alarm_zone_draw());
#ifdef CONFIG_BABY
    respone_dp_str(TUYA_DP_PERM_AREA, IPC_APP_get_perm_area_draw());
    #ifdef TUYA_DP_ALARM_ZONE_DRAW
    respone_dp_str(TUYA_DP_PERM_AREA_2, IPC_APP_get_perm_area_2_draw());
    #endif
#endif
}
#endif

#ifdef TUYA_DP_WATERMARK
STATIC VOID handle_DP_WATERMARK(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T watermark_on_off = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_watermark_onoff(watermark_on_off);
    watermark_on_off = IPC_APP_get_watermark_onoff();

    respone_dp_bool(TUYA_DP_WATERMARK, watermark_on_off);
}
#endif

#ifdef TUYA_DP_WDR
STATIC VOID handle_DP_WDR(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T wdr_on_off = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_wdr_onoff(wdr_on_off);
    wdr_on_off = IPC_APP_get_wdr_onoff();

    respone_dp_bool(TUYA_DP_WDR, wdr_on_off);
}
#endif

#ifdef TUYA_DP_NIGHT_MODE
STATIC VOID handle_DP_NIGHT_MODE(IN TY_OBJ_DP_S *p_obj_dp)
{
    if( (p_obj_dp == NULL) || (p_obj_dp->type != PROP_ENUM) )
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }
    CHAR_T tmp_str[2] = {0};
    tmp_str[0] = '0' + p_obj_dp->value.dp_enum;

    IPC_APP_set_night_mode(tmp_str);
    CHAR_T *p_night_mode = IPC_APP_get_night_mode();

    respone_dp_enum(TUYA_DP_NIGHT_MODE, p_night_mode);
}
#endif


#ifdef TUYA_DP_ALARM_FUNCTION
STATIC VOID handle_DP_ALARM_FUNCTION(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T alarm_on_off = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_alarm_function_onoff(alarm_on_off);
    alarm_on_off = IPC_APP_get_alarm_function_onoff();

    respone_dp_bool(TUYA_DP_ALARM_FUNCTION, alarm_on_off);
}
#endif

#ifdef TUYA_DP_ALARM_SENSITIVITY
STATIC VOID handle_DP_ALARM_SENSITIVITY(IN TY_OBJ_DP_S *p_obj_dp)
{
    if( (p_obj_dp == NULL) || (p_obj_dp->type != PROP_ENUM) )
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }

    CHAR_T tmp_str[2] = {0};
    tmp_str[0] = '0' + p_obj_dp->value.dp_enum;

    IPC_APP_set_alarm_sensitivity(tmp_str);
    CHAR_T *p_sensitivity = IPC_APP_get_alarm_sensitivity();

    respone_dp_enum(TUYA_DP_ALARM_SENSITIVITY, p_sensitivity);
}
#endif

#ifdef TUYA_DP_ALARM_INTERVAL
STATIC VOID handle_DP_ALARM_INTERVAL(IN TY_OBJ_DP_S *p_obj_dp)
{
    if( (p_obj_dp == NULL) || (p_obj_dp->type != PROP_ENUM) )
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }

    CHAR_T tmp_str[2] = {0};
    tmp_str[0] = '0' + p_obj_dp->value.dp_enum;

    IPC_APP_set_alarm_interval(tmp_str);
    CHAR_T *p_alarmInterval = IPC_APP_get_alarm_interval();

    respone_dp_enum(TUYA_DP_ALARM_INTERVAL, p_alarmInterval);
}
#endif

#ifdef TUYA_DP_ALARM_ZONE_ENABLE
STATIC VOID handle_DP_ALARM_ZONE_ENABLE(IN TY_OBJ_DP_S *p_dp_json)
{
    if(p_dp_json == NULL )
    {
        printf("Error!! type invalid %p \r\n", p_dp_json);
        return;
    }
    BOOL_T alarm_zone_enable = check_dp_bool_invalid(p_dp_json);
    IPC_APP_set_alarm_zone_onoff(alarm_zone_enable);
    respone_dp_bool(TUYA_DP_ALARM_ZONE_ENABLE, IPC_APP_get_alarm_zone_onoff());
}
#endif

#ifdef TUYA_DP_ALARM_ZONE_DRAW
STATIC VOID handle_DP_ALARM_ZONE_DRAW(IN TY_OBJ_DP_S *p_dp_json)
{
    if(p_dp_json == NULL )
    {
        printf("Error!! type invalid\r\n");
        return;
    }
    IPC_APP_set_alarm_zone_draw((cJSON *)(p_dp_json->value.dp_str));
    respone_dp_str(TUYA_DP_ALARM_ZONE_DRAW, IPC_APP_get_alarm_zone_draw());
}
#endif

#ifdef TUYA_DP_SD_STATUS_ONLY_GET
STATIC VOID handle_DP_SD_STATUS_ONLY_GET(IN TY_OBJ_DP_S *p_obj_dp)
{
    INT_T sd_status = IPC_APP_get_sd_status();

    respone_dp_value(TUYA_DP_SD_STATUS_ONLY_GET, sd_status);
}
#endif

#ifdef TUYA_DP_SD_STORAGE_ONLY_GET
STATIC VOID handle_DP_SD_STORAGE_ONLY_GET(IN TY_OBJ_DP_S *p_obj_dp)
{
    CHAR_T tmp_str[100] = {0};

    UINT_T total = 100;
    UINT_T used = 0;
    UINT_T empty = 100;
    IPC_APP_get_sd_storage(&total, &used, &empty);

    //"total capacity|Current usage|remaining capacity"
    snprintf(tmp_str, 100, "%u|%u|%u", total, used, empty);
    respone_dp_str(TUYA_DP_SD_STORAGE_ONLY_GET, tmp_str);
}
#endif

#ifdef TUYA_DP_SD_RECORD_ENABLE
STATIC VOID handle_DP_SD_RECORD_ENABLE(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T sd_record_on_off = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_sd_record_onoff(sd_record_on_off);
    sd_record_on_off = IPC_APP_get_sd_record_onoff();

    respone_dp_bool(TUYA_DP_SD_RECORD_ENABLE, sd_record_on_off);
}
#endif

#ifdef TUYA_DP_SD_RECORD_MODE
STATIC VOID handle_DP_SD_RECORD_MODE(IN TY_OBJ_DP_S *p_obj_dp)
{
    if( (p_obj_dp == NULL) || (p_obj_dp->type != PROP_ENUM) )
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }

    IPC_APP_set_sd_record_mode(p_obj_dp->value.dp_enum);
    UINT_T mode = IPC_APP_get_sd_record_mode();
    CHAR_T sMode[2];
    snprintf(sMode,2,"%d",mode);
    respone_dp_enum(TUYA_DP_SD_RECORD_MODE,sMode);
}
#endif

#ifdef TUYA_DP_SD_UMOUNT
STATIC VOID handle_DP_SD_UMOUNT(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T umount_result = IPC_APP_unmount_sd_card();
    respone_dp_bool(TUYA_DP_SD_UMOUNT, umount_result);
}
#endif

#ifdef TUYA_DP_SD_FORMAT
STATIC VOID handle_DP_SD_FORMAT(IN TY_OBJ_DP_S *p_obj_dp)
{
    IPC_APP_format_sd_card();
    respone_dp_bool(TUYA_DP_SD_FORMAT, TRUE);
}
#endif

#ifdef TUYA_DP_PERSON_TRACKER
STATIC VOID handle_DP_PERSONTRACKER(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T person_tracker = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_person_tracker(person_tracker);
    person_tracker = IPC_APP_get_person_tracker();

    respone_dp_bool(TUYA_DP_PERSON_TRACKER, person_tracker);
}
#endif

#ifdef TUYA_DP_SD_FORMAT_STATUS_ONLY_GET
STATIC VOID handle_DP_SD_FORMAT_STATUS_ONLY_GET(IN TY_OBJ_DP_S *p_obj_dp)
{
    INT_T progress = IPC_APP_get_sd_format_status();
    respone_dp_value(TUYA_DP_SD_FORMAT_STATUS_ONLY_GET, progress);
}
#endif

#ifdef TUYA_DP_TEMPERATURE
STATIC VOID handle_DP_Temperature_ONLY_GET(IN TY_OBJ_DP_S *p_obj_dp)
{
    INT_T progress = IPC_APP_get_sd_format_status();
    respone_dp_value(TUYA_DP_TEMPERATURE, progress);
}
#endif
#ifdef TUYA_DP_HUMIDITY
STATIC VOID handle_DP_Humidity_ONLY_GET(IN TY_OBJ_DP_S *p_obj_dp)
{
    INT_T progress = IPC_APP_get_sd_format_status();
    respone_dp_value(TUYA_DP_HUMIDITY, progress);
}
#endif

#ifdef TUYA_DP_PTZ_CONTROL
STATIC VOID handle_DP_PTZ_CONTROL(IN TY_OBJ_DP_S *p_obj_dp)
{
    if( (p_obj_dp == NULL) || (p_obj_dp->type != PROP_ENUM) )
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }

    //dp 119 format: {"range":["1","2","3","4","5","6","7","0"],"type":"enum"}
    UINT_T dp_directions[8] = {0,1,2,3,4,5,6,7};
    UINT_T direction = dp_directions[p_obj_dp->value.dp_enum];
    CHAR_T tmp_str[2] = {0};
    snprintf(tmp_str,2,"%d",direction);    
    IPC_APP_ptz_start_move(tmp_str);
    respone_dp_enum(TUYA_DP_PTZ_CONTROL,tmp_str);
}
#endif

#ifdef TUYA_DP_PTZ_STOP
STATIC VOID handle_DP_PTZ_STOP(IN TY_OBJ_DP_S *p_obj_dp)
{
    IPC_APP_ptz_stop_move();
    respone_dp_bool(TUYA_DP_PTZ_STOP, TRUE);
}
#endif

#ifdef TUYA_DP_PTZ_CHECK
STATIC VOID handle_DP_PTZ_CHECK(IN TY_OBJ_DP_S *p_obj_dp)
{
    IPC_APP_ptz_check();
    respone_dp_bool(TUYA_DP_PTZ_CHECK, TRUE);
}
#endif

#ifdef TUYA_DP_TRACK_ENABLE
STATIC VOID handle_DP_TRACK_ENABLE(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T track_enable = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_track_enable(track_enable);

    respone_dp_bool(TUYA_DP_TRACK_ENABLE, track_enable);
}

#endif

#ifdef TUYA_DP_LINK_MOVE_ACTION
STATIC VOID handle_DP_LINK_MOVE(IN TY_OBJ_DP_S *p_obj_dp)
{
    if( (p_obj_dp == NULL) || (p_obj_dp->type != PROP_ENUM) )
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }

    CHAR_T tmp_str[2] = {0};
    int bind_move = 0;
    bind_move = p_obj_dp->value.dp_enum;
    tmp_str[0] = '0' + p_obj_dp->value.dp_enum;

    IPC_APP_set_link_move(bind_move);
    respone_dp_enum(TUYA_DP_LINK_MOVE_ACTION, tmp_str);
}
#endif

#ifdef TUYA_DP_LINK_MOVE_SET
STATIC VOID handle_DP_LINK_MOVE_SET(IN TY_OBJ_DP_S *p_obj_dp)
{
    if( (p_obj_dp == NULL) || (p_obj_dp->type != PROP_ENUM) )
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }

    CHAR_T tmp_str[2] = {0};
    int bind_move = 0;
    bind_move = p_obj_dp->value.dp_enum;
    tmp_str[0] = '0' + p_obj_dp->value.dp_enum;

    IPC_APP_set_link_pos(bind_move);
    respone_dp_enum(TUYA_DP_LINK_MOVE_SET, tmp_str);
}
#endif


#ifdef TUYA_DP_HUM_FILTER
STATIC VOID handle_DP_HUM_FILTER(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T hum_filter = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_human_filter(hum_filter);
    hum_filter = IPC_APP_get_human_filter();
 
    respone_dp_bool(TUYA_DP_HUM_FILTER, hum_filter);
}
#endif

#ifdef TUYA_DP_PATROL_MODE
STATIC VOID handle_DP_patrol_mode(IN TY_OBJ_DP_S *p_obj_dp)
{
    if( (p_obj_dp == NULL) || (p_obj_dp->type != PROP_ENUM) )
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }
    /*PTZ巡航模式 0自动巡航 1按预设巡航*/
    CHAR_T tmpBuf[2] = {0};

    sprintf(tmpBuf,"%d", p_obj_dp->value.dp_enum);
    IPC_APP_set_patrol_mode(tmpBuf);
    respone_dp_enum(TUYA_DP_PATROL_MODE,(IPC_APP_get_patrol_mode() ? "1" : "0"));
}


#endif

#ifdef TUYA_DP_PATROL_SWITCH
STATIC VOID handle_DP_patrol_switch(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T patrol_mode = TRUE;

    CHAR_T  tmpBuf[2] = {0};

    sprintf(tmpBuf, "%d", p_obj_dp->value.dp_bool);
    IPC_APP_set_patrol_switch(tmpBuf);

    patrol_mode = IPC_APP_get_patrol_switch();
    respone_dp_bool(TUYA_DP_PATROL_SWITCH, patrol_mode);
}
#endif

#ifdef TUYA_DP_PATROL_TMODE
STATIC VOID handle_DP_patrol_tmode(IN TY_OBJ_DP_S *p_obj_dp)
{
    if( (p_obj_dp == NULL) || (p_obj_dp->type != PROP_ENUM) )
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }

    CHAR_T tmpBuf[2] = {0};

    sprintf(tmpBuf, "%d", p_obj_dp->value.dp_enum); 
    IPC_APP_set_patrol_tmode(tmpBuf);
    respone_dp_enum(TUYA_DP_PATROL_TMODE, (IPC_APP_get_patrol_tmode() ? "1" : "0"));
}
#endif

#ifdef TUYA_DP_PATROL_TIME
STATIC VOID handle_DP_patrol_time(IN TY_OBJ_DP_S *p_dp_json)
{
    char buf[128]={0};
    char ptStart[64]={0};
    char ptEnd[64]={0};

    printf("patrol time [%s]\n",p_dp_json->value.dp_str);
    IPC_APP_set_patrol_time((cJSON *)(p_dp_json->value.dp_str));

    IPC_APP_get_patrol_time(TUYA_IPC_DP_HANDLER_PT_START, ptStart);
    IPC_APP_get_patrol_time(TUYA_IPC_DP_HANDLER_PT_END, ptEnd);
    printf("teset------------------------ptStart %s\n", ptStart);
    printf("teset------------------------ptEnd %s\n", ptEnd);
    snprintf(buf,128,"{\\\"t_start\\\":\\\"%s\\\",\\\"t_end\\\":\\\"%s\\\"}", ptStart, ptEnd);

    respone_dp_str(TUYA_DP_PATROL_TIME, buf);
}
#endif

#ifdef TUYA_DP_PATROL_STATE
STATIC VOID handle_DP_patrol_state(IN TY_OBJ_DP_S *p_dp_json)
{
    int patrol_state = 0;
    //printf("---get_patrol_state\n");
    IPC_APP_patrol_state(&patrol_state);
    printf("---get_patrol_state:%d\n",patrol_state);

    CHAR_T sd_mode[4];
    snprintf(sd_mode,4,"%d",patrol_state);
    respone_dp_enum(TUYA_DP_PATROL_STATE, sd_mode);
    return ;
}
#endif

#ifdef TUYA_DP_PRESET_SET
STATIC VOID handle_DP_SET_PRESET(IN TY_OBJ_DP_S *p_dp_json)
{
  
    IPC_APP_set_preset((cJSON *)(p_dp_json->value.dp_str));
    return;
}
#endif


#ifdef TUYA_DP_DOOR_BELL
STATIC VOID handle_DP_DOOR_BELL(IN TY_OBJ_DP_S *p_obj_dp)
{
    printf("error! door bell can only trigged by IPC side.\r\n");
    respone_dp_str(TUYA_DP_DOOR_BELL, "-1");
}
#endif

#ifdef TUYA_DP_BLUB_SWITCH
STATIC VOID handle_DP_BLUB_SWITCH(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T blub_on_off = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_blub_onoff(blub_on_off);
    blub_on_off = IPC_APP_get_blub_onoff();

    respone_dp_bool(TUYA_DP_BLUB_SWITCH, blub_on_off);
}
#endif

#ifdef TUYA_DP_ELECTRICITY
STATIC VOID handle_DP_ELECTRICITY(IN TY_OBJ_DP_S *p_obj_dp)
{
    INT_T percent = IPC_APP_get_battery_percent();
    printf("current battery percent:%d\r\n", percent);
    respone_dp_value(TUYA_DP_ELECTRICITY, percent);
}
#endif

#ifdef TUYA_DP_POWERMODE
STATIC VOID handle_DP_POWERMODE(IN TY_OBJ_DP_S *p_obj_dp)
{
    CHAR_T *power_mode = IPC_APP_get_power_mode();
    printf("current power mode:%s\r\n", power_mode);
    respone_dp_enum(TUYA_DP_POWERMODE, power_mode);
}
#endif

#ifdef TUYA_DP_SOUND_DETECT
STATIC VOID handle_DP_SOUND_DETECT(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T sound_onoff = check_dp_bool_invalid(p_obj_dp);

    printf("sound detect onoff:%d\r\n", sound_onoff);

    IPC_APP_set_sound_onoff(sound_onoff);
    sound_onoff = IPC_APP_get_sound_onoff();

    respone_dp_bool(TUYA_DP_SOUND_DETECT, sound_onoff);
}
#endif
#ifdef TUYA_DP_SOUND_SENSITIVITY
STATIC VOID handle_DP_SOUND_SENSI(IN TY_OBJ_DP_S *p_obj_dp)
{
    if ((p_obj_dp == NULL) || (p_obj_dp->type != PROP_ENUM))
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }

    BOOL_T sound_sensi = p_obj_dp->value.dp_enum;

    printf("sound detect sensitivity:%d\r\n", sound_sensi);

    IPC_APP_set_sound_sensi(sound_sensi);
    sound_sensi = IPC_APP_get_sound_sensi();

    respone_dp_enum(TUYA_DP_SOUND_SENSITIVITY, (sound_sensi ? "1" : "0"));
}
#endif
#ifdef TUYA_DP_CRY_DETECT
STATIC VOID handle_DP_CRY_ONOFF(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T cry_onoff = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_cry_onoff(cry_onoff);
    cry_onoff = IPC_APP_get_cry_onoff();

    respone_dp_bool(TUYA_DP_CRY_DETECT, cry_onoff);
}
#endif
#ifdef TUYA_DP_CRY_SENSI
STATIC VOID handle_DP_CRY_SENSI(IN TY_OBJ_DP_S *p_obj_dp)
{
    if ((p_obj_dp == NULL) || (p_obj_dp->type != PROP_ENUM))
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }

    BOOL_T cry_sensi = p_obj_dp->value.dp_enum;

    printf("cry detect sensitivity:%d\r\n", cry_sensi);

    IPC_APP_set_cry_sensi(cry_sensi);
    cry_sensi = IPC_APP_get_cry_sensi();

    respone_dp_enum(TUYA_DP_CRY_SENSI, (cry_sensi ? "1" : "0"));
}
#endif

#ifdef TUYA_DP_REBOOT
STATIC VOID handle_DP_REBOOT(IN TY_OBJ_DP_S *p_obj_dp)
{
    if (check_dp_bool_invalid(p_obj_dp))
    {
        respone_dp_bool(TUYA_DP_REBOOT, 1);
        IPC_APP_set_reboot();
    }

}
#endif
#ifdef TUYA_DP_FACTORY
STATIC VOID handle_DP_FACTORY(IN TY_OBJ_DP_S *p_obj_dp)
{
    if (check_dp_bool_invalid(p_obj_dp))
    {
        respone_dp_bool(TUYA_DP_FACTORY, 1);
        IPC_APP_set_factory();
    }
}
#endif

//#ifdef TUYA_DP_CRADLESONG
//STATIC VOID handle_DP_CRADLESONG(IN TY_OBJ_DP_S *p_obj_dp)
//{
//    BOOL_T cradlesong = check_dp_bool_invalid(p_obj_dp);

//    IPC_APP_set_cradlesong_onoff(cradlesong);
//}
//#endif

#ifdef TUYA_DP_PLAYMUSIC
STATIC VOID handle_DP_PLAYMUSIC(IN TY_OBJ_DP_S *p_obj_dp)
{
    if ((p_obj_dp == NULL) || (p_obj_dp->type != PROP_VALUE))
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }
    if (TRUE == IPC_APP_get_sleep_mode())
    {
        printf("warning:play Music fail: sleep_mode \r\n");
        return;
    }

    INT_T playmusic = p_obj_dp->value.dp_enum;

    respone_dp_value(TUYA_DP_PLAYMUSIC, playmusic);
    //printf("playmusic=%d \n", playmusic);
    IPC_APP_set_playmusic_onoff(playmusic);
}
#endif

#ifdef TUYA_DP_ALARM_CRADLESONG
STATIC VOID handle_DP_ALARM_CRADLESONG(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T alarm_cradlesong = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_alarm_cradlesong_onoff(alarm_cradlesong);
    alarm_cradlesong = IPC_APP_get_alarm_cradlesong_onoff();

    respone_dp_bool(TUYA_DP_ALARM_CRADLESONG, alarm_cradlesong);
}
#endif

#ifdef TUYA_DP_ALARM_SOUND
STATIC VOID handle_DP_ALARM_SOUND(IN TY_OBJ_DP_S *p_obj_dp)
{
    if ((p_obj_dp == NULL) || (p_obj_dp->type != PROP_ENUM))
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }

    CHAR_T tmp_str[2] = {0};
    tmp_str[0] = '0' + p_obj_dp->value.dp_enum;

    IPC_APP_set_alarm_sound(tmp_str);

    CHAR_T *p_alarmSound = IPC_APP_get_alarm_sound();

    respone_dp_enum(TUYA_DP_ALARM_SOUND, p_alarmSound);
}
#endif

//#ifdef BABY_PRO
#ifdef TUYA_DP_AO_VOLUME
STATIC VOID handle_DP_AO_VOLUME(IN TY_OBJ_DP_S *p_obj_dp)
{
    if ((p_obj_dp == NULL) || (p_obj_dp->type != PROP_VALUE))
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }
    if (TRUE == IPC_APP_get_sleep_mode())
    {
        printf("warning:set VOLUME fail: sleep_mode \r\n");
        return;
    }
    INT_T AO_VALUE = p_obj_dp->value.dp_value;
    IPC_APP_set_AO_VOLUME(AO_VALUE);

    respone_dp_value(TUYA_DP_AO_VOLUME, IPC_APP_get_AO_VOLUME());
}
#endif

#ifdef TUYA_DP_ALBUM_ONOFF
STATIC VOID handle_DP_ALBUM_ONOFF(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T albumOnoff = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_Album_onoff(albumOnoff);

    respone_dp_bool(TUYA_DP_ALBUM_ONOFF, IPC_APP_get_Album_onoff());
}
#endif
//#endif
STATIC VOID handle_DP_RESERVED(IN TY_OBJ_DP_S *p_obj_dp)
{
    printf("error! not implememt yet. type:%d\r\n", p_obj_dp->type);
}

/****************************baby monitor start************************************/
#ifdef CONFIG_BABY
#ifdef TUYA_DP_SMILE_SHOT_ONOFF
STATIC VOID handle_DP_SmileShot_ONOFF(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T smile_shot_onoff = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_smile_shot_onoff(smile_shot_onoff);
    smile_shot_onoff = IPC_APP_get_smile_shot_onoff();

    respone_dp_bool(TUYA_DP_SMILE_SHOT_ONOFF, smile_shot_onoff);
}
#endif
#ifdef TUYA_DP_COVER_FACE_ONOFF
STATIC VOID handle_DP_CoverFace_ONOFF(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T cover_face_onoff = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_cover_face_onoff(cover_face_onoff);
    cover_face_onoff = IPC_APP_get_cover_face_onoff();

    respone_dp_bool(TUYA_DP_COVER_FACE_ONOFF, cover_face_onoff);
}
#endif
#ifdef TUYA_DP_PERM_ALARM_ONOFF
STATIC VOID handle_DP_PermAlarm_ONOFF(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T perm_alarm_onoff = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_perm_alarm_onoff(perm_alarm_onoff);
    perm_alarm_onoff = IPC_APP_get_perm_alarm_onoff();

    respone_dp_bool(TUYA_DP_PERM_ALARM_ONOFF, perm_alarm_onoff);
}
#endif
#ifdef TUYA_DP_TH_ALARM_ONOFF
STATIC VOID handle_DP_TH_Alarm_ONOFF(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T th_alarm_onoff = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_th_alarm_onoff(th_alarm_onoff);
    th_alarm_onoff = IPC_APP_get_th_alarm_onoff();

    respone_dp_bool(TUYA_DP_TH_ALARM_ONOFF, th_alarm_onoff);
}
#endif
#ifdef TUYA_DP_PERM_AREA
STATIC VOID handle_DP_PermArea_DRAW(IN TY_OBJ_DP_S *p_dp_json)
{
    if (p_dp_json == NULL)
    {
        printf("Error!! type invalid\r\n");
        return;
    }
    IPC_APP_set_perm_area_draw((cJSON *)(p_dp_json->value.dp_str));
    respone_dp_str(TUYA_DP_PERM_AREA, IPC_APP_get_perm_area_draw());
}
#endif
#ifdef TUYA_DP_PERM_AREA_2
STATIC VOID handle_DP_PermArea_DRAW_2(IN TY_OBJ_DP_S *p_dp_json)
{
    if (p_dp_json == NULL)
    {
        printf("Error!! type invalid\r\n");
        return;
    }
    IPC_APP_set_perm_area_2_draw((cJSON *)(p_dp_json->value.dp_str));
    respone_dp_str(TUYA_DP_PERM_AREA_2, IPC_APP_get_perm_area_2_draw());
}
#endif
#ifdef TUYA_DP_SLEEP_DET_ONOFF
STATIC VOID handle_DP_sleepDet_ONOFF(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T sleep_det_onoff = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_sleep_det_onoff(sleep_det_onoff);
    sleep_det_onoff = IPC_APP_get_sleep_det_onoff();

    //respone_dp_bool(TUYA_DP_COVER_FACE_ONOFF, sleep_det_onoff);
    respone_dp_bool(TUYA_DP_SLEEP_DET_ONOFF, sleep_det_onoff);//cxj test
}
#endif
#ifdef TUYA_DP_NURSERY_RHYME
STATIC VOID handle_DP_NURSERY_RHYME(IN TY_OBJ_DP_S *p_obj_dp)
{
    if ((p_obj_dp == NULL) || (p_obj_dp->type != PROP_VALUE))
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }

    INT_T tmp_val = p_obj_dp->value.dp_enum;

    respone_dp_value(TUYA_DP_NURSERY_RHYME, tmp_val);
    //printf("tmp_val=%d \n", tmp_val);
    IPC_APP_set_nursery_rhyme(tmp_val);

}
#endif
#ifdef TUYA_DP_CUSTOMED_VOICE
STATIC VOID handle_DP_CUSTOMED_VOICE(IN TY_OBJ_DP_S *p_obj_dp)
{
    if ((p_obj_dp == NULL) || (p_obj_dp->type != PROP_ENUM))
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }

    CHAR_T tmp_str[2] = {0};
    tmp_str[0] = '0' + p_obj_dp->value.dp_enum;

    IPC_APP_set_customed_voice_action(tmp_str);

    CHAR_T *p_action = IPC_APP_get_customed_voice_action();

    respone_dp_enum(TUYA_DP_CUSTOMED_VOICE, p_action);
}
#endif
#ifdef TUYA_DP_ASK_CAMERA
STATIC VOID handle_DP_ASK_CAMERA(IN TY_OBJ_DP_S *p_dp_cmd)
{
    if (p_dp_cmd == NULL)
    {
        printf("Error!! type invalid\r\n");
        return;
    }
    IPC_APP_set_cmd_ask_camera(p_dp_cmd->value.dp_str);
    respone_dp_str(TUYA_DP_ASK_CAMERA, IPC_APP_get_ret_ask_camera());

}
#endif

#ifdef TUYA_DP_PUKE_ONOFF
STATIC VOID handle_DP_PukeAlarm_ONOFF(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T puke_alarm_onoff = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_puke_alarm_onoff(puke_alarm_onoff);
    puke_alarm_onoff = IPC_APP_get_puke_alarm_onoff();

    respone_dp_bool(TUYA_DP_PUKE_ONOFF, puke_alarm_onoff);
}
#endif

#ifdef TUYA_DP_KICKQUILT_ONOFF
STATIC VOID handle_DP_KickQuiltAlarm_ONOFF(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T kickquilt_onoff = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_kickquilt_alarm_onoff(kickquilt_onoff);
    kickquilt_onoff = IPC_APP_get_kickquilt_alarm_onoff();

    respone_dp_bool(TUYA_DP_KICKQUILT_ONOFF, kickquilt_onoff);
}
#endif

#endif


/****************************baby monitor end************************************/

typedef VOID (*TUYA_DP_HANDLER)(IN TY_OBJ_DP_S *p_obj_dp);
typedef struct
{
    BYTE_T dp_id;
    TUYA_DP_HANDLER handler;
}TUYA_DP_INFO_S;

STATIC TUYA_DP_INFO_S s_dp_table[] =
    {
#ifdef TUYA_DP_SLEEP_MODE
        {TUYA_DP_SLEEP_MODE, handle_DP_SLEEP_MODE},
#endif
#ifdef TUYA_DP_LIGHT
        {TUYA_DP_LIGHT, handle_DP_LIGHT},
#endif

#ifdef TUYA_DP_REBOOT
        {TUYA_DP_REBOOT, handle_DP_REBOOT},
#endif
#ifdef TUYA_DP_FACTORY
        {TUYA_DP_FACTORY, handle_DP_FACTORY},
#endif

#ifdef TUYA_DP_FLIP
        {TUYA_DP_FLIP, handle_DP_FLIP},
#endif
#ifdef TUYA_DP_WATERMARK
        {TUYA_DP_WATERMARK, handle_DP_WATERMARK},
#endif
#ifdef TUYA_DP_WDR
        {TUYA_DP_WDR, handle_DP_WDR},
#endif
#ifdef TUYA_DP_NIGHT_MODE
        {TUYA_DP_NIGHT_MODE, handle_DP_NIGHT_MODE},
#endif
#ifdef TUYA_DP_ALARM_FUNCTION
        {TUYA_DP_ALARM_FUNCTION, handle_DP_ALARM_FUNCTION},
#endif
#ifdef TUYA_DP_ALARM_SENSITIVITY
        {TUYA_DP_ALARM_SENSITIVITY, handle_DP_ALARM_SENSITIVITY},
#endif
#ifdef TUYA_DP_ALARM_INTERVAL
        {TUYA_DP_ALARM_INTERVAL, handle_DP_ALARM_INTERVAL},
#endif
#ifdef TUYA_DP_ALARM_ZONE_ENABLE
        {TUYA_DP_ALARM_ZONE_ENABLE, handle_DP_ALARM_ZONE_ENABLE},
#endif

#ifdef TUYA_DP_ALARM_ZONE_DRAW
        {TUYA_DP_ALARM_ZONE_DRAW, handle_DP_ALARM_ZONE_DRAW},
#endif

#ifdef TUYA_DP_SD_STATUS_ONLY_GET
        {TUYA_DP_SD_STATUS_ONLY_GET, handle_DP_SD_STATUS_ONLY_GET},
#endif
#ifdef TUYA_DP_SD_STORAGE_ONLY_GET
        {TUYA_DP_SD_STORAGE_ONLY_GET, handle_DP_SD_STORAGE_ONLY_GET},
#endif
#ifdef TUYA_DP_SD_RECORD_ENABLE
        {TUYA_DP_SD_RECORD_ENABLE, handle_DP_SD_RECORD_ENABLE},
#endif
#ifdef TUYA_DP_SD_RECORD_MODE
        {TUYA_DP_SD_RECORD_MODE, handle_DP_SD_RECORD_MODE},
#endif
#ifdef TUYA_DP_SD_UMOUNT
        {TUYA_DP_SD_UMOUNT, handle_DP_SD_UMOUNT},
#endif
#ifdef TUYA_DP_SD_FORMAT
        {TUYA_DP_SD_FORMAT, handle_DP_SD_FORMAT},
#endif
#ifdef TUYA_DP_SD_FORMAT_STATUS_ONLY_GET
        {TUYA_DP_SD_FORMAT_STATUS_ONLY_GET, handle_DP_SD_FORMAT_STATUS_ONLY_GET},
#endif
#ifdef TUYA_DP_PTZ_CONTROL
        {TUYA_DP_PTZ_CONTROL, handle_DP_PTZ_CONTROL},
#endif
#ifdef TUYA_DP_PTZ_STOP
        {TUYA_DP_PTZ_STOP, handle_DP_PTZ_STOP},
#endif
#ifdef TUYA_DP_PTZ_CHECK
        {TUYA_DP_PTZ_CHECK, handle_DP_PTZ_CHECK},
#endif
#ifdef TUYA_DP_TRACK_ENABLE
        {TUYA_DP_TRACK_ENABLE, handle_DP_TRACK_ENABLE},
#endif
#ifdef TUYA_DP_HUM_FILTER
        {TUYA_DP_HUM_FILTER, handle_DP_HUM_FILTER},
#endif
#ifdef TUYA_DP_PATROL_MODE
        {TUYA_DP_PATROL_MODE, handle_DP_patrol_mode},
#endif
#ifdef TUYA_DP_PATROL_SWITCH
        {TUYA_DP_PATROL_SWITCH, handle_DP_patrol_switch},
#endif
#ifdef TUYA_DP_PATROL_TMODE
        {TUYA_DP_PATROL_TMODE, handle_DP_patrol_tmode},
#endif
#ifdef TUYA_DP_PATROL_TIME
        {TUYA_DP_PATROL_TIME, handle_DP_patrol_time},
#endif

#ifdef TUYA_DP_PATROL_STATE
        {TUYA_DP_PATROL_STATE, handle_DP_patrol_state},
#endif

#ifdef TUYA_DP_PRESET_SET
        {TUYA_DP_PRESET_SET, handle_DP_SET_PRESET},
#endif

#ifdef TUYA_DP_LINK_MOVE_ACTION
        {TUYA_DP_LINK_MOVE_ACTION, handle_DP_LINK_MOVE},
#endif
#ifdef TUYA_DP_LINK_MOVE_SET
        {TUYA_DP_LINK_MOVE_SET, handle_DP_LINK_MOVE_SET},
#endif

#ifdef TUYA_DP_DOOR_BELL
        {TUYA_DP_DOOR_BELL, handle_DP_DOOR_BELL},
#endif
#ifdef TUYA_DP_BLUB_SWITCH
        {TUYA_DP_BLUB_SWITCH, handle_DP_BLUB_SWITCH},
#endif
#ifdef TUYA_DP_SOUND_DETECT
        {TUYA_DP_SOUND_DETECT, handle_DP_SOUND_DETECT},
#endif
#ifdef TUYA_DP_SOUND_SENSITIVITY
        {TUYA_DP_SOUND_SENSITIVITY, handle_DP_SOUND_SENSI},
#endif
#ifdef TUYA_DP_SOUND_ALARM
        {TUYA_DP_SOUND_ALARM, handle_DP_RESERVED},
#endif
#ifdef TUYA_DP_TEMPERATURE
        {TUYA_DP_TEMPERATURE, handle_DP_Temperature_ONLY_GET},
#endif
#ifdef TUYA_DP_HUMIDITY
        {TUYA_DP_HUMIDITY, handle_DP_Humidity_ONLY_GET},
#endif
#ifdef TUYA_DP_ELECTRICITY
        {TUYA_DP_ELECTRICITY, handle_DP_ELECTRICITY},
#endif
#ifdef TUYA_DP_POWERMODE
        {TUYA_DP_POWERMODE, handle_DP_POWERMODE},
#endif
#ifdef TUYA_DP_LOWELECTRIC
        {TUYA_DP_LOWELECTRIC, handle_DP_RESERVED},
#endif

#ifdef TUYA_DP_PERSON_TRACKER
        {TUYA_DP_PERSON_TRACKER, handle_DP_PERSONTRACKER},
#endif


#ifdef TUYA_DP_CRY_DETECT
        {TUYA_DP_CRY_DETECT, handle_DP_CRY_ONOFF},
#endif
#ifdef TUYA_DP_CRY_SENSI
        {TUYA_DP_CRY_SENSI, handle_DP_CRY_SENSI},
#endif
#ifdef TUYA_DP_ALARM_SOUND
        {TUYA_DP_ALARM_SOUND, handle_DP_ALARM_SOUND},
#endif
#ifdef TUYA_DP_ALARM_CRADLESONG
        {TUYA_DP_ALARM_CRADLESONG, handle_DP_ALARM_CRADLESONG},
#endif
//#ifdef TUYA_DP_CRADLESONG
//        {TUYA_DP_CRADLESONG, handle_DP_CRADLESONG},
//#endif
#ifdef TUYA_DP_PLAYMUSIC
        {TUYA_DP_PLAYMUSIC, handle_DP_PLAYMUSIC},
#endif
//#ifdef BABY_PRO
#ifdef TUYA_DP_AO_VOLUME
        {TUYA_DP_AO_VOLUME,handle_DP_AO_VOLUME},
#endif
#ifdef TUYA_DP_ALBUM_ONOFF
        {TUYA_DP_ALBUM_ONOFF,handle_DP_ALBUM_ONOFF},
#endif
//#endif
#ifdef CONFIG_BABY
#ifdef TUYA_DP_SMILE_SHOT_ONOFF
        {TUYA_DP_SMILE_SHOT_ONOFF, handle_DP_SmileShot_ONOFF},
#endif
#ifdef TUYA_DP_COVER_FACE_ONOFF
        {TUYA_DP_COVER_FACE_ONOFF, handle_DP_CoverFace_ONOFF},
#endif
#ifdef TUYA_DP_PERM_ALARM_ONOFF
        {TUYA_DP_PERM_ALARM_ONOFF, handle_DP_PermAlarm_ONOFF},
#endif
#ifdef TUYA_DP_TH_ALARM_ONOFF
        {TUYA_DP_TH_ALARM_ONOFF, handle_DP_TH_Alarm_ONOFF},
#endif
#ifdef TUYA_DP_PERM_AREA
        {TUYA_DP_PERM_AREA, handle_DP_PermArea_DRAW},
#endif
#ifdef TUYA_DP_PERM_AREA_2
        {TUYA_DP_PERM_AREA_2, handle_DP_PermArea_DRAW_2},
#endif
#ifdef TUYA_DP_SLEEP_DET_ONOFF
        {TUYA_DP_SLEEP_DET_ONOFF, handle_DP_sleepDet_ONOFF},
#endif

#ifdef TUYA_DP_NURSERY_RHYME
        {TUYA_DP_NURSERY_RHYME, handle_DP_NURSERY_RHYME},
#endif
#ifdef TUYA_DP_CUSTOMED_VOICE
        {TUYA_DP_CUSTOMED_VOICE, handle_DP_CUSTOMED_VOICE},
#endif

#ifdef TUYA_DP_ASK_CAMERA
        {TUYA_DP_ASK_CAMERA, handle_DP_ASK_CAMERA},
#endif

#ifdef TUYA_DP_PUKE_ONOFF
        {TUYA_DP_PUKE_ONOFF, handle_DP_PukeAlarm_ONOFF},
#endif

#ifdef TUYA_DP_KICKQUILT_ONOFF
        {TUYA_DP_KICKQUILT_ONOFF, handle_DP_KickQuiltAlarm_ONOFF},
#endif

#endif
};

VOID IPC_APP_handle_dp_cmd_objs(IN CONST TY_RECV_OBJ_DP_S *dp_rev)
{
    TY_OBJ_DP_S *dp_data = (TY_OBJ_DP_S *)(dp_rev->dps);
    UINT_T cnt = dp_rev->dps_cnt;
    INT_T table_idx = 0;
    INT_T table_count = ( sizeof(s_dp_table) / sizeof(s_dp_table[0]) );
    INT_T index = 0;
    for(index = 0; index < cnt; index++)
    {
        TY_OBJ_DP_S *p_dp_obj = dp_data + index;

        for(table_idx = 0; table_idx < table_count; table_idx++)
        {
            if(s_dp_table[table_idx].dp_id == p_dp_obj->dpid)
            {
                s_dp_table[table_idx].handler(p_dp_obj);
                break;
            }
        }
    }
}
VOID IPC_APP_handle_dp_query_objs(IN CONST TY_DP_QUERY_S *dp_query)
{
    INT_T table_idx = 0;
    INT_T table_count = ( sizeof(s_dp_table) / sizeof(s_dp_table[0]) );
    INT_T index = 0;
    for(index = 0; index < dp_query->cnt; index++)
    {
        for(table_idx = 0; table_idx < table_count; table_idx++)
        {
            if(s_dp_table[table_idx].dp_id == dp_query->dpid[index])
            {
                s_dp_table[table_idx].handler(NULL);
                break;
            }
        }
    }
}

/*  The following interface has been abandoned,please refer to "tuya_ipc_notify_motion_detection" and "tuya_ipc_notify_door_bell_press" in tuya_ipc_api.h

OPERATE_RET IPC_APP_Send_Motion_Alarm_From_Buffer(CHAR_T *data, UINT_T size, NOTIFICATION_CONTENT_TYPE_E type)
{
    OPERATE_RET ret = OPRT_OK;
    INT_T try = 3;
    INT_T count = 1;
    VOID *message = NULL;
    INT_T message_size = 0;
#ifdef TUYA_DP_ALARM_FUNCTION
    if(IPC_APP_get_alarm_function_onoff() != TRUE)
    {
        printf("motion alarm upload not enabled.skip \r\n");
        return OPRT_COM_ERROR;
    }
#endif

    printf("Send Motion Alarm. size:%d type:%d\r\n", size, type);
    message_size = tuya_ipc_notification_message_malloc(count, &message);
    if((message_size == 0)||(message == NULL))
    {
        printf("tuya_ipc_notification_message_malloc failed\n");
        return OPRT_COM_ERROR;
    }

    memset(message, 0, message_size);
    while (try != 0)
    {
        ret = tuya_ipc_notification_content_upload_from_buffer(type,data,size,message);
        if(ret != OPRT_OK)
        {
            try --;
            continue;
        }
        break;
    }
    if(ret == OPRT_OK)
    {
        ret = tuya_ipc_notification_message_upload(TUYA_DP_MOTION_DETECTION_ALARM, message, 5);
    }

    tuya_ipc_notification_message_free(message);

    return ret;
}

OPERATE_RET IPC_APP_Send_Motion_Alarm(CHAR_T *p_abs_file, NOTIFICATION_CONTENT_TYPE_E file_type)
{
#ifdef TUYA_DP_ALARM_FUNCTION
    if(IPC_APP_get_alarm_function_onoff() != TRUE)
    {
        printf("motion alarm upload not enabled.skip \r\n");
        return OPRT_COM_ERROR;
    }
#endif

    OPERATE_RET ret = OPRT_OK;
    INT_T try = 3;
    INT_T count = 1;
    VOID *message = NULL;
    INT_T size = 0;

    printf("Send Motion Alarm. type:%d File:%s\r\n", file_type, p_abs_file);

    size = tuya_ipc_notification_message_malloc(count, &message);
    if((size == 0)||(message == NULL))
    {
        printf("tuya_ipc_notification_message_malloc failed\n");
        return OPRT_COM_ERROR;
    }

    memset(message, 0, size);
    while (try != 0)
    {
        ret = tuya_ipc_notification_content_upload_from_file(p_abs_file, file_type, message);
        if(ret != OPRT_OK)
        {
            try --;
            continue;
        }
        break;
    }
    if(ret == OPRT_OK)
    {
        ret = tuya_ipc_notification_message_upload(TUYA_DP_MOTION_DETECTION_ALARM, message, 5);
    }

    tuya_ipc_notification_message_free(message);

    return ret;
}

OPERATE_RET IPC_APP_Send_DoorBell_Snap(CHAR_T *p_snap_file, NOTIFICATION_CONTENT_TYPE_E file_type)
{
    OPERATE_RET ret = OPRT_OK;
    INT_T try = 3;
    INT_T count = 1;
    VOID *message = NULL;
    INT_T size = 0;

    printf("Send DoorBell Snap. type:%d File:%s\r\n", file_type, p_snap_file);
    size = tuya_ipc_notification_message_malloc(count, &message);
    if((size == 0)||(message == NULL))
    {
        printf("tuya_ipc_notification_message_malloc failed\n");
        return OPRT_COM_ERROR;
    }

    memset(message, 0, size);
    while (try != 0)
    {
        ret = tuya_ipc_notification_content_upload_from_file(p_snap_file, file_type, message);
        if(ret != OPRT_OK)
        {
            try --;
            continue;
        }
        break;
    }
    if(ret == OPRT_OK)
    {
        ret = tuya_ipc_snapshot_message_upload(TUYA_DP_DOOR_BELL_SNAP, message, 5);
    }

    tuya_ipc_notification_message_free(message);

    return ret;
}
*/
