/*************************************************************************
 > File Name: lux_actionAlg.h
 > Description: 
 > Author: cxj
 > Mail: Xiangjie.Chen@luxshare-ict.com
 > Created Time: Tue 06 Jun 2023 06:13:42 AM UTC
 ************************************************************************/
#if 0
#ifndef __LUX_ACTIONALG_H__
#define __LUX_ACTIONALG_H__

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
#include "ssd_actionDet.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /*__cplusplus*/

#define LUX_ACTION_CONFIDENCE 0.4
#define LUX_ACTION_TAG_NUM  5
#define LUX_ACTION_NUM_MAX  5

typedef enum
{
    LUX_ACITON_NONE,
    LUX_ACTION_YAWN,
    LUX_ACTION_SUCK_FINGERS,
    LUX_ACTION_RUB_EYES,
    LUX_ACTION_PULL_EARS,
    LUX_ACTION_KICK,
    LUX_ACTION_BUTTON,
} LUX_ACTION_TAG_EN;

typedef struct 
{
    BOOL_X bInit;
    BOOL_X bStart;
    UINT_X alarmTime;
    UINT_X actionAlarmTime;
    LUX_EVENT_ATTR_ST alarmCbk;
} LUX_ALG_ACTION_DET_ATTR;

extern LUX_FSRC_ATTR_ST g_framSrcAttr;

VOID_X LUX_ALG_ActionDet_Start();

VOID_X LUX_ALG_ActionDet_Stop();

INT_X LUX_ALG_ActionDet_Alarm();

INT_X LUX_ALG_ActionDet_Init();

INT_X LUX_ALG_ActionDet_GetResult(LUX_PERSONDET_OUTPUT_ST* action_result);

VOID_X action_det_process_thread();

INT_X LUX_ALG_ActionDet_Exit();


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /*__cplusplus*/

#endif /*__LUX_ACTIONALG_H__*/
#endif