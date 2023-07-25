/*
 * lux_video.h
 *
 * 视频模块，包括编解码码流获取转发，主要使用编码encoder
 * 
 */

#ifndef __LUX_VIDEO_H__
#define __LUX_VIDEO_H__

#include "comm_def.h"
#include "comm_func.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

typedef enum
{
  LUX_ENC_SLICE_SI = 4,         /**< AVC SI Slice */
  LUX_ENC_SLICE_SP = 3,         /**< AVC SP Slice */
  LUX_ENC_SLICE_GOLDEN = 3,     /**< Golden Slice */
  LUX_ENC_SLICE_I = 2,          /**< I Slice (can contain I blocks) */
  LUX_ENC_SLICE_P = 1,          /**< P Slice (can contain I and P blocks) */
  LUX_ENC_SLICE_B = 0,          /**< B Slice (can contain I, P and B blocks) */
  LUX_ENC_SLICE_CONCEAL = 6,    /**< Conceal Slice (slice was concealed) */
  LUX_ENC_SLICE_SKIP = 7,       /**< Skip Slice */
  LUX_ENC_SLICE_REPEAT = 8,     /**< Repeat Slice (repeats the content of its reference) */
  LUX_ENC_SLICE_MAX_ENUM,       /**< sentinel */
} LUX_ENCODE_SLICE_TYPE;

typedef struct 
{
    UINT_X chnId;
    VOID_X *pAddr;
    UINT_X streamLen;
    LUX_ENCODE_SLICE_TYPE type;
    UINT_X   pts;
    UINT64_X timeStamp;
    VOID_X  *pPrivData;    /* 私有数据，不对外使用 */
} LUX_ENCODE_STREAM_ST;

/**
 * @description: 请求通道I帧
 * @param [in] chnId：编码通道号
 * @return 成功：0，失败：错误码；
 */
INT_X LUX_Video_Encoder_RequestIDR(LUX_STREAM_CHANNEL_EN channel);

/**
 * @description: 刷新通道码流数据，丢掉旧的数据，重新开始
 * @param [in] chnId：编码通道号
 * @return 成功：0，失败：错误码；
 */
INT_X LUX_Video_Encoder_FlushStream(LUX_STREAM_CHANNEL_EN channel);

/**
 * @description: 获取编码帧
 * @param [in] chnId：编码通道号
 * @param [out] pOutStream：输出拼包转换后的码流结构
 * @return 成功：0，失败：错误码；
 */
INT_X LUX_Video_Encoder_GetStream(LUX_STREAM_CHANNEL_EN channel, LUX_ENCODE_STREAM_ST *pOutStream);

/**
 * @description: 释放编码帧
 * @param [in] chnId：编码通道号
 * @param [in] pStream：输出拼包转换后的码流结构
 * @return 成功：0，失败：错误码；
 */
INT_X LUX_Video_Encoder_ReleaseStream(LUX_STREAM_CHANNEL_EN channel, LUX_ENCODE_STREAM_ST *pStream);

/**
 * @description: 获取编码通道参数
 * @param [in] chnId：编码通道号
 * @param [out] pVencParam：编码通道属性参数
 * @return 成功：0，失败：错误码；
 */
INT_X LUX_Video_Encoder_GetChnParam(LUX_STREAM_CHANNEL_EN channel, LUX_VENC_CHN_PARAM_ST *pVencParam);

/**
 * @description: 开始编码
 * @param [in] chnId：编码通道号
 * @return 成功：0，失败：错误码；
 */
INT_X LUX_Video_Encoder_Start(LUX_STREAM_CHANNEL_EN channel);

/**
 * @description: 停止编码
 * @param [in] chnId：编码通道号
 * @return 成功：0，失败：错误码；
 */
INT_X LUX_Video_Encoder_Stop(LUX_STREAM_CHANNEL_EN channel);

/**
 * @description: 使能编码模块
 * @param [void]
 * @return 成功：0，失败：错误码；
 */
INT_X LUX_Video_Encoder_Init();

/**
 * @description: 销毁编码模块
 * @param [void]
 * @return 成功：0，失败：错误码；
 */
INT_X LUX_Video_Encoder_Exit();

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __LUX_VIDEO_H__ */
