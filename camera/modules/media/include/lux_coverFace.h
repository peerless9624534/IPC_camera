/*
 * lux_coverFace.h
 *
 * 遮脸检测
 *
 */

#ifndef LUX_COVER_DET_H
#define LUX_COVER_DET_H
#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

#include "lux_event.h"
#include "lux_fsrc.h"
#include "lux_ivs.h"
#include <ivs_inf_faceDet.h>

    #define TIME_OUT 1500
    #define LUX_IVS_FACE_CONFIDENCE 0.60 //定义面部识别置信度
    #define LUX_IVS_FACE_COVER_TIME  (60 * 1000) //1min检测不到人脸报警，unit:ms

    typedef struct
    {
        BOOL_X bInit;
        // BOOL_X bStart;
        BOOL_X bCoverFaceStart;

        int ivsGrpNum;
        int ivsChnNum;
        IMPCell srcCell;
        IMPCell dstCell;
        IMPIVSInterface* pInterface;
        OSI_MUTEX mutex;

        UINT_X alarmTime;
        UINT_X faceAlarmTime;
        LUX_EVENT_ATTR_ST alarmCbk;
    } LUX_IVS_FACE_DET_ATTR;

    int LUX_IVS_FaceDet_Init();
    int LUX_IVS_FaceDet_Exit();
    void LUX_IVS_FaceDet_Process();
    // int LUX_IVS_FaceDet_Start();
    // int LUX_IVS_FaceDet_Stop();
    void LUX_IVS_CoverFace_Start();
    void LUX_IVS_CoverFace_Stop();
    int LUX_IVS_CoverFaceAlarm();
    int LUX_IVS_CoverFace_Event();
    int LUX_IVS_CoverFace_SSDEvent(LUX_FACEDET_OUTPUT_ST *result);
    INT_X LUX_Face_Alg_Process(INT_X chnID, UINT_X width, UINT_X height, PVOID_X result);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* LUX_FACE_DET_H */
