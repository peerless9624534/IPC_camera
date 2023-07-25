/*
 * lux_ivs.h
 *
 * 智能模块，包括移动侦测、人脸检测、人形侦测等功能
 *
 */

#ifndef __LUX_IVS_H__
#define __LUX_IVS_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

#include <comm_def.h>
#include <lux_base.h>
#include <iaac.h>
#include <ivs_inf_move.h>
#include <ivs_interface.h>
#include "ivs_inf_faceDet.h"
#include <ivs_inf_personDet.h>
#include <ivs_inf_person_tracker.h>
    /*算法类型*/
    typedef enum
    {
        LUX_IVS_MOVE_DET_EN,
        LUX_IVS_PERSON_DET_EN,
        LUX_IVS_PERSON_TRACK_EN,
        LUX_IVS_PERSON_BUTTON,

    }LUX_IVS_ALGO_EN;

    /*时间间隔*/
    typedef enum
    {
        LUX_IVS_TIMEINTERVAL_1MIN,
        LUX_IVS_TIMEINTERVAL_3MIN,
        LUX_IVS_TIMEINTERVAL_5MIN,
        LUX_IVS_TIME_INTERVAL_BUTTON,
    }LUX_IVS_TIMEINTERVAL_EN;

    /*灵敏度设置*/
    typedef enum
    {
        LUX_IVS_SENSITIVE_LOW_EN,
        LUX_IVS_SENSITIVE_MID_EN,
        LUX_IVS_SENSITIVE_HIG_EN,
        LUX_IVS_SENSITIVE_BUTTON,
    }LUX_IVS_SENSITIVE_EN;

    typedef struct
    {
        int classid;
        int x0;
        int y0;
        int x1;
        int y1;
        float confidence;
    }LUX_OBJ_INFO_ST;

    typedef LUX_OBJ_INFO_ST LUX_FACE_INFO_ST;/*classid: 1 人脸 2 遮脸*/

    typedef LUX_OBJ_INFO_ST LUX_PERSON_INFO_ST;

    typedef struct
    {
        int count;//人脸数
        LUX_FACE_INFO_ST face[NUM_OF_FACES];
    }LUX_FACEDET_OUTPUT_ST;

    typedef struct
    {
        int count;//人形数
        LUX_PERSON_INFO_ST person[NUM_OF_PERSONS];
    }LUX_PERSONDET_OUTPUT_ST;

#ifdef CONFIG_PTZ_IPC
    /*人形追踪结果队列*/
    typedef struct
    {
        BOOL_X bUsed;
        person_tracker_param_output_t ivsPersonTrackResult;

    }LUX_IVS_PTRESULT_QUE_ST;
#endif

/*移动侦测感兴趣坐标*/
typedef struct
{
    IVSPoint    ul;     /*左上角*/
    IVSPoint    br;     /*右下角*/

}LUX_IVS_ROI_ST;

/*报警感兴趣区域*/
typedef struct
{
    BOOL_X  bOpen;
    LUX_IVS_ROI_ST movDet;      /*移动侦测报警区域信息*/

}LUX_IVS_ALARM_ZONE_ST;

/*事件报警事件间隔*/
typedef struct
{
    INT_X   timeInterval;        /*时间间隔*/
    UINT_X  lastTime;
    UINT_X  curTime;

}LUX_IVS_TIME_INTERVAL_ST;


/*IVS模块的绑定信息*/
typedef struct
{
    INT_X grpId;                          /* 组ID号 */
    INT_X       chnNum[LUX_IVS_ALGO_NUM]; /*算法通道*/
    INT_X             bindSrcChan;        /* 编码通道要绑定的源通道 */
    LUX_MODULS_DEV_EN bindSrcModule;      /* 编码通道要绑定的源模块 */
    INT_X             bindDstChan;        /* 编码通道要绑定的目标通道 */
    LUX_MODULS_DEV_EN bindDstModule;      /* 编码通道要绑定的目标模块 */


}LUX_IVS_BIND_ATTR_ST;


/*移动侦测结构体*/
typedef struct
{
    move_param_input_t   param;             /*移动侦测的算法参数设置*/
    move_param_output_t* ivsMovResult;     /*移动侦测的计算结果*/
    INT_X   sensParam;                      /*移动侦测灵敏度*/
}LUX_IVS_MOVE_ATTR_ST;

/*人形侦测结构体*/
typedef struct
{
    persondet_param_input_t param;                  /*人形侦测的算法参数设置*/
    persondet_param_output_t* ivsPersonDetResult;   /*人形侦测的计算结果*/

}LUX_IVS_PERSONDET_ATTR_ST;

#ifdef CONFIG_PTZ_IPC

#define LUX_PTRESULT_QUE_LEN    32
/*人形追踪结构体*/
typedef struct
{
    person_tracker_param_input_t  param;                                   /*人形追踪的算法参数设置*/
    person_tracker_param_output_t* ivsPersonTrackResult;                  /*人形追踪的计算结果*/
    LUX_IVS_PTRESULT_QUE_ST       ivsPTResultQue[LUX_PTRESULT_QUE_LEN];    /*人形追踪结果队列信息*/

}LUX_IVS_PERSONTRACK_ATTR_ST;

#endif /*CONFIG_PTZ_IPC*/

typedef struct
{
    BOOL_X      bInit;
    pthread_t   threadId[LUX_IVS_ALGO_NUM];
    OSI_MUTEX   mux[LUX_IVS_ALGO_NUM];                    /*通道锁*/
    OSI_MUTEX   paramMux;                                 /*设置属性锁*/
    BOOL_X      IsRunning[LUX_IVS_ALGO_NUM];              /*是否正在执行*/
    BOOL_X      bIvsInit[LUX_IVS_ALGO_NUM];               /*算法初始化*/
#ifdef CONFIG_PTZ_IPC
    BOOL_X      bStart[LUX_IVS_ALGO_TYPE];
#else
    BOOL_X      bStart[LUX_IVS_ALGO_NUM];
#endif
    IAACInfo iaacInfo;                                   /*智能算法注册信息*/
    LUX_IVS_TIME_INTERVAL_ST    rptEventTI;              /*事件报警时间间隔*/
    LUX_IVS_ALARM_ZONE_ST       alarmZone;               /*报警区域*/
    LUX_IVS_BIND_ATTR_ST        bindAttr;                /*绑定信息*/
    LUX_IVS_MOVE_ATTR_ST        ivsMovAttr;              /*移动侦测属性*/
    LUX_IVS_PERSONDET_ATTR_ST   personDetAttr;           /*人形侦测属性*/
#ifdef CONFIG_PTZ_IPC
    LUX_IVS_PERSONTRACK_ATTR_ST  personTrackAttr;         /*人形追踪属性*/
#endif
    IMPIVSInterface* inteface[LUX_IVS_ALGO_NUM];  /*算法句柄*/
}LUX_IVS_ATTR_ST;

    INT_X LUX_Person_Alg_Process(INT_X chnID, UINT_X width, UINT_X height, PVOID_X result);

    INT_X LUX_Face_Alg_Process(INT_X chnID, UINT_X width, UINT_X height, PVOID_X result);


    //Detection area structure
    typedef struct
    {
        INT_X pointX;    //Starting point x  [0-100]
        INT_X pointY;    //Starting point Y  [0-100]
        INT_X width;     //width    [0-100]
        INT_X height;    //height    [0-100]

    }LUX_IVS_ZONE_BASE_ST;

    /* 解析报警区域 */
    INT_X LUX_Ivs_alarmZone_Parse(CHAR_X* pStrAlarmZoneInfo, LUX_IVS_ZONE_BASE_ST* pAlarmZone);

    /**
     * @description: ivs算法认证初始化
     * @return 0成功, 非0错误
     * @attention 需要连接网路后初始化
     */
    INT_X LUX_IVS_Iaac_Init();

    /**
     * @description: 报警区域开关
     * @param [in] bOpen IVS处理结果
     * @return 0 成功，非零失败
     */
    INT_X LUX_IVS_AlarmZone_Open(BOOL_X bOpen);

    /**
     * @description: ivs事件报警间隔倒计时
     * @return 倒计时未到返回FALSE, 到了返回 TURE,
     */
    BOOL_X LUX_IVS_GetReportEvent_Countdown();

    /**
     * @description: 获取报警区域开关
     * @return 0 成功，非零失败
     */
    BOOL_X LUX_IVS_GetAlarmZone_OnOff();

    /**
     * @description: 判断移动侦测的结果是否在感兴趣区域内
     * @param [in] IvsResult IVS处理结果
     * @return 在区域内返回ture，否则返回false
     */
    BOOL_X LUX_IVS_MovDetRegion_Process(LUX_BASE_IVSDATA_ST* MovResult);

    /**
     * @description: 设置报警区域
     * @param  [in] zoneInfo 报警区域信息
     * @return 0 成功，非零失败，返回错误码
     */
    INT_X LUX_IVS_SetAlarmRegion(LUX_IVS_ZONE_BASE_ST* zoneInfo);

    /**
     * @description: 设置报警区域转换
     * @return 0 成功，非零失败，返回错误码
     */
    INT_X LUX_IVS_SetAlarmRegion_Flip();

    /**
     * @description: 设置多边形报警区域转换
     * @return 0 成功，非零失败，返回错误码
     */
    INT_X LUX_IVS_SetAlarmRegion_2_Flip();

    /**
     * @description: 设置ivs通道算法灵敏度
     * @param [in] chnIdx  通道号
     * @param [in] sns     灵敏度
     * @return 0 成功，非零失败，返回错误码
     */
    INT_X LUX_IVS_SetSensitive(LUX_IVS_ALGO_EN chnIdx, LUX_IVS_SENSITIVE_EN sns);

    /**
     * @description: 获取移动侦测灵敏度
     * @return 返回灵敏度
     */
    INT_X LUX_Ivs_GetMovSensitive();

    /**
     * @description: ivs事件报警间隔
     * @param  [in] ts 时间间隔,单位s
     * @return 0 成功，非零失败，返回错误码
     */
    VOID_X LUX_IVS_SetTimeInterval(LUX_IVS_TIMEINTERVAL_EN ts);

    /**
     * @description: 开启IVS侦测
     * @param [in] algo  算法类型
     * @return 0 成功，非零失败，返回错误码
     */
    INT_X LUX_IVS_StartAlgo(LUX_IVS_ALGO_EN algo);

    /**
     * @description: 关闭IVS侦测
     * @param [in] algo  算法类型
     * @return 0 成功，非零失败，返回错误码
     */
    INT_X LUX_IVS_StopAlgo(LUX_IVS_ALGO_EN algo);

    /**
     * @description: ivs模块初始化
     * @return 0 成功，非零失败，返回错误码
     */
    INT_X LUX_IVS_Init();

    /**
     * @description: IVS模块去初始化
     * @return 0 成功，非零失败，返回错误码
     */
    INT_X LUX_IVS_Deinit();

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __LUX_IVS_H__ */
