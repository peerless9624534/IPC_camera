#ifndef __IVS_INF_FACEDET_H__
#define __IVS_INF_FACEDET_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

#include "ivs_common.h"

#define NUM_OF_FACES 10

/*
 * 人脸信息角度结构体
 */
typedef struct {
    float yaw;
    float pitch;
    float roll;
}face_pose;

/*
 * 人脸信息结构体
 */
typedef struct {
    IVSRect box; /**< 人脸区域坐标,用来做人脸识别 */
    IVSRect show_box; /**< 人脸区域真实坐标 */
    face_pose face_pose_res; /**< 人脸角度**/
    float confidence; /**< 人脸检测结果的置信度 */
    float blur_score; /**< 人脸模糊得分 1:模糊 0:清晰 */
    int landmark[10];
    int landmark10[20];
    int left_eye_status; /**< 睁眼闭眼检测-左  1:睁眼 0:闭眼 -1:不确定*/
    int right_eye_status; /**< 睁眼闭眼检测-右  1:睁眼 0:闭眼 -1:不确定*/
    float faceLivenessScore; /**< 人脸活体检测得分: 当得分大于0.99时为非活体 */
}face_info;

/*
 * 人脸检测输入结构体
 */
typedef struct {
    unsigned int max_face_box; /**< 人脸检测处理过程中保留的框数量 */
    int sense; /**< 检测灵敏度 0~3 0:最不灵敏 3:最灵敏 default:1 */
    int detdist; /**< 检测距离 0~2  0:3.5米 max(img_w, img_h) >= 320 ;  1:5米  max(img_w, img_h) >= 480 \
                    2:6米 max(img_w, img_h) >= 640  default:0 */
    int skip_num; /**< 跳帧数目 */
    unsigned int delay; /**< 延时识别时间，默认为0 */
    IVSFrameInfo frameInfo; /**< 帧信息 */
    bool rot90; /**< 图像是否顺时针旋转90度*/
    bool switch_liveness; /**< 人脸活体检测模块开关, 当true时会做活体检测, 当false时不做活体检测返回-1 */
    bool switch_face_pose; /**< 人脸角度检测模块开关, 当true时会做人脸角度检测, 当false时不做人脸角度检测返回-1 */
    bool switch_face_blur; /**< 人脸模糊检测模块开关, 当true时会做人脸模糊检测, 当false时不做人脸模糊检测返回-1 */
    bool switch_landmark;	/* <人脸关键检测开关> */
}facedet_param_input_t;

/*
 * 人脸检测输出结构体
 */
typedef struct {
    int count; /**< 识别出人脸的个数 */
    face_info face[NUM_OF_FACES]; /**< 识别出的人脸信息 */
    int64_t timeStamp; /**< 时间戳 */
}facedet_param_output_t;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
#endif /* __IVS_INF_FACEDET_H__ */
