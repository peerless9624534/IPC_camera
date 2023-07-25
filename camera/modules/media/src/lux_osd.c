/*
 * lux_osd.c
 *
 * 基于Ingenic平台封装的osd模块，包括时间显示、画线等
 *
 */
#include <lux_osd.h>
#include <imp_osd.h>
#include <imp_encoder.h>
#include <imp_log.h>
#include <imp_common.h>
#include <string.h>
#include <lux_base.h>
#include <stdlib.h>
#include <lux_iniparse.h>

#include "bitmapinfo_1440p.h"
#include "bitmapinfo_1080p.h"
#include "bitmapinfo_360p.h"

#include "comm_def.h"
#include "lux_ivs.h"
#include "lux_coverFace.h"


 // #include "logodata_106x58_bgra.h"
 // #include "logodata_382x88_bgra.h"
 // #include "logodata_436x236_bgra.h"

#define TAG "lux_osd"

IMPRgnHandle* g_prHander[LUX_OSD_GRP_NUM];
LUX_COMM_FUNC_SNS_WH g_tSensorWH;
BOOL_X g_osdInit = LUX_FALSE;

#ifdef CONFIG_PTZ_IPC
#define LOGO_SIZE_HD  411584
static uint8_t logodata_436x236_bgra[LOGO_SIZE_HD];
#else
#define LOGO_SIZE_HD  134464
static uint8_t logodata_382x88_bgra[LOGO_SIZE_HD];
#endif
#define LOGO_SIZE_SD  24592
static uint8_t logodata_106x58_bgra[LOGO_SIZE_SD];

LUX_OSD_ATTR_ST g_osdAttr[LUX_OSD_REGION_NUM] =
{
    {
        .enable = 1,
        .RgnName = "font",
        .OSDRgnAttr =
        {
            {
                .type = OSD_REG_PIC,
                .rect.p0.x = LUX_OSD_FONT_GRP0_START_X,
                .rect.p0.y = LUX_OSD_FONT_GRP0_START_Y,
                .rect.p1.x = LUX_OSD_FONT_GRP0_START_X + 20 * OSD_GRP0_REGION_1080P_W - 1, //p0 is start，and p1 well be epual p0+width(or heigth)-1
                .rect.p1.y = LUX_OSD_FONT_GRP0_START_Y + OSD_GRP0_REGION_1080P_H - 1,
                .fmt = PIX_FMT_BGRA,
                .data.picData.pData = NULL,
            },
            {
                .type = OSD_REG_PIC,
                .rect.p0.x = LUX_OSD_FONT_GRP1_START_X,
                .rect.p0.y = LUX_OSD_FONT_GRP1_START_Y,
                .rect.p1.x = LUX_OSD_FONT_GRP1_START_X + 20 * OSD_GRP1_REGION_360P_W - 1, //p0 is start，and p1 well be epual p0+width(or heigth)-1
                .rect.p1.y = LUX_OSD_FONT_GRP1_START_Y + OSD_GRP1_REGION_360P_H - 1,
                .fmt = PIX_FMT_BGRA,
                .data.picData.pData = NULL,
            },
        },
        .OSDGrpRgnAttr =
        {
            .show = 0,
            .gAlphaEn = 1,
            .fgAlhpa = 0xff,
            .layer = 3,
        },
    },
    {
        .enable = 1,
        .RgnName = "logo",
        .OSDRgnAttr =
        {
            {
                .type = OSD_REG_PIC,
                .rect.p0.x = LUX_OSD_LOGO_GRP0_START_X,
                .rect.p0.y = LUX_OSD_LOGO_GRP0_START_Y,
                .rect.p1.x = LUX_OSD_LOGO_GRP0_START_X + LUX_OSD_GRP0_LOGO_W - 1,
                .rect.p1.y = LUX_OSD_LOGO_GRP0_START_Y + LUX_OSD_GRP0_LOGO_H - 1,
                .fmt = PIX_FMT_BGRA,
                // #ifdef CONFIG_PTZ_IPC
                //                         .data.picData.pData = logodata_436x236_bgra,
                // #else
                //                         .data.picData.pData = logodata_382x88_bgra,
                // #endif
            },
            {
                .type = OSD_REG_PIC,
                .rect.p0.x = LUX_OSD_LOGO_GRP1_START_X,
                .rect.p0.y = LUX_OSD_LOGO_GRP1_START_Y,
                .rect.p1.x = LUX_OSD_LOGO_GRP1_START_X + LUX_OSD_GRP1_LOGO_W - 1,
                .rect.p1.y = LUX_OSD_LOGO_GRP1_START_Y + LUX_OSD_GRP1_LOGO_H - 1,
                .fmt = PIX_FMT_BGRA,
                // .data.picData.pData = logodata_106x58_bgra,
            },
        },
        .OSDGrpRgnAttr =
        {
            .show = 0,
            .gAlphaEn = 1,
            .fgAlhpa = 0x7f,
            .layer = 2,
        },
    },
    {
        .enable = 1,
        .RgnName = "cover",
        .OSDRgnAttr =
            {
                {
                    .type = OSD_REG_COVER,
                    .rect.p0.x = 0,
                    .rect.p0.y = 0,
                    .rect.p1.x = SENSOR_WIDTH_DEFAULT - 1,
                    .rect.p1.y = SENSOR_HEIGHT_DEFAULT - 1,
                    .fmt = PIX_FMT_BGRA,
                    .data.coverData.color = OSD_WHITE,
                },
                {
                    .type = OSD_REG_COVER,
                    .rect.p0.x = 0,
                    .rect.p0.y = 0,
                    .rect.p1.x = SENSOR_WIDTH_SECOND - 1,
                    .rect.p1.y = SENSOR_HEIGHT_SECOND - 1,
                    .fmt = PIX_FMT_BGRA,
                    .data.coverData.color = OSD_WHITE,
                },
            },
        .OSDGrpRgnAttr =
            {
                .show = 0,
                .gAlphaEn = 1,
                .fgAlhpa = 0x7f,
                .layer = 2,
            },
    },
    {
        .enable = 1,
        .RgnName = "rect",//矩形框,用于人形识别和人脸识别，绿框人脸，红框人形
        .OSDRgnAttr =
        {
            {
                .type = OSD_REG_RECT,
                .rect.p0.x = 0,
                .rect.p0.y = 0,
                .rect.p1.x = 0,
                .rect.p1.y = 0,
                .fmt = PIX_FMT_MONOWHITE,
                .data.lineRectData.color = OSD_GREEN,
                .data.lineRectData.linewidth = 5,
            },
            {
                .type = OSD_REG_RECT,
                .rect.p0.x = 0,
                .rect.p0.y = 0,
                .rect.p1.x = 0,
                .rect.p1.y = 0,
                .fmt = PIX_FMT_MONOWHITE,
                .data.lineRectData.color = OSD_GREEN,
                .data.lineRectData.linewidth = 5,
            },
        },
        .OSDGrpRgnAttr =
        {
            .show = 0,
            .layer = 1,
            .scalex = 1,
            .scaley = 1,
        },
    },
};

VOID_X LUX_BlackCover_Show(BOOL_X bShow)
{
    INT_X ret = LUX_ERR;
    INT_X i;

    for (i = 0; i < LUX_OSD_GRP_NUM; i++) {
        //展示区域
        ret = IMP_OSD_ShowRgn(g_prHander[i][2], i, bShow);
        if (ret != LUX_OK)
        {
            IMP_LOG_ERR(TAG, "IMP_OSD_ShowRgn() group%d region 2 error\n", i);
            return;
        }
        else
        {
            IMP_LOG_ERR(TAG, "IMP_OSD_ShowRgn() group%d region 2 success\n", i);
            return;
        }

    }
}

//越界点值转换
VOID_X LUX_OutBoundPoint_Convert(INT_X channel, IMPPoint *pPoint)
{
    if (pPoint == NULL)
    {
        IMP_LOG_ERR(TAG, "LUX_OutBoundPoint_Convert failed, empty point\n");
        return;
    }
    if (pPoint->x < 0) pPoint->x = 0;
    if (pPoint->y < 0) pPoint->y = 0;

    if (channel == LUX_STREAM_MAIN)
    {
        if (pPoint->x >= SENSOR_WIDTH_DEFAULT) pPoint->x = SENSOR_WIDTH_DEFAULT - 1;
        if (pPoint->y >= SENSOR_HEIGHT_DEFAULT) pPoint->y = SENSOR_HEIGHT_DEFAULT - 1;
    }
    else if (channel == LUX_STREAM_SUB)
    {
        if (pPoint->x >= SENSOR_WIDTH_SECOND) pPoint->x = SENSOR_WIDTH_SECOND - 1;
        if (pPoint->y >= SENSOR_HEIGHT_SECOND) pPoint->y = SENSOR_HEIGHT_SECOND - 1;
    }
}

/**
 * @description: osd矩形框绘制接口
 * @param [in] chnID        通道，支持主码流和子码流通道
 * @param [in] rectNum      OSD矩形框区域编号：0到LUX_RECT_NUM_MAX-1
 * @param [in] rect         矩形信息，包含左上角ul和右下角br坐标
 * @param [in] color        矩形框颜色，支持的颜色看LUX_RECT_OSD_COLOR_EN
 * @param [in] linewidth    矩形框线宽
 */
VOID_X LUX_OSD_Rect_Draw(LUX_STREAM_CHANNEL_EN chnID, INT_X rectNum, LUX_OSD_Rect* rect, LUX_RECT_OSD_COLOR_EN color, INT_X linewidth)
{
    if (chnID < LUX_STREAM_MAIN || chnID >= LUX_STREAM_THRD)
    {
        IMP_LOG_ERR(TAG, "LUX_OSD_Draw_Rect failed, not support chnID [%d]\n", chnID);
        return;
    }
    if (rectNum >= LUX_RECT_NUM_MAX + LUX_LAST_REGION_NUM || rectNum < 0)
    {
        IMP_LOG_ERR(TAG, "LUX_OSD_Rect_Draw failed, not support rectNum [%d]\n", rectNum);
        return;
    }
    if (rect == NULL)
    {
        IMP_LOG_ERR(TAG, "LUX_OSD_Rect_Draw failed, empty point\n");
        return;
    }

    if(color != LUX_RECT_OSD_BLACK &&
       color != LUX_RECT_OSD_BLUE &&
       color != LUX_RECT_OSD_GREEN &&
       color != LUX_RECT_OSD_RED &&
       color != LUX_RECT_OSD_WHITE)
    {
        IMP_LOG_ERR(TAG, "LUX_OSD_Rect_Draw failed, not support color [%d]\n", color);
        return;
    }
#if 0
    if (ul->x >= br->x || ul->y >= br->y)
    {
        IMP_LOG_ERR(TAG, "LUX_OSD_Rect_Draw failed, coordinates do not comply with the rectangle rule ! ul.x=%d, ul.y=%d, br.x=%d, br.y=%d \n", ul->x, ul->y, br->x, br->y);
        return;
    }
#endif

    INT_X ret = LUX_ERR;

    /* 设置OSD区域属性 */
    g_osdAttr[rectNum].OSDRgnAttr[chnID].type = OSD_REG_RECT;
    g_osdAttr[rectNum].OSDRgnAttr[chnID].rect = *rect;
    LUX_OutBoundPoint_Convert(chnID, &g_osdAttr[rectNum].OSDRgnAttr[chnID].rect.p0);
    LUX_OutBoundPoint_Convert(chnID, &g_osdAttr[rectNum].OSDRgnAttr[chnID].rect.p1);
    g_osdAttr[rectNum].OSDRgnAttr[chnID].data.lineRectData.color = color;
    g_osdAttr[rectNum].OSDRgnAttr[chnID].data.lineRectData.linewidth = linewidth;
    ret = IMP_OSD_SetRgnAttr(g_prHander[chnID][rectNum], &g_osdAttr[rectNum].OSDRgnAttr[chnID]);
    if (ret != LUX_OK)
    {
        IMP_LOG_ERR(TAG, "IMP_OSD_SetRgnAttr Rect error !\n");
        return;
    }
    ret = IMP_OSD_ShowRgn(g_prHander[chnID][rectNum], chnID, LUX_TRUE);
    if (ret != LUX_OK)
    {
        IMP_LOG_ERR(TAG, "IMP_OSD_ShowRgn() group%d region%d error\n", chnID, rectNum);
        return;
    }
}

/**
 * @description: osd矩形框消失接口
 * @param [in] chnID        通道，支持主码流和子码流通道
 * @param [in] rectNum       OSD矩形框区域编号：0到LUX_RECT_NUM_MAX-1
 */
VOID_X LUX_OSD_Rect_Vanish(LUX_STREAM_CHANNEL_EN chnID, INT_X rectNum)
{
    if (chnID < LUX_STREAM_MAIN || chnID >= LUX_STREAM_THRD)
    {
        IMP_LOG_ERR(TAG, "LUX_OSD_Rect_Vanish failed, not support chnID [%d]\n", chnID);
        return;
    }
    if (rectNum >= LUX_RECT_NUM_MAX + LUX_LAST_REGION_NUM || rectNum < 0)
    {
        IMP_LOG_ERR(TAG, "LUX_OSD_Rect_Vanish failed, not support rectNum [%d]\n", rectNum);
        return;
    }

    INT_X ret = LUX_ERR;

    ret = IMP_OSD_ShowRgn(g_prHander[chnID][rectNum], chnID, LUX_FALSE);
    if (ret != LUX_OK)
    {
        IMP_LOG_ERR(TAG, "IMP_OSD_ShowRgn() group%d region%d error\n", chnID, rectNum);
        return;
    }
}

VOID_X LUX_OSD_PersonRect_Show(persondet_param_output_t* ivsPersonDetRet)
{
    INT_X i, j;
    INT_X personCnt = 0;
    INT_X scale = 1;
    LUX_OSD_Rect rect;
    if (ivsPersonDetRet->count > LUX_HUMANRECT_NUM_MAX)
    {
        ivsPersonDetRet->count = LUX_HUMANRECT_NUM_MAX;
    }

    for (i = 0; i < LUX_OSD_GRP_NUM; i++)
    {
        if (i == LUX_STREAM_MAIN)
        {
            scale = SENSOR_HEIGHT_DEFAULT / SENSOR_HEIGHT_SECOND;
        }
        else
        {
            scale = 1;
        }

        for (j = LUX_HUMAN_START_POS; j < LUX_HUMAN_START_POS + ivsPersonDetRet->count; j++)
        {
            personCnt = j - LUX_HUMAN_START_POS;
            memset(&rect, 0, sizeof(LUX_OSD_Rect));
            rect.p0.x = (ivsPersonDetRet->person + personCnt)->show_box.ul.x * scale;
            rect.p0.y = (ivsPersonDetRet->person + personCnt)->show_box.ul.y * scale;
            rect.p1.x = (ivsPersonDetRet->person + personCnt)->show_box.br.x * scale;
            rect.p1.y = (ivsPersonDetRet->person + personCnt)->show_box.br.y * scale;
            LUX_OSD_Rect_Draw(i, j, &rect, LUX_RECT_OSD_RED, scale);
        }
    }
}

VOID_X LUX_OSD_PersonRect_Miss()
{
    INT_X i, j;
    for (i = 0; i < LUX_OSD_GRP_NUM; i++)
    {
        for (j = LUX_HUMAN_START_POS; j < LUX_HUMAN_END_POS; j++)
        {
            LUX_OSD_Rect_Vanish(i, j);
        }
    }
}

VOID_X LUX_OSD_PukeRect_SSDShow(LUX_FACEDET_OUTPUT_ST* ivsFaceDetRet)
{
    INT_X i, j;
    INT_X faceCnt = 0;
    INT_X scale = 1;
    LUX_OSD_Rect rect;
    if (ivsFaceDetRet->count > LUX_PUKERECT_NUM_MAX)
    {
        ivsFaceDetRet->count = LUX_PUKERECT_NUM_MAX;
    }

    for (i = 0; i < LUX_OSD_GRP_NUM; i++)
    {
        if (i == LUX_STREAM_MAIN)
        {
            scale = SENSOR_HEIGHT_DEFAULT / SENSOR_HEIGHT_SECOND;
        }
        else
        {
            scale = 1;
        }

        for (j = LUX_PUKE_START_POS; j < LUX_PUKE_START_POS + ivsFaceDetRet->count; j++)
        {
            faceCnt = j - LUX_PUKE_START_POS;
            memset(&rect, 0, sizeof(LUX_OSD_Rect));
            rect.p0.x = (ivsFaceDetRet->face + faceCnt)->x0 * scale;
            rect.p0.y = (ivsFaceDetRet->face + faceCnt)->y0 * scale;
            rect.p1.x = (ivsFaceDetRet->face + faceCnt)->x1 * scale;
            rect.p1.y = (ivsFaceDetRet->face + faceCnt)->y1 * scale;
            LUX_OSD_Rect_Draw(i, j, &rect, LUX_RECT_OSD_BLUE, scale);
        }
    }
}

VOID_X LUX_OSD_PukeRect_Miss()
{
    INT_X i, j;
    for (i = 0; i < LUX_OSD_GRP_NUM; i++)
    {
        for (j = LUX_PUKE_START_POS; j < LUX_PUKE_END_POS; j++)
        {
            LUX_OSD_Rect_Vanish(i, j);
        }
    }
}

VOID_X LUX_OSD_KickQuiltRect_SSDShow(LUX_PERSONDET_OUTPUT_ST* ivsPersonDetRet)
{
    INT_X i, j;
    INT_X personCnt = 0;
    INT_X scale = 1;
    LUX_OSD_Rect rect;
    if (ivsPersonDetRet->count > LUX_KICKQUILT_NUM_MAX)
    {
        ivsPersonDetRet->count = LUX_KICKQUILT_NUM_MAX;
    }

    for (i = 0; i < LUX_OSD_GRP_NUM; i++)
    {
        if (i == LUX_STREAM_MAIN)
        {
            scale = SENSOR_HEIGHT_DEFAULT / SENSOR_HEIGHT_SECOND;
        }
        else
        {
            scale = 1;
        }

        for (j = LUX_KICKQUILT_START_POS; j < LUX_KICKQUILT_START_POS + ivsPersonDetRet->count; j++)
        {
            personCnt = j - LUX_KICKQUILT_START_POS;
            memset(&rect, 0, sizeof(LUX_OSD_Rect));
            rect.p0.x = (ivsPersonDetRet->person + personCnt)->x0 * scale;
            rect.p0.y = (ivsPersonDetRet->person + personCnt)->y0 * scale;
            rect.p1.x = (ivsPersonDetRet->person + personCnt)->x1 * scale;
            rect.p1.y = (ivsPersonDetRet->person + personCnt)->y1 * scale;
            LUX_OSD_Rect_Draw(i, j, &rect, LUX_RECT_OSD_RED, scale);
        }
    }
}

VOID_X LUX_OSD_KickQuiltRect_Miss()
{
    INT_X i, j;
    for (i = 0; i < LUX_OSD_GRP_NUM; i++)
    {
        for (j = LUX_KICKQUILT_START_POS; j < LUX_KICKQUILT_END_POS; j++)
        {
            LUX_OSD_Rect_Vanish(i, j);
        }
    }
}

VOID_X LUX_OSD_FaceRect_Show(facedet_param_output_t* ivsFaceDetRet)
{
    INT_X i, j;
    INT_X faceCnt = 0;
    INT_X scale = 1;
    LUX_OSD_Rect rect;
    if (ivsFaceDetRet->count > LUX_FACTRECT_NUM_MAX)
    {
        ivsFaceDetRet->count = LUX_FACTRECT_NUM_MAX;
    }

    for (i = 0; i < LUX_OSD_GRP_NUM; i++)
    {
        if (i == LUX_STREAM_MAIN)
        {
            scale = SENSOR_HEIGHT_DEFAULT / SENSOR_HEIGHT_SECOND;
        }
        else
        {
            scale = 1;
        }

        for (j = LUX_FACE_START_POS; j < LUX_FACE_START_POS + ivsFaceDetRet->count; j++)
        {
            faceCnt = j - LUX_FACE_START_POS;
            memset(&rect, 0, sizeof(LUX_OSD_Rect));
            rect.p0.x = (ivsFaceDetRet->face + faceCnt)->show_box.ul.x * scale;
            rect.p0.y = (ivsFaceDetRet->face + faceCnt)->show_box.ul.y * scale;
            rect.p1.x = (ivsFaceDetRet->face + faceCnt)->show_box.br.x * scale;
            rect.p1.y = (ivsFaceDetRet->face + faceCnt)->show_box.br.y * scale;
            LUX_OSD_Rect_Draw(i, j, &rect, LUX_RECT_OSD_GREEN, scale);
        }
    }
}

VOID_X LUX_OSD_FaceRect_SSDShow(LUX_FACEDET_OUTPUT_ST* ivsFaceDetRet)
{
    INT_X i, j;
    INT_X faceCnt = 0;
    INT_X scale = 1;
    LUX_OSD_Rect rect;
    if (ivsFaceDetRet->count > LUX_FACTRECT_NUM_MAX)
    {
        ivsFaceDetRet->count = LUX_FACTRECT_NUM_MAX;
    }

    for (i = 0; i < LUX_OSD_GRP_NUM; i++)
    {
        if (i == LUX_STREAM_MAIN)
        {
            scale = SENSOR_HEIGHT_DEFAULT / SENSOR_HEIGHT_SECOND;
        }
        else
        {
            scale = 1;
        }

        for (j = LUX_FACE_START_POS; j < LUX_FACE_START_POS + ivsFaceDetRet->count; j++)
        {
            faceCnt = j - LUX_FACE_START_POS;
            memset(&rect, 0, sizeof(LUX_OSD_Rect));
            rect.p0.x = (ivsFaceDetRet->face + faceCnt)->x0 * scale;
            rect.p0.y = (ivsFaceDetRet->face + faceCnt)->y0 * scale;
            rect.p1.x = (ivsFaceDetRet->face + faceCnt)->x1 * scale;
            rect.p1.y = (ivsFaceDetRet->face + faceCnt)->y1 * scale;
            LUX_OSD_Rect_Draw(i, j, &rect, LUX_RECT_OSD_GREEN, scale);
        }
    }
}

VOID_X LUX_OSD_FaceRect_Miss()
{
    INT_X i, j;
    for (i = 0; i < LUX_OSD_GRP_NUM; i++)
    {
        for (j = LUX_FACE_START_POS; j < LUX_FACE_END_POS; j++)
        {
            LUX_OSD_Rect_Vanish(i, j);
        }
    }
}

/**
 * @description: 设置创建的几个组区域是否显示，
 * @return     ：0 设置成功. 非0 设置失败
 */
INT_X LUX_OSD_Show(BOOL_X bShow)
{
    int ret;
    int i, j;

    for (i = 0; i < LUX_OSD_GRP_NUM; i++) {
        for (j = 0; j < LUX_OSD_REGION_NUM; j++) {
            if (g_osdAttr[j].enable) {
                //black cover初始化不出现
                if (2 == j)
                {
                    continue;
                }
                ret = IMP_OSD_ShowRgn(g_prHander[i][j], i, bShow);
                if (ret != 0) {
                    IMP_LOG_ERR(TAG, "IMP_OSD_ShowRgn() group%d region%d error\n", i, j);
                    return LUX_ERR;
                }
            }
        }
    }

    return LUX_OK;
}


static void* LUX_OSD_Time_Stamp_Update_Thread(void* p)
{
    int ret;

    /*generate time*/
    char DateStr[LUX_OSD_LETTER_NUM];
    time_t currTime;
    struct tm currDate;
    unsigned i = 0, j = 0;
    void* grp0DateData = NULL;
    void* grp1DateData = NULL;
    IMPOSDRgnAttrData grp0RgnAttrData;
    IMPOSDRgnAttrData grp1RgnAttrData;
    UINT_X* grp0TimeStampData = NULL;
    BOOL_X bShow = LUX_TRUE;

    if (g_tSensorWH.width == 2560) {
        grp0TimeStampData = malloc(LUX_OSD_LETTER_NUM * OSD_GRP0_REGION_1440P_H * OSD_GRP0_REGION_1440P_W * sizeof(UINT_X));
    }
    else {
        grp0TimeStampData = malloc(LUX_OSD_LETTER_NUM * OSD_GRP0_REGION_1080P_H * OSD_GRP0_REGION_1080P_W * sizeof(UINT_X));
    }
    if (grp0TimeStampData == NULL) {
        IMP_LOG_ERR(TAG, "malloc grp0TimeStampData error\n");
        return NULL;
    }

    UINT_X* grp1TimeStampData = malloc(LUX_OSD_LETTER_NUM * OSD_GRP1_REGION_360P_H * OSD_GRP1_REGION_360P_W * sizeof(UINT_X));
    if (grp1TimeStampData == NULL) {
        IMP_LOG_ERR(TAG, "malloc grp1TimeStampData error\n");
        return NULL;
    }

    /* 读取配置 */
    ret = LUX_INIParse_GetBool(TUYA_INI_CONFIG_FILE_NAME, "function", "watermark_onoff", &bShow);
    if (0 != ret)
    {
        IMP_LOG_ERR(TAG, "LUX_INIParse_GetBool OSD_watermark_onoff failed!, error num [0x%x] ", ret);
    }

    ret = LUX_OSD_Show(bShow);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "OSD show error\n");
        return NULL;
    }

    while (1)
    {
        int grp0_penpos_t = 0;
        int grp0_fontadv = 0;
        int grp1_penpos_t = 0;
        int grp1_fontadv = 0;

        currTime = time(NULL) + getTimeStampdDiff();
        memset(DateStr, 0, sizeof(DateStr));
        strftime(DateStr, sizeof(DateStr), "%Y-%m-%d %H:%M:%S", gmtime_r(&currTime, &currDate));   /* %H:24小时制 %I:12小时制 */
        for (i = 0; i < LUX_OSD_LETTER_NUM; i++) {
            switch (DateStr[i]) {
            case '0' ... '9':
                if (g_tSensorWH.width == 2560) {
                    grp0DateData = (void*)gBitmap_1440p[DateStr[i] - '0'].pdata;
                    grp0_fontadv = gBitmap_1440p[DateStr[i] - '0'].width;
                    grp0_penpos_t += gBitmap_1440p[DateStr[i] - '0'].width;
                }
                else {
                    grp0DateData = (void*)gBitmap_1080p[DateStr[i] - '0'].pdata;
                    grp0_fontadv = gBitmap_1080p[DateStr[i] - '0'].width;
                    grp0_penpos_t += gBitmap_1080p[DateStr[i] - '0'].width;

                }
                grp1DateData = (void*)gBitmap_360p[DateStr[i] - '0'].pdata;
                grp1_fontadv = gBitmap_360p[DateStr[i] - '0'].width;
                grp1_penpos_t += gBitmap_360p[DateStr[i] - '0'].width;
                break;
            case '-':
                if (g_tSensorWH.width == 2560) {
                    grp0DateData = (void*)gBitmap_1440p[10].pdata;
                    grp0_fontadv = gBitmap_1440p[10].width;
                    grp0_penpos_t += gBitmap_1440p[10].width;
                }
                else {
                    grp0DateData = (void*)gBitmap_1080p[10].pdata;
                    grp0_fontadv = gBitmap_1080p[10].width;
                    grp0_penpos_t += gBitmap_1080p[10].width;
                }
                grp1DateData = (void*)gBitmap_360p[10].pdata;
                grp1_fontadv = gBitmap_360p[10].width;
                grp1_penpos_t += gBitmap_360p[10].width;
                break;
            case ' ':
                if (g_tSensorWH.width == 2560) {
                    grp0DateData = (void*)gBitmap_1440p[11].pdata;
                    grp0_fontadv = gBitmap_1440p[11].width;
                    grp0_penpos_t += gBitmap_1440p[11].width;
                }
                else {
                    grp0DateData = (void*)gBitmap_1080p[11].pdata;
                    grp0_fontadv = gBitmap_1080p[11].width;
                    grp0_penpos_t += gBitmap_1080p[11].width;
                }
                grp1DateData = (void*)gBitmap_360p[11].pdata;
                grp1_fontadv = gBitmap_360p[11].width;
                grp1_penpos_t += gBitmap_360p[11].width;
                break;
            case ':':
                if (g_tSensorWH.width == 2560) {
                    grp0DateData = (void*)gBitmap_1440p[12].pdata;
                    grp0_fontadv = gBitmap_1440p[12].width;
                    grp0_penpos_t += gBitmap_1440p[12].width;
                }
                else {
                    grp0DateData = (void*)gBitmap_1080p[12].pdata;
                    grp0_fontadv = gBitmap_1080p[12].width;
                    grp0_penpos_t += gBitmap_1080p[12].width;
                }
                grp1DateData = (void*)gBitmap_360p[12].pdata;
                grp1_fontadv = gBitmap_360p[12].width;
                grp1_penpos_t += gBitmap_360p[12].width;
                break;
            default:
                break;
            }
            if (g_tSensorWH.width == 2560) {
                for (j = 0; j < OSD_GRP0_REGION_1440P_H; j++) {
                    memcpy((void*)((UINT_X*)grp0TimeStampData + j * LUX_OSD_LETTER_NUM * OSD_GRP0_REGION_1440P_W + grp0_penpos_t),
                            (void*)((UINT_X*)grp0DateData + j * grp0_fontadv), grp0_fontadv * sizeof(UINT_X));
                }
            }
            else {
                for (j = 0; j < OSD_GRP0_REGION_1080P_H; j++) {
                    memcpy((void*)((UINT_X*)grp0TimeStampData + j * LUX_OSD_LETTER_NUM * OSD_GRP0_REGION_1080P_W + grp0_penpos_t),
                            (void*)((UINT_X*)grp0DateData + j * grp0_fontadv), grp0_fontadv * sizeof(UINT_X));
                }
            }

            for (j = 0; j < OSD_GRP1_REGION_360P_H; j++) {
                memcpy((void*)((UINT_X*)grp1TimeStampData + j * LUX_OSD_LETTER_NUM * OSD_GRP1_REGION_360P_W + grp1_penpos_t),
                        (void*)((UINT_X*)grp1DateData + j * grp1_fontadv), grp1_fontadv * sizeof(UINT_X));
            }
        }

        grp0RgnAttrData.picData.pData = grp0TimeStampData;
        grp1RgnAttrData.picData.pData = grp1TimeStampData;

        if (g_osdAttr[0].enable) {
            if (LUX_OK != IMP_OSD_UpdateRgnAttrData(g_prHander[0][0], &grp0RgnAttrData)) {
                IMP_LOG_ERR(TAG, "IMP_OSD_UpdateRgnAttrData() group0 error\n");
            }
            if (LUX_OSD_GRP_NUM == 2) {
                if (LUX_OK != IMP_OSD_UpdateRgnAttrData(g_prHander[1][0], &grp1RgnAttrData)) {
                    IMP_LOG_ERR(TAG, "IMP_OSD_UpdateRgnAttrData() group1 error\n");
                }
            }
        }
        sleep(1);
    }

    free(grp0TimeStampData);
    free(grp1TimeStampData);

    return NULL;
}


/**
 * @description: 创建OSD的bgramap更新线程
 * @return     ：0 成功. 非0 失败
 */
INT_X LUX_OSD_Time_Stamp_Update()
{
    INT_X ret = LUX_ERR;

    ret = Task_CreateThread(LUX_OSD_Time_Stamp_Update_Thread, NULL);
    if (LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "Task_CreateThread LUX_OSD_Time_Stamp_Update_Thread failed.\n");
        return LUX_ERR;
    }
    return LUX_OK;
}

/**
 * @description: 获取sensor的像素宽度和高度，重置主码流各OSD区域的位置参数
* @return	  ：0 成功. 非0 失败
*/
INT_X LUX_OSD_SetRgnPositionParams(int rgnNum)
{
    CHAR_X sensorName[64] = { 0 };
    INT_X ret = LUX_ERR;

    /*读取配置文件的sensor名字*/
    ret = LUX_INIParse_GetString(START_INI_CONFIG_FILE_NAME, "sensor", "name", sensorName);
    if (LUX_OK != ret) {
        IMP_LOG_ERR(TAG, "LUX_INIParse_GetString failed,error [0x%x]\n", ret);
        return LUX_ERR;
    }

    /*获取镜头分辨率大小*/
    ret = LUX_COMM_GetSensorWH(sensorName, &g_tSensorWH);
    if (LUX_OK != ret) {
        IMP_LOG_ERR(TAG, "LUX_INIParse_GetString failed,error [0x%x]\n", ret);
        return LUX_ERR;
    }

    /*配置主码流OSD区域的位置参数*/
    switch (rgnNum)
    {
    case 0:
    {
        g_osdAttr[0].OSDRgnAttr[0].rect.p0.x = 10;
        g_osdAttr[0].OSDRgnAttr[0].rect.p0.y = 10;
        if (g_tSensorWH.width == 2560) {
            g_osdAttr[0].OSDRgnAttr[0].rect.p1.x = g_osdAttr[0].OSDRgnAttr[0].rect.p0.x + 20 * OSD_GRP0_REGION_1440P_W - 1;
            g_osdAttr[0].OSDRgnAttr[0].rect.p1.y = g_osdAttr[0].OSDRgnAttr[0].rect.p0.y + OSD_GRP0_REGION_1440P_H - 1;

        }
        else {
            g_osdAttr[0].OSDRgnAttr[0].rect.p1.x = g_osdAttr[0].OSDRgnAttr[0].rect.p0.x + 20 * OSD_GRP0_REGION_1080P_W - 1;
            g_osdAttr[0].OSDRgnAttr[0].rect.p1.y = g_osdAttr[0].OSDRgnAttr[0].rect.p0.y + OSD_GRP0_REGION_1080P_H - 1;
        }
        break;
    }
    case 1:
    {
        g_osdAttr[1].OSDRgnAttr[0].rect.p0.x = LUX_OSD_LOGO_GRP0_START_X;
        g_osdAttr[1].OSDRgnAttr[0].rect.p0.y = LUX_OSD_LOGO_GRP0_START_Y;
        g_osdAttr[1].OSDRgnAttr[0].rect.p1.x = g_osdAttr[1].OSDRgnAttr[0].rect.p0.x + LUX_OSD_GRP0_LOGO_W - 1;
        g_osdAttr[1].OSDRgnAttr[0].rect.p1.y = g_osdAttr[1].OSDRgnAttr[0].rect.p0.y + LUX_OSD_GRP0_LOGO_H - 1;
        break;
    }
#if 0
    case 2:
    {
        g_osdAttr[2].OSDRgnAttr[0].rect.p0.x = g_tSensorWH.width / 2 - 100;
        g_osdAttr[2].OSDRgnAttr[0].rect.p0.y = g_tSensorWH.height / 2 - 100;
        g_osdAttr[2].OSDRgnAttr[0].rect.p1.x = g_osdAttr[2].OSDRgnAttr[0].rect.p0.x + g_tSensorWH.width / 2 - 1 + 50;
        g_osdAttr[2].OSDRgnAttr[0].rect.p1.y = g_osdAttr[2].OSDRgnAttr[0].rect.p0.y + g_tSensorWH.height / 2 - 1 + 50;
        break;
    }

    default:
    {
        IMP_LOG_ERR(TAG, "Invaild rgnNum%d\n", rgnNum);
        return LUX_ERR;
        break;
    }
#endif
    }

    return LUX_OK;
}


/**
 * @description: 创建&注册OSD区域到OSD组、设置OSD组区域属性和区域属性、打开OSD开关.
 * @return     ：返回指向OSD创建区域的地址
 */
INT_X* LUX_OSD_CreateRgn(int grpNum)
{
    int ret;
    IMPRgnHandle* pRgnHander[LUX_OSD_GRP_NUM];
    IMPRgnHandle RgnHanderNum[LUX_OSD_REGION_NUM];
    INT_X i, j;

    pRgnHander[grpNum] = malloc(LUX_OSD_REGION_NUM * sizeof(IMPRgnHandle));
    if (pRgnHander[grpNum] <= 0) {
        IMP_LOG_ERR(TAG, "malloc() prHander[%d] error !\n", grpNum);
        return NULL;
    }

    for (j = 1; j < LUX_RECT_NUM_MAX; j++)
    {
        memcpy(&g_osdAttr[j + LUX_LAST_REGION_NUM], &g_osdAttr[LUX_LAST_REGION_NUM], sizeof(LUX_OSD_ATTR_ST));
    }

    for (i = 0; i < LUX_OSD_REGION_NUM; i++) {
        if (g_osdAttr[i].enable)
        {
            /* 创建OSD区域 */
            RgnHanderNum[i] = IMP_OSD_CreateRgn(NULL);

            if (RgnHanderNum[i] == INVHANDLE) {
                IMP_LOG_ERR(TAG, "IMP_OSD_CreateRgn TimeStamp error !\n");
                return NULL;
            }

            /* 注册OSD区域至OSD组中 */
            ret = IMP_OSD_RegisterRgn(RgnHanderNum[i], grpNum, NULL);
            if (ret < 0) {
                IMP_LOG_ERR(TAG, "IVS IMP_OSD_RegisterRgn failed\n");
                return NULL;
            }

            /* 根据sensor信息配置主码流OSD区域位置参数 */
            ret = LUX_OSD_SetRgnPositionParams(i);
            if (ret < 0) {
                IMP_LOG_ERR(TAG, "IMP_OSD_SetRgnAttr TimeStamp error !\n");
                return NULL;
            }
            /* 设置OSD区域属性 */
            ret = IMP_OSD_SetRgnAttr(RgnHanderNum[i], &g_osdAttr[i].OSDRgnAttr[grpNum]);
            if (ret < 0) {
                IMP_LOG_ERR(TAG, "IMP_OSD_SetRgnAttr TimeStamp error !\n");
                return NULL;
            }

            /* 设置OSD组区域属性 */
            if (IMP_OSD_SetGrpRgnAttr(RgnHanderNum[i], grpNum, &g_osdAttr[i].OSDGrpRgnAttr) < 0) {
                IMP_LOG_ERR(TAG, "IMP_OSD_SetGrpRgnAttr Logo error !\n");
                return NULL;
            }
        }

    }

    /* 启动OSD */
    ret = IMP_OSD_Start(grpNum);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_OSD_Start TimeStamp, Logo, Cover and Rect error !\n");
        return NULL;
    }

    for (i = 0; i < LUX_OSD_REGION_NUM; i++) {
        pRgnHander[grpNum][i] = RgnHanderNum[i];
    }

    return pRgnHander[grpNum];
}

/**
 * @description: 创建OSD组.
 * @returnreturn 0 成功. 非0 失败
 */
INT_X LUX_OSD_CreateGroup(INT_X grpNum)
{
    return IMP_OSD_CreateGroup(grpNum);
}

// extern LUX_COMM_ATTR_ST g_commAttr;
/**
 * @description: OSD模块初始化 读取水印文件
 * @return 0 成功，非0 失败
 */
INT_X LUX_OSD_Init()
{
    INT_X i = 0;
    INT_X ret = LUX_ERR;
    LUX_MODULES_BIND_ST bind;
    FILE* fp = NULL;
#ifdef CONFIG_PTZ_IPC
    fp = fopen("/system/bin/logo/logodata_436x236.bin", "rb");
    if (fp != NULL)
    {
        memset(logodata_436x236_bgra, 0, LOGO_SIZE_HD);
        fread(logodata_436x236_bgra, 1, LOGO_SIZE_HD, fp);
        fclose(fp);
    }
    g_osdAttr[1].OSDRgnAttr[0].data.picData.pData = logodata_436x236_bgra;
#else
    fp = fopen("/system/bin/logo/logodata_382x88.bin", "rb");
    if (fp != NULL)
    {
        memset(logodata_382x88_bgra, 0, LOGO_SIZE_HD);
        fread(logodata_382x88_bgra, 1, LOGO_SIZE_HD, fp);
        fclose(fp);
    }

    g_osdAttr[1].OSDRgnAttr[0].data.picData.pData = logodata_382x88_bgra;
#endif
    fp = fopen("/system/bin/logo/logodata_106x58.bin", "rb");
    if (fp != NULL)
    {
        memset(logodata_106x58_bgra, 0, LOGO_SIZE_SD);
        fread(logodata_106x58_bgra, 1, LOGO_SIZE_SD, fp);
        fclose(fp);
    }
    g_osdAttr[1].OSDRgnAttr[1].data.picData.pData = logodata_106x58_bgra;


    for (i = 0; i < LUX_OSD_GRP_NUM; i++)
    {
        /* Step.1 create group */
        ret = LUX_OSD_CreateGroup(i);
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "IMP_OSD_CreateGroup(%d) error!\n", i);
            return LUX_OSD_CREATE_GROUP_ERR;
        }

        /* Step.2 create osd region */
        g_prHander[i] = LUX_OSD_CreateRgn(i);
        if (g_prHander[i] <= 0)
        {
            IMP_LOG_ERR(TAG, "LUX_OSD_CreateRgn(%d) error!\n", i);
            return LUX_OSD_CREATE_REGION_ERR;
        }

        /* 绑定osd通道到源模块通道 */
        memset(&bind, 0, sizeof(bind));
        bind.srcChan = 0;
        bind.srcModule = (i ? LUX_MODULS_IVS : LUX_MODULS_FSRC); /* osd通道0绑定framesource0，osd通道1绑定ivs通道0（此处通道的概念对应君正组的概念） */
        bind.dstChan = i;
        bind.dstModule = LUX_MODULS_OSD;
        ret = LUX_COMM_Bind_AddModule(&bind);
        // printf("*********************** %s:%d bindNum=%d *********************** \n", __func__, __LINE__,g_commAttr.bindNum-1);//cxj test
        if (LUX_OK != ret)
        {
            IMP_LOG_ERR(TAG, "LUX_COMM_Bind_AddModule (%d) osd error!\n", i);
            return LUX_VIDEO_CHAN_CREATE_ERR;
        }
    }
    g_osdInit = LUX_TRUE;
    return LUX_OK;
}


/**
 * @description: OSD模块去初始化
 * @return 0 成功，非零失败，返回错误码
 */
INT_X LUX_OSD_Exit()
{
    INT_X i, j;
    INT_X ret = LUX_ERR;

    for (i = 0; i < LUX_OSD_GRP_NUM; i++) {
        for (j = 0; j < LUX_OSD_REGION_NUM; j++) {
            if (g_osdAttr[j].enable) {
                /*设置显示区域为不显示*/
                ret = IMP_OSD_ShowRgn(g_prHander[i][j], i, 0);
                if (ret < 0) {
                    IMP_LOG_ERR(TAG, "IMP_OSD_ShowRgn close timeStamp error\n");
                    return LUX_ERR;
                }

                /* 注销OSD区域 */
                ret = IMP_OSD_UnRegisterRgn(g_prHander[i][j], i);
                if (ret < 0) {
                    IMP_LOG_ERR(TAG, "IMP_OSD_UnRegisterRgn timeStamp error\n");
                    return LUX_ERR;
                }

                IMP_OSD_DestroyRgn(g_prHander[i][j]);

            }
        }

        ret = IMP_OSD_DestroyGroup(i);
        if (ret < 0) {
            IMP_LOG_ERR(TAG, "IMP_OSD_DestroyGroup(0) error\n");
            return LUX_ERR;
        }

        free(g_prHander[i]);
        g_prHander[i] = NULL;
    }
    g_osdInit = LUX_FALSE;

    return LUX_OK;
}

