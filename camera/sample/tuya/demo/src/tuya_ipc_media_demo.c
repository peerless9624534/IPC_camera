/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *www.tuya.comm
  *FileName:    tuya_ipc_media_demo
**********************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/statfs.h>  
#include "tuya_ipc_common_demo.h"
#include "tuya_ipc_api.h"
#include "tuya_ipc_echo_show.h"
#include "tuya_ipc_chromecast.h"
#include "tuya_ipc_media_demo.h"
#include "tuya_ipc_stream_storage.h"
#include "tuya_ipc_cloud_storage.h"
#include "tuya_ring_buffer.h"
#include "tuya_ipc_dp_utils.h"

#include "comm_func.h"
#include "lux_event.h"
#include "lux_video.h"
#include "lux_audio.h"
#include "lux_base.h"
#include "lux_ivs.h"
#include "lux_hal_misc.h"
#include "lux_hal_th.h"
#include "lux_pwm.h"
#include "tuya_ipc_system_control_demo.h"
#include "lux_fsrc.h"

#include "rtsp_api.h"



IPC_MEDIA_INFO_S s_media_info;
OSI_SEM g_liveAudioSem;
OSI_SEM g_liveVideoMainSem;
OSI_SEM g_liveVideoSubSem;

extern VOID IPC_APP_set_factory();

/* Set audio and video properties */
VOID IPC_APP_Set_Media_Info(VOID)
{
    LUX_VENC_CHN_PARAM_ST chnParam;
    TUYA_CODEC_ID  encType = TUYA_CODEC_VIDEO_H264;

    memset(&s_media_info, 0 , sizeof(IPC_MEDIA_INFO_S));
    memset(&chnParam, 0, sizeof(LUX_VENC_CHN_PARAM_ST));

    LUX_Video_Encoder_GetChnParam(LUX_STREAM_MAIN, &chnParam);

    /* main stream(HD), video configuration*/
    /* NOTE
    FIRST:If the main stream supports multiple video stream configurations, set each item to the upper limit of the allowed configuration.
    SECOND:E_CHANNEL_VIDEO_MAIN must exist.It is the data source of SDK.
    please close the E_CHANNEL_VIDEO_SUB for only one stream*/

    if(LUX_ENC_TYPE_H264 == chnParam.format)
    {
        encType = TUYA_CODEC_VIDEO_H264;
    }
    else if(LUX_ENC_TYPE_H265 == chnParam.format)
    {
        encType = TUYA_CODEC_VIDEO_H265;
    }

    s_media_info.channel_enable[E_CHANNEL_VIDEO_MAIN] = TRUE;    /* Whether to enable local HD video streaming */
    s_media_info.video_fps[E_CHANNEL_VIDEO_MAIN] = chnParam.fps;     /* FPS */
    s_media_info.video_gop[E_CHANNEL_VIDEO_MAIN] = chnParam.gop;         /* GOP */
    s_media_info.video_bitrate[E_CHANNEL_VIDEO_MAIN] = TUYA_VIDEO_BITRATE_2M; /* Rate limit */
    s_media_info.video_width[E_CHANNEL_VIDEO_MAIN] = chnParam.width;                  /* Single frame resolution of width*/
    s_media_info.video_height[E_CHANNEL_VIDEO_MAIN] = chnParam.height;                       /* Single frame resolution of height */
    s_media_info.video_freq[E_CHANNEL_VIDEO_MAIN] = 90000; /* Clock frequency */
    s_media_info.video_codec[E_CHANNEL_VIDEO_MAIN] = encType; /* Encoding format */

    /* substream(HD), video configuration */
    /* Please note that if the substream supports multiple video stream configurations, please set each item to the upper limit of the allowed configuration. */
    LUX_Video_Encoder_GetChnParam(LUX_STREAM_SUB, &chnParam);
    if(LUX_ENC_TYPE_H264 == chnParam.format)
    {
        encType = TUYA_CODEC_VIDEO_H264;
    }
    else if(LUX_ENC_TYPE_H265 == chnParam.format)
    {
        encType = TUYA_CODEC_VIDEO_H265;
    }

    s_media_info.channel_enable[E_CHANNEL_VIDEO_SUB] = TRUE;     /* Whether to enable local SD video stream */
    s_media_info.video_fps[E_CHANNEL_VIDEO_SUB] = chnParam.fps;  /* FPS */
    s_media_info.video_gop[E_CHANNEL_VIDEO_SUB] = chnParam.gop;  /* GOP */
    s_media_info.video_bitrate[E_CHANNEL_VIDEO_SUB] = TUYA_VIDEO_BITRATE_768K; /* Rate limit */
    s_media_info.video_width[E_CHANNEL_VIDEO_SUB] = chnParam.width;             /* Single frame resolution of width */
    s_media_info.video_height[E_CHANNEL_VIDEO_SUB] = chnParam.height;           /* Single frame resolution of height */
    s_media_info.video_freq[E_CHANNEL_VIDEO_SUB] = 90000; /* Clock frequency */
    s_media_info.video_codec[E_CHANNEL_VIDEO_SUB] = encType; /* Encoding format */
    
    LUX_AUDIO_PARAM_ST audioParam;

    memset(&audioParam, 0, sizeof(audioParam));
    LUX_AUDIO_GetParam(&audioParam);

    /* Audio stream configuration. 
    Note: The internal P2P preview, cloud storage, and local storage of the SDK are all use E_CHANNEL_AUDIO data. */
    s_media_info.channel_enable[E_CHANNEL_AUDIO] = TRUE;         /* Whether to enable local sound collection */
    s_media_info.audio_codec[E_CHANNEL_AUDIO] = TUYA_CODEC_AUDIO_PCM;/* Encoding format */
    s_media_info.audio_sample[E_CHANNEL_AUDIO] = (TUYA_AUDIO_SAMPLE_E)audioParam.sampleRate;   /* Sampling Rate */
    s_media_info.audio_databits[E_CHANNEL_AUDIO] = (TUYA_AUDIO_DATABITS_E)audioParam.bitWidth; /* Bit width */
    s_media_info.audio_channel[E_CHANNEL_AUDIO] = TUYA_AUDIO_CHANNEL_MONO;                     /* channel */
    s_media_info.audio_fps[E_CHANNEL_AUDIO] = audioParam.fps;                            /* Fragments per second */

    Semaphore_Create(0, &g_liveAudioSem);
    Semaphore_Create(0, &g_liveVideoMainSem);
    Semaphore_Create(0, &g_liveVideoSubSem);

    PR_DEBUG("channel_enable:%d %d %d", s_media_info.channel_enable[0], s_media_info.channel_enable[1], s_media_info.channel_enable[2]);

    PR_DEBUG("fps:%u", s_media_info.video_fps[E_CHANNEL_VIDEO_MAIN]);
    PR_DEBUG("gop:%u", s_media_info.video_gop[E_CHANNEL_VIDEO_MAIN]);
    PR_DEBUG("bitrate:%u kbps", s_media_info.video_bitrate[E_CHANNEL_VIDEO_MAIN]);
    PR_DEBUG("video_main_width:%u", s_media_info.video_width[E_CHANNEL_VIDEO_MAIN]);
    PR_DEBUG("video_main_height:%u", s_media_info.video_height[E_CHANNEL_VIDEO_MAIN]);
    PR_DEBUG("video_freq:%u", s_media_info.video_freq[E_CHANNEL_VIDEO_MAIN]);
    PR_DEBUG("video_codec:%d", s_media_info.video_codec[E_CHANNEL_VIDEO_MAIN]);

    PR_DEBUG("audio_codec:%d", s_media_info.audio_codec[E_CHANNEL_AUDIO]);
    PR_DEBUG("audio_sample:%d", s_media_info.audio_sample[E_CHANNEL_AUDIO]);
    PR_DEBUG("audio_databits:%d", s_media_info.audio_databits[E_CHANNEL_AUDIO]);
    PR_DEBUG("audio_channel:%d", s_media_info.audio_channel[E_CHANNEL_AUDIO]);
}

/* 开启/关闭视频业务 */
void TUYA_APP_Enable_Video(BOOL_T bEnable)
{
    INT_T ret = 0;

    if (bEnable)
    {
        ret = LUX_Video_Encoder_Start(LUX_STREAM_MAIN);
        if (0 != ret)
        {
            PR_ERR("LUX_Video_Encoder_Start error:%x", ret);
        }
        ret = LUX_Video_Encoder_Start(LUX_STREAM_SUB);
        if (0 != ret)
        {
            PR_ERR("LUX_Video_Encoder_Start error:%x", ret);
        }
        // ret = LUX_Video_Encoder_Start(LUX_STREAM_JPEG);
        // if (0 != ret)
        // {
        //     PR_ERR("LUX_Video_Encoder_Start error:%x", ret);
        // }
    }
    else
    {
        ret = LUX_Video_Encoder_Stop(LUX_STREAM_MAIN);
        if (0 != ret)
        {
            PR_ERR("LUX_Video_Encoder_Stop error:%x", ret);
        }

        ret = LUX_Video_Encoder_Stop(LUX_STREAM_SUB);
        if (0 != ret)
        {
            PR_ERR("LUX_Video_Encoder_Stop error:%x", ret);
        }

        // ret = LUX_Video_Encoder_Stop(LUX_STREAM_JPEG);
        // if (0 != ret)
        // {
        //     PR_ERR("LUX_Video_Encoder_Stop error:%x", ret);
        // }
    }

    return ;
}
/* 开启/关闭音频业务 */
void TUYA_APP_Enable_Audio(BOOL_T bEnable)
{
    INT_T ret = 0;

    if (bEnable)
    {
        ret = LUX_AO_Start();
        if (0 != ret)
        {
            PR_ERR("LUX_AO_Start error:%x", ret);
        }
        ret = LUX_AI_Start();
        if (0 != ret)
        {
            PR_ERR("LUX_AI_Start error:%x", ret);
        }
    }
    else
    {
        ret = LUX_AO_Stop();
        if (0 != ret)
        {
            PR_ERR("LUX_AO_Stop error:%x", ret);
        }
        ret = LUX_AI_Stop();
        if (0 != ret)
        {
            PR_ERR("LUX_AI_Stop error:%x", ret);
        }
    }

    return ;
}

/*
 * The sample code simulates audio and video by reading and writing files in rawfiles.tar.gz
 */
#define AUDIO_FRAME_SIZE 640
#define AUDIO_FPS 25
#define VIDEO_BUF_SIZE	(1024 * 400) //Maximum frame

extern LUX_IVS_TIMEINTERVAL_EN g_alarmInterval; /* 报警时间间隔 */

static BOOL_T TUYA_APP_IsAlarmByInterval(UINT_X lastTime)
{
    UINT_X timeInterval = 0;
    UINT_X curTime = 0;
    BOOL_T bAlarm = FALSE;

    switch (g_alarmInterval)
    {
        case LUX_IVS_TIMEINTERVAL_1MIN:
            timeInterval = 60;
            break;
        case LUX_IVS_TIMEINTERVAL_3MIN:
            timeInterval = 3 * 60;
            break;
        case LUX_IVS_TIMEINTERVAL_5MIN:
            timeInterval = 5 * 60;
            break;
        default:
            timeInterval = 60;
            break;
    }

    curTime = getTime_getS();
    if (curTime - lastTime > timeInterval)
    {
        bAlarm = TRUE;
    }
    //printf("INFO: %s:%d timeInterval:%d curTime:%d lastTime:%d-%d bAlarm:%d\n",
    //       __func__, __LINE__, timeInterval, curTime, lastTime, *pLastTime, bAlarm);

    return bAlarm;
}
int LUX_stopEventRecordCB()
{
    //事件录制时间大于30秒，并且录制状态为未停止
    unsigned int NowTime = getTime_getMs();
    unsigned int EventTime = GetEventTime();
    printf("%s:%d NowTime:[%d] \n", __func__, __LINE__, NowTime);
    if(NowTime - EventTime >= LUX_COMM_EVENT_RECORD_TIME) 
    {
        if(E_STORAGE_STOP != tuya_ipc_ss_get_status())
        {
            tuya_ipc_ss_stop_event();
        }
    }
    return 0;
}
/* This is for demo only. Should be replace with real PCM/AAC/G711 encoder output */
void *thread_live_audio(void *arg)
{
    INT_T ret = 0;
    MEDIA_FRAME_S pcm_frame = {0};
    LUX_AUDIO_FRAME_ST frame;

    /* 阻塞等待mqtt上线并初始化完成 */
    Semaphore_Pend(&g_liveAudioSem, SEM_FOREVER);
        /*哭声检测*/
    LUX_AI_CryDet_Init();
    pcm_frame.type = E_AUDIO_FRAME;
    
    while(1)
    {
        memset(&frame, 0, sizeof(frame));
        ret = LUX_AI_GetStream(&frame);
        if (0 != ret)
        {
            usleep(15*1000);
            continue;
        }
        
        pcm_frame.size = frame.len;
        pcm_frame.p_buf = (BYTE_T*)frame.pData;
        pcm_frame.pts = frame.seq;
        pcm_frame.timestamp = frame.timeStamp;
        
        if (FALSE == IPC_APP_get_sleep_mode())
        {
            TUYA_APP_Put_Frame(E_CHANNEL_AUDIO, &pcm_frame);
        }

        LUX_AI_ReleaseStream(&frame);
        usleep(15 * 1000);
    }
    
    LUX_AI_CryDet_DeInit();
    
    pthread_exit(0);
}

void *thread_live_video(void* arg)
{
    INT_X ret = 0;
    LUX_STREAM_CHANNEL_EN channel = (LUX_STREAM_CHANNEL_EN)arg;
    MEDIA_FRAME_S h264_frame = {0};
    LUX_ENCODE_STREAM_ST luxFrame;

    /* 阻塞等待mqtt上线 */
    if (LUX_STREAM_MAIN == channel)
        Semaphore_Pend(&g_liveVideoMainSem, SEM_FOREVER);
    else if (LUX_STREAM_SUB == channel)
        Semaphore_Pend(&g_liveVideoSubSem, SEM_FOREVER);

#ifdef RTSP_ENABLE
    if (1 == channel)
        RTSP_Server_Init();
#endif

    while (1)
    {        
        ret = LUX_Video_Encoder_GetStream(channel, &luxFrame);
        if (0 != ret)
        {
            usleep(100*1000);
            continue;
        }
        
        if (luxFrame.type == LUX_ENC_SLICE_P)
        {
            h264_frame.type = E_VIDEO_PB_FRAME;
        }
        else
        {
            h264_frame.type = E_VIDEO_I_FRAME;
        }

        h264_frame.size = luxFrame.streamLen;
        h264_frame.p_buf = luxFrame.pAddr;
        h264_frame.pts = luxFrame.pts;
        h264_frame.timestamp = luxFrame.timeStamp;
        if (FALSE == IPC_APP_get_sleep_mode())
        {
            if (LUX_STREAM_MAIN == channel)
            {
                /* Send HD video data to the SDK */
                TUYA_APP_Put_Frame(E_CHANNEL_VIDEO_MAIN, &h264_frame);

            #ifdef RTSP_ENABLE
                RTSP_STREAM_S rtspStream;
                memcpy(&rtspStream, &h264_frame, sizeof(rtspStream));
                RTSP_Send2Client_Sync(&rtspStream);
            #endif

            }
            else if (LUX_STREAM_SUB == channel)
            {
                /* Send SD video data to the SDK */
                TUYA_APP_Put_Frame(E_CHANNEL_VIDEO_SUB, &h264_frame);
            }
        }

        LUX_Video_Encoder_ReleaseStream(channel, &luxFrame);

        usleep(15 * 1000);
    }

    pthread_exit(0);
}

/* 恢复出厂设置 */
void *thread_reset_key(void *arg)
{
    int cnt = 0;
    int keyTmp = 0;
    int last = getTime_getS();

    while (1)
    {
        keyTmp = LUX_Hal_Misc_ResetKey_GetStatus();
        if (keyTmp)
        {
            printf("%s last:%d cur:%d timeOut:%d\n", __func__, last, getTime_getS(), LUX_KEY_RESET_TIME_INTERVAL - cnt);
            if ((++cnt >= LUX_KEY_RESET_TIME_INTERVAL) && (getTime_getS() - last) > LUX_KEY_RESET_TIME_INTERVAL)
                IPC_APP_set_factory();
        }
        else
        {
            last = getTime_getS();
            cnt = 0;
        }
        sleep(1);
    }

    return NULL;
}

BOOL_X g_halTempHumiEnable = FALSE;

/* 温湿度异常报警、数值上报接口 */
void *thread_th_proc(void *arg)
{
    INT_T ret = 0;
    UINT_T lastTime = 0;
    LUX_HAL_TH_PARAM thParam;
    int curTemp = 0, lastTemp = 0;
    int curHumi = 0, lastHumi = 0;
    EVENT_ID event_id = 0;
    LUX_ENCODE_STREAM_ST capPic;
    LUX_BASE_TIMER_PARAM timerParam;

    while (1)
    {
        memset(&thParam, 0, sizeof(thParam));
        LUX_HAL_GetTemperatureAndHumidity(&thParam);

        /* 温湿度上报接口，数值为正常值的10倍 */
        curTemp = (int)((thParam.cTemp + 0.05) * 10 - 20); /* (x + 0.05)为四舍五入操作, -20 是温度校准 */
        if (abs(curTemp - lastTemp) >= 3)
        {
            //printf(">>>>>>>>>>>>>>>>>>>>>Temperature in Celsius : %f C \n", thParam.cTemp);
            IPC_APP_report_temperature_value(curTemp);
            lastTemp = curTemp;
        }
        curHumi = (int)((thParam.humidity + 0.05) * 10 + 50);
        if (abs(curHumi - lastHumi) >= 5)
        {
            //printf(">>>>>>>>>>>>>>>>>>>Relative Humidity is : %f RH \n", thParam.humidity);
            IPC_APP_report_humidity_value(curHumi);
            lastHumi = curHumi;
        }

        if (g_halTempHumiEnable 
            && TUYA_APP_IsAlarmByInterval(lastTime) 
            && (curTemp > LUX_TUYA_TEMP_ALARM_THRESHOLD || curHumi > LUX_TUYA_HUMI_ALARM_THRESHOLD))
        {
            memset(&capPic, 0, sizeof(capPic));
            ret = LUX_BASE_CapturePic_HD(LUX_STREAM_SUB,&capPic);
            //ret = LUX_BASE_CapturePic(LUX_STREAM_JPEG, &capPic);
            if (0 == ret)
            {
                /* 抓图成功，上报温湿度异常报警信息 */
                //IPC_APP_report_temp_humi_alarm("temp&humi alarm", 16);
                tuya_ipc_notify_motion_detect((char*)capPic.pAddr, capPic.streamLen, NOTIFICATION_CONTENT_JPEG);

                //Storage interruption caused by maximum duration of internal events, restart new events
                if (SS_WRITE_MODE_EVENT == tuya_ipc_ss_get_write_mode())
                {
                    ret = LUX_Event_Record(LUX_COMM_EVENT_RECORD_TIME);
                    if (0 != ret)
                    {
                        printf("temp_humi Event Record Error");
                    }
                    /*记录当前事件时间戳*/
                    SetEventTime();
                    GetEventTime();
                    //LUX_BASE_TIMER_PARAM timerParam;

                    /* 创建本地录像处理定时器 */
                    memset(&timerParam, 0, sizeof(timerParam));
                    timerParam.bTimeStartEnable = LUX_TRUE;
                    timerParam.timeStartValue = LUX_COMM_EVENT_RECORD_TIME;
                    timerParam.pTimerProcess = (void *)LUX_stopEventRecordCB;

                    UINT_X ssTimerId = LUX_BASE_TimerCreate(&timerParam);
                    LUX_BASE_TimerStart(ssTimerId);
                    if(E_STORAGE_STOP == tuya_ipc_ss_get_status())
                    {
                        tuya_ipc_ss_start_event();
                    }
                    printf("%s:%d local storage event!\n", __func__, __LINE__);
                }
                if (ClOUD_STORAGE_TYPE_EVENT == tuya_ipc_cloud_storage_get_store_mode()
                   && tuya_ipc_cloud_storage_get_event_status_by_id(event_id) == EVENT_NONE)
                {
                    event_id = tuya_ipc_cloud_storage_event_add(capPic.pAddr, capPic.streamLen, 0, LUX_CLOUD_STORAGE_TIME);
                    printf("%s:%d cloud_event_id:%d\n", __func__, __LINE__, event_id);
                }
            }
            LUX_BASE_ReleasePic_HD(LUX_STREAM_SUB,&capPic);
            //LUX_BASE_ReleasePic(LUX_STREAM_JPEG, &capPic);
            printf("REPORT INFO: %s:%d TMP:%d HUMI:%d snap picture time:%d!\n", __func__, __LINE__, curTemp, curHumi, lastTime);
            lastTime = getTime_getS();
        }

        sleep(3);
    }

    return NULL;
}

/*
---------------------------------------------------------------------------------
code related RingBuffer
--------------------------------------------------------------------------------
 
 
 -
*/
OPERATE_RET TUYA_APP_Init_Ring_Buffer(VOID)
{
    STATIC BOOL_T s_ring_buffer_inited = FALSE;
    if(s_ring_buffer_inited == TRUE)
    {
        PR_DEBUG("The Ring Buffer Is Already Inited");
        return OPRT_OK;
    }

    CHANNEL_E channel;
    OPERATE_RET ret;
    for( channel = E_CHANNEL_VIDEO_MAIN; channel < E_CHANNEL_MAX; channel++ )
    {
        PR_DEBUG("init ring buffer Channel:%d Enable:%d", channel, s_media_info.channel_enable[channel]);
        if(s_media_info.channel_enable[channel] == TRUE)
        {
            if(channel == E_CHANNEL_AUDIO)
            {
                PR_DEBUG("audio_sample %d, audio_databits %d, audio_fps %d",s_media_info.audio_sample[E_CHANNEL_AUDIO],s_media_info.audio_databits[E_CHANNEL_AUDIO],s_media_info.audio_fps[E_CHANNEL_AUDIO]);
                ret = tuya_ipc_ring_buffer_init(channel, s_media_info.audio_sample[E_CHANNEL_AUDIO]*s_media_info.audio_databits[E_CHANNEL_AUDIO]/1024,s_media_info.audio_fps[E_CHANNEL_AUDIO],0,NULL);
            }
            
            else
            {
                PR_DEBUG("video_bitrate %d, video_fps %d",s_media_info.video_bitrate[channel],s_media_info.video_fps[channel]);
                ret = tuya_ipc_ring_buffer_init(channel, s_media_info.video_bitrate[channel],s_media_info.video_fps[channel],0,NULL);
            }
            if(ret != 0)
            {
                PR_ERR("init ring buffer fails. %d %d", channel, ret);
                return OPRT_MALLOC_FAILED;
            }
            PR_DEBUG("init ring buffer success. channel:%d", channel);
        }
    }

    s_ring_buffer_inited = TRUE;

    return OPRT_OK;
}

OPERATE_RET TUYA_APP_Put_Frame(IN CONST CHANNEL_E channel, IN CONST MEDIA_FRAME_S *p_frame)
{
    PR_TRACE("Put Frame. Channel:%d type:%d size:%u pts:%llu ts:%llu",
             channel, p_frame->type, p_frame->size, p_frame->pts, p_frame->timestamp);


    OPERATE_RET ret = tuya_ipc_ring_buffer_append_data(channel,p_frame->p_buf, p_frame->size,p_frame->type,p_frame->pts);

    if(ret != OPRT_OK)
    {
        PR_ERR("Put Frame Fail.%d Channel:%d type:%d size:%u pts:%llu ts:%llu",ret,
                 channel, p_frame->type, p_frame->size, p_frame->pts, p_frame->timestamp);
    }

    return ret;
}

OPERATE_RET TUYA_APP_Get_Frame_Backwards(IN CONST CHANNEL_E channel,
                                                  IN CONST USER_INDEX_E user_index,
                                                  IN CONST UINT_T backward_frame_num,
                                                  INOUT MEDIA_FRAME_S *p_frame)
{
    if(p_frame == NULL)
    {
        PR_ERR("input is null");
        return OPRT_INVALID_PARM;
    }

    Ring_Buffer_Node_S *node;
    if(channel == E_CHANNEL_VIDEO_MAIN || channel == E_CHANNEL_VIDEO_SUB)
        node = tuya_ipc_ring_buffer_get_pre_video_frame(channel,user_index,backward_frame_num);
    else
        node = tuya_ipc_ring_buffer_get_pre_audio_frame(channel,user_index,backward_frame_num);
    if(node != NULL)
    {
        p_frame->p_buf = node->rawData;
        p_frame->size = node->size;
        p_frame->timestamp = node->timestamp;
        p_frame->type = node->type;
        p_frame->pts = node->pts;
        return OPRT_OK;
    }
    else
    {
        PR_ERR("Fail to re-locate for user %d backward %d frames, channel %d",user_index,backward_frame_num,channel);
        return OPRT_COM_ERROR;
    }
}

OPERATE_RET TUYA_APP_Get_Frame(IN CONST CHANNEL_E channel, IN CONST USER_INDEX_E user_index, IN CONST BOOL_T isRetry, IN CONST BOOL_T ifBlock, INOUT MEDIA_FRAME_S *p_frame)
{
    if(p_frame == NULL)
    {
        PR_ERR("input is null");
        return OPRT_INVALID_PARM;
    }
    PR_TRACE("Get Frame Called. channel:%d user:%d retry:%d", channel, user_index, isRetry);

    Ring_Buffer_Node_S *node = NULL;
    while(node == NULL)
    {
        if(channel == E_CHANNEL_VIDEO_MAIN || channel == E_CHANNEL_VIDEO_SUB)
        {
            node = tuya_ipc_ring_buffer_get_video_frame(channel,user_index,isRetry);
        }
        else if(channel == E_CHANNEL_AUDIO)
        {
            node = tuya_ipc_ring_buffer_get_audio_frame(channel,user_index,isRetry);
        }
        if(NULL == node)
        {
            if(ifBlock)
            {
                usleep(10*1000);
            }
            else
            {
                return OPRT_NO_MORE_DATA;
            }
        }
    }
    p_frame->p_buf = node->rawData;
    p_frame->size = node->size;
    p_frame->timestamp = node->timestamp;
    p_frame->type = node->type;
    p_frame->pts = node->pts;

    PR_TRACE("Get Frame Success. channel:%d user:%d retry:%d size:%u ts:%ull type:%d pts:%llu",
             channel, user_index, isRetry, p_frame->size, p_frame->timestamp, p_frame->type, p_frame->pts);
    return OPRT_OK;
}

/*
---------------------------------------------------------------------------------
---------------------------------------------------------------------------------
*/

/*
---------------------------------------------------------------------------------
code related EchoShow
---------------------------------------------------------------------------------
*/
#if ENABLE_ECHO_SHOW == 1

INT_T TUYA_APP_Echoshow_Start(PVOID_T context, PVOID_T priv_data)
{
    printf("echoshow start...\n");

    return 0;
}

INT_T TUYA_APP_Echoshow_Stop(PVOID_T context, PVOID_T priv_data)
{
    printf("echoshow stop...\n");

    return 0;
}

INT_T TUYA_APP_Chromecast_Start(PVOID_T context, PVOID_T priv_data)
{
    printf("chromecast start...\n");

    return 0;
}

INT_T TUYA_APP_Chromecast_Stop(PVOID_T context, PVOID_T priv_data)
{
    printf("chromecast stop...\n");

    return 0;
}

OPERATE_RET TUYA_APP_Enable_EchoShow_Chromecast(VOID)
{
     STATIC BOOL_T s_echoshow_inited = FALSE;
     if(s_echoshow_inited == TRUE)
     {
         PR_DEBUG("The EchoShow Is Already Inited");
         return OPRT_OK;
     }

    PR_DEBUG("Init EchoShow");

    TUYA_ECHOSHOW_PARAM_S es_param = {0};

    es_param.pminfo = &s_media_info;
    es_param.cbk.pcontext = NULL;
    es_param.cbk.start = TUYA_APP_Echoshow_Start;
    es_param.cbk.stop = TUYA_APP_Echoshow_Stop;
    /*Channel settings according to requirements*/
    es_param.vchannel = 1;
    es_param.mode = TUYA_ECHOSHOW_MODE_ECHOSHOW;

    tuya_ipc_echoshow_init(&es_param);

    TUYA_CHROMECAST_PARAM_S param = {0};

    param.pminfo = &s_media_info;
    /*Channel settings according to requirements*/
    param.audio_channel = E_CHANNEL_AUDIO_2RD;
    param.video_channel = E_CHANNEL_VIDEO_SUB;
    param.src = TUYA_STREAM_SOURCE_RINGBUFFER;
    param.mode = TUYA_STREAM_TRANSMIT_MODE_ASYNC;
    param.cbk.pcontext = NULL;
    param.cbk.start = TUYA_APP_Chromecast_Start;
    param.cbk.stop = TUYA_APP_Chromecast_Stop;
    param.cbk.get_frame = NULL;

    tuya_ipc_chromecast_init(&param);

    s_echoshow_inited = TRUE;

    return OPRT_OK;
}
#endif
/*
---------------------------------------------------------------------------------

---------------------------------------------------------------------------------
*/


