#include "lux_faceAlg.h"
#define TAG "lux_faceAlg"

jzdl::BaseNet* fDetModel;
static PUINT8_X pOutFrameData;
static PUINT8_X pRgbFace, pDstFace;

INT_X LUX_ALG_FaceDet_Init()
{
	/* Step.1 detection model init */
	fDetModel = jzdl::net_create();
	// printf("jzdl::BaseNet *fDetModel=jzdl::net_create();\n");

	int ret = jz_faceDet_init(fDetModel);
	// printf("ret = jz_faceDet_init(fDetModel) success;\n");
	if (ret < 0)
	{
		printf("jz_faceDet_init(fDetModel) failed\n");
		return LUX_ERR;
	}
	UINT_X size = SENSOR_WIDTH_SECOND * SENSOR_HEIGHT_SECOND * 3;
	pOutFrameData = new UINT8_X[size / 2];
	pRgbFace = new UINT8_X[size];
	pDstFace = new UINT8_X[size];
	return LUX_OK;
}

INT_X LUX_ALG_FaceDet_GetResult(LUX_FACEDET_OUTPUT_ST* result)
{
	INT_X ret = LUX_ERR;
	IMPFrameInfo frameData;
	UINT_X rgb_size = SENSOR_WIDTH_SECOND * SENSOR_HEIGHT_SECOND * 3;
	UINT_X yuv_size = rgb_size / 2;
	UINT_X nowTime = 0;

	memset(&frameData, 0, sizeof(IMPFrameInfo));
	memset(pOutFrameData, 0, yuv_size);
	Thread_Mutex_Lock(&g_framSrcAttr.fsChn[LUX_STREAM_SUB].mux);
	ret = IMP_FrameSource_SnapFrame(LUX_STREAM_SUB,
									PIX_FMT_NV12,
									SENSOR_WIDTH_SECOND,
									SENSOR_HEIGHT_SECOND,
									pOutFrameData,
									&frameData);
	Thread_Mutex_UnLock(&g_framSrcAttr.fsChn[LUX_STREAM_SUB].mux);
	if (LUX_OK != ret)
	{
		IMP_LOG_ERR(TAG, "%s(%d):IMP_FrameSource_SnapFrame(chn%d) faild\n", __func__, __LINE__, LUX_STREAM_SUB);
		return LUX_ERR;
	}

	memset(pRgbFace, 0, rgb_size);
	ret = LUX_NV12_C_RGB24(SENSOR_WIDTH_SECOND, SENSOR_HEIGHT_SECOND, (PUINT8_X)pOutFrameData, pRgbFace);
	if (LUX_OK != ret) {
		IMP_LOG_ERR(TAG, "LUX_NV12_C_RGB24 failed, error number [x0%x]\n", ret);
		return LUX_ERR;
	}

	memset(pDstFace, 0, rgb_size);
	ret = LUX_RGB24_NHWC_ARRAY(SENSOR_WIDTH_SECOND, SENSOR_HEIGHT_SECOND, pRgbFace, pDstFace, 3);
	if (LUX_OK != ret) {
		IMP_LOG_ERR(TAG, "LUX_RGB24_NHWC_ARRAY failed, error number [x0%x]\n", ret);
		return LUX_ERR;
	}


	nowTime = getTime_getMs();
	std::vector<objbox> face_list;
	face_list.clear();
	ret = jz_faceDet_start(fDetModel, face_list, pDstFace, SENSOR_WIDTH_SECOND, SENSOR_HEIGHT_SECOND);
	if (ret < 0)
	{
		printf("%d sample facedet start error\n", 1);
		return LUX_ERR;
	}
	// printf("\n\n****************************jz_faceDet_start time: %u(ms)****************************\n\n", getTime_getMs() - nowTime);

	result->count = (int)face_list.size();
	// printf("face_count:%d \n\n", result->count);
	UINT_X j;
	for (j = 0; j < face_list.size(); j++)
	{
		// printf("face_classID:%d \n", face_list[j].clsid);
		result->face[j].classid = face_list[j].clsid;
		result->face[j].x0 = (int)(face_list[j].x0);
		result->face[j].y0 = (int)(face_list[j].y0);
		result->face[j].x1 = (int)(face_list[j].x1);
		result->face[j].y1 = (int)(face_list[j].y1);
		result->face[j].confidence = face_list[j].score;
	}

	return LUX_OK;
}

INT_X LUX_ALG_FaceDet_Exit()
{
	delete pRgbFace;
	delete pDstFace;
	delete pOutFrameData;
	return jz_faceDet_exit(fDetModel);
}