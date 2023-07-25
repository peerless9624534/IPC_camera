/*
 * lux_fsrc.h
 *
 * Ingenic 视频源模块
 */

#ifndef __LUX_FSRC_H__
#define __LUX_FSRC_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

#include <imp_common.h>
#include <imp_osd.h>
#include <imp_framesource.h>
#include <imp_isp.h>
#include <imp_encoder.h>

#include <comm_error_code.h>
#include <comm_def.h>
#include <lux_base.h>

/**
 * IMP帧图像信息定义.
 */
typedef struct {
	int index;			/**< 帧序号 */
	int pool_idx;		/**< 帧所在的Pool的ID */

	UINT_X width;			/**< 帧宽 */
	UINT_X height;			/**< 帧高 */
	UINT_X pixfmt;			/**< 帧的图像格式 */
	UINT_X size;			/**< 帧所占用空间大小 */

	UINT_X phyAddr;	/**< 帧的物理地址 */
	UINT_X virAddr;	/**< 帧的虚拟地址 */

	INT64_X timeStamp;	/**< 帧的时间戳 */
	UINT_X priv[0];	/* 私有数据 */
} LUX_DUMP_NV12_PIC_INFO_ST;

/*视频源模块通道属性*/
typedef struct
{
    UINT_X index; //0 for main channel ,1 for second channel
    UINT_X enable;
    OSI_MUTEX mux;
    IMPFSChnAttr fsChnAttr;
} LUX_FSRC_CHN_ATTR_ST;

/*视频源模块属性*/
typedef struct
{
    BOOL_X bInit;
    LUX_FSRC_CHN_ATTR_ST fsChn[FS_CHN_NUM];
} LUX_FSRC_ATTR_ST;

#define PIC_SAVE_PATH "/tmp"

INT_X convert_value(INT_X value);
INT_X save_dumpRGB24Pic(INT_X chnID, PVOID_X rgbData, IMPFrameInfo *frameInfo);
INT_X save_dumpNV12Pic(INT_X chnID, PVOID_X pOutFrameDate, LUX_DUMP_NV12_PIC_INFO_ST *frameInfo, BOOL_X enhanceEnable);
INT_X LUX_NV12_C_RGB24(UINT_X width , UINT_X height , PUINT8_X yuyv , PUINT8_X rgb);
INT_X LUX_RGB24_NHWC_ARRAY(UINT_X Width, UINT_X Height, PUINT8_X src_image, PUINT8_X dst_image, INT_X channel);

/**
 * @description: 获取指定通道的YUV一帧的图片数据,用于HD图像
 * @param [in]  chnID：frameSource通道号
 * @param [out]	picInfo	图片数据指针的大小
 * @return 成功：0，失败：错误码；
 */
INT_X LUX_FSrc_DumpNV12_HD(INT_X chnID,LUX_ENCODE_STREAM_ST *picInfo);

/**
 * @description: 获取指定通道的YUV一帧的图片数据
 * @param [in]  chnID：frameSource通道号
 * @param [in]  enhanceEnable, 是否开启图像增强, 0,关闭, 1开启
 * @return 成功：0，失败：错误码；
 */
INT_X LUX_FSrc_SaveDumpNV12(INT_X chnID, BOOL_X enhanceEnable);

/**
 * @description: 获取视频源通道图像
 * @param [in]	chnID	通道的编号
 * @param [out]	framedata	拷贝图像的内存指针
 * @param [out]	frame	获取到图像信息
 * @return 0 成功， 非零失败，返回错误码
 */
INT_X LUX_FSrc_DumpNV12(INT_X chnID, void *framedata, LUX_DUMP_NV12_PIC_INFO_ST *frame);

/**
 * @description: 视频源初始化
 * @return 0 成功， 非零失败，返回错误码
 */
INT_X LUX_FSrc_Init(void);

/**
 * @description: 视频源去初始化
 * @return 0 成功， 非零失败，返回错误码
 */
INT_X LUX_FSrc_Exit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __LUX_FSRC_H__ */
