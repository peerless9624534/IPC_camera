#include <string.h>
#include <unistd.h>
#include <imp_log.h>
#include <imp_common.h>
#include <imp_system.h>
#include <imp_framesource.h>

#include <ivs_common.h>
#include <ivs_interface.h>
#include <ivs_inf_perm.h>
#include <imp_ivs.h>

//#include "sample-common.h"
#include "comm_def.h"
#include "lux_base.h"
#include "lux_perm.h"
#include "lux_event.h"
#include "lux_video.h"
#include "lux_iniparse.h"
#include "lux_ivs.h"
#include "lux_fsrc.h"
#include "cJSON.h"

#define TAG "SAMPLE-PERM"

typedef enum
{
    LUX_ISP_PERM_SQUARE,
    LUX_ISP_PERM_POLYGON,
}LUX_ISP_PERM_TYPE_EN;


typedef struct
{
    BOOL_X bInit;
    BOOL_X bStart;
    int ivsGrpNum;
    int ivsChnNum;
    IMPIVSInterface *pInterface;
    OSI_MUTEX mutex;
    
    IVSPoint pointTmp[6];
    perm_param_input_t param;
    LUX_ISP_PERM_TYPE_EN permType;
    LUX_EVENT_ATTR_ST alarmCbk;
    UINT_X alarmTime;
} LUX_IVS_PERM_ATTR;

LUX_IVS_PERM_ATTR g_permAttr =
{
    .ivsGrpNum = 0,
    .ivsChnNum = 4,
};
void LUX_IVS_Perm_Process();

static int LUX_IVS_PermAlarm()
{
    int ret = 0;
    BOOL_X recordOnOff = 0;
    int recordMode = 0;
    LUX_ENCODE_STREAM_ST picInfo[2];

    /*报警时间间隔*/
    if (LUX_Event_IsAlarmByInterval(g_permAttr.alarmTime))
    {
        g_permAttr.alarmTime = getTime_getS();
        ret = LUX_BASE_CapturePic_HD(LUX_STREAM_SUB,&picInfo[LUX_STREAM_SUB]);
        //ret = LUX_BASE_CapturePic(LUX_STREAM_JPEG, &picInfo);
        if (0 != ret)
        {
            LUX_BASE_ReleasePic_HD(LUX_STREAM_SUB,&picInfo[LUX_STREAM_SUB]);
            IMP_LOG_ERR(TAG, " LUX_BASE_CapturePic failed\n");
            return -1;
        }
        /*上报抓图*/
        if (NULL != g_permAttr.alarmCbk.pEvntReportFuncCB)
        {
            g_permAttr.alarmCbk.pEvntReportFuncCB(picInfo[LUX_STREAM_SUB].pAddr, picInfo[LUX_STREAM_SUB].streamLen, LUX_NOTIFICATION_NAME_LINGER);
        }
        
        /* 云存储 */
        if (NULL != g_permAttr.alarmCbk.pAddCloudEvent && NULL != g_permAttr.alarmCbk.pDeletCloudEvent &&
            NULL != g_permAttr.alarmCbk.pGetCloudEventStatus)
        {
            
            g_permAttr.alarmCbk.cloudId = g_permAttr.alarmCbk.pAddCloudEvent(picInfo[LUX_STREAM_SUB].pAddr, picInfo[LUX_STREAM_SUB].streamLen, LUX_EVENT_TYPE_LINGER, LUX_CLOUD_STORAGE_TIME); //最长录300s
            printf("%s:%d cloudId:%d \n", __func__, __LINE__, g_permAttr.alarmCbk.cloudId);
            
        }
        LUX_BASE_ReleasePic_HD(LUX_STREAM_SUB,&picInfo[LUX_STREAM_SUB]);
        // ret = LUX_BASE_ReleasePic(LUX_STREAM_JPEG, &picInfo);
        // if (0 != ret)
        // {
        //     IMP_LOG_ERR(TAG, " LUX_BASE_ReleasePic failed\n");
        //     return -1;
        // }

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
    }

    return ret;
}

/* rect to point, 同时将百分比坐标换算为实际sensor宽高坐标 */
static void LUX_IVS_Perm_Rect2Point(LUX_IVS_PERM_INPUT_POINT* pPoint, IVSPoint ivsPoint[4])
{
    ivsPoint[0].x = pPoint->x * SENSOR_WIDTH_SECOND / 100;
    ivsPoint[0].y = pPoint->y * SENSOR_HEIGHT_SECOND / 100;
    ivsPoint[1].x = (pPoint->x + pPoint->width) * SENSOR_WIDTH_SECOND / 100;
    ivsPoint[1].y = pPoint->y * SENSOR_HEIGHT_SECOND / 100;
    ivsPoint[2].x = (pPoint->x + pPoint->width) * SENSOR_WIDTH_SECOND / 100;
    ivsPoint[2].y = (pPoint->y + pPoint->height) * SENSOR_HEIGHT_SECOND / 100;
    ivsPoint[3].x = pPoint->x * SENSOR_WIDTH_SECOND / 100;
    ivsPoint[3].y = (pPoint->y + pPoint->height) * SENSOR_HEIGHT_SECOND / 100;
}



void LUX_IVS_Perm_Json2St_2(CHAR_X * p_alarm_zone, LUX_ALARM_AREA_ST* strAlarmAreaInfo)
{
    INT_X i = 0;
    cJSON * pJson  = NULL;
    cJSON * tmp = NULL;
    CHAR_X region[12] = {0};
    cJSON * cJSONRegion = NULL;

    pJson = cJSON_Parse((CHAR_X *)p_alarm_zone);
    if (NULL == pJson)
    {
        printf("%s %d step error\n",__FUNCTION__,__LINE__);
        //free(pResult);
        return;
    }

    tmp = cJSON_GetObjectItem(pJson, "num");
    if (NULL == tmp)
    {
        printf("%s %d step error\n",__FUNCTION__,__LINE__);
        cJSON_Delete(pJson);
        //free(pResult);
        return ;
    }

    memset(strAlarmAreaInfo, 0x00, sizeof(LUX_ALARM_AREA_ST));

    strAlarmAreaInfo->iPointNum = tmp->valueint;
    IMP_LOG_INFO(TAG,"%s %d points num[%d]\n",__FUNCTION__,__LINE__,strAlarmAreaInfo->iPointNum );

    if (strAlarmAreaInfo->iPointNum  > MAX_PERM_AREA_POINTS_NUM)
    {
        printf("#####error point num too big[%d]\n",strAlarmAreaInfo->iPointNum);
        cJSON_Delete(pJson);
        //free(pResult);
        return ;
    }

    for (i = 0; i < strAlarmAreaInfo->iPointNum ; i++)
    {
        snprintf(region, 12, "point%d",i);

        cJSONRegion = cJSON_GetObjectItem(pJson, region);
        if (NULL == cJSONRegion)
        {
            printf("#####[%s][%d]error\n",__FUNCTION__,__LINE__);
            cJSON_Delete(pJson);
            //free(pResult);
            return;
        }

        strAlarmAreaInfo->alarmPonits[i].pointX = cJSON_GetObjectItem(cJSONRegion, "x")->valueint;
        strAlarmAreaInfo->alarmPonits[i].pointY = cJSON_GetObjectItem(cJSONRegion, "y")->valueint;

        IMP_LOG_INFO(TAG,"[%s][%d][%d,%d]\n",__FUNCTION__,__LINE__,strAlarmAreaInfo->alarmPonits[i].pointX,strAlarmAreaInfo->alarmPonits[i].pointY);

    }

    cJSON_Delete(pJson);
    //free(pResult);
}


int LUX_IVS_Perm_Init()
{
    INT_X ret = 0;
    BOOL_X bStart = LUX_FALSE;
    BOOL_X sleepMode = LUX_FALSE;
    char permArea[248] = {0};
    LUX_IVS_ZONE_BASE_ST alarmZone;
    LUX_ALARM_AREA_ST alarmArea;
    IVSPoint convPoints[6];
    LUX_IVS_PERM_INPUT_POINT rect;
    INT_X i = 0;
    /* 获取报警回调函数 */
    LUX_Event_GetAlarmCbk(&g_permAttr.alarmCbk);

    /* 周界区域默认参数 */
    g_permAttr.param.sense = 2;
    g_permAttr.param.frameInfo.width = SENSOR_WIDTH_THIRD;
    g_permAttr.param.frameInfo.height = SENSOR_HEIGHT_THIRD;
    g_permAttr.param.isSkipFrame = false;

    g_permAttr.param.light = true;
    g_permAttr.param.level = 0;
    g_permAttr.param.timeon = 110;
    g_permAttr.param.timeoff = 310;

    g_permAttr.param.permcnt = 1;
    g_permAttr.param.perms[0].fun = 0;
    g_permAttr.param.perms[0].pcnt = 4;
    g_permAttr.param.perms[0].p = g_permAttr.pointTmp;

    g_permAttr.permType = LUX_ISP_PERM_SQUARE;
    /* 获取、配置多边形算法区域 */
    memset(&alarmArea,0,sizeof(alarmArea));    
    if(LUX_OK == LUX_INIParse_GetString(TUYA_INI_CONFIG_FILE_NAME, "function", "perm_area_2", permArea))
    {
        ret = LUX_OK;
        LUX_IVS_Perm_Json2St_2(permArea,&alarmArea);
    }
    if((LUX_OK == ret )&&(3 <= alarmArea.iPointNum))
    {
        g_permAttr.permType = LUX_ISP_PERM_POLYGON;
        printf("%s:%d Do polygon\n",__func__,__LINE__);
        g_permAttr.param.perms[0].pcnt = alarmArea.iPointNum;
        for(i = 0 ; i < alarmArea.iPointNum ; i++)
        {
            convPoints[i].x = alarmArea.alarmPonits[i].pointX * SENSOR_WIDTH_SECOND /100;
            convPoints[i].y = alarmArea.alarmPonits[i].pointY * SENSOR_HEIGHT_SECOND /100;
            g_permAttr.param.perms[0].p[i].x = alignment_down(convPoints[i].x, 2);
            g_permAttr.param.perms[0].p[i].y = alignment_down(convPoints[i].y , 2);
            IMP_LOG_INFO(TAG," perm_area(%d point): POINT%d(%d,%d)\n",g_permAttr.param.perms[0].pcnt,i,
                g_permAttr.param.perms[0].p[i].x,g_permAttr.param.perms[0].p[i].y);
        }
    }
    else
    {
        /* 获取、配置算法区域 */
        LUX_INIParse_GetString(TUYA_INI_CONFIG_FILE_NAME, "function", "perm_area", permArea);
        g_permAttr.permType = LUX_ISP_PERM_SQUARE;
        memset(&alarmZone, 0, sizeof(alarmZone));
        LUX_Ivs_alarmZone_Parse(permArea, &alarmZone);
        if (alarmZone.pointX < 0 || alarmZone.width <= 0
            || (alarmZone.pointX + alarmZone.width > SENSOR_WIDTH_SECOND)
            || (alarmZone.pointX + alarmZone.width <= 0))
        {
            alarmZone.pointX = 0;
            alarmZone.width  = SENSOR_WIDTH_SECOND;
        }
        if (alarmZone.pointY < 0 || alarmZone.height <= 0
            ||(alarmZone.pointY + alarmZone.height > SENSOR_HEIGHT_SECOND) 
            || (alarmZone.pointY + alarmZone.height <= 0))
        {
            alarmZone.pointY = 0;
            alarmZone.height = SENSOR_HEIGHT_SECOND;
        }
        printf("%s:%d Do square\n",__func__,__LINE__);
        memcpy(&rect, &alarmZone, sizeof(rect));
        LUX_IVS_Perm_Rect2Point(&rect, convPoints);
        g_permAttr.param.perms[0].pcnt = 4;
        g_permAttr.param.perms[0].p[0].x = alignment_down(convPoints[0].x, 2);
        g_permAttr.param.perms[0].p[0].y = alignment_down(convPoints[0].y, 2);
        g_permAttr.param.perms[0].p[1].x = alignment_down(convPoints[1].x, 2);
        g_permAttr.param.perms[0].p[1].y = alignment_down(convPoints[1].y, 2);
        g_permAttr.param.perms[0].p[2].x = alignment_down(convPoints[2].x, 2);
        g_permAttr.param.perms[0].p[2].y = alignment_down(convPoints[2].y, 2);
        g_permAttr.param.perms[0].p[3].x = alignment_down(convPoints[3].x, 2);
        g_permAttr.param.perms[0].p[3].y = alignment_down(convPoints[3].y, 2);
    }

    Thread_Mutex_Create(&g_permAttr.mutex);
    Task_CreateThread(LUX_IVS_Perm_Process, NULL);
    
    g_permAttr.bInit = LUX_TRUE;
    
    /* 隐私模式下不启用算法 */
    LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "sleep_mode", &sleepMode);
    if (LUX_FALSE == sleepMode)
    {
        /* 根据配置文件是否启动算法 */
        LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "perm_alarm_onoff", &bStart);
        if (bStart)
        {
            LUX_IVS_Perm_Start();
        }
    }

    return 0;
}

int LUX_IVS_Perm_Exit()
{
    g_permAttr.bInit = LUX_FALSE;

    return 0;
}

int LUX_IVS_Perm_Start()
{
    if (!g_permAttr.bInit || g_permAttr.bStart)
    {
        IMP_LOG_ERR(TAG, "LUX_IVS_Perm_Start failed1 bInit:%d bStart:%d\n", g_permAttr.bInit, g_permAttr.bStart);
        return 0;
    }
    int ret = 0;

    Thread_Mutex_Lock(&g_permAttr.mutex);
    do
    {
        g_permAttr.pInterface = PermInterfaceInit(&g_permAttr.param);
        if (g_permAttr.pInterface == NULL)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_CreateGroup(%d) failed\n", g_permAttr.ivsGrpNum);
            break;
        }
        ret = IMP_IVS_CreateChn(g_permAttr.ivsChnNum, g_permAttr.pInterface);
        if (ret < 0)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_CreateChn(%d) failed\n", g_permAttr.ivsChnNum);
            break;
        }
        ret = IMP_IVS_RegisterChn(g_permAttr.ivsGrpNum, g_permAttr.ivsChnNum);
        if (ret < 0)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_RegisterChn(%d, %d) failed\n", g_permAttr.ivsGrpNum, g_permAttr.ivsChnNum);
            break;
        }
        ret = IMP_IVS_StartRecvPic(g_permAttr.ivsChnNum);
        if (ret < 0)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_StartRecvPic(%d) failed\n", g_permAttr.ivsChnNum);
            break;
        }
        g_permAttr.bStart = LUX_TRUE;
    } while (0);
    Thread_Mutex_UnLock(&g_permAttr.mutex);

    return 0;
}

int LUX_IVS_Perm_Stop()
{
    if (!g_permAttr.bStart)
    {
        return 0;
    }
    int ret = -1;

    Thread_Mutex_Lock(&g_permAttr.mutex);
    do
    {
        ret = IMP_IVS_StopRecvPic(g_permAttr.ivsChnNum);
        if (ret < 0)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_StopRecvPic(%d) failed\n", g_permAttr.ivsChnNum);
            break;
        }
        sleep(1);

        ret = IMP_IVS_UnRegisterChn(g_permAttr.ivsChnNum);
        if (ret < 0)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_UnRegisterChn(%d) failed\n", g_permAttr.ivsChnNum);
            break;
        }

        ret = IMP_IVS_DestroyChn(g_permAttr.ivsChnNum);
        if (ret < 0)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_DestroyChn(%d) failed\n", g_permAttr.ivsChnNum);
            break;
        }

        PermInterfaceExit(g_permAttr.pInterface);
        g_permAttr.bStart = LUX_FALSE;
    } while (0);
    Thread_Mutex_UnLock(&g_permAttr.mutex);

    return 0;
}

/* 配置周界区域 */
int LUX_IVS_Perm_ConfigArea(LUX_IVS_PERM_INPUT_POINT *pPoint)
{
    if (NULL == pPoint)
    {
        printf("%s:%d param error\n", __func__, __LINE__);
        return -1;
    }
    int ret = 0;
    IVSPoint convPoints[4];

    /* 配置参数先停止算法，再重新打开 */
    LUX_IVS_Perm_Stop();

    Thread_Mutex_Lock(&g_permAttr.mutex);
    do
    {
        LUX_IVS_Perm_Rect2Point(pPoint, convPoints);
        g_permAttr.param.perms[0].pcnt = 4;
        g_permAttr.param.perms[0].p[0].x = alignment_down(convPoints[0].x, 2);
        g_permAttr.param.perms[0].p[0].y = alignment_down(convPoints[0].y, 2);
        g_permAttr.param.perms[0].p[1].x = alignment_down(convPoints[1].x, 2);
        g_permAttr.param.perms[0].p[1].y = alignment_down(convPoints[1].y, 2);
        g_permAttr.param.perms[0].p[2].x = alignment_down(convPoints[2].x, 2);
        g_permAttr.param.perms[0].p[2].y = alignment_down(convPoints[2].y, 2);
        g_permAttr.param.perms[0].p[3].x = alignment_down(convPoints[3].x, 2);
        g_permAttr.param.perms[0].p[3].y = alignment_down(convPoints[3].y, 2);

        IMP_LOG_INFO(TAG, "IMP_IVS_SetParam perm_area: XY(%d,%d) WH(%d,%d) --> 0(%d,%d) 1(%d,%d) 2(%d,%d) 3(%d,%d)\n",
            pPoint->x, pPoint->y, pPoint->width, pPoint->height,
            g_permAttr.param.perms[0].p[0].x, g_permAttr.param.perms[0].p[0].y, 
            g_permAttr.param.perms[0].p[1].x, g_permAttr.param.perms[0].p[1].y, 
            g_permAttr.param.perms[0].p[2].x, g_permAttr.param.perms[0].p[2].y, 
            g_permAttr.param.perms[0].p[3].x, g_permAttr.param.perms[0].p[3].y);
    } while(0);
    Thread_Mutex_UnLock(&g_permAttr.mutex);
    
    LUX_IVS_Perm_Start();
    
    return ret;
}


/* 配置周界区域 */
int LUX_IVS_Perm_ConfigArea2(LUX_ALARM_AREA_ST *alarmArea)
{
    if (NULL == alarmArea)
    {
        printf("%s:%d alarmArea error\n", __func__, __LINE__);
        return -1;
    }
    int ret = 0;
    IVSPoint convPoints[6];
    int i = 0;
    if(3 > alarmArea->iPointNum )
    {
        IMP_LOG_ERR(TAG, "[ERROR] alarmArea Point Number atleast 3 !!!\n");
        return -1;
    }

    /* 配置参数先停止算法，再重新打开 */
    LUX_IVS_Perm_Stop();

    Thread_Mutex_Lock(&g_permAttr.mutex);
    do
    {
        g_permAttr.param.perms[0].pcnt = alarmArea->iPointNum;
        for(i = 0 ; i < alarmArea->iPointNum ; i++)
        {
            convPoints[i].x = alarmArea->alarmPonits[i].pointX * SENSOR_WIDTH_SECOND /100;
            convPoints[i].y = alarmArea->alarmPonits[i].pointY * SENSOR_HEIGHT_SECOND /100;
            g_permAttr.param.perms[0].p[i].x = alignment_down(convPoints[i].x, 2);
            g_permAttr.param.perms[0].p[i].y = alignment_down(convPoints[i].y , 2);
            IMP_LOG_INFO(TAG," perm_area(%d point): POINT%d(%d,%d)",g_permAttr.param.perms[0].pcnt,i,
                g_permAttr.param.perms[0].p[i].x,g_permAttr.param.perms[0].p[i].y);
        }
    } while(0);
    Thread_Mutex_UnLock(&g_permAttr.mutex);
    
    LUX_IVS_Perm_Start();
    
    return ret;
}

void LUX_IVS_Perm_Process()
{
    int ret = 0;
    perm_param_output_t *result = NULL;

    while (1)
    {
        if (!g_permAttr.bInit || !g_permAttr.bStart)
        {
            usleep(100*1000);
            continue;
        }
        Thread_Mutex_Lock(&g_permAttr.mutex);
        do
        {
            ret = IMP_IVS_PollingResult(g_permAttr.ivsChnNum, IMP_IVS_DEFAULT_TIMEOUTMS);
            if (ret < 0)
            {
                IMP_LOG_ERR(TAG, "IMP_IVS_PollingResult(%d, %d) failed\n", 0, IMP_IVS_DEFAULT_TIMEOUTMS);
                break;
            }
            ret = IMP_IVS_GetResult(g_permAttr.ivsChnNum, (void **)&result);
            if (ret < 0)
            {
                IMP_LOG_ERR(TAG, "IMP_IVS_GetResult(%d) failed\n", 0);
                break;
            }

            if (result->ret)
            {
                LUX_IVS_PermAlarm();
            }

            ret = IMP_IVS_ReleaseResult(g_permAttr.ivsChnNum, (void *)result);
            if (ret < 0)
            {
                IMP_LOG_ERR(TAG, "IMP_IVS_ReleaseResult(%d) failed\n", 0);
                break;
            }
        } while (0);
        Thread_Mutex_UnLock(&g_permAttr.mutex);

        usleep(30*1000);
    }

    return ;
}