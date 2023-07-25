#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>
#include "tuya_cloud_base_defs.h"
#include "tuya_ipc_dp_utils.h"
#include "tuya_ipc_cloud_storage.h"
#include "tuya_ipc_common_demo.h"
#include "tuya_ipc_system_control_demo.h"
#include "tuya_ipc_media_demo.h"
#include "tuya_ipc_motion_detect_demo.h"
#include "tuya_ipc_doorbell_demo.h"
#include "tuya_iot_config.h"
#include "comm_func.h" 
#include "comm_def.h"
#include <lux_hal_led.h>
#include "lux_iniparse.h"
#include "lux_base.h"
#include "lux_audio.h"
#include "lux_ivs.h"
#include <lux_event.h>
#include "lux_hal_misc.h"
#include "tuya_ipc_stream_storage.h"
#include "lux_perm.h"
#include "lux_smileDet.h"
#include "lux_coverFace.h"
#include "lux_nv2jpeg.h"
#include "tuya_ipc_p2p.h"
#include "lux_sleepDet.h"

#define IPC_APP_STORAGE_PATH    "/system/tuya/"   //Path to save tuya sdk DB files, should be readable, writeable and storable
#define IPC_APP_UPGRADE_FILE    "/tmp/"      //File with path to download file during OTA
//#define IPC_APP_SD_BASE_PATH    "/system/sdcard/"         //SD card mount directory for 1.6.x
#define IPC_APP_SD_BASE_PATH    "/mnt/sdcard/"         //SD card mount directory for 1.7.x
//#define IPC_APP_VERSION         "1.6.3"     //Firmware version displayed on TUYA APP
#define IPC_APP_VERSION         "2.1.4.10"     //Firmware version displayed on TUYA APP
char s_raw_path[32] = "/tmp/";

IPC_MGR_INFO_S s_mgr_info;
STATIC INT_T s_mqtt_status;
BOOL_T g_bFirstState = TRUE;

STATIC VOID __IPC_APP_Get_Net_Status_cb(IN CONST BYTE_T stat)
{
    int connStatus = 0;

    PR_DEBUG("Net status change to:%d", stat);
    switch(stat)
    {
#if defined(WIFI_GW) && (WIFI_GW==1)
        case STAT_CLOUD_CONN:        //for wifi ipc
        case STAT_MQTT_ONLINE:       //for low-power wifi ipc
#endif
#if defined(WIFI_GW) && (WIFI_GW==0)
        case GB_STAT_CLOUD_CONN:     //for wired ipc
#endif
        {
            PR_DEBUG("mqtt is online\r\n");

            /* 避免掉线重连后重复设置 led sound 状态 */
            if (0 == s_mqtt_status && g_bFirstState)
            {
                IPC_APP_Sync_Utc_Time();
                s_mqtt_status = 1;
                g_bFirstState = FALSE;
                IPC_APP_AUDIO_WaitConn_Stop();
                IPC_APP_Notify_LED_Sound_Status_CB(IPC_MQTT_ONLINE);

                /* 获取连接状态，若网络尚未注册，播放连接状态提示音 */
                LUX_INIParse_GetInt(TUYA_INI_CONFIG_FILE_NAME, "function", "connect_status", &connStatus);
                if (0 == connStatus)
                {
                    LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, "function", "connect_status", "1");
                }
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

static int IPC_APP_GetKeyStr(char *pTag, char value[256])
{
    if (NULL == pTag || NULL == value)
    {
        printf("%s:%d null ptr\n", __func__, __LINE__);
        return 0;
    }
    int read = 0;
    int len = 0;
    FILE *fp = NULL;
    char *line = NULL;

    fp = fopen("/dev/mtdblock3", "r"); 
    if (NULL == fp)
    {
        printf("%s:%d open fd error!\n", __func__, __LINE__);
        return 0;
    }

    while (((read = getline(&line, (size_t*)&len, fp)) != -1) && !feof(fp))
    {
        if (0 == strncmp(pTag, line, strlen(pTag)))
        {
            /* 获取字符=和\n之间的有效字符串 */
            sscanf(line, "%*[^=]=%[^\n]", value);
            break;
        }
        len = 0;
    }

    if (line)
        free(line);

    return len;
}

UINT_X IPC_APP_CloudStorage_EventAdd(CHAR_T *snapshot_buffer, UINT_T snapshot_size, ClOUD_STORAGE_EVENT_TYPE_E type, UINT_T max_duration)
{
    static UINT_X event_id = 0;

#if 0
    LUX_BASE_TIMER_PARAM timerParam;

    if (SS_WRITE_MODE_EVENT == tuya_ipc_ss_get_write_mode() && E_STORAGE_STOP == tuya_ipc_ss_get_status())
    {
        /* 创建本地录像处理定时器 */
        memset(&timerParam, 0, sizeof(timerParam));
        timerParam.bTimeStartEnable = LUX_TRUE;
        timerParam.timeStartValue = LUX_COMM_EVENT_RECORD_TIME;
        timerParam.pTimerProcess = (void*)tuya_ipc_ss_stop_event;

        UINT_X ssTimerId = LUX_BASE_TimerCreate(&timerParam);
        LUX_BASE_TimerStart(ssTimerId);

        tuya_ipc_ss_start_event();
    }
#endif
    if (ClOUD_STORAGE_TYPE_EVENT == tuya_ipc_cloud_storage_get_store_mode() 
        && tuya_ipc_cloud_storage_get_event_status_by_id(event_id) == EVENT_NONE)
    {
        event_id = tuya_ipc_cloud_storage_event_add(snapshot_buffer, snapshot_size, 0, max_duration);
        printf("%s:%d @@@@@@@@@@@@@@@@@@@@@@@@ event_id：%d\n", __func__, __LINE__, event_id);
    }

    return event_id;
}

OPERATE_RET IPC_APP_Init_SDK(WIFI_INIT_MODE_E init_mode, CHAR_T *p_token)
{
    int ret = LUX_ERR;
    char tuya_pid[256] = {0};
    char tuya_uuid[256] = {0};
    char tuya_auth_key[256] = {0};
    LUX_EVENT_ATTR_ST IVSFuncCB;
    FILE *fp = NULL;
    CHAR_X tmp[4] = "\0";
    int Debug_level = 0;

    ret = IPC_APP_GetKeyStr("app_pid", tuya_pid);
    if (ret <= 0)
    {
        PR_ERR("ERR: get tuya_pid, ret:%#x", ret);
        return ret;
    }
    ret = IPC_APP_GetKeyStr("app_uuid", tuya_uuid);
    if (ret <= 0)
    {
        PR_ERR("ERR: get tuya_pid, ret:%#x", ret);
        return ret;
    }
    ret = IPC_APP_GetKeyStr("app_key", tuya_auth_key);
    if (ret <= 0)
    {
        PR_ERR("ERR: get tuya_pid, ret:%#x", ret);
        return ret;
    }

    PR_DEBUG("SDK Version:%s\r\n", tuya_ipc_get_sdk_info());

    memset(&s_mgr_info, 0, sizeof(IPC_MGR_INFO_S));
    strcpy(s_mgr_info.storage_path, IPC_APP_STORAGE_PATH);
    strcpy(s_mgr_info.upgrade_file_path, IPC_APP_UPGRADE_FILE);
    strcpy(s_mgr_info.sd_base_path, IPC_APP_SD_BASE_PATH);
    strcpy(s_mgr_info.product_key, tuya_pid);
    strcpy(s_mgr_info.uuid, tuya_uuid);
    strcpy(s_mgr_info.auth_key, tuya_auth_key);
    strcpy(s_mgr_info.dev_sw_version, IPC_APP_VERSION);
    s_mgr_info.max_p2p_user = 5; //TUYA P2P supports 5 users at the same time, including live preview and playback
    PR_DEBUG("Init Value.storage_path %s", s_mgr_info.storage_path);
    PR_DEBUG("Init Value.upgrade_file_path %s", s_mgr_info.upgrade_file_path);
    PR_DEBUG("Init Value.sd_base_path %s", s_mgr_info.sd_base_path);
    PR_DEBUG("Init Value.product_key %s", s_mgr_info.product_key);
    PR_DEBUG("Init Value.uuid %s", s_mgr_info.uuid);
    PR_DEBUG("Init Value.auth_key %s", s_mgr_info.auth_key);
    PR_DEBUG("Init Value.p2p_id %s", s_mgr_info.p2p_id);
    PR_DEBUG("Init Value.dev_sw_version %s", s_mgr_info.dev_sw_version);
    PR_DEBUG("Init Value.max_p2p_user %u", s_mgr_info.max_p2p_user);

    IPC_APP_Set_Media_Info();
    TUYA_APP_Init_Ring_Buffer();

    //IPC_APP_Notify_LED_Sound_Status_CB(IPC_BOOTUP_FINISH);

    TUYA_IPC_ENV_VAR_S env;

    memset(&env, 0, sizeof(TUYA_IPC_ENV_VAR_S));

    strcpy(env.storage_path, s_mgr_info.storage_path);
    strcpy(env.product_key,s_mgr_info.product_key);
    strcpy(env.uuid, s_mgr_info.uuid);
    strcpy(env.auth_key, s_mgr_info.auth_key);
    strcpy(env.dev_sw_version, s_mgr_info.dev_sw_version);
    strcpy(env.dev_serial_num, "tuya_ipc");
    env.dev_obj_dp_cb = IPC_APP_handle_dp_cmd_objs;
    env.dev_dp_query_cb = IPC_APP_handle_dp_query_objs;
    env.status_changed_cb = __IPC_APP_Get_Net_Status_cb;
    env.gw_ug_cb = IPC_APP_Upgrade_Inform_cb;
    env.gw_rst_cb = IPC_APP_Reset_System_CB;
    env.gw_restart_cb = IPC_APP_Restart_Process_CB;
    env.mem_save_mode = FALSE;
    
    /*注册IVS事件上报函数*/
    IVSFuncCB.pEvntReportFuncCB = (pAlarmEventReportCB *)IPC_APP_IVS_AlarmNotify;
    IVSFuncCB.pStartEventRecordFuncCB = (pStartAlarmEventRecordCB *)tuya_ipc_ss_start_event;
    IVSFuncCB.pStopEventRecordFuncCB  = (pStopAlarmEventRecordCB *)tuya_ipc_ss_stop_event;
    /*用于判断录像事件是否在执行*/
    IVSFuncCB.pGetRecordstatus     = (pGetStatusAlarmEventRecordCB)tuya_ipc_ss_get_status;

    IVSFuncCB.pAddCloudEvent        = (pAddCloudStorageEventCB)IPC_APP_CloudStorage_EventAdd;
    IVSFuncCB.pDeletCloudEvent      = (pDeletCloudStorageEventCB)tuya_ipc_cloud_storage_event_delete;
    IVSFuncCB.pGetCloudEventStatus  = (pGetCloudStorageStatusCB)tuya_ipc_cloud_storage_get_event_status_by_id;

    ret = LUX_EVENT_IvsRegistCB(&IVSFuncCB);
    if(ret)
    {
        PR_ERR("LUX_EVENT_IvsRegistCB failed.\n");
    }

    /* 音频提示音功能初始化 */
    IPC_APP_AUDIO_BeepInit();

    IPC_APP_Notify_LED_Sound_Status_CB(IPC_BOOTUP_FINISH);


    fp = fopen(TUYA_DEBUG_LEVEL_FILE_NAME,"r");
    if(NULL != fp)
    {
        fgets(tmp,sizeof(tmp),fp);
        Debug_level = atoi(tmp);
        printf("\r\n[%s]%d: Debug_level = %d\n", __func__, __LINE__, Debug_level);
        fclose(fp);
    }


    tuya_ipc_init_sdk(&env);
    tuya_ipc_set_log_attr(Debug_level, NULL);    /* tuya日志等级[0,5] */
    tuya_ipc_start_sdk(init_mode, p_token);
    return OPRT_OK;
}

VOID usage(CHAR_T *app_name)
{
    printf("%s -m mode -t token -r raw path -h\n", (CHAR_T *)basename(app_name));
    printf("\t m: 0-WIFI_INIT_AUTO 1-WIFI_INIT_AP 2-WIFI_INIT_DEBUG, refer to WIFI_INIT_MODE_E\n"
        "\t t: token get form qrcode info\n"
        "\t r: raw source file path\n"
        "\t h: help info\n");

    return;
}

extern OSI_SEM g_liveVideoMainSem;
extern OSI_SEM g_liveVideoSubSem;
extern OSI_SEM g_liveAudioSem;
extern OSI_SEM g_ivsAlgSem;
#if 0
extern INT_X LUX_ALG_PukeDet_Init();
extern INT_X LUX_ALG_KickQuiltDet_Init();
extern INT_X LUX_ALG_ActionDet_Init();
#endif

int tuya_demo_main(char *inputToken)
{
    WIFI_INIT_MODE_E mode = WIFI_INIT_DEBUG;
    UINT_T timeCnt = 0;
    INT_T i = 0;

#if defined(WIFI_GW) && (WIFI_GW==0)
    mode = WIFI_INIT_NULL;
#endif

    mode = WIFI_INIT_AP;

    /* Init SDK */
    IPC_APP_Init_SDK(mode, NULL);

    /*Demo uses files to simulate audio/video/jpeg inputs. 
    The actual data acquisition needs to be realized by developers. */
    pthread_t main_thread;
    pthread_create(&main_thread, NULL, thread_live_video, (void *)LUX_STREAM_MAIN);
    pthread_detach(main_thread);

    pthread_t sub_thread;
    pthread_create(&sub_thread, NULL, thread_live_video, (void *)LUX_STREAM_SUB);
    pthread_detach(sub_thread);

    pthread_t audio_thread;
    pthread_create(&audio_thread, NULL, thread_live_audio, NULL);
    pthread_detach(audio_thread);

    pthread_t reset_thread;
    pthread_create(&reset_thread, NULL, thread_reset_key, NULL);
    pthread_detach(reset_thread);
    

    /* whether SDK is connected to MQTT */
    while(s_mqtt_status != 1)
    {
        usleep(30000);
    }
    /*At least one system time synchronization after networking*/
    //IPC_APP_Sync_Utc_Time();
    usleep(10000);
    
    /*IVS算法注册,需要连网*/
    LUX_IVS_Iaac_Init();

    /*ivs算法初始化*/
    for (i = 0; i < LUX_IVS_ALGO_NUM; i++)
    {
        Semaphore_Post(&g_ivsAlgSem);
    }

    /* 视频主、子码流开始推流 */
    Semaphore_Post(&g_liveVideoMainSem);
    Semaphore_Post(&g_liveVideoSubSem);
    /* 音频开始推流 */
    Semaphore_Post(&g_liveAudioSem);
    
    //判断文件夹是否存在
    if(-1 == access(TUYA_EVENTS_TIME_DIR,F_OK))
    {
        LUX_BASE_System_CMD("mkdir "TUYA_EVENTS_TIME_DIR);
    }
    /* 微笑抓拍 */
    LUX_IVS_SmileDet_Init();
    /* 人脸检测初始化 */
    LUX_IVS_FaceDet_Init();
    /* 周界检测 */
    LUX_IVS_Perm_Init();
    // /* 相册抓拍 */
    // LUX_Jpeg_Album_Init();

    LUX_IVS_SleepDet_Init();
#if 0
    /*吐奶检测*/
    LUX_ALG_PukeDet_Init();

    /*踢被检测*/
    LUX_ALG_KickQuiltDet_Init();

    /*动作检测*/
    LUX_ALG_ActionDet_Init();
#endif
    /* Start local storage. Tt is recommended to be after ONLINE, or make sure the system time is correct */
    TUYA_APP_Init_Stream_Storage(s_mgr_info.sd_base_path);

    /* Enable TUYA P2P service after the network is CONNECTED. 
       Note: For low-power camera, invoke this API as early as possible(can be before mqtt online) */
    /* lowpower product could call this func at startup*/
    TUYA_APP_Enable_P2PTransfer(s_mgr_info.max_p2p_user);
    
    /* Upload all local configuration item (DP) status when MQTT connection is successful */
    IPC_APP_upload_all_status();

    /* 使能云存储功能 */
    TUYA_APP_Enable_CloudStorage();

    //TUYA_APP_Enable_EchoShow_Chromecast();

    TUYA_APP_Enable_AI_Detect();

   // TUYA_APP_Enable_DOORBELL();

    /*!!!very important! After all module inited, update skill to tuya cloud */
    tuya_ipc_upload_skills();

    /* Starting the detection tasks and trigger alarm reporting/local storage/cloud storage tasks through detection results  */

    //  pthread_t motion_detect_thread;
    //  pthread_create(&motion_detect_thread, NULL, thread_md_proc, NULL);
    //  pthread_detach(motion_detect_thread);

    pthread_t temp_humi_thread;
    pthread_create(&temp_humi_thread, NULL, thread_th_proc, NULL);
    pthread_detach(temp_humi_thread);
    
    UINT_T lastNum = 0;
    while (1)
    {
        /* 60秒校时一次 */

        if (timeCnt >= 60)
        {
            IPC_APP_Sync_Utc_Time();
            timeCnt = 0;
        }
        timeCnt++;
        if(timeCnt % 3 == 0)
        {
            UINT_T p_client_num = 0;
            CLIENT_CONNECT_INFO_S *p_p_conn_info;
            tuya_ipc_get_client_conn_info(OUT &p_client_num,OUT &p_p_conn_info);
            if(lastNum != p_client_num)
            {
                printf("%s:%d TUYA P2P User count = %d\n", __func__, __LINE__,p_client_num);
                lastNum = p_client_num;
            }
            tuya_ipc_free_client_conn_info(p_p_conn_info);    
        }

        sleep(1);
    }

    return 0;
}

