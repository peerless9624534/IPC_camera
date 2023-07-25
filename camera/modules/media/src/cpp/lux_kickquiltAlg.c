#if 0
#include "lux_kickquiltAlg.h"

#define TAG "lux_kickquiltAlg"

using namespace std;
using namespace jzdl;

BaseNet* kickquiltDetModel;
static PUINT8_X pOutFrameData;
static PUINT8_X pRgbKickQuilt, pDstKickQuilt;
static UCHAR_X kq_print_cnt = 0;

LUX_ALG_KICKQUILT_DET_ATTR g_kickquiltAttr =
{
    .bInit = LUX_FALSE,
    .bStart = LUX_FALSE,
    .alarmTime = 0,
    .kickquiltAlarmTime = 0,
};

VOID_X LUX_ALG_KickQuiltDet_Start()
{
    g_kickquiltAttr.bStart = LUX_TRUE;
}

VOID_X LUX_ALG_KickQuiltDet_Stop()
{
    g_kickquiltAttr.bStart = LUX_FALSE;
}

INT_X LUX_ALG_KickQuiltDet_Alarm()
{
    INT_X ret = 0;
    BOOL_X recordOnOff = LUX_FALSE;
    INT_X recordMode = 0;
    LUX_ENCODE_STREAM_ST picInfo[2];

    /*报警时间间隔*/
    if (LUX_Event_IsAlarmByInterval(g_kickquiltAttr.alarmTime))
    {
        g_kickquiltAttr.alarmTime = getTime_getS();
        ret = LUX_BASE_CapturePic_HD(LUX_STREAM_SUB, &picInfo[LUX_STREAM_SUB]);
        //ret = LUX_BASE_CapturePic(LUX_STREAM_JPEG, &picInfo);
        if (0 != ret)
        {
            LUX_BASE_ReleasePic_HD(LUX_STREAM_SUB, &picInfo[LUX_STREAM_SUB]);
            IMP_LOG_ERR(TAG, " LUX_BASE_CapturePic failed\n");
            return LUX_ERR;
        }
        /*上报抓图*/
        if (NULL != g_kickquiltAttr.alarmCbk.pEvntReportFuncCB)
        {
            g_kickquiltAttr.alarmCbk.pEvntReportFuncCB((PCHAR_X)picInfo[LUX_STREAM_SUB].pAddr, (UINT_X)picInfo[LUX_STREAM_SUB].streamLen, LUX_NOTIFICATION_NAME_PASSBY);
        }

        /* 云存储 */
        if (NULL != g_kickquiltAttr.alarmCbk.pAddCloudEvent && NULL != g_kickquiltAttr.alarmCbk.pDeletCloudEvent &&
            NULL != g_kickquiltAttr.alarmCbk.pGetCloudEventStatus)
        {
            g_kickquiltAttr.alarmCbk.cloudId = g_kickquiltAttr.alarmCbk.pAddCloudEvent((PCHAR_X)picInfo[LUX_STREAM_SUB].pAddr, (UINT_X)picInfo[LUX_STREAM_SUB].streamLen, LUX_EVENT_TYPE_PASSBY, (UINT_X)LUX_CLOUD_STORAGE_TIME); //最长录300s
            printf("%s:%d cloudId:%d \n", __func__, __LINE__, g_kickquiltAttr.alarmCbk.cloudId);
        }
        LUX_BASE_ReleasePic_HD(LUX_STREAM_SUB, &picInfo[LUX_STREAM_SUB]);

        /*事件录像*/
        ret = LUX_INIParse_GetBool((PCHAR_X)TUYA_INI_CONFIG_FILE_NAME, (PCHAR_X)"record", (PCHAR_X)"record_onoff", &recordOnOff);
        if (0 != ret)
        {
            IMP_LOG_ERR(TAG, "ERROR:LUX_INIParse_SetKey ret:%#x", ret);
        }

        ret = LUX_INIParse_GetInt((PCHAR_X)TUYA_INI_CONFIG_FILE_NAME, (PCHAR_X)"record", (PCHAR_X)"record_mode", &recordMode);
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
    }

    return ret;
}

INT_X LUX_ALG_KickQuiltDet_Init()
{
    BOOL_X bStart = LUX_FALSE;
    BOOL_X sleepMode = LUX_FALSE;
    /* 获取报警回调函数 */
    LUX_Event_GetAlarmCbk(&g_kickquiltAttr.alarmCbk);

    /* Step.1 detection model init */
    kickquiltDetModel = net_create();
    printf("BaseNet *kickquiltDetModel=net_create();\n");

    INT_X ret = ssd_kickquiltDet_init(kickquiltDetModel);
    printf("ret = ssd_kickquiltDet_init(kickquiltDetModel);\n");
    if (ret < 0) {
        printf("ssd_kickquiltDet_init(kickquiltDetModel) failed\n");
    }

    UINT_X size = SENSOR_WIDTH_SECOND * SENSOR_HEIGHT_SECOND * 3;
    pOutFrameData = new UINT8_X[size / 2];
    pRgbKickQuilt = new UINT8_X[size];
    pDstKickQuilt = new UINT8_X[size];

    thread t_kickquilt(kickquilt_det_process_thread);
    t_kickquilt.detach();

    g_kickquiltAttr.bInit = LUX_TRUE;
    /* 隐私模式下不启用算法 */
    LUX_INIParse_GetBool((PCHAR_X)TUYA_INI_CONFIG_FILE_NAME, (PCHAR_X)"function", (PCHAR_X)"sleep_mode", &sleepMode);
    if (LUX_FALSE == sleepMode)
    {
        /* 根据配置文件是否启动算法 */
        LUX_INIParse_GetBool((PCHAR_X)TUYA_INI_CONFIG_FILE_NAME, (PCHAR_X)"function", (PCHAR_X)"kickquilt_onoff", &bStart);
        if (bStart)
        {
            LUX_ALG_KickQuiltDet_Start();
        }
    }

    return ret;
}

INT_X LUX_ALG_KickQuiltDet_GetResult(LUX_PERSONDET_OUTPUT_ST* kickquilt_result)
{
    if (kickquilt_result == nullptr)
    {
        IMP_LOG_ERR(TAG, "LUX_ALG_KickQuiltDet_GetResult failed, empty pointer\n");
        return LUX_PARM_NULL_PTR;
    }

    INT_X ret = LUX_ERR;
    IMPFrameInfo frameData;
    UINT_X i;
    UINT_X rgb_size = SENSOR_WIDTH_SECOND * SENSOR_HEIGHT_SECOND * 3;
    UINT_X yuv_size = rgb_size / 2;
    vector<kq_objbox> kickquilt_list;

    /*Step.1 Snap pic*/
    memset(&frameData, 0, sizeof(IMPFrameInfo));
    memset(pOutFrameData, 0, yuv_size);

    Thread_Mutex_Lock(&g_framSrcAttr.fsChn[LUX_STREAM_SUB].mux);
    ret = IMP_FrameSource_SnapFrame(LUX_STREAM_SUB,
                                    PIX_FMT_NV12,
                                    SENSOR_WIDTH_SECOND,
                                    SENSOR_HEIGHT_SECOND,
                                    pOutFrameData,
                                    &frameData);
    Thread_Mutex_UnLock(&g_framSrcAttr.fsChn[LUX_STREAM_SUB].mux);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "%s(%d):IMP_FrameSource_SnapFrame(chn%d) faild\n", __func__, __LINE__, LUX_STREAM_SUB);
        return LUX_ERR;
    }
    // printf("frameData.size=%d, frameData.width=%d, frameData.height=%d\n\n\n", frameData.size, frameData.width, frameData.height);


    /*Step.2 convert array*/
    memset(pRgbKickQuilt, 0, rgb_size);
    ret = LUX_NV12_C_RGB24(SENSOR_WIDTH_SECOND, SENSOR_HEIGHT_SECOND, pOutFrameData, pRgbKickQuilt);
    if (LUX_OK != ret) {
        IMP_LOG_ERR(TAG, "LUX_NV12_C_RGB24 failed, error number [x0%x]\n", ret);
        return LUX_ERR;
    }

    memset(pDstKickQuilt, 0, rgb_size);
    ret = LUX_RGB24_NHWC_ARRAY(SENSOR_WIDTH_SECOND, SENSOR_HEIGHT_SECOND, pRgbKickQuilt, pDstKickQuilt, 3);
    if (LUX_OK != ret) {
        IMP_LOG_ERR(TAG, "LUX_RGB24_NHWC_ARRAY failed, error number [x0%x]\n", ret);
        return LUX_ERR;
    }

    /* Step.3 Get result */
    kickquilt_list.clear();
    ret = ssd_kickquiltDet_start(kickquiltDetModel, kickquilt_list, pDstKickQuilt, SENSOR_WIDTH_SECOND, SENSOR_HEIGHT_SECOND);
    if (ret < 0) {
        printf("%d sample kickquiltdet start error\n", 1);
        return LUX_ERR;
    }

    memset(kickquilt_result, 0, sizeof(LUX_PERSONDET_OUTPUT_ST));
    for (i = 0; i < kickquilt_list.size(); i++) {
        if (1 == kickquilt_list[i].clsid && kickquilt_list[i].score > LUX_KICKQUILT_CONFIDENCE) {
            kickquilt_result->person[kickquilt_result->count].x0 = kickquilt_list[i].x0;
            kickquilt_result->person[kickquilt_result->count].y0 = kickquilt_list[i].y0;
            kickquilt_result->person[kickquilt_result->count].x1 = kickquilt_list[i].x1;
            kickquilt_result->person[kickquilt_result->count].y1 = kickquilt_list[i].y1;
            kickquilt_result->count++;
        }
    }

    return LUX_OK;
}

VOID_X kickquilt_det_process_thread()
{
    CHAR_X threadName[64] = { 0 };
    CHAR_X ivsChnName[32] = { 0 };
    INT_X ret = LUX_ERR;
    UCHAR_X kq_cnt = 0;
    UINT_X nowtime = 0;
    LUX_PERSONDET_OUTPUT_ST outResult;

    strcpy(ivsChnName, "KICKQUILT_DETECTION");
    sprintf(threadName, "ALG_%s", ivsChnName);
    prctl(PR_SET_NAME, threadName);

    while (1)
    {
        if (!g_kickquiltAttr.bInit || !g_kickquiltAttr.bStart)
        {
            usleep(400 * 1000);
            continue;
        }
        do
        {
            /*获取宝宝踢被侦测结果*/
            nowtime = getTime_getMs();
            ret = LUX_ALG_KickQuiltDet_GetResult(&outResult);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "LUX_ALG_KickQuiltDet_GetResult failed.\n");
                break;
            }
            if (outResult.count > 0)
            {
                kq_cnt++;
            }
            if (kq_cnt > 4)
            {
                LUX_ALG_KickQuiltDet_Alarm();
                kq_cnt=0;
            }
#if 1
            INT_X i;
            if (kq_print_cnt > 2)
            {
                printf("kickquiltDet prossess time: [%u]ms\n", getTime_getMs() - nowtime);
                for (i = 0; i < outResult.count; i++)
                {
                    printf("in sample  ");
                    printf("index:%d \n", i);
                    printf("minx:%d \n", outResult.person[i].x0);
                    printf("miny:%d \n", outResult.person[i].y0);
                    printf("maxx:%d \n", outResult.person[i].x1);
                    printf("maxy:%d \n", outResult.person[i].y1);
                }
                kq_print_cnt = 0;
            }
            kq_print_cnt++;
            if (g_kickquiltAttr.bStart)
            {
                if (outResult.count == 0)
                {
                    LUX_OSD_KickQuiltRect_Miss();
                }
                else if (outResult.count > 0)
                {
                    LUX_OSD_KickQuiltRect_Miss();
                    LUX_OSD_KickQuiltRect_SSDShow(&outResult);
                }
            }
            else
            {
                LUX_OSD_KickQuiltRect_Miss();
            }
#endif
            usleep(30 * 1000);
        } while (0);
    }
}

INT_X LUX_ALG_KickQuiltDet_Exit()
{
    delete pRgbKickQuilt;
    delete pDstKickQuilt;
    delete pOutFrameData;
    return ssd_kickquiltDet_exit(kickquiltDetModel);
}
#endif