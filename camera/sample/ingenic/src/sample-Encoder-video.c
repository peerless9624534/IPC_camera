/*
 * sample-Encoder-video.c
 *
 * Copyright (C) 2014 Ingenic Semiconductor Co.,Ltd
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <imp_log.h>
#include <imp_common.h>
#include <imp_system.h>
#include <imp_framesource.h>
#include <imp_encoder.h>
#include <imp_log.h>
#include <comm_def.h>
#include <comm_error_code.h>
#include <comm_func.h>
#include <lux_isp.h>
#include <lux_fsrc.h>
#include <lux_video.h>
#include <lux_base.h>
#include "lux_audio.h"
#include <lux_iniparse.h>
#include <lux_hal_led.h>
#include <lux_hal_lightsns.h>
#include <lux_hal_misc.h>
#include <lux_daynight.h>
#include <lux_ivs.h>
#include <lux_osd.h>
#include <lux_pwm.h>

#include <ivs_common.h>
#include <ivs_inf_move.h>
#include <ivs_interface.h>
#include <lux_hal_motor.h>

#include <lux_nv2jpeg.h>

#define TAG 	"Sample-Encoder-video"

 //#define TEST_DUMP_NV12

extern int rtsp_demo_rtsp_demo_main();
extern int tuya_demo_main(char* inputToken);
extern BOOL_X g_halTempHumiEnable;
extern BOOL_X g_pwmBlubEnable;
extern BOOL_X g_bcryDetctAlarmSong;

#ifdef TEST_DUMP_NV12
static void*
Therad_TestFunc(void* args)
{
    INT_X	ret = LUX_ERR;

    sleep(2);
    ret = LUX_FSrc_SaveDumpNV12(1, LUX_TRUE);
    if (LUX_OK != ret) {
        IMP_LOG_ERR(TAG, "save_dumpNV12Pic failed. error number [x0%x]\n", ret);
        return (void*)LUX_ERR;
    }

    return (void*)LUX_OK;
}

/**
 * @description: 创建一个线程,并设置分离属性
 * @return 0：成功，-1：失败；返回错误码;
 */
static INT_X SAMPLE_EcodeVideo_TestFunc(void)
{
    INT_X ret = LUX_ERR;
    ret = Task_CreateThread(Therad_TestFunc, NULL);
    if (LUX_OK != ret) {
        IMP_LOG_ERR(TAG, "Task_CreateThread failed.\n");
        return LUX_ERR;
    }

    return LUX_OK;
}
#endif
#if 1  /*包装过的sample*/
INT_X LUSHARE_test_sample_Encode_video(INT_X argc, PCHAR_X argv)
{
    INT_X ret = 0;
    BOOL_X luxLightStat = LUX_FALSE;
    BOOL_X bTempHumiStart = LUX_FALSE;
    BOOL_X bPwmStart = LUX_FALSE;
    BOOL_X bSongStart = LUX_FALSE;

    /*检测相关文件的是否与版本匹配*/
    // ret = LUX_BASE_Check_File_Vertion();
    // if (LUX_OK != ret) {
    // 	IMP_LOG_ERR(TAG, "LUX_BASE_Start_Wdt, return error[0x%x]\n",ret);
    // 	return LUX_ERR;
    // }

    /*设置打印属性*/
    IMP_Log_Set_Option(IMP_LOG_OP_DEFAULT);

    /*解析启动配置文件*/
    ret = LUX_INIParse_Load(START_INI_CONFIG_FILE_NAME);
    if (LUX_OK != ret) {
        IMP_LOG_ERR(TAG, "LUX_INIParse_Load, return error[0x%x]\n", ret);
        return LUX_ERR;
    }

    /*解析涂鸦配置文件*/
    ret = LUX_INIParse_Load(TUYA_INI_CONFIG_FILE_NAME);
    if (LUX_OK != ret) {
        IMP_LOG_ERR(TAG, "LUX_INIParse_Load, return error[0x%x]\n", ret);
        return LUX_ERR;
    }

    /*初始化计时器*/
    ret = LUX_BASE_TimeInit();
    if (LUX_OK != ret) {
        IMP_LOG_ERR(TAG, "LUX_BASE_TimeInit, return error[0x%x]\n", ret);
        return LUX_ERR;
    }

#ifdef CONFIG_PTZ_IPC
    /*PTZ电机初始化*/
    ret = LUX_HAL_Motor_CtrlInit();
    if (LUX_OK != ret) {
        IMP_LOG_ERR(TAG, "LUX_HAL_Motor_CtrlInit, return error[0x%x]\n", ret);
        return LUX_ERR;
    }

    /*巡航模式启动配置*/
    ret = LUX_HAL_MOTOR_CruiseModeConfig();
    if (LUX_OK != ret) {
        IMP_LOG_ERR(TAG, "LUX_HAL_MOTOR_CruiseModeConfig, return error[0x%x]\n", ret);
        return LUX_ERR;
    }

#endif
#if 0
    //TODO:TSET, wait to be deleted
    ret = LUX_HAL_MOTOR_SetCruiseMode(LUX_HAL_MOTOR_CRUISE_TIMING, 400);
    if (LUX_OK != ret) {
        IMP_LOG_ERR(TAG, "LUX_HAL_MOTOR_SetCruiseMode, return error[0x%x]\n", ret);
        return LUX_ERR;
    }
    while (1)
    {
        sleep(2);
    }
#endif

    ret = LUX_HAL_LedInit();
    if (0 != ret)
    {
        IMP_LOG_ERR(TAG, "LUX_HAL_LedInit failed! ");
    }

    /*根据涂鸦的配置led状态*/
    ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "hal", "led", &luxLightStat);
    if (0 != ret)
    {
        IMP_LOG_ERR(TAG, "LUX_INIParse_GetBool failed!, error num [0x%x] ", ret);
    }
    
    LUX_HAL_LedEnable(luxLightStat);

    ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "alarm_cradlesong_onoff", &bSongStart);
    if (bSongStart)
        g_bcryDetctAlarmSong = LUX_TRUE;
    else
        g_bcryDetctAlarmSong = LUX_FALSE;

#ifdef CONFIG_BABY
    BOOL_X sleepMode = LUX_FALSE;

    /* 根据温湿度报警状态配置led状态 */
    LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "blub_onoff", &bPwmStart);
    if (bPwmStart)
        g_pwmBlubEnable = LUX_TRUE;
    else
        g_pwmBlubEnable = LUX_FALSE;

    /* 隐私模式下不启用算法 */
    LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "sleep_mode", &sleepMode);
    if (LUX_FALSE == sleepMode)
    {
        /* 根据温湿度报警状态配置led状态 */
        LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "th_alarm_onoff", &bTempHumiStart);
        if (bTempHumiStart)
        {
            g_halTempHumiEnable = LUX_TRUE;
        }
    }
#endif
    /* Step.1 System init */
    ret = LUX_ISP_Init();
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "LUX_ISP_Init failed, return error[0x%x]\n", ret);
        return LUX_ERR;
    }

    /* Step.2 FrameSource init */
    ret = LUX_FSrc_Init();
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "FrameSource init failed\n");
        return LUX_ERR;
    }

    ret = LUX_IVS_Init();
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "Encoder init failed\n");
        return LUX_ERR;
    }

    /* Step.3 Encoder init */
    ret = LUX_Video_Encoder_Init();
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "Encoder init failed\n");
        return LUX_ERR;
    }

    /* Step.4 OSD init */
    ret = LUX_OSD_Init();
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "OSD init failed\n");
        return LUX_ERR;
    }

    /* AUDIO IN Init */
    ret = LUX_AUDIO_Init();
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "LUX_AI_Init failed\n");
        return LUX_ERR;
    }

    /* Step.4 Bind */
    ret = LUX_COMM_Bind_Connect();
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "Encoder init failed\n");
        return LUX_ERR;
    }

    /* Step.6 OSD time stamp update */
    ret = LUX_OSD_Time_Stamp_Update();
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "LUX_OSD_Time_Stamp_Update failed\n");
        return LUX_ERR;
    }

    // ret = LUX_OSD_FaceRect_Update();
    // if (LUX_OK != ret)
    // {
    //     IMP_LOG_ERR(TAG, "LUX_OSD_FaceRect_Update failed\n");
    //     return LUX_ERR;
    // }

    // ret = LUX_OSD_PersonRect_Update();
    // if (LUX_OK != ret)
    // {
    //     IMP_LOG_ERR(TAG, "LUX_OSD_PersonRect_Update failed\n");
    //     return LUX_ERR;
    // }

    ret = LUX_Video_Encoder_Start(LUX_STREAM_MAIN);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "ImpStreamOn failed chan:%d\n", LUX_STREAM_MAIN);
    }
    ret = LUX_Video_Encoder_Start(LUX_STREAM_SUB);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "ImpStreamOn failed chan:%d\n", LUX_STREAM_SUB);
    }
    // ret = LUX_Video_Encoder_Start(LUX_STREAM_JPEG);
    // if (LUX_OK != ret)
    // {
    //     IMP_LOG_ERR(TAG, "ImpStreamOn failed chan:%d\n", LUX_STREAM_JPEG);
    // }

    /* audio in start */
    ret = LUX_AI_Start();
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "ImpStreamOn failed\n");
        return LUX_ERR;
    }

    /* audio out start */
    ret = LUX_AO_Start();
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "ImpStreamOn failed\n");
        return LUX_ERR;
    }

#ifdef TEST_DUMP_NV12
    /*测试功能代码用的线程*/
    ret = (INT_X)SAMPLE_EcodeVideo_TestFunc();
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "SAMPLE_EcodeVideo_TestFunc failed\n");
        return LUX_ERR;
    }
#endif
#ifdef CONFIG_BABY
    ret = LUX_PWM_Init();
    if (0 != ret)
    {
        IMP_LOG_ERR(TAG, "LUX_PWM_Init failed!, error num [0x%x] \n", ret);
    }
#endif
    ret = LUX_DayNight_Init();
    if (0 != ret)
    {
        IMP_LOG_ERR(TAG, "LUX_DayNight_Init failed!, error num [0x%x] \n", ret);
    }

    ret = tuya_demo_main(NULL);
    IMP_LOG_ERR(TAG, "tuya_demo_main ret:%d\n", ret);

    ret = LUX_AO_Stop();
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "LUX_AO_Stop failed\n");
        return LUX_ERR;
    }

    ret = LUX_AI_Stop();
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "LUX_AI_Stop failed\n");
        return LUX_ERR;
    }

    ret = LUX_Video_Encoder_Stop(LUX_STREAM_MAIN);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "ImpStreamOff failed chan:%d\n", LUX_STREAM_MAIN);
    }
    ret = LUX_Video_Encoder_Stop(LUX_STREAM_SUB);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "ImpStreamOff failed chan:%d\n", LUX_STREAM_SUB);
    }
    // ret = LUX_Video_Encoder_Stop(LUX_STREAM_JPEG);
    // if (LUX_OK != ret)
    // {
    //     IMP_LOG_ERR(TAG, "ImpStreamOff failed chan:%d\n", LUX_STREAM_JPEG);
    // }

    /* Step.b UnBind */
    ret = LUX_COMM_Bind_DisConnect();
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "LUX_COMM_Bind_DisConnect failed\n");
        return LUX_ERR;
    }

    /* Step.c OSD exit */
    ret = LUX_OSD_Exit();
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "OSD init failed\n");
        return LUX_ERR;
    }

    LUX_AUDIO_Exit();

    /* Step.c Encoder exit */
    ret = LUX_Video_Encoder_Exit();
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "Encoder exit failed\n");
        return LUX_ERR;
    }

    /* Step.d FrameSource exit */
    ret = LUX_FSrc_Exit();
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "LUX_FSrc_Exit failed\n");
        return LUX_ERR;
    }

    /* Step.e System exit */
    ret = LUX_ISP_Exit();
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "LUX_ISP_Exit failed\n");
        return LUX_ERR;
    }

    return 0;
}
#endif  /*包装过的sample*/
