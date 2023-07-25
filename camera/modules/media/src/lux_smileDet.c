#include <string.h>
#include <unistd.h>
#include <imp_log.h>
#include <imp_common.h>
#include <imp_system.h>
#include <imp_framesource.h>
#include <imp_ivs.h>
#include <iaac.h>

#include <ivs_common.h>
#include <ivs_interface.h>
#include <ivs_inf_smileDet.h>

//#include "comm_def.h"
#include "lux_base.h"
#include "ivs_interface.h"
#include "lux_event.h"
#include "lux_video.h"
#include "lux_iniparse.h"
#include "lux_smileDet.h"
#include "lux_fsrc.h"
#include "lux_summary.h"

//#include "sample-common.h"


#define TAG "SAMPLE-SMILEDET"
#define TIME_OUT 1500

#define LUX_IVS_SMILE_CONFIDENCE 0.90
#define LUX_IVS_FACE_COVER_TIME  (60 * 1000) //1min检测不到人脸报警，unit:ms

typedef struct 
{
    BOOL_X bInit;
    BOOL_X bStart;
    // BOOL_X bCoverFaceStart;

    int ivsGrpNum;
    int ivsChnNum;
    IMPCell srcCell;
    IMPCell dstCell;
    IMPIVSInterface *pInterface;
    OSI_MUTEX mutex;

    UINT_X alarmTime;
    UINT_X faceAlarmTime;
    LUX_EVENT_ATTR_ST alarmCbk;
} LUX_IVS_SMILE_DET_ATTR;

LUX_IVS_SMILE_DET_ATTR g_smileAttr = 
{
    .ivsGrpNum = 0,
    .ivsChnNum = 3,
    .srcCell = 
    {
        .deviceID = DEV_ID_FS,
        .groupID = 2,
        .outputID = 2,
    },
    .dstCell = 
    {
        .deviceID = DEV_ID_IVS,
        .groupID = 0,
        .outputID = 0,
    }
};

void LUX_IVS_SmileDet_Process();
// void LUX_IVS_CoverFace_Process();

int LUX_IVS_SmileDet_Init()
{
    INT_X ret = 0;
    BOOL_X bStart = LUX_FALSE;
    //BOOL_X bCoverFaceStart = LUX_FALSE;
    BOOL_X sleepMode = LUX_FALSE;

    /* 获取报警回调函数 */
    LUX_Event_GetAlarmCbk(&g_smileAttr.alarmCbk);

    smiledet_param_input_t param;

    memset(&param, 0, sizeof(smiledet_param_input_t));
    param.frameInfo.width = SENSOR_WIDTH_THIRD;
    param.frameInfo.height = SENSOR_HEIGHT_THIRD;

    param.skip_num = 0;
    param.max_face_box = 3;
    param.scale_factor = 0.6;
    param.rot90 = false; // true:图像顺时针旋转90度 false:图像不旋转

    do
    {
        g_smileAttr.pInterface = SmileDetInterfaceInit(&param);
        if (g_smileAttr.pInterface == NULL)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_CreateGroup(%d) failed\n", g_smileAttr.ivsGrpNum);
            break;
        }

        ret = IMP_IVS_CreateChn(g_smileAttr.ivsChnNum, g_smileAttr.pInterface);
        if (ret < 0)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_CreateChn(%d) failed\n", g_smileAttr.ivsChnNum);
            break;
        }

        ret = IMP_IVS_RegisterChn(g_smileAttr.ivsGrpNum, g_smileAttr.ivsChnNum);
        if (ret < 0)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_RegisterChn(%d, %d) failed\n", g_smileAttr.ivsGrpNum, g_smileAttr.ivsChnNum);
            break;
        }

        ret = IMP_IVS_StartRecvPic(g_smileAttr.ivsChnNum);
        if (ret < 0)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_StartRecvPic(%d) failed\n", g_smileAttr.ivsChnNum);
            break;
        }
    } while (0);
    
    Thread_Mutex_Create(&g_smileAttr.mutex);

    Task_CreateThread(LUX_IVS_SmileDet_Process, NULL);
    // Task_CreateThread(LUX_IVS_CoverFace_Process, NULL);

    g_smileAttr.bInit = LUX_TRUE;
    
    LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "sleep_mode", &sleepMode);
    if (LUX_FALSE == sleepMode)
    {
        LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "smile_shot_onoff", &bStart);
        if (bStart)
        {
            LUX_IVS_SmileDet_Start();
        }
        // LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "cover_face_onoff", &bCoverFaceStart);
        // if (bCoverFaceStart)
        // {
        //     LUX_IVS_CoverFace_Start();
        // }
    }

    return 0;
}

int LUX_IVS_SmileDet_Exit()
{
    INT_X ret = 0;

    do
    {
        ret = IMP_IVS_StopRecvPic(g_smileAttr.ivsChnNum);
        if (ret < 0)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_StopRecvPic(%d) failed\n", g_smileAttr.ivsChnNum);
            break;
        }
        sleep(1);

        ret = IMP_IVS_UnRegisterChn(g_smileAttr.ivsChnNum);
        if (ret < 0)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_UnRegisterChn(%d) failed\n", g_smileAttr.ivsChnNum);
            break;
        }

        ret = IMP_IVS_DestroyChn(g_smileAttr.ivsChnNum);
        if (ret < 0)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_DestroyChn(%d) failed\n", g_smileAttr.ivsChnNum);
            break;
        }

        SmileDetInterfaceExit(g_smileAttr.pInterface);
    } while (0);
    g_smileAttr.bInit = LUX_FALSE;

    return 0;
}


int LUX_IVS_SmileDet_Start()
{
    g_smileAttr.bStart = LUX_TRUE;

    return 0;
}

int LUX_IVS_SmileDet_Stop()
{
    g_smileAttr.bStart = LUX_FALSE;

    return 0;
}

static int LUX_IVS_SmileSnap()
{
    int ret = 0;
    LUX_ENCODE_STREAM_ST picInfo;
    if (LUX_Event_IsAlarmByInterval(g_smileAttr.alarmTime))
    {
        LUX_SUM_Update_Event(LUX_SUM_EVENT_SMILE_DET,gettimeStampS(),NULL);

        g_smileAttr.alarmTime = getTime_getS();
        ret = LUX_BASE_CapturePic_HD(LUX_STREAM_MAIN,&picInfo);
        //ret = LUX_BASE_CapturePic(LUX_STREAM_JPEG, &picInfo);
        if (0 != ret)
        {
            LUX_BASE_ReleasePic_HD(LUX_STREAM_MAIN,&picInfo);
            IMP_LOG_ERR(TAG, " LUX_BASE_CapturePic failed\n");
            return -1;
        }

        /*上报抓图*/
        if (NULL != g_smileAttr.alarmCbk.pEvntReportFuncCB)
        {
            g_smileAttr.alarmCbk.pEvntReportFuncCB(picInfo.pAddr, picInfo.streamLen, LUX_NOTIFICATION_NAME_DEV_LINK);
            printf("%s:%d smile snapshot report success!\n", __func__, __LINE__);
        }

	    LUX_BASE_ReleasePic_HD(LUX_STREAM_MAIN,&picInfo);

        // ret = LUX_BASE_ReleasePic(LUX_STREAM_JPEG, &picInfo);
        // if (0 != ret)
        // {
        //     IMP_LOG_ERR(TAG, " LUX_BASE_ReleasePic failed\n");
        //     return -1;
        // }
    }

    return 0;
}

// static int LUX_IVS_CoverFaceAlarm()
// {
//     int ret = 0;
//     BOOL_X recordOnOff = 0;
//     int recordMode = 0;
//     LUX_ENCODE_STREAM_ST picInfo[2];

//     /*报警时间间隔*/
//     //if (LUX_Event_IsAlarmByInterval(g_smileAttr.alarmTime))
//     {
//         //g_smileAttr.alarmTime = getTime_getS();
//         ret = LUX_BASE_CapturePic_HD(LUX_STREAM_SUB,&picInfo[LUX_STREAM_SUB]);
//         if (0 != ret)
//         {
//             LUX_BASE_ReleasePic_HD(LUX_STREAM_SUB,&picInfo[LUX_STREAM_SUB]);
//             IMP_LOG_ERR(TAG, " LUX_BASE_CapturePic failed\n");
//             return -1;
//         }
//         /*上报抓图*/
//         if (NULL != g_smileAttr.alarmCbk.pEvntReportFuncCB)
//         {
//             g_smileAttr.alarmCbk.pEvntReportFuncCB(picInfo[LUX_STREAM_SUB].pAddr, picInfo[LUX_STREAM_SUB].streamLen, LUX_NOTIFICATION_NAME_FACE);
//         }
        
//         /* 云存储 */
//         if (NULL != g_smileAttr.alarmCbk.pAddCloudEvent && NULL != g_smileAttr.alarmCbk.pDeletCloudEvent 
//             && NULL != g_smileAttr.alarmCbk.pGetCloudEventStatus)
//         {

//             g_smileAttr.alarmCbk.cloudId = g_smileAttr.alarmCbk.pAddCloudEvent(picInfo[LUX_STREAM_SUB].pAddr, picInfo[LUX_STREAM_SUB].streamLen, LUX_EVENT_TYPE_FACE, LUX_CLOUD_STORAGE_TIME); //最长录300s
//             printf("%s:%d cloudId:%d \n", __func__, __LINE__, g_smileAttr.alarmCbk.cloudId);
            
//         }
//         LUX_BASE_ReleasePic_HD(LUX_STREAM_SUB,&picInfo[LUX_STREAM_SUB]);
//         // ret = LUX_BASE_ReleasePic(LUX_STREAM_JPEG, &picInfo);
//         // if (0 != ret)
//         // {
//         //     IMP_LOG_ERR(TAG, " LUX_BASE_ReleasePic failed\n");
//         //     return -1;
//         // }

//         /*事件录像*/
//         ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "record", "record_onoff", &recordOnOff);
//         if (0 != ret)
//         {
//             IMP_LOG_ERR(TAG, "ERROR:LUX_INIParse_SetKey ret:%#x", ret);
//         }

//         ret = LUX_INIParse_GetInt(TUYA_INI_CONFIG_FILE_NAME, "record", "record_mode", &recordMode);
//         if (0 != ret)
//         {
//             IMP_LOG_ERR(TAG, "ERROR:LUX_INIParse_SetKey ret:%#x", ret);
//         }

//         if (LUX_TRUE == recordOnOff && 0 == recordMode)
//         {
//             ret = LUX_Event_Record(LUX_COMM_EVENT_RECORD_TIME);
//             if (0 != ret)
//             {
//                 IMP_LOG_ERR(TAG, "ERROR:LUX_INIParse_SetKey ret:%#x", ret);
//             }
//         }
//     }

//     return ret;
// }

// void LUX_IVS_CoverFace_Start()
// {
//     g_smileAttr.bCoverFaceStart = LUX_TRUE;
// }
// void LUX_IVS_CoverFace_Stop()
// {
//     g_smileAttr.bCoverFaceStart = LUX_FALSE;
// }

// void LUX_IVS_CoverFace_Process()
// {
//     UINT_X currTime = getTime_getMs();


//     while (1)
//     {
//         Thread_Mutex_Lock(&g_smileAttr.mutex);
//         do
//         {
//             if (!g_smileAttr.bCoverFaceStart || (0 == g_smileAttr.faceAlarmTime))
//             {
//                 g_smileAttr.faceAlarmTime = getTime_getMs();
//                 break;
//             }

//             /* 遮脸检测统计及上报 */
//             currTime = getTime_getMs();
//             if (currTime - g_smileAttr.faceAlarmTime > LUX_IVS_FACE_COVER_TIME)
//             {                
//                 g_smileAttr.faceAlarmTime = currTime;
//                 LUX_IVS_CoverFaceAlarm();

//                 printf("%s:%d timeDiff:%d,%d\n", __FUNCTION__, __LINE__, g_smileAttr.faceAlarmTime, currTime - g_smileAttr.faceAlarmTime);
//             }
//         } while (0);
//         Thread_Mutex_UnLock(&g_smileAttr.mutex);

//         usleep(100*1000);
//     }

//     return ;
// }

void LUX_IVS_SmileDet_Process()
{
    int ret = 0;
    smiledet_param_output_t *result = NULL;

    while (1)
    {
        if (!g_smileAttr.bInit || !g_smileAttr.bStart)
        {
            usleep(100 * 1000);
            continue;
        }

        Thread_Mutex_Lock(&g_smileAttr.mutex);
        do
        {
            ret = IMP_IVS_PollingResult(g_smileAttr.ivsChnNum, TIME_OUT);
            if (ret < 0)
            {
                //IMP_LOG_ERR(TAG, "IMP_IVS_PollingResult(%d, %d) failed\n", 0, IMP_IVS_DEFAULT_TIMEOUTMS);
                break;
            }
            ret = IMP_IVS_GetResult(g_smileAttr.ivsChnNum, (void **)&result);
            if (ret < 0)
            {
                IMP_LOG_ERR(TAG, "IMP_IVS_GetResult(%d) failed\n", 0);
                break;
            }

            if (g_smileAttr.bStart 
                && result->count > 0 
                && result->face[0].confidence > LUX_IVS_SMILE_CONFIDENCE 
                && 1 == result->face[0].expression_score)
            {
                LUX_IVS_SmileSnap();
            }

            /* 更新当前人脸出现的时间戳 */
            if (result->count > 0 || (0 == g_smileAttr.faceAlarmTime))
            {
                g_smileAttr.faceAlarmTime = getTime_getMs();
            }
            /*
            for (i = 0; i < r->count; i++)
            {
                LUX_IVS_CoverFace_Report();
                IVSRect *rect = &r->face[i].show_box;
                printf("face[%d] location:%d, %d, %d, %d\n", i, rect->ul.x, rect->ul.y, rect->br.x, rect->br.y);
                printf("face expression score:%f\n", r->face[i].expression_score);
                printf("face confidence:%f\n", r->face[i].confidence);
            }
            */
            ret = IMP_IVS_ReleaseResult(g_smileAttr.ivsChnNum, (void *)result);
            if (ret < 0)
            {
                IMP_LOG_ERR(TAG, "IMP_IVS_ReleaseResult(%d) failed\n", 0);
            }
        } while (0);
        Thread_Mutex_UnLock(&g_smileAttr.mutex);
        
        usleep(30 * 1000);
    }

    return ;
}
