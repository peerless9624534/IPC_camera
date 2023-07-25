/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *www.tuya.comm
**********************************************************************************/

#ifndef _TUYA_IPC_MGR_HANDLER_H
#define _TUYA_IPC_MGR_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_com_defs.h"
#include <pthread.h>
#include "lux_base.h"

#define LUX_TEMP_SONG_PATH   "/tmp/Mycradlesong.pcm"
#define LUX_CUSTOM_SONG_PATH  "/system/bin/audio/Mycradlesong.pcm"

typedef enum
{
    IPC_BOOTUP_FINISH,
    IPC_START_WIFI_CFG,
    IPC_REV_WIFI_CFG,
    IPC_CONNECTING_WIFI,
    IPC_MQTT_ONLINE,
    IPC_RESET_SUCCESS,
}IPC_APP_NOTIFY_EVENT_E;

/* ��ʾ��ö�� */
typedef enum
{
    IPC_APP_AUDIO_WAIT_CONN,
    IPC_APP_AUDIO_SCAN_OK,
    IPC_APP_AUDIO_CONN_OK,
    IPC_APP_AUDIO_MUSIC,
    IPC_APP_AUDIO_BLESSING,
    IPC_APP_AUDIO_DING,
    IPC_APP_AUDIO_MAX,
} IPC_APP_AUDIO_BEEP_TYPE;


typedef enum
{
    IPC_BOOT_NORMAL,
    IPC_BOOT_ENORMAL,
} IPC_BOOT_TYPE;

/* 音频文件操作 */
typedef enum
{
    LUX_SONG_STOP,
    LUX_SONG_PAUSE,
    LUX_SONG_PLAY,
    
} IPC_MUSIC_STATUS;

typedef enum
{
    LUX_SONG_SELECT_FAILED = 98,
    LUX_SONG_OPENSD_FAILED,
} IPC_MUSIC_ERR_CODE;

typedef enum
{
    LUX_AUDIO_NONE = 0,
    LUX_AUDIO_MUSIC,
    LUX_AUDIO_BEEP,
    LUX_AUDIO_P2P,
} IPC_AUDIO_TYPE;

typedef struct 
{
    char path[64];
    OSI_SEM sem;
    unsigned long filePos; //当前播放位置
    unsigned int startTime;//开始播放时间
    int musicIndex; //当前曲目
    char p2pAudioOnOff; //p2p音频
    char audioType; //类型：音乐，铃声，p2p
    char playStatus;
    char cyclesLeft;
} IPC_APP_AUDIO_BEEP_CTRL;
/* Update local time */

OPERATE_RET IPC_APP_Sync_Utc_Time(VOID);
VOID IPC_APP_Show_OSD_Time(VOID);

VOID IPC_APP_Reset_System_CB(GW_RESET_TYPE_E reboot_type);

VOID IPC_APP_Upgrade_Inform_cb(IN CONST FW_UG_S *fw);

VOID IPC_APP_Restart_Process_CB(VOID);

VOID IPC_APP_Upgrade_Firmware_CB(VOID);

VOID IPC_APP_Notify_LED_Sound_Status_CB(IPC_APP_NOTIFY_EVENT_E notify_event);

/* ��Ƶ��ʾ�����ܳ�ʼ�� */
VOID IPC_APP_AUDIO_BeepInit();

/* ��Ƶ��ʾ�������˳� */
VOID IPC_APP_AUDIO_BeepExit();
//play pcm file
void IPC_NurseryRhymePause();
void IPC_NurseryRhymeStart();
int IPC_NurseryRhymeStatus();
int IPC_NurseryRhymeSet(int i);
/* ѡ����Ƶ��ʾ�� */
int IPC_APP_AUDIO_PlayBeep(IPC_APP_AUDIO_BEEP_TYPE type);

/* �Ƴ���ʾ�� ���ȴ����ӡ� ���߳� */
VOID IPC_APP_AUDIO_WaitConn_Stop();

VOID IPC_APP_IVS_AlarmNotify(IN CONST CHAR_T *snap_buffer, IN CONST UINT_T snap_size, IN CONST UINT_T type);

VOID IPC_APP_UpDate_MyCradlesong();

/* 上报相册抓拍开启关闭状态 */
VOID IPC_APP_ALBUM_Stat(BOOL_T bvalue);

#ifdef __cplusplus
}
#endif

#endif  /*_TUYA_IPC_MGR_HANDLER_H*/
