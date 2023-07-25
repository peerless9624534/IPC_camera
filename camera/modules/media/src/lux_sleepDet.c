#include <string.h>
#include <unistd.h>
#include "lux_sleepDet.h"
#include "lux_summary.h"
#include <imp_log.h>
#include "lux_iniparse.h"

#define TAG "lux_Sleep_Det"

LUX_IVS_SLEEP_DET_ATTR g_SleepAttr;

int LUX_IVS_SleepDet_Init()
{
    INT_X ret = 0;
    BOOL_X sleepDet_onoff = 0;
    BOOL_X sleepMode = LUX_FALSE;
#if 1//cxj test
    /* 获取报警回调函数 */
    LUX_Event_GetAlarmCbk(&g_SleepAttr.alarmCbk);
    g_SleepAttr.bInit = true;
    LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "sleep_mode", &sleepMode);
    if (LUX_FALSE == sleepMode)
    {
        ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "sleep_det_onoff", &sleepDet_onoff);
        if (0 != ret)
        {
            printf("%s:%d LUX_IVS_SleepDet_Init fail \n", __func__, __LINE__);
            return -1;
        }
        if (sleepDet_onoff)
        {
            LUX_IVS_SleepDet_Start();
        }
    }
#endif
    return 0;
}
int LUX_IVS_SleepDet_Exit()
{
    g_SleepAttr.bInit = LUX_FALSE;
    return 0;
}

void LUX_IVS_SleepDet_Start()
{
    memset(&g_SleepAttr.face,0,sizeof(LUX_IVS_SLEEP_FACE_ST));
    memset(&g_SleepAttr.person,0,sizeof(LUX_IVS_SLEEP_PERSON_ST));
    g_SleepAttr.bStart = true;
}
void LUX_IVS_SleepDet_Stop()
{
    g_SleepAttr.bStart = false;
    //因关闭检测而停止睡眠
    if(0 != g_SleepAttr.sleepStatus)
    {
        g_SleepAttr.sleepStatus = 0;
        //结束睡觉
        g_SleepAttr.sleepStopTime = gettimeStampS();
        if(g_SleepAttr.sleepStopTime > g_SleepAttr.sleepStartTime)
        {
            g_SleepAttr.sleepLength = g_SleepAttr.sleepStopTime - g_SleepAttr.sleepStartTime;
        }

        printf("%s:%d =================================================\n", __func__, __LINE__);
        printf("%s:%d sleepStatus stop[%d] !!!\n", __func__, __LINE__,g_SleepAttr.sleepStatus);
        printf("%s:%d sleepStartTime !!!(%d)\n", __func__, __LINE__,g_SleepAttr.sleepStartTime);
        printf("%s:%d sleepStopTime !!!(%d)\n", __func__, __LINE__,g_SleepAttr.sleepStopTime);
        printf("%s:%d sleepLength !!!(%d)\n", __func__, __LINE__,g_SleepAttr.sleepLength);
        char sSleepEnd[36] = {'\0'};
        sprintf(sSleepEnd,"%d",g_SleepAttr.sleepLength);
        LUX_SUM_Update_Event(LUX_SUM_EVENT_SLEEP_STOP ,g_SleepAttr.sleepStopTime,sSleepEnd);
    }
}

int LUX_IVS_SleepDet_Face_Event(facedet_param_output_t *result)
{
    BOOL_X lastStatus = g_SleepAttr.face.faceStatus;
    if(result->count == 0)
    {
        g_SleepAttr.face.countNoFace++;
        //printf("%s:%d g_SleepAttr.countNoFace(%d)\n", __func__, __LINE__,g_SleepAttr.face.countNoFace);
        if(LUX_SLEEP_FACE_RESET_TIMES <= g_SleepAttr.face.countNoFace)       //大于 LUX_SLEEP_AVERAGE_TIMES次未识别到人脸，则重置计数器
        {
            //面部丢失
            FLOAT_X faceValue = (0*1.0)/LUX_SLEEP_FACE_RESET_TIMES;
            printf("%s:%d faceValue(%d /%d ) = %f \n", __func__, __LINE__,0,LUX_SLEEP_FACE_AVERAGE_TIMES,faceValue);
            g_SleepAttr.face.faceStatus = 0;
            if(g_SleepAttr.face.faceStatus != lastStatus)
            {
                printf("%s:%d =================================================\n", __func__, __LINE__);
                printf("%s:%d faceStatus change[%d] !!!\n", __func__, __LINE__,g_SleepAttr.face.faceStatus);
            }
            g_SleepAttr.face.doCount = 0;
            g_SleepAttr.face.countNoFace = 0;
            g_SleepAttr.face.dofaceDetCount = 0;
            memset(g_SleepAttr.face.allStatus,0,LUX_SLEEP_FACE_AVERAGE_TIMES);
        }
    }
    else
    {   
        g_SleepAttr.face.countNoFace = 0;
        g_SleepAttr.face.doCount++;
        //每LUX_SLEEP_FACE_PICK_TIMES次结果记录一次
        if(0 == (g_SleepAttr.face.doCount % LUX_SLEEP_FACE_PICK_TIMES))
        {
            g_SleepAttr.face.doCount = 0;
            int i = 0;
            {
                for (i = 0; i < result->count; i++)
                {
                    //printf("%s:%d result->face[%d].confidence = %f\n", __func__, __LINE__,i,result->face[i].confidence);
                }
                //将执行度达标面部置1，其他为0
                if(LUX_IVS_FACE_CONFIDENCE <result->face[0].confidence)
                {
                    g_SleepAttr.face.allStatus[g_SleepAttr.face.dofaceDetCount % LUX_SLEEP_FACE_AVERAGE_TIMES] = 1;
                }
            }
            g_SleepAttr.face.dofaceDetCount++;
            //printf("%s:%d g_SleepAttr.dofaceDetCount(%d)\n", __func__, __LINE__,g_SleepAttr.face.dofaceDetCount);
            if(LUX_SLEEP_FACE_AVERAGE_TIMES <= g_SleepAttr.face.dofaceDetCount)
            {
                int x = 0;
                for(i = 0 ; i < LUX_SLEEP_FACE_AVERAGE_TIMES ; i++)
                {
                    x += g_SleepAttr.face.allStatus[i];
                }
                FLOAT_X faceValue = (x*1.0)/LUX_SLEEP_FACE_AVERAGE_TIMES;
                printf("%s:%d faceValue(%d /%d ) = %f \n", __func__, __LINE__,x,LUX_SLEEP_FACE_AVERAGE_TIMES,faceValue);
                if(LUX_SLEEP_IS_FACE_VALUE < faceValue)
                {
                    g_SleepAttr.face.faceStatus = 1;
                }
                else
                {
                    g_SleepAttr.face.faceStatus = 0;
                }
                //面部状态发生改变 //面部稳定出现在画面中
                if(g_SleepAttr.face.faceStatus != lastStatus)
                {
                    printf("%s:%d =================================================\n", __func__, __LINE__);
                    printf("%s:%d faceStatus change[%d] !!!\n", __func__, __LINE__, g_SleepAttr.face.faceStatus);
                }
                //清理计数器
                g_SleepAttr.face.dofaceDetCount = 0;
                memset(g_SleepAttr.face.allStatus,0,LUX_SLEEP_FACE_AVERAGE_TIMES);
            }
        }
    }
    return 0;

}

int LUX_IVS_SleepDet_SSDFace_Event(LUX_FACEDET_OUTPUT_ST *result)
{
    if (!g_SleepAttr.bInit || !g_SleepAttr.bStart)
    {
        return 0;
    }

    BOOL_X lastStatus = g_SleepAttr.face.faceStatus;
    if(result->count == 0)
    {
        g_SleepAttr.face.countNoFace++;
        //printf("%s:%d g_SleepAttr.countNoFace(%d)\n", __func__, __LINE__,g_SleepAttr.face.countNoFace);
        if(LUX_SLEEP_FACE_RESET_TIMES <= g_SleepAttr.face.countNoFace)       //大于 LUX_SLEEP_AVERAGE_TIMES次未识别到人脸，则重置计数器
        {
            //面部丢失
            FLOAT_X faceValue = (0*1.0)/LUX_SLEEP_FACE_RESET_TIMES;
            printf("%s:%d faceValue(%d /%d ) = %f \n", __func__, __LINE__,0,LUX_SLEEP_FACE_AVERAGE_TIMES,faceValue);
            g_SleepAttr.face.faceStatus = 0;
            if(g_SleepAttr.face.faceStatus != lastStatus)
            {
                printf("%s:%d =================================================\n", __func__, __LINE__);
                printf("%s:%d faceStatus change[%d] !!!\n", __func__, __LINE__,g_SleepAttr.face.faceStatus);
            }
            g_SleepAttr.face.doCount = 0;
            g_SleepAttr.face.countNoFace = 0;
            g_SleepAttr.face.dofaceDetCount = 0;
            memset(g_SleepAttr.face.allStatus,0,LUX_SLEEP_FACE_AVERAGE_TIMES);
        }
    }
    else
    {   
        g_SleepAttr.face.countNoFace = 0;
        g_SleepAttr.face.doCount++;
        //每LUX_SLEEP_FACE_PICK_TIMES次结果记录一次
        if(0 == (g_SleepAttr.face.doCount % LUX_SLEEP_FACE_PICK_TIMES))
        {
            g_SleepAttr.face.doCount = 0;
            int i = 0;
            {
                for (i = 0; i < result->count; i++)
                {
                    //printf("%s:%d result->face[%d].confidence = %f\n", __func__, __LINE__,i,result->face[i].confidence);
                }
                //将执行度达标面部置1，其他为0
                if(LUX_IVS_FACE_CONFIDENCE <result->face[0].confidence)
                {
                    g_SleepAttr.face.allStatus[g_SleepAttr.face.dofaceDetCount % LUX_SLEEP_FACE_AVERAGE_TIMES] = 1;
                }
            }
            g_SleepAttr.face.dofaceDetCount++;
            //printf("%s:%d g_SleepAttr.dofaceDetCount(%d)\n", __func__, __LINE__,g_SleepAttr.face.dofaceDetCount);
            if(LUX_SLEEP_FACE_AVERAGE_TIMES <= g_SleepAttr.face.dofaceDetCount)
            {
                int x = 0;
                for(i = 0 ; i < LUX_SLEEP_FACE_AVERAGE_TIMES ; i++)
                {
                    x += g_SleepAttr.face.allStatus[i];
                }
                FLOAT_X faceValue = (x*1.0)/LUX_SLEEP_FACE_AVERAGE_TIMES;
                printf("%s:%d faceValue(%d /%d ) = %f \n", __func__, __LINE__,x,LUX_SLEEP_FACE_AVERAGE_TIMES,faceValue);
                if(LUX_SLEEP_IS_FACE_VALUE < faceValue)
                {
                    g_SleepAttr.face.faceStatus = 1;
                }
                else
                {
                    g_SleepAttr.face.faceStatus = 0;
                }
                //面部状态发生改变 //面部稳定出现在画面中
                if(g_SleepAttr.face.faceStatus != lastStatus)
                {
                    printf("%s:%d =================================================\n", __func__, __LINE__);
                    printf("%s:%d faceStatus change[%d] !!!\n", __func__, __LINE__, g_SleepAttr.face.faceStatus);
                }
                //清理计数器
                g_SleepAttr.face.dofaceDetCount = 0;
                memset(g_SleepAttr.face.allStatus,0,LUX_SLEEP_FACE_AVERAGE_TIMES);
            }
        }
    }
    return 0;

}


int LUX_IVS_SleepDet_PersonMov_Event(LUX_BASE_IVSDATA_ST *result)
{
    if(!g_SleepAttr.bInit || !g_SleepAttr.bStart)
    {
        return 0;
    }

    int ret = 0;
    BOOL_X lastStatus = g_SleepAttr.sleepStatus;
    UINT_X currTime  = gettimeStampS();
    g_SleepAttr.person.doCount++;
    g_SleepAttr.person.moveValue = g_SleepAttr.person.moveValue + result->rectcnt;
    //每LUX_SLEEP_PERSON_PICK_TIMES次结果记录一次

    if(0 == lastStatus) //判断入睡
    {
        if(0 == (g_SleepAttr.person.doCount % LUX_SLEEP_PERSON_PICK_TIMES))
        {
            // if(0 == g_SleepAttr.person.doPersonDetCount)
            // {
            //     g_SleepAttr.tmpStartTime = currTime;
            //     printf("%s:%d tmpStartTime[%d] !!!\n", __func__, __LINE__,g_SleepAttr.tmpStartTime);
            // }
            //printf("%s:%d moveValue[%d] = %d \n", __func__, __LINE__,g_SleepAttr.person.doPersonDetCount,g_SleepAttr.person.moveValue);
            if(g_SleepAttr.person.moveValue > 0)
            {
                g_SleepAttr.person.asleepStatus[g_SleepAttr.person.doPersonDetCount % LUX_SLEEP_PERSON_ASLEEP_TIMES] = 1;
            }
            else
            {
                g_SleepAttr.person.asleepStatus[g_SleepAttr.person.doPersonDetCount % LUX_SLEEP_PERSON_ASLEEP_TIMES] = 0;
            }
            g_SleepAttr.person.doCount = 0;
            g_SleepAttr.person.moveValue = 0;
            g_SleepAttr.person.doPersonDetCount++;
            //当计数达到次数，并且在画面中，判断是否达到入睡状态
            if(0 == (g_SleepAttr.person.doPersonDetCount % LUX_SLEEP_PERSON_ASLEEP_TIMES))
            {
                int i = 0;
                int x = 0;
                for(i = 0 ; i < LUX_SLEEP_PERSON_ASLEEP_TIMES ; i++)
                {
                    x = x + g_SleepAttr.person.asleepStatus[i];
                }
                FLOAT_X sleepValue = (x*1.0)/LUX_SLEEP_PERSON_ASLEEP_TIMES;
                printf("%s:%d sleepValue(x =%d/ %d) = %f \n", __func__, __LINE__,x,LUX_SLEEP_PERSON_ASLEEP_TIMES,sleepValue);
                x = 0;
                if(g_SleepAttr.face.faceStatus)   //存在面部
                {
                    if(LUX_SLEEP_START_VALUE > sleepValue)  
                    {
                        g_SleepAttr.sleepStatus = 1;        //入睡
                    }
                    else
                    {
                        //维持现状，，或提示宝宝有动静
                    }
                    printf("%s:%d sleepStatus %d faceStatus %d\n", __func__, __LINE__,g_SleepAttr.sleepStatus,g_SleepAttr.face.faceStatus);
                    if(g_SleepAttr.sleepStatus != lastStatus) 
                    {
                        //开始睡觉
                        printf("%s:%d =================================================\n", __func__, __LINE__);
                        printf("%s:%d sleepStatus start[%d] !!!\n", __func__, __LINE__,g_SleepAttr.sleepStatus);
                        g_SleepAttr.sleepStartTime = currTime;
                        g_SleepAttr.sleepKeepTime = currTime;
                        printf("%s:%d sleepStartTime !!!(%d)\n", __func__, __LINE__,g_SleepAttr.sleepStartTime);
                        LUX_SUM_Update_Event(LUX_SUM_EVENT_SLEEP_START,g_SleepAttr.sleepStartTime,NULL);

                        //消息告警 宝宝睡着了
                        do
                        {    
                            LUX_ENCODE_STREAM_ST picInfo[2];
                            ret = LUX_BASE_CapturePic_HD(LUX_STREAM_SUB,&picInfo[LUX_STREAM_SUB]);
                            if (0 != ret)
                            {
                                LUX_BASE_ReleasePic_HD(LUX_STREAM_SUB,&picInfo[LUX_STREAM_SUB]);
                                IMP_LOG_ERR(TAG, " LUX_BASE_CapturePic failed\n");
                                break;
                            }
                            /*上报抓图*/
                            if (NULL != g_SleepAttr.alarmCbk.pEvntReportFuncCB)
                            {
                                g_SleepAttr.alarmCbk.pEvntReportFuncCB(picInfo[LUX_STREAM_SUB].pAddr, picInfo[LUX_STREAM_SUB].streamLen, LUX_NOTIFICATION_NAME_CALL_ACCEPT);
                            }
                            LUX_BASE_ReleasePic_HD(LUX_STREAM_SUB,&picInfo[LUX_STREAM_SUB]);

                        } while (0);

                    }
                }
                g_SleepAttr.person.doPersonDetCount = 0;
                memset(g_SleepAttr.person.asleepStatus,0,LUX_SLEEP_PERSON_ASLEEP_TIMES);
                memset(g_SleepAttr.person.awakeStatus,0,LUX_SLEEP_PERSON_AWAKE_TIMES);
            }

        }
    }
    else    //判断睡醒
    {
        if(0 == (g_SleepAttr.person.doCount % LUX_SLEEP_PERSON_PICK_TIMES))
        {
            //printf("%s:%d moveValue[%d] = %d \n", __func__, __LINE__,g_SleepAttr.person.doPersonDetCount,g_SleepAttr.person.moveValue);
            if(g_SleepAttr.person.moveValue > 0)
            {
                g_SleepAttr.person.awakeStatus[g_SleepAttr.person.doPersonDetCount % LUX_SLEEP_PERSON_AWAKE_TIMES] = 1;
            }
            else
            {
                g_SleepAttr.person.awakeStatus[g_SleepAttr.person.doPersonDetCount % LUX_SLEEP_PERSON_AWAKE_TIMES] = 0;
            }
            g_SleepAttr.person.doCount = 0;
            g_SleepAttr.person.moveValue = 0;
            g_SleepAttr.person.doPersonDetCount++;
            //当计数达到次数，并且在画面中，判断是否达到入睡状态
            if(0 == (g_SleepAttr.person.doPersonDetCount % LUX_SLEEP_PERSON_AWAKE_TIMES))
            {
                int i = 0;
                int x = 0;
                bool bmakeAwake = false; //强制睡醒标识
                //睡眠时间段记录
                if(currTime - g_SleepAttr.sleepKeepTime >= LUX_SLEEP_KEEP_SLEEP)
                {
                    g_SleepAttr.sleepKeepTime = currTime;
                    LUX_SUM_Update_Event(LUX_SUM_EVENT_SLEEP_KEEP ,g_SleepAttr.sleepKeepTime,NULL);
                }

                for(i = 0 ; i < LUX_SLEEP_PERSON_AWAKE_TIMES ; i++)
                {
                    x += g_SleepAttr.person.awakeStatus[i];
                }
                FLOAT_X sleepValue = (x*1.0) /LUX_SLEEP_PERSON_AWAKE_TIMES;
                printf("%s:%d sleepValue(x =%d /%d) = %f \n", __func__, __LINE__,x,LUX_SLEEP_PERSON_AWAKE_TIMES,sleepValue);
                x = 0;
                if(g_SleepAttr.face.faceStatus)   //存在面部
                {
                    if(LUX_SLEEP_STOP_VALUE < sleepValue)  
                    {
                        g_SleepAttr.sleepStatus = 0;        //睡醒
                    }
                    else
                    {
                        //维持现状，，或提示宝宝有动静
                    }
                }
                if(g_SleepAttr.sleepStatus)
                {
                    if (currTime > (g_SleepAttr.sleepStartTime + LUX_SLEEP_FORCE_AWAKE_TIMES))//超12小时强制睡醒,不提醒
                    {
                        g_SleepAttr.sleepStatus = 0;
                        bmakeAwake = true;
                    } 
                }
                if(0 == sleepValue && 0 == g_SleepAttr.face.faceStatus)         //连续多次无人时，强迫睡醒,不提醒
                {
                    g_SleepAttr.sleepNoPersonCnt++;
                    printf("%s:%d sleepNoPersonCnt = %d \n", __func__, __LINE__,g_SleepAttr.sleepNoPersonCnt);
                    if(g_SleepAttr.sleepNoPersonCnt >= LUX_SLEEP_NO_PERSON_AWAKE_CNT)
                    {
                        g_SleepAttr.sleepStatus = 0;
                        bmakeAwake = true;
                    }
                }
                else
                {
                    g_SleepAttr.sleepNoPersonCnt = 0;
                }
                printf("%s:%d sleepStatus %d faceStatus %d\n", __func__, __LINE__,g_SleepAttr.sleepStatus,g_SleepAttr.face.faceStatus);
                if(g_SleepAttr.sleepStatus != lastStatus)
                {
                    //结束睡觉
                    g_SleepAttr.sleepStopTime = currTime;
                    if(g_SleepAttr.sleepStopTime > g_SleepAttr.sleepStartTime)
                    {
                        g_SleepAttr.sleepLength = g_SleepAttr.sleepStopTime - g_SleepAttr.sleepStartTime;
                    }
                    printf("%s:%d =================================================\n", __func__, __LINE__);
                    printf("%s:%d sleepStatus stop[%d] !!!\n", __func__, __LINE__,g_SleepAttr.sleepStatus);
                    printf("%s:%d sleepStartTime !!!(%d)\n", __func__, __LINE__,g_SleepAttr.sleepStartTime);
                    printf("%s:%d sleepStopTime !!!(%d)\n", __func__, __LINE__,g_SleepAttr.sleepStopTime);
                    printf("%s:%d sleepLength !!!(%d)\n", __func__, __LINE__,g_SleepAttr.sleepLength);
                    char sSleepEnd[36] = {'\0'};
                    sprintf(sSleepEnd,"%d",g_SleepAttr.sleepLength);
                    LUX_SUM_Update_Event(LUX_SUM_EVENT_SLEEP_STOP ,g_SleepAttr.sleepStopTime,sSleepEnd);
                    //消息告警 宝宝醒了
                    if(!bmakeAwake) //如果强制睡醒，则不推送消息
                    {
                        do
                        {
                            LUX_ENCODE_STREAM_ST picInfo[2];
                            ret = LUX_BASE_CapturePic_HD(LUX_STREAM_SUB,&picInfo[LUX_STREAM_SUB]);
                            if (0 != ret)
                            {
                                LUX_BASE_ReleasePic_HD(LUX_STREAM_SUB,&picInfo[LUX_STREAM_SUB]);
                                IMP_LOG_ERR(TAG, " LUX_BASE_CapturePic failed\n");
                                break;
                            }
                            /*上报抓图*/
                            if (NULL != g_SleepAttr.alarmCbk.pEvntReportFuncCB)
                            {
                                g_SleepAttr.alarmCbk.pEvntReportFuncCB(picInfo[LUX_STREAM_SUB].pAddr, picInfo[LUX_STREAM_SUB].streamLen, LUX_NOTIFICATION_NAME_CALL_NOT_ACCEPT);
                            }
                            LUX_BASE_ReleasePic_HD(LUX_STREAM_SUB,&picInfo[LUX_STREAM_SUB]);
                        } while (0);
                    }
                }

                g_SleepAttr.person.doPersonDetCount = 0;
                memset(g_SleepAttr.person.asleepStatus,0,LUX_SLEEP_PERSON_ASLEEP_TIMES);
                memset(g_SleepAttr.person.awakeStatus,0,LUX_SLEEP_PERSON_AWAKE_TIMES);
            }
        }
    }



    return 0;
}
