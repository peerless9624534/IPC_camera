/*
 * lux_ivs.c
 *
 * 基于Ingenic平台封装的智能模块，包括移动侦测、人脸检测、人形侦测等
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/prctl.h>

#include <imp_log.h>
#include <imp_common.h>
#include <imp_system.h>
#include <imp_framesource.h>
#include <imp_encoder.h>
#include <imp_ivs.h>
#include <ivs_common.h>

#include <lux_ivs.h>
#include <comm_def.h>
#include <comm_func.h>
#include <comm_error_code.h>
#include <lux_iniparse.h>
#include <lux_event.h>
#include "lux_smileDet.h"
#include "lux_coverFace.h"
#include "lux_perm.h"
#include "lux_osd.h"

 /*TODO:调试编译用, 合并代码时请删除*/
 //#define CONFIG_PTZ_IPC

#define TAG "lux_ivs"

#define PERSON_DETCTION        0

/*人形侦测周界坐标*/
#define  PERSON_DET_PERMCNT     4

OSI_SEM g_ivsAlgSem;

LUX_IVS_ATTR_ST g_ivsAttr =
{
    .iaacInfo =
    {
        .license_path = INGENIT_ALG_FILE_NAME,
        .sn = "fc99a05479571355d2b9da163308fa07",
    },
    .rptEventTI =
    {
        .timeInterval = 1 * 60,
    },

    .bindAttr =
    {
        .grpId = 0,
#ifdef CONFIG_PTZ_IPC        
        .chnNum = {0, 1},
#else
        .chnNum = {0, 1},
#endif/*CONFIG_PTZ_IPC*/ 
        .bindSrcChan = 1,
        .bindSrcModule = LUX_MODULS_FSRC,
        .bindDstChan = 0,                   /*绑定的channal相当于君正的group*/
        .bindDstModule = LUX_MODULS_IVS,
    },
    /*0通道移动侦测, 1通道人形侦测*/
.ivsMovAttr =
{
    .param =
    {
        .isSkipFrame = LUX_TRUE,
        .frameInfo.width = SENSOR_WIDTH_SECOND,
        .frameInfo.height = SENSOR_HEIGHT_SECOND,
        .cntRoi = LUX_IVS_REGION_NUM,
        .det_h = 40,
        .det_w = 40,
        .level = 0.6,
        .timeon = 40,
        .timeoff = 100,
    },
    .sensParam = 1600 * 35,
},
.personDetAttr =
{
    .param =
    {
        .frameInfo.width = SENSOR_WIDTH_SECOND,
        .frameInfo.height = SENSOR_HEIGHT_SECOND,
        .skip_num = 3,
        .ptime = LUX_FALSE,
        .sense = 5,
        .detdist = 3,
        .enable_move = LUX_TRUE,
        .enable_perm = LUX_FALSE,
        .permcnt = LUX_IVS_REGION_NUM,
        .perms[0].pcnt = PERSON_DET_PERMCNT,
    }
},
#ifdef CONFIG_PTZ_IPC
    .personTrackAttr =
    {
        .param =
        {
            .frameInfo.width = SENSOR_WIDTH_SECOND,
            .frameInfo.height = SENSOR_HEIGHT_SECOND,
            .stable_time_out = 4,
            .move_sense = 2,
            .detdist = 3,
            .obj_min_width = 40,
            .obj_min_height = 40,
            .move_thresh_denoise = 20,
            .add_smooth = 0,
            .conf_thresh = 0.8,
            .conf_lower = 0.2,
            .frozen_in_boundary = 5,
            .boundary_ratio = 0.25,
            .mode = 1,
            .enable_move = LUX_FALSE,
            .area_ratio = 0.2,
            .exp_x = 1.0,
        },

    },

#endif

};

static IVSPoint g_pd_perm[PERSON_DET_PERMCNT];


//TODO:编译的时候请开启宏定义
#ifdef CONFIG_PTZ_IPC

/**
 * @description: 设置人形追踪的属性
 * @param  [in] param 属性
 */
VOID_X LUX_Ivs_SetPTAttr(person_tracker_param_input_t* pPtParam)
{
    Thread_Mutex_Lock(&g_ivsAttr.paramMux);
    memcpy(pPtParam, &g_ivsAttr.personTrackAttr.param, sizeof(persondet_param_input_t));
    Thread_Mutex_UnLock(&g_ivsAttr.paramMux);

    return;
}

/**
 * @description: 设置人形追踪结果buf
 * @param  [out] pOutPTResultBuf 人形追踪输出结果 mm
 */
static INT_X LUX_Ivs_GetResultBuf(LUX_IVS_PTRESULT_QUE_ST** pOutPTResultBuf, person_tracker_param_output_t* pPTResult)
{
    if (NULL == pPTResult)
    {
        IMP_LOG_ERR(TAG, "LUX_Ivs_GetResultBuf failed, empty pointer\n");
        return LUX_PARM_NULL_PTR;
    }

    INT_X i = 0;
    LUX_IVS_PTRESULT_QUE_ST* PTResultBuf = g_ivsAttr.personTrackAttr.ivsPTResultQue;

    for (i = 0; i < LUX_PTRESULT_QUE_LEN; i++)
    {
        if (LUX_TRUE != PTResultBuf[i].bUsed)
        {
            memcpy(&PTResultBuf[i], pPTResult, sizeof(person_tracker_param_output_t));
            *pOutPTResultBuf = &PTResultBuf[i];
            PTResultBuf[i].bUsed = LUX_TRUE;

            return LUX_OK;
        }
    }

    IMP_LOG_ERR(TAG, "LUX_Ivs_GetResultBuf failed, The buffer is full\n");
    return LUX_ERR;

}

/**
 * @description: 人形追踪的结果释放函数
 * @param  [in] pParam 传入的参数
 * @return 0 成功，非零失败，返回错误码
 */
static VOID_X LUX_Ivs_PTRelease(PVOID_X pParam)
{
    LUX_IVS_PTRESULT_QUE_ST* pEle = (LUX_IVS_PTRESULT_QUE_ST*)pParam;
    pEle->bUsed = LUX_FALSE;

    return;
}

/**
 * @description: 发送人形追踪的消息
 * @param  [in] idx  人形追踪的结果数组下标
 * @param  [in] pPTResult 人形追踪的结果
 * @return 0 成功，非零失败，返回错误码
 */
static INT_X LUX_Ivs_SendPTMsg(LUX_IVS_PTRESULT_QUE_ST* pPTResult)
{
    if (NULL == pPTResult)
    {
        IMP_LOG_ERR(TAG, "LUX_Ivs_SendPTMsg failed, empty pointer\n");
        return LUX_PARM_NULL_PTR;
    }

    INT_X ret = LUX_ERR;
    LUX_BASE_IVSDATA_ST popResult;
    popResult.type = LUX_BASE_PERSONTRACK_QUE_EN;
    popResult.IVSResult = (PVOID_X)pPTResult;
    popResult.pCallBackInfo = (PVOID_X)pPTResult;
    popResult.pCallBackFunc = LUX_Ivs_PTRelease;

    ret = PushQueue(&g_ivs_que, &popResult);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " PopQueue failed\n");
        return LUX_ERR;
    }

    return LUX_OK;
}

#endif



/**
 * @description: ivs算法认证初始化
 * @return 0成功, 非0错误
 * @attention 需要连接网路后初始化
 */
INT_X LUX_IVS_Iaac_Init()
{
    INT_X ret = LUX_ERR;

    ret = IAAC_Init(&g_ivsAttr.iaacInfo);
    if (ret)
    {
        IMP_LOG_ERR(TAG, "IAAC_Init error!\n");
        return LUX_ERR;
    }

    return LUX_OK;
}

/**
 * @description: ivs事件报警间隔倒计时
 * @return 倒计时未到返回FALSE, 到了返回 TURE,
 */
BOOL_X LUX_IVS_GetReportEvent_Countdown()
{
    INT_X ret = LUX_FALSE;

    Thread_Mutex_Lock(&g_ivsAttr.paramMux);
    g_ivsAttr.rptEventTI.curTime = getTime_getMs();

    if (g_ivsAttr.rptEventTI.curTime - g_ivsAttr.rptEventTI.lastTime > g_ivsAttr.rptEventTI.timeInterval)
    {
        g_ivsAttr.rptEventTI.lastTime = g_ivsAttr.rptEventTI.curTime;
        ret = LUX_TRUE;
    }
    Thread_Mutex_UnLock(&g_ivsAttr.paramMux);

    return ret;
}

/**
 * @description: 发送移动侦测的消息
 * @param  [in] pMovResult 移动侦测的结果
 * @return 0 成功，非零失败，返回错误码
 */
static INT_X LUX_Ivs_SendMovMsg(move_param_output_t* pMovResult)
{
    if (NULL == pMovResult)
    {
        IMP_LOG_ERR(TAG, "LUX_Ivs_SendMovMsg failed\n");
        return LUX_PARM_NULL_PTR;
    }

    INT_X ret = LUX_ERR;
    LUX_BASE_IVSDATA_ST popResult;

    popResult.type = LUX_BASE_MOVDET_QUE_EN;
    popResult.ivsStart[LUX_BASE_MOVDET_QUE_EN] = g_ivsAttr.bStart[LUX_BASE_MOVDET_QUE_EN];
    popResult.retRoi = pMovResult->ret;
    popResult.detcount = pMovResult->detcount;
    memcpy(&popResult.rectInfo, &pMovResult->g_res, sizeof(move_g));

    ret = PushQueue(&g_ivs_que, &popResult);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " PopQueue failed\n");
        return LUX_ERR;
    }

    return LUX_OK;
}

/**
 * @description: 发送人形侦测的消息
 * @param  [in] pPDResult 移动侦测的结果
 * @return 0 成功，非零失败，返回错误码
 */
static INT_X LUX_Ivs_SendPDMsg(persondet_param_output_t* pPDResult)
{
    if (NULL == pPDResult)
    {
        IMP_LOG_ERR(TAG, "LUX_Ivs_SendMovMsg failed\n");
        return LUX_PARM_NULL_PTR;
    }

    INT_X ret = LUX_ERR;
    LUX_BASE_IVSDATA_ST popResult;
    popResult.type = LUX_BASE_PERSONDET_QUE_EN;
    popResult.ivsStart[LUX_IVS_PERSON_DET_EN] = g_ivsAttr.bStart[LUX_IVS_PERSON_DET_EN];
    popResult.rectcnt = pPDResult->count;
    memcpy(popResult.person, pPDResult->person, sizeof(person_info) * pPDResult->count);

    ret = PushQueue(&g_ivs_que, &popResult);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " PopQueue failed\n");
        return LUX_ERR;
    }

    return LUX_OK;
}


/**
 * @description: 报警区域开关
 * @param [in] bOpen IVS处理结果
 * @return 0 成功，非零失败
 */
INT_X LUX_IVS_AlarmZone_Open(BOOL_X bOpen)
{
    INT_X ret = LUX_OK;
    persondet_param_input_t PDParam;

    Thread_Mutex_Lock(&g_ivsAttr.paramMux);
    do {
        g_ivsAttr.alarmZone.bOpen = bOpen;

        /*人形侦测区域报警开关*/
        g_ivsAttr.personDetAttr.param.enable_perm = bOpen;

        ret = IMP_IVS_GetParam(LUX_IVS_PERSON_DET_EN, (PVOID_X)&PDParam);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_GetParam failed, chn(%d) . \n", LUX_IVS_PERSON_DET_EN);
            ret = LUX_IVS_GET_PARAM_ERR;
            break;
        }

        PDParam.enable_perm = g_ivsAttr.personDetAttr.param.enable_perm;

        ret = IMP_IVS_SetParam(LUX_IVS_PERSON_DET_EN, (PVOID_X)&PDParam);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_SetParam failed, chn(%d) . \n", LUX_IVS_PERSON_DET_EN);
            ret = LUX_IVS_GET_PARAM_ERR;
            break;
        }
    } while (0);
    Thread_Mutex_UnLock(&g_ivsAttr.paramMux);

    return ret;
}


/**
 * @description: 获取报警区域开关
 * @return 0 成功，非零失败
 */
BOOL_X LUX_IVS_GetAlarmZone_OnOff()
{
    BOOL_X OnOff = LUX_FALSE;

    Thread_Mutex_Lock(&g_ivsAttr.paramMux);
    OnOff = g_ivsAttr.alarmZone.bOpen;
    Thread_Mutex_UnLock(&g_ivsAttr.paramMux);

    return OnOff;
}


/**
 * @description: 判断移动侦测的结果是否在感兴趣区域内
 * @param [in] IvsResult IVS处理结果
 * @return 在区域内返回ture，否则返回false
 */
BOOL_X LUX_IVS_MovDetRegion_Process(LUX_BASE_IVSDATA_ST* MovResult)
{
    INT_X i = 0;
    INT_X resultArea = 0;


    /*开启感兴趣区域*/
    if (LUX_IVS_GetAlarmZone_OnOff())
    {
        for (i = 0; i < MovResult->detcount; i++)
        {
            if (MovResult->rectInfo.det_rects[i].x >= g_ivsAttr.alarmZone.movDet.ul.x &&
               MovResult->rectInfo.det_rects[i].x < g_ivsAttr.alarmZone.movDet.br.x &&
               MovResult->rectInfo.det_rects[i].y >= g_ivsAttr.alarmZone.movDet.ul.y &&
               MovResult->rectInfo.det_rects[i].y < g_ivsAttr.alarmZone.movDet.br.y)
            {
                /*计算是否达到触发报警的面积(灵敏度)*/
                resultArea += MovResult->rectInfo.area[i];
                if (resultArea >= LUX_Ivs_GetMovSensitive())
                {
                    return LUX_TRUE;
                }
            }
        }
    }
    else /*非感兴趣区域*/
    {
        for (i = 0; i < MovResult->detcount; i++)
        {
            resultArea += MovResult->rectInfo.area[i];
            if (resultArea >= LUX_Ivs_GetMovSensitive())
            {
                return LUX_TRUE;
            }
        }
    }

    return LUX_FALSE;
}


/**
 * @description: 获取移动侦测灵敏度
 * @return 返回灵敏度
 */
INT_X LUX_Ivs_GetMovSensitive()
{
    INT_X sns;

    Thread_Mutex_Lock(&g_ivsAttr.paramMux);
    sns = g_ivsAttr.ivsMovAttr.sensParam;
    Thread_Mutex_UnLock(&g_ivsAttr.paramMux);

    return sns;
}


/**
 * @description: 设置移动侦测算法灵敏度
 * @param [in] sns   灵敏度
 * @return 0 成功，非零失败，返回错误码
 */
static INT_X LUX_Ivs_SetMovSense(LUX_IVS_SENSITIVE_EN sns)
{
    INT_X ret = LUX_OK;
    move_param_input_t MovParam;

    memset(&MovParam, 0, sizeof(move_param_input_t));

    Thread_Mutex_Lock(&g_ivsAttr.paramMux);
    do {
        switch (sns)
        {
        case LUX_IVS_SENSITIVE_LOW_EN:
            g_ivsAttr.ivsMovAttr.sensParam = 1600 * 25;
            break;
        case LUX_IVS_SENSITIVE_MID_EN:
            g_ivsAttr.ivsMovAttr.sensParam = 1600 * 10;
            break;
        case LUX_IVS_SENSITIVE_HIG_EN:
            g_ivsAttr.ivsMovAttr.sensParam = 1600 * 5;
            break;
        default:
            IMP_LOG_ERR(TAG, "LUX_Ivs_SetMovSense failed, chn(%d), not suppport param,\
                            it will be set default param [%d]  \n", LUX_IVS_MOVE_DET_EN, LUX_IVS_SENSITIVE_LOW_EN);
            g_ivsAttr.ivsMovAttr.sensParam = 1600 * 25;
            break;
        }

        ret = IMP_IVS_GetParam(LUX_IVS_MOVE_DET_EN, (PVOID_X)&MovParam);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_GetParam chn(%d) failed. \n", LUX_IVS_MOVE_DET_EN);
            ret = LUX_IVS_GET_PARAM_ERR;
            break;
        }

        if (sns < LUX_IVS_SENSITIVE_LOW_EN || sns >= LUX_IVS_SENSITIVE_BUTTON)
        {
            IMP_LOG_ERR(TAG, "LUX_IVS_SetSensitive chn(%d) failed,not suppport param,\
                            it will be set default param [%d]  \n", LUX_IVS_MOVE_DET_EN, LUX_IVS_SENSITIVE_LOW_EN);
            MovParam.sense = LUX_IVS_SENSITIVE_LOW_EN;
        }
        else
        {
            MovParam.sense = sns;
        }

        ret = IMP_IVS_SetParam(LUX_IVS_MOVE_DET_EN, (PVOID_X)&MovParam);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_SetParam failed, chn(%d). \n", LUX_IVS_MOVE_DET_EN);
            ret = LUX_IVS_SET_PARAM_ERR;
            break;
        }

    } while (0);
    Thread_Mutex_UnLock(&g_ivsAttr.paramMux);

    return ret;
}

/**
 * @description: 设置人形侦测算法灵敏度
 * @param [in] sns   灵敏度
 * @return 0 成功，非零失败，返回错误码
 */
static INT_X LUX_Ivs_SetPDSense(sns)
{
    INT_X ret = LUX_OK;
    INT_X PDSense = 5;
    persondet_param_input_t PDParam;

    memset(&PDParam, 0, sizeof(persondet_param_input_t));

    Thread_Mutex_Lock(&g_ivsAttr.paramMux);
    do {
        switch (sns)
        {
        case LUX_IVS_SENSITIVE_LOW_EN:
            PDSense = 1;
            break;
        case LUX_IVS_SENSITIVE_MID_EN:
            PDSense = 3;
            break;
        case LUX_IVS_SENSITIVE_HIG_EN:
            PDSense = 4;
            break;
        default:
            IMP_LOG_ERR(TAG, "LUX_Ivs_SetPDSense failed, chn(%d), not suppport param,\
                            it will be set default param [%d]  \n", LUX_IVS_MOVE_DET_EN, LUX_IVS_SENSITIVE_LOW_EN);
            PDSense = 1;
            break;
        }

        ret = IMP_IVS_GetParam(LUX_IVS_PERSON_DET_EN, (PVOID_X)&PDParam);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_GetParam failed, chn(%d) . \n", LUX_IVS_PERSON_DET_EN);
            ret = LUX_IVS_GET_PARAM_ERR;
            break;
        }

        PDParam.sense = PDSense;

        ret = IMP_IVS_SetParam(LUX_IVS_PERSON_DET_EN, (PVOID_X)&PDParam);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_SetParam chn(%d) failed. \n", LUX_IVS_PERSON_DET_EN);
            ret = LUX_IVS_SET_PARAM_ERR;
            break;
        }
    } while (0);
    Thread_Mutex_UnLock(&g_ivsAttr.paramMux);

    return ret;
}

/**
 * @description: 设置ivs通道算法灵敏度
 * @param [in] chnIdx  通道号
 * @param [in] sns     灵敏度
 * @return 0 成功，非零失败，返回错误码
 */
INT_X LUX_IVS_SetSensitive(LUX_IVS_ALGO_EN chnIdx, LUX_IVS_SENSITIVE_EN sns)
{

    INT_X  ret = LUX_ERR;

    if (LUX_IVS_MOVE_DET_EN == chnIdx)
    {
        ret = LUX_Ivs_SetMovSense(sns);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "LUX_Ivs_SetMovSense failed, chn(%d), return error [0x%x]. \n", chnIdx, ret);
            return LUX_ERR;
        }
    }
    else if (LUX_IVS_PERSON_DET_EN == chnIdx)
    {
        ret = LUX_Ivs_SetPDSense(sns);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "LUX_Ivs_SetPDSense failed, chn(%d), return error [0x%x]. \n", chnIdx, ret);
            return LUX_ERR;
        }
    }

    return LUX_OK;
}


/**
 * @description: 开启ivs通道
 * @param [in] chnIdx  通道号
 * @return 0 成功，非零失败，返回错误码
 */
static INT_X LUX_Ivs_StartChn(INT_X chnIdx)
{
    INT_X ret = LUX_OK;

    if (LUX_TRUE == g_ivsAttr.bIvsInit[chnIdx])
    {
        IMP_LOG_ERR(TAG, "Warnning, LUX_Ivs_StartChn(%d) already Started\n", chnIdx);
        return LUX_OK;
    }

    Thread_Mutex_Lock(&g_ivsAttr.mux[chnIdx]);
    do {
        ret = IMP_IVS_StartRecvPic(g_ivsAttr.bindAttr.chnNum[chnIdx]);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_StartRecvPic(%d) failed\n", g_ivsAttr.bindAttr.chnNum[chnIdx]);
            break;
        }

        g_ivsAttr.bIvsInit[chnIdx] = LUX_TRUE;
    } while (0);
    Thread_Mutex_UnLock(&g_ivsAttr.mux[chnIdx]);

    return ret;
}

/**
 * @description: 关闭ivs通道
 * @param [in] chnIdx  通道号
 * @return 0 成功，非零失败，返回错误码
 */
static INT_X LUX_Ivs_StopChn(INT_X chnIdx)
{
    INT_X ret = LUX_OK;

    if (LUX_FALSE == g_ivsAttr.bIvsInit[chnIdx])
    {
        IMP_LOG_ERR(TAG, "Warnning, LUX_Ivs_StopChn(%d) already Stopped\n", chnIdx);
        return LUX_OK;
    }

    Thread_Mutex_Lock(&g_ivsAttr.mux[chnIdx]);
    do {
        ret = IMP_IVS_StopRecvPic(g_ivsAttr.bindAttr.chnNum[chnIdx]);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_StopRecvPic(%d) failed\n", g_ivsAttr.bindAttr.chnNum[chnIdx]);
            break;
        }

        g_ivsAttr.bIvsInit[chnIdx] = LUX_FALSE;
    } while (0);
    Thread_Mutex_UnLock(&g_ivsAttr.mux[chnIdx]);

    return ret;
}


/**
 * @description: 开启IVS侦测
 * @param [in] algo  算法类型
 * @return 0 成功，非零失败，返回错误码
 */
INT_X LUX_IVS_StartAlgo(LUX_IVS_ALGO_EN algo)
{
    INT_X ret = LUX_ERR;
    if (LUX_IVS_MOVE_DET_EN == algo)
    {
        IMP_LOG_INFO(TAG, "LUX_Ivs_StartChn(%d) \n", algo);
        ret = LUX_Ivs_StartChn(algo);
        if (ret < 0)
        {
            IMP_LOG_ERR(TAG, "LUX_Ivs_StartChn(%d) failed\n", algo);
            return LUX_IVS_START_RECIVE_PIC_ERR;
        }
    }
    g_ivsAttr.bStart[algo] = LUX_TRUE;

    return LUX_OK;
}

/**
 * @description: 关闭IVS侦测
 * @param [in] algo  算法类型
 * @return 0 成功，非零失败，返回错误码
 */
INT_X LUX_IVS_StopAlgo(LUX_IVS_ALGO_EN algo)
{
    INT_X ret = LUX_ERR;
    if (LUX_IVS_MOVE_DET_EN == algo)
    {
        IMP_LOG_INFO(TAG, "LUX_Ivs_StopChn(%d) \n", algo);
        ret = LUX_Ivs_StopChn(algo);
        if (ret < 0)
        {
            IMP_LOG_ERR(TAG, "LUX_Ivs_StopChn(%d) failed\n", algo);
            return LUX_IVS_STOP_RECIVE_PIC_ERR;
        }
    }
    g_ivsAttr.bStart[algo] = LUX_FALSE;
    return LUX_OK;
}

/**
 * @description: ivs事件报警间隔
 * @param  [in] ts 时间间隔,单位s
 * @return 0 成功，非零失败，返回错误码
 */
VOID_X LUX_IVS_SetTimeInterval(LUX_IVS_TIMEINTERVAL_EN ts)
{
    if (ts < LUX_IVS_TIMEINTERVAL_1MIN || ts >= LUX_IVS_TIME_INTERVAL_BUTTON)
    {
        IMP_LOG_ERR(TAG, "LUX_IVS_SetTimeInterval failed, not support param [%d]\n", ts);
        g_ivsAttr.rptEventTI.timeInterval = 1 * 60 * 1000;
    }

    Thread_Mutex_Lock(&g_ivsAttr.paramMux);
    switch (ts)
    {
    case LUX_IVS_TIMEINTERVAL_1MIN:
        g_ivsAttr.rptEventTI.timeInterval = 1 * 60 * 1000;
        break;
    case LUX_IVS_TIMEINTERVAL_3MIN:
        g_ivsAttr.rptEventTI.timeInterval = 3 * 60 * 1000;
        break;
    case LUX_IVS_TIMEINTERVAL_5MIN:
        g_ivsAttr.rptEventTI.timeInterval = 5 * 60 * 1000;
        break;
    case LUX_IVS_TIME_INTERVAL_BUTTON:
    default:
        IMP_LOG_ERR(TAG, "LUX_IVS_SetTimeInterval failed, not support param [%d]\n", ts);
        g_ivsAttr.rptEventTI.timeInterval = 1 * 60 * 1000;
        break;
    }
    Thread_Mutex_UnLock(&g_ivsAttr.paramMux);

    return;
}

/**
 * @description: 设置移动侦测报警区域
 * @param  [in] zoneInfo  坐标信息
 */
static INT_X LUX_Ivs_SetMovDet_Region(LUX_IVS_ZONE_BASE_ST* zoneInfo)
{
    if (NULL == zoneInfo)
    {
        IMP_LOG_ERR(TAG, "LUX_Ivs_SetMovDet_Region failed, empty point.\n", LUX_IVS_PERSON_DET_EN);
        return LUX_PARM_NULL_PTR;
    }

    /*注意:转换后的x,y 不是坐标,而是行列;所以转换后x,y的值相互颠倒*/
    Thread_Mutex_Lock(&g_ivsAttr.paramMux);
    g_ivsAttr.alarmZone.movDet.ul.x = (INT_X)(zoneInfo->pointY * (g_ivsAttr.ivsMovAttr.param.frameInfo.height / 100.0)) /
        g_ivsAttr.ivsMovAttr.param.det_h;
    g_ivsAttr.alarmZone.movDet.ul.y = (INT_X)(zoneInfo->pointX * (g_ivsAttr.ivsMovAttr.param.frameInfo.width / 100.0)) /
        g_ivsAttr.ivsMovAttr.param.det_w;
    g_ivsAttr.alarmZone.movDet.br.x = (INT_X)((zoneInfo->pointY + zoneInfo->height) * (g_ivsAttr.ivsMovAttr.param.frameInfo.height / 100.0)) /
        g_ivsAttr.ivsMovAttr.param.det_h;
    g_ivsAttr.alarmZone.movDet.br.y = (INT_X)((zoneInfo->pointX + zoneInfo->width) * (g_ivsAttr.ivsMovAttr.param.frameInfo.width / 100.0)) /
        g_ivsAttr.ivsMovAttr.param.det_w;
    Thread_Mutex_UnLock(&g_ivsAttr.paramMux);

    IMP_LOG_INFO(TAG, "alarmZone movDet ul.x[%d], ul.y[%d], br.x[%d], y[%d]\n", g_ivsAttr.alarmZone.movDet.ul.x,
                 g_ivsAttr.alarmZone.movDet.ul.y, g_ivsAttr.alarmZone.movDet.br.x, g_ivsAttr.alarmZone.movDet.br.y);

    return LUX_OK;
}


/**
 * @description: 设置人形侦测报警区域
 * @param  [in] zoneInfo  坐标信息
 * @return 0 成功，非零失败，返回错误码
 */
static INT_X LUX_Ivs_SetPDDet_Region(LUX_IVS_ZONE_BASE_ST* zoneInfo)
{
    if (NULL == zoneInfo)
    {
        IMP_LOG_ERR(TAG, "LUX_Ivs_SetPDDet_Region failed, empty point.\n");
        return LUX_PARM_NULL_PTR;
    }

    INT_X ret = LUX_OK;
    persondet_param_input_t PDParam;
    DOUBLE_X widthRate = g_ivsAttr.personDetAttr.param.frameInfo.width / 100.0;
    DOUBLE_X heightRate = g_ivsAttr.personDetAttr.param.frameInfo.height / 100.0;

    Thread_Mutex_Lock(&g_ivsAttr.paramMux);
    do {
        /*左上角为(0,0)p0点, 顺时针排列顺序, 依次为p0,p1,p2,p3*/
        g_ivsAttr.personDetAttr.param.perms[0].p[0].x = alignment_down(((INT_X)(zoneInfo->pointX * widthRate)), 4);
        g_ivsAttr.personDetAttr.param.perms[0].p[0].y = alignment_down(((INT_X)(zoneInfo->pointY * heightRate)), 4);
        g_ivsAttr.personDetAttr.param.perms[0].p[1].x = alignment_down(((INT_X)((zoneInfo->pointX + zoneInfo->width) * widthRate) - 1), 4);
        g_ivsAttr.personDetAttr.param.perms[0].p[1].y = alignment_down(((INT_X)(zoneInfo->pointY * heightRate)), 4);
        g_ivsAttr.personDetAttr.param.perms[0].p[2].x = alignment_down(((INT_X)((zoneInfo->pointX + zoneInfo->width) * widthRate) - 1), 4);
        g_ivsAttr.personDetAttr.param.perms[0].p[2].y = alignment_down(((INT_X)((zoneInfo->pointY + zoneInfo->height) * heightRate) - 1), 4);
        g_ivsAttr.personDetAttr.param.perms[0].p[3].x = alignment_down(((INT_X)(zoneInfo->pointX * widthRate)), 4);
        g_ivsAttr.personDetAttr.param.perms[0].p[3].y = alignment_down(((INT_X)((zoneInfo->pointY + zoneInfo->height) * heightRate) - 1), 4);

        memset(&PDParam, 0, sizeof(persondet_param_input_t));

        ret = IMP_IVS_GetParam(LUX_IVS_PERSON_DET_EN, (PVOID_X)&PDParam);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_GetParam failed, chn(%d) . \n", LUX_IVS_PERSON_DET_EN);
            ret = LUX_IVS_GET_PARAM_ERR;
            break;
        }

        PDParam.enable_perm = g_ivsAttr.alarmZone.bOpen;
        PDParam.perms[0].p[0].x = g_ivsAttr.personDetAttr.param.perms[0].p[0].x;
        PDParam.perms[0].p[0].y = g_ivsAttr.personDetAttr.param.perms[0].p[0].y;
        PDParam.perms[0].p[1].x = g_ivsAttr.personDetAttr.param.perms[0].p[1].x;
        PDParam.perms[0].p[1].y = g_ivsAttr.personDetAttr.param.perms[0].p[1].y;
        PDParam.perms[0].p[2].x = g_ivsAttr.personDetAttr.param.perms[0].p[2].x;
        PDParam.perms[0].p[2].y = g_ivsAttr.personDetAttr.param.perms[0].p[2].y;
        PDParam.perms[0].p[3].x = g_ivsAttr.personDetAttr.param.perms[0].p[3].x;
        PDParam.perms[0].p[3].y = g_ivsAttr.personDetAttr.param.perms[0].p[3].y;

        ret = IMP_IVS_SetParam(LUX_IVS_PERSON_DET_EN, (PVOID_X)&PDParam);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_SetParam failed, chn(%d) . \n", LUX_IVS_PERSON_DET_EN);
            ret = LUX_IVS_SET_PARAM_ERR;
            break;
        }

        IMP_LOG_INFO(TAG, "LUX_Ivs_SetPDDet_Region(P0(%d,%d),P1(%d,%d),P2(%d,%d),P3(%d,%d)). \n",
                    PDParam.perms[0].p[0].x, PDParam.perms[0].p[0].y, PDParam.perms[0].p[1].x, PDParam.perms[0].p[1].y,
                    PDParam.perms[0].p[2].x, PDParam.perms[0].p[2].y, PDParam.perms[0].p[3].x, PDParam.perms[0].p[3].y);
    } while (0);
    Thread_Mutex_UnLock(&g_ivsAttr.paramMux);

    return ret;
}

/**
 * @description: 设置报警区域
 * @param  [in] zoneInfo 报警区域信息
 * @return 0 成功，非零失败，返回错误码
 */
INT_X LUX_IVS_SetAlarmRegion(LUX_IVS_ZONE_BASE_ST* zoneInfo)
{
    INT_X ret = LUX_ERR;

    if (0 >= zoneInfo->height || 0 >= zoneInfo->width)
    {
        IMP_LOG_ERR(TAG, "LUX_IVS_SetAlarmRegion failed, invalid param. \n");
        return LUX_IVS_SET_PARAM_ERR;
    }

    if ((zoneInfo->pointX + zoneInfo->width) > 100 || (zoneInfo->pointY + zoneInfo->height) > 100)
    {
        IMP_LOG_ERR(TAG, "LUX_IVS_SetAlarmRegion failed, invalid param. \n");
        return LUX_IVS_SET_PARAM_ERR;
    }

    /*移动侦测区域*/
    LUX_Ivs_SetMovDet_Region(zoneInfo);

    /*人形侦测区域*/
    ret = LUX_Ivs_SetPDDet_Region(zoneInfo);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "LUX_Ivs_SetPDDet_Region failed, chn(%d) . \n", LUX_IVS_PERSON_DET_EN);
        return LUX_IVS_SET_PARAM_ERR;
    }

    return LUX_OK;
}

/**
 * @description: 坐标转换
 * @param [in] pAlarmZone 坐标参数
 * @return 0 成功，非零失败，返回错误码
 */
static INT_X LUX_Ivs_ChangeCoord(LUX_IVS_ZONE_BASE_ST* pAlarmZone)
{
    if (NULL == pAlarmZone)
    {
        IMP_LOG_ERR(TAG, " LUX_Ivs_ChangeCoord failed\n");
        return LUX_PARM_NULL_PTR;
    }

    if ((pAlarmZone->pointX + pAlarmZone->width) > 100 ||
       (pAlarmZone->pointY - pAlarmZone->height) > 100)
    {
        IMP_LOG_ERR(TAG, " LUX_Ivs_ChangeCoord failed, set param invalid\n");
        return LUX_IVS_SET_PARAM_ERR;
    }

    pAlarmZone->pointX = 100 - pAlarmZone->pointX - pAlarmZone->width;
    pAlarmZone->pointY = 100 - pAlarmZone->pointY - pAlarmZone->height;
    pAlarmZone->width = pAlarmZone->width;
    pAlarmZone->height = pAlarmZone->height;

    return LUX_OK;
}

/**
 * @description: 解析报警区域字符串,将其转化为结构体
 * @param [out] pStrAlarmZoneInfo 区域报警参数
 * @return 0 成功，非零失败，返回错误码
 */
INT_X LUX_Ivs_alarmZone_Parse(CHAR_X* pStrAlarmZoneInfo, LUX_IVS_ZONE_BASE_ST* pAlarmZone)
{
    if (NULL == pStrAlarmZoneInfo || NULL == pAlarmZone)
    {
        IMP_LOG_ERR(TAG, " LUX_Ivs_alarmZone_Parse failed, empty point\n");
        return   LUX_PARM_NULL_PTR;
    }

    LUX_COMM_ParseStr(pStrAlarmZoneInfo, "x", &pAlarmZone->pointX);
    LUX_COMM_ParseStr(pStrAlarmZoneInfo, "y", &pAlarmZone->pointY);
    LUX_COMM_ParseStr(pStrAlarmZoneInfo, "xlen", &pAlarmZone->width);
    LUX_COMM_ParseStr(pStrAlarmZoneInfo, "ylen", &pAlarmZone->height);

    return LUX_OK;
}

/**
 * @description: 设置报警区域转换
 * @return 0 成功，非零失败，返回错误码
 */
INT_X LUX_IVS_SetAlarmRegion_Flip()
{
    INT_X ret = LUX_ERR;
    CHAR_X strAlarmZoneInfo[64] = { 0 };
    LUX_IVS_ZONE_BASE_ST alarmZone;

    ret = LUX_INIParse_GetString(TUYA_INI_CONFIG_FILE_NAME, "function", "alarm_zone_size", strAlarmZoneInfo);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " LUX_INIParse_GetString key[%s] failed\n", "alarm_zone_size");
        return LUX_GET_INI_VALUE_ERR;
    }

    ret = LUX_Ivs_alarmZone_Parse(strAlarmZoneInfo, &alarmZone);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " LUX_Ivs_alarmZone_Parse failed, return error [0x%x]\n", ret);
        return LUX_IVS_SET_PARAM_ERR;
    }

    ret = LUX_Ivs_ChangeCoord(&alarmZone);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " LUX_Ivs_ChangeCoord failed, return error [0x%x]\n", ret);
        return LUX_IVS_SET_PARAM_ERR;
    }

    ret = LUX_IVS_SetAlarmRegion(&alarmZone);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " LUX_IVS_SetAlarmRegion failed\n");
        return LUX_IVS_SET_PARAM_ERR;
    }

    memset(strAlarmZoneInfo, 0, sizeof(strAlarmZoneInfo) / sizeof(&strAlarmZoneInfo[0]));
    sprintf(strAlarmZoneInfo, "{\"num\":1,\"region0\":{\"x\":%d,\"xlen\":%d,\"y\":%d,\"ylen\":%d}}",
            alarmZone.pointX, alarmZone.width, alarmZone.pointY, alarmZone.height);

    ret = LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, "function", "alarm_zone_size", strAlarmZoneInfo);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "LUX_INIParse_SetKey failed,  key[%s]!, error num [0x%x] ", "alarm_zone_size", ret);
        return LUX_INI_SET_PARAM_FAILED;
    }

    return LUX_OK;
}

/**
 * @description: 设置报警区域转换
 * @return 0 成功，非零失败，返回错误码
 */
INT_X LUX_IVS_SetAlarmRegion_2_Flip()
{
    INT_X ret = LUX_ERR;
    CHAR_X permArea[248] = { 0 };
    CHAR_X region[248] = { 0 };
    LUX_ALARM_AREA_ST alarmArea;
    LUX_ALARM_AREA_ST newAlarmArea;
    INT_X i = 0;
    ret = LUX_INIParse_GetString(TUYA_INI_CONFIG_FILE_NAME, "function", "perm_area_2", permArea);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " LUX_INIParse_GetString key[%s] failed\n", "perm_area_2");
        return LUX_GET_INI_VALUE_ERR;
    }
    LUX_IVS_Perm_Json2St_2(permArea, &alarmArea);

    newAlarmArea.iPointNum = alarmArea.iPointNum;
    if ((0 < alarmArea.iPointNum) || (MAX_PERM_AREA_POINTS_NUM > alarmArea.iPointNum))
    {
        INT_X j = alarmArea.iPointNum;
        for (i = 0; i < alarmArea.iPointNum; i++)
        {
            newAlarmArea.alarmPonits[i].pointX = 100 - alarmArea.alarmPonits[--j].pointX;
            newAlarmArea.alarmPonits[i].pointY = 100 - alarmArea.alarmPonits[--j].pointY;
        }
    }
    memset(permArea, 0, sizeof(permArea));
    for (i = 0; i < newAlarmArea.iPointNum; i++)
    {
        //{"num":4,"point0":{"x":0,"y":0},"point1":{"x":100,"y":0},"point2":{"x":100,"y":100},"point3":{"x":0,"y":100}}
        if (0 == i)
        {
            snprintf(permArea, 248, "{\"num\":%d", newAlarmArea.iPointNum);
        }

        snprintf(region, 248, ",\"point%d\":{\"x\":%d,\"y\":%d}", i, newAlarmArea.alarmPonits[i].pointX, newAlarmArea.alarmPonits[i].pointY);
        //printf("\n\n\n\n\n\n\n ------------%s\n", region);
        strcat(permArea, region);

        if (i == (newAlarmArea.iPointNum - 1))
        {
            strcat(permArea, "}");
        }
    }
    ret = LUX_INIParse_SetKey(TUYA_INI_CONFIG_FILE_NAME, "function", "perm_area_2", permArea);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "LUX_INIParse_SetKey failed,  key[%s]!, error num [0x%x] ", "alarm_zone_size", ret);
        return LUX_INI_SET_PARAM_FAILED;
    }
    // if(LUX_OK != LUX_IVS_Perm_ConfigArea2(newAlarmArea))
    // {
    //     IMP_LOG_ERR(TAG, "LUX_IVS_Perm_ConfigArea2 failed");
    //     return LUX_INI_SET_PARAM_FAILED;
    // }

    return LUX_OK;
}

/**
 * @description: 设置移动侦测的属性
 * @param  [in] param 属性
 */
static VOID_X LUX_Ivs_SetMovDet_Attr(move_param_input_t* param)
{
    Thread_Mutex_Lock(&g_ivsAttr.paramMux);
    memcpy(param, &g_ivsAttr.ivsMovAttr.param, sizeof(move_param_input_t));
    Thread_Mutex_UnLock(&g_ivsAttr.paramMux);

    return;
}

/**
 * @description: 设置人形侦测的属性
 * @param  [in] param 属性
 */
static VOID_X LUX_Ivs_SetPDDet_ATTR(persondet_param_input_t* pParam)
{
    Thread_Mutex_Lock(&g_ivsAttr.paramMux);
    g_ivsAttr.personDetAttr.param.perms[0].p = &g_pd_perm[0];
    memcpy(pParam, &g_ivsAttr.personDetAttr.param, sizeof(persondet_param_input_t));
    Thread_Mutex_UnLock(&g_ivsAttr.paramMux);

    return;
}

/**
 * @description: 移动侦测启动设置配置
 * @return 0 成功，非零失败，返回错误码
 */
INT_X LUX_Ivs_Set_MovConf(VOID_X)
{
    INT_X ret = LUX_ERR;
    INT_X timeInterval = 60;
    BOOL_X alarmOnOff = LUX_FALSE;
    INT_X alarmSen = LUX_IVS_SENSITIVE_LOW_EN;
    BOOL_X sleepMode = LUX_FALSE;

    /*检测是否在隐私模式下*/
    ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "sleep_mode", &sleepMode);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " LUX_INIParse_GetBool key[%s] failed\n", "sleep_mode");
        return LUX_GET_INI_VALUE_ERR;
    }

    /*隐私模式下不开启人形追踪*/
    if (LUX_TRUE == sleepMode)
    {
        /*不使能通道*/
        ret = LUX_Ivs_StopChn(LUX_IVS_MOVE_DET_EN);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "LUX_Ivs_StopChn(%d) failed\n", LUX_IVS_MOVE_DET_EN);
            return LUX_IVS_START_RECIVE_PIC_ERR;
        }

        ret = LUX_IVS_StopAlgo(LUX_IVS_MOVE_DET_EN);
        if (0 != ret)
        {
            IMP_LOG_ERR(TAG, "LUX_IVS_StopAlgo failed!(%d) failed\n", LUX_IVS_MOVE_DET_EN);
        }
    }
    else
    {
        /*设置是否开启移动侦测警告*/
        ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "alarm_onoff", &alarmOnOff);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, " LUX_INIParse_GetBool key[%s] failed\n", "alarm_onoff");
            return LUX_GET_INI_VALUE_ERR;
        }

        if (LUX_TRUE != alarmOnOff)
        {
            /*关闭人形侦测*/
            ret = LUX_Ivs_StopChn(LUX_IVS_MOVE_DET_EN);
            if (0 != ret)
            {
                IMP_LOG_ERR(TAG, "LUX_IVS_StopAlgo failed!(%d) failed\n", LUX_IVS_MOVE_DET_EN);
            }

            ret = LUX_IVS_StopAlgo(LUX_IVS_PERSON_DET_EN);
            if (0 != ret)
            {
                IMP_LOG_ERR(TAG, "LUX_IVS_StopAlgo failed!(%d) failed\n", LUX_IVS_MOVE_DET_EN);
            }
        }
        else
        {
            ret = LUX_Ivs_StartChn(LUX_IVS_MOVE_DET_EN);
            if (0 != ret)
            {
                IMP_LOG_ERR(TAG, "LUX_IVS_StopAlgo failed!(%d) failed\n", LUX_IVS_MOVE_DET_EN);
            }

            ret = LUX_IVS_StartAlgo(LUX_IVS_MOVE_DET_EN);
            if (0 != ret)
            {
                IMP_LOG_ERR(TAG, "LUX_IVS_StopAlgo failed!(%d) failed\n", LUX_IVS_MOVE_DET_EN);
            }

            // ret = LUX_IVS_StartAlgo(LUX_IVS_PERSON_DET_EN);
            // if(0 != ret)
            // {
            //     IMP_LOG_ERR(TAG, "LUX_IVS_StopAlgo failed!(%d) failed\n", LUX_IVS_MOVE_DET_EN);
            // }
        }
    }

    /*设置报警时间间隔*/
    ret = LUX_INIParse_GetInt(TUYA_INI_CONFIG_FILE_NAME, "function", "alarm_interval", &timeInterval);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " LUX_INIParse_GetInt key[%s] failed\n", "alarm_interval");
        return LUX_GET_INI_VALUE_ERR;
    }

    LUX_IVS_SetTimeInterval((LUX_IVS_TIMEINTERVAL_EN)timeInterval);

    /*设置灵敏度*/
    ret = LUX_INIParse_GetInt(TUYA_INI_CONFIG_FILE_NAME, "function", "alarm_sen", &alarmSen);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " LUX_INIParse_GetInt key[%s] failed\n", "alarm_interval");
        return LUX_GET_INI_VALUE_ERR;
    }

    ret = LUX_IVS_SetSensitive(LUX_IVS_MOVE_DET_EN, (LUX_IVS_SENSITIVE_EN)alarmSen);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " LUX_INIParse_GetInt key[%s] failed\n", "alarm_interval");
        return LUX_IVS_SET_PARAM_ERR;
    }

    return LUX_OK;
}

/**
 * @description: 人形侦测启动设置配置
 * @return 0 成功，非零失败，返回错误码
 */
INT_X LUX_Ivs_Set_PDConf(VOID_X)
{
    INT_X ret = LUX_ERR;
    BOOL_X alarmHunFilter = LUX_FALSE;
    //BOOL_X alarmOnOff = LUX_FALSE;
    INT_X alarmSen = LUX_IVS_SENSITIVE_LOW_EN;
    BOOL_X alarmZoneOnOff = LUX_FALSE;
    CHAR_X strAlarmZoneInfo[64] = { 0 };
    LUX_IVS_ZONE_BASE_ST alarmZone;
    BOOL_X sleepMode = LUX_FALSE;

    memset(&strAlarmZoneInfo, 0, sizeof(LUX_IVS_ZONE_BASE_ST));

    ret = LUX_Ivs_StartChn(LUX_IVS_PERSON_DET_EN);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "LUX_Ivs_StartChn(%d) failed\n", LUX_IVS_PERSON_DET_EN);
        return LUX_IVS_START_RECIVE_PIC_ERR;
    }

    /*检测是否在隐私模式下*/
    ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "sleep_mode", &sleepMode);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " LUX_INIParse_GetBool key[%s] failed\n", "sleep_mode");
        return LUX_GET_INI_VALUE_ERR;
    }

    /*隐私模式下不开启人形追踪*/
    if (LUX_TRUE == sleepMode)
    {
        ret = LUX_IVS_StopAlgo(LUX_IVS_PERSON_DET_EN);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "LUX_IVS_StopAlgo(%d) failed\n", LUX_IVS_PERSON_DET_EN);
            return LUX_IVS_START_RECIVE_PIC_ERR;
        }
    }
    else
    {
#ifdef CONFIG_BABY
        /*设置是否开启人形侦测侦测警告*/
        ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "alarm_human_filter", &alarmHunFilter);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, " LUX_INIParse_GetBool key[%s] failed\n", "alarmHunFilter");
            return LUX_GET_INI_VALUE_ERR;
        }

        if (LUX_TRUE == alarmHunFilter)
        {
            ret = LUX_IVS_StartAlgo(LUX_IVS_PERSON_DET_EN);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "LUX_IVS_StartAlgo(%d) failed\n", LUX_IVS_PERSON_DET_EN);
                return LUX_IVS_START_RECIVE_PIC_ERR;
            }
        }
        else
        {
            ret = LUX_IVS_StopAlgo(LUX_IVS_PERSON_DET_EN);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "LUX_Ivs_StopChn(%d) failed\n", LUX_IVS_PERSON_DET_EN);
                return LUX_IVS_START_RECIVE_PIC_ERR;
            }
        }
#else
        /*设置是否开启侦测警告开关*/
        ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "alarm_onoff", &alarmOnOff);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, " LUX_INIParse_GetBool key[%s] failed\n", "alarm_onoff");
            return LUX_GET_INI_VALUE_ERR;
        }

        if (LUX_FALSE == alarmOnOff)
        {
            ret = LUX_Ivs_StopChn(LUX_IVS_PERSON_DET_EN);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "LUX_Ivs_StopChn(%d) failed\n", LUX_IVS_PERSON_DET_EN);
                return LUX_IVS_START_RECIVE_PIC_ERR;
            }
        }
        else
        {
            /*设置是否开启人形侦测侦测警告*/
            ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "alarm_human_filter", &alarmHunFilter);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, " LUX_INIParse_GetBool key[%s] failed\n", "alarmHunFilter");
                return LUX_GET_INI_VALUE_ERR;
            }

            if (LUX_TRUE == alarmHunFilter)
            {
                ret = LUX_Ivs_StopChn(LUX_IVS_MOVE_DET_EN);
                if (LUX_OK != ret)
                {
                    IMP_LOG_ERR(TAG, "LUX_Ivs_StopChn(%d) failed\n", LUX_IVS_PERSON_DET_EN);
                    return LUX_IVS_START_RECIVE_PIC_ERR;
                }
            }
            else
            {
                ret = LUX_Ivs_StopChn(LUX_IVS_PERSON_DET_EN);
                if (LUX_OK != ret)
                {
                    IMP_LOG_ERR(TAG, "LUX_Ivs_StopChn(%d) failed\n", LUX_IVS_PERSON_DET_EN);
                    return LUX_IVS_START_RECIVE_PIC_ERR;
                }
            }
        }
#endif

    }

    /*设置灵敏度*/
    ret = LUX_INIParse_GetInt(TUYA_INI_CONFIG_FILE_NAME, "function", "alarm_sen", &alarmSen);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " LUX_INIParse_GetInt key[%s] failed\n", "alarm_interval");
        return LUX_GET_INI_VALUE_ERR;
    }

    ret = LUX_IVS_SetSensitive(LUX_IVS_PERSON_DET_EN, (LUX_IVS_SENSITIVE_EN)alarmSen);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " LUX_INIParse_GetInt key[%s] failed\n", "alarm_interval");
        return LUX_IVS_SET_PARAM_ERR;
    }

    /*设置区域报警*/
    ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "alarm_zone_onoff", &alarmZoneOnOff);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " LUX_INIParse_GetBool key[%s] failed\n", "alarm_zone_onoff");
        return LUX_GET_INI_VALUE_ERR;
    }

    ret = LUX_IVS_AlarmZone_Open(alarmZoneOnOff);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " LUX_IVS_AlarmZone_Open failed\n");
        return LUX_GET_INI_VALUE_ERR;
    }

    ret = LUX_INIParse_GetString(TUYA_INI_CONFIG_FILE_NAME, "function", "alarm_zone_size", strAlarmZoneInfo);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " LUX_INIParse_GetString key[%s] failed\n", "alarm_zone_size");
        return LUX_GET_INI_VALUE_ERR;
    }

    ret = LUX_Ivs_alarmZone_Parse(strAlarmZoneInfo, &alarmZone);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " LUX_Ivs_alarmZone_Parse failed, return error [0x%x]\n", ret);
        return LUX_IVS_SET_PARAM_ERR;
    }

    ret = LUX_IVS_SetAlarmRegion(&alarmZone);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " LUX_IVS_SetAlarmRegion failed\n");
        return LUX_IVS_SET_PARAM_ERR;
    }

    return LUX_OK;
}

#ifdef CONFIG_PTZ_IPC
/**
 * @description: 人形追踪启动设置配置
 * @return 0 成功，非零失败，返回错误码
 */
INT_X LUX_Ivs_Set_PTConf(VOID_X)
{
    INT_X ret = LUX_ERR;
    BOOL_X PTOnOff = LUX_FALSE;
    BOOL_X sleepMode = LUX_FALSE;

    /*检测是否在隐私模式下*/
    ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "sleep_mode", &sleepMode);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " LUX_INIParse_GetBool key[%s] failed\n", "sleep_mode");
        return LUX_GET_INI_VALUE_ERR;
    }

    /*隐私模式下不开启人形追踪*/
    if (LUX_TRUE == sleepMode)
    {
        /* 不使能通道 */
        ret = LUX_Ivs_StopChn(LUX_IVS_PERSON_TRACK_EN);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "LUX_Ivs_StopChn(%d) failed\n", LUX_IVS_PERSON_TRACK_EN);
            return LUX_IVS_START_RECIVE_PIC_ERR;
        }
    }
    else
    {
        /*设置是否开启人形追踪警告*/
        ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "person_tracker", &PTOnOff);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, " LUX_INIParse_GetBool key[%s] failed\n", "person_tracker");
            return LUX_GET_INI_VALUE_ERR;
        }

        if (LUX_TRUE != PTOnOff)
        {
            /*不使能通道*/
            ret = LUX_Ivs_StopChn(LUX_IVS_PERSON_TRACK_EN);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "LUX_Ivs_StopChn(%d) failed\n", LUX_IVS_PERSON_TRACK_EN);
                return LUX_IVS_START_RECIVE_PIC_ERR;
            }
        }
        else
        {
            ret = LUX_IVS_StartAlgo(LUX_IVS_PERSON_TRACK_EN);
            if (0 != ret)
            {
                IMP_LOG_ERR(TAG, "LUX_IVS_StopAlgo failed!(%d) failed\n", LUX_IVS_MOVE_DET_EN);
            }
        }
    }

    return LUX_OK;
}
#endif


/**
 * @description: 根据涂鸦配置文件设置初始化参数
 * @return 0 成功，非零失败，返回错误码
 */
static  INT_X LUX_Ivs_Set_StartConf(INT_X chnIdx)
{
    INT_X ret = LUX_ERR;

    if (LUX_IVS_MOVE_DET_EN == chnIdx)
    {
        ret = LUX_Ivs_Set_MovConf();
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "LUX_Ivs_Set_MovConf failed, return error [0x%x]\n", ret);
            return LUX_IVS_CHAN_RIGISTER_ERR;
        }
    }
    else if (LUX_IVS_PERSON_DET_EN == chnIdx)
    {
        ret = LUX_Ivs_Set_PDConf();
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "LUX_Ivs_Set_PDConf failed, return error [0x%x]\n", ret);
            return LUX_IVS_CHAN_RIGISTER_ERR;
        }
    }
#ifdef CONFIG_PTZ_IPC
    else if (LUX_IVS_PERSON_TRACK_EN == chnIdx)
    {
        ret = LUX_Ivs_Set_PTConf();
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "LUX_Ivs_Set_PTConf failed, return error [0x%x]\n", ret);
            return LUX_IVS_CHAN_RIGISTER_ERR;
        }
    }
#endif

    return LUX_OK;
}

/**
 * @description: 算法的初始化函数
 * @param [in] chnIdx 通道
 * @return 0成功,非0返回失败返回错误码
 */
static INT_X LUX_Ivs_Algos_Init(INT_X chnIdx)
{
    INT_X ret = LUX_ERR;

    /*创建通道*/
    ret = IMP_IVS_CreateChn(g_ivsAttr.bindAttr.chnNum[chnIdx], g_ivsAttr.inteface[chnIdx]);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "IMP_IVS_CreateChn(%d) failed\n", g_ivsAttr.bindAttr.chnNum[chnIdx]);
        return LUX_IVS_CHAN_CREATE_ERR;
    }


    /*通道和ivs组绑定*/
    ret = IMP_IVS_RegisterChn(g_ivsAttr.bindAttr.grpId, g_ivsAttr.bindAttr.chnNum[chnIdx]);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "IMP_IVS_RegisterChn(%d, %d) failed\n", g_ivsAttr.bindAttr.grpId, g_ivsAttr.bindAttr.chnNum[chnIdx]);
        return LUX_IVS_CHAN_RIGISTER_ERR;
    }

    /*根据涂鸦的配置是否使能通道*/
    ret = LUX_Ivs_Set_StartConf(chnIdx);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " LUX_Ivs_set_TuyaConf_Init failed, return error [0x%x]\n", ret);
        return LUX_IVS_SET_STARTCONFIG_ERR;
    }

    return LUX_OK;
}

IMPRgnHandle PersonRectRgnHander[LUX_HUMANRECT_NUM_MAX];
extern BOOL_X g_osdInit;
extern LUX_OSD_ATTR_ST g_osdAttr[LUX_OSD_REGION_NUM];
/**
 * @description: ivs算法处理线程
 * @param  [in] arg 通道号
 * @return 0 成功，非零失败，返回错误码
 */
static PVOID_X lux_ivs_get_result_process(PVOID_X arg)
{
    INT_X chnIdx = (INT_X)arg;
    INT_X ret = LUX_ERR;
    CHAR_X threadName[64] = { 0 };
    CHAR_X ivsChnName[32] = { 0 };
    // BOOL_X human_det_onoff = LUX_FALSE;
    BOOL_X sleep_det_onoff = LUX_FALSE;
    INT_X missPersonCnt = 0;
#ifdef CONFIG_PTZ_IPC
    LUX_IVS_PTRESULT_QUE_ST* pIvsPTResultQue = NULL;
#endif

    if (LUX_IVS_MOVE_DET_EN == chnIdx)
    {
        strcpy(ivsChnName, "MOVE_DETECTION");
        g_ivsAttr.IsRunning[chnIdx] = LUX_TRUE;
    }
    else if (LUX_IVS_PERSON_DET_EN == chnIdx)
    {
        strcpy(ivsChnName, "PERSON_DETECTION");
        g_ivsAttr.IsRunning[chnIdx] = LUX_TRUE;
    }
#if 0
#ifdef CONFIG_PTZ_IPC    
    else if (LUX_IVS_PERSON_TRACK_EN == chnIdx)
    {
        strcpy(ivsChnName, "PERSON_TRACKER");
        g_ivsAttr.IsRunning[chnIdx] = LUX_TRUE;
    }
#endif
#endif
    sprintf(threadName, "IVS_%s", ivsChnName);
    prctl(PR_SET_NAME, threadName);

    Semaphore_Pend(&g_ivsAlgSem, SEM_FOREVER);

    ret = LUX_Ivs_Algos_Init(chnIdx);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "LUX_Ivs_Algos_Init(%d) failed, return error [0x%x]\n", chnIdx, ret);
        return (PVOID_X)LUX_ERR;
    }

    while (g_ivsAttr.IsRunning[chnIdx])
    {
        if (LUX_IVS_PERSON_DET_EN != chnIdx)
        {
            if (LUX_TRUE != g_ivsAttr.bStart[chnIdx])
            {
                usleep(100 * 1000);
                continue;
            }
        }

        Thread_Mutex_Lock(&g_ivsAttr.mux[chnIdx]);
        ret = IMP_IVS_PollingResult(chnIdx, IMP_IVS_DEFAULT_TIMEOUTMS);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_PollingResult(%d, %d) failed\n", chnIdx, IMP_IVS_DEFAULT_TIMEOUTMS);
        }

        /*移动侦测*/
        if (LUX_IVS_MOVE_DET_EN == chnIdx)
        {
            ret = IMP_IVS_GetResult(chnIdx, (PVOID_X*)&g_ivsAttr.ivsMovAttr.ivsMovResult);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "IMP_IVS_GetResult(%d) failed\n", chnIdx);
            }

            ret = LUX_Ivs_SendMovMsg(g_ivsAttr.ivsMovAttr.ivsMovResult);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "LUX_Ivs_SendMovMsg failed\n", chnIdx);
            }

            ret = IMP_IVS_ReleaseResult(chnIdx, (PVOID_X)g_ivsAttr.ivsMovAttr.ivsMovResult);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "IMP_IVS_ReleaseResult(%d) failed\n", chnIdx);
            }

        }
        /*人形侦测*/
        else if (LUX_IVS_PERSON_DET_EN == chnIdx)
        {
            ret = IMP_IVS_GetResult(chnIdx, (PVOID_X*)&g_ivsAttr.personDetAttr.ivsPersonDetResult);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "IMP_IVS_GetResult(%d) failed\n", chnIdx);
            }

            ret = LUX_Ivs_SendPDMsg(g_ivsAttr.personDetAttr.ivsPersonDetResult);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "LUX_Ivs_SendMovMsg(%d) failed\n", chnIdx);
            }
#if 0
            ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "sleep_det_onoff", &sleep_det_onoff);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "LUX_INIParse_GetBool alarm_human_filter failed!, error num [0x%x] ", ret);
            }
            /* 人形矩形框显示 */
            if (g_ivsAttr.bStart[LUX_IVS_PERSON_DET_EN] || sleep_det_onoff)
            {
                if (g_ivsAttr.personDetAttr.ivsPersonDetResult->count > 0)
                {
                    LUX_OSD_PersonRect_Miss();
                    LUX_OSD_PersonRect_Show(g_ivsAttr.personDetAttr.ivsPersonDetResult);
                    missPersonCnt = 0;
                }
                else if (missPersonCnt >= 3)
                {
                    LUX_OSD_PersonRect_Miss();
                }
                else
                {
                    missPersonCnt++;
                }
            }
            else
            {
                LUX_OSD_PersonRect_Miss();
            }
#endif

            ret = IMP_IVS_ReleaseResult(chnIdx, (PVOID_X)g_ivsAttr.personDetAttr.ivsPersonDetResult);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "IMP_IVS_ReleaseResult(%d) failed\n", chnIdx);
            }
        }
#ifdef CONFIG_PTZ_IPC
        /*人形追踪*/
        else if (LUX_IVS_PERSON_TRACK_EN == chnIdx)
        {
            ret = IMP_IVS_GetResult(chnIdx, (PVOID_X*)&g_ivsAttr.personTrackAttr.ivsPersonTrackResult);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "IMP_IVS_GetResult(%d) failed\n", chnIdx);
            }

#if 0
            //TODO:调式信息, 调试完删除
            INT_X cnt = 0;
            person_tracker_param_output_t* result = g_ivsAttr.personTrackAttr.ivsPersonTrackResult;
            printf("------------------ result->count=%d, dx=%d, dy=%d, step=%d\n",
                                            result->count, result->dx, result->dy, result->step);
            for (cnt = 0; cnt < result->count; cnt++)
            {
                printf("----person_status %d\n", result->person_status);
                printf("----rect[%d]=((%d,%d),(%d,%d))\n",
                        cnt, result->rect[cnt].ul.x, result->rect[cnt].ul.y, result->rect[cnt].ul.x, result->rect[cnt].ul.y);
            }
#endif

            /*检测到人物*/
            if (0 != g_ivsAttr.personTrackAttr.ivsPersonTrackResult->count)
            {
                //TODO:有可能要更改
                ret = LUX_Ivs_GetResultBuf(&pIvsPTResultQue, g_ivsAttr.personTrackAttr.ivsPersonTrackResult);
                if (LUX_OK != ret)
                {
                    IMP_LOG_ERR(TAG, "LUX_Ivs_GetResultBuf(%d) failed\n", chnIdx);
                }
                else
                {
                    ret = LUX_Ivs_SendPTMsg(pIvsPTResultQue);
                    if (LUX_OK != ret)
                    {
                        IMP_LOG_ERR(TAG, "LUX_Ivs_SendPTMsg(%d) failed, return error[0x%x]\n", chnIdx, ret);
                    }
                }
            }
#if 1       //调试完后删除, 如果不加下面代码的话,会导捕获到人后就不再捕捉了.
            person_tracker_param_input_t param;
            ret = IMP_IVS_GetParam(chnIdx, &param);
            if (ret < 0)
            {
                IMP_LOG_ERR(TAG, "IMP_IVS_GetParam(%d) failed\n", 0);
            }
            param.is_motor_stop = 1;
            param.is_feedback_motor_status = 1;

            ret = IMP_IVS_SetParam(chnIdx, &param);
            if (ret < 0)
            {
                IMP_LOG_ERR(TAG, "IMP_IVS_SetParam(%d) failed\n", 0);
            }
#endif
            ret = IMP_IVS_ReleaseResult(chnIdx, (PVOID_X)g_ivsAttr.personTrackAttr.ivsPersonTrackResult);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "IMP_IVS_ReleaseResult(%d) failed\n", chnIdx);
            }
        }
#endif /*CONFIG_PTZ_IPC*/

        Thread_Mutex_UnLock(&g_ivsAttr.mux[chnIdx]);
        // usleep(150 * 1000);
        usleep(30 * 1000);
    }

    return (PVOID_X)LUX_OK;
}

/**
 * @description: ivs模块初始化
 * @return 0 成功，非零失败，返回错误码
 */
INT_X LUX_IVS_Init(VOID_X)
{
    INT_X ret = LUX_ERR;
    INT_X  chnIdx = 0;
    LUX_MODULES_BIND_ST bind;
    move_param_input_t param;
    persondet_param_input_t pdParam;
#ifdef CONFIG_PTZ_IPC
    //person_tracker_param_input_t ptparam;
#endif

    /*创建ivs通道*/
    ret = IMP_IVS_CreateGroup(g_ivsAttr.bindAttr.grpId);
    if (ret != LUX_OK)
    {
        IMP_LOG_ERR(TAG, "IMP_IVS_CreateGroup(%d) failed\n", g_ivsAttr.bindAttr.grpId);
        return LUX_IVS_GROUP_CREATE_ERR;
    }

    /* 绑定venc通道到源模块通道 */
    memset(&bind, 0, sizeof(bind));
    bind.srcChan = g_ivsAttr.bindAttr.bindSrcChan;
    bind.srcModule = g_ivsAttr.bindAttr.bindSrcModule;
    bind.dstChan = g_ivsAttr.bindAttr.bindDstChan;
    bind.dstModule = g_ivsAttr.bindAttr.bindDstModule;

    ret = LUX_COMM_Bind_AddModule(&bind);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "LUX_COMM_Bind_AddModule error!\n");
        return LUX_IVS_CHAN_CREATE_ERR;
    }

    /*设置移动侦测的属性*/
    memset(&param, 0, sizeof(move_param_input_t));
    LUX_Ivs_SetMovDet_Attr(&param);

    /*获取移动侦测算法句柄*/
    g_ivsAttr.inteface[0] = MoveInterfaceInit(&param);
    if (NULL == g_ivsAttr.inteface[0])
    {
        IMP_LOG_ERR(TAG, "IMP_IVS_CreateMoveInterface failed\n");
        return LUX_IVS_SET_PARAM_ERR;
    }

    /*设置人形侦测的属性*/
    memset(&pdParam, 0, sizeof(persondet_param_input_t));
    LUX_Ivs_SetPDDet_ATTR(&pdParam);

    /*获取人形侦测算法句柄*/
    g_ivsAttr.inteface[1] = PersonDetInterfaceInit(&pdParam);
    if (NULL == g_ivsAttr.inteface[1])
    {
        IMP_LOG_ERR(TAG, "PersonDetInterfaceInit failed\n");
        return LUX_IVS_SET_PARAM_ERR;
    }

#if 0
#ifdef CONFIG_PTZ_IPC
    /*设置人形追踪的属性*/
    memset(&ptparam, 0, sizeof(person_tracker_param_input_t));
    LUX_Ivs_SetPTAttr(&ptparam);

    /*设置人形追踪的属性*/
    g_ivsAttr.inteface[2] = Person_TrackerInterfaceInit(&ptparam);
    if (NULL == g_ivsAttr.inteface[2])
    {
        IMP_LOG_ERR(TAG, "Person_TrackerInterfaceInit failed\n");
        return LUX_IVS_SET_PARAM_ERR;
    }
#endif
#endif
    Semaphore_Create(0, &g_ivsAlgSem);

    Thread_Mutex_Create(&g_ivsAttr.paramMux);

    for (chnIdx = 0; chnIdx < LUX_IVS_ALGO_NUM; chnIdx++)
    {
        /*创建互斥锁*/
        Thread_Mutex_Create(&g_ivsAttr.mux[chnIdx]);

        /*创建线程处理ivs结果*/
        ret = pthread_create(&g_ivsAttr.threadId[chnIdx], NULL, lux_ivs_get_result_process, (PVOID_X)g_ivsAttr.bindAttr.chnNum[chnIdx]);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, " pthread_create failed\n");
            return LUX_IVS_CREAT_THREAD_ERR;
        }
    }

    /*IVS事件处理函数初始化*/
    ret = LUX_EVENT_IVSProcess_Init();
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, " LUX_EVENT_IVSProcess_Init failed\n");
        return LUX_IVS_CAPTURE_PIC_ERR;
    }

#ifdef CONFIG_BABY
    // /* 笑脸抓拍初始化 */
    // LUX_IVS_SmileDet_Init();
    // /* 人脸检测初始化 */
    // LUX_IVS_FaceDet_Init();
    // /* 周界算法初始化 */
    // LUX_IVS_Perm_Init();
#endif

    return LUX_OK;
}

/**
 * @description: IVS模块去初始化
 * @return 0 成功，非零失败，返回错误码
 */
INT_X LUX_IVS_Deinit()
{
    INT_X ret = LUX_ERR;
    INT_X chnIdx = 0;

    for (chnIdx = 0; chnIdx < LUX_IVS_ALGO_NUM; chnIdx++)
    {
        g_ivsAttr.IsRunning[chnIdx] = LUX_FALSE;
        pthread_join(g_ivsAttr.threadId[chnIdx], NULL);

        ret = LUX_Ivs_StopChn(chnIdx);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "LUX_Ivs_StopChn(%d) failed\n", chnIdx);
            return LUX_IVS_STOP_RECIVE_PIC_ERR;
        }

        ret = IMP_IVS_UnRegisterChn(chnIdx);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_UnRegisterChn(%d) failed\n", chnIdx);
            return LUX_IVS_CHAN_UNRIGISTER_ERR;
        }

        ret = IMP_IVS_DestroyChn(chnIdx);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "IMP_IVS_DestroyChn(%d) failed\n", chnIdx);
            return LUX_IVS_CHAN_DESTROY_ERR;
        }

        MoveInterfaceExit(g_ivsAttr.inteface[chnIdx]);

        Thread_Mutex_Destroy(&g_ivsAttr.mux[chnIdx]);
    }

    Thread_Mutex_Destroy(&g_ivsAttr.paramMux);
    /*TODO: 是否需要解绑group*/
    return LUX_OK;
}
