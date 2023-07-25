/*
 * lux_osd.h
 *
 * OSD叠加功能
 *
 */

#ifndef __LUX_OSD_H__
#define __LUX_OSD_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

#include <comm_def.h>
#include <comm_func.h>
#include <comm_error_code.h>
#include "ivs_inf_faceDet.h"
#include "ivs_inf_personDet.h"
#include "lux_ivs.h"

#define LUX_OSD_LETTER_NUM 		20

#define LUX_OSD_GRP_NUM 		2

#ifdef CONFIG_PTZ_IPC
#define LUX_OSD_GRP0_LOGO_W 436
#define LUX_OSD_GRP0_LOGO_H 236
#else
#define LUX_OSD_GRP0_LOGO_W 382
#define LUX_OSD_GRP0_LOGO_H 88
#endif
#define LUX_OSD_GRP1_LOGO_W 106
#define LUX_OSD_GRP1_LOGO_H 58


#define LUX_OSD_FONT_GRP0_START_X  10
#define LUX_OSD_FONT_GRP0_START_Y  10
#define LUX_OSD_FONT_GRP1_START_X  5
#define LUX_OSD_FONT_GRP1_START_Y  5

#define LUX_OSD_LOGO_GRP0_START_X  16
#define LUX_OSD_LOGO_GRP0_START_Y (SENSOR_HEIGHT_DEFAULT - LUX_OSD_GRP0_LOGO_H - 12)
#define LUX_OSD_LOGO_GRP1_START_X  8
#define LUX_OSD_LOGO_GRP1_START_Y (SENSOR_HEIGHT_SECOND - LUX_OSD_GRP1_LOGO_H - 6)

#define LUX_OSD_COVER_GRP0_START_X SENSOR_WIDTH_DEFAULT/2-100
#define LUX_OSD_COVER_GRP0_START_Y SENSOR_HEIGHT_DEFAULT/2-100
#define LUX_OSD_COVER_GRP1_START_X SENSOR_WIDTH_SECOND/2-100
#define LUX_OSD_COVER_GRP1_START_Y SENSOR_HEIGHT_SECOND/2-100

#define LUX_OSD_RECT_GRP0_START_X  300
#define LUX_OSD_RECT_GRP0_START_Y  300
#define LUX_OSD_RECT_GRP1_START_X  300
#define LUX_OSD_RECT_GRP1_START_Y  300

/*数量*/
#define LUX_MODULES_BIND_NUM    32
#define LUX_FACTRECT_NUM_MAX    NUM_OF_FACES   //人脸矩形框最大数量，根据头文件ivs_inf_faceDet.h中的NUM_OF_FACES选定 
#define LUX_HUMANRECT_NUM_MAX   LUX_FACTRECT_NUM_MAX   //人形矩形框最大数量，跟LUX_FACTRECT_NUM_MAX保持一致
#define LUX_PUKERECT_NUM_MAX    LUX_FACTRECT_NUM_MAX   //吐奶人脸框最大数量，跟LUX_FACTRECT_NUM_MAX保持一致
#define LUX_KICKQUILT_NUM_MAX   LUX_HUMANRECT_NUM_MAX  //踢被人形框最大数量，跟LUX_HUMANRECT_NUM_MAX保持一致
#define LUX_LAST_REGION_NUM     3
#define LUX_RECT_NUM_MAX        (LUX_FACTRECT_NUM_MAX + LUX_HUMANRECT_NUM_MAX + LUX_PUKERECT_NUM_MAX + LUX_KICKQUILT_NUM_MAX) //矩形框最大数量
#define LUX_OSD_REGION_NUM      (LUX_RECT_NUM_MAX + LUX_LAST_REGION_NUM)

/*位置*/
#define LUX_FACE_START_POS      LUX_LAST_REGION_NUM
#define LUX_FACE_END_POS        (LUX_FACE_START_POS + LUX_FACTRECT_NUM_MAX)
#define LUX_HUMAN_START_POS     LUX_FACE_END_POS
#define LUX_HUMAN_END_POS       (LUX_HUMAN_START_POS + LUX_HUMANRECT_NUM_MAX)
#define LUX_PUKE_START_POS      LUX_HUMAN_END_POS
#define LUX_PUKE_END_POS        (LUX_PUKE_START_POS + LUX_PUKERECT_NUM_MAX)
#define LUX_KICKQUILT_START_POS LUX_PUKE_END_POS
#define LUX_KICKQUILT_END_POS   (LUX_KICKQUILT_START_POS + LUX_KICKQUILT_NUM_MAX)

#ifdef CONFIG_LUXALG
#define MISS_NUM                0
#else
#define MISS_NUM                3   //连续MISS_NUM次未识别到才消失
#endif
#define RECT_OSD_SHOW_DELAY     30 * 1000

    typedef IMPRect LUX_OSD_Rect;

    typedef enum {
        LUX_RECT_OSD_BLACK = 0xff000000, /**< 黑色 */
        LUX_RECT_OSD_BLUE = 0xff0000ff, /**< 蓝色 */
        LUX_RECT_OSD_GREEN = 0xff00ff00, /**< 绿色 */
        LUX_RECT_OSD_RED = 0xffff0000, /**< 红色 */
        LUX_RECT_OSD_WHITE = 0xffffffff, /**< 白色 */
    }LUX_RECT_OSD_COLOR_EN;

    typedef struct {
        UINT_X enable;
        PCHAR_X RgnName;
        IMPOSDRgnAttr OSDRgnAttr[LUX_OSD_GRP_NUM];
        IMPOSDGrpRgnAttr OSDGrpRgnAttr;
    }LUX_OSD_ATTR_ST;

    typedef struct
    {
        UINT_X bindNum;
        LUX_MODULES_BIND_ST binds[LUX_MODULES_BIND_NUM];
    } LUX_COMM_ATTR_ST;


    VOID_X LUX_BlackCover_Show(BOOL_X bShow);

    VOID_X LUX_OSD_PersonRect_Show(persondet_param_output_t* ivsPersonDetRet);

    VOID_X LUX_OSD_PersonRect_Miss();

    INT_X LUX_OSD_PersonRect_Update();

    VOID_X LUX_OSD_KickQuiltRect_SSDShow(LUX_PERSONDET_OUTPUT_ST* ivsPersonDetRet);

    VOID_X LUX_OSD_KickQuiltRect_Miss();

    VOID_X LUX_OSD_PukeRect_SSDShow(LUX_FACEDET_OUTPUT_ST* ivsFaceDetRet);

    VOID_X LUX_OSD_PukeRect_Miss();

    VOID_X LUX_OSD_FaceRect_SSDShow(LUX_FACEDET_OUTPUT_ST* ivsFaceDetRet);

    VOID_X LUX_OSD_FaceRect_Show(facedet_param_output_t* ivsFaceDetRet);

    VOID_X LUX_OSD_FaceRect_Miss();

    INT_X LUX_OSD_FaceRect_Update();

    /**
     * @description: osd矩形框绘制接口
     * @param [in] chnID        通道，支持主码流和子码流通道
     * @param [in] rectNum      OSD矩形框区域编号：0到LUX_RECT_NUM_MAX-1
     * @param [in] rect         矩形信息，包含左上角ul和右下角br坐标
     * @param [in] color        矩形框颜色，支持的颜色看LUX_RECT_OSD_COLOR_EN
     * @param [in] linewidth    矩形框线宽
     */
    VOID_X LUX_OSD_Rect_Draw(LUX_STREAM_CHANNEL_EN chnID, INT_X rectNum, LUX_OSD_Rect* rect, LUX_RECT_OSD_COLOR_EN color, INT_X linewidth);

    /**
     * @description: osd矩形框消失接口
     * @param [in] chnID        通道，支持主码流和子码流通道
     * @param [in] rectNum       OSD矩形框区域编号：0到LUX_RECT_NUM_MAX-1
     */
    VOID_X LUX_OSD_Rect_Vanish(LUX_STREAM_CHANNEL_EN chnID, INT_X rectNum);

    /**
     * @description: 设置创建的几个组区域是否显示，
     * @return     ：0 设置成功. 非0 设置失败
     */
    INT_X LUX_OSD_Show(BOOL_X bShow);

    /**
     * @description: 获取sensor的像素宽度和高度，重置主码流各OSD区域的位置参数
    * @return	  ：0 成功. 非0 失败
    */
    INT_X  LUX_OSD_SetRgnPositionParams(int rgnNum);

    /**
     * @description: 创建OSD区域、注册OSD区域到OSD组、设置OSD组区域属性和区域属性，打开OSD开关.
     * @return     ：返回指向OSD创建区域的指针
     */
    INT_X* LUX_OSD_CreateRgn(int grpNum);

    /**
     * @description: 创建OSD的bgramap更新线程，
     * @return     ：0 成功. 非0 失败
     */
    INT_X LUX_OSD_Time_Stamp_Update();

    /**
     * @description: 创建OSD组.
     * @returnreturn 0 成功. 非0 失败
     * @remarks 系统初始化后时间戳自动被初始化。系统去初始化后时间戳失效.
     */
    INT_X LUX_OSD_CreateGroup(INT_X grpNum);

    /**
     * @description: OSD始化
     * @return 0 成功，非零失败，返回错误码
     */
    INT_X LUX_OSD_Init();

    /**
     * @description: OSD初始化
     * @return 0 成功，非零失败，返回错误码
     */
    INT_X LUX_OSD_Exit();


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __LUX_OSD_H__ */
