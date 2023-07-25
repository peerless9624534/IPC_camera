#ifndef __LUX_FACEALG_H__
#define __LUX_FACEALG_H__

#include "imp_log.h"
#include "comm_def.h"
#include "lux_fsrc.h"
#include "lux_ivs.h"
#include "jz_faceDet.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /*__cplusplus*/

extern LUX_FSRC_ATTR_ST g_framSrcAttr;

INT_X LUX_ALG_FaceDet_Init();

INT_X LUX_ALG_FaceDet_GetResult(LUX_FACEDET_OUTPUT_ST* result);

INT_X LUX_ALG_FaceDet_Exit();

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /*__cplusplus*/

#endif /*__LUX_FACEALG_H__*/
