/*
 * lux_sleepDet.h
 *
 * 遮脸检测
 * 
 */

#ifndef LUX_SLEEP_DET_H
#define LUX_SLEEP_DET_H

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

#include <ivs_inf_faceDet.h>
#include "lux_base.h"
#include "lux_event.h"
#include "lux_ivs.h"
#include "lux_coverFace.h"

#define LUX_SLEEP_FACE_PICK_TIMES       6              //用于处理人脸检测结果的次数间隔
#define LUX_SLEEP_FACE_RESET_TIMES      30             //当超过该次数，未识别到面部，则所有计数重置//认为面部丢失
#define LUX_SLEEP_FACE_AVERAGE_TIMES    20              //记录最近的面部次数
#define LUX_SLEEP_IS_FACE_VALUE         0.7             //当记录最近的面部次数平均值大于该值时，认为宝宝在画面中

#define LUX_SLEEP_PERSON_PICK_TIMES     15               //每几次的移动数据进行全部累加，依然为0认为未发生移动，其他认为发生移动
#define LUX_SLEEP_PERSON_MOVE_VALUE     1               //累加数量大于该值认为在运动
#define LUX_SLEEP_PERSON_ASLEEP_TIMES   60             //最近N次是否发生移动的次数，用于判断入睡
#define LUX_SLEEP_PERSON_AWAKE_TIMES    20              //最近N次是否发生移动的次数，用于睡醒
#define LUX_SLEEP_FORCE_AWAKE_TIMES     (12*60*60)      //强迫睡醒的时间
#define LUX_SLEEP_NO_PERSON_AWAKE_CNT   5               //判断睡醒时，多次无人脸且无人形，强迫睡醒的次数

#define LUX_SLEEP_STOP_VALUE            0.5             //定义最近的活动次数平均值大于该值时，认为宝宝在睡醒了
#define LUX_SLEEP_START_VALUE           0.2             //定义最近的活动次数平均值小于该值时，认为宝宝在睡着了
#define LUX_SLEEP_KEEP_SLEEP            240             //当入睡后，每多长时间，记录一次SleepKeep，用于数据统计

typedef struct 
{
    INT_X  countNoFace;                                 //记录未识别到人脸的次数
    INT_X  doCount;                                     //记录过滤的次数
    INT_X  dofaceDetCount;                              //记录处理检测结果的次数
    INT16_X  allStatus[LUX_SLEEP_FACE_AVERAGE_TIMES];     //记录最近N次是否出现面部
    BOOL_X faceStatus;                                  //当前面部在画面中

}LUX_IVS_SLEEP_FACE_ST;


typedef struct 
{
    INT_X  doCount;                                     //处理移动的次数
    INT_X  moveValue;                                   //最近LUX_SLEEP_PERSON_PICK_TIMES次移动的数量，为0认为未发生移动，其他认为发生移动
    INT_X  doPersonDetCount;                            //记录处理检测结果的次数
    INT16_X  asleepStatus[LUX_SLEEP_PERSON_ASLEEP_TIMES]; //记录最近的N次是否发生移动 ，发生为1，未发生为0
    INT16_X  awakeStatus[LUX_SLEEP_PERSON_AWAKE_TIMES];   //记录最近的N次是否发生移动 ，发生为1，未发生为0
}LUX_IVS_SLEEP_PERSON_ST;

typedef struct 
{
    BOOL_X bInit;
    BOOL_X bStart;

    LUX_IVS_SLEEP_FACE_ST face;
    LUX_IVS_SLEEP_PERSON_ST person;
    BOOL_X sleepStatus;                                  //当前的睡眠状态
    // UINT_X tmpStartTime;            //临时睡眠开始时间，当确定为睡眠时生效
    UINT_X sleepStartTime;           //最近一次睡眠开始时间
    UINT_X sleepStopTime;           //最近一次睡眠开始时间
    UINT_X sleepLength;                //最近一次睡眠时长
    UINT_X sleepKeepTime;               //用于记录保持睡眠的时间，超过LUX_SLEEP_KEEP_SLEEP
    UINT_X sleepNoPersonCnt;        //睡醒时无人次数判断
    LUX_EVENT_ATTR_ST alarmCbk;

}LUX_IVS_SLEEP_DET_ATTR;


int LUX_IVS_SleepDet_Init();
int LUX_IVS_SleepDet_Exit();
void LUX_IVS_SleepDet_Start();
void LUX_IVS_SleepDet_Stop();
int LUX_IVS_SleepDet_Face_Event(facedet_param_output_t *result);
int LUX_IVS_SleepDet_SSDFace_Event(LUX_FACEDET_OUTPUT_ST *result);
int LUX_IVS_SleepDet_PersonMov_Event(LUX_BASE_IVSDATA_ST *result);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* LUX_SLEEP_DET_H */