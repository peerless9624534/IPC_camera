/*
 * lux_fsrc.c
 *
 *基于Ingenic封装的framesource模块
 * 
 */
#include <stdlib.h>
#include <string.h>
#include <imp_log.h>
#include <imp_common.h>
#include <imp_framesource.h>
#include <imp_encoder.h>

#include <lux_isp.h>
#include <lux_fsrc.h>
#include <comm_func.h>
#include <lux_iniparse.h>

#include <lux_nv2jpeg.h>

#define TAG "lux_fs"

#if 1
LUX_FSRC_ATTR_ST g_framSrcAttr =
{
    .bInit = LUX_FALSE,
    .fsChn =
    {
        {
            /* 通道0（主码流）视频源参数 */
            .index = CH0_INDEX,
            .enable = CHN0_EN,
            .fsChnAttr =
            {
                .pixFmt = PIX_FMT_NV12,
                .picWidth = SENSOR_WIDTH_DEFAULT,
                .picHeight = SENSOR_HEIGHT_DEFAULT,
                .outFrmRateNum = SENSOR_FRAME_RATE_NUM,
                .outFrmRateDen = SENSOR_FRAME_RATE_DEN,
                .nrVBs = 2,
                .type = FS_PHY_CHANNEL,
		   },
        },
        {
            /* 通道1（子码流）视频源参数 */
            .index = CH1_INDEX,
            .enable = CHN1_EN,
            .fsChnAttr =
            {
			    .pixFmt = PIX_FMT_NV12,
				.picWidth = SENSOR_WIDTH_SECOND,
	    		.picHeight = SENSOR_HEIGHT_SECOND,
		    	.outFrmRateNum = SENSOR_FRAME_RATE_NUM,
			    .outFrmRateDen = SENSOR_FRAME_RATE_DEN,
	    		.nrVBs = 2,
	    		.type = FS_PHY_CHANNEL,

				/*	//不需要裁剪
	    		.crop.enable = 0,
	    		.crop.top = 0,
	    		.crop.left = 0,
	    		.crop.width = SENSOR_WIDTH_SECOND,
	    		.crop.height = SENSOR_HEIGHT_SECOND,
				*/

	    		.scaler.enable = LUX_TRUE,
	    		.scaler.outwidth = SENSOR_WIDTH_SECOND,
	    		.scaler.outheight = SENSOR_HEIGHT_SECOND,
		   },
        },
	}
};
#endif

	/*********************************************************************************
	* Tuya qrcode image enhance
    * in_data  		input YUV420 ,from platform decode
	* int_w			input width	,from platform decode
	* in_h			input height ,from platform decode
	* out_data		output YUV420 ,send in zbar
	* out_w			output width ,send in zbar
	* out_h			output height ,send in zabr

	* binary_thres	image binary threshold，depends on different ISP para
					binary data range: 100-150, recommend 128. 
	* scale_flag	image scale, needed when Low-resolution images
					1: scale; 0: no scale
					if origin size 16*9, the output will be 32*18 when seting 1
					recommend original size 640*360, slower when scale to 1280*720.
	**********************************************************************************/
extern INT_X Tuya_Ipc_QRCode_Enhance(UCHAR_X *in_data, INT_X in_w, INT_X in_h, 
							  UCHAR_X **out_data, INT_X* out_w, INT_X* out_h, 
							  INT_X binary_thres, BOOL_X scale_flag);

INT_X convert_value(INT_X value)
{
	if (value > 255)
	{
		value = 255;
	}
	else if (value < 0)
	{
		value = 0;
	}
	return value;
}

INT_X LUX_NV12_C_RGB24(UINT_X width , UINT_X height , PUINT8_X yuyv , PUINT8_X rgb)
{
	if ((height == 0) || (0 == width))
	{
		IMP_LOG_ERR(TAG, "Pic height or width failed\n");
		return LUX_IVS_CAPTURE_PIC_ERR;
	}
	if ((yuyv == NULL) || (NULL == rgb))
	{
		IMP_LOG_ERR(TAG, "Pic yuv_img or rgb_img failed\n");
		return LUX_IVS_CAPTURE_PIC_ERR;
	}
	UINT8_X y, u, v;
    UINT_X  i, j, index = 0, rgb_index = 0;
    INT_X r, g, b, nv_index = 0;
	const INT_X nv_start = width * height;
 
    for(i = 0; i <  height; i++)
    {
		for(j = 0; j < width; j++){
			nv_index = i / 2  * width + j - j % 2;

			y = yuyv[rgb_index];
			v = yuyv[nv_start + nv_index ];
			u = yuyv[nv_start + nv_index + 1];

			r = y + ((360 * (v - 128) + 128) >> 8);
            g = y - (((88 * (u - 128) + 184 * (v - 128)) - 128) >> 8);
            b = y + ((455 * (u - 128) + 128) >> 8);
			
			index = rgb_index % width + i* width;
			rgb[index * 3+0] = convert_value(b);
			rgb[index * 3+1] = convert_value(g);
			rgb[index * 3+2] = convert_value(r);
			rgb_index++;
		}
    }
    return LUX_OK;
}

INT_X LUX_RGB24_NHWC_ARRAY(UINT_X Width, UINT_X Height, PUINT8_X src_image, PUINT8_X dst_image, INT_X channel)
{
	if ((Height == 0) || (0 == Width))
	{
		IMP_LOG_ERR(TAG, "Pic Height or Width failed\n");
		return LUX_IVS_CAPTURE_PIC_ERR;
	}
	if ((src_image == NULL) || (NULL == dst_image))
	{
		IMP_LOG_ERR(TAG, "Pic src_img or dst_image failed\n");
		return LUX_IVS_CAPTURE_PIC_ERR;
	}
	
	UINT_X h, w, k;
	UINT_X size = Height * Width;
	UINT_X Idx;
	
	for (k = 0; k < channel; ++k)
	{
		for(h = 0; h < Height; ++h)
		{
			for (w = 0; w < Width; ++w)
    		{
    		        Idx = k * size + h * Width + w;
    		        dst_image[Idx] = src_image[Idx];
    		}
		}
	}
	return LUX_OK;
}
/**
 * @description: 保存dump下来的图片数据
 * @param [in]	chnID	通道的编号
 * @param [in]	frameInfo	要保存的图片信息
 * @param [in]	enhanceEnable	是否开启QRcode图片增强，0关闭，1开启；
 * @return 0 成功， 非零失败，返回错误码
 */
// static INT_X save_dumpNV12Pic(INT_X chnID, void *pOutFrameDate, LUX_DUMP_NV12_PIC_INFO_ST *frameInfo, BOOL_X enhanceEnable)
INT_X save_dumpNV12Pic(INT_X chnID, PVOID_X pOutFrameDate, LUX_DUMP_NV12_PIC_INFO_ST *frameInfo, BOOL_X enhanceEnable)
{
	if(NULL == pOutFrameDate || NULL == frameInfo)
	{
		IMP_LOG_ERR(TAG, "pOutFrameDate or frameInfo point  error.\n");
		return LUX_PARM_NULL_PTR;
	}

	FILE *fp = NULL;
	// INT_X ret = LUX_ERR;
	CHAR_X saveName[64] = {0};
	UINT_X outFrameSize = frameInfo->width * frameInfo->height * 3 / 2;
	sprintf(saveName, "%s/dump_nv12_pic_chn%d.YUV", PIC_SAVE_PATH, chnID);
	fp = fopen(saveName, "wb");
	if (NULL == fp)
	{
		IMP_LOG_ERR(TAG, "fopen(%s, wb) failed.\n", saveName);
		return LUX_ERR;
	}
	fwrite(pOutFrameDate, outFrameSize, 1, fp);       
	// ret = fwrite(pOutFrameDate, 1, outFrameSize, fp);
	// if (ret != enhanceQRSize)
   	// {
    // 	IMP_LOG_ERR(TAG, "save_dumpNV12Pic write ret(%d) streamLen(%d)\n", ret, enhanceQRSize);
    // 	return LUX_ERR;
    // }
	fclose(fp);

	if(enhanceEnable){
		PUINT8_X enhanceQRYUV=NULL;
		INT_X enhanceQRWidth = 0;
		INT_X enhanceQRHeight = 0;
		INT_X enhanceQRSize = 0;
		CHAR_X saveNameEnhance[64] = {0};

		/*涂鸦二维码增强*/
		Tuya_Ipc_QRCode_Enhance(pOutFrameDate, frameInfo->width, frameInfo->height,
								&enhanceQRYUV, &enhanceQRWidth, &enhanceQRHeight, 128, 0);
		printf("enhance_qrcode_width = [%d] \n", enhanceQRWidth);
		printf("enhance_qrcode_height = [%d] \n", enhanceQRWidth);
		enhanceQRSize = enhanceQRWidth * enhanceQRWidth * 3 / 2;
		enhanceQRYUV = (PUINT8_X)malloc(enhanceQRSize);
		if(NULL == enhanceQRYUV)
		{
			IMP_LOG_ERR(TAG, "Tuya_Ipc_QRCode_Enhance enhanceQRYUV point error.\n");
			return LUX_ERR;
		}

		FILE* fp_h = NULL;
		sprintf(saveNameEnhance, "/mnt/sdcard/dump_enhance_pic_chn%d.YUV", chnID);
		fp_h = fopen(saveNameEnhance, "wb");
		if (NULL == fp_h)
		{
			IMP_LOG_ERR(TAG, "fopen(%s, wb) failed.\n", saveNameEnhance);
			return LUX_ERR;
		}       
		fwrite(enhanceQRYUV, enhanceQRSize, 1, fp_h);
		// ret = fwrite(enhanceQRYUV, 1, enhanceQRSize, fp_h);
		// if (ret != enhanceQRSize)
   		// {
        // 	IMP_LOG_ERR(TAG, "save_dumpNV12Pic write ret(%d) streamLen(%d)\n", ret, enhanceQRSize);
        // 	return LUX_ERR;
    	// }
		fclose(fp_h);
		free(enhanceQRYUV);
	}
	
	return LUX_OK;
}

INT_X save_dumpRGB24Pic(INT_X chnID, PVOID_X rgbData, IMPFrameInfo *frameInfo)
{
	if(NULL == rgbData || NULL == frameInfo)
	{
		IMP_LOG_ERR(TAG, "rgbData or frameInfo point  error.\n");
		return LUX_PARM_NULL_PTR;
	}

	FILE *fp = NULL;
	CHAR_X saveName[64] = {0};
	UINT_X outFrameSize = frameInfo->width * frameInfo->height * 3;

	sprintf(saveName, "%s/dump_rgb24_pic_chn%d.RGB", PIC_SAVE_PATH, chnID);
	fp = fopen(saveName, "wb");
	if (NULL == fp)
	{
		IMP_LOG_ERR(TAG, "fopen(%s, wb) failed.\n", saveName);
		return LUX_ERR;
	}	
	fwrite(rgbData, outFrameSize, 1, fp);
	fclose(fp);

	return LUX_OK;
}


/**
 * @description: 获取指定通道的YUV一帧的图片数据,用于HD图像
 * @param [in]  chnID：frameSource通道号
 * @return 成功：0，失败：错误码；
 */
INT_X LUX_FSrc_DumpNV12_HD(INT_X chnID,LUX_ENCODE_STREAM_ST *picInfo)
{
	
	INT_X ret = LUX_ERR;
	LUX_DUMP_NV12_PIC_INFO_ST frameInfo;
	void *pOutFrameDate;
	if(NULL == picInfo)
	{
		IMP_LOG_ERR(TAG, "picInfo point  error.\n");
		return LUX_ERR;
	}

	pOutFrameDate = malloc(g_framSrcAttr.fsChn[chnID].fsChnAttr.picWidth * 
						   g_framSrcAttr.fsChn[chnID].fsChnAttr.picHeight * 2);
	if (NULL == pOutFrameDate) 
	{
		IMP_LOG_ERR(TAG, "pOutFrameDate malloc failed.\n");
		return LUX_ERR;
	}
	do
	{
		/*获取指定通道图片数据*/
		ret = LUX_FSrc_DumpNV12(chnID, pOutFrameDate, &frameInfo);
		if (LUX_OK != ret) {
			IMP_LOG_ERR(TAG, "LUX_FSrc_DumpNV12 failed, error number [x0%x]\n", ret);
			break;
		}
		
		ret = LUX_Jpeg_Encode(chnID,(uint8_t*)pOutFrameDate,frameInfo.size,picInfo);
		if (ret != LUX_OK)
		{
			IMP_LOG_ERR(TAG, "LUX_Jpeg_Encode Fail ret(%d)\n", ret);
			break;
		}
	} while (0);
	free(pOutFrameDate);
	return ret;
}

/**
 * @description: 获取指定通道的YUV一帧的图片数据
 * @param [in]  chnID：frameSource通道号
 * @param [in]  enhanceEnable, 是否开启图像增强, 0,关闭, 1开启
 * @return 成功：0，失败：错误码；
 */
INT_X LUX_FSrc_SaveDumpNV12(INT_X chnID, BOOL_X enhanceEnable)
{
	
	INT_X ret = LUX_ERR;
	LUX_DUMP_NV12_PIC_INFO_ST frameInfo;
	void *pOutFrameDate;

	pOutFrameDate = malloc(g_framSrcAttr.fsChn[chnID].fsChnAttr.picWidth * 
						   g_framSrcAttr.fsChn[chnID].fsChnAttr.picHeight * 2);
	if (NULL == pOutFrameDate) 
	{
		IMP_LOG_ERR(TAG, "pOutFrameDate malloc failed.\n");
		return LUX_PARM_NULL_PTR;
	}
	
	do
	{
		/*获取指定通道图片数据*/
		ret = LUX_FSrc_DumpNV12(chnID, pOutFrameDate, &frameInfo);
		if (LUX_OK != ret) {
			IMP_LOG_ERR(TAG, "LUX_FSrc_DumpNV12 failed, error number [x0%x]\n", ret);
			break;
		}

		printf("DAMP_NV12_PIC_INFO: frameInfo.index  = [%d]\n ",frameInfo.index);
		printf("DAMP_NV12_PIC_INFO: frameInf.height = [%d]\n ",frameInfo.height);
		printf("DAMP_NV12_PIC_INFO: frameInfo.width  = [%d]\n ",frameInfo.width);
		printf("DAMP_NV12_PIC_INFO: frameInfo.size   = [%d]\n ",frameInfo.size);

		/*保存图片*/
		ret = save_dumpNV12Pic(chnID, pOutFrameDate, &frameInfo, enhanceEnable);
		if (LUX_OK != ret) {
			IMP_LOG_ERR(TAG, "save_dumpNV12Pic failed, error number [x0%x]\n", ret);
			break;
		}
	} while (0);

	free(pOutFrameDate);
	return LUX_OK;
}	

/**
 * @description: 获取视频源通道图像
 * @param [in]	chnID	通道的编号
 * @param [out]	framedata	拷贝图像的内存指针
 * @param [out]	frame	获取到图像信息
 * @return 0 成功， 非零失败，返回错误码
 */
INT_X LUX_FSrc_DumpNV12(INT_X chnID, void *framedata, LUX_DUMP_NV12_PIC_INFO_ST *frame)
{
    if(NULL == framedata || NULL == frame)
    {
        IMP_LOG_ERR(TAG, "LUX_FSrc_DumpNV12 failed!\n");
        return LUX_PARM_NULL_PTR;
    }

	INT_X ret = LUX_ERR;
	IMPFrameInfo *TempFrame = (IMPFrameInfo *)frame;

    Thread_Mutex_Lock(&g_framSrcAttr.fsChn[chnID].mux);
    ret = IMP_FrameSource_SnapFrame(g_framSrcAttr.fsChn[chnID].index,
                                    g_framSrcAttr.fsChn[chnID].fsChnAttr.pixFmt,
                                    g_framSrcAttr.fsChn[chnID].fsChnAttr.picWidth,
                                    g_framSrcAttr.fsChn[chnID].fsChnAttr.picHeight,
                                    framedata, TempFrame);
	//printf("IMP_FrameSource_SnapFrame Done\n");
    Thread_Mutex_UnLock(&g_framSrcAttr.fsChn[chnID].mux);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "IMP_FrameSource_SnapFrame(chn%d) faild !\n",chnID);
        return LUX_ERR;
    }

    return LUX_OK;
}

/**
 * @description: 视频源初始化
 * @return 0 成功， 非零失败，返回错误码
 */
INT_X LUX_FSrc_Init(void)
{
	INT_X chnID = 0;
    INT_X ret = LUX_ERR;
    BOOL_X FlipStat;
    LUX_ISP_SNS_ATTR_EN senceMirFlip;
	CHAR_X sensorName[64] = {0};
	LUX_COMM_FUNC_SNS_WH sensorWH;


    if (LUX_TRUE == g_framSrcAttr.bInit)
    {
        IMP_LOG_ERR(TAG, "LUX_FSrc_Init has been initialized!\n");
        return LUX_FSRC_ALREADY_INIT;
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

	/*设置通道0的分辨率大小为镜头的分辨率大小*/
	g_framSrcAttr.fsChn[0].fsChnAttr.picWidth = sensorWH.width;
	g_framSrcAttr.fsChn[0].fsChnAttr.picHeight = sensorWH.height;

	for (chnID = 0; chnID < FS_CHN_NUM; chnID++) {
		if (g_framSrcAttr.fsChn[chnID].enable)
        {
            ret = IMP_FrameSource_CreateChn(g_framSrcAttr.fsChn[chnID].index,
                                           &g_framSrcAttr.fsChn[chnID].fsChnAttr);
            if(LUX_OK != ret){
                IMP_LOG_ERR(TAG, "IMP_FrameSource_CreateChn(chn%d) error !\n", chnID);
                return LUX_ERR;
			}

			ret = IMP_FrameSource_SetChnAttr(g_framSrcAttr.fsChn[chnID].index,
                                            &g_framSrcAttr.fsChn[chnID].fsChnAttr);
			if (LUX_OK != ret) {
				IMP_LOG_ERR(TAG, "IMP_FrameSource_SetChnAttr(chn%d) error !\n", chnID);
				return LUX_ERR;
			}

            /* 使能framesource通道，开始处理数据 */
            ret = IMP_FrameSource_EnableChn(g_framSrcAttr.fsChn[chnID].index);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "IMP_FrameSource_EnableChn(%d) error: %d\n",
                            g_framSrcAttr.fsChn[chnID].index, ret);
                return LUX_ERR;
            }

            ret = Thread_Mutex_Create(&g_framSrcAttr.fsChn[chnID].mux);
            if (LUX_OK != ret) {
				IMP_LOG_ERR(TAG, "Thread_Mutex_Create(chn%d) failed !\n", chnID);
				return LUX_ERR;
			}
        }
	}

    /*图像翻转设置, 根据涂鸦配置文件设置*/
    ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "video", "flip" , &FlipStat);
    if(0 != ret)
    {
        IMP_LOG_ERR(TAG,"LUX_INIParse_GetBool failed!, error num [0x%x] ", ret);
    }

    if(FlipStat)
    {
#ifdef CONFIG_PTZ_IPC
        senceMirFlip = LUX_ISP_SNS_MIRROR_FLIP;
#else
		senceMirFlip = LUX_ISP_SNS_NORMAL;
#endif
    }
    else
    {
#ifdef CONFIG_PTZ_IPC
        senceMirFlip = LUX_ISP_SNS_NORMAL;
#else
		senceMirFlip = LUX_ISP_SNS_MIRROR_FLIP;
#endif
    }


	//ret = LUX_ISP_SetSenceFlip(g_ispAttr.senceMirFlip);
    ret = LUX_ISP_SetSenceFlip(senceMirFlip);
	if (LUX_OK != ret){
        IMP_LOG_ERR(TAG, "LUX_ISP_SetSenceFlip failed, return error[0x%x]\n",ret);
        return LUX_ISP_INVALID_PARM;
    }

    g_framSrcAttr.bInit = LUX_TRUE;

    return LUX_OK;
}


/**
 * @description: 视频源去初始化
 * @return 0 成功， 非零失败，返回错误码
 */
INT_X LUX_FSrc_Exit(void)
{
	INT_X chnID = 0;
    INT_X ret = LUX_ERR;

    if (LUX_FALSE == g_framSrcAttr.bInit)
    {
        IMP_LOG_ERR(TAG, "LUX_FSrc_Init has not been initialized!\n");
        return LUX_FSRC_NO_INIT;
    }

	for (chnID = 0; chnID <  FS_CHN_NUM; chnID++)
    {
		if (g_framSrcAttr.fsChn[chnID].enable)
        {
            ret = IMP_FrameSource_DisableChn(g_framSrcAttr.fsChn[chnID].index);
            if (LUX_OK != ret)
            {
                IMP_LOG_ERR(TAG, "IMP_FrameSource_DisableChn(%d) error: %d\n",
                            g_framSrcAttr.fsChn[chnID].index, ret);
                return LUX_ERR;
            }

            /*Destroy channel */
			ret = IMP_FrameSource_DestroyChn(chnID);
			if (LUX_OK != ret) {
				IMP_LOG_ERR(TAG, "IMP_FrameSource_DestroyChn(%d) error: %d\n", chnID, ret);
				return LUX_ERR;
			}

            ret = Thread_Mutex_Destroy(&g_framSrcAttr.fsChn[chnID].mux);
            if (LUX_OK != ret) {
				IMP_LOG_ERR(TAG, "Thread_Mutex_Destroy(chn%d) failed.\n", chnID);
				return LUX_ERR;
			}
		}
	}

	return LUX_OK;
}
