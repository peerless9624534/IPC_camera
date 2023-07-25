/*********************************************************************************
  *Copyright(C),2015-2020, TUYA company www.tuya.comm
  *FileName: tuya_ipc_system_control_demo.c
  *
  * File description：
  * The demo shows how the SDK uses callback to achieve system control, such as：
  * 1. Setting local ID
  * 2. Restart System and Restart Process
  * 3. OTA upgrade
  * 4. Sound and LED prompts.
  *
**********************************************************************************/

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include "tuya_ipc_media.h"
#include "tuya_cloud_com_defs.h"
#include "tuya_cloud_types.h"
#include "tuya_ipc_common_demo.h"
#include "tuya_ipc_system_control_demo.h"
#include "tuya_ipc_dp_utils.h"
#include "tuya_ipc_dp_handler.h"
#include "dirent.h"

#include "lux_audio.h"
#include <lux_hal_led.h>
#include "lux_event.h"
#include "lux_iniparse.h"
#include "lux_hal_misc.h"
#include "lux_base.h"

#include "imp_audio.h"
#include "tuya_ipc_api.h"

#define IPC_APP_AUDIO_WAIT_CONN_INTERVAL 60 /* 循环播放“等待连接”音频间隔，单位：秒 */
#define AUDIO_BEEP_SOUND_BUF_LEN 164 * 1024
#define AUDIO_PCM_BYTES_PER_PACK 640
#define LUX_CRADLESONG_PATH     "/system/sdcard2/Music/CradleSong"
#define LUX_WHITENOISE_PATH     "/system/sdcard2/Music/WhiteNoise"
#define LUX_POETRYSONG_PATH     "/system/sdcard2/Music/PoetrySong"
#define LUX_ENGLISHSONG_PATH    "/system/sdcard2/Music/EnglishSong"
#define LUX_SONGLEN                      10
#define NURSERY_AUDIO_PCM_BYTES_PER_PACK 320
#define NURSERY_AUDIO_PLAY_DURATION      600
#define CRADLE_SONG_CYCLE_COUNT 3
//extern BOOL_X b_newselectsong;
//extern DWORD_X songplay_pieces;
//extern DWORD_X handletype_of_playmusic;
extern DWORD_X bPlayingCryMusic;

//BOOL_X start_audio_status_reported = LUX_FALSE;

//INT_T error_number_response = 1000;

extern unsigned int start_nursery_record_time;

/* 信息上报至平台 */
static void my_respone_dp_bool(unsigned char dp_id, int true_false)
{
    tuya_ipc_dp_report(NULL, dp_id,PROP_BOOL,&true_false,1);
}

IPC_APP_AUDIO_BEEP_CTRL g_audioBeepCtrl;

const char* const AUDIO_LIST[] = {
    "/system/bin/audio/wait_conn.pcm",
    "/system/bin/audio/scan_ok.pcm",
    "/system/bin/audio/connect_ok.pcm",
    "/system/bin/audio/cradlesong.pcm",
    "/system/bin/audio/blessing.pcm",
    "/system/bin/audio/ding.pcm"
};

extern IPC_MGR_INFO_S s_mgr_info;
INT_T IPC_APP_PutAudioPcm(IPC_APP_AUDIO_BEEP_TYPE type);
IPC_BOOT_TYPE IPC_CheckBootStatus();
int IPC_PcmPlay(const char* path);

VOID IPC_APP_NURSERY_RHYME_Process();

VOID IPC_APP_AUDIO_BeepInit()
{
    memset(&g_audioBeepCtrl, 0, sizeof(g_audioBeepCtrl));
    Semaphore_Create(0, &g_audioBeepCtrl.sem);
    Task_CreateThread(IPC_APP_NURSERY_RHYME_Process, NULL);
    return;
}
/* 播放音频提示音 */
int IPC_APP_AUDIO_PlayBeep(IPC_APP_AUDIO_BEEP_TYPE type)
{
    
    if(g_audioBeepCtrl.playStatus == LUX_SONG_PLAY || g_audioBeepCtrl.p2pAudioOnOff)
    {
        printf("play beep %d,%s return audio playing \n", type, AUDIO_LIST[type]);
        return 0;
    }
    //printf("play beep %d,%s \n", type, AUDIO_LIST[type]);
    memset(g_audioBeepCtrl.path, 0, 64);
    if(type == IPC_APP_AUDIO_MUSIC && access(LUX_CUSTOM_SONG_PATH, F_OK) == 0)
    {
        strncpy(g_audioBeepCtrl.path, LUX_CUSTOM_SONG_PATH, strlen(LUX_CUSTOM_SONG_PATH));
    }
    else
    {
        strncpy(g_audioBeepCtrl.path, AUDIO_LIST[type], strlen(AUDIO_LIST[type]));
    }
    if(type == IPC_APP_AUDIO_MUSIC)
    {
        g_audioBeepCtrl.cyclesLeft = CRADLE_SONG_CYCLE_COUNT;
    }
    else
    {
        g_audioBeepCtrl.cyclesLeft = 1;
    }
    g_audioBeepCtrl.audioType = LUX_AUDIO_BEEP;
    g_audioBeepCtrl.playStatus = LUX_SONG_PLAY;
    Semaphore_Post(&g_audioBeepCtrl.sem);
    //Task_CreateThread(IPC_APP_BeepTask, AUDIO_LIST[type]);
    return 0;
}


extern void __tuya_app_int2char(int value, char cBuf[8]);
int IPC_NurseryRhymeSet(int i)
{
    char buff[64] = {0}; 
    if ((opendir("/system/sdcard2/Music/") == NULL) && (i != 0))
    {
        int err = LUX_SONG_OPENSD_FAILED;
        tuya_ipc_dp_report(NULL, TUYA_DP_PLAYMUSIC, PROP_VALUE, &err, 1);
        printf("open Music path failed, please check if sd card exits\n");
        return OPRT_RESOURCE_NOT_READY;
    }
    __tuya_app_int2char(i, buff);
    LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, "function", "choose_nursery_rhyme", buff);
    //memset(g_audioBeepCtrl.path, 0, sizeof(g_audioBeepCtrl.path));
    memset(buff, 0, 64);
    if (i == 0)
    {
        strcpy(buff, LUX_TEMP_SONG_PATH);
    }
    else if ((i >= 1) && (i <= LUX_SONGLEN))
    {
        sprintf(buff, "%s/%d.pcm", LUX_CRADLESONG_PATH, i);
    }
    else if ((i >= (LUX_SONGLEN + 1)) && (i <= (2 * LUX_SONGLEN)))
    {
        sprintf(buff, "%s/%d.pcm", LUX_WHITENOISE_PATH, i - LUX_SONGLEN);
    }
    else if ((i >= (2 * LUX_SONGLEN + 1)) && (i <= (3 * LUX_SONGLEN)))
    {
        sprintf(buff, "%s/%d.pcm", LUX_POETRYSONG_PATH, i - 2 * LUX_SONGLEN);
    }
    else if ((i >= (3 * LUX_SONGLEN + 1)) && (i <= (4 * LUX_SONGLEN)))
    {
        sprintf(buff, "%s/%d.pcm", LUX_ENGLISHSONG_PATH, i - 3 * LUX_SONGLEN);
    }
    
    if(strncmp(buff, g_audioBeepCtrl.path, strlen(buff)))
    {
        g_audioBeepCtrl.playStatus = LUX_SONG_STOP;
        while (g_audioBeepCtrl.playStatus == LUX_SONG_PLAY)
        {
            //wait stop
            usleep(1000);
        }
        //start new song 
        memset(g_audioBeepCtrl.path, 0, 64);
        strncpy(g_audioBeepCtrl.path, buff, strlen(buff));
        g_audioBeepCtrl.filePos = 0;
        g_audioBeepCtrl.startTime = getTime_getS();
    }
    if(i == 0)
    {
        g_audioBeepCtrl.filePos = 0;
    }
    g_audioBeepCtrl.musicIndex = i;
   
    printf("IPC_NurseryRhymeSet %s \n",buff);

    return 1;
}


void IPC_NurseryRhymeStart()
{
    
    //g_audioBeepCtrl.playStatus = LUX_SONG_PLAY;
    g_audioBeepCtrl.audioType = LUX_AUDIO_MUSIC;
    g_audioBeepCtrl.playStatus = LUX_SONG_PLAY;
    Semaphore_Post(&g_audioBeepCtrl.sem);
}

//pause music because of type
void IPC_NurseryRhymePause()
{
    g_audioBeepCtrl.playStatus = LUX_SONG_PAUSE;
}

int IPC_NurseryRhymeStatus()
{
    return g_audioBeepCtrl.playStatus;
}



int IPC_PcmPlay(const char* path)
{
    char buf[NURSERY_AUDIO_PCM_BYTES_PER_PACK] = {0};
    LUX_AUDIO_FRAME_ST aoStream = {0};
    long fpos = 0;
    int rdlen;
    FILE* fp;
    char playend = 1;
    fp = fopen(path, "r");
    if(fp == NULL)
    {
        printf("file: %s not exsit\n", path);
        return -1;
    }

    if(g_audioBeepCtrl.audioType == LUX_AUDIO_MUSIC)
    {
        fseek(fp, g_audioBeepCtrl.filePos, SEEK_SET);
    }
    aoStream.len = NURSERY_AUDIO_PCM_BYTES_PER_PACK;
    printf("playing:=%s= pos %ld\n", path, g_audioBeepCtrl.filePos);
    while ((rdlen = fread(buf, 1, NURSERY_AUDIO_PCM_BYTES_PER_PACK, fp)))
    {
        aoStream.pData = buf;
        LUX_AO_PutStream(&aoStream);
        if(g_audioBeepCtrl.audioType == LUX_AUDIO_MUSIC)
        {
            g_audioBeepCtrl.filePos = ftell(fp);
        }
        if(g_audioBeepCtrl.playStatus != LUX_SONG_PLAY || g_audioBeepCtrl.p2pAudioOnOff)
        {
            if(g_audioBeepCtrl.audioType == LUX_AUDIO_MUSIC)
            {
                LUX_AO_ClearBuffer();
            }
            playend = 0;
            break;
        }
        memset(buf, 0, NURSERY_AUDIO_PCM_BYTES_PER_PACK);
    }
    fclose(fp);
    if(playend)
    {
        g_audioBeepCtrl.filePos = 0;
    }
    IMP_AO_FlushChnBuf(0, 0);
    return 0;
}


/* 童谣处理线程 */ 
VOID IPC_APP_NURSERY_RHYME_Process()
{
    char path[64];
    int errcode = LUX_SONG_STOP;
    int bShouldStop = 1;
    nice(10);//lower priority
    while (1)
    {
        Semaphore_Pend(&g_audioBeepCtrl.sem, SEM_FOREVER);
        //printf(" p2pAudioOnOff %d audioType %d path:%s!!!\n",g_audioBeepCtrl.p2pAudioOnOff, g_audioBeepCtrl.audioType, g_audioBeepCtrl.path);
        if(g_audioBeepCtrl.playStatus != LUX_SONG_PLAY || g_audioBeepCtrl.p2pAudioOnOff)
        {
            continue;
        }
        //printf("IPC_APP_NURSERY_RHYME_Process %s!!!\n",g_audioBeepCtrl.path);
        if(g_audioBeepCtrl.audioType == AUDIO_MUSIC)
        {
            g_audioBeepCtrl.playStatus = LUX_SONG_PLAY;
            tuya_ipc_dp_report(NULL, TUYA_DP_PLAYMUSIC, PROP_VALUE, &g_audioBeepCtrl.playStatus, 1);
        }
        errcode = IPC_PcmPlay(g_audioBeepCtrl.path);
        printf("g_audioBeepCtrl.audioType%d\n",g_audioBeepCtrl.audioType);
        do
        {
            bShouldStop = 1;
            if(errcode || g_audioBeepCtrl.playStatus != LUX_SONG_PLAY)
            {
                break;
            }
            if(strncmp(g_audioBeepCtrl.path, LUX_TEMP_SONG_PATH, 20) == 0)
            {
                break;
            }
            if(g_audioBeepCtrl.p2pAudioOnOff)
            {//p2p开始
                memset(g_audioBeepCtrl.path, 0, sizeof(g_audioBeepCtrl.path));
                break;
            }
            if(g_audioBeepCtrl.audioType == LUX_AUDIO_BEEP)
            {
                if(--g_audioBeepCtrl.cyclesLeft <= 0)
                {
                    memset(g_audioBeepCtrl.path, 0, sizeof(g_audioBeepCtrl.path));
                    break;
                }
            }
            if(g_audioBeepCtrl.audioType == LUX_AUDIO_MUSIC && getTime_getS() - g_audioBeepCtrl.startTime >= NURSERY_AUDIO_PLAY_DURATION)
            {
                printf("song played more than 10min \n");
                break;
            }
            bShouldStop = 0;
            sleep(1);
        }while (0);
        
        if(bShouldStop)
        {
            printf("play stop \n");
            g_audioBeepCtrl.audioType = LUX_AUDIO_NONE;
            g_audioBeepCtrl.playStatus = LUX_SONG_STOP;
            tuya_ipc_dp_report(NULL, TUYA_DP_PLAYMUSIC, PROP_VALUE, &g_audioBeepCtrl.playStatus, 1);
        }
        else
        {
            printf("play again \n");
            Semaphore_Post(&g_audioBeepCtrl.sem);
        }
    }
    return ;
}

//更新自定义语音
VOID IPC_APP_UpDate_MyCradlesong()
{
    
}

/* IVS 报警提示音+上传报警信息+图片 */
VOID IPC_APP_IVS_AlarmNotify(IN CONST CHAR_T *snap_buffer, IN CONST UINT_T snap_size, IN CONST LUX_NOTIFICATION_NAME_E type)
{
    //tuya_ipc_notify_motion_detect(snap_buffer, snap_size, (NOTIFICATION_CONTENT_TYPE_E)type);
    switch (type)
    {
        case LUX_NOTIFICATION_NAME_MOTION:
            IPC_APP_report_move_alarm("move alarmd");
            break;
    #ifdef CONFIG_BABY
        case LUX_NOTIFICATION_NAME_DEV_LINK:
            IPC_APP_report_smile_snap("smile snap");
            break;
        case LUX_NOTIFICATION_NAME_FACE:
            IPC_APP_report_cover_face_alarm("cover alarmd");
            break;
        case LUX_NOTIFICATION_NAME_LINGER:
            IPC_APP_report_perm_alarm("perm alarmd");
            break;
        #ifdef TUYA_DP_ALBUM_ONOFF
        case LUX_NOTIFICATION_NAME_HUMAN:
            IPC_APP_report_album_alarm("album alarmd");
            break;
        #endif
        #ifdef TUYA_DP_SLEEP_DET_ONOFF
        case LUX_NOTIFICATION_NAME_CALL_ACCEPT:
            IPC_APP_report_sleep_det_alarm("sleep start");
            break;
        case LUX_NOTIFICATION_NAME_CALL_NOT_ACCEPT:
            IPC_APP_report_sleep_det_alarm("sleep End");
            break;
        #endif
        #ifdef TUYA_DP_PUKE_ONOFF
        case LUX_NOTIFICATION_NAME_DOORBELL:
            IPC_APP_report_puke_alarm("puke alarmd");
            break;
        #endif
        #ifdef TUYA_DP_KICKQUILT_ONOFF
        case LUX_NOTIFICATION_NAME_PASSBY:
            IPC_APP_report_kickquilt_alarm("kick quilt alarmd");
            break;
        #endif

    #endif
        default:
            printf("%s:%d invalid alarm\n", __func__, __LINE__);
            break;
    }
    tuya_ipc_notify_with_event(snap_buffer, snap_size, NOTIFICATION_CONTENT_JPEG, (NOTIFICATION_NAME_E)type);

    return;
}


#ifdef TUYA_DP_ALBUM_ONOFF
/* 上报相册抓拍开启关闭状态 */
VOID IPC_APP_ALBUM_Stat(BOOL_T bvalue)
{
    IPC_APP_report_album_onoff(bvalue);
}
#endif
/* 
Callback when the user clicks on the APP to remove the device
*/
VOID IPC_APP_Reset_System_CB(GW_RESET_TYPE_E type)
{
    printf("reset ipc success. please restart the ipc %d\n", type);
    IPC_APP_Notify_LED_Sound_Status_CB(IPC_RESET_SUCCESS);
    
    /* 删除设备要将设备恢复出厂设置 */
    //system("/system/init/backup/factory_reset.sh");
    LUX_BASE_System_CMD("/system/init/backup/factory_reset.sh");
    sleep(1);

    /* Developers need to restart IPC operations */
    LUX_Event_Reboot();

    return ;
}

VOID IPC_APP_Restart_Process_CB(VOID)
{
    printf("sdk internal restart request. please restart the ipc\n");
    
    LUX_Event_Reboot();

    return ;
    /* Developers need to implement restart operations. Restart the process or restart the device. */
}

/* OTA */
//Callback after downloading OTA files
VOID __IPC_APP_upgrade_notify_cb(IN CONST FW_UG_S *fw, IN CONST INT_T download_result, IN PVOID_T pri_data)
{
    FILE *p_upgrade_fd = (FILE *)pri_data;
    fclose(p_upgrade_fd);

    if(download_result == 0)
    {
        /* The developer needs to implement the operation of OTA upgrade, 
        when the OTA file has been downloaded successfully to the specified path. [ p_mgr_info->upgrade_file_path ]*/
        system("/system/init/ota_run.sh");
    }
    
    return ;
}

//To collect OTA files in fragments and write them to local files
OPERATE_RET __IPC_APP_get_file_data_cb(IN CONST FW_UG_S *fw, IN CONST UINT_T total_len,IN CONST UINT_T offset,
                             IN CONST BYTE_T *data,IN CONST UINT_T len,OUT UINT_T *remain_len, IN PVOID_T pri_data)
{
    PR_DEBUG("Rev File Data");
    PR_DEBUG("total_len:%d  fw_url:%s", total_len, fw->fw_url);
    PR_DEBUG("Offset:%d Len:%d", offset, len);

    //report UPGRADE process, NOT only download percent, consider flash-write time
    //APP will report overtime fail, if uprgade process is not updated within 60 seconds
    int percent = ((offset * 100) / (total_len + 1));
    tuya_ipc_upgrade_progress_report(percent);
    
    FILE *p_upgrade_fd = (FILE *)pri_data;
    fwrite(data, 1, len, p_upgrade_fd);
    *remain_len = 0;
    
    return OPRT_OK;
}

VOID IPC_APP_Upgrade_Inform_cb(IN CONST FW_UG_S *fw)
{
    PR_DEBUG("Rev Upgrade Info");
    PR_DEBUG("fw->fw_url:%s", fw->fw_url);
    PR_DEBUG("fw->fw_md5:%s", fw->fw_md5);
    PR_DEBUG("fw->sw_ver:%s", fw->sw_ver);
    PR_DEBUG("fw->file_size:%u", fw->file_size);
    
    /* 释放内存 */
    char ota_name[128] = {0};

    sprintf(ota_name, "%s/ota.zip", s_mgr_info.upgrade_file_path);
    FILE* p_upgrade_fd = fopen(ota_name, "w+b");

    tuya_ipc_upgrade_sdk(fw, __IPC_APP_get_file_data_cb, __IPC_APP_upgrade_notify_cb, p_upgrade_fd);
}


static int g_waitConnOngoing = 1;

VOID IPC_APP_AUDIO_WaitConn_Stop()
{
    g_waitConnOngoing = 0;

    return ;
}

/* 循环播放“等待连接”提示音处理线程 */
VOID IPC_APP_AUDIO_WaitConnProcess()
{
    UINT_X timeBegin = (0 - IPC_APP_AUDIO_WAIT_CONN_INTERVAL);
    UINT_X timeNow = 0;

    while (g_waitConnOngoing)
    {
        timeNow = getTime_getS();
        if (timeNow - timeBegin >= IPC_APP_AUDIO_WAIT_CONN_INTERVAL)
        {
            timeBegin = timeNow;
            IPC_APP_AUDIO_PlayBeep(IPC_APP_AUDIO_WAIT_CONN);
        }
        usleep(100*1000);
    }
    printf("%s:%d thread exit! g_waitConnOngoing:%d\n", __func__, __LINE__, g_waitConnOngoing);

    return ;
}

IPC_BOOT_TYPE IPC_CheckBootStatus()
{
    if(access(UNNORMAL_REBOOT_FILE_NAME,F_OK) == 0)
    {
        return IPC_BOOT_ENORMAL;
    }
    return IPC_BOOT_NORMAL;
}

/* Developers need to implement the corresponding prompt sound playback and LED prompts,
   you can refer to the SDK attached files, using TUYA audio files. */
VOID IPC_APP_Notify_LED_Sound_Status_CB(IPC_APP_NOTIFY_EVENT_E notify_event)
{
    printf("curr event:%d \r\n", notify_event);
    switch (notify_event)
    {
        case IPC_BOOTUP_FINISH: /* Startup success */
        {
           //if(IPC_CheckBootStatus() == IPC_BOOT_NORMAL)
            {
                IPC_APP_AUDIO_PlayBeep(IPC_APP_AUDIO_BLESSING);
            }
            break;
        }
        case IPC_START_WIFI_CFG: /* Start configuring the network */
        {
            Task_CreateThread(IPC_APP_AUDIO_WaitConnProcess, NULL);
            LUX_HAL_LedSetStatus(LUX_HAL_LED_GREEN, LUX_HAL_LED_1_HZ);
            LUX_HAL_LedSetStatus(LUX_HAL_LED_RED, LUX_HAL_LED_OFF);
            
            break;
        }
        case IPC_REV_WIFI_CFG: /* Receive network configuration information */
        {
            break;
        }
        case IPC_CONNECTING_WIFI: /* Start Connecting WIFI */
        {
            IPC_APP_AUDIO_PlayBeep(IPC_APP_AUDIO_SCAN_OK);
            LUX_HAL_LedSetStatus(LUX_HAL_LED_GREEN, LUX_HAL_LED_1_HZ);
            break;
        }
        case IPC_MQTT_ONLINE: /* MQTT on-line */
        {
            if(IPC_CheckBootStatus() == IPC_BOOT_NORMAL)
            {
                IPC_APP_AUDIO_PlayBeep(IPC_APP_AUDIO_CONN_OK);
            }
            else
            {
                remove(UNNORMAL_REBOOT_FILE_NAME);
            }
            LUX_HAL_LedSetStatus(LUX_HAL_LED_GREEN, LUX_HAL_LED_ON);
            LUX_HAL_LedSetStatus(LUX_HAL_LED_RED, LUX_HAL_LED_OFF);
            int tmp_light_onoff = IPC_APP_get_light_onoff();
            usleep(200*1000);
            if(tmp_light_onoff == TRUE)
            {
                IPC_APP_set_light_onoff(FALSE);
                usleep(200*1000);
                my_respone_dp_bool(TUYA_DP_LIGHT, FALSE);
            }
            break;
        }
        case IPC_RESET_SUCCESS: /* Reset completed */
        {
            LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, "function", "connect_status", "0");
            usleep(200*1000);
            break;
        }
        default:
        {
            break;
        }
    }
}

/* Callback of talkback mode,turn on or off speaker hardware*/
VOID TUYA_APP_Enable_Speaker_CB(BOOL_T enabled)
{
    printf("enable speaker %d \r\n", enabled);
    g_audioBeepCtrl.p2pAudioOnOff = enabled;
    //TODO
    /* Developers need to turn on or off speaker hardware operations. 
    If IPC hardware features do not need to be explicitly turned on, the function can be left blank. */
}

#define LUX_CRADLESONG_DURATION 20  /* 自定义音频最大时长 单位(s) */
FILE *cradlesongFp = NULL;
extern INT_T gcustomed_action_get;
extern DWORD_T gstart_record_time;
/* Callback of talkback mode,turn on or off the sound */
VOID TUYA_APP_Rev_Audio_CB(IN CONST MEDIA_FRAME_S *p_audio_frame,
                           TUYA_AUDIO_SAMPLE_E audio_sample,
                           TUYA_AUDIO_DATABITS_E audio_databits,
                           TUYA_AUDIO_CHANNEL_E audio_channel)
{
    LUX_AUDIO_FRAME_ST frame;

    frame.pData = p_audio_frame->p_buf;
    frame.len = p_audio_frame->size;
    if (gcustomed_action_get == LUX_AUDIO_RECORD_START)
    {
        if(getTime_getS() - gstart_record_time > 20)
        {
            return;
        }
        if(cradlesongFp)
        {
            fwrite(frame.pData, 1, frame.len, cradlesongFp);
            fflush(cradlesongFp);
        }
        return;
    }
    else if(g_audioBeepCtrl.p2pAudioOnOff)
    {
        //仅在p2p打开的时候播放音频流
        LUX_AO_PutStream(&frame);
    }
    /* Developers need to implement the operations of voice playback*/
    return ;
}

OPERATE_RET IPC_APP_Sync_Utc_Time(VOID)
{
    OPERATE_RET ret = 0;
    TIME_T time_utc;
    INT_T time_zone;
    ret = tuya_ipc_get_service_time_force(&time_utc, &time_zone);
    ret = stime(&time_utc);
    setTimeStampdDiff(time_zone);
    printf("IPC_APP_Sync_Utc_Time:%s %d %s\n", ctime(&time_utc), time_zone, ret?"faild":"ok");
    return OPRT_OK;
}

VOID IPC_APP_Show_OSD_Time(VOID)
{
    struct tm localTime;
    tuya_ipc_get_tm_with_timezone_dls(&localTime);
    PR_DEBUG("show OSD [%04d-%02d-%02d %02d:%02d:%02d]",localTime.tm_year,localTime.tm_mon,localTime.tm_mday,localTime.tm_hour,localTime.tm_min,localTime.tm_sec);
}


