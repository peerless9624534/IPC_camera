/*************************************************************************
 > File Name: lux_kickquiltAlg.h
 > Description: 
 > Author: cxj
 > Mail: Xiangjie.Chen@luxshare-ict.com
 > Created Time: Sat 03 Jun 2023 06:14:06 AM UTC
 ************************************************************************/
#if 0
#ifndef __LUX_KICKQUILTALG_H__
#define __LUX_KICKQUILTALG_H__

#include <sys/prctl.h>
#include <thread>
#include "comm_func.h"
#include "imp_log.h"
#include "imp_common.h"
#include "lux_fsrc.h"
#include "lux_ivs.h"
#include "lux_osd.h"
#include "lux_event.h"
#include "lux_iniparse.h"
#include "ssd_kickquiltDet.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /*__cplusplus*/

#define LUX_KICKQUILT_CONFIDENCE 0.6

typedef struct 
{
    BOOL_X bInit;
    BOOL_X bStart;
    UINT_X alarmTime;
    UINT_X kickquiltAlarmTime;
    LUX_EVENT_ATTR_ST alarmCbk;
} LUX_ALG_KICKQUILT_DET_ATTR;

extern LUX_FSRC_ATTR_ST g_framSrcAttr;

VOID_X LUX_ALG_KickQuiltDet_Start();

VOID_X LUX_ALG_KickQuiltDet_Stop();

INT_X LUX_ALG_KickQuiltDet_Alarm();

INT_X LUX_ALG_KickQuiltDet_Init();

INT_X LUX_ALG_KickQuiltDet_GetResult(LUX_PERSONDET_OUTPUT_ST* kickquilt_result);

VOID_X kickquilt_det_process_thread();

INT_X LUX_ALG_KickQuiltDet_Exit();

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /*__cplusplus*/

#endif /*__LUX_KICKQUILTALG_H__*/
#endif