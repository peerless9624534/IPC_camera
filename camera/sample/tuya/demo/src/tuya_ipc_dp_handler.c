/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *www.tuya.comm

  *FileName: tuya_ipc_dp_handler.c
  *
  * File Description：
  * 1. DP Point Setting and Acquisition Function API
  *
  * Developer work：
  * 1. Local configuration acquisition and update.
  * 2. Set local IPC attributes, such as picture flip, time watermarking, etc.
  *    If the function is not supported, leave the function blank.
  *
**********************************************************************************/
#include "tuya_ipc_dp_utils.h"
#include "tuya_ipc_dp_handler.h"
#include "tuya_ipc_stream_storage.h"
#include <tuya_ipc_common_demo.h>
#include <tuya_ipc_chromecast.h>
#include "tuya_ipc_system_control_demo.h"

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include <lux_iniparse.h>
#include <lux_hal_led.h>
#include <lux_isp.h>
#include <lux_daynight.h>
#include "lux_audio.h"
#include "lux_event.h"
#include "lux_osd.h"
#include "tuya_ipc_media_demo.h"
#include "lux_ivs.h"
#include "lux_hal_motor.h"
#include "lux_pwm.h"
#include "lux_hal_th.h"
#include "lux_smileDet.h"
#include "lux_coverFace.h"
#include "lux_perm.h"
#include "lux_nv2jpeg.h"
#include "lux_base.h"
#include "lux_sleepDet.h"
#include "lux_summary.h"
#include "lux_hal_misc.h"

#define CALCULATE_STR(x) ((((x) >= '0') && ((x) <= '9')) ? ((x) - '0') : ((x) - 'a' + 10))

#define TUYA_CONFIG_RECORD_TITLE "record"
#define TUYA_CONFIG_RECORD_ONOFF "record_onoff"
#define TUYA_CONFIG_RECORD_MODE  "record_mode"
#define TUYA_CONFIG_FUNCTION "function"
#define TUYA_CONFIG_SOUND_ONOFF "sound_det_onoff"
#define TUYA_CONFIG_SOUND_SENSI "sound_det_sensi"


#define TUYA_SOUND_DET_SENSI_LOW  (0.90f)
#define TUYA_SOUND_DET_SENSI_HIGH (0.80f)

FLOAT_T g_cryDetectSensitive = TUYA_SOUND_DET_SENSI_LOW;
BOOL_T  g_bcryDetctAlarmSong = FALSE;

extern BOOL_X g_halTempHumiEnable;

extern FILE *cradlesongFp;
//BOOL_X b_newselectsong = FALSE;
//DWORD_X songplay_pieces = 0;
//DWORD_X handletype_of_playmusic = 0;
INT_T gcustomed_action_get = 0;
DWORD_T gstart_record_time = 0;
DWORD_X bPlayingCryMusic = 0;;
unsigned int start_nursery_record_time = 0;

#define PRESET_PIC_MAX_SIZE 6
volatile int patrol_index = 0;

void __tuya_app_int2char(int value, char cBuf[8])
{
    char table[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
    int i = 0, cnt = 0, count1 = 0;
    char str[10] = {0};

    for (i = 0; i < 8; i++)
    {
        str[i] = table[(((value >> (32 - 4*(i+1)))) & 0xf)];
        if (('0' != str[i]) || count1)
            count1++;
    }

    if (0 == count1)
    {
        strcpy(cBuf, "0");
        return;
    }

    for (i = (8-count1); i < 8; i++)
    {
        sprintf(&cBuf[cnt], "%c", str[i]);
        cnt++;
        if (cnt == count1)
            break;
    }
    return;
}
static int __tuya_app_char2int(char cBuf[8])
{
    int i = 0, value = 0;
    int len = strlen(cBuf);

    for (i = 0; i < 8; i++)
    {
        if (cBuf[i])
            value += (CALCULATE_STR(cBuf[i])) << ((len-(i+1)) * 4);
    }

    return value;
}

/* Setting and Getting Integer Local Configuration.
The reference code here is written directly to tmp, requiring developers to replace the path.*/
STATIC VOID __tuya_app_write_INT(CHAR_T * key, INT_T value)
{
    INT_T ret = 0;
    char tmp[8] = {0};

    __tuya_app_int2char(value, tmp);
    ret = LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_FUNCTION, key, tmp);
    if (0 != ret)
    {
        PR_ERR("LUX_INIParse_SetKey failed!, error num [0x%x] ", ret);
    }

    return;
}

STATIC INT_T __tuya_app_read_INT(CHAR_T *key)
{
    INT_T ret = 0;
    char tmp[8] = {0};

    ret = LUX_INIParse_GetString(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_FUNCTION, key, tmp);
    if (0 != ret)
    {
        PR_ERR("__tuya_app_read_INT failed!, error num [0x%x] ", ret);
    }

    return __tuya_app_char2int(tmp);
}

/* Setting and Getting String Local Configuration.
The reference code here is written directly to tmp, requiring developers to replace the path.*/
STATIC VOID __tuya_app_write_STR(CHAR_T *key, CHAR_T *value)
{
    INT_T ret = 0;

    ret = LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_FUNCTION, key, value);
    if (0 != ret)
    {
        PR_ERR("LUX_INIParse_SetKey failed, key[%s]!, error num [0x%x] ",key, ret);
    }

    return;
}

STATIC INT_T __tuya_app_read_STR(CHAR_T *key, CHAR_T *value, INT_T value_size)
{
    CHAR_T tmp[248] = {0};
    INT_T ret = 0;

    ret = LUX_INIParse_GetString(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_FUNCTION, key, tmp);
    if (0 != ret)
    {
        PR_ERR("LUX_INIParse_GetInt failed,  key[%s]!, error num [0x%x] ", key, ret);
    }

    memset(value, 0, value_size);
    if (strlen(tmp) >= value_size)
    {
        strncpy(value, tmp, value_size - 1);
    }
    else
    {
        strcpy(value, tmp);
    }

    return 0;
}

//------------------------------------------

#ifdef TUYA_DP_LIGHT
VOID IPC_APP_set_light_onoff(BOOL_T light_on_off)
{
    INT_T ret = -1;
    PR_DEBUG("set light_on_off:%d \r\n", light_on_off);
    LUX_HAL_LedEnable(light_on_off);
    ret = LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, "hal", "led", light_on_off? "true": "false");
    if(0 != ret)
    {
        PR_DEBUG("LUX_INIParse_SetKey failed!, error num [0x%x] ", ret);
    }
}


BOOL_T IPC_APP_get_light_onoff(VOID)
{
    BOOL_X luxLightStat = LUX_FALSE;
    INT_T ret;
    
    ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "hal", "led" , &luxLightStat);
    if(0 != ret)
    {
        PR_DEBUG("LUX_INIParse_GetBool failed!, error num [0x%x] ", ret);
    }

    PR_DEBUG("curr light_on_off:%d \r\n", luxLightStat);

    return (luxLightStat ? TRUE : FALSE);
}
#endif

#ifdef TUYA_DP_SOUND_DETECT
BOOL_T IPC_APP_get_sound_onoff(VOID)
{
    BOOL_X onoff = LUX_FALSE;
    INT_T ret;

    PR_DEBUG("IPC_APP_get_sound_onoff \r\n");

    ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_FUNCTION, TUYA_CONFIG_SOUND_ONOFF, &onoff);
    if (0 != ret)
    {
        PR_DEBUG("LUX_INIParse_GetBool failed!, error num [0x%x] ", ret);
    }

    return onoff;
}

VOID IPC_APP_set_sound_onoff(BOOL_T value)
{
    INT_T ret;

    PR_DEBUG("set IPC_APP_set_sound_sensi:%d \r\n", value);

    ret = LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_FUNCTION, TUYA_CONFIG_SOUND_ONOFF, (value ? "1" : "0"));
    if (0 != ret)
    {
        PR_ERR("LUX_INIParse_SetKey failed!, error num [0x%x] ", ret);
    }

    return;
}
#endif
#ifdef TUYA_DP_SOUND_SENSITIVITY
BOOL_T IPC_APP_get_sound_sensi(VOID)
{
    BOOL_X sensi = LUX_FALSE;

    PR_DEBUG("curr IPC_APP_get_sound_sensi\r\n");

    return sensi;
}

VOID IPC_APP_set_sound_sensi(BOOL_T value)
{
    PR_DEBUG("set IPC_APP_set_sound_sensi:%d \r\n", value);

    return;
}
#endif
#ifdef TUYA_DP_CRY_DETECT
BOOL_T IPC_APP_get_cry_onoff(VOID)
{
    printf("curr cry_on_off:%d \r\n", LUX_AI_GetCryDet());

    return LUX_AI_GetCryDet();
}

VOID IPC_APP_set_cry_onoff(BOOL_T on_off)
{
    INT_T ret = -1;

    PR_DEBUG("set cry_on_off:%d \r\n", on_off);

    /*在隐私模式下不执行相关动作*/
    if(TRUE == IPC_APP_get_sleep_mode())
    {
        PR_DEBUG("WARNING: set sd_cry_onoff:%d invalied in the sleep mode!\r\n", on_off);

        ret = LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_FUNCTION, "cry_onoff", on_off ? "1" : "0");
        if (0 != ret)
        {
            PR_ERR("ERROR:LUX_INIParse_SetKey ret:%#x", ret);
        }

        return;
    }

    /* Status indicator,BOOL type,true means on,false means off */
    LUX_AI_SetCryDet(on_off);

    ret = LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_FUNCTION, "cry_onoff", on_off ? "1" : "0");
    if (LUX_OK != ret)
    {
        PR_DEBUG("cry_onoff failed,error [0x%x]\n", ret);
    }

    return;
}
#endif

#ifdef TUYA_DP_CRY_SENSI
BOOL_T IPC_APP_get_cry_sensi(VOID)
{
    BOOL_X sensi = LUX_FALSE;
    INT_T ret;

    PR_DEBUG("curr IPC_APP_get_cry_sensi\r\n");

    ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_FUNCTION, TUYA_CONFIG_SOUND_SENSI, &sensi);
    if (0 != ret)
    {
        PR_DEBUG("LUX_INIParse_GetBool failed!, error num [0x%x] ", ret);
    }
    g_cryDetectSensitive = sensi ? TUYA_SOUND_DET_SENSI_HIGH : TUYA_SOUND_DET_SENSI_LOW;

    return sensi;
}

VOID IPC_APP_set_cry_sensi(BOOL_T value)
{
    INT_T ret;

    ret = LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_FUNCTION, TUYA_CONFIG_SOUND_SENSI, (value ? "1" : "0"));
    if (0 != ret)
    {
        PR_ERR("LUX_INIParse_SetKey failed!, error num [0x%x] ", ret);
    }
    g_cryDetectSensitive = value ? TUYA_SOUND_DET_SENSI_HIGH : TUYA_SOUND_DET_SENSI_LOW;

    PR_DEBUG("set IPC_APP_set_cry_sensi:%d g_cryDetectSensitive:%f\r\n", value, g_cryDetectSensitive);

    return;
}
#endif

//#ifdef TUYA_DP_CRADLESONG
//VOID IPC_APP_set_cradlesong_onoff(BOOL_T cradlesong_onoff)
//{
//    printf("set cradlesong_onoff:%d \r\n", cradlesong_onoff);

//    return;
//}
//#endif
#ifdef TUYA_DP_PLAYMUSIC
VOID IPC_APP_set_playmusic_onoff(INT_T playmusic_onoff)
{
    printf("set playmusic_onoff:%d \r\n", playmusic_onoff);

    __tuya_app_write_INT("playmusic_onoff", playmusic_onoff);
    if(playmusic_onoff == LUX_SONG_PLAY)
    {
        IPC_NurseryRhymeStart();
    }
    else if(playmusic_onoff == LUX_SONG_PAUSE)
    {
        IPC_NurseryRhymePause();
    }
//    return;
}
INT_T IPC_APP_get_playmusic_onoff(VOID)
{
    //INT_T playmusic_onoff = __tuya_app_read_INT("playmusic_onoff");
    INT_T playmusic_onoff = IPC_NurseryRhymeStatus();
    printf("curr playmusic_onoff:%d \r\n", playmusic_onoff);

    return playmusic_onoff;
}
#endif

#ifdef TUYA_DP_ALARM_CRADLESONG
VOID IPC_APP_set_alarm_cradlesong_onoff(BOOL_T alarm_cradlesong_onoff)
{
    printf("set alarm_cradlesong_onoff:%d \r\n", alarm_cradlesong_onoff);
    
    g_bcryDetctAlarmSong = alarm_cradlesong_onoff;
    
    __tuya_app_write_INT("alarm_cradlesong_onoff", alarm_cradlesong_onoff);
}

BOOL_T IPC_APP_get_alarm_cradlesong_onoff(VOID)
{
    BOOL_T alarm_cradlesong_onoff = __tuya_app_read_INT("alarm_cradlesong_onoff");
    printf("curr alarm_cradlesong_onoff:%d \r\n", alarm_cradlesong_onoff);

    /* 解决重启后哭声报警不触发安眠曲问题 */
    if (g_bcryDetctAlarmSong != alarm_cradlesong_onoff)
    {
        g_bcryDetctAlarmSong = alarm_cradlesong_onoff;
    }

    return alarm_cradlesong_onoff;
}
#endif

/*报警提示音选择*/
#ifdef TUYA_DP_ALARM_SOUND
CHAR_T *IPC_APP_get_alarm_sound(VOID)
{
    STATIC CHAR_T s_alarm_sound[4] = {0};
    
    __tuya_app_read_STR("choose_alarm_sound", s_alarm_sound, 4);
    printf("get choose_alarm_sound:%s \r\n", s_alarm_sound);
    return s_alarm_sound;
}
VOID IPC_APP_set_alarm_sound(CHAR_T *p_sound)
{
    printf("set choose_alarm_sound:%s \r\n", p_sound);
    __tuya_app_write_STR("choose_alarm_sound", p_sound);
}
#endif

#ifdef TUYA_DP_SLEEP_MODE
BOOL_T IPC_APP_get_sleep_mode(VOID)
{
    BOOL_T sleep_mode = __tuya_app_read_INT("sleep_mode");
    
    // printf("curr sleep_mode:%#x \r\n", sleep_mode);

    return sleep_mode;
}

VOID IPC_APP_set_sleep_mode(BOOL_T sleep_mode)
{
    INT_T ret = -1;

    printf("set sleep_mode:%d \r\n", sleep_mode);

    /* 重复配置检查 */
    if (sleep_mode == IPC_APP_get_sleep_mode())
    {
        printf("warning:reconfig sleep_mode:%d \r\n", sleep_mode);
        return;
    }

    /* 隐私功能开启，停掉音视频流、算法相关业务 */
    if (sleep_mode)
    {
        __tuya_app_write_INT("sleep_mode", sleep_mode);

        /* 关闭录像功能 */
        if(TRUE == IPC_APP_get_sd_record_onoff())
        {
            tuya_ipc_ss_set_write_mode(SS_WRITE_MODE_NONE);
		
        }

        /* 停止哭声检测 */
        if(TRUE == IPC_APP_get_cry_onoff())
        {
            LUX_AI_SetCryDet(FALSE);
        }

/* 解决隐私模式由开到关，人形侦测失效问题：babymonitor项目该部分不起作用 */
#if 0
        /* 停止侦测功能(移动人形) */
        if(TRUE == IPC_APP_get_alarm_function_onoff())
        {
            ret = LUX_IVS_StopAlgo(LUX_IVS_MOVE_DET_EN);
            if(0 != ret)
            {
                PR_DEBUG("LUX_IVS_StopAlgo failed!, error num [0x%x] ", ret);
            }
        }
#endif

        /*关停睡眠侦测 cxj test*/
        if (TRUE == IPC_APP_get_sleep_det_onoff())
        {
            LUX_IVS_SleepDet_Stop();
        }
        /* 停止人形侦测 */
        if (TRUE == IPC_APP_get_human_filter())
        {
            ret = LUX_IVS_StopAlgo(LUX_IVS_PERSON_DET_EN);
            if (0 != ret)
            {
                PR_DEBUG("LUX_IVS_StopAlgo failed!, error num [0x%x] ", ret);
            }
        }

        /* 停止人脸功能（遮脸检测、微笑抓拍） */
        if (TRUE == IPC_APP_get_cover_face_onoff())
        {
            LUX_IVS_CoverFace_Stop();
        }
        if (TRUE == IPC_APP_get_smile_shot_onoff())
        {
            LUX_IVS_SmileDet_Stop();
        }
        /* 停止电子围栏 */
        if (TRUE == IPC_APP_get_perm_alarm_onoff())
        {
            LUX_IVS_Perm_Stop();
        }
        /* 停止温湿度异常报警 */
        if (TRUE == IPC_APP_get_th_alarm_onoff())
        {
            g_halTempHumiEnable = FALSE;
        }
        #ifdef TUYA_DP_ALBUM_ONOFF
        /* 关停相册抓拍 */
        if (TRUE == IPC_APP_get_Album_onoff())
        {
            LUX_Jpeg_Album_Stop();
            IPC_APP_ALBUM_Stat(LUX_FALSE);
        }
        #endif

/* 解决隐私模式由开到关，人形侦测失效问题：babymonitor项目以下部分不生效 */
#if 0
#ifdef CONFIG_PTZ_IPC

        /* 停止人形追踪 */
        if(TRUE == IPC_APP_get_person_tracker())
        {
            ret = LUX_IVS_StopAlgo(LUX_IVS_PERSON_TRACK_EN);
            if(0 != ret)
            {
                PR_DEBUG("LUX_IVS_StartAlgo failed.\r\n");
            }
        }

        /* PTZ停止巡航 */
        if(TRUE == IPC_APP_get_patrol_switch())
        {
            ret = LUX_HAL_MOTOR_SetCruiseMode(LUX_HAL_MOTOR_CRUISE_STOP, 0);
            if(0 != ret)
            {
                PR_DEBUG("LUX_HAL_MOTOR_SetCruiseMode failed!, error num [0x%x] \n", ret);
            }
        }

#endif
#endif
        IPC_set_watermark_onoff(FALSE);
        /* 停掉音频 */
        LUX_Hal_Misc_SpeakerOnOff(LUX_FALSE);
    }
    else
    {
        __tuya_app_write_INT("sleep_mode", sleep_mode);
        LUX_Hal_Misc_SpeakerOnOff(LUX_TRUE);
        IPC_set_watermark_onoff(IPC_APP_get_watermark_onoff());       

        /* 还原人脸功能 */
        IPC_APP_set_cover_face_onoff(IPC_APP_get_cover_face_onoff());
        IPC_APP_set_smile_shot_onoff(IPC_APP_get_smile_shot_onoff());
        /* 还原电子围栏 */
        IPC_APP_set_perm_alarm_onoff(IPC_APP_get_perm_alarm_onoff());
        /* 还原温湿度异常报警 */
        IPC_APP_set_th_alarm_onoff(IPC_APP_get_th_alarm_onoff());
        /* 还原人形侦测 */
        IPC_APP_set_human_filter(IPC_APP_get_human_filter());

        /* 还原哭声检测配置 */
        IPC_APP_set_cry_onoff(IPC_APP_get_cry_onoff());

        /* 还原录像功能配置 */
        IPC_APP_set_sd_record_onoff(IPC_APP_get_sd_record_onoff());
        /*还原睡眠侦测配置 cxj test*/
        IPC_APP_set_sleep_det_onoff(IPC_APP_get_sleep_det_onoff());

/* 解决隐私模式由开到关，人形侦测失效问题：babymonitor项目该模块不起作用 */
#if 0
        /* 还原侦测功能 */
        IPC_APP_set_alarm_function_onoff(IPC_APP_get_alarm_function_onoff());
#endif

/* 解决隐私模式由开到关，人形侦测失效问题：babymonitor项目以下部分不生效 */
#if 0
#ifdef CONFIG_PTZ_IPC

        /* 还原人形追踪配置 */
        IPC_APP_set_person_tracker(IPC_APP_get_person_tracker());

        /* 还原巡航设置 */
        IPC_APP_set_patrol_switch(((FALSE == IPC_APP_get_patrol_switch()) ? "0" : "1"));
#endif
#endif
#ifdef CONFIG_BABY

#endif
    }

    return;
}
#endif

#ifdef TUYA_DP_REBOOT
VOID IPC_APP_set_reboot()
{
    PR_DEBUG("curr IPC_APP_set_reboot\r\n");
    sync();
    LUX_Event_Reboot();
    return;
}
#endif
#ifdef TUYA_DP_FACTORY
VOID IPC_APP_set_factory()
{
    PR_DEBUG("curr IPC_APP_set_factory\r\n");

    //system("/system/init/backup/factory_reset.sh");
    LUX_BASE_System_CMD("/system/init/backup/factory_reset.sh");
    sleep(1);

    LUX_Event_Reboot();

    return;
}
#endif


#ifdef TUYA_DP_FLIP


VOID IPC_APP_set_flip_onoff(BOOL_T flip_on_off)
{
    INT_T ret = -1;
    CHAR_T tmpStr[8] = {0};

    PR_DEBUG("set flip_on_off:%d \r\n", flip_on_off);
    /* flip state,BOOL type,true means inverse,false means normal */

    if(flip_on_off)
    {

#ifdef CONFIG_PTZ_IPC
        ret = LUX_ISP_SetSenceFlip(LUX_ISP_SNS_MIRROR_FLIP);
#else/*ac*/
        ret = LUX_ISP_SetSenceFlip(LUX_ISP_SNS_NORMAL);
#endif  /* endif CONFIG_PTZ_IPC */

        if(0 != ret)
        {
            PR_DEBUG("LUX_ISP_SetSenceFlip failed!, error num [0x%x] ", ret);
        }
        
        /*区域报警坐标翻转*/
        ret = LUX_IVS_SetAlarmRegion_Flip();
        if(0 != ret)
        {
            PR_DEBUG("LUX_IVS_SetAlarmRegion_Flip failed!, error num [0x%x] ", ret);
        }
        /*区域多边形报警坐标翻转*/
        ret = LUX_IVS_SetAlarmRegion_2_Flip();
        if(0 != ret)
        {
            PR_DEBUG("LUX_IVS_SetAlarmRegion_2_Flip failed!, error num [0x%x] ", ret);
        }
        memcpy(tmpStr, "true", 5);
    }
    else
    {
#ifdef CONFIG_PTZ_IPC
        ret = LUX_ISP_SetSenceFlip(LUX_ISP_SNS_NORMAL);
#else/*ac*/
        ret = LUX_ISP_SetSenceFlip(LUX_ISP_SNS_MIRROR_FLIP);
#endif  /* endif CONFIG_PTZ_IPC */
        if(0 != ret)
        {
            PR_DEBUG("LUX_ISP_SetSenceFlip failed!, error num [0x%x] ", ret);
        }

        /*区域报警坐标翻转*/
        ret = LUX_IVS_SetAlarmRegion_Flip();
        if(0 != ret)
        {
            PR_DEBUG("LUX_IVS_SetAlarmRegion_Flip failed!, error num [0x%x] ", ret);
        }
        /*区域多边形报警坐标翻转*/
        ret = LUX_IVS_SetAlarmRegion_2_Flip();
        if(0 != ret)
        {
            PR_DEBUG("LUX_IVS_SetAlarmRegion_2_Flip failed!, error num [0x%x] ", ret);
        }
        memcpy(tmpStr, "false", 5);
    }

    ret = LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, "video","flip", tmpStr);
    if(0 != ret)
    {
        PR_DEBUG("LUX_INIParse_SetKey failed!, error num [0x%x] ", ret);
    }
}

BOOL_T IPC_APP_get_flip_onoff(VOID)
{
    BOOL_X luxFlipStat = LUX_FALSE;
    INT_T ret;
    
    ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "video", "flip" , &luxFlipStat);
    if(0 != ret)
    {
        PR_DEBUG("LUX_INIParse_GetBool failed!, error num [0x%x] ", ret);
    }

    PR_DEBUG("curr flip_onoff:%d \r\n", luxFlipStat);

    return (luxFlipStat ? TRUE : FALSE);
}
#endif

//------------------------------------------

#ifdef TUYA_DP_WATERMARK
//cxj test
VOID IPC_set_watermark_onoff(BOOL_T watermark_on_off)
{
    printf("set watermark_on_off:%d \r\n", watermark_on_off);
    
    /* Video watermarking (OSD),BOOL type,true means adding watermark on,false means not */
    LUX_OSD_Show(watermark_on_off);
}

VOID IPC_APP_set_watermark_onoff(BOOL_T watermark_on_off)
{
    printf("set watermark_on_off:%d \r\n", watermark_on_off);
    
    /* Video watermarking (OSD),BOOL type,true means adding watermark on,false means not */
    LUX_OSD_Show(watermark_on_off);
    
    __tuya_app_write_INT("watermark_onoff", watermark_on_off);
}

BOOL_T IPC_APP_get_watermark_onoff(VOID)
{
    BOOL_T watermark_on_off = __tuya_app_read_INT("watermark_onoff");
    printf("curr watermark_on_off:%d \r\n", watermark_on_off);
    return watermark_on_off;
}
#endif

//------------------------------------------

#ifdef TUYA_DP_WDR
VOID IPC_APP_set_wdr_onoff(BOOL_T wdr_on_off)
{
    printf("set wdr_on_off:%d \r\n", wdr_on_off);
    //TODO
    /* Wide Dynamic Range Model,BOOL type,true means on,false means off */

    __tuya_app_write_INT("wdr_onoff", wdr_on_off);

}

BOOL_T IPC_APP_get_wdr_onoff(VOID)
{
    BOOL_T wdr_on_off = __tuya_app_read_INT("wdr_onoff");
    printf("curr watermark_on_off:%d \r\n", wdr_on_off);
    return wdr_on_off;
}
#endif

//------------------------------------------

#ifdef TUYA_DP_NIGHT_MODE
VOID IPC_APP_set_night_mode(CHAR_T *p_night_mode)
{
    INT_T ret  = -1;

    /*0-automatic 1-off 2-on*/
    PR_DEBUG("set night_mode:%s \r\n", p_night_mode); 

    /*自动模式*/
    if('0' == *p_night_mode)
    {
        ret = LUX_DayNight_SetAutoMode(LUX_DAYNIGHT_AUTO_MODE);
        if(0 != ret)
        {
            PR_DEBUG("LUX_DayNight_SetAutoMode failed!, error num [0x%x] ", ret);
        }
    }
    /*手动白天*/
    else if('1' == *p_night_mode)
    {
        ret = LUX_DayNight_SetAutoMode(LUX_DAYNIGHT_MANUAL_MODE);
        if(0 != ret)
        {
            PR_DEBUG("LUX_DayNight_SetAutoMode failed!, error num [0x%x] ", ret);
        }

        ret = LUX_DayNight_SetMode(LUX_DAYNIGHT_DAY_MODE);
        if(0 != ret)
        {
            PR_DEBUG("LUX_DayNight_SetMode failed!, error num [0x%x] ", ret);
        }
    }
    /*手动夜晚*/
    else
    {
        ret = LUX_DayNight_SetAutoMode(LUX_DAYNIGHT_MANUAL_MODE);
        if(0 != ret)
        {
            PR_DEBUG("LUX_DayNight_SetAutoMode failed!, error num [0x%x] ", ret);
        }

        ret = LUX_DayNight_SetMode(LUX_DAYNIGHT_NIGHT_MODE);
        if(0 != ret)
        {
            PR_DEBUG("LUX_DayNight_SetMode failed!, error num [0x%x] ", ret);
        }

    }

    ret = LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, "video","daynight", p_night_mode);
    if(0 != ret)
    {
        PR_DEBUG("LUX_INIParse_SetKey failed!, error num [0x%x] ", ret);
    }

}

CHAR_T *IPC_APP_get_night_mode(VOID)
{   
    INT_T ret = -1;
    static CHAR_T tmpStr[8];

    memset(tmpStr, 0, sizeof(tmpStr));

    ret = LUX_INIParse_GetString(TUYA_INI_CONFIG_FILE_NAME, "video", "daynight" , tmpStr);
    if(0 != ret)
    {
        PR_DEBUG("LUX_INIParse_GetBool failed!, error num [0x%x] ", ret);
    }

    return tmpStr;

}
#endif

//------------------------------------------

#ifdef TUYA_DP_ALARM_FUNCTION
VOID IPC_APP_set_alarm_function_onoff(BOOL_T alarm_on_off)
{
    INT_X ret = LUX_ERR;
    BOOL_T alarm_human_filter = __tuya_app_read_INT("alarm_human_filter");

    PR_DEBUG("set alarm_on_off:%d \r\n", alarm_on_off);
    /* motion detection alarm switch,BOOL type,true means on,false means off.*/

    /*在隐私模式下不执行相关动作*/
    if(TRUE == IPC_APP_get_sleep_mode())
    {
        PR_DEBUG("WARNING: set sd_alarm_onoff:%d invalied in the sleep mode!\r\n", alarm_on_off);

        ret = LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_RECORD_TITLE, "alarm_onoff", alarm_on_off ? "1" : "0");
        if (0 != ret)
        {
            PR_ERR("ERROR:LUX_INIParse_SetKey ret:%#x", ret);
        }

        return;
    }

    if(TRUE == alarm_on_off)
    {
        /*移动侦测开启, 如果检查到人形侦测开启,那么移动侦测不起作用,人形侦测起作用*/
        if(TRUE == alarm_human_filter)
        {
            ret = LUX_IVS_StartAlgo(LUX_IVS_PERSON_DET_EN);
            if(0 != ret)
            {
                PR_DEBUG("LUX_IVS_StartAlgo failed!, error num [0x%x] ", ret);
            }
        }
        else
        {
            ret = LUX_IVS_StartAlgo(LUX_IVS_MOVE_DET_EN);
            if(0 != ret)
            {
                PR_DEBUG("LUX_IVS_StartAlgo failed!, error num [0x%x] ", ret);
            }
        }
    }
    else
    {
        /*移动侦测报警停止, 人形侦测也需要停止*/
        ret = LUX_IVS_StopAlgo(LUX_IVS_MOVE_DET_EN);
        if(0 != ret)
        {
            PR_DEBUG("LUX_IVS_StopAlgo failed!, error num [0x%x] ", ret);
        }

        ret = LUX_IVS_StopAlgo(LUX_IVS_PERSON_DET_EN);
        if(0 != ret)
        {
            PR_DEBUG("LUX_IVS_StopAlgo failed!, error num [0x%x] ", ret);
        }
    }

    __tuya_app_write_INT("alarm_onoff", alarm_on_off);
}

BOOL_T IPC_APP_get_alarm_function_onoff(VOID)
{
    BOOL_T alarm_on_off = __tuya_app_read_INT("alarm_onoff");
    printf("curr alarm_on_off:%d \r\n", alarm_on_off);
    return alarm_on_off;
}
#endif

//------------------------------------------

#ifdef TUYA_DP_ALARM_SENSITIVITY
STATIC CHAR_T s_alarm_sensitivity[4] = {0};//for demo
VOID IPC_APP_set_alarm_sensitivity(CHAR_T *p_sensitivity)
{
    PR_DEBUG("set alarm_sensitivity:%s \r\n", p_sensitivity);
    
    INT_T ret = -1;
    LUX_IVS_SENSITIVE_EN sns = 0;

    /* Motion detection alarm sensitivity,ENUM type,0 means low sensitivity,1 means medium sensitivity,2 means high sensitivity*/
    switch(*p_sensitivity)
    {
        case '0':
            sns = LUX_IVS_SENSITIVE_LOW_EN;
            break;
        case '1':
            sns = LUX_IVS_SENSITIVE_MID_EN;
            break;
        case '2':
            sns = LUX_IVS_SENSITIVE_HIG_EN;
            break;
        default:
            PR_DEBUG("IPC_APP_set_alarm_sensitivity failed!, not supprot param [0x%d] ,set default param.", *p_sensitivity);
            sns = LUX_IVS_SENSITIVE_LOW_EN;
    }

    /*移动侦测灵敏度*/
    ret = LUX_IVS_SetSensitive(LUX_IVS_MOVE_DET_EN, sns);
    if(0 != ret)
    {
        PR_DEBUG("IPC_APP_set_alarm_sensitivity failed!, error num [0x%x] ", ret);
    }

    /*人形侦测灵敏度*/
    ret = LUX_IVS_SetSensitive(LUX_IVS_PERSON_DET_EN, sns);
    if(0 != ret)
    {
        PR_DEBUG("IPC_APP_set_alarm_sensitivity failed!, error num [0x%x] ", ret);
    }

    __tuya_app_write_STR("alarm_sen", p_sensitivity);
}

CHAR_T *IPC_APP_get_alarm_sensitivity(VOID)
{
    __tuya_app_read_STR("alarm_sen", s_alarm_sensitivity, 4);
    PR_DEBUG("curr alarm_sensitivity:%s \r\n", s_alarm_sensitivity);
    return s_alarm_sensitivity;
}
#endif



#ifdef TUYA_DP_ALARM_ZONE_DRAW

#define MAX_ALARM_ZONE_NUM      (6)     //Supports the maximum number of detection areas
//Detection area structure
typedef struct{
    int pointX;    //Starting point x  [0-100]
    int pointY;    //Starting point Y  [0-100]
    int width;     //width    [0-100]
    int height;    //height    [0-100]
}ALARM_ZONE_T;

typedef struct{
    int iZoneNum;   //Number of detection areas
    ALARM_ZONE_T alarmZone[MAX_ALARM_ZONE_NUM];
}ALARM_ZONE_INFO_T;

VOID tuya_transJs2St(CHAR_T * p_alarm_zone, ALARM_ZONE_INFO_T* strAlarmZoneInfo)
{
    INT_T i = 0;
    cJSON * pJson  = NULL;
    cJSON * tmp = NULL;
    CHAR_T region[12] = {0};
    cJSON * cJSONRegion = NULL;

    pJson = cJSON_Parse((CHAR_T *)p_alarm_zone);
    if (NULL == pJson)
    {
        printf("%s %d step error\n",__FUNCTION__,__LINE__);
        //free(pResult);
        return;
    }

    tmp = cJSON_GetObjectItem(pJson, "num");
    if (NULL == tmp)
    {
        printf("%s %d step error\n",__FUNCTION__,__LINE__);
        cJSON_Delete(pJson);
        //free(pResult);
        return ;
    }

    memset(strAlarmZoneInfo, 0x00, sizeof(ALARM_ZONE_INFO_T));

    strAlarmZoneInfo->iZoneNum = tmp->valueint;
    PR_DEBUG("%s %d step num[%d]\n",__FUNCTION__,__LINE__,strAlarmZoneInfo->iZoneNum);
    if (strAlarmZoneInfo->iZoneNum > MAX_ALARM_ZONE_NUM)
    {
        printf("#####error zone num too big[%d]\n",strAlarmZoneInfo->iZoneNum);
        cJSON_Delete(pJson);
        //free(pResult);
        return ;
    }

    for (i = 0; i < strAlarmZoneInfo->iZoneNum; i++)
    {
        snprintf(region, 12, "region%d",i);

        cJSONRegion = cJSON_GetObjectItem(pJson, region);
        if (NULL == cJSONRegion)
        {
            printf("#####[%s][%d]error\n",__FUNCTION__,__LINE__);
            cJSON_Delete(pJson);
            //free(pResult);
            return;
        }

        strAlarmZoneInfo->alarmZone[i].pointX = cJSON_GetObjectItem(cJSONRegion, "x")->valueint;
        strAlarmZoneInfo->alarmZone[i].pointY = cJSON_GetObjectItem(cJSONRegion, "y")->valueint;
        strAlarmZoneInfo->alarmZone[i].width = cJSON_GetObjectItem(cJSONRegion,  "xlen")->valueint;
        strAlarmZoneInfo->alarmZone[i].height = cJSON_GetObjectItem(cJSONRegion, "ylen")->valueint;

        PR_DEBUG("#####[%s][%d][%d,%d,%d,%d]\n",__FUNCTION__,__LINE__,strAlarmZoneInfo->alarmZone[i].pointX,\
            strAlarmZoneInfo->alarmZone[i].pointY,strAlarmZoneInfo->alarmZone[i].width,strAlarmZoneInfo->alarmZone[i].height);
    }

    cJSON_Delete(pJson);
    //free(pResult);
}

VOID IPC_APP_set_alarm_zone_draw(cJSON * p_alarm_zone)
{
    ALARM_ZONE_INFO_T strAlarmZoneInfo;
    INT_T ret = 0;

    if (NULL == p_alarm_zone){
        return ;
    }

    /*Motion detection area setting switch*/
    printf("%s %d set alarm_zone_set:%s \r\n",__FUNCTION__,__LINE__, (char *)p_alarm_zone);

    /*解析JSON后赋值给strAlarmZoneInfo结构体*/
    tuya_transJs2St((char *)p_alarm_zone, &strAlarmZoneInfo);

    ret = LUX_IVS_SetAlarmRegion((LUX_IVS_ZONE_BASE_ST *) &strAlarmZoneInfo.alarmZone[0]);
    if (ret)
    {
        PR_DEBUG(" LUX_IVS_SetAlarmRegion failed, return error[0x%x]\n", ret);
        return;
    }
 
    __tuya_app_write_STR("alarm_zone_size", (char *)p_alarm_zone);

    return ;
}

static CHAR_T s_alarm_zone[64] = {0};
char * IPC_APP_get_alarm_zone_draw(VOID)
{
    
    INT_X i;
    CHAR_T alarm_zone[64] = {0};
    ALARM_ZONE_INFO_T strAlarmZoneInfo;
    CHAR_T region[64] = {0};

    memset(&strAlarmZoneInfo, 0x00, sizeof(ALARM_ZONE_INFO_T));

    __tuya_app_read_STR("alarm_zone_size", alarm_zone, 64);
    tuya_transJs2St((char *)alarm_zone, &strAlarmZoneInfo);

    if (strAlarmZoneInfo.iZoneNum > MAX_ALARM_ZONE_NUM)
    {
        printf("[%s] [%d ]get iZoneNum[%d] error",__FUNCTION__,__LINE__,strAlarmZoneInfo.iZoneNum);
        return s_alarm_zone;
    }

    for (i = 0; i < strAlarmZoneInfo.iZoneNum; i++)
    {
        //{"169":"{\"num\":1,\"region0\":{\"x\":0,\"y\":0,\"xlen\":50,\"ylen\":50}}"}
        if (0 == i)
        {
            snprintf(s_alarm_zone, 64, "{\\\"num\\\":%d",strAlarmZoneInfo.iZoneNum);
        }

        snprintf(region, 64, ",\\\"region%d\\\":{\\\"x\\\":%d,\\\"y\\\":%d,\\\"xlen\\\":%d,\\\"ylen\\\":%d}",i,strAlarmZoneInfo.alarmZone[i].pointX,\
                 strAlarmZoneInfo.alarmZone[i].pointY,strAlarmZoneInfo.alarmZone[i].width,strAlarmZoneInfo.alarmZone[i].height);
        //printf("\n\n\n\n\n\n\n ------------%s\n", region);
        strcat(s_alarm_zone, region);

        if(i == (strAlarmZoneInfo.iZoneNum - 1))
        {
            strcat(s_alarm_zone, "}");
        }
    }

    PR_DEBUG("[%s][%d] alarm zone[%s]\n",__FUNCTION__,__LINE__, s_alarm_zone);

    return s_alarm_zone;
}

/*设置报警区域初始化值*/
VOID IPC_APP_SetAlarmZone_InitValue(VOID)
{
    INT_T ret = -1;
    ALARM_ZONE_INFO_T strAlarmZoneInfo;
    CHAR_T s_alarm_zone_set[64] = {0};

    __tuya_app_read_STR("alarm_zone_size", s_alarm_zone_set, 64);
    PR_DEBUG("tuya_alarm_zone: %s \n", s_alarm_zone_set);
    
    /*解析JSON后赋值给strAlarmZoneInfo结构体*/
    tuya_transJs2St(s_alarm_zone_set, &strAlarmZoneInfo);

    ret = LUX_IVS_SetAlarmRegion((LUX_IVS_ZONE_BASE_ST *) &strAlarmZoneInfo.alarmZone[0]);
    if (ret)
    {
        PR_DEBUG(" LUX_IVS_SetAlarmRegion failed\n");
    }
}
#endif

#ifdef TUYA_DP_ALARM_ZONE_ENABLE
VOID IPC_APP_set_alarm_zone_onoff(BOOL_T alarm_zone_on_off)
{
    BOOL_X bOpen = LUX_FALSE;

    /* Motion detection area setting switch,BOOL type,true means on,false is off*/
    PR_DEBUG("set alarm_zone_onoff:%d \r\n", alarm_zone_on_off);

    /* This feature has been implemented, and developers can make local configuration settings and properties.*/
    bOpen = (BOOL_X)alarm_zone_on_off;
    LUX_IVS_AlarmZone_Open(bOpen);

    if(TRUE == alarm_zone_on_off)
    { 
        IPC_APP_SetAlarmZone_InitValue();
    }

    __tuya_app_write_INT("alarm_zone_onoff", alarm_zone_on_off);

}

BOOL_T IPC_APP_get_alarm_zone_onoff(VOID)
{
    BOOL_T alarm_zone_on_off = __tuya_app_read_INT("alarm_zone_onoff");
    PR_DEBUG("curr alarm_zone_onoff:%d \r\n", alarm_zone_on_off);
    return alarm_zone_on_off;
}
#endif


#ifdef TUYA_DP_ALARM_INTERVAL
CHAR_T s_alarm_interval[4];
LUX_IVS_TIMEINTERVAL_EN g_alarmInterval;

VOID IPC_APP_set_alarm_interval(CHAR_T *p_interval)
{
    
    PR_DEBUG("set alarm_interval:%s \r\n", p_interval); 
    /* Motion detection alarm interval,unit is minutes,ENUM type,"0","1","2"*/
    LUX_IVS_TIMEINTERVAL_EN  interval = LUX_IVS_TIMEINTERVAL_1MIN;

    /*换算成分钟*/
    switch(*p_interval)
    {
        case '0' :
            interval = LUX_IVS_TIMEINTERVAL_1MIN;
            break;
        case '1' :
            interval = LUX_IVS_TIMEINTERVAL_3MIN;
            break;
        case '2' :
            interval = LUX_IVS_TIMEINTERVAL_5MIN;
            break;
        default:
            PR_DEBUG("set alarm_interval value type[%s] not support, it will be setting default value.\r\n", p_interval); 
            interval = LUX_IVS_TIMEINTERVAL_1MIN;
    }
    LUX_IVS_SetTimeInterval(interval);
    g_alarmInterval = interval;

    __tuya_app_write_STR("alarm_interval", p_interval);
}

CHAR_T *IPC_APP_get_alarm_interval(VOID)
{
    /* Motion detection alarm interval,unit is minutes,ENUM type,"0","1","2"*/
    __tuya_app_read_STR("alarm_interval",s_alarm_interval, 4);
    PR_DEBUG("curr alarm_intervaly:%s \r\n", s_alarm_interval);

   return s_alarm_interval;
}
#endif

#ifdef TUYA_DP_SD_STATUS_ONLY_GET
INT_T IPC_APP_get_sd_status(VOID)
{
    INT_T sd_status = 1;
    /* SD card status, VALUE type, 1-normal, 2-anomaly, 3-insufficient space, 4-formatting, 5-no SD card */
    /* Developer needs to return local SD card status */

    sd_status = (INT_T)tuya_ipc_sd_get_status();

    printf("curr sd_status:%d \r\n", sd_status);
    return sd_status;
}
#endif

//------------------------------------------

#ifdef TUYA_DP_SD_STORAGE_ONLY_GET
VOID IPC_APP_get_sd_storage(UINT_T *p_total, UINT_T *p_used, UINT_T *p_empty)
{
    UINT_T total = 0, used = 0, free = 0;

    //unit is kb
    tuya_ipc_sd_get_capacity(&total, &used, &free);
    /* Developer needs to return local SD card status */
    *p_total = total;
    *p_used = used;
    *p_empty = *p_total - *p_used;

    printf("curr sd total:%u used:%u empty:%u \r\n", *p_total, *p_used, *p_empty);
}
#endif

//------------------------------------------

#ifdef TUYA_DP_SD_RECORD_ENABLE
VOID IPC_APP_set_sd_record_onoff(BOOL_T sd_record_on_off)
{
    printf("set sd_record_on_off:%d \r\n", sd_record_on_off);
    /* SD card recording function swith, BOOL type, true means on, false means off.
     * This function has been implemented, and developers can make local configuration settings and properties.*/
    INT_T  ret = -1;

    /*在隐私模式下不执行相关动作*/
    if(TRUE == IPC_APP_get_sleep_mode())
    {
        PR_DEBUG("WARNING: set sd_record_onoff:%d invalied in the sleep mode!\r\n", sd_record_on_off);

        ret = LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_RECORD_TITLE, TUYA_CONFIG_RECORD_ONOFF, sd_record_on_off ? "1" : "0");
        if (0 != ret)
        {
            PR_ERR("ERROR:LUX_INIParse_SetKey ret:%#x", ret);
        }

        return;
    }

    if (sd_record_on_off == TRUE)
    {
        IPC_APP_set_sd_record_mode(IPC_APP_get_sd_record_mode());
    }
    else
    {
        tuya_ipc_ss_set_write_mode(SS_WRITE_MODE_NONE);
    }

    ret = LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_RECORD_TITLE, TUYA_CONFIG_RECORD_ONOFF, sd_record_on_off ? "1" : "0");
    if (0 != ret)
    {
        PR_ERR("ERROR:LUX_INIParse_SetKey ret:%#x", ret);
    }

    return;
}

BOOL_T IPC_APP_get_sd_record_onoff(VOID)
{
    INT_T ret = -1;
    BOOL_X on_off = FALSE;

    ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_RECORD_TITLE, TUYA_CONFIG_RECORD_ONOFF, &on_off);
    if (0 != ret)
    {
        PR_ERR("ERROR:LUX_INIParse_SetKey ret:%#x", ret);
    }
    //printf("curr sd_record_on_off:%d \r\n", on_off);

    return on_off;
}
#endif

//------------------------------------------

#ifdef TUYA_DP_SD_RECORD_MODE
VOID IPC_APP_set_sd_record_mode(UINT_T sd_record_mode)
{
    INT_T ret = -1;

    printf("set sd_record_mode:%d \r\n", sd_record_mode);
    if(0 == sd_record_mode)
    {
         tuya_ipc_ss_set_write_mode(SS_WRITE_MODE_EVENT);
    }
    else if(1 == sd_record_mode)
    {
         tuya_ipc_ss_set_write_mode(SS_WRITE_MODE_ALL);
    }
    else
    {
        printf("Error, should not happen\r\n");
        tuya_ipc_ss_set_write_mode(SS_WRITE_MODE_ALL);
    }

    ret = LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_RECORD_TITLE, TUYA_CONFIG_RECORD_MODE, (sd_record_mode ? "1" : "0"));
    if (0 != ret)
    {
        PR_ERR("ERROR:LUX_INIParse_SetKey ret:%#x", ret);
    }

    return;
}

UINT_T IPC_APP_get_sd_record_mode(VOID)
{
    INT_T ret = -1;
    INT_T mode = 0;

    ret = LUX_INIParse_GetInt(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_RECORD_TITLE, TUYA_CONFIG_RECORD_MODE, &mode);
    if (0 != ret)
    {
        PR_ERR("ERROR:LUX_INIParse_SetKey ret:%#x", ret);
    }
    //printf("curr sd_record_mode:%d \r\n", mode);
    
    return mode;
}

#endif

//------------------------------------------

#ifdef TUYA_DP_SD_UMOUNT
BOOL_T IPC_APP_unmount_sd_card(VOID)
{
    BOOL_T umount_ok = TRUE;

    CHAR_T umount_cmd[256] = {0};
    char buffer[512] = {0};

    snprintf(umount_cmd, 256, "umount /dev/mmcblk0p1;");

    FILE *pp = popen(umount_cmd, "r");
    if (NULL != pp)
    {
        fgets(buffer, sizeof(buffer), pp);
        printf("%s\n", buffer);
        pclose(pp);
    }
    else
    {
        umount_ok = FALSE;
    }
    printf("unmount result:%d \r\n", umount_ok);
    
    return umount_ok;
}
#endif

//------------------------------------------

#ifdef TUYA_DP_SD_FORMAT
/* -2000: SD card is being formatted, -2001: SD card formatting is abnormal, -2002: No SD card, 
   -2003: SD card error. Positive number is formatting progress */
STATIC INT_T s_sd_format_progress = 0;
void *thread_sd_format(void *arg)
{
    /* First notify to app, progress 0% */
    s_sd_format_progress = 0;
    IPC_APP_report_sd_format_status(s_sd_format_progress);
    sleep(1);

    /* Stop local SD card recording and playback, progress 10%*/
    s_sd_format_progress = 10;
    IPC_APP_report_sd_format_status(s_sd_format_progress);
    tuya_ipc_ss_set_write_mode(SS_WRITE_MODE_NONE);
    tuya_ipc_ss_pb_stop_all();
    sleep(1);

    /* Delete the media files in the SD card, the progress is 30% */
    s_sd_format_progress = 30;
    IPC_APP_report_sd_format_status(s_sd_format_progress);
    //tuya_ipc_ss_delete_all_files();
    sleep(1);

    /* Perform SD card formatting operation */
    tuya_ipc_sd_format();

    s_sd_format_progress = 80;
    IPC_APP_report_sd_format_status(s_sd_format_progress);
    //TODO
    //tuya_ipc_ss_set_write_mode(SS_WRITE_MODE_ALL);
    IPC_APP_set_sd_record_onoff( IPC_APP_get_sd_record_onoff());

    sleep(1);
    IPC_APP_report_sd_storage();
    /* progress 100% */
    s_sd_format_progress = 100;
    IPC_APP_report_sd_format_status(s_sd_format_progress);

    pthread_exit(0);
}

VOID IPC_APP_format_sd_card(VOID)
{
    printf("start to format sd_card \r\n");
    /* SD card formatting.
     * The SDK has already completed the writing of some of the code, 
     and the developer only needs to implement the formatting operation. */

    pthread_t sd_format_thread;
    pthread_create(&sd_format_thread, NULL, thread_sd_format, NULL);
    pthread_detach(sd_format_thread);
}
#endif

#ifdef TUYA_DP_SD_FORMAT_STATUS_ONLY_GET
INT_T IPC_APP_get_sd_format_status(VOID)
{
    return s_sd_format_progress;
}
#endif

#ifdef TUYA_DP_TEMPERATURE
INT_T IPC_APP_get_temperature_value(VOID)
{
    LUX_HAL_TH_PARAM thParam;
    memset(&thParam, 0, sizeof(thParam));
    LUX_HAL_GetTemperatureAndHumidity(&thParam);

    return (int)(thParam.cTemp * 100);
}
#endif

#ifdef TUYA_DP_HUMIDITY
INT_T IPC_APP_get_humidity_value(VOID)
{
    LUX_HAL_TH_PARAM thParam;
    memset(&thParam, 0, sizeof(thParam));
    LUX_HAL_GetTemperatureAndHumidity(&thParam);

    return (int)(thParam.humidity * 100);
}
#endif

#ifdef TUYA_DP_PERSON_TRACKER
VOID IPC_APP_set_person_tracker(BOOL_T person_tracker)
{
    INT_T ret = -1;
    CHAR_T tmpStr[8] = {0};

    PR_DEBUG("set person_tracker:%d \r\n", person_tracker);

    /*在隐私模式下不执行相关动作*/
    if(TRUE == IPC_APP_get_sleep_mode())
    {
        PR_DEBUG("WARNING: set person_tracker:%d invalied in the sleep mode!\r\n", person_tracker);

        if(person_tracker)
        {
            memcpy(tmpStr, "true", 5);
        }
        else
        {
            memcpy(tmpStr, "false", 6);
        }

        ret = LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_FUNCTION,"person_tracker", tmpStr);
        if(0 != ret)
        {
            PR_DEBUG("LUX_INIParse_SetKey failed!, error num [0x%x] ", ret);
        }

        return;
    }

    if(IPC_APP_get_patrol_switch())
    {
        PR_DEBUG("Set person_tracker error, Motor in the patrol mode \r\n");
        return;
    }

    /* Status indicator,BOOL type,true means on,false means off */
    if(person_tracker)
    {
        ret = LUX_IVS_StartAlgo(LUX_IVS_PERSON_TRACK_EN);
        if(0 != ret)
        {
            PR_DEBUG("LUX_IVS_StartAlgo failed.\r\n");
            return;
        }
        memcpy(tmpStr, "true", 5);
    }
    else
    {
        ret = LUX_IVS_StopAlgo(LUX_IVS_PERSON_TRACK_EN);
        if(0 != ret)
        {
            PR_DEBUG("LUX_IVS_StopAlgo failed.\r\n");
            return;
        }
        memcpy(tmpStr, "false", 6);
    }

    ret = LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_FUNCTION,"person_tracker", tmpStr);
    if(0 != ret)
    {
        PR_DEBUG("LUX_INIParse_SetKey failed!, error num [0x%x] ", ret);
    }

    return;
}


BOOL_T IPC_APP_get_person_tracker(VOID)
{
    BOOL_X PTStatus = LUX_FALSE;
    INT_T ret;
    
    ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_FUNCTION, "person_tracker", &PTStatus);
    if(0 != ret)
    {
        PR_DEBUG("LUX_INIParse_GetBool failed!, error num [0x%x] ", ret);
    }

    PR_DEBUG("curr person_tracker:%d \r\n", PTStatus);

    return (PTStatus ? TRUE : FALSE);

}
#endif

//------------------------------------------
/*PTZ控制0-up, 1-right upper, 2-right, 3-right down, 4-down*/
#ifdef TUYA_DP_PTZ_CONTROL
VOID IPC_APP_ptz_start_move(CHAR_T *p_direction)
{
    PR_DEBUG("ptz start move:direction %s \r\n", p_direction);

    //0-up, 1-upper right, 2-right, 3-lower right, 4-down, 5-down left, 6-left, 7-top left
    INT_T ret = -1;
    LUX_HAL_MOTOR_MOV_DIRECTION_EN direction = LUX_HAL_MOTOR_UP;

    switch(*p_direction)
    {
        case '0' :
            direction = LUX_HAL_MOTOR_UP;
            break;

        case '1' :
            direction = LUX_HAL_MOTOR_UP_RIGHT;
            break;

        case '2' :
            direction = LUX_HAL_MOTOR_RIGHT;
            break;

        case '3' :
            direction = LUX_HAL_MOTOR_DOWN_RIGHT;
            break;

        case '4' :
            direction = LUX_HAL_MOTOR_DOWN;
            break;

        case '5' :
            direction = LUX_HAL_MOTOR_DOWN_LEFT;
            break;

        case '6' :
            direction = LUX_HAL_MOTOR_LEFT;
            break;

        case '7' :
            direction = LUX_HAL_MOTOR_UP_LEFT;
            break;
    }
    
    ret = LUX_HAL_Motor_CtrlMovOneStep(direction);
    if(0 != ret)
    {
        PR_ERR("ERROR:LUX_HAL_Motor_CtrlMovOneStep ret:%#x", ret);
    }

    return;
}
#endif


#ifdef TUYA_DP_PTZ_STOP
VOID IPC_APP_ptz_stop_move(VOID)
{
    PR_DEBUG("ptz stop move \r\n");
    /* PTZ rotation stops */
    LUX_HAL_MOTOR_Stop();

    return;

}
#endif

/*PTZ自检默认关闭*/
#ifdef TUYA_DP_PTZ_CHECK
void IPC_APP_ptz_check(VOID)
{
    PR_DEBUG("ptz check \r\n");
}
#endif

/*PTZ移动追踪开关 Move tracking switch, BOOL type, true means on, false means closed*/
#ifdef TUYA_DP_TRACK_ENABLE
void IPC_APP_track_enable(BOOL_T track_enable)
{
    PR_DEBUG("track_enable %d\r\n",track_enable);
}

BOOL_T IPC_APP_get_track_enable(void)
{
    char track_enable = 0;
    //the value you get yourself
    return (BOOL_T)track_enable;
}

#endif

#ifdef TUYA_DP_HUM_FILTER
#ifdef TUYA_DP_ALARM_FUNCTION
VOID IPC_APP_set_human_filter(BOOL_T hum_filter)
{
    INT_X ret = LUX_ERR;
    INT_X alarm_sensitivity = LUX_IVS_SENSITIVE_LOW_EN;

    PR_DEBUG("set human fliter :%d \r\n", hum_filter);

#ifdef CONFIG_BABY
    if (TRUE == hum_filter)
    {
        ret = LUX_IVS_StartAlgo(LUX_IVS_PERSON_DET_EN);
        if (0 != ret)
        {
            PR_DEBUG("LUX_IVS_StartAlgo failed!, error num [0x%x] ", ret);
        }

        /*人形侦测灵敏度, 通道关闭时,设置灵敏度无效, 需要开启通道的时候重新设置下*/
        alarm_sensitivity = __tuya_app_read_INT("alarm_sen");
        ret = LUX_IVS_SetSensitive(LUX_IVS_PERSON_DET_EN, alarm_sensitivity);
        if (0 != ret)
        {
            PR_DEBUG("IPC_APP_set_alarm_sensitivity failed!, error num [0x%x] ", ret);
        }
    }
    else
    {
        ret = LUX_IVS_StopAlgo(LUX_IVS_PERSON_DET_EN);
        if (0 != ret)
        {
            PR_DEBUG("LUX_IVS_StartAlgo failed!, error num [0x%x] ", ret);
        }
    }
#else
    /* motion detection alarm switch,BOOL type,true means on,false means off.*/
    if (0 == __tuya_app_read_INT("alarm_onoff"))
    {
        PR_DEBUG("mux with alarm_onoff:%d hum_filter:%d\r\n", 0, hum_filter);
        return;
    }

    if(TRUE == hum_filter)
    {
        ret = LUX_IVS_StartAlgo(LUX_IVS_PERSON_DET_EN);
        if(0 != ret)
        {
            PR_DEBUG("LUX_IVS_StartAlgo failed!, error num [0x%x] ", ret);
        }

        /*人形侦测灵敏度, 通道关闭时,设置灵敏度无效, 需要开启通道的时候重新设置下*/
        alarm_sensitivity = __tuya_app_read_INT("alarm_sen");
        ret = LUX_IVS_SetSensitive(LUX_IVS_PERSON_DET_EN, alarm_sensitivity);
        if(0 != ret)
        {
            PR_DEBUG("IPC_APP_set_alarm_sensitivity failed!, error num [0x%x] ", ret);
        }

        /*开启人形侦测需要关闭移动侦测*/
        ret = LUX_IVS_StopAlgo(LUX_IVS_MOVE_DET_EN);
        if(0 != ret)
        {
            PR_DEBUG("LUX_IVS_StartAlgo failed!, error num [0x%x] ", ret);
        }
    }
    else
    {
        ret = LUX_IVS_StopAlgo(LUX_IVS_PERSON_DET_EN);
        if(0 != ret)
        {
            PR_DEBUG("LUX_IVS_StartAlgo failed!, error num [0x%x] ", ret);
        }

        /*关闭人形侦测, 需要开启移动侦测*/
        ret = LUX_IVS_StartAlgo(LUX_IVS_MOVE_DET_EN);
        if(0 != ret)
        {
            PR_DEBUG("LUX_IVS_StartAlgo failed!, error num [0x%x] ", ret);
        }
    }
#endif

    __tuya_app_write_INT("alarm_human_filter", hum_filter);
}

BOOL_T IPC_APP_get_human_filter(VOID)
{
    BOOL_T alarm_on_off = __tuya_app_read_INT("alarm_human_filter");
    PR_DEBUG("curr human fliter :%d \r\n", alarm_on_off);
    return alarm_on_off;
}
#endif
#endif


#ifdef TUYA_DP_PATROL_MODE
void IPC_APP_set_patrol_mode(CHAR_T *patrol_mode)
{
    if(NULL == patrol_mode)
    {
       PR_ERR("Empty point!\n");
       return;
    }

    /*0:全景巡航, 1:收藏点巡航*/
    PR_DEBUG("patrol_mode %s\r\n", patrol_mode);

    INT_T ret = -1;

    /* 修改参数前停止电机，销毁之前设定的定时器 */
    ret = LUX_HAL_MOTOR_SetCruiseMode(LUX_HAL_MOTOR_CRUISE_STOP, 300);
    if (0 != ret)
    {
        PR_DEBUG("LUX_HAL_MOTOR_Cruise failed!, error num [0x%x] \n", ret);
    }

    /* 全景巡航 */
    if('0' == *patrol_mode)
    {
        /*整日巡航*/
        if( 0 == IPC_APP_get_patrol_tmode())
        {
            ret = LUX_HAL_MOTOR_SetCruiseMode(LUX_HAL_MOTOR_CRUISE_ALLDAY, 300);
            if(0 != ret)
            {
                PR_DEBUG("LUX_HAL_MOTOR_SetCruiseMode failed!, error num [0x%x] \n", ret);
            }
        }
        /*预设时间巡航*/
        else
        {
            ret = LUX_HAL_MOTOR_SetCruiseMode(LUX_HAL_MOTOR_CRUISE_TIMING, 300);
            if(0 != ret)
            {
                PR_DEBUG("LUX_HAL_MOTOR_SetCruiseMode failed!, error num [0x%x] \n", ret);
            }
        }
    }
    /*按预设巡航模式预设时间和收藏点*/
    else if('1' == *patrol_mode)
    {

    }
    LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, "motor", "patrol_mode", patrol_mode);
    return;
}

int IPC_APP_get_patrol_mode(void)
{
    int patrol_model_type;
    LUX_INIParse_GetInt(TUYA_INI_CONFIG_FILE_NAME, "motor", "patrol_mode", &patrol_model_type);

    return patrol_model_type;
}
#endif

/*PTZ巡航选择开关 0 means closed  ，1 means open*/      
#ifdef TUYA_DP_PATROL_SWITCH
void IPC_APP_set_patrol_switch(CHAR_T *patrol_switch)
{
    PR_DEBUG("patrol_switch %s\r\n", patrol_switch);
    INT_T ret = -1;

    /*在隐私模式下不执行相关动作*/
    if(TRUE == IPC_APP_get_sleep_mode())
    {
        PR_DEBUG("WARNING: set patrol_onoff:%s invalied in the sleep mode!\r\n", patrol_switch);

        ret = LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_RECORD_TITLE, "patrol_switch", patrol_switch ? "1" : "0");
        if (0 != ret)
        {
            PR_ERR("ERROR:LUX_INIParse_SetKey ret:%#x", ret);
        }

        return;
    }

    if(IPC_APP_get_person_tracker())
    {
        PR_ERR("Set patrol switch error, Motor in the person tracker mode \r\n");
        return;
    }

    /* 先停一下电机，防止正在转动时配置参数失败，保证配置参数优先级高 */
    ret = LUX_HAL_MOTOR_SetCruiseMode(LUX_HAL_MOTOR_CRUISE_STOP, 0);
    if (0 != ret)
    {
        PR_DEBUG("LUX_HAL_MOTOR_SetCruiseMode failed!, error num [0x%x] \n", ret);
    }

    if('1' == *patrol_switch)
    {
        /*全景巡航*/
        if(0 == IPC_APP_get_patrol_mode())
        {
            /*整日巡航*/
            if(0 == IPC_APP_get_patrol_tmode())
            {
                ret = LUX_HAL_MOTOR_SetCruiseMode(LUX_HAL_MOTOR_CRUISE_ALLDAY, 300);
                if(0 != ret)
                {
                    PR_DEBUG("LUX_HAL_MOTOR_Cruise failed!, error num [0x%x] \n", ret);
                }
            }
            /*预设时间巡航*/
            else
            {
                ret = LUX_HAL_MOTOR_SetCruiseMode(LUX_HAL_MOTOR_CRUISE_TIMING, 300);
                if(0 != ret)
                {
                    PR_DEBUG("LUX_HAL_MOTOR_Cruise failed!, error num [0x%x] \n", ret);
                }
            }
        }
        /*收藏点巡航*/
        else
        {
            //TODO:
        }
    }

    LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, "motor", "patrol_switch", patrol_switch);

    return;
}

int IPC_APP_get_patrol_switch(void)
{
    int patrol_switch = 0;
    LUX_INIParse_GetInt(TUYA_INI_CONFIG_FILE_NAME, "motor", "patrol_switch",&patrol_switch);

    return patrol_switch;

}

void IPC_APP_ptz_preset_reset(S_PRESET_CFG *preset_cfg)
{
    /*Synchronize data from server*/
    return;
}

#endif

/*设置巡航时间模式 0整日巡航  1预设时间巡航*/
#ifdef TUYA_DP_PATROL_TMODE
void IPC_APP_set_patrol_tmode(CHAR_T *patrol_tmode)
{
    
    PR_DEBUG("patrol_tmode %s\n", patrol_tmode);
    int ret = -1;

    /* 修改参数前停止电机，销毁之前设定的定时器 */
    ret = LUX_HAL_MOTOR_SetCruiseMode(LUX_HAL_MOTOR_CRUISE_STOP, 300);
    if (0 != ret)
    {
        PR_DEBUG("LUX_HAL_MOTOR_Cruise failed!, error num [0x%x] \n", ret);
    }

    /*全景巡航*/
    if(0 == IPC_APP_get_patrol_mode())
    {
        if( '0' == *patrol_tmode)
        {
            ret = LUX_HAL_MOTOR_SetCruiseMode(LUX_HAL_MOTOR_CRUISE_ALLDAY, 300);
            if(0 != ret)
            {
                PR_DEBUG("LUX_HAL_MOTOR_Cruise failed!, error num [0x%x] \n", ret);
            }
        }
        else
        {
            ret = LUX_HAL_MOTOR_SetCruiseMode(LUX_HAL_MOTOR_CRUISE_TIMING, 300);
            if(0 != ret)
            {
                PR_DEBUG("LUX_HAL_MOTOR_Cruise failed!, error num [0x%x] \n", ret);
            }
        }
    }
    /*收藏点巡航*/
    else
    {
        //TODO:
    }
    LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, "motor", "patrol_tmode", patrol_tmode);    
    return;
}

int IPC_APP_get_patrol_tmode(void)
{
    int patrol_tmode = 0;
    LUX_INIParse_GetInt(TUYA_INI_CONFIG_FILE_NAME, "motor", "patrol_tmode", &patrol_tmode);    
    return patrol_tmode;
}

#endif

/*巡航时间设置*/
#ifdef TUYA_DP_PATROL_TIME
void IPC_APP_set_patrol_time(cJSON * p_patrol_time)
{
   //set patrol_time
    cJSON * pJson = cJSON_Parse((CHAR_T *)p_patrol_time);
    if (NULL == pJson)
    {
        PR_DEBUG("cJSON_Parse error\n");
        return;
    }
    cJSON* t_start = cJSON_GetObjectItem(pJson, "t_start");
    cJSON* t_end = cJSON_GetObjectItem(pJson, "t_end");
    if ((NULL == t_start) || (NULL == t_end))
    {
        PR_DEBUG("cJSON_GetObjectItem error.\n");
        cJSON_Delete(pJson);
        return ;
    }

    LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, "motor","patrol_time_start", t_start->valuestring);
    LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, "motor","patrol_time_end", t_end->valuestring);
    PR_DEBUG("patrol time start[%s], end[%s]\n", t_start->valuestring, t_end->valuestring);

    return;
}

int IPC_APP_get_patrol_time(TUYA_IPC_DP_HANDLER_PT_ST Ttype, char *pOutPTStr)
{
    if(NULL == pOutPTStr)
    {
        PR_ERR("error, empty point\n");
        return -1;
    }

    if( Ttype < TUYA_IPC_DP_HANDLER_PT_START || Ttype > TUYA_IPC_DP_HANDLER_PT_BOTTOM)
    {
        PR_ERR("IPC_APP_get_patrol_time error, return start time\n");
        LUX_INIParse_GetString(TUYA_INI_CONFIG_FILE_NAME, "motor","patrol_time_start", pOutPTStr);
        return 0;
    }

    if(TUYA_IPC_DP_HANDLER_PT_START == Ttype)
    {
        LUX_INIParse_GetString(TUYA_INI_CONFIG_FILE_NAME, "motor","patrol_time_start", pOutPTStr);
    }
    else if(TUYA_IPC_DP_HANDLER_PT_END == Ttype)
    {
        LUX_INIParse_GetString(TUYA_INI_CONFIG_FILE_NAME, "motor","patrol_time_end", pOutPTStr);
    }

    return 0;
}

#endif

/*巡航预设点设置*/
#ifdef TUYA_DP_PRESET_SET
void IPC_APP_set_preset(cJSON * p_preset_param)
{
    INT_T  i = 0;
    int name_len = 0;
    int error_num = 0;
    char respond_add[128] = {0}; 

    cJSON * pJson = cJSON_Parse((CHAR_T *)p_preset_param);
    if (NULL == pJson){
        PR_ERR("null preset set input");
        return;
    }
    cJSON* type = cJSON_GetObjectItem(pJson, "type");
    cJSON* data = cJSON_GetObjectItem(pJson, "data");
    if ((NULL == type) || (NULL == data)){
        PR_ERR("invalid preset set input");
        return;
    }

    PR_DEBUG("preset set type: %d",type->valueint);
    //1:add preset point 2:delete preset point 3:call preset point
    if(type->valueint == 1)
    {
        //char pic_buffer[PRESET_PIC_MAX_SIZE] = {0};  
        //int pic_size = sizeof(pic_buffer);  /*Image to be shown*/
        S_PRESET_POSITION preset_pos;

        /*mpId is 1,2,3,4,5,6，The server will generate a set of its own preset point number information based on the mpid.*/
        preset_pos.mpId = 1;
        preset_pos.ptz.pan = 100; /*horizontal position*/
        preset_pos.ptz.tilt = 60;/*vertical position*/
        cJSON* name = cJSON_GetObjectItem(data, "name");

        if(name == NULL)
        {
            PR_ERR("name is null\n");
            return;
        }

        name_len = strlen(name->valuestring);
        name_len = (name_len > 30)?30:name_len;
        memcpy(&preset_pos.name,name->valuestring,(name_len));
        preset_pos.name[name_len] = '\0';
        error_num = tuya_ipc_preset_add(&preset_pos);

        snprintf(respond_add,128,"{\\\"type\\\":%d,\\\"data\\\":{\\\"error\\\":%d}}",type->valueint,error_num);

        tuya_ipc_dp_report(NULL, TUYA_DP_PRESET_SET,PROP_STR,respond_add,1);

        //tuya_ipc_preset_add_pic(pic_buffer,pic_size); /*if you need show pic ,you should set this api*/
    }
    else if(type->valueint == 2)
    {
        cJSON* num = cJSON_GetObjectItem(data, "num"); //can delete one or more
        cJSON* sets = cJSON_GetObjectItem(data, "sets");
        //char respond_del[128] = {0}; 
        cJSON* del_data;
        int del_num = num->valueint;
        TUYA_STREAM_FRAME_S del_preset;
        for(i = 0; i < del_num; i++)
        {
            del_data = cJSON_GetArrayItem(sets,i);
            cJSON* devId = cJSON_GetObjectItem(del_data, "devId");  /*devid is the preset point number registered in the server*/
            cJSON* mpId = cJSON_GetObjectItem(del_data, "mpId");  /*mpid is the preset point number managed on the device*/
            if((NULL == devId) || (NULL == mpId))
            {
                printf("devid or mpid is error\n");
                return;
            }
            del_preset.seq = atoi(mpId->valuestring);

            printf("%d---%s\n",del_preset.seq,devId->valuestring);

            error_num = (int)time(NULL);

            tuya_ipc_preset_del(devId->valuestring);

            snprintf(respond_add,128,"{\\\"type\\\":%d,\\\"data\\\":{\\\"error\\\":%d}}",type->valueint,error_num);
        }
    }
    else if(type->valueint == 3)
    {
       // cJSON* mpId = cJSON_GetObjectItem(data, "mpId");

        //preset_seq = atoi(mpId->valuestring);
        //get your seq pos and go there
    }

    return;
}

#endif

/*巡航状态查询*/
//TODO:待更改, app端暂时没有用到该功能
#ifdef TUYA_DP_PATROL_STATE
void IPC_APP_patrol_state(int *patrol_state)
{
    int val = 0;
    //return your patrol_state
    *patrol_state = val;
    printf("patrol_state %d\r\n", *patrol_state);
}
#endif


#ifdef TUYA_DP_LINK_MOVE_SET
VOID IPC_APP_set_link_pos(INT_T bind_seq)
{
    /*set the link pos*/
    printf("IPC_APP_set_bind_pos:%d \r\n", bind_seq);
    /*demo
    step1: get the current position
    step2: save the position to flash
    */
    return;
}
#endif


#ifdef TUYA_DP_LINK_MOVE_ACTION
VOID IPC_APP_set_link_move(INT_T bind_seq)
{
    /*move to the link pos*/
    printf("IPC_APP_set_bind_move:%d \r\n", bind_seq);
    /*demo
     step1: get the position base seq
     step2: go to the target position
    */
    return;
}
#endif

//------------------------------------------

#ifdef TUYA_DP_BLUB_SWITCH
extern BOOL_X g_pwmBlubEnable;
VOID IPC_APP_set_blub_onoff(BOOL_T blub_onoff)
{
    printf("set blub_onoff:%d \r\n", blub_onoff);
    
    if(blub_onoff)
        g_pwmBlubEnable = LUX_TRUE;
    else
        g_pwmBlubEnable = LUX_FALSE;

    __tuya_app_write_INT("blub_onoff", blub_onoff);
}

BOOL_T IPC_APP_get_blub_onoff(VOID)
{
    BOOL_T blub_onoff = __tuya_app_read_INT("blub_onoff");
    printf("curr blub_on_off:%d \r\n", blub_onoff);
    return blub_onoff;
}
#endif

//------------------------------------------

#ifdef TUYA_DP_ELECTRICITY
INT_T IPC_APP_get_battery_percent(VOID)
{
    //TODO
    /* battery power percentage VALUE type,[0-100] */

    return 80;
}
#endif

#ifdef TUYA_DP_POWERMODE
CHAR_T *IPC_APP_get_power_mode(VOID)
{
    //TODO
    /* Power supply mode, ENUM type, 
    "0" is the battery power supply state, "1" is the plug-in power supply state (or battery charging state) */

    return "1";
}
#endif
//#ifdef BABY_PRO
#ifdef TUYA_DP_AO_VOLUME

VOID IPC_APP_set_AO_VOLUME(INT_T VALUE)
{
    if (VALUE == 0)
    {
        LUX_AO_Set_Vloume(1);
    }
    else
    {
        LUX_AO_Set_Vloume(VALUE);
    }
    IPC_APP_AUDIO_PlayBeep(IPC_APP_AUDIO_DING);
}

INT_T IPC_APP_get_AO_VOLUME(VOID)
{
    return LUX_AO_Get_Volume();
}
#endif

#ifdef TUYA_DP_ALBUM_ONOFF
VOID IPC_APP_set_Album_onoff(BOOL_T album_onoff)
{    
    /*在隐私模式下不执行相关动作*/
    if (TRUE == IPC_APP_get_sleep_mode())
    {
        PR_DEBUG("WARNING: set IPC_APP_set_Album_onoff[%d] mux with sleep mode!\r\n", album_onoff);
        return;
    }
    if(LUX_TRUE == album_onoff)
    {
        LUX_Jpeg_Album_Start();
    }
    else
    {   
        printf("LUX_Jpeg_Album_Stop by user:%d \r\n", album_onoff);
        LUX_Jpeg_Album_Stop();
    }
    return;
}
BOOL_T IPC_APP_get_Album_onoff(VOID)
{
    return LUX_Jpeg_Album_IsStart();
}
#endif

//#endif
/*****************************baby monitor***********************************/
#ifdef CONFIG_BABY
#ifdef TUYA_DP_SMILE_SHOT_ONOFF
BOOL_T IPC_APP_get_smile_shot_onoff(VOID)
{
    int smile_shot_onoff = __tuya_app_read_INT("smile_shot_onoff");
    printf("curr smile_shot_onoff:%d \r\n", smile_shot_onoff);

    return smile_shot_onoff;
}
VOID IPC_APP_set_smile_shot_onoff(BOOL_T smile_shot_onoff)
{
    int ret = 0;

    /*在隐私模式下不执行相关动作*/
    if (TRUE == IPC_APP_get_sleep_mode())
    {
        LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_FUNCTION, "smile_shot_onoff", smile_shot_onoff ? "1" : "0");
        PR_DEBUG("WARNING: set smile_shot_onoff[%d] mux with sleep mode!\r\n", smile_shot_onoff);
        return;
    }

    //start & stop
    do
    {
        printf("set smile_shot_onoff:%d \r\n", smile_shot_onoff);

        if (smile_shot_onoff)
        {
            ret = LUX_IVS_SmileDet_Start();
            if (ret != 0)
            {
                printf("%s:%d ERROR!\r\n", __func__, __LINE__);
                break;
            }
        }
        else
        {
            ret = LUX_IVS_SmileDet_Stop();
            if (ret != 0)
            {
                printf("%s:%d ERROR!\r\n", __func__, __LINE__);
                break;
            }
        }
        __tuya_app_write_INT("smile_shot_onoff", smile_shot_onoff);
    } while (0);

    return ;
}
#endif

#ifdef TUYA_DP_COVER_FACE_ONOFF
BOOL_T IPC_APP_get_cover_face_onoff(VOID)
{
    int cover_face_onoff = __tuya_app_read_INT("cover_face_onoff");
    printf("curr cover_face_onoff:%d \r\n", cover_face_onoff);
    return cover_face_onoff;
}
VOID IPC_APP_set_cover_face_onoff(BOOL_T cover_face_onoff)
{
    //start & stop
    printf("set cover_face_onoff:%d \r\n", cover_face_onoff);

    /*在隐私模式下不执行相关动作*/
    if (TRUE == IPC_APP_get_sleep_mode())
    {
        LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_FUNCTION, "cover_face_onoff", cover_face_onoff ? "1" : "0");
        PR_DEBUG("WARNING: set cover_face_onoff[%d] mux with sleep mode!\r\n", cover_face_onoff);
        return;
    }

    if (cover_face_onoff)
    {
        LUX_IVS_CoverFace_Start();
    }
    else
    {
        LUX_IVS_CoverFace_Stop();
    }
    __tuya_app_write_INT("cover_face_onoff", cover_face_onoff);



    return;
}
#endif

#ifdef TUYA_DP_PERM_ALARM_ONOFF
BOOL_T IPC_APP_get_perm_alarm_onoff(VOID)
{
    int perm_alarm_onoff = __tuya_app_read_INT("perm_alarm_onoff");
    printf("curr perm_alarm_onoff:%d \r\n", perm_alarm_onoff);

    return perm_alarm_onoff;
}
VOID IPC_APP_set_perm_alarm_onoff(BOOL_T perm_alarm_onoff)
{
    int ret = 0;

    /*在隐私模式下不执行相关动作*/
    if (TRUE == IPC_APP_get_sleep_mode())
    {
        LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_FUNCTION, "perm_alarm_onoff", perm_alarm_onoff ? "1" : "0");
        PR_DEBUG("WARNING: set perm_alarm_onoff[%d] mux with sleep mode!\r\n", perm_alarm_onoff);
        return;
    }

    //start & stop
    do
    {
        printf("set perm_alarm_onoff:%d \r\n", perm_alarm_onoff);

        if (perm_alarm_onoff)
        {
            ret = LUX_IVS_Perm_Start();
            if (ret != 0)
            {
                printf("%s:%d ERROR!\r\n", __func__, __LINE__);
                break;
            }
        }
        else
        {
            ret = LUX_IVS_Perm_Stop();
            if (ret != 0)
            {
                printf("%s:%d ERROR!\r\n", __func__, __LINE__);
                break;
            }
        }
        __tuya_app_write_INT("perm_alarm_onoff", perm_alarm_onoff);
    } while (0);

    return;
}
#endif

#ifdef TUYA_DP_TH_ALARM_ONOFF

BOOL_T IPC_APP_get_th_alarm_onoff(VOID)
{
    int th_alarm_onoff = __tuya_app_read_INT("th_alarm_onoff");
    printf("curr th_alarm_onoff:%d \r\n", th_alarm_onoff);

    return th_alarm_onoff;
}
VOID IPC_APP_set_th_alarm_onoff(BOOL_T th_alarm_onoff)
{
    printf("set th_alarm_onoff:%d \r\n", th_alarm_onoff);
    
    /*在隐私模式下不执行相关动作*/
    if (TRUE == IPC_APP_get_sleep_mode())
    {
        LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_FUNCTION, "th_alarm_onoff", th_alarm_onoff ? "1" : "0");
        PR_DEBUG("WARNING: set th_alarm_onoff[%d] mux with sleep mode!\r\n", th_alarm_onoff);
        return;
    }

    if (th_alarm_onoff)
    {
        g_halTempHumiEnable = TRUE;
    }
    else
    {
        g_halTempHumiEnable = FALSE;
    }
    __tuya_app_write_INT("th_alarm_onoff", th_alarm_onoff);

    return;
}
#endif
#ifdef TUYA_DP_PERM_AREA
VOID IPC_APP_set_perm_area_draw(cJSON *p_alarm_zone)
{
    ALARM_ZONE_INFO_T strAlarmZoneInfo;
    LUX_IVS_PERM_INPUT_POINT inputPoint;

    if (NULL == p_alarm_zone)
    {
        return;
    }

    /*Motion detection area setting switch*/
    printf("%s %d set alarm_zone_set:%s \r\n", __FUNCTION__, __LINE__, (char *)p_alarm_zone);

    /*解析JSON后赋值给strAlarmZoneInfo结构体*/
    tuya_transJs2St((char *)p_alarm_zone, &strAlarmZoneInfo);

    //todo :perm_alarm region setting

    memset(&inputPoint, 0, sizeof(inputPoint));
    inputPoint.x = strAlarmZoneInfo.alarmZone[0].pointX;
    inputPoint.y = strAlarmZoneInfo.alarmZone[0].pointY;
    inputPoint.width = strAlarmZoneInfo.alarmZone[0].width;
    inputPoint.height = strAlarmZoneInfo.alarmZone[0].height;

    LUX_IVS_Perm_ConfigArea(&inputPoint);

    __tuya_app_write_STR("perm_area", (char *)p_alarm_zone);

    return;
}
static CHAR_T s_perm_area_zone[248] = {0};
char *IPC_APP_get_perm_area_draw(VOID)
{
    INT_X i;
    CHAR_T alarm_zone[64] = {0};
    ALARM_ZONE_INFO_T strAlarmZoneInfo;
    CHAR_T region[64] = {0};

    memset(&strAlarmZoneInfo, 0x00, sizeof(ALARM_ZONE_INFO_T));

    __tuya_app_read_STR("perm_area", alarm_zone, 64);
    tuya_transJs2St((char *)alarm_zone, &strAlarmZoneInfo);

    if (strAlarmZoneInfo.iZoneNum > MAX_ALARM_ZONE_NUM)
    {
        printf("[%s] [%d ]get iZoneNum[%d] error", __FUNCTION__, __LINE__, strAlarmZoneInfo.iZoneNum);
        return s_perm_area_zone;
    }

    for (i = 0; i < strAlarmZoneInfo.iZoneNum; i++)
    {
        //{"169":"{\"num\":1,\"region0\":{\"x\":0,\"y\":0,\"xlen\":50,\"ylen\":50}}"}
        if (0 == i)
        {
            snprintf(s_perm_area_zone, 64, "{\\\"num\\\":%d", strAlarmZoneInfo.iZoneNum);
        }

        snprintf(region, 64, ",\\\"region%d\\\":{\\\"x\\\":%d,\\\"y\\\":%d,\\\"xlen\\\":%d,\\\"ylen\\\":%d}", i, strAlarmZoneInfo.alarmZone[i].pointX,
                 strAlarmZoneInfo.alarmZone[i].pointY, strAlarmZoneInfo.alarmZone[i].width, strAlarmZoneInfo.alarmZone[i].height);
        //printf("\n\n\n\n\n\n\n ------------%s\n", region);
        strcat(s_perm_area_zone, region);

        if (i == (strAlarmZoneInfo.iZoneNum - 1))
        {
            strcat(s_perm_area_zone, "}");
        }
    }

    PR_DEBUG("[%s][%d] s_perm_area_zone[%s]\n", __FUNCTION__, __LINE__, s_perm_area_zone);

    return s_perm_area_zone;
}
#endif

#ifdef TUYA_DP_PERM_AREA_2
VOID IPC_APP_set_perm_area_2_draw(cJSON *p_alarm_zone)
{
    LUX_ALARM_AREA_ST strAlarmAreaInfo;
    //LUX_IVS_PERM_INPUT_POINT inputPoint;

    if (NULL == p_alarm_zone)
    {
        return;
    }

    /*Motion detection area setting switch*/
    printf("%s %d set alarm_zone_2_set:%s \r\n", __FUNCTION__, __LINE__, (char *)p_alarm_zone);

    /*解析JSON后赋值给strAlarmZoneInfo结构体*/
    LUX_IVS_Perm_Json2St_2((char *)p_alarm_zone, &strAlarmAreaInfo);

    // //todo :perm_alarm region setting

    LUX_IVS_Perm_ConfigArea2(&strAlarmAreaInfo);
    __tuya_app_write_STR("perm_area_2", (char *)p_alarm_zone);

    return;
}
//static CHAR_T s_perm_area_2_zone[248] = {0};
char *IPC_APP_get_perm_area_2_draw(VOID)
{
    INT_X i;
    CHAR_T alarm_zone[248] = {0};
    LUX_ALARM_AREA_ST strAlarmAreaInfo;
    CHAR_T region[248] = {0};

    memset(s_perm_area_zone, 0, sizeof(s_perm_area_zone));
    
    __tuya_app_read_STR("perm_area_2", alarm_zone, 248);
    LUX_IVS_Perm_Json2St_2((char *)alarm_zone, &strAlarmAreaInfo);

    if (strAlarmAreaInfo.iPointNum > MAX_PERM_AREA_POINTS_NUM)
    {
        printf("[%s] [%d ]get iPointNum[%d] error", __FUNCTION__, __LINE__, strAlarmAreaInfo.iPointNum);
    }

    for (i = 0; i < strAlarmAreaInfo.iPointNum; i++)
    {
        //{"num":4,"point0":{"x":0,"y":0},"point1":{"x":100,"y":0},"point2":{"x":100,"y":100},"point3":{"x":0,"y":100}}
        if (0 == i)
        {
            snprintf(s_perm_area_zone, 248, "{\\\"num\\\":%d", strAlarmAreaInfo.iPointNum);
        }

        snprintf(region, 248, ",\\\"point%d\\\":{\\\"x\\\":%d,\\\"y\\\":%d}", i, strAlarmAreaInfo.alarmPonits[i].pointX,strAlarmAreaInfo.alarmPonits[i].pointY);
        //printf("\n\n\n\n\n\n\n ------------%s\n", region);
        strcat(s_perm_area_zone, region);

        if (i == (strAlarmAreaInfo.iPointNum - 1))
        {
            strcat(s_perm_area_zone, "}");
        }
    }
    printf("%s size %d  strlen %d\n",s_perm_area_zone,sizeof(s_perm_area_zone),strlen(s_perm_area_zone));

    PR_DEBUG("s_perm_area_zone[%s]\n", s_perm_area_zone);
    //memset(s_perm_area_zone , 0 ,sizeof(s_perm_area_zone));
    return s_perm_area_zone;
}
#endif

#ifdef TUYA_DP_SLEEP_DET_ONOFF
BOOL_T IPC_APP_get_sleep_det_onoff(VOID)
{
    int sleep_det_onoff = __tuya_app_read_INT("sleep_det_onoff");
    printf("curr sleep_det_onoff:%d \r\n", sleep_det_onoff);

    return sleep_det_onoff;
}
VOID IPC_APP_set_sleep_det_onoff(BOOL_T sleep_det_onoff)
{
    //start & stop
    printf("set set_sleep_det_onoff:%d \r\n", sleep_det_onoff);

    /*在隐私模式下不执行相关动作*/
    if (TRUE == IPC_APP_get_sleep_mode())
    {
        LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_FUNCTION, "sleep_det_onoff", sleep_det_onoff ? "1" : "0");
        PR_DEBUG("WARNING: set sleep_det_onoff[%d] mux with sleep mode!\r\n", sleep_det_onoff);
        return;
    }

    if (sleep_det_onoff)
    {
        LUX_IVS_SleepDet_Start();
    }
    else
    {
        LUX_IVS_SleepDet_Stop();
    }
    __tuya_app_write_INT("sleep_det_onoff", sleep_det_onoff);



    return;
}
#endif

#ifdef TUYA_DP_NURSERY_RHYME
INT_T IPC_APP_get_nursery_rhyme(VOID)
{
    STATIC INT_T nursery_rhyme = 0;
    
    nursery_rhyme = __tuya_app_read_INT("choose_nursery_rhyme");
    printf("get choose_nursery_rhyme:%d \r\n", nursery_rhyme);
    return nursery_rhyme;
}

VOID IPC_APP_set_nursery_rhyme(INT_T nursery_rhyme)
{
    printf("set choose_nursery_rhyme:%d \r\n", nursery_rhyme);
    __tuya_app_write_INT("choose_nursery_rhyme", nursery_rhyme);
    IPC_NurseryRhymeSet(nursery_rhyme);
}
#endif
#if 1//def TUYA_DP_CUSTOMED_VOICE
CHAR_T *IPC_APP_get_customed_voice_action(VOID)
{
    STATIC CHAR_T s_customed_voice_action[4] = {0};
    
    __tuya_app_read_STR("customed_voice_action", s_customed_voice_action, 4);
    printf("get customed_voice_action:%s \r\n", s_customed_voice_action);
    return s_customed_voice_action;
}
VOID IPC_APP_set_customed_voice_action(CHAR_T *p_action)
{
    // STATIC INT_T last_action_status;
    printf("set customed_voice_action:%s \r\n", p_action);
    __tuya_app_write_STR("customed_voice_action", p_action);

    gcustomed_action_get = *p_action -'0';

    if (gcustomed_action_get == LUX_AUDIO_RECORD_START)
    {
        gstart_record_time = getTime_getS();
        if(!cradlesongFp)
        {
            cradlesongFp = fopen(LUX_TEMP_SONG_PATH, "w+");
        }
    }
    else if (gcustomed_action_get == LUX_AUDIO_RECORD_UPDATE)
    {
        if(cradlesongFp != NULL)
        {
            fclose(cradlesongFp);
            cradlesongFp = NULL;
        }
        // char tmpCmd[64] = {0};
        system("mv " LUX_TEMP_SONG_PATH " " LUX_CUSTOM_SONG_PATH);
        gcustomed_action_get = LUX_AUDIO_RECORD_STOP;
    }
    else if(gcustomed_action_get == LUX_AUDIO_RECORD_STOP)
    {
        if(cradlesongFp != NULL)
        {
            fclose(cradlesongFp);
            cradlesongFp = NULL;
        }
    }
    //last_action_status = gcustomed_action_get;
}
#endif

#endif
#ifdef TUYA_DP_ASK_CAMERA
static CHAR_T ret_string[248] = {0};
VOID IPC_APP_set_cmd_ask_camera(CHAR_T *cmd)
{
    strcpy(ret_string,"NULL");
    LUX_SUM_DP_CMD(cmd,ret_string);
}
char *IPC_APP_get_ret_ask_camera(VOID)
{
    return ret_string;
}


#endif


#ifdef TUYA_DP_PUKE_ONOFF
extern VOID_X LUX_ALG_PukeDet_Start();
extern VOID_X LUX_ALG_PukeDet_Stop();
BOOL_T IPC_APP_get_puke_alarm_onoff(VOID)
{
    int puke_alarm_onoff = __tuya_app_read_INT("puke_alarm_onoff");
    printf("curr puke_alarm_onoff:%d \r\n", puke_alarm_onoff);

    return puke_alarm_onoff;
}

VOID IPC_APP_set_puke_alarm_onoff(BOOL_T puke_alarm_onoff)
{
    /*在隐私模式下不执行相关动作*/
    if (TRUE == IPC_APP_get_sleep_mode())
    {
        LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_FUNCTION, "puke_alarm_onoff", puke_alarm_onoff ? "1" : "0");
        PR_DEBUG("WARNING: set puke_alarm_onoff[%d] mux with sleep mode!\r\n", puke_alarm_onoff);
        return;
    }

    //start & stop
    printf("set puke_alarm_onoff:%d \r\n", puke_alarm_onoff);
#if 0
    if (puke_alarm_onoff)
    {
        LUX_ALG_PukeDet_Start();
    }
    else
    {
        LUX_ALG_PukeDet_Stop();
    }
#endif
    __tuya_app_write_INT("puke_alarm_onoff", puke_alarm_onoff);

    return;
}
#endif

#ifdef TUYA_DP_KICKQUILT_ONOFF
extern VOID_X LUX_ALG_KickQuiltDet_Start();
extern VOID_X LUX_ALG_KickQuiltDet_Stop();
BOOL_T IPC_APP_get_kickquilt_alarm_onoff(VOID)
{
    int kickquilt_onoff = __tuya_app_read_INT("kickquilt_onoff");
    printf("curr kickquilt_onoff:%d \r\n", kickquilt_onoff);

    return kickquilt_onoff;
}

VOID IPC_APP_set_kickquilt_alarm_onoff(BOOL_T kickquilt_onoff)
{
    /*在隐私模式下不执行相关动作*/
    if (TRUE == IPC_APP_get_sleep_mode())
    {
        LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, TUYA_CONFIG_FUNCTION, "kickquilt_onoff", kickquilt_onoff ? "1" : "0");
        PR_DEBUG("WARNING: set kickquilt_onoff[%d] mux with sleep mode!\r\n", kickquilt_onoff);
        return;
    }

    //start & stop
    printf("set kickquilt_onoff:%d \r\n", kickquilt_onoff);
#if 0
    if (kickquilt_onoff)
    {
        LUX_ALG_KickQuiltDet_Start();
    }
    else
    {
        LUX_ALG_KickQuiltDet_Stop();
    }
#endif
    __tuya_app_write_INT("kickquilt_onoff", kickquilt_onoff);

    return;
}
#endif