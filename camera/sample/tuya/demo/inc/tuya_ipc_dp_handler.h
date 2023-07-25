/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *www.tuya.comm
  *
  * File Description：
  * 1. DP point setting and get function definition, please refer to the code comment in the .c file for details.
**********************************************************************************/

#ifndef _TUYA_IPC_DP_HANDLER_H
#define _TUYA_IPC_DP_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_types.h"
#include "tuya_ipc_dp_utils.h"
#include "tuya_ipc_ptz.h"


#ifdef TUYA_DP_SLEEP_MODE
VOID IPC_APP_set_sleep_mode(BOOL_T sleep_mode);
BOOL_T IPC_APP_get_sleep_mode(VOID);
#endif

#ifdef TUYA_DP_LIGHT
VOID IPC_APP_set_light_onoff(BOOL_T light_on_off);
BOOL_T IPC_APP_get_light_onoff(VOID);
#endif

#ifdef TUYA_DP_SOUND_DETECT
BOOL_T IPC_APP_get_sound_onoff(VOID);
VOID IPC_APP_set_sound_onoff(BOOL_T value);
#endif

#ifdef TUYA_DP_SOUND_SENSITIVITY
BOOL_T IPC_APP_get_sound_sensi(VOID);
VOID IPC_APP_set_sound_sensi(BOOL_T value);
#endif

#ifdef TUYA_DP_CRY_DETECT
BOOL_T IPC_APP_get_cry_onoff(VOID);
VOID IPC_APP_set_cry_onoff(BOOL_T on_off);
#endif

#ifdef TUYA_DP_CRY_SENSI
BOOL_T IPC_APP_get_cry_sensi(VOID);
VOID IPC_APP_set_cry_sensi(BOOL_T value);
#endif

#ifdef TUYA_DP_REBOOT
VOID IPC_APP_set_reboot(VOID);
#endif
#ifdef TUYA_DP_FACTORY
VOID IPC_APP_set_factory(VOID);
#endif

#ifdef TUYA_DP_FLIP
VOID IPC_APP_set_flip_onoff(BOOL_T flip_on_off);
BOOL_T IPC_APP_get_flip_onoff(VOID);
#endif

#ifdef TUYA_DP_WATERMARK
VOID IPC_set_watermark_onoff(BOOL_T watermark_on_off);
VOID IPC_APP_set_watermark_onoff(BOOL_T watermark_on_off);
BOOL_T IPC_APP_get_watermark_onoff(VOID);
#endif

#ifdef TUYA_DP_WDR
VOID IPC_APP_set_wdr_onoff(BOOL_T wdr_on_off);
BOOL_T IPC_APP_get_wdr_onoff(VOID);
#endif

#ifdef TUYA_DP_NIGHT_MODE
VOID IPC_APP_set_night_mode(CHAR_T *p_night_mode);
CHAR_T *IPC_APP_get_night_mode(VOID);
#endif


#ifdef TUYA_DP_ALARM_FUNCTION
VOID IPC_APP_set_alarm_function_onoff(BOOL_T alarm_on_off);
BOOL_T IPC_APP_get_alarm_function_onoff(VOID);
#endif

#ifdef TUYA_DP_ALARM_SENSITIVITY
VOID IPC_APP_set_alarm_sensitivity(CHAR_T *p_sensitivity);
CHAR_T *IPC_APP_get_alarm_sensitivity(VOID);
#endif

#ifdef TUYA_DP_ALARM_ZONE_DRAW
VOID IPC_APP_set_alarm_zone_draw(cJSON * p_alarm_zone);
char * IPC_APP_get_alarm_zone_draw(VOID);
#endif

#ifdef TUYA_DP_ALARM_ZONE_ENABLE
VOID IPC_APP_set_alarm_zone_onoff(BOOL_T alarm_zone_on_off);
BOOL_T IPC_APP_get_alarm_zone_onoff(VOID);
#endif


#ifdef TUYA_DP_ALARM_INTERVAL
VOID IPC_APP_set_alarm_interval(CHAR_T *p_interval);
CHAR_T *IPC_APP_get_alarm_interval(VOID);
#endif

#ifdef TUYA_DP_SD_STATUS_ONLY_GET
INT_T IPC_APP_get_sd_status(VOID);
#endif

#ifdef TUYA_DP_SD_STORAGE_ONLY_GET
VOID IPC_APP_get_sd_storage(UINT_T *p_total, UINT_T *p_used, UINT_T *p_empty);
#endif

#ifdef TUYA_DP_SD_RECORD_ENABLE
VOID IPC_APP_set_sd_record_onoff(BOOL_T sd_record_on_off);
BOOL_T IPC_APP_get_sd_record_onoff(VOID);
#endif

#ifdef TUYA_DP_SD_RECORD_MODE
VOID IPC_APP_set_sd_record_mode(UINT_T sd_record_mode);
UINT_T IPC_APP_get_sd_record_mode(VOID);
#endif

#ifdef TUYA_DP_SD_UMOUNT
BOOL_T IPC_APP_unmount_sd_card(VOID);
#endif

#ifdef TUYA_DP_SD_FORMAT
VOID IPC_APP_format_sd_card(VOID);
#endif

#ifdef TUYA_DP_SD_FORMAT_STATUS_ONLY_GET
INT_T IPC_APP_get_sd_format_status(VOID);
#endif
#ifdef TUYA_DP_TEMPERATURE
INT_T IPC_APP_get_temperature_value(VOID);
#endif
#ifdef TUYA_DP_HUMIDITY
INT_T IPC_APP_get_humidity_value(VOID);
#endif

#ifdef TUYA_DP_PTZ_CONTROL
VOID IPC_APP_ptz_start_move(CHAR_T *p_direction);
#endif

#ifdef TUYA_DP_PTZ_STOP
VOID IPC_APP_ptz_stop_move(VOID);
#endif

#ifdef TUYA_DP_PTZ_CHECK
void IPC_APP_ptz_check(VOID);
#endif

#ifdef TUYA_DP_TRACK_ENABLE
void IPC_APP_track_enable(BOOL_T track_enable);

BOOL_T IPC_APP_get_track_enable(void);

#endif

#ifdef TUYA_DP_LINK_MOVE_ACTION
VOID IPC_APP_set_link_move(INT_T bind_seq);
#endif

#ifdef TUYA_DP_LINK_MOVE_SET
VOID IPC_APP_set_link_pos(INT_T bind_seq);
#endif


#ifdef TUYA_DP_HUM_FILTER
void IPC_APP_set_human_filter(BOOL_T hum_filter);
BOOL_T IPC_APP_get_human_filter(VOID);
#endif

#ifdef TUYA_DP_PERSON_TRACKER
VOID IPC_APP_set_person_tracker(BOOL_T person_tracker);
BOOL_T IPC_APP_get_person_tracker(VOID);
#endif

#ifdef TUYA_DP_PATROL_MODE
void IPC_APP_set_patrol_mode(CHAR_T *patrol_mode);
int IPC_APP_get_patrol_mode(void);

#endif

#ifdef TUYA_DP_PATROL_SWITCH
void IPC_APP_set_patrol_switch(CHAR_T *patrol_switch);

BOOL_T IPC_APP_get_patrol_switch(void);

void IPC_APP_ptz_preset_reset(S_PRESET_CFG *preset_cfg);

#endif

#ifdef TUYA_DP_PATROL_TMODE
void IPC_APP_set_patrol_tmode(CHAR_T *patrol_tmode);

int IPC_APP_get_patrol_tmode(void);
#endif

#ifdef TUYA_DP_PATROL_TIME
typedef enum
{
    TUYA_IPC_DP_HANDLER_PT_START,
    TUYA_IPC_DP_HANDLER_PT_END,
    TUYA_IPC_DP_HANDLER_PT_BOTTOM,
}TUYA_IPC_DP_HANDLER_PT_ST;
void IPC_APP_set_patrol_time(cJSON * p_patrol_time);
int IPC_APP_get_patrol_time(TUYA_IPC_DP_HANDLER_PT_ST Ttype, char *pOutPTStr);
#endif

#ifdef TUYA_DP_PRESET_SET
void IPC_APP_set_preset(cJSON * p_preset_param);

#endif

#ifdef TUYA_DP_PATROL_STATE
void IPC_APP_patrol_state(int *patrol_state);
#endif


#ifdef TUYA_DP_BLUB_SWITCH
VOID IPC_APP_set_blub_onoff(BOOL_T blub_on_off);
BOOL_T IPC_APP_get_blub_onoff(VOID);
#endif

#ifdef TUYA_DP_ELECTRICITY
INT_T IPC_APP_get_battery_percent(VOID);
#endif

#ifdef TUYA_DP_POWERMODE
CHAR_T *IPC_APP_get_power_mode(VOID);
#endif

#ifdef TUYA_DP_ALARM_CRADLESONG
VOID IPC_APP_set_alarm_cradlesong_onoff(BOOL_T alarm_cradlesong_onoff);
BOOL_T IPC_APP_get_alarm_cradlesong_onoff(VOID);
#endif

//#ifdef TUYA_DP_CRADLESONG
//VOID IPC_APP_set_cradlesong_onoff(BOOL_T cradlesong_onoff);
//#endif

#ifdef TUYA_DP_PLAYMUSIC
VOID IPC_APP_set_playmusic_onoff(INT_T playmusic_onoff);
INT_T IPC_APP_get_playmusic_onoff(VOID);
#endif

#ifdef TUYA_DP_ALARM_SOUND
VOID IPC_APP_set_alarm_sound(CHAR_T *p_sound);
CHAR_T *IPC_APP_get_alarm_sound(VOID);
#endif

//#ifdef BABY_PRO
#ifdef TUYA_DP_AO_VOLUME
VOID IPC_APP_set_AO_VOLUME(INT_T VALUE);
INT_T IPC_APP_get_AO_VOLUME(VOID);
#endif

#ifdef TUYA_DP_ALBUM_ONOFF
VOID IPC_APP_set_Album_onoff(BOOL_T album_onoff);
BOOL_T IPC_APP_get_Album_onoff(VOID);
#endif
//#endif
/*******************baby monitor***************************/
#ifdef CONFIG_BABY
#ifdef TUYA_DP_SMILE_SHOT_ONOFF
BOOL_T IPC_APP_get_smile_shot_onoff(VOID);
VOID IPC_APP_set_smile_shot_onoff(BOOL_T on_off);
#endif
#ifdef TUYA_DP_COVER_FACE_ONOFF
BOOL_T IPC_APP_get_cover_face_onoff(VOID);
VOID IPC_APP_set_cover_face_onoff(BOOL_T on_off);
#endif
#ifdef TUYA_DP_PERM_ALARM_ONOFF
BOOL_T IPC_APP_get_perm_alarm_onoff(VOID);
VOID IPC_APP_set_perm_alarm_onoff(BOOL_T on_off);
#endif
#ifdef TUYA_DP_TH_ALARM_ONOFF
BOOL_T IPC_APP_get_th_alarm_onoff(VOID);
VOID IPC_APP_set_th_alarm_onoff(BOOL_T on_off);
#endif
#ifdef TUYA_DP_PERM_AREA
VOID IPC_APP_set_perm_area_draw(cJSON *p_alarm_zone);
char *IPC_APP_get_perm_area_draw(VOID);
#endif
#ifdef TUYA_DP_PERM_AREA_2
VOID IPC_APP_set_perm_area_2_draw(cJSON *p_alarm_zone);
char *IPC_APP_get_perm_area_2_draw(VOID);
#endif

#ifdef TUYA_DP_SLEEP_DET_ONOFF
BOOL_T IPC_APP_get_sleep_det_onoff(VOID);
VOID IPC_APP_set_sleep_det_onoff(BOOL_T on_off);
#endif

#ifdef TUYA_DP_NURSERY_RHYME
INT_T IPC_APP_get_nursery_rhyme(VOID);
VOID IPC_APP_set_nursery_rhyme(INT_T nursery_rhyme);
#endif
#ifdef TUYA_DP_CUSTOMED_VOICE
VOID IPC_APP_set_customed_voice_action(CHAR_T *p_action);
CHAR_T *IPC_APP_get_customed_voice_action(VOID);
#endif

#ifdef TUYA_DP_ASK_CAMERA
VOID IPC_APP_set_cmd_ask_camera(CHAR_T *cmd);
char *IPC_APP_get_ret_ask_camera(VOID);
#endif

#ifdef TUYA_DP_PUKE_ONOFF
BOOL_T IPC_APP_get_puke_alarm_onoff(VOID);
VOID IPC_APP_set_puke_alarm_onoff(BOOL_T on_off);
#endif

#ifdef TUYA_DP_KICKQUILT_ONOFF
BOOL_T IPC_APP_get_kickquilt_alarm_onoff(VOID);
VOID IPC_APP_set_kickquilt_alarm_onoff(BOOL_T on_off);
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif  /*_TUYA_IPC_DP_HANDLER_H*/
