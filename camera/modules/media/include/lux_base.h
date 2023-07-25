/*
 * lux_base.h
 *
 * 基础功能、通用功能函数头文件
 */

#ifndef __LUX_BASE_H__
#define __LUX_BASE_H__

#include <semaphore.h>
#include <pthread.h>

 //#include <ivs_inf_move.h>
 //#include <ivs_inf_personDet.h>

#include <comm_def.h>
#include <comm_func.h>
#include <lux_video.h>

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

    /* base模块通用错误 */
#define BASE_COMM_ERR (-1)
#define alignment_down(a, size) (a & (~(size-1)) )
#define alignment_up(a, size)   ((a+size-1) & (~ (size-1)))

/*计时器个数*/
#define     LUX_BASE_TIEMR_NUM      16

/* 线程、线程锁 */
    typedef pthread_mutex_t OSI_MUTEX;

    /* 信号量 */
#define SEM_NO_WAIT      (0)    /* [in]  尝试获取，获取不到立刻返回 */
#define SEM_TRY_FAILED   (-2)   /* [out] 尝试获取，未获取到返回码 */

#define SEM_FOREVER      (-1)   /* [in]  阻塞式等待 */
#define SEM_WAIT_TIMEOUT (-3)   /* [out] 阻塞等待超时的返回码 */

    typedef sem_t OSI_SEM;

#define MIN_RR_PRIORITY    1           /* 实时轮转策略最低优先级 */
#define NORM_PRIORITY   0               /* 默认优先级 */
#define MAX_RR_PRIORITY    99          /* 实时轮转策略最高优先级 */
#define SECOND_RR_PRIORITY    98          /* 实时轮转策略第二优先级 */
    /* 线程调度结构体 */
    typedef struct
    {
        int policy;             /* 调度策略 */
        int priority;           /* 线程优先级 */
        pthread_t threadId;     /* 线程id */
    }ThrdSchd, * P_ThrdSchd;

    /* 函数定义 */
    int Show_Policy_Max_Min_Priority(int policy);
    /**
     * @description: 创建一个线程,并设置分离属性
     * @param [in] func：需要创建的线程函数
     * @param [in] arg：线程函数输入参数，可以为NULL
     * @param [in] thrdschd：线程调度参数
     * @return 0：成功，-1：失败；
     */
    int TaskPolicy_Prio_CreateThread(void* func, void* arg, ThrdSchd thrdschd);

    /**
 * @description: 创建一个实时调度策略线程,设置优先级，并设置分离属性
 * @param [in] func：需要创建的线程函数
 * @param [in] arg：线程函数输入参数，可以为NULL
 * @param [in] priority：实时调度策略线程优先级
 * @return 0：成功，-1：失败；
 */
    int TaskRR_Prio_CreateThread(void* func, void* arg, int priority);

    /**
     * @description: 获取线程调度策略及线程优先级
     * @param [in] p_thrdschd：线程调度参数指针
     * @return 0：成功，-1：失败；
     */
    int GetThread_Policy_Prio(P_ThrdSchd p_thrdschd);

    /**
     * @description: 设置线程调度策略及优先级
     * @param [in] thrdschd：线程调度参数
     * @return 0：成功，-1：失败；
     */
    int SetThread_Policy_Prio(ThrdSchd thrdschd);

    /**
 * @description: 设置实时调度策略线程优先级
 * @param [in] priority：实时调度策略线程优先级
 * @return 0：成功，-1：失败；
 */
    int SetThread_RR_Prio(int priority);

    /**
     * @description: 创建一个线程,并设置分离属性
     * @param [in] func：需要创建的线程函数
     * @param [in] arg：线程函数输入参数，可以为NULL
     * @return 0：成功，-1：失败；
     */
    int Task_CreateThread(void* func, void* arg);

    /**
     * @description: 创建互斥锁
     * @param [out] pMutex：互斥锁handle
     * @return 0：成功，-1：失败；
     */
    int Thread_Mutex_Create(OSI_MUTEX* pMutex);

    /**
     * @description: 销毁互斥锁
     * @param [in] pMutex：handle
     * @return 0：成功，-1：失败；
     */
    int Thread_Mutex_Destroy(OSI_MUTEX* pMutex);

    /**
     * @description: 互斥锁加锁
     * @param [in] pMutex：handle
     * @return 0：成功，-1：失败；
     */
    int Thread_Mutex_Lock(OSI_MUTEX* pMutex);

    /**
     * @description: 互斥锁解锁
     * @param [in] pMutex：handle
     * @return 0：成功，-1：失败；
     */
    int Thread_Mutex_UnLock(OSI_MUTEX* pMutex);

    /**
     * @description: 创建信号量,仅限单进程可用，多进程不可用
     * @param [in] value：创建资源个数，为0可当作互斥锁使用，大于0当作资源锁使用
     * @param [out] pSem：信号量handle
     * @return 0：成功，-1：失败
     */
    int Semaphore_Create(unsigned int value, OSI_SEM* pSem);

    /**
     * @description: 销毁信号量
     * @param [in] pSem：信号量handle
     * @return 0：成功，-1：失败
     */
    int Semaphore_Destroy(OSI_SEM* pSem);

    /**
     * @description: 释放信号量，+1操作
     * @param [in] pSem：信号量handle
     * @return 0：成功，-1：失败
     */
    int Semaphore_Post(OSI_SEM* pSem);

    /**
     * @description: 获取信号量，-1操作
     * @param [in] pSem：信号量handle
     * @param [in] waitMs：等待时间，-1：死等，0：立即返回，大于0：超时时间
     * @return 0：成功，-1：失败
     */
    int Semaphore_Pend(OSI_SEM* pSem, int waitMs);

    /**
     * @description: 保存时区时间差
     * @param void
     * @return [out] 时间：单位:S
     */
    void setTimeStampdDiff(int time_zone);

    /**
     * @description: 获取时区时间差
     * @param void
     * @return [out] 时间：单位:S
     */
    int getTimeStampdDiff(void);

    /**
     * @description: 获取当前时间戳
     * @param void
     * @return [out] 时间：单位:S
     */
    unsigned int gettimeStampS(void);

    /**
     * @description: 获取系统启动时间
     * @param void
     * @return [out] 时间：单位:S
     */
    unsigned int getTime_getS(void);

    /**
     * @description: 获取系统启动时间
     * @param void
     * @return [out] 时间：单位:ms
     */
    unsigned int getTime_getMs(void);

    /**
     * @description: 获取系统启动时间
     * @param void
     * @return [out] 时间：单位:us
     */
    unsigned long int getTime_getUs(void);

#define     LUX_BASE_QUE_MAXSIZE     64
#define     LUX_BASE_RECT_NUM_MAX    8

    /**
     * @description: 获取EventTime的值，时间戳
     * @return unsigned int
     */
    unsigned int GetEventTime();
    /**
     * @description: 设置当前时间为EventTime的值，时间戳
     * @return void
     */
    void SetEventTime();

    /******************************************************************
     *                                                                *
     *                         队列基本操作函数                        *
     *                                                                *
     ******************************************************************/
    typedef enum
    {
        LUX_Base_QUE_NONE,
        LUX_BASE_MOVDET_QUE_EN,
        LUX_BASE_PERSONDET_QUE_EN,
#ifdef CONFIG_PTZ_IPC
        LUX_BASE_PERSONTRACK_QUE_EN,
#endif
        LUX_BASE_QUE_BUTTON,

    }LUX_BASE_ALGOQUE_TYPE_EN;

    /*点的定义*/
    typedef struct {
        int x;    /**< 横向的坐标值 */
        int y;    /**< 纵向的坐标值 */

    }LUX_BASE_IVSPOINT_ST;

    /* 矩形框的定义*/
    typedef struct {
        LUX_BASE_IVSPOINT_ST  ul;    /**< 左上角点坐标 */
        LUX_BASE_IVSPOINT_ST  br;    /**< 右下角点坐标 */

    }LUX_BASE_IVSRECT_ST;

    typedef struct
    {
        int x; /**< 横向的坐标值 */
        int y; /**< 纵向的坐标值 */
    } LUX_IVS_Point;
    typedef struct
    {
        LUX_IVS_Point ul; /**< 左上角点坐标 */
        LUX_IVS_Point br; /**< 右下角点坐标 */
    } LUX_IVS_Rect;

    typedef struct
    {
        LUX_IVS_Rect box; /**reserved*/
        LUX_IVS_Rect show_box; /**< 人形区域坐标 */
        LUX_IVS_Rect pred_box; /**< 预验人形区域坐标 */
        float confidence; /**< 人形检测结果的置信度 */
        int id;
    } LUX_PERSON_INFO;

    typedef struct {
        LUX_IVS_Point det_rects[200]; /**< 检测出移动区域的宫格坐标 */
        int area[200];/**< 移动区域的area */
    }LUX_BASE_MOVE_REGINFO_ST;


    typedef void(*CallBackFunc)(void* pCallBackInfo);
    /*消息数据*/
    typedef struct
    {
        LUX_BASE_ALGOQUE_TYPE_EN   type;

        bool ivsStart[LUX_IVS_ALGO_NUM];                    /*该算法是否可用*/
        /*MOVE DETECT*/
        int   retRoi;                                       /*是否探测到*/
        int detcount;                                       /*检测出移动区域的宫格数量*/
        LUX_BASE_MOVE_REGINFO_ST    rectInfo;               /*检测初移动区域信息*/
        /*PERSON DETECT*/
        int rectcnt;                                        /*识别人形数量*/
        LUX_PERSON_INFO person[20];                         /*识别出的人形信息*/
        /*PERSON TRACKER*/
        void* IVSResult;                                    /*IVS算法处理结果*/
        void* pCallBackInfo;                                /*回调函数入口*/
        CallBackFunc pCallBackFunc;                         /*回调函数*/

    }LUX_BASE_IVSDATA_ST;


    /*队列结构体*/
    typedef struct
    {
        LUX_BASE_IVSDATA_ST IVSDetData[LUX_BASE_QUE_MAXSIZE];
        int front;
        int rear;

    }LUX_BASE_IVSQUE_ST;

    extern LUX_BASE_IVSQUE_ST g_ivs_que;
    extern OSI_MUTEX g_ivs_que_mux;
    /**
     * @description: 消息队列初始化
     * @return 0 成功，非零失败
     */
    int InitQueue();

    /**
     * @description: 消息队列长度
     * @param [in] pQue  队列
     * @return 0 成功，非零失败
     */
    int QueueLength(LUX_BASE_IVSQUE_ST* pQue);

    /**
     * @description: 消息入队
     * @param [in] pQue  队列
     * @param [in] pInData 出队数据
     * @return 0 成功，非零失败
     */
    int PushQueue(LUX_BASE_IVSQUE_ST* pQue, LUX_BASE_IVSDATA_ST* pInData);

    /**
     * @description: 消息出队
     * @param [in] pQue  队列
     * @param [out] pOutData 出队数据
     * @return 0 成功，非零失败
     */
    int PopQueue(LUX_BASE_IVSQUE_ST* pQue, LUX_BASE_IVSDATA_ST* pOutData);

    /*执行系统指令 */
    int LUX_BASE_System_CMD(char* pCmd);

    /* 检测相关文件的是否与版本匹配 */
    int LUX_BASE_Check_File_Vertion();

    /**
     * @description: 抓取高清图片
     * @param [in] picType 抓取照片的
     * @param [out] pPicInfo 抓取的图片数据
     * @return 0 成功，非零失败
     */
    int LUX_BASE_CapturePic_HD(LUX_STREAM_CHANNEL_EN picType, LUX_ENCODE_STREAM_ST* pPicInfo);


    /**
     * @description: 释放图片
     * @param [in] IvsResult IVS处理结果
     * @return 0 成功，非零失败
     */
    int LUX_BASE_ReleasePic_HD(LUX_STREAM_CHANNEL_EN picType, LUX_ENCODE_STREAM_ST* pPicInfo);
    /**
     * @description: 抓取图片
     * @param [in] picType 抓取照片的
     * @param [out] pPicInfo 抓取的图片数据
     * @return 0 成功，非零失败
     */
    int LUX_BASE_CapturePic(LUX_STREAM_CHANNEL_EN picType, LUX_ENCODE_STREAM_ST* pPicInfo);

    /**
     * @description: 释放图片
     * @param [in] IvsResult IVS处理结果
     * @return 0 成功，非零失败
     */
    int LUX_BASE_ReleasePic(LUX_STREAM_CHANNEL_EN picType, LUX_ENCODE_STREAM_ST* pPicInfo);



    /******************************************************************
     *                                                                *
     *                         计时器                                  *
     *                                                                *
     ******************************************************************/
     /*计时器绑定回调函数类型*/
    typedef void* (pFuncTimerProcessCB)(void* args);

    /*计时器结构体*/
    typedef struct
    {
        int bTimeStartEnable;           /*计时器开始执行时间间隔使能，开始执行时间间隔指的是多久后再执行process函数*/
        int     timeStartValue;         /*计时器开始执行时间间隔*/
        int     bIntervalEnable;        /*计时器循环使能*/
        unsigned int    timeInterval;   /*计时器循环时间间隔，单位：ms*/
        pFuncTimerProcessCB* pTimerProcess; /*计时器处理函数*/
        void* cbArgs;

    }LUX_BASE_TIMER_PARAM;

    /**
     * @description: 计时器初始化
     * @return 0 成功，非零失败
     */
    int LUX_BASE_TimeInit();

    /**
     * @description: 创建计时器
     * @param [in] pParam 计时器参数
     * @return 成功返回计时器ID, 失败返回-1
     */
    int LUX_BASE_TimerCreate(LUX_BASE_TIMER_PARAM* pParam);

    /**
     * @description: 使能计时器
     * @param [in] timerID 计时器id
     * @return 0 成功，非零失败
     */
    int LUX_BASE_TimerStart(int timerID);

    /**
     * @description: 销毁计时器
     * @param [in] timerID 计时器id
     * @return 0 成功，非零失败
     */
    int LUX_BASE_TimerDestroy(int timerID);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __LUX_BASE_H__ */
