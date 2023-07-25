#if 0
#ifndef __LUX_PUKEALG_H__
#define __LUX_PUKEALG_H__

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
#include "ssd_pukeDet.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /*__cplusplus*/

#define LUX_PUKE_CONFIDENCE 0.6

typedef struct 
{
    BOOL_X bInit;
    BOOL_X bStart;
    UINT_X alarmTime;
    UINT_X pukeAlarmTime;
    LUX_EVENT_ATTR_ST alarmCbk;
} LUX_ALG_PUKE_DET_ATTR;

extern LUX_FSRC_ATTR_ST g_framSrcAttr;

VOID_X LUX_ALG_PukeDet_Start();

VOID_X LUX_ALG_PukeDet_Stop();

INT_X LUX_ALG_PukeDet_Alarm();

INT_X LUX_ALG_PukeDet_Init();

INT_X LUX_ALG_PukeDet_GetResult(LUX_FACEDET_OUTPUT_ST* puke_result);

VOID_X puke_det_process_thread();

INT_X LUX_ALG_PukeDet_Exit();



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /*__cplusplus*/

#endif /*__LUX_PUKEALG_H__*/
#endif