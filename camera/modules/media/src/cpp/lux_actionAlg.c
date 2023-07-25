#if 0
#include "lux_actionAlg.h"

#define TAG "lux_actionAlg"

using namespace std;
using namespace jzdl;

BaseNet* actionDetModel;
static PUINT8_X pOutFrameData;
static PUINT8_X pRgbAction, pDstAction;
static UCHAR_X act_print_cnt = 0;
#if 0
static vector <int> baby_nums; //宝宝编号
static vector <int> act_tag[LUX_ACTION_NUM_MAX]; //宝宝标签
#endif

LUX_ALG_ACTION_DET_ATTR g_actionAttr =
{
    .bInit = LUX_FALSE,
    .bStart = LUX_FALSE,
    .alarmTime = 0,
    .actionAlarmTime = 0,
};

VOID_X LUX_ALG_ActionDet_Start()
{
    g_actionAttr.bStart = LUX_TRUE;
}

VOID_X LUX_ALG_ActionDet_Stop()
{
    g_actionAttr.bStart = LUX_FALSE;
}

INT_X LUX_ALG_ActionDet_Alarm()
{
    INT_X ret = 0;
    BOOL_X recordOnOff = LUX_FALSE;
    INT_X recordMode = 0;
    LUX_ENCODE_STREAM_ST picInfo[2];

    /*报警时间间隔*/
    if (LUX_Event_IsAlarmByInterval(g_actionAttr.alarmTime))
    {
        g_actionAttr.alarmTime = getTime_getS();
        ret = LUX_BASE_CapturePic_HD(LUX_STREAM_SUB, &picInfo[LUX_STREAM_SUB]);
        //ret = LUX_BASE_CapturePic(LUX_STREAM_JPEG, &picInfo);
        if (0 != ret)
        {
            LUX_BASE_ReleasePic_HD(LUX_STREAM_SUB, &picInfo[LUX_STREAM_SUB]);
            IMP_LOG_ERR(TAG, " LUX_BASE_CapturePic failed\n");
            return LUX_ERR;
        }
        /*上报抓图*/
        if (NULL != g_actionAttr.alarmCbk.pEvntReportFuncCB)
        {
            g_actionAttr.alarmCbk.pEvntReportFuncCB((PCHAR_X)picInfo[LUX_STREAM_SUB].pAddr, (UINT_X)picInfo[LUX_STREAM_SUB].streamLen, LUX_NOTIFICATION_NAME_PASSBY);
        }

        /* 云存储 */
        if (NULL != g_actionAttr.alarmCbk.pAddCloudEvent && NULL != g_actionAttr.alarmCbk.pDeletCloudEvent &&
            NULL != g_actionAttr.alarmCbk.pGetCloudEventStatus)
        {
            g_actionAttr.alarmCbk.cloudId = g_actionAttr.alarmCbk.pAddCloudEvent((PCHAR_X)picInfo[LUX_STREAM_SUB].pAddr, (UINT_X)picInfo[LUX_STREAM_SUB].streamLen, LUX_EVENT_TYPE_PASSBY, (UINT_X)LUX_CLOUD_STORAGE_TIME); //最长录300s
            printf("%s:%d cloudId:%d \n", __func__, __LINE__, g_actionAttr.alarmCbk.cloudId);
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

INT_X LUX_ALG_ActionDet_Init()
{
    BOOL_X bStart = LUX_FALSE;
    BOOL_X sleepMode = LUX_FALSE;
    /* 获取报警回调函数 */
    LUX_Event_GetAlarmCbk(&g_actionAttr.alarmCbk);

    /* Step.1 detection model init */
    actionDetModel = net_create();
    printf("BaseNet *actionDetModel=net_create();\n");

    INT_X ret = ssd_actionDet_init(actionDetModel);
    printf("ret = ssd_actionDet_init(actionDetModel);\n");
    if (ret < 0) {
        printf("ssd_actionDet_init(actionDetModel) failed\n");
    }

    UINT_X size = SENSOR_WIDTH_SECOND * SENSOR_HEIGHT_SECOND * 3;
    pOutFrameData = new UINT8_X[size / 2];
    pRgbAction = new UINT8_X[size];
    pDstAction = new UINT8_X[size];

    thread t_action(action_det_process_thread);
    t_action.detach();

    g_actionAttr.bInit = LUX_TRUE;
    /* 隐私模式下不启用算法 */
    LUX_INIParse_GetBool((PCHAR_X)TUYA_INI_CONFIG_FILE_NAME, (PCHAR_X)"function", (PCHAR_X)"sleep_mode", &sleepMode);
    if (LUX_FALSE == sleepMode)
    {
        /* 根据配置文件是否启动算法 */
        LUX_INIParse_GetBool((PCHAR_X)TUYA_INI_CONFIG_FILE_NAME, (PCHAR_X)"function", (PCHAR_X)"action_onoff", &bStart);
        if (bStart)
        {
            LUX_ALG_ActionDet_Start();
        }
    }

    return ret;
}

INT_X LUX_ALG_ActionDet_GetResult(LUX_PERSONDET_OUTPUT_ST* action_result)
{
    if (action_result == nullptr)
    {
        IMP_LOG_ERR(TAG, "LUX_ALG_ActionDet_GetResult failed, empty pointer\n");
        return LUX_PARM_NULL_PTR;
    }

    INT_X ret = LUX_ERR;
    IMPFrameInfo frameData;
    INT_X i;// j;
    UINT_X rgb_size = SENSOR_WIDTH_SECOND * SENSOR_HEIGHT_SECOND * 3;
    UINT_X yuv_size = rgb_size / 2;
    vector<act_objbox> action_list;

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
    memset(pRgbAction, 0, rgb_size);
    ret = LUX_NV12_C_RGB24(SENSOR_WIDTH_SECOND, SENSOR_HEIGHT_SECOND, pOutFrameData, pRgbAction);
    if (LUX_OK != ret) {
        IMP_LOG_ERR(TAG, "LUX_NV12_C_RGB24 failed, error number [x0%x]\n", ret);
        return LUX_ERR;
    }

    memset(pDstAction, 0, rgb_size);
    ret = LUX_RGB24_NHWC_ARRAY(SENSOR_WIDTH_SECOND, SENSOR_HEIGHT_SECOND, pRgbAction, pDstAction, 3);
    if (LUX_OK != ret) {
        IMP_LOG_ERR(TAG, "LUX_RGB24_NHWC_ARRAY failed, error number [x0%x]\n", ret);
        return LUX_ERR;
    }

    /* Step.3 Get result */
    action_list.clear();
    ret = ssd_actionDet_start(actionDetModel, action_list, pDstAction, SENSOR_WIDTH_SECOND, SENSOR_HEIGHT_SECOND);
    if (ret < 0) {
        printf("%d sample actiondet start error\n", 1);
        return LUX_ERR;
    }

    memset(action_result, 0, sizeof(LUX_PERSONDET_OUTPUT_ST));
    int act_personcnt = 0;
    if (action_list.size() <= LUX_ACTION_NUM_MAX)
    {
        act_personcnt = action_list.size();
    }
    else
    {
        act_personcnt = LUX_ACTION_NUM_MAX;
    }

    for (i = 0; i < act_personcnt; i++) {
        printf("+++ 0: action_list[%d].clsid = %d, action_list[%d].score = %.2f +++\n", i, action_list[i].clsid, i, action_list[i].score);
        if ((LUX_ACITON_NONE < action_list[i].clsid && action_list[i].clsid < LUX_ACTION_BUTTON))//&& 
            //action_list[i].score > LUX_ACTION_CONFIDENCE)
        {
            printf("+++ 1: action_list[%d].clsid = %d, action_list[%d].score = %.2f +++\n", i, action_list[i].clsid, i, action_list[i].score);
            action_result->person[i].x0 = action_list[i].x0;
            action_result->person[i].y0 = action_list[i].y0;
            action_result->person[i].x1 = action_list[i].x1;
            action_result->person[i].y1 = action_list[i].y1;
            action_result->count++;
            // if (act_tag[i].empty() || action_list[i].clsid == act_tag[i].at(act_tag[i].size()-1))
#if 0
            if (act_tag[i].empty() || action_list[i].clsid == *(act_tag[i].rbegin()))
            {
                act_tag[i].push_back(action_list[i].clsid);
            }
            else
            {
                act_tag[i].clear();
            }

            if (act_tag[i].size() >= 2)
            {
                baby_nums.push_back(i);
                act_tag[i].clear();
            }
#endif
        }
#if 0
        else if (!baby_nums.empty())
        {
            baby_nums.erase(std::remove(baby_nums.begin(), baby_nums.end(), i), baby_nums.end());
        }
#endif
    }

#if 0
    for (j = 0; j < baby_nums.size(); j++)
    {
        printf("+++ 2: baby_nums[%d] = %d +++\n", j, baby_nums[j]);
        action_result->person[j].x0 = action_list[baby_nums[j]].x0;
        action_result->person[j].y0 = action_list[baby_nums[j]].y0;
        action_result->person[j].x1 = action_list[baby_nums[j]].x1;
        action_result->person[j].y1 = action_list[baby_nums[j]].y1;
        action_result->count++;
    }
#endif

    return LUX_OK;
}

VOID_X action_det_process_thread()
{
    CHAR_X threadName[64] = { 0 };
    CHAR_X ivsChnName[32] = { 0 };
    INT_X ret = LUX_ERR;
    UCHAR_X act_cnt = 0;
    UINT_X nowtime = 0;
    LUX_PERSONDET_OUTPUT_ST outResult;

    strcpy(ivsChnName, "ACTION_DETECTION");
    sprintf(threadName, "ALG_%s", ivsChnName);
    prctl(PR_SET_NAME, threadName);

    while (1)
    {
        if (!g_actionAttr.bInit || !g_actionAttr.bStart)
        {
            usleep(400 * 1000);
            continue;
        }
        do
        {
            /*获取宝宝动作抓拍结果*/
            nowtime = getTime_getMs();
            ret = LUX_ALG_ActionDet_GetResult(&outResult);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "LUX_ALG_ActionDet_GetResult failed.\n");
                break;
            }
#if 0
            INT_X i;
            // if (act_print_cnt > 2)
            // {
                printf("actionDet prossess time: [%u]ms action.count=%d\n", getTime_getMs() - nowtime, outResult.count);
                for (i = 0; i < outResult.count; i++)
                {
                    printf("in sample  ");
                    printf("index:%d \n", i);
                    printf("minx:%d \n", outResult.person[i].x0);
                    printf("miny:%d \n", outResult.person[i].y0);
                    printf("maxx:%d \n", outResult.person[i].x1);
                    printf("maxy:%d \n", outResult.person[i].y1);
                }
            //     act_print_cnt = 0;
            // }
            // act_print_cnt++;

            if (g_actionAttr.bStart)
            {
                if (outResult.count == 0)
                {
                    LUX_OSD_ActionRect_Miss();
                }
                else if (outResult.count > 0)
                {
                    LUX_OSD_ActionRect_Miss();
                    LUX_OSD_ActionRect_SSDShow(&outResult);
                }
            }
            else
            {
                LUX_OSD_ActionRect_Miss();
            }
#endif
            if (outResult.count > 0)
            {
                LUX_ALG_ActionDet_Alarm();
            }
            // usleep(30 * 1000);
        } while (0);
    }
}

INT_X LUX_ALG_ActionDet_Exit()
{
    delete pRgbAction;
    delete pDstAction;
    delete pOutFrameData;
    return ssd_actionDet_exit(actionDetModel);
}
#endif