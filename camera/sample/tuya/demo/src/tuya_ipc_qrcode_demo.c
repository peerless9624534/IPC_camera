/*********************************************************************************
  *Copyright(C),2015-2020, TUYA www.tuya.comm
  *FileName:    tuya_ipc_qrcode_demo
**********************************************************************************/
#include <unistd.h>
#include <string.h>
#include "tuya_ipc_api.h"
#include "tuya_ipc_qrcode_proc.h"
#include "comm_func.h"
#include "lux_fsrc.h"
#include "tuya_ipc_system_control_demo.h"

#if defined(WIFI_GW) && (WIFI_GW==1)

#define QR_CODE_CHN_WIDTH   640
#define QR_CODE_CHN_HEIGHT  360
#define QR_CODE_THRESH      128
static unsigned char *g_pQrcodeY;
static int g_qrThreadOngoing = 1;

STATIC int __tuya_linux_get_snap_qrcode(char *pQrcodeStr)
{
    int ret   = -1;
    int out_w = 0;
    int out_h = 0;
    unsigned char *enhance_qrcode_yuv = NULL;
    LUX_DUMP_NV12_PIC_INFO_ST frame;

    if (NULL == g_pQrcodeY || NULL == pQrcodeStr)
    {
        return ret;
    }

    memset(&frame, 0, sizeof(frame));
    ret = LUX_FSrc_DumpNV12(1, g_pQrcodeY, &frame);
    if (0 != ret)
    {
        printf("%s:%d ret:%#x\n", __func__, __LINE__, ret);
        return ret;
    }

    ret = Tuya_Ipc_QRCode_Enhance(g_pQrcodeY, QR_CODE_CHN_WIDTH, QR_CODE_CHN_HEIGHT, &enhance_qrcode_yuv, &out_w, &out_h, QR_CODE_THRESH, 0);
    if (0 != ret)
    {
        printf("%s:%d ret:%#x\n", __func__, __LINE__, ret);
        return ret;
    }

    //use enhance_qrcode_yuv to parse
    ret = LUX_COMM_Zbar_Parse((void*)enhance_qrcode_yuv, out_w, out_h, pQrcodeStr);
    if (0 != ret)
    {
        //printf("%s:%d ret:%#x\n", __func__, __LINE__, ret);
        ret = -1;
    }

    //free
    if (enhance_qrcode_yuv)
    {
        free(enhance_qrcode_yuv);
        enhance_qrcode_yuv = NULL;
    }

    return ret;
}

/* 推出二维码扫描线程 */
void QR_Thread_Exit()
{
    g_qrThreadOngoing = 0;
    
    return ;
}

void *thread_qrcode(void *arg)
{
    OPERATE_RET ret = 0;
    char g_qrcodeStr[128] = {0};

    printf("Qrcode Thread start\r\n");

    if (NULL == g_pQrcodeY)
    {
        g_pQrcodeY = malloc(QR_CODE_CHN_WIDTH * QR_CODE_CHN_HEIGHT * 2);
        if (NULL == g_pQrcodeY)
        {
            printf("QRCODE malloc memery error!\n");
            return NULL;
        }
    }

    while (g_qrThreadOngoing)
    {
        memset(g_qrcodeStr, 0, 128);
        ret = __tuya_linux_get_snap_qrcode(g_qrcodeStr);
        if (0 == ret)
        {
            printf("get string from qrcode %s\r\n", g_qrcodeStr);
            ret = tuya_ipc_direct_connect(g_qrcodeStr, TUYA_IPC_DIRECT_CONNECT_QRCODE);
            if (ret == OPRT_OK)
            {
                printf("register to tuya cloud via qrcode success\r\n");
                break;
            }
        }
        usleep(100 * 1000);
    }

    /* 两种情况：1.ap配网成功，不播提示音；2.扫码成功，播提示音 */
    if (g_qrThreadOngoing)
        IPC_APP_Notify_LED_Sound_Status_CB(IPC_CONNECTING_WIFI);

    if (NULL != g_pQrcodeY)
        free(g_pQrcodeY);

    printf("Qrcode Proc Finish\r\n");
    return NULL;
}
#endif

