#if 0
#include "lux_pukeAlg.h"

#define TAG "lux_pukeAlg"

using namespace std;
using namespace jzdl;

BaseNet* pukeDetModel;
static PUINT8_X pOutFrameData;
static PUINT8_X pRgbPuke, pDstPuke;
static UCHAR_X pk_print_cnt = 0;

LUX_ALG_PUKE_DET_ATTR g_pukeAttr =
{
    .bInit = LUX_FALSE,
    .bStart = LUX_FALSE,
    .alarmTime = 0,
    .pukeAlarmTime = 0,
};

VOID_X LUX_ALG_PukeDet_Start()
{
    g_pukeAttr.bStart = LUX_TRUE;
}

VOID_X LUX_ALG_PukeDet_Stop()
{
    g_pukeAttr.bStart = LUX_FALSE;
}

INT_X LUX_ALG_PukeDet_Alarm()
{
    INT_X ret = 0;
    BOOL_X recordOnOff = LUX_FALSE;
    INT_X recordMode = 0;
    LUX_ENCODE_STREAM_ST picInfo[2];

    /*报警时间间隔*/
    if (LUX_Event_IsAlarmByInterval(g_pukeAttr.alarmTime))
    {
        g_pukeAttr.alarmTime = getTime_getS();
        ret = LUX_BASE_CapturePic_HD(LUX_STREAM_SUB, &picInfo[LUX_STREAM_SUB]);
        //ret = LUX_BASE_CapturePic(LUX_STREAM_JPEG, &picInfo);
        if (0 != ret)
        {
            LUX_BASE_ReleasePic_HD(LUX_STREAM_SUB, &picInfo[LUX_STREAM_SUB]);
            IMP_LOG_ERR(TAG, " LUX_BASE_CapturePic failed\n");
            return LUX_ERR;
        }
        /*上报抓图*/
        if (NULL != g_pukeAttr.alarmCbk.pEvntReportFuncCB)
        {
            g_pukeAttr.alarmCbk.pEvntReportFuncCB((PCHAR_X)picInfo[LUX_STREAM_SUB].pAddr, (UINT_X)picInfo[LUX_STREAM_SUB].streamLen, LUX_NOTIFICATION_NAME_DOORBELL);
        }

        /* 云存储 */
        if (NULL != g_pukeAttr.alarmCbk.pAddCloudEvent && NULL != g_pukeAttr.alarmCbk.pDeletCloudEvent &&
            NULL != g_pukeAttr.alarmCbk.pGetCloudEventStatus)
        {
            g_pukeAttr.alarmCbk.cloudId = g_pukeAttr.alarmCbk.pAddCloudEvent((PCHAR_X)picInfo[LUX_STREAM_SUB].pAddr, (UINT_X)picInfo[LUX_STREAM_SUB].streamLen, LUX_EVENT_TYPE_DOOR_BELL, (UINT_X)LUX_CLOUD_STORAGE_TIME); //最长录300s
            printf("%s:%d cloudId:%d \n", __func__, __LINE__, g_pukeAttr.alarmCbk.cloudId);
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

INT_X LUX_ALG_PukeDet_Init()
{
    BOOL_X bStart = LUX_FALSE;
    BOOL_X sleepMode = LUX_FALSE;
    /* 获取报警回调函数 */
    LUX_Event_GetAlarmCbk(&g_pukeAttr.alarmCbk);

    /* Step.1 detection model init */
    pukeDetModel = net_create();
    printf("BaseNet *pukeDetModel=net_create();\n");

    INT_X ret = ssd_pukeDet_init(pukeDetModel);
    printf("ret = ssd_pukeDet_init(pukeDetModel);\n");
    if (ret < 0) {
        printf("ssd_pukeDet_init(pukeDetModel) failed\n");
    }

    UINT_X size = SENSOR_WIDTH_SECOND * SENSOR_HEIGHT_SECOND * 3;
    pOutFrameData = new UINT8_X[size / 2];
    pRgbPuke = new UINT8_X[size];
    pDstPuke = new UINT8_X[size];

    thread t_puke(puke_det_process_thread);
    t_puke.detach();

    g_pukeAttr.bInit = LUX_TRUE;
    /* 隐私模式下不启用算法 */
    LUX_INIParse_GetBool((PCHAR_X)TUYA_INI_CONFIG_FILE_NAME, (PCHAR_X)"function", (PCHAR_X)"sleep_mode", &sleepMode);
    if (LUX_FALSE == sleepMode)
    {
        /* 根据配置文件是否启动算法 */
        LUX_INIParse_GetBool((PCHAR_X)TUYA_INI_CONFIG_FILE_NAME, (PCHAR_X)"function", (PCHAR_X)"puke_alarm_onoff", &bStart);
        if (bStart)
        {
            LUX_ALG_PukeDet_Start();
        }
    }

    return ret;
}

INT_X LUX_ALG_PukeDet_GetResult(LUX_FACEDET_OUTPUT_ST* puke_result)
{
    if (puke_result == nullptr)
    {
        IMP_LOG_ERR(TAG, "LUX_ALG_PukeDet_GetResult failed, empty pointer\n");
        return LUX_PARM_NULL_PTR;
    }

    INT_X ret = LUX_ERR;
    IMPFrameInfo frameData;
    UINT_X i;
    UINT_X rgb_size = SENSOR_WIDTH_SECOND * SENSOR_HEIGHT_SECOND * 3;
    UINT_X yuv_size = rgb_size / 2;
    vector<pk_objbox> puke_list;

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
    memset(pRgbPuke, 0, rgb_size);
    ret = LUX_NV12_C_RGB24(SENSOR_WIDTH_SECOND, SENSOR_HEIGHT_SECOND, pOutFrameData, pRgbPuke);
    if (LUX_OK != ret) {
        IMP_LOG_ERR(TAG, "LUX_NV12_C_RGB24 failed, error number [x0%x]\n", ret);
        return LUX_ERR;
    }

    memset(pDstPuke, 0, rgb_size);
    ret = LUX_RGB24_NHWC_ARRAY(SENSOR_WIDTH_SECOND, SENSOR_HEIGHT_SECOND, pRgbPuke, pDstPuke, 3);
    if (LUX_OK != ret) {
        IMP_LOG_ERR(TAG, "LUX_RGB24_NHWC_ARRAY failed, error number [x0%x]\n", ret);
        return LUX_ERR;
    }

    /* Step.3 Get result */
    puke_list.clear();
    ret = ssd_pukeDet_start(pukeDetModel, puke_list, pDstPuke, SENSOR_WIDTH_SECOND, SENSOR_HEIGHT_SECOND);
    if (ret < 0) {
        printf("%d sample pukedet start error\n", 1);
        return LUX_ERR;
    }

    memset(puke_result, 0, sizeof(LUX_FACEDET_OUTPUT_ST));
    for (i = 0; i < puke_list.size(); i++) {
        if (1 == puke_list[i].clsid && puke_list[i].score > LUX_PUKE_CONFIDENCE) {
            puke_result->face[puke_result->count].x0 = puke_list[i].x0;
            puke_result->face[puke_result->count].y0 = puke_list[i].y0;
            puke_result->face[puke_result->count].x1 = puke_list[i].x1;
            puke_result->face[puke_result->count].y1 = puke_list[i].y1;
            puke_result->count++;
        }
    }

    return LUX_OK;
}

VOID_X puke_det_process_thread()
{
    CHAR_X threadName[64] = { 0 };
    CHAR_X ivsChnName[32] = { 0 };
    INT_X ret = LUX_ERR;
    UINT_X nowtime = 0;
    LUX_FACEDET_OUTPUT_ST outResult;

    strcpy(ivsChnName, "PUKE_DETECTION");
    sprintf(threadName, "ALG_%s", ivsChnName);
    prctl(PR_SET_NAME, threadName);

    while (1)
    {
        if (!g_pukeAttr.bInit || !g_pukeAttr.bStart)
        {
            usleep(400 * 1000);
            continue;
        }
        do
        {
            /*获取宝宝吐奶侦测结果*/
            nowtime = getTime_getMs();
            ret = LUX_ALG_PukeDet_GetResult(&outResult);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "LUX_ALG_PukeDet_GetResult failed.\n");
                break;
            }
            if (outResult.count > 0)
            {
                LUX_ALG_PukeDet_Alarm();
            }
#if 1
            INT_X i;
            if (pk_print_cnt > 2)
            {
                printf("pukeDet prossess time: [%u]ms\n", getTime_getMs() - nowtime);
                for (i = 0; i < outResult.count; i++)
                {
                    printf("in sample  ");
                    printf("index:%d \n", i);
                    printf("minx:%d \n", outResult.face[i].x0);
                    printf("miny:%d \n", outResult.face[i].y0);
                    printf("maxx:%d \n", outResult.face[i].x1);
                    printf("maxy:%d \n", outResult.face[i].y1);
                }
                pk_print_cnt=0;
            }
            pk_print_cnt++;
            if (g_pukeAttr.bStart)
            {
                if (outResult.count == 0)
                {
                    LUX_OSD_PukeRect_Miss();
                }
                else if (outResult.count > 0)
                {
                    LUX_OSD_PukeRect_Miss();
                    LUX_OSD_PukeRect_SSDShow(&outResult);
                }
            }
            else
            {
                LUX_OSD_PukeRect_Miss();
            }
#endif
            usleep(30 * 1000);
        } while (0);
    }
}

INT_X LUX_ALG_PukeDet_Exit()
{
    delete pRgbPuke;
    delete pDstPuke;
    delete pOutFrameData;
    return ssd_pukeDet_exit(pukeDetModel);
}
#endif

