#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <imp_log.h>
#include <imp_common.h>
#include <imp_system.h>
#include <imp_framesource.h>
#include <imp_ivs.h>
#include <iaac.h>

#include <ivs_common.h>
#include <ivs_inf_faceDet.h>
#include <ivs_interface.h>


// #include "comm_def.h"
#include "lux_base.h"
#include "ivs_interface.h"
#include "lux_video.h"
#include "lux_iniparse.h"
#include "lux_coverFace.h"
#include "lux_sleepDet.h"
#include "lux_summary.h"
#include "lux_osd.h"

// #include "sample-common.h"

#define TAG "SAMPLE-FACEDET"

LUX_IVS_FACE_DET_ATTR g_faceAttr =
{
    .ivsGrpNum = 0,
    .ivsChnNum = 5, //笑脸检测为chn3，人脸检测设为chn5
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

extern INT_X LUX_ALG_FaceDet_Init();
extern INT_X LUX_ALG_FaceDet_GetResult(LUX_FACEDET_OUTPUT_ST* result);
extern INT_X LUX_ALG_FaceDet_Exit();

int LUX_IVS_FaceDet_Init()
{
    // BOOL_X bStart = LUX_FALSE;
    BOOL_X bCoverFaceStart = LUX_FALSE;
    // BOOL_X bSleepDetStart = LUX_FALSE;
    BOOL_X sleepMode = LUX_FALSE;
    INT_X ret = LUX_ERR;

    /* 获取报警回调函数 */
    LUX_Event_GetAlarmCbk(&g_faceAttr.alarmCbk);
#ifdef CONFIG_LUXALG
    /* detection model init */
    ret = LUX_ALG_FaceDet_Init();
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "LUX_ALG_FaceDet_Init failed\n");
        return LUX_ERR;
    }
#else
    facedet_param_input_t param;
    memset(&param, 0, sizeof(facedet_param_input_t));
    // param.frameInfo.data;                /**< 帧的起始地址 */
    param.frameInfo.width = SENSOR_WIDTH_THIRD;
    param.frameInfo.height = SENSOR_HEIGHT_THIRD;
    // param.frameInfo.pixfmt;              /**< 帧的图像格式 */
    //param.frameInfo.timeStamp = 6688;          /**< 帧的时间戳 */
    param.max_face_box = 3;                 /**< 人脸检测处理过程中保留的框数量 */
    param.sense = 3;                        /**< 检测灵敏度 0~3 0:最不灵敏 3:最灵敏 default:1 */
    param.detdist = 0;                      /**< 检测距离 0~2  0:3.5米 max(img_w, img_h) >= 320 ;  1:5米  max(img_w, img_h) >= 480 2:6米 max(img_w, img_h) >= 640  default:0 */
    param.skip_num = 0;                     /**< 跳帧数目 */
    param.delay = 0;                        /**< 延时识别时间，默认为0 */
    param.rot90 = false;                    /**< 图像是否顺时针旋转90度 */
    param.switch_liveness = true;           /**< 人脸活体检测模块开关, 当true时会做活体检测, 当false时不做活体检测返回-1 */
    param.switch_face_pose = true;          /**< 人脸角度检测模块开关, 当true时会做人脸角度检测, 当false时不做人脸角度检测返回-1 */
    param.switch_face_blur = true;          /**< 人脸模糊检测模块开关, 当true时会做人脸模糊检测, 当false时不做人脸模糊检测返回-1 */
    param.switch_landmark = true;	        /* < 人脸关键检测开关 */
    do
    {
        g_faceAttr.pInterface = FaceDetInterfaceInit(&param);
        if (g_faceAttr.pInterface == NULL)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_CreateGroup(%d) failed\n", g_faceAttr.ivsGrpNum);
            break;
        }

        ret = IMP_IVS_CreateChn(g_faceAttr.ivsChnNum, g_faceAttr.pInterface);
        if (ret < 0)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_CreateChn(%d) failed\n", g_faceAttr.ivsChnNum);
            break;
        }

        ret = IMP_IVS_RegisterChn(g_faceAttr.ivsGrpNum, g_faceAttr.ivsChnNum);
        if (ret < 0)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_RegisterChn(%d, %d) failed\n", g_faceAttr.ivsGrpNum, g_faceAttr.ivsChnNum);
            break;
        }

        ret = IMP_IVS_StartRecvPic(g_faceAttr.ivsChnNum);
        if (ret < 0)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_StartRecvPic(%d) failed\n", g_faceAttr.ivsChnNum);
            break;
        }
    } while (0);
#endif
    Thread_Mutex_Create(&g_faceAttr.mutex);
    Task_CreateThread(LUX_IVS_FaceDet_Process, NULL);

    g_faceAttr.bInit = LUX_TRUE;

    LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "sleep_mode", &sleepMode);
    if (LUX_FALSE == sleepMode)
    {
        LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "cover_face_onoff", &bCoverFaceStart);
        if (bCoverFaceStart)
        {
            g_faceAttr.faceAlarmTime = getTime_getMs();
            LUX_IVS_CoverFace_Start();
        }
    }

    return 0;
}

int LUX_IVS_FaceDet_Exit()
{
    INT_X ret = LUX_ERR;
    LUX_IVS_CoverFace_Stop();
#ifdef CONFIG_LUXALG
    ret = LUX_ALG_FaceDet_Exit();
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "LUX_ALG_FaceDet_Exit failed\n");
        return LUX_ERR;
    }
#else
    do
    {
        ret = IMP_IVS_StopRecvPic(g_faceAttr.ivsChnNum);
        if (ret < 0)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_StopRecvPic(%d) failed\n", g_faceAttr.ivsChnNum);
            break;
        }
        sleep(1);

        ret = IMP_IVS_UnRegisterChn(g_faceAttr.ivsChnNum);
        if (ret < 0)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_UnRegisterChn(%d) failed\n", g_faceAttr.ivsChnNum);
            break;
        }

        ret = IMP_IVS_DestroyChn(g_faceAttr.ivsChnNum);
        if (ret < 0)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_DestroyChn(%d) failed\n", g_faceAttr.ivsChnNum);
            break;
        }

        FaceDetInterfaceExit(g_faceAttr.pInterface);
    } while (0);
#endif
    g_faceAttr.bInit = LUX_FALSE;
    return 0;
}


int LUX_IVS_CoverFaceAlarm()
{
    int ret = 0;
    BOOL_X recordOnOff = 0;
    int recordMode = 0;
    LUX_ENCODE_STREAM_ST picInfo[2];
    // LUX_SUM_Update_Event(LUX_SUM_EVENT_COVER_FACE ,gettimeStampS(),NULL);
    /*上报抓图*/
    ret = LUX_BASE_CapturePic_HD(LUX_STREAM_SUB, &picInfo[LUX_STREAM_SUB]);
    if (0 != ret)
    {
        LUX_BASE_ReleasePic_HD(LUX_STREAM_SUB, &picInfo[LUX_STREAM_SUB]);
        IMP_LOG_ERR(TAG, " LUX_BASE_CapturePic failed\n");
        return -1;
    }
    if (NULL != g_faceAttr.alarmCbk.pEvntReportFuncCB)
    {
        g_faceAttr.alarmCbk.pEvntReportFuncCB(picInfo[LUX_STREAM_SUB].pAddr, picInfo[LUX_STREAM_SUB].streamLen, LUX_NOTIFICATION_NAME_FACE);
    }

    /* 云存储 */
    if (NULL != g_faceAttr.alarmCbk.pAddCloudEvent && NULL != g_faceAttr.alarmCbk.pDeletCloudEvent
        && NULL != g_faceAttr.alarmCbk.pGetCloudEventStatus)
    {

        g_faceAttr.alarmCbk.cloudId = g_faceAttr.alarmCbk.pAddCloudEvent(picInfo[LUX_STREAM_SUB].pAddr, picInfo[LUX_STREAM_SUB].streamLen, LUX_EVENT_TYPE_FACE, LUX_CLOUD_STORAGE_TIME); //最长录300s
        printf("%s:%d cloudId:%d \n", __func__, __LINE__, g_faceAttr.alarmCbk.cloudId);
    }
    LUX_BASE_ReleasePic_HD(LUX_STREAM_SUB, &picInfo[LUX_STREAM_SUB]);

    /*事件录像*/
    ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "record", "record_onoff", &recordOnOff);
    if (0 != ret)
    {
        IMP_LOG_ERR(TAG, "ERROR:LUX_INIParse_SetKey ret:%#x", ret);
    }

    ret = LUX_INIParse_GetInt(TUYA_INI_CONFIG_FILE_NAME, "record", "record_mode", &recordMode);
    if (0 != ret)
    {
        IMP_LOG_ERR(TAG, "ERROR:LUX_INIParse_SetKey ret:%#x", ret);
    }

    if (LUX_TRUE == recordOnOff && 0 == recordMode)
    {
        ret = LUX_Event_Record(LUX_COMM_EVENT_RECORD_TIME);
        if (0 != ret)
        {
            IMP_LOG_ERR(TAG, "ERROR:LUX_INIParse_SetKey ret:%#x", ret);
        }
    }

    return ret;
}

void LUX_IVS_CoverFace_Start()
{
    g_faceAttr.bCoverFaceStart = LUX_TRUE;
}

void LUX_IVS_CoverFace_Stop()
{
    g_faceAttr.bCoverFaceStart = LUX_FALSE;
}

extern LUX_IVS_SLEEP_DET_ATTR g_SleepAttr;
int LUX_IVS_CoverFace_Event()
{
    do
    {
        if (!g_faceAttr.bCoverFaceStart)
        {
            break;
        }
        UINT_X nowTime = getTime_getMs();
        if (g_SleepAttr.face.faceStatus)
        {
            g_faceAttr.faceAlarmTime = nowTime;
            break;
        }
        /* 遮脸检测统计及上报 */
        if (nowTime - g_faceAttr.faceAlarmTime > LUX_IVS_FACE_COVER_TIME)
        {
            g_faceAttr.faceAlarmTime = nowTime;
            LUX_IVS_CoverFaceAlarm();
            printf("%s:%d timeDiff:%d,%d\n", __FUNCTION__, __LINE__, g_faceAttr.faceAlarmTime, nowTime - g_faceAttr.faceAlarmTime);
        }
    } while (0);


    return 0;
}

int LUX_IVS_CoverFace_SSDEvent(LUX_FACEDET_OUTPUT_ST *result)
{
    BOOL_X b_coverFace = LUX_FALSE;
    INT_X i;
    do
    {
        if (!g_faceAttr.bCoverFaceStart)
        {
            break;
        }
        UINT_X nowTime = getTime_getMs();
        if (result->count > 0)
        {
            for (i = 0; i < result->count; i++)
            {
                if (result->face[i].classid == 2)
                {
                    b_coverFace = LUX_TRUE;
                }
            }

            if (b_coverFace)
            {
                /* 遮脸检测统计及上报 */
                if (nowTime - g_faceAttr.faceAlarmTime > LUX_IVS_FACE_COVER_TIME)
                {
                    g_faceAttr.faceAlarmTime = nowTime;
                    LUX_IVS_CoverFaceAlarm();
                    printf("%s:%d timeDiff:%d,%d\n", __FUNCTION__, __LINE__, g_faceAttr.faceAlarmTime, nowTime - g_faceAttr.faceAlarmTime);
                }
                break;
            }
            g_faceAttr.faceAlarmTime = nowTime;
        }
        else
        {
            /* 遮脸检测统计及上报 */
            if (nowTime - g_faceAttr.faceAlarmTime > LUX_IVS_FACE_COVER_TIME)
            {
                g_faceAttr.faceAlarmTime = nowTime;
                LUX_IVS_CoverFaceAlarm();
                printf("%s:%d timeDiff:%d,%d\n", __FUNCTION__, __LINE__, g_faceAttr.faceAlarmTime, nowTime - g_faceAttr.faceAlarmTime);
            }
        }
    } while (0);

    return 0;
}

void LUX_IVS_FaceDet_Process()
{
    INT_X ret = LUX_ERR;
    INT_X missFaceCnt = 0;
    BOOL_X SleepDet_Onoff = LUX_FALSE;
    facedet_param_output_t* resultPtr = NULL;
    LUX_FACEDET_OUTPUT_ST resultSSD;
#ifdef CONFIG_LUXALG
#if 1
    BOOL_X last_coverface_det_onoff = LUX_TRUE;
    BOOL_X last_sleep_det_onoff = LUX_TRUE;
    ThrdSchd thrdschd;
#endif
#endif
    while (1)
    {
        if (!g_faceAttr.bInit)
        {
            usleep(100 * 1000);
            continue;
        }

        Thread_Mutex_Lock(&g_faceAttr.mutex);
        do
        {
            /* 读取配置 */
            ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "sleep_det_onoff", &SleepDet_Onoff);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "LUX_INIParse_GetBool alarm_human_filter failed!, error num [0x%x] ", ret);
            }
#ifndef CONFIG_LUXALG
            ret = IMP_IVS_PollingResult(g_faceAttr.ivsChnNum, TIME_OUT);
            if (ret != LUX_OK)
            {
                IMP_LOG_ERR(TAG, "IMP_IVS_PollingResult(%d, %d) failed\n", 0, IMP_IVS_DEFAULT_TIMEOUTMS);
                break;
            }
            ret = IMP_IVS_GetResult(g_faceAttr.ivsChnNum, (void**)&resultPtr);
            if (ret < 0)
            {
                IMP_LOG_ERR(TAG, "IMP_IVS_GetResult(%d) failed\n", 0);
                break;
            }
            //睡眠检测
            LUX_IVS_SleepDet_Face_Event(resultPtr);
            //遮脸检测处理，改变遮脸报警逻辑，考虑算法误报的情况
            LUX_IVS_CoverFace_Event();
        #if 1
            /* 人脸矩形框显示 */
            if (g_faceAttr.bCoverFaceStart || SleepDet_Onoff)
            {
                if (resultPtr)
                {
                    if (resultPtr->count > 0)
                    {
                        LUX_OSD_FaceRect_Miss();
                        LUX_OSD_FaceRect_Show(resultPtr);
                        missFaceCnt = 0;
                    }
                    else if (missFaceCnt >= MISS_NUM)
                    {
                        LUX_OSD_FaceRect_Miss();
                    }
                    else
                    {
                        missFaceCnt++;
                    }
                }
            }
            else
            {
                LUX_OSD_FaceRect_Miss();
            }
        #endif
            ret = IMP_IVS_ReleaseResult(g_faceAttr.ivsChnNum, (void*)resultPtr);
            if (ret != LUX_OK)
            {
                IMP_LOG_ERR(TAG, "IMP_IVS_ReleaseResult(%d) failed\n", 0);
            }
#else
        #if 1/* 线程调度策略更改 */
            if((last_coverface_det_onoff != g_faceAttr.bCoverFaceStart) || (last_sleep_det_onoff != SleepDet_Onoff))
            {
                printf("\n\n######################### LUX_IVS_FaceDet_Process Policy_Prio Change #########################\n");
                if((g_faceAttr.bCoverFaceStart || SleepDet_Onoff))
                {
                    SetThread_RR_Prio(MAX_RR_PRIORITY);
                }
                else if( (!g_faceAttr.bCoverFaceStart && !SleepDet_Onoff))
                {
                    thrdschd.policy = SCHED_OTHER;
                    thrdschd.priority = 0;
                    SetThread_Policy_Prio(thrdschd);
                }
                last_coverface_det_onoff = g_faceAttr.bCoverFaceStart;
                last_sleep_det_onoff = SleepDet_Onoff;
                GetThread_Policy_Prio(&thrdschd);
            }
        #endif
            memset(&resultSSD, 0, sizeof(LUX_FACEDET_OUTPUT_ST));
            LUX_ALG_FaceDet_GetResult(&resultSSD);
            //遮脸检测处理，改变遮脸报警逻辑，考虑算法误报的情况
            LUX_IVS_CoverFace_SSDEvent(&resultSSD);
            //睡眠检测
            LUX_IVS_SleepDet_SSDFace_Event(&resultSSD);
        #if 1
            /* 人脸矩形框显示 */
            if (g_faceAttr.bCoverFaceStart || SleepDet_Onoff)
            {
                if (resultSSD.count > 0)
                {
                    LUX_OSD_FaceRect_Miss();
                    LUX_OSD_FaceRect_SSDShow(&resultSSD);
                    missFaceCnt = 0;
                }
                else if (missFaceCnt >= MISS_NUM)
                {
                    LUX_OSD_FaceRect_Miss();
                }
                else
                {
                    missFaceCnt++;
                }
            }
            else
            {
                LUX_OSD_FaceRect_Miss();
            }
        #endif
#endif
        } while (0);
        Thread_Mutex_UnLock(&g_faceAttr.mutex);
        usleep(30 * 1000);
    }
}
