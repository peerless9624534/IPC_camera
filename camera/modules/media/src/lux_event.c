/*
 * lux_event.c
 *
 * 事件处理中心，报警，事件下发、上传
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/prctl.h>

#include <imp_log.h>

#include "su_base.h"
#include "lux_event.h"
#include "comm_error_code.h"
#include <lux_base.h>
#include <lux_ivs.h>
#include "comm_def.h"
#include <lux_iniparse.h>
#include <lux_hal_motor.h>
#include "lux_fsrc.h"
#include "lux_sleepDet.h"
#include "lux_summary.h"

#define TAG     "lux_event"

#define LUX_EVENT_PD_LIMIT 0.64

BOOL_X recordReboot = 1;  //判断是否为重新上电，1表示重新上电

LUX_EVENT_ATTR_ST   g_eventAttr = {.cloudId = 0XFFFFFFFF};  //初始化无效值


/**
 * @description: 重启设备
 * @return 成功：0，失败：-1；
 */
INT_X LUX_Event_Reboot()
{
    return SU_Base_Reboot();
}

/**
 * @description: 移动侦测事件处理函数
 * @param [in] pFuncCBAttr 事件上报回调函数
 */
INT_X LUX_EVENT_IvsRegistCB(LUX_EVENT_ATTR_ST *pFuncCBAttr)
{

    if(NULL == pFuncCBAttr)
    {
        printf("%s:%d LUX_EVENT_IvsRegistCB failed, empty pointer\n", __func__, __LINE__);
        return LUX_ERR;
    }

    if(NULL == pFuncCBAttr->pEvntReportFuncCB || NULL == pFuncCBAttr->pStartEventRecordFuncCB || NULL == pFuncCBAttr->pStopEventRecordFuncCB || NULL == pFuncCBAttr->pGetRecordstatus
        || NULL == pFuncCBAttr->pAddCloudEvent || NULL == pFuncCBAttr->pDeletCloudEvent || NULL == pFuncCBAttr->pGetCloudEventStatus)
    {
        printf("%s:%d LUX_EVENT_IvsRegistCB failed, empty funcCB pointer\n", __func__, __LINE__);
        return LUX_ERR;
    }

    memcpy(&g_eventAttr, pFuncCBAttr, sizeof(LUX_EVENT_ATTR_ST));

    return LUX_OK;
}

/**
 * @description: 停止事件录像回调函数
 */
static void *LUX_Base_stopEventRecordCB(void *arg)
{
    //事件录制时间大于30秒，并且录制状态为未停止
    unsigned int NowTime = getTime_getMs();
    unsigned int EventTime = GetEventTime();
    //printf("%s:%d NowTime:[%d] \n", __func__, __LINE__, NowTime);
    //printf("%s:%d EventTime:[%d] \n", __func__, __LINE__, EventTime);
    if(NowTime - EventTime >= LUX_COMM_EVENT_RECORD_TIME) 
    {
        if(LUX_RECOR_STOP != (LUX_RECOR_STATUS_E)g_eventAttr.pGetRecordstatus())
        {
            g_eventAttr.pStopEventRecordFuncCB();
        }
    }
    return (void *)0;
}

/**
 * @description: 侦测事件录像
 * @param [in] ts 录像时长,单位ms
 * @return 0 成功，非零失败
 */
INT_X LUX_Event_Record(UINT_X ts)
{   
    INT_X ret = LUX_ERR;
    INT_X timerID = LUX_ERR;
    LUX_BASE_TIMER_PARAM RecordParam;

    /*记录当前事件时间戳*/
    SetEventTime();
    GetEventTime();
    RecordParam.bIntervalEnable = 0;
    RecordParam.pTimerProcess = LUX_Base_stopEventRecordCB;
    RecordParam.cbArgs = NULL;
    RecordParam.timeInterval  = ts;

    timerID = LUX_BASE_TimerCreate(&RecordParam);
    if(LUX_ERR == timerID)
    {
        IMP_LOG_ERR(TAG, " LUX_BASE_TimerCreate failed\n");
        return -1;
    }
    /*录制状态为停止时开始录制*/
    if (LUX_RECOR_STOP == (LUX_RECOR_STATUS_E)g_eventAttr.pGetRecordstatus())
    {   
        g_eventAttr.pStartEventRecordFuncCB();
    }

    ret = LUX_BASE_TimerStart(timerID);
    if(LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " LUX_BASE_TimerStart failed\n");
        return -1;
    }

    return LUX_OK;
}



/**
 * @description: 移动侦测事件处理函数
 * @param [in] IvsResult IVS处理结果
 * @return 0 成功，非零失败
 */
static INT_X LUX_Event_MovDet_Process(LUX_BASE_IVSDATA_ST *MovResult)
{
    INT_X ret = LUX_ERR;
    LUX_ENCODE_STREAM_ST picInfo[2];
    BOOL_X recordOnOff = 0;
    INT_X  recordMode = 0;

    if(NULL == MovResult)
    {
        printf("%s:%d LUX_Event_MovDet_Process failed, empty MovResult\n", __func__, __LINE__);
        return LUX_ERR;
    }

    /*报警事件结果*/
    if (LUX_TRUE == MovResult->retRoi)
    {
        /* 检测是否在报警区域的移动 */
        if (LUX_IVS_MovDetRegion_Process(MovResult))
        {
            /* 若满足报警间隔，上报事件、同时更新报警时间 */
            if (LUX_FALSE == LUX_IVS_GetReportEvent_Countdown())
            {
                return LUX_OK;
            }
            // LUX_SUM_Update_Event(LUX_SUM_EVENT_MOVE_DET ,gettimeStampS(),NULL);
            ret = LUX_BASE_CapturePic_HD(LUX_STREAM_SUB,&picInfo[LUX_STREAM_SUB]);
            if(LUX_OK != ret)
            {
                LUX_BASE_ReleasePic_HD(LUX_STREAM_SUB,&picInfo[LUX_STREAM_SUB]);
                IMP_LOG_ERR(TAG, " LUX_BASE_CapturePic failed\n");
                return LUX_ERR;
            }
            /*上报抓图*/
            if(NULL != g_eventAttr.pEvntReportFuncCB)
            {
                g_eventAttr.pEvntReportFuncCB(picInfo[LUX_STREAM_SUB].pAddr, picInfo[LUX_STREAM_SUB].streamLen, LUX_NOTIFICATION_NAME_MOTION);
            }

            /* 云存储 */
            if (NULL != g_eventAttr.pAddCloudEvent && NULL != g_eventAttr.pDeletCloudEvent && NULL != g_eventAttr.pGetCloudEventStatus)
            {            
                //if (g_eventAttr.pGetCloudEventStatus(g_eventAttr.cloudId))
                g_eventAttr.cloudId = g_eventAttr.pAddCloudEvent((char *)picInfo[LUX_STREAM_SUB].pAddr, picInfo[LUX_STREAM_SUB].streamLen, LUX_EVENT_TYPE_MOTION_DETECT, LUX_CLOUD_STORAGE_TIME); //最长录300s
                printf("%s:%d cloudId:%d \n", __func__, __LINE__, g_eventAttr.cloudId);

            }
            LUX_BASE_ReleasePic_HD(LUX_STREAM_SUB,&picInfo[LUX_STREAM_SUB]);
            // ret = LUX_BASE_ReleasePic(LUX_STREAM_JPEG, &picInfo);
            // if(LUX_OK != ret)
            // {
            //     IMP_LOG_ERR(TAG, " LUX_BASE_ReleasePic failed\n");
            //     return LUX_ERR;
            // }

            /*事件录像*/
            ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "record", "record_onoff", &recordOnOff);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "ERROR:LUX_INIParse_SetKey ret:%#x", ret);
            }

            ret = LUX_INIParse_GetInt(TUYA_INI_CONFIG_FILE_NAME, "record", "record_mode", &recordMode);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "ERROR:LUX_INIParse_SetKey ret:%#x", ret);
            }

            if(LUX_TRUE == recordOnOff && 0 == recordMode)
            {
                ret = LUX_Event_Record(LUX_COMM_EVENT_RECORD_TIME);
                if (LUX_OK != ret)
                {
                    IMP_LOG_ERR(TAG, "ERROR:LUX_INIParse_SetKey ret:%#x", ret);
                }
            }
        }
    }

    return LUX_OK;
}

int LUX_Event_GetAlarmCbk(LUX_EVENT_ATTR_ST *pEventCbk)
{
    if (NULL == pEventCbk)
    {
        printf("%s:%d null prt!\n", __func__, __LINE__);
        return -1;
    }

    memcpy(pEventCbk, &g_eventAttr, sizeof(LUX_EVENT_ATTR_ST));

    return 0;
}

/**
 * @description: 人形侦测事件处理函数
 * @param [in] IvsResult IVS处理结果
 * @return 0 成功，非零失败
 */
static INT_X LUX_Event_PD_Process(LUX_BASE_IVSDATA_ST *PDResult)
{
    INT_X ret = LUX_ERR;
    INT_X peopleNum = 1;
    LUX_ENCODE_STREAM_ST picInfo[2];
    BOOL_X recordOnOff = 0;
    INT_X  recordMode = 0;
    INT_X i;
    BOOL_X findPerson = 0;

    if(LUX_TRUE != PDResult->ivsStart[LUX_IVS_PERSON_DET_EN])
    {
        return 0;
    }
    if(NULL == PDResult)
    {
        printf("%s:%d LUX_Event_PD_Process failed, empty PDResult\n", __func__, __LINE__);
        return LUX_ERR;
    }

    if(PDResult->rectcnt >= peopleNum)
    {
        for (i = 0; i < PDResult->rectcnt; i++)
        { 
            if(PDResult->person[i].confidence > LUX_EVENT_PD_LIMIT)
            {
                //printf("%s:%d confidence:%f \n", __func__, __LINE__, PDResult->person[i].confidence);
                findPerson = 1;
            }
        }
    }
    /*判断置信度 && 时间间隔*/
    if (findPerson && LUX_IVS_GetReportEvent_Countdown())
    {
        //LUX_SUM_Update_Event(LUX_SUM_EVENT_PERSON_DET ,gettimeStampS(),NULL);

        ret = LUX_BASE_CapturePic_HD(LUX_STREAM_SUB,&picInfo[LUX_STREAM_SUB]);
        if(LUX_OK != ret)
        {
            LUX_BASE_ReleasePic_HD(LUX_STREAM_SUB,&picInfo[LUX_STREAM_SUB]);
            IMP_LOG_ERR(TAG, " LUX_BASE_CapturePic failed\n");
            return LUX_ERR;
        }
        /*上报抓图*/
        if(NULL != g_eventAttr.pEvntReportFuncCB)
        {                                                                   //LUX_NOTIFICATION_NAME_HUMAN
            g_eventAttr.pEvntReportFuncCB(picInfo[LUX_STREAM_SUB].pAddr, picInfo[LUX_STREAM_SUB].streamLen, LUX_NOTIFICATION_NAME_MOTION);
        }

        /* 云存储 */
        if (NULL != g_eventAttr.pAddCloudEvent && NULL != g_eventAttr.pDeletCloudEvent && NULL != g_eventAttr.pGetCloudEventStatus)
        {

            //if (g_eventAttr.pGetCloudEventStatus(g_eventAttr.cloudId))
            g_eventAttr.cloudId = g_eventAttr.pAddCloudEvent((char *)picInfo[LUX_STREAM_SUB].pAddr, picInfo[LUX_STREAM_SUB].streamLen, LUX_EVENT_TYPE_MOTION_DETECT, LUX_CLOUD_STORAGE_TIME); //最长录300s
            printf("%s:%d cloudId:%d \n", __func__, __LINE__, g_eventAttr.cloudId);
        }
        LUX_BASE_ReleasePic_HD(LUX_STREAM_SUB,&picInfo[LUX_STREAM_SUB]);
        // ret = LUX_BASE_ReleasePic(LUX_STREAM_JPEG, &picInfo);
        // if(LUX_OK != ret)
        // {
        //     IMP_LOG_ERR(TAG, " LUX_BASE_ReleasePic failed\n");
        //     return LUX_ERR;
        // }

        /*事件录像*/
        ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "record", "record_onoff", &recordOnOff);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "ERROR:LUX_INIParse_SetKey ret:%#x", ret);
        }

        ret = LUX_INIParse_GetInt(TUYA_INI_CONFIG_FILE_NAME, "record", "record_mode", &recordMode);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "ERROR:LUX_INIParse_SetKey ret:%#x", ret);
        }

        if(LUX_TRUE == recordOnOff && 0 == recordMode)
        {   
            ret = LUX_Event_Record(LUX_COMM_EVENT_RECORD_TIME);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "ERROR:LUX_INIParse_SetKey ret:%#x", ret);
            }
        }
        
    }
        
    
    
    return LUX_OK;
}

#ifdef CONFIG_PTZ_IPC
/**
 * @description: 人形侦测事件处理函数
 * @param [in] IvsResult IVS处理结果
 * @return 0 成功，非零失败
 */
static INT_X LUX_Event_PT_Process(LUX_BASE_IVSDATA_ST *pPTResult)
{
    person_tracker_param_output_t *pResult =(person_tracker_param_output_t *)pPTResult->IVSResult;
    CallBackFunc  pFuncReleaseRet = (CallBackFunc)pPTResult->pCallBackFunc; 
   // printf("dx [%d], dy[%d], tracking[%d], capture_lost[%d]\n", pResult->dx, pResult->dy, pResult->tracking, pResult->capture_lost);
    LUX_HAL_MOTOR_PersonTracker(pResult->dx, pResult->dy, pResult->tracking, pResult->capture_lost);
    pFuncReleaseRet(pPTResult->pCallBackInfo);

    return LUX_OK;
}
#endif

/**
 * @description: IVS智能模块事件处理函数
 * @param [in] IvsResult IVS处理结果
 * @return 0 成功，非零失败，返回错误码
 */
static INT_X LUX_Event_IVSProcess(LUX_BASE_IVSDATA_ST *IvsResult)
{
    INT_X ret = LUX_ERR;

    if(NULL == IvsResult)
    {
        printf("%s:%d LUX_Event_IVSProcess failed, empty IvsResult\n", __func__, __LINE__);
        return LUX_ERR;
    }
    
    switch(IvsResult->type)
    {
        case LUX_BASE_MOVDET_QUE_EN:
            ret = LUX_Event_MovDet_Process(IvsResult);
            if(LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, " LUX_Event_MovDetIVS_Process failed\n");
                return LUX_ERR;
            }
            break;

        case LUX_BASE_PERSONDET_QUE_EN:

            ret = LUX_Event_PD_Process(IvsResult);
            if(LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "LUX_Event_PDIVS_Process failed\n");
                return LUX_ERR;
            }
            //睡眠检测
            ret = LUX_IVS_SleepDet_PersonMov_Event(IvsResult);
            if(LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "LUX_Event_PDIVS_Process failed\n");
                return LUX_ERR;
            }
            break;

#ifdef CONFIG_PTZ_IPC            
        case LUX_BASE_PERSONTRACK_QUE_EN:
            ret = LUX_Event_PT_Process(IvsResult);
            if(LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "LUX_Event_PT_Process failed\n");
                return LUX_ERR;
            }
            break;
#endif
        default:
                IMP_LOG_ERR(TAG, "process IVS event error, not support the event!\n");
                return LUX_ERR;
    }

    return LUX_OK;
}

/**
 * @description: IVS智能模块抓图处理线程
 * @return 0 成功，非零失败，返回错误码
 */
static void *lux_event_ivs_process_thread(void* arg)
{
    LUX_BASE_IVSDATA_ST getIvsResult;
    INT_X ret = LUX_ERR;

    prctl(PR_SET_NAME, "IVS_EVENT_PROCESS");

    while(1)
    {
        if(0 == QueueLength(&g_ivs_que))
        {
            usleep(100 * 1000);
            continue;
        }

        ret = PopQueue(&g_ivs_que, &getIvsResult);

        /*分类处理事件*/
        ret = LUX_Event_IVSProcess(&getIvsResult);
        if(LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, " LUX_Event_IVSProcess failed\n");
        }
        usleep(10 * 1000);
    }

    return (void*)LUX_OK;
}


/**
 * @description: IVS智能模块抓图初始化
 * @return 0 成功，非零失败，返回错误码
 */
INT_X LUX_EVENT_IVSProcess_Init()
{
    INT_X ret = LUX_ERR;

    ret = Task_CreateThread(lux_event_ivs_process_thread, NULL);
    if(LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " pthread_create failed\n");
		return LUX_ERR;
    }

    return LUX_OK;
}

CHAR_X s_alarm_interval_x[4] = {0};

static INT_X __tuya_app_read_STR_X(CHAR_X *key, CHAR_X *value, INT_X value_size)
{
    CHAR_X tmp[64] = {0};
    INT_X ret = 0;

    ret = LUX_INIParse_GetString(TUYA_INI_CONFIG_FILE_NAME, "function", key, tmp);

    if (0 != ret)
    {
        IMP_LOG_ERR(TAG, "LUX_INIParse_GetInt failed,  key[%s]!, error num [0x%x] ", key, ret);
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

static CHAR_X *IPC_APP_get_alarm_interval_X(VOID)
{
    /* Motion detection alarm interval,unit is minutes,ENUM type,"0","1","2"*/
    __tuya_app_read_STR_X("alarm_interval",s_alarm_interval_x, 4);
    printf("%s:%d curr alarm_interval:%s \r\n", __func__, __LINE__, s_alarm_interval_x);

   return s_alarm_interval_x;
}
extern LUX_IVS_TIMEINTERVAL_EN g_alarmInterval; /* 报警时间间隔 */

BOOL_X LUX_Event_IsAlarmByInterval(UINT_X lastTime)
{
    UINT_X timeInterval = 0;
    UINT_X curTime = 0;
    BOOL_X bAlarm = 0;

    if (1 == recordReboot)  //重新上电读取配置文件值
    {
        CHAR_X *p_getalarmInterval = IPC_APP_get_alarm_interval_X();
        g_alarmInterval = (LUX_IVS_TIMEINTERVAL_EN)(*p_getalarmInterval - '0');
        recordReboot = 0;
    }

    switch (g_alarmInterval)
    {
    case LUX_IVS_TIMEINTERVAL_1MIN:
        timeInterval = 60;
        break;
    case LUX_IVS_TIMEINTERVAL_3MIN:
        timeInterval = 3 * 60;
        break;
    case LUX_IVS_TIMEINTERVAL_5MIN:
        timeInterval = 5 * 60;
        break;
    default:
        timeInterval = 60;
        break;
    }

    curTime = getTime_getS();
    if (curTime - lastTime > timeInterval)
    {
        bAlarm = 1;
    }
    //printf("INFO: %s:%d timeInterval:%d curTime:%d lastTime:%d-%d bAlarm:%d\n",
    //       __func__, __LINE__, timeInterval, curTime, lastTime, *pLastTime, bAlarm);

    return bAlarm;
}
