/*
* lux_video.c
*
* 基于Ingenic平台编解码模块封装的视频功能，主要使用编码encoder
* 
*/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "comm_def.h"
#include "comm_error_code.h"
#include "comm_func.h"

#include "imp_encoder.h"
#include "imp_log.h"

#include "lux_video.h"
#include "lux_base.h"
#include <lux_iniparse.h>


#define TAG "lux_video"

//保存码流开关
//#define SAVE_STREAM

/* 编码通道数据流 */
#define LUX_VENC_CHN_TMPBUF_LEN (512 * 1024)
#define LUX_VENC_CHN_STREAM_CNT (8)

/* 编码通道控制流 */
#define LUX_VENC_GRP_NUM 2  /* 编码模块group数 */
#define LUX_VENC_CHN_NUM 2  /* 每个 group 可用的channel数 */

/* 编码模块通道属性 */
typedef struct 
{
    INT_X       chnId;      /* 通道ID号 */
    BOOL_X      bValid;     /* 通道是否有效，不使用此通道置为 FALSE，静态属性，不可更改 */
    BOOL_X      bStart;     /* 通道开启状态，TRUE 表示已开始编码 */
    OSI_MUTEX   chnMux;
    LUX_STREAM_CHANNEL_EN streamChn; /* 码流通道通道出流模式 */

    VOID_X *pTmpBuf;           /* 临时缓存，sdk获取的码流数据回头时，拼接使用 */
    UINT_X streamIdx;          /* 当前使用的 streamTmp 索引 */
    IMPEncoderStream streamTmp[LUX_VENC_CHN_STREAM_CNT]; /* 码流缓存个数，依赖于sdk定义的编码缓存个数 */
    IMPEncoderChnAttr chnAttr; /* 通道属性（IMP） */
} ENCODER_CHN_ATTR_ST;

/* 编码模块通道属性 */
typedef struct
{
    INT_X grpId;                     /* 组ID号 */
    INT_X             bindSrcChan;   /* 编码通道要绑定的源通道 */
    LUX_MODULS_DEV_EN bindSrcModule; /* 编码通道要绑定的源模块 */

    ENCODER_CHN_ATTR_ST encChnAttr[LUX_VENC_CHN_NUM]; /* grpId 下的通道，不同grp的chn号也是递增形式 */
} ENCODER_GRP_ATTR_ST;

/* 编码模块属性结构体 */
typedef struct 
{
    BOOL_X  bInit;  /* 模块初始化标识 */
    ENCODER_GRP_ATTR_ST encGrpAttr[LUX_VENC_GRP_NUM];
} LUX_VIDEO_ENC_ATTR_ST;

LUX_VIDEO_ENC_ATTR_ST g_VEncAttr =
{
    .bInit = LUX_FALSE,
    .encGrpAttr[0] =
    {
        .grpId = 0,
        .bindSrcModule = LUX_MODULS_FSRC,
        .bindSrcChan   = 0,
        .encChnAttr[0] = /* Group 0,channel 0 视频主码流：1080p */
        {
            .chnId   = CH0_INDEX,
            .bValid  = CHN0_EN,
            .streamChn = LUX_STREAM_MAIN,
            .chnMux = PTHREAD_MUTEX_INITIALIZER,
        },
    },
    .encGrpAttr[1] =
    {
        .grpId = 1,
        .bindSrcModule = LUX_MODULS_IVS,
        .bindSrcChan   = 0,
        .encChnAttr[0] = /* Group 1 channel 0 视频子码流：360p */
        {
            .chnId   = CH1_INDEX,
            .bValid  = CHN1_EN,
            .streamChn = LUX_STREAM_SUB,
            .chnMux = PTHREAD_MUTEX_INITIALIZER,

        },
        .encChnAttr[1] = /* Group 1 channel 1 抓图通道：360p */
        {
            .chnId   = CH2_INDEX,
            .bValid  = CHN2_EN,     //修改为0禁用图片流
            .streamChn = LUX_STREAM_JPEG,
            .chnMux = PTHREAD_MUTEX_INITIALIZER,

        },
    }
};


typedef struct
{
    INT_X eProfile;
    INT_X uLevel;
    INT_X uTier;
    INT_X ePicFormat;
    INT_X eEncOptions;
    INT_X eEncTools;

}LUX_VIDEO_ENCATTR_PARAM;

typedef struct
{
    INT_X rcMode;
    INT_X uTargetBitRate;
    INT_X uMaxBitRate;
    INT_X iInitialQP;
    INT_X iMinQP;
    INT_X iMaxQP;
    INT_X iIPDelta;
    INT_X iPBDelta;
    INT_X eRcOptions;
    INT_X uMaxPictureSize;
    INT_X uMaxPSNR;
    INT_X frmRateDen;

}LUX_VIDEO_RCATTR_PARAM;


typedef struct
{
    INT_X uGopCtrlMode;
    INT_X uGopLength;
    INT_X uNumB;
    INT_X uMaxSameSenceCnt;
    INT_X bEnableLT;
    INT_X uFreqLT;
    INT_X bLTRC;

}LUX_VIDEO_GOP_PARAM;

typedef struct
{
    LUX_VIDEO_ENCATTR_PARAM encAttr;
    LUX_VIDEO_RCATTR_PARAM rcAttr;
    LUX_VIDEO_GOP_PARAM gopAttr;

}LUX_VIDEO_PARAM_ST;


#if 0
/**
 * @description: 内部函数，配置编码器低码率编码属性
 * @param [in]  ChnAttr 通道属性
 * @param [in] uTargetBitRate：目标码率值
 * @return 成功：0，失败：错误码；
 */
static INT_X LUX_Video_Encoder_LowBitStream(IMPEncoderChnAttr *pChnAttr, UINT_X uTargetBitRate)
{
    IMPEncoderRcAttr *rcAttr = &pChnAttr->rcAttr;
    //uTargetBitRate /= 2;

    switch (rcAttr->attrRcMode.rcMode)
    {
        case IMP_ENC_RC_MODE_FIXQP:
            rcAttr->attrRcMode.attrFixQp.iInitialQP = 38;
            break;
        case IMP_ENC_RC_MODE_CBR:
            rcAttr->attrRcMode.attrCbr.uTargetBitRate = uTargetBitRate;
            rcAttr->attrRcMode.attrCbr.iInitialQP = -1;
            rcAttr->attrRcMode.attrCbr.iMinQP = 34;
            rcAttr->attrRcMode.attrCbr.iMaxQP = 51;
            rcAttr->attrRcMode.attrCbr.iIPDelta = -1;
            rcAttr->attrRcMode.attrCbr.iPBDelta = -1;
            rcAttr->attrRcMode.attrCbr.eRcOptions = IMP_ENC_RC_SCN_CHG_RES | IMP_ENC_RC_OPT_SC_PREVENTION;
            rcAttr->attrRcMode.attrCbr.uMaxPictureSize = uTargetBitRate * 4 / 3;
            break;
        case IMP_ENC_RC_MODE_VBR:
            rcAttr->attrRcMode.attrVbr.uTargetBitRate = uTargetBitRate;
            rcAttr->attrRcMode.attrVbr.uMaxBitRate = uTargetBitRate * 4 / 3;
            rcAttr->attrRcMode.attrVbr.iInitialQP = -1;
            rcAttr->attrRcMode.attrVbr.iMinQP = 34;
            rcAttr->attrRcMode.attrVbr.iMaxQP = 51;
            rcAttr->attrRcMode.attrVbr.iIPDelta = -1;
            rcAttr->attrRcMode.attrVbr.iPBDelta = -1;
            rcAttr->attrRcMode.attrVbr.eRcOptions = IMP_ENC_RC_SCN_CHG_RES | IMP_ENC_RC_OPT_SC_PREVENTION;
            rcAttr->attrRcMode.attrVbr.uMaxPictureSize = uTargetBitRate * 4 / 3;
            break;
        case IMP_ENC_RC_MODE_CAPPED_VBR:
            rcAttr->attrRcMode.attrCappedVbr.uTargetBitRate = uTargetBitRate;
            rcAttr->attrRcMode.attrCappedVbr.uMaxBitRate = uTargetBitRate * 4 / 3;
            rcAttr->attrRcMode.attrCappedVbr.iInitialQP = -1;
            rcAttr->attrRcMode.attrCappedVbr.iMinQP = 34;
            rcAttr->attrRcMode.attrCappedVbr.iMaxQP = 51;
            rcAttr->attrRcMode.attrCappedVbr.iIPDelta = -1;
            rcAttr->attrRcMode.attrCappedVbr.iPBDelta = -1;
            rcAttr->attrRcMode.attrCappedVbr.eRcOptions = IMP_ENC_RC_SCN_CHG_RES | IMP_ENC_RC_OPT_SC_PREVENTION;
            rcAttr->attrRcMode.attrCappedVbr.uMaxPictureSize = uTargetBitRate * 4 / 3;
            rcAttr->attrRcMode.attrCappedVbr.uMaxPSNR = 42;
            break; 
        case IMP_ENC_RC_MODE_CAPPED_QUALITY:
            rcAttr->attrRcMode.attrCappedQuality.uTargetBitRate = uTargetBitRate;
            rcAttr->attrRcMode.attrCappedQuality.uMaxBitRate = uTargetBitRate * 4 / 3;
            rcAttr->attrRcMode.attrCappedQuality.iInitialQP = -1;
            rcAttr->attrRcMode.attrCappedQuality.iMinQP = 34;
            rcAttr->attrRcMode.attrCappedQuality.iMaxQP = 51;
            rcAttr->attrRcMode.attrCappedQuality.iIPDelta = -1;
            rcAttr->attrRcMode.attrCappedQuality.iPBDelta = -1;
            rcAttr->attrRcMode.attrCappedQuality.eRcOptions = IMP_ENC_RC_SCN_CHG_RES | IMP_ENC_RC_OPT_SC_PREVENTION;
            rcAttr->attrRcMode.attrCappedQuality.uMaxPictureSize = uTargetBitRate * 4 / 3;
            rcAttr->attrRcMode.attrCappedQuality.uMaxPSNR = 42;
            break;
        case IMP_ENC_RC_MODE_INVALID:
            IMP_LOG_ERR(TAG, "unsupported rcmode:%d, we only support fixqp, cbr vbr and capped vbr\n", rcAttr->attrRcMode.rcMode);
            return LUX_VIDEO_INVALID_PARM;
    }
    
    return LUX_OK;
}
#endif


/**
 * @description: 内部函数，配置编码通道默认参数
 * @param [in] pChnAttr: 通道属性
 * @param [in] pImpChnAttr ：通道属性
 * @return 成功：0，失败：错误码；
 */
INT_X LUX_Video_Encoder_SetDefaultParam(ENCODER_CHN_ATTR_ST *pChnAttr, IMPEncoderChnAttr *pImpChnAttr)
{
    if (NULL == pChnAttr || NULL == pImpChnAttr)
    {
        IMP_LOG_ERR(TAG, "LUX_Video_Encoder_SetDefaultParam, null ptr!\n");
        return LUX_VIDEO_NULL_PRT;
    }
    INT_X ret = LUX_ERR;
    FLOAT_X ratio = 1.00;
    UINT_X uTargetBitRate = 0;
    UINT_X frmRateNum = 0;
    UINT_X frmRateDen = 0;
    INT_X chnWidth    = 0;
    INT_X chnHeight   = 0;

    /* 校验通道宽高 */
    chnWidth = (INT_X)((0 == pChnAttr->chnAttr.encAttr.uWidth) ? SENSOR_WIDTH_DEFAULT : pChnAttr->chnAttr.encAttr.uWidth);
    chnHeight = (INT_X)((0 == pChnAttr->chnAttr.encAttr.uHeight) ? SENSOR_HEIGHT_DEFAULT : pChnAttr->chnAttr.encAttr.uHeight);

    if (((uint64_t)chnWidth * chnHeight) > (1280 * 720))
    {
        ratio = log10f(((uint64_t)chnWidth * chnHeight) / (1280 * 720.0)) + 1;
    }
    else
    {
        ratio = 1.0 / (log10f((1280 * 720.0) / ((uint64_t)chnWidth * chnHeight)) + 1);
    }
    ratio = ratio > 0.1 ? ratio : 0.1;
    uTargetBitRate = BITRATE_720P_Kbs * ratio;

    frmRateNum = pChnAttr->chnAttr.rcAttr.outFrmRate.frmRateNum ? pChnAttr->chnAttr.rcAttr.outFrmRate.frmRateNum : 15;
    frmRateDen = pChnAttr->chnAttr.rcAttr.outFrmRate.frmRateDen ? pChnAttr->chnAttr.rcAttr.outFrmRate.frmRateDen : 1;

    /* 设置通道参数 */
    ret = IMP_Encoder_SetDefaultParam(pImpChnAttr, pChnAttr->chnAttr.encAttr.eProfile, pChnAttr->chnAttr.rcAttr.attrRcMode.rcMode,
                                      chnWidth, chnHeight, frmRateNum, frmRateDen, frmRateNum / frmRateDen, 2,
                                      (pChnAttr->chnAttr.rcAttr.attrRcMode.rcMode == IMP_ENC_RC_MODE_FIXQP) ? 35 : -1,
                                      uTargetBitRate);
    if (ret < 0)
    {
        IMP_LOG_ERR(TAG, "IMP_Encoder_SetDefaultParam error !\n");
        return LUX_VIDEO_CHAN_PARAM_ERR;
    }

#if 0    
    ret = LUX_Video_Encoder_LowBitStream(pImpChnAttr, uTargetBitRate);
    if (ret < 0)
    {
        IMP_LOG_ERR(TAG, "LUX_Video_Encoder_LowBitStream error ret:%#x\n", ret);
        return LUX_VIDEO_CHAN_PARAM_ERR;
    }
#endif
    return ret;
}

static VOID_X LUX_Video_Encoder_ChnAttr_DebugInfo(INT_X chnID, LUX_VIDEO_PARAM_ST *pChnConfig)
{
    printf("--------------------------------Video Encoder Attr--------------------------------------\n");
    printf("chn[%d]_encAttr_eProfile = [%d]\n", chnID, pChnConfig->encAttr.eProfile);
    printf("chn[%d]_encAttr_uLevel = [%d]\n", chnID, pChnConfig->encAttr.uLevel);
    printf("chn[%d]_encAttr_uTier = [%d]\n", chnID, pChnConfig->encAttr.uTier);
    printf("chn[%d]_encAttr_ePicFormat = [%d]\n", chnID, pChnConfig->encAttr.ePicFormat);
    printf("chn[%d]_encAttr_eEncOptions = [%d]\n", chnID, pChnConfig->encAttr.eEncOptions);
    printf("chn[%d]_encAttr_eEncTools = [%d]\n", chnID, pChnConfig->encAttr.eEncTools);
    printf("chn[%d]_rcAttr_rcMode = [%d]\n", chnID, pChnConfig->rcAttr.rcMode);
    printf("chn[%d]_rcAttr_uTargetBitRate = [%d]\n", chnID, pChnConfig->rcAttr.uTargetBitRate);
    printf("chn[%d]_rcAttr_uMaxBitRate = [%d]\n", chnID, pChnConfig->rcAttr.uMaxBitRate);
    printf("chn[%d]_rcAttr_iInitialQ = [%d]\n", chnID, pChnConfig->rcAttr.iInitialQP);
    printf("chn[%d]_rcAttr_iMinQP = [%d]\n", chnID, pChnConfig->rcAttr.iMinQP);
    printf("chn[%d]_rcAttr_iMaxQP = [%d]\n", chnID, pChnConfig->rcAttr.iMaxQP);
    printf("chn[%d]_rcAttr_iIPDelta = [%d]\n", chnID, pChnConfig->rcAttr.iPBDelta);
    printf("chn[%d]_rcAttr_iPBDelta = [%d]\n", chnID, pChnConfig->rcAttr.iPBDelta);
    printf("chn[%d]_rcAttr_eRcOptions = [%d]\n", chnID, pChnConfig->rcAttr.eRcOptions);
    printf("chn[%d]_rcAttr_uMaxPictureSize = [%d]\n", chnID, pChnConfig->rcAttr.uMaxPictureSize);
    printf("chn[%d]_rcAttr_uMaxPSNR = [%d]\n", chnID, pChnConfig->rcAttr.uMaxPSNR);
    printf("chn[%d]_rcAttr_frmRateDen = [%d]\n", chnID, pChnConfig->rcAttr.frmRateDen);
    printf("chn[%d]_gopAttr_uGopCtrlMode = [%d]\n", chnID, pChnConfig->gopAttr.uGopCtrlMode);
    printf("chn[%d]_gopAttr_uGopLength = [%d]\n", chnID, pChnConfig->gopAttr.uGopLength);
    printf("chn[%d]_gopAttr_uNumB = [%d]\n", chnID, pChnConfig->gopAttr.uNumB);
    printf("chn[%d]_gopAttr_uMaxSameSenceCnt = [%d]\n", chnID, pChnConfig->gopAttr.uMaxSameSenceCnt);
    printf("chn[%d]_gopAttr_bEnableLT = [%d]\n", chnID, pChnConfig->gopAttr.bEnableLT);
    printf("chn[%d]_gopAttr_uFreqLT = [%d]\n", chnID, pChnConfig->gopAttr.uFreqLT);
    printf("chn[%d]_gopAttr_bLTRC = [%d]\n", chnID, pChnConfig->gopAttr.bLTRC);

    return;
}


/**
 * @description: 获取配置编码器编码属性
 *  @param [in] chnID：通道号
 * @param [in]  ChnAttr 通道属性
 * @return 成功：0，失败：错误码；
 */
static INT_X LUX_Video_Encoder_GetChnConfig(INT_X chnID, LUX_VIDEO_PARAM_ST *pChnConfig)
{
    if(NULL == pChnConfig)
    {
        IMP_LOG_ERR(TAG, "LUX_Video_Encoder_SetChnParam, null ptr!\n");
        return LUX_VIDEO_NULL_PRT;
    }

    switch(chnID)
    {
        case CH0_INDEX:
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_encAttr_eProfile", &pChnConfig->encAttr.eProfile);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_encAttr_uLevel", &pChnConfig->encAttr.uLevel);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_encAttr_uTier",  &pChnConfig->encAttr.uTier);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_encAttr_ePicFormat", &pChnConfig->encAttr.ePicFormat);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_encAttr_eEncOptions", &pChnConfig->encAttr.eEncOptions);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_encAttr_eEncTools", &pChnConfig->encAttr.eEncTools);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_rcAttr_rcMode", &pChnConfig->rcAttr.rcMode);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_rcAttr_uTargetBitRate", &pChnConfig->rcAttr.uTargetBitRate);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_rcAttr_uMaxBitRate", &pChnConfig->rcAttr.uMaxBitRate);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_rcAttr_iInitialQP", &pChnConfig->rcAttr.iInitialQP);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_rcAttr_iMinQP", &pChnConfig->rcAttr.iMinQP);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_rcAttr_iMaxQP", &pChnConfig->rcAttr.iMaxQP);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_rcAttr_iIPDelta", &pChnConfig->rcAttr.iIPDelta);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_rcAttr_iPBDelta", &pChnConfig->rcAttr.iPBDelta);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_rcAttr_eRcOptions", &pChnConfig->rcAttr.eRcOptions);
            //LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_rcAttr_uMaxPictureSize", &pChnConfig->rcAttr.uMaxPictureSize);
            pChnConfig->rcAttr.uMaxPictureSize = 1792;
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_rcAttr_uMaxPSNR", &pChnConfig->rcAttr.uMaxPSNR);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_rcAttr_frmRateDen", &pChnConfig->rcAttr.frmRateDen);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_gopAttr_uGopCtrlMode", &pChnConfig->gopAttr.uGopCtrlMode);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_gopAttr_uGopLength", &pChnConfig->gopAttr.uGopLength);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_gopAttr_uNumB", &pChnConfig->gopAttr.uNumB);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_gopAttr_uMaxSameSenceCnt", &pChnConfig->gopAttr.uMaxSameSenceCnt);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_gopAttr_bEnableLT", &pChnConfig->gopAttr.bEnableLT);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_gopAttr_uFreqLT", &pChnConfig->gopAttr.uFreqLT);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn0_gopAttr_bLTRC", &pChnConfig->gopAttr.bLTRC);
            break;

        case CH1_INDEX:
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_encAttr_eProfile", &pChnConfig->encAttr.eProfile);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_encAttr_uLevel", &pChnConfig->encAttr.uLevel);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_encAttr_uTier",  &pChnConfig->encAttr.uTier);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_encAttr_ePicFormat", &pChnConfig->encAttr.ePicFormat);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_encAttr_eEncOptions", &pChnConfig->encAttr.eEncOptions);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_encAttr_eEncTools", &pChnConfig->encAttr.eEncTools);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_rcAttr_rcMode", &pChnConfig->rcAttr.rcMode);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_rcAttr_uTargetBitRate", &pChnConfig->rcAttr.uTargetBitRate);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_rcAttr_uMaxBitRate", &pChnConfig->rcAttr.uMaxBitRate);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_rcAttr_iInitialQP", &pChnConfig->rcAttr.iInitialQP);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_rcAttr_iMinQP", &pChnConfig->rcAttr.iMinQP);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_rcAttr_iMaxQP", &pChnConfig->rcAttr.iMaxQP);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_rcAttr_iIPDelta", &pChnConfig->rcAttr.iIPDelta);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_rcAttr_iPBDelta", &pChnConfig->rcAttr.iPBDelta);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_rcAttr_eRcOptions", &pChnConfig->rcAttr.eRcOptions);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_rcAttr_uMaxPictureSize", &pChnConfig->rcAttr.uMaxPictureSize);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_rcAttr_uMaxPSNR", &pChnConfig->rcAttr.uMaxPSNR);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_rcAttr_frmRateDen", &pChnConfig->rcAttr.frmRateDen);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_gopAttr_uGopCtrlMode", &pChnConfig->gopAttr.uGopCtrlMode);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_gopAttr_uGopLength", &pChnConfig->gopAttr.uGopLength);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_gopAttr_uNumB", &pChnConfig->gopAttr.uNumB);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_gopAttr_uMaxSameSenceCnt", &pChnConfig->gopAttr.uMaxSameSenceCnt);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_gopAttr_bEnableLT", &pChnConfig->gopAttr.bEnableLT);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_gopAttr_uFreqLT", &pChnConfig->gopAttr.uFreqLT);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn1_gopAttr_bLTRC", &pChnConfig->gopAttr.bLTRC);
            break;

        case CH2_INDEX:
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn2_encAttr_eProfile", &pChnConfig->encAttr.eProfile);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn2_encAttr_uLevel", &pChnConfig->encAttr.uLevel);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn2_encAttr_uTier",  &pChnConfig->encAttr.uTier);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn2_encAttr_ePicFormat", &pChnConfig->encAttr.ePicFormat);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn2_encAttr_eEncOptions", &pChnConfig->encAttr.eEncOptions);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn2_encAttr_eEncTools", &pChnConfig->encAttr.eEncTools);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn2_rcAttr_rcMode", &pChnConfig->rcAttr.rcMode);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn2_rcAttr_iInitialQP", &pChnConfig->rcAttr.iInitialQP);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn2_rcAttr_frmRateDen", &pChnConfig->rcAttr.frmRateDen);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn2_gopAttr_uGopCtrlMode", &pChnConfig->gopAttr.uGopCtrlMode);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn2_gopAttr_uGopLength", &pChnConfig->gopAttr.uGopLength);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn2_gopAttr_uNumB", &pChnConfig->gopAttr.uNumB);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn2_gopAttr_uMaxSameSenceCnt", &pChnConfig->gopAttr.uMaxSameSenceCnt);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn2_gopAttr_bEnableLT", &pChnConfig->gopAttr.bEnableLT);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn2_gopAttr_uFreqLT", &pChnConfig->gopAttr.uFreqLT);
            LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "video", "chn2_gopAttr_bLTRC", &pChnConfig->gopAttr.bLTRC);
            break;
        
        default:
            IMP_LOG_ERR(TAG, " invalid chnnal(%d)\n", chnID);
            return LUX_VIDEO_INVALID_PARM;
    }

    LUX_Video_Encoder_ChnAttr_DebugInfo(chnID, pChnConfig);

    return LUX_OK;
}

/**
 * @description: 配置编码器编码属性
 *  @param [in] chnID：通道号
 * @param [in]  ChnAttr 通道属性
 * @return 成功：0，失败：错误码；
 */
static INT_X LUX_Video_Encoder_SetChnParam(INT_X chnID, IMPEncoderChnAttr *pChnAttr)
{

    if(NULL == pChnAttr)
    {
        IMP_LOG_ERR(TAG, "LUX_Video_Encoder_SetChnParam, null ptr!\n");
        return LUX_VIDEO_NULL_PRT;
    }


#if 0 //默认配置
    INT_X ret = LUX_ERR;
    IMPEncoderChnAttr encChnAttr;
#endif
    int ret = -1;
    IMPFSChnAttr chnAttr = {0};
    LUX_VIDEO_PARAM_ST chnAttrParam;
    memset(&chnAttrParam, 0 ,sizeof(LUX_VIDEO_PARAM_ST));

    /*获取视频源的通道属性*/
    ret = IMP_FrameSource_GetChnAttr(chnID, &chnAttr);
    if(LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "LUX_Video_Encoder_SetChnParam, null ptr!\n");
    }

    LUX_Video_Encoder_GetChnConfig(chnID, &chnAttrParam);

    switch(chnID)
    {
        case CH0_INDEX:
            pChnAttr->encAttr.uWidth = (0 != chnAttr.picWidth) ? chnAttr.picWidth : SENSOR_WIDTH_DEFAULT;
            pChnAttr->encAttr.uHeight = (0 != chnAttr.picHeight) ? chnAttr.picHeight : SENSOR_HEIGHT_DEFAULT; 
            pChnAttr->rcAttr.outFrmRate.frmRateNum = VIDEO_ENC_MAIN_FRAMERATE;
            pChnAttr->encAttr.eProfile = chnAttrParam.encAttr.eProfile;
            pChnAttr->encAttr.uLevel = chnAttrParam.encAttr.uLevel;
            pChnAttr->encAttr.uTier = chnAttrParam.encAttr.uTier;
            pChnAttr->encAttr.ePicFormat = chnAttrParam.encAttr.ePicFormat;
            pChnAttr->encAttr.eEncOptions = chnAttrParam.encAttr.eEncOptions;
            pChnAttr->encAttr.eEncTools = chnAttrParam.encAttr.eEncTools;

            pChnAttr->rcAttr.attrRcMode.rcMode = chnAttrParam.rcAttr.rcMode;
            pChnAttr->rcAttr.attrRcMode.attrCappedQuality.uTargetBitRate = chnAttrParam.rcAttr.uTargetBitRate;
            pChnAttr->rcAttr.attrRcMode.attrCappedQuality.uMaxBitRate = chnAttrParam.rcAttr.uMaxBitRate;
            pChnAttr->rcAttr.attrRcMode.attrCappedQuality.iInitialQP = chnAttrParam.rcAttr.iInitialQP;
            pChnAttr->rcAttr.attrRcMode.attrCappedQuality.iMinQP = chnAttrParam.rcAttr.iMinQP;
            pChnAttr->rcAttr.attrRcMode.attrCappedQuality.iMaxQP = chnAttrParam.rcAttr.iMaxQP;
            pChnAttr->rcAttr.attrRcMode.attrCappedQuality.iIPDelta = chnAttrParam.rcAttr.iIPDelta;
            pChnAttr->rcAttr.attrRcMode.attrCappedQuality.iPBDelta = chnAttrParam.rcAttr.iPBDelta;
            pChnAttr->rcAttr.attrRcMode.attrCappedQuality.eRcOptions = chnAttrParam.rcAttr.eRcOptions;
            pChnAttr->rcAttr.attrRcMode.attrCappedQuality.uMaxPictureSize = chnAttrParam.rcAttr.uMaxPictureSize;
            pChnAttr->rcAttr.attrRcMode.attrCappedQuality.uMaxPSNR = chnAttrParam.rcAttr.uMaxPSNR;
            pChnAttr->rcAttr.outFrmRate.frmRateDen = chnAttrParam.rcAttr.frmRateDen;

            pChnAttr->gopAttr.uGopCtrlMode = chnAttrParam.gopAttr.uGopCtrlMode;
            pChnAttr->gopAttr.uGopLength = chnAttrParam.gopAttr.uGopLength;
            pChnAttr->gopAttr.uNumB = chnAttrParam.gopAttr.uNumB;
            pChnAttr->gopAttr.uMaxSameSenceCnt = chnAttrParam.gopAttr.uMaxSameSenceCnt;
            pChnAttr->gopAttr.bEnableLT = chnAttrParam.gopAttr.bEnableLT;
            pChnAttr->gopAttr.uFreqLT = chnAttrParam.gopAttr.uFreqLT;
            pChnAttr->gopAttr.bLTRC = chnAttrParam.gopAttr.bLTRC;
            break;

        case CH1_INDEX:
            pChnAttr->encAttr.uWidth = (0 != chnAttr.picWidth) ? chnAttr.picWidth : SENSOR_WIDTH_SECOND;
            pChnAttr->encAttr.uHeight = (0 != chnAttr.picHeight) ? chnAttr.picHeight : SENSOR_HEIGHT_SECOND;
            pChnAttr->rcAttr.outFrmRate.frmRateNum = VIDEO_ENC_MAIN_FRAMERATE;
            pChnAttr->encAttr.eProfile = chnAttrParam.encAttr.eProfile;
            pChnAttr->encAttr.uLevel = chnAttrParam.encAttr.uLevel;
            pChnAttr->encAttr.uTier = chnAttrParam.encAttr.uTier;
            pChnAttr->encAttr.ePicFormat = chnAttrParam.encAttr.ePicFormat;
            pChnAttr->encAttr.eEncOptions = chnAttrParam.encAttr.eEncOptions;
            pChnAttr->encAttr.eEncTools = chnAttrParam.encAttr.eEncTools;

            pChnAttr->rcAttr.attrRcMode.rcMode = chnAttrParam.rcAttr.rcMode;
            pChnAttr->rcAttr.attrRcMode.attrCappedQuality.uTargetBitRate = chnAttrParam.rcAttr.uTargetBitRate;
            pChnAttr->rcAttr.attrRcMode.attrCappedQuality.uMaxBitRate = chnAttrParam.rcAttr.uMaxBitRate;
            pChnAttr->rcAttr.attrRcMode.attrCappedQuality.iInitialQP = chnAttrParam.rcAttr.iInitialQP;
            pChnAttr->rcAttr.attrRcMode.attrCappedQuality.iMinQP = chnAttrParam.rcAttr.iMinQP;
            pChnAttr->rcAttr.attrRcMode.attrCappedQuality.iMaxQP = chnAttrParam.rcAttr.iMaxQP;
            pChnAttr->rcAttr.attrRcMode.attrCappedQuality.iIPDelta = chnAttrParam.rcAttr.iIPDelta;
            pChnAttr->rcAttr.attrRcMode.attrCappedQuality.iPBDelta = chnAttrParam.rcAttr.iPBDelta;
            pChnAttr->rcAttr.attrRcMode.attrCappedQuality.eRcOptions = chnAttrParam.rcAttr.eRcOptions;
            pChnAttr->rcAttr.attrRcMode.attrCappedQuality.uMaxPictureSize = chnAttrParam.rcAttr.uMaxPictureSize;
            pChnAttr->rcAttr.attrRcMode.attrCappedQuality.uMaxPSNR = chnAttrParam.rcAttr.uMaxPSNR;
            pChnAttr->rcAttr.outFrmRate.frmRateDen = chnAttrParam.rcAttr.frmRateDen;

            pChnAttr->gopAttr.uGopCtrlMode = chnAttrParam.gopAttr.uGopCtrlMode;
            pChnAttr->gopAttr.uGopLength = chnAttrParam.gopAttr.uGopLength;
            pChnAttr->gopAttr.uNumB = chnAttrParam.gopAttr.uNumB;
            pChnAttr->gopAttr.uMaxSameSenceCnt = chnAttrParam.gopAttr.uMaxSameSenceCnt;
            pChnAttr->gopAttr.bEnableLT = chnAttrParam.gopAttr.bEnableLT;
            pChnAttr->gopAttr.uFreqLT = chnAttrParam.gopAttr.uFreqLT;
            pChnAttr->gopAttr.bLTRC = chnAttrParam.gopAttr.bLTRC;
            break;

        case CH2_INDEX:
            pChnAttr->encAttr.uWidth = (0 != chnAttr.picWidth) ? chnAttr.picWidth : SENSOR_WIDTH_SECOND;
            pChnAttr->encAttr.uHeight = (0 != chnAttr.picHeight) ? chnAttr.picHeight : SENSOR_HEIGHT_SECOND;

            pChnAttr->encAttr.eProfile = chnAttrParam.encAttr.eProfile;
            pChnAttr->encAttr.ePicFormat = chnAttrParam.encAttr.ePicFormat;
            pChnAttr->encAttr.eEncOptions = chnAttrParam.encAttr.eEncOptions;
            pChnAttr->encAttr.eEncTools = chnAttrParam.encAttr.eEncTools;

            pChnAttr->rcAttr.attrRcMode.rcMode = chnAttrParam.rcAttr.rcMode;
            pChnAttr->rcAttr.attrRcMode.attrFixQp.iInitialQP = chnAttrParam.rcAttr.iInitialQP;
            pChnAttr->rcAttr.outFrmRate.frmRateNum = 5;
            pChnAttr->rcAttr.outFrmRate.frmRateDen = chnAttrParam.rcAttr.frmRateDen;

            pChnAttr->gopAttr.uGopCtrlMode = chnAttrParam.gopAttr.uGopCtrlMode;
            pChnAttr->gopAttr.uGopLength = chnAttrParam.gopAttr.uGopLength;
            pChnAttr->gopAttr.uNumB = chnAttrParam.gopAttr.uNumB;
            pChnAttr->gopAttr.uMaxSameSenceCnt = chnAttrParam.gopAttr.uMaxSameSenceCnt;
            pChnAttr->gopAttr.bEnableLT = chnAttrParam.gopAttr.bEnableLT;
            pChnAttr->gopAttr.uFreqLT = chnAttrParam.gopAttr.uFreqLT;
            pChnAttr->gopAttr.bLTRC = chnAttrParam.gopAttr.bLTRC;
            break;

#if 0 //默认配置
            memset(&encChnAttr, 0, sizeof(encChnAttr));
            ret = LUX_Video_Encoder_SetDefaultParam(&g_VEncAttr.encGrpAttr[1].encChnAttr[1], &encChnAttr);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "IMP_Encoder_CreateChn(%d) error ret：%#x!\n", ret);
                return LUX_VIDEO_CHAN_CREATE_ERR;
            }
            memcpy(pChnAttr, &encChnAttr, sizeof(IMPEncoderChnAttr));
            break;


#endif
        default:
            IMP_LOG_ERR(TAG, " invalid chnnal(%d)\n", chnID);
            return LUX_VIDEO_INVALID_PARM;
    }

    return LUX_OK;
}



/**
 * @description: 内部函数，创建编码器通道
 * @param [in] grpId：编码器组ID
 * @return 成功：0，失败：错误码；
 */
static INT_X LUX_Video_Encoder_CreateChn(UINT_X grpId)
{
    if (grpId >= LUX_VENC_GRP_NUM)
    {
        IMP_LOG_ERR(TAG, "LUX_Video_Encoder_CreateChn, param error grpId:%d!\n", grpId);
        return LUX_VIDEO_NULL_PRT;
    }
    
    UINT_X i = 0;
    INT_X ret = LUX_ERR;
    ENCODER_CHN_ATTR_ST *pChnAttr = NULL;

    for (i = 0; i < LUX_VENC_CHN_NUM; i++)
    {
        pChnAttr = &g_VEncAttr.encGrpAttr[grpId].encChnAttr[i];
        if (pChnAttr->bValid)
        {
   
#if 0
            /* 设置编码通道参数 */
            memset(&encChnAttr, 0, sizeof(encChnAttr));
            ret = LUX_Video_Encoder_SetDefaultParam(pChnAttr, &encChnAttr);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "IMP_Encoder_CreateChn(%d) error ret：%#x!\n", ret);
                return LUX_VIDEO_CHAN_CREATE_ERR;
            }
#endif
            /* 设置编码通道参数 */
            ret = LUX_Video_Encoder_SetChnParam(pChnAttr->chnId, &pChnAttr->chnAttr);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "IMP_Encoder_CreateChn(%d) error ret：%#x!\n", pChnAttr->chnId, ret);
                return LUX_VIDEO_CHAN_CREATE_ERR;
            }  

            /* 创建编码通道 */
            ret = IMP_Encoder_CreateChn(pChnAttr->chnId,  &pChnAttr->chnAttr);
            if (ret < 0)
            {
                IMP_LOG_ERR(TAG, "IMP_Encoder_CreateChn(%d,%d) error:%#x !\n", grpId, pChnAttr->chnId, ret);
                return LUX_VIDEO_CHAN_CREATE_ERR;
            }

            /* 注册编码通道 */
            ret = IMP_Encoder_RegisterChn(grpId, pChnAttr->chnId);
            if (ret < 0)
            {
                IMP_LOG_ERR(TAG, "IMP_Encoder_RegisterChn(%d,%d) error: %d\n", grpId, pChnAttr->chnId, ret);
                return LUX_VIDEO_CHAN_CREATE_ERR;
            }
#if 0
            /* gop属性 */
            if (pChnAttr->chnAttr.gopAttr.uGopLength)
            {
                ret = IMP_Encoder_SetChnGopLength(pChnAttr->chnId, pChnAttr->chnAttr.gopAttr.uGopLength);
                if (ret < 0)
                {
                    IMP_LOG_ERR(TAG, "IMP_Encoder_SetChnGopLength error !\n");
                }
                encChnAttr.gopAttr.uGopLength = pChnAttr->chnAttr.gopAttr.uGopLength;
            }
#endif
            /* 申请 LUX_VENC_CHN_TMPBUF_LEN 大小的通道码流缓存数据 */
            pChnAttr->pTmpBuf = malloc(LUX_VENC_CHN_TMPBUF_LEN);
            if (NULL == pChnAttr->pTmpBuf)
            {
                IMP_LOG_ERR(TAG, "malloc memory error !!!\n");
                return LUX_VIDEO_MALLOC_ERR;
            }
        }
    }

    return ret;
}

/**
 * @description: 内部函数，销毁编码器通道
 * @param [in] grpId：编码器组ID
 * @return 成功：0，失败：错误码；
 */
static INT_X LUX_Video_Encoder_DestroyChn(UINT_X grpId)
{
    if (grpId >= LUX_VENC_GRP_NUM)
    {
        IMP_LOG_ERR(TAG, "LUX_Video_Encoder_DestroyChn, param error grpId:%d!\n", grpId);
        return LUX_VIDEO_INVALID_PARM;
    }

    INT_X i = 0;
    INT_X ret = LUX_OK;
    IMPEncoderChnStat chnStat;
    ENCODER_CHN_ATTR_ST *pChnAttr = NULL;

    for (i = 0; i < LUX_VENC_CHN_NUM; i++)
    {
        pChnAttr = &g_VEncAttr.encGrpAttr[grpId].encChnAttr[i];

        /* 通道已经使能，释放掉资源 */
        if (pChnAttr->bValid)
        {
            /* 释放临时缓存 */
            if (pChnAttr->pTmpBuf)
            {
                free(pChnAttr->pTmpBuf);
            }

            memset(&chnStat, 0, sizeof(chnStat));
            ret = IMP_Encoder_Query(pChnAttr->chnId, &chnStat);
            if (ret < 0)
            {
                IMP_LOG_ERR(TAG, "IMP_Encoder_Query(%d) error: %d\n", grpId, ret);
                continue;
            }

            if (chnStat.registered)
            {
                ret = IMP_Encoder_UnRegisterChn(pChnAttr->chnId);
                if (ret < 0)
                {
                    IMP_LOG_ERR(TAG, "IMP_Encoder_UnRegisterChn(%d) error: %d\n", grpId, ret);
                    continue;
                }

                ret = IMP_Encoder_DestroyChn(pChnAttr->chnId);
                if (ret < 0)
                {
                    IMP_LOG_ERR(TAG, "IMP_Encoder_DestroyChn(%d) error: %d\n", grpId, ret);
                    continue;
                }
            }
        }
    }

    return ret;
}

#ifdef SAVE_STREAM  /* 测试代码 */
static int save_stream(int chnId, IMPEncoderStream *stream)
{
    int ret = 0;
    int i = 0;
    int nr_pack = stream->packCount;
    IMPEncoderPack *pack = NULL;
    uint32_t remSize = 0;
    char saveName[32] = {0};
    char *formatTable[3] = {"h264", "h264", "jpg"};
    static FILE *fp[VIDEO_ENC_CHN_MAX] = {NULL};

    if (!fp[chnId])
    {
        sprintf(saveName, "video_stream_chn%d.%s", chnId, formatTable[chnId]);

        fp[chnId] = fopen(saveName, "wb");
        if (NULL == fp[chnId])
            return -1;
    }

    for (i = 0; i < nr_pack; i++) 
    {
        pack = &stream->pack[i];

        if (pack->length)
        {
            remSize = stream->streamSize - pack->offset;
            if (remSize < pack->length)
            {
                ret = fwrite((void *)(stream->virAddr + pack->offset), 1, remSize, fp[chnId]);
                if (ret != remSize)
                {
                    IMP_LOG_ERR(TAG, "stream write ret(%d) != pack[%d].remSize(%d) len:%d\n", ret, i, remSize, ret);
                    return -1;
                }
                ret = fwrite((void *)stream->virAddr, 1, pack->length - remSize, fp[chnId]);
                if (ret != (pack->length - remSize))
                {
                    IMP_LOG_ERR(TAG, "stream write ret(%d) != pack[%d].(length-remSize)(%d) len:%d\n", ret, i, (pack->length - remSize), ret);
                    return -1;
                }
            }
            else
            {
                ret = fwrite((void *)(stream->virAddr + pack->offset), 1, pack->length, fp[chnId]);
                if (ret != pack->length)
                {
                    IMP_LOG_ERR(TAG, "stream write ret(%d) != pack[%d].length(%d) len:%d\n", ret, i, pack->length, ret);
                    return -1;
                }
            }
        }
    }

    return 0;
}

/* 测试代码，用来保存拼包后的stream */
static int save_pack_stream(UINT_X chnId, LUX_ENCODE_STREAM_ST *pStream)
{
    if (NULL == pStream)
    {
        return 0;
    }
    int ret = 0;
    char saveName[32] = {0};
    char *formatTable[3] = {"h265", "h265", "jpg"};
    static FILE *fp[VIDEO_ENC_CHN_MAX] = {NULL};
    if(0 == chnId)
    {
        if (!fp[chnId])
        {
            sprintf(saveName, "/tmp/video_pack_stream_chn%d.%s", chnId, formatTable[chnId]);

            fp[chnId] = fopen(saveName, "wb");
            if (NULL == fp[chnId])
            {
                return -1;
            }
        }

        ret = fwrite(pStream->pAddr, 1, pStream->streamLen, fp[chnId]);
        if (ret != pStream->streamLen)
        {
            IMP_LOG_ERR(TAG, "stream write ret(%d) streamLen(%d)\n", ret, pStream->streamLen);
            return -1;
        }
        fflush(fp[chnId]);
    }
    return 0;
}
#endif

/**
 * @description: 编码码流拼包
 * @param [in]  pStream：sdk获取的码流结构
 * @param [out] pOutStream：输出拼包转换后的码流结构
 * @return 成功：0，失败：错误码；
 */
static INT_X LUX_Video_Encoder_PackStream(IMPEncoderStream *pStream, LUX_ENCODE_STREAM_ST *pOutStream, VOID_X *pTmpBuf)
{
    if ((NULL == pStream) || (NULL == pOutStream) || (NULL == pTmpBuf))
    {
        IMP_LOG_ERR(TAG, "NULL PTR!\n");
        return LUX_VIDEO_NULL_PRT;
    }
    UINT_X i = 0;
    UINT_X streamLen = 0;
    BOOL_X bNeedCpyTemp = LUX_FALSE;
    UINT_X reminSize = 0;
    IMPEncoderPack *pPack = NULL;

    /* 计算是否需要拼包 */
    for (i = 0; i < pStream->packCount; i++)
    {
        if ((pStream->streamSize - pStream->pack[i].offset) < pStream->pack[i].length)
        {
            bNeedCpyTemp = LUX_TRUE;
            printf("================= bNeedCpyTemp =====================\n");
            break;
        }
    }

    /* 需要拷贝到tmp缓存，即需要拼包 */
    if (bNeedCpyTemp)
    {
        for (i = 0; i < pStream->packCount; i++)
        {
            pPack = &pStream->pack[i];
            reminSize = pStream->streamSize - pPack->offset;

            if (reminSize < pPack->length)
            {
                /* pTmpBuf 大小 LUX_VENC_CHN_TMPBUF_LEN （512K），够用 */
                memcpy((void*)(pTmpBuf + streamLen), (void*)(pStream->virAddr + pPack->offset), reminSize);
                streamLen += reminSize;

                memcpy((void*)(pTmpBuf + streamLen), (void *)pStream->virAddr, pPack->length - reminSize);
                streamLen += pPack->length - reminSize;
            }
            else
            {
                memcpy((void*)(pTmpBuf + streamLen), (void*)(pStream->virAddr + pPack->offset), pPack->length);
                streamLen += pPack->length;
            }
        }

        pOutStream->pAddr = pTmpBuf;
        pOutStream->streamLen = streamLen;
        pOutStream->type = pPack ? pPack->sliceType : 0;
        pOutStream->timeStamp = pPack ? pPack->timestamp : getTime_getUs();
    }
    else    /* 不需要拷贝拼包，直接统计码流总长度 */
    {
        for (i = 0; i < pStream->packCount; i++)
        {
            pPack = &pStream->pack[i];
            streamLen += pStream->pack[i].length;
        }

        /* 不拼包时，赋值使用sdk接口内部指针 */
        pOutStream->pAddr       = (void *)pStream->virAddr + pStream->pack[0].offset;
        pOutStream->streamLen   = streamLen;
        pOutStream->type        = pStream->pack[i-1].sliceType;
        pOutStream->timeStamp   = pStream->pack[i-1].timestamp;
    }
    if(pOutStream->streamLen > 384*1024)
    {
        if(LUX_ENC_SLICE_I == pOutStream->type)
        {
            printf("p_frame->pts : IIIIIIIIIIIIIIIIIII \n");
        }
        else
        {
            printf("p_frame->pts : p \n");
        }
        printf("p_frame->size : %d \n",pOutStream->streamLen);
        printf("p_frame->pts : %d \n",pStream->seq);
    }
    return LUX_OK;
}

/**
 * @description: 通过码流通道模式获取到实际编码器通道参数
 * @param [in]  streamChn：通道模式枚举
 * @param [out]  pAttr：通道属性结构体
 * @return 成功：通道号，失败：-1；
 */
static INT_X LUX_Video_Encoder_GetChnAttr(LUX_STREAM_CHANNEL_EN channel, ENCODER_CHN_ATTR_ST **ppAttr)
{
    INT_X i = 0;
    INT_X j = 0;
    INT_X ret = LUX_ERR;

    for (i = 0; i < LUX_VENC_GRP_NUM; i++)
    {
        for (j = 0; j < LUX_VENC_CHN_NUM; j++)
        {
            if (g_VEncAttr.encGrpAttr[i].encChnAttr[j].bValid
                && g_VEncAttr.encGrpAttr[i].encChnAttr[j].streamChn == channel)
            {
                *ppAttr = &g_VEncAttr.encGrpAttr[i].encChnAttr[j];
                ret = LUX_OK;
                break;
            }
        }
    }

    return ret;
}

/**
 * @description: 请求通道I帧
 * @param [in] chnId：编码通道号
 * @return 成功：0，失败：错误码；
 */
INT_X LUX_Video_Encoder_RequestIDR(LUX_STREAM_CHANNEL_EN channel)
{
    if (channel < 0 || channel >= LUX_STREAM_BUTT)
    {
        IMP_LOG_ERR(TAG, "Start Encoder ERR chnId:%d!\n", channel);
        return LUX_VIDEO_INVALID_PARM;
    }

    INT_X ret = LUX_OK;
    ENCODER_CHN_ATTR_ST *pAttr = NULL;

    /* 根据通道模式匹配编码器通道 */
    ret = LUX_Video_Encoder_GetChnAttr(channel, &pAttr);
    if (LUX_OK != ret || NULL == pAttr)
    {
        IMP_LOG_ERR(TAG, "Start Encoder ERR chnId:%d channel:%d!\n", pAttr->chnId, channel);
        return LUX_ERR;
    }

    /* 通道未开启 */
    if (!pAttr->bStart)
    {
        IMP_LOG_WARN(TAG, "ERR chnId:%d channel:%d bStart:%d\n", pAttr->chnId, channel, pAttr->bStart);
        return ret;
    }

    Thread_Mutex_Lock(&pAttr->chnMux);

    ret = IMP_Encoder_RequestIDR(pAttr->chnId);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "IMP_Encoder_RequestIDR(%d) timeout\n", pAttr->chnId);
        ret = LUX_VIDEO_REQUEST_IDR_ERR;
    }

    Thread_Mutex_UnLock(&pAttr->chnMux);

    IMP_LOG_INFO(TAG, "Request chan:%d,%d IDR Frame Success:%d!\n", pAttr->chnId, channel);

    return ret;
}

/**
 * @description: 刷新通道码流数据，丢掉旧的数据，重新开始
 * @param [in] chnId：编码通道号
 * @return 成功：0，失败：错误码；
 */
INT_X LUX_Video_Encoder_FlushStream(LUX_STREAM_CHANNEL_EN channel)
{
    if (channel < 0 || channel >= LUX_STREAM_BUTT)
    {
        IMP_LOG_ERR(TAG, "Start Encoder ERR chnId:%d!\n", channel);
        return LUX_VIDEO_INVALID_PARM;
    }
    INT_X ret = LUX_OK;
    ENCODER_CHN_ATTR_ST *pAttr = NULL;

    /* 根据通道模式匹配编码器通道 */
    ret = LUX_Video_Encoder_GetChnAttr(channel, &pAttr);
    if (LUX_OK != ret || NULL == pAttr)
    {
        IMP_LOG_ERR(TAG, "Start Encoder ERR chnId:%d channel:%d!\n", pAttr->chnId, channel);
        return LUX_ERR;
    }

    /* 通道未开启 */
    if (!pAttr->bStart)
    {
        IMP_LOG_WARN(TAG, "ERR chnId:%d channel:%d bStart:%d\n", pAttr->chnId, channel, pAttr->bStart);
        return ret;
    }

    Thread_Mutex_Lock(&pAttr->chnMux);

    ret = IMP_Encoder_FlushStream(pAttr->chnId);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "IMP_Encoder_RequestIDR(%d) timeout\n", pAttr->chnId);
        ret = LUX_VIDEO_REQUEST_IDR_ERR;
    }

    Thread_Mutex_UnLock(&pAttr->chnMux);

    IMP_LOG_INFO(TAG, "Flush chan:%d,%d Stream Success:%d!\n", pAttr->chnId, channel);

    return ret;
}

/**
 * @description: 获取编码帧
 * @param [in] chnId：编码通道号
 * @param [out] pOutStream：输出拼包转换后的码流结构
 * @return 成功：0，失败：错误码；
 */
INT_X LUX_Video_Encoder_GetStream(LUX_STREAM_CHANNEL_EN channel, LUX_ENCODE_STREAM_ST *pOutStream)
{
    if ((channel < 0 || channel >= LUX_STREAM_BUTT)  || NULL == pOutStream)
    {
        IMP_LOG_ERR(TAG, "Start Encoder ERR chnId:%d!\n", channel);
        return LUX_VIDEO_INVALID_PARM;
    }
    INT_X ret = LUX_OK;
    INT_X chnId = 0;
    UINT_X streamIdx = 0;
    IMPEncoderStream *pStream = NULL;
    ENCODER_CHN_ATTR_ST *pAttr = NULL;

    /* 根据通道模式匹配编码器通道 */
    ret = LUX_Video_Encoder_GetChnAttr(channel, &pAttr);
    if (LUX_OK != ret || NULL == pAttr)
    {
        IMP_LOG_ERR(TAG, "Start Encoder ERR chnId:%d channel:%d!\n", pAttr->chnId, channel);
        return LUX_ERR;
    }

    /* 通道未开启 */
    if (!pAttr->bStart)
    {
        return LUX_VIDEO_GET_STREAM_ERR;
    }

    Thread_Mutex_Lock(&pAttr->chnMux);
    do
    {
        /* 使用全局 streamIdx 变量记录取帧的位置，后续释放时需要 */
        chnId = pAttr->chnId;
        streamIdx = pAttr->streamIdx % LUX_VENC_CHN_STREAM_CNT;
        pStream = &pAttr->streamTmp[streamIdx];
        pAttr->streamIdx++;

        /* polling一帧数据 */
        ret = IMP_Encoder_PollingStream(chnId, 1000);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "IMP_Encoder_PollingStream(%d) timeout\n", chnId);
            ret = LUX_VIDEO_GET_STREAM_ERR;
            break;
        }

        /* Get H264 or H265 Stream */
        memset(pStream, 0, sizeof(IMPEncoderStream));
        ret = IMP_Encoder_GetStream(chnId, pStream, 1);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "IMP_Encoder_GetStream(%d) failed\n", chnId);
            ret = LUX_VIDEO_GET_STREAM_ERR;
            break;
        }

#ifdef SAVE_STREAM
        //save_stream(chnId, pStream);
#endif
        /* 打包码流包，存在一帧多包和码流缓冲回头情况，需要拼包 */
        ret = LUX_Video_Encoder_PackStream(pStream, pOutStream, pAttr->pTmpBuf);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "LUX_Video_Encoder_PackStream(%d) failed\n", chnId);
            ret = LUX_VIDEO_GET_STREAM_ERR;
            break;
        }

#ifdef SAVE_STREAM


        if((pStream->seq < 3000) &&(LUX_STREAM_MAIN == channel))
        {        
            char DateStr[40];
            time_t currTime;
            struct tm *currDate;
            time(&currTime);
            currTime = currTime + gettimeStampS();
            currDate = localtime(&currTime);
            memset(DateStr, 0, 40);
            strftime(DateStr, 40, "%Y-%m-%d %H:%M:%S", currDate);   /* %H:24小时制 %I:12小时制 */
            save_pack_stream(chnId, pOutStream);
            printf("[%s]save_pack_stream[%d]\n",DateStr,pStream->seq);
        }
#endif
        pOutStream->chnId = chnId;
        pOutStream->pts = pStream->seq;
        pOutStream->pPrivData = (VOID_X*)pStream;

    } while (0);
    Thread_Mutex_UnLock(&pAttr->chnMux);

    return ret;
}

/**
 * @description: 释放编码帧
 * @param [in] chnId：编码通道号
 * @param [in] pStream：输出拼包转换后的码流结构
 * @return 成功：0，失败：错误码；
 */
INT_X LUX_Video_Encoder_ReleaseStream(LUX_STREAM_CHANNEL_EN channel, LUX_ENCODE_STREAM_ST *pStream)
{
    if ((channel < 0 || channel >= LUX_STREAM_BUTT)
        || (NULL == pStream || NULL == pStream->pPrivData))
    {
        IMP_LOG_ERR(TAG, "Invalid param,channel:%d!\n", channel);
        return LUX_VIDEO_NULL_PRT;
    }
    UINT_X ret = LUX_ERR;
    ENCODER_CHN_ATTR_ST *pAttr = NULL;

    /* 根据通道模式匹配编码器通道 */
    ret = LUX_Video_Encoder_GetChnAttr(channel, &pAttr);
    if (LUX_OK != ret || NULL == pAttr)
    {
        IMP_LOG_ERR(TAG, "Start Encoder ERR chnId:%d channel:%d!\n", pAttr->chnId, channel);
        return LUX_ERR;
    }

    /* 通道未开启 */
    if (!pAttr->bStart)
    {
        return LUX_VIDEO_RLS_STREAM_ERR;
    }

    Thread_Mutex_Lock(&pAttr->chnMux);

    ret = IMP_Encoder_ReleaseStream(pStream->chnId, (IMPEncoderStream *)pStream->pPrivData);
    if (ret < 0)
    {
        IMP_LOG_ERR(TAG, "IMP_Encoder_ReleaseStream(%d) failed\n", pStream->chnId);
        ret = LUX_VIDEO_RLS_STREAM_ERR;
    }
    pStream->pPrivData = NULL;

    Thread_Mutex_UnLock(&pAttr->chnMux);

    return LUX_OK;
}

/**
 * @description: 获取编码通道参数
 * @param [in] chnId：编码通道号
 * @param [out] pVencParam：编码通道属性参数
 * @return 成功：0，失败：错误码；
 */
INT_X LUX_Video_Encoder_GetChnParam(LUX_STREAM_CHANNEL_EN channel, LUX_VENC_CHN_PARAM_ST *pVencParam)
{
    if ((channel < 0 || channel >= LUX_STREAM_BUTT) || NULL == pVencParam)
    {
        IMP_LOG_ERR(TAG, "invalid param channle:%d!\n", channel);
        return LUX_VIDEO_INVALID_PARM;
    }

    UINT_X ret = LUX_ERR;
    ENCODER_CHN_ATTR_ST *pAttr = NULL;
    IMPEncoderChnAttr impChnAttr;
    IMPEncoderEncType impEncType = IMP_ENC_TYPE_AVC;
    LUX_VIDEO_ENC_TYPE_EN encType = LUX_ENC_TYPE_H264;
    LUX_VENC_RC_MODE_EN encRcMode = LUX_VENC_RC_MODE_CBR;

    /* 根据通道模式匹配编码器通道 */
    ret = LUX_Video_Encoder_GetChnAttr(channel, &pAttr);
    if (LUX_OK != ret || NULL == pAttr)
    {
        IMP_LOG_ERR(TAG, "get chan attr ERR chnId:%d channel:%d!\n", pAttr->chnId, channel);
        return LUX_ERR;
    }

    Thread_Mutex_Lock(&pAttr->chnMux);
    do
    {
        /* 通道未开启 */
        if (!pAttr->bStart)
        {
            IMP_LOG_WARN(TAG, "ERR chnId:%d channel:%d bStart:%d\n", pAttr->chnId, channel, pAttr->bStart);
            break;
        }

        memset(&impChnAttr, 0, sizeof(impChnAttr));
        ret = IMP_Encoder_GetChnAttr(pAttr->chnId, &impChnAttr);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "get chan attr ERR chnId:%d channel:%d!\n", pAttr->chnId, channel);
            ret = LUX_VIDEO_GET_ATTR_ERR;
            break;
        }

        ret = IMP_Encoder_GetChnEncType(pAttr->chnId, &impEncType);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "get chan attr ERR chnId:%d channel:%d!\n", pAttr->chnId, channel);
            ret = LUX_VIDEO_GET_ATTR_ERR;
            break;
        }

        /* 编码类型转换 */
        if (IMP_ENC_TYPE_AVC == impEncType)
        {
            encType = LUX_ENC_TYPE_H264;
        }
        else if (IMP_ENC_TYPE_HEVC == impEncType)
        {
            encType = LUX_ENC_TYPE_H265;
        }
        else if (IMP_ENC_TYPE_JPEG == impEncType)
        {
            encType = LUX_ENC_TYPE_JPEG;
        }

        /* 码流控制类型转换 */
        if (IMP_ENC_RC_MODE_CBR == impChnAttr.rcAttr.attrRcMode.rcMode)
        {
            encRcMode = LUX_VENC_RC_MODE_CBR;
        }
        else if (IMP_ENC_RC_MODE_VBR == impChnAttr.rcAttr.attrRcMode.rcMode
                || IMP_ENC_RC_MODE_CAPPED_VBR == impChnAttr.rcAttr.attrRcMode.rcMode)
        {
            encRcMode = LUX_VENC_RC_MODE_VBR;
        }

        pVencParam->bEnable = LUX_TRUE;
        pVencParam->channel = channel;
        pVencParam->fps = impChnAttr.rcAttr.outFrmRate.frmRateDen ? 
                            (impChnAttr.rcAttr.outFrmRate.frmRateNum / impChnAttr.rcAttr.outFrmRate.frmRateDen) : 
                            VIDEO_ENC_MAIN_FRAMERATE;
        pVencParam->gop     = (UINT_X)impChnAttr.gopAttr.uGopLength;
        pVencParam->width   = (UINT_X)impChnAttr.encAttr.uWidth;
        pVencParam->height  = (UINT_X)impChnAttr.encAttr.uHeight;
        pVencParam->bitrate = (UINT_X)BITRATE_720P_Kbs;
        pVencParam->rcMode  = encRcMode;
        pVencParam->format  = encType;
    } while (0);

    Thread_Mutex_UnLock(&pAttr->chnMux);

    return ret;
}

/**
 * @description: 开始编码
 * @param [in] chnId：编码通道号
 * @return 成功：0，失败：错误码；
 */
INT_X LUX_Video_Encoder_Start(LUX_STREAM_CHANNEL_EN channel)
{
    INT_X ret = LUX_OK;
    ENCODER_CHN_ATTR_ST *pAttr = NULL;

    /* 根据通道模式匹配编码器通道 */
    ret = LUX_Video_Encoder_GetChnAttr(channel, &pAttr);
    if (LUX_OK != ret || NULL == pAttr)
    {
        IMP_LOG_ERR(TAG, "Start Encoder ERR chnId:%d channel:%d!\n", pAttr->chnId, channel);
        return LUX_ERR;
    }
    /* 重复start */
    if (pAttr->bStart)
    {
        IMP_LOG_WARN(TAG, "ERR chnId:%d channel:%d bStart:%d\n", pAttr->chnId, channel, pAttr->bStart);
        return LUX_VIDEO_CHAN_START_ERR;
    }

    Thread_Mutex_Lock(&pAttr->chnMux);
    do {
        ret = IMP_Encoder_FlushStream(pAttr->chnId);
        if (ret < 0)
        {
            IMP_LOG_ERR(TAG, "IMP_Encoder_FlushStream(%d) failed\n", pAttr->chnId);
        }

        ret = IMP_Encoder_StartRecvPic(pAttr->chnId);
        if (ret < 0)
        {
            IMP_LOG_ERR(TAG, "IMP_Encoder_StartRecvPic(%d) failed\n", pAttr->chnId);
            ret = LUX_VIDEO_CHAN_START_ERR;
            break;
        }
        pAttr->bStart = LUX_TRUE;
    } while (0);
    Thread_Mutex_UnLock(&pAttr->chnMux);

    return ret;
}

/**
 * @description: 停止编码
 * @param [in] chnId：编码通道号
 * @return 成功：0，失败：错误码；
 */
INT_X LUX_Video_Encoder_Stop(LUX_STREAM_CHANNEL_EN channel)
{
    INT_X ret = LUX_OK;
    ENCODER_CHN_ATTR_ST *pAttr = NULL;

    /* 根据通道模式匹配编码器通道 */
    ret = LUX_Video_Encoder_GetChnAttr(channel, &pAttr);
    if (LUX_OK != ret || NULL == pAttr)
    {
        IMP_LOG_ERR(TAG, "Start Encoder ERR chnId:%d channel:%d!\n", pAttr->chnId, channel);
        return LUX_ERR;
    }

    /* 重复stop */
    if (!pAttr->bStart)
    {
        IMP_LOG_ERR(TAG, "Start Encoder ERR chnId:%d channel:%d bStart:%d\n", pAttr->chnId, channel, pAttr->bStart);
        return LUX_VIDEO_CHAN_STOP_ERR;
    }

    Thread_Mutex_Lock(&pAttr->chnMux);
    do
    {
        ret = IMP_Encoder_StopRecvPic(pAttr->chnId);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "IMP_Encoder_StopRecvPic(%d) failed\n", pAttr->chnId);
            ret = LUX_VIDEO_CHAN_STOP_ERR;
            break;
        }
        pAttr->bStart = LUX_FALSE;
    } while (0);
    Thread_Mutex_UnLock(&pAttr->chnMux);

    return ret;
}

/**
 * @description: 使能编码模块
 * @param [void]
 * @return 成功：0，失败：错误码；
 */
INT_X LUX_Video_Encoder_Init()
{
    INT_X i   = 0;
    INT_X ret = LUX_ERR;
    LUX_MODULES_BIND_ST bind;
    CHAR_X sensorName[64] = {0};
	LUX_COMM_FUNC_SNS_WH sensorWH;

    if (g_VEncAttr.bInit)
    {
        IMP_LOG_ERR(TAG, "LUX_Video_Encoder_Init reInit, return OK!\n");
        return LUX_OK;
    }

    /*读取配置文件的sensor名字*/
	ret = LUX_INIParse_GetString(START_INI_CONFIG_FILE_NAME, "sensor", "name", sensorName);
	if(LUX_OK != ret){
		IMP_LOG_ERR(TAG, "LUX_INIParse_GetString failed,error [0x%x]\n",ret);
		return LUX_ERR;
	}

	/*获取镜头分辨率大小*/
	ret = LUX_COMM_GetSensorWH(sensorName, &sensorWH);
	if(LUX_OK != ret){
		IMP_LOG_ERR(TAG, "LUX_INIParse_GetString failed,error [0x%x]\n",ret);
		return LUX_ERR;
	}
    
    /*设置编码grop0，通道零0的编码分辨率大小为sensor分辨率大小*/
    g_VEncAttr.encGrpAttr[0].encChnAttr[0].chnAttr.encAttr.uWidth = sensorWH.width;
    g_VEncAttr.encGrpAttr[0].encChnAttr[0].chnAttr.encAttr.uHeight = sensorWH.height;

    /* video编码器 GROUP+CHAN 初始化 */
    for (i = 0; i < LUX_VENC_GRP_NUM; i++)
    {
        /* 创建 group ，用于传输码流和抓图 */
        ret = IMP_Encoder_CreateGroup(i);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "IMP_Encoder_CreateGroup(%d) error!\n", i);
            return LUX_VIDEO_CHAN_CREATE_ERR;
        }

        /* 绑定venc通道到osd通道 */
        memset(&bind, 0, sizeof(bind));
        bind.srcChan = i;
        bind.srcModule = LUX_MODULS_OSD;
        bind.dstChan = i;
        bind.dstModule = LUX_MODULS_VENC;
        ret = LUX_COMM_Bind_AddModule(&bind);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "LUX_COMM_Bind_AddModule(%d) venc error!\n", i);
            return LUX_VIDEO_CHAN_CREATE_ERR;
        }

        /* 创建 venc 通道, 将通道注册到 group */
        ret = LUX_Video_Encoder_CreateChn(i);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "LUX_Video_Encoder_CreateChn(%d) error!\n", i);
            return LUX_VIDEO_CHAN_CREATE_ERR;
        }
    }
    g_VEncAttr.bInit = LUX_TRUE;

    return LUX_OK;
}

/**
 * @description: 销毁编码模块
 * @param [void]
 * @return 成功：0，失败：错误码；
 */
INT_X LUX_Video_Encoder_Exit()
{
    INT_X ret = LUX_OK;
    INT_X i = 0;

    if (!g_VEncAttr.bInit)
    {
        IMP_LOG_ERR(TAG, "LUX_Video_Encoder_Exit, src not Init, return OK!\n");
        return LUX_OK;
    }

    for (i = 0; i < LUX_VENC_GRP_NUM; i++)
    {    
        /* 销毁video编码器 CHANNEL */
        ret = LUX_Video_Encoder_DestroyChn(i);
        if (ret < 0)
        {
            IMP_LOG_ERR(TAG, "LUX_Video_Encoder_DestroyChn(%d) error: %#x\n", i, ret);
            //return LUX_VIDEO_CHAN_DESTROY_ERR;
        }

        /* 销毁 video编码器的group */
        ret = IMP_Encoder_DestroyGroup(i);
        if (ret < 0)
        {
            IMP_LOG_ERR(TAG, "IMP_Encoder_DestroyGroup grpId(%d)\n", i);
            //return LUX_VIDEO_GROUP_DESTROY_ERR;
        }
    }
    g_VEncAttr.bInit = LUX_FALSE;

    return LUX_OK;
}
