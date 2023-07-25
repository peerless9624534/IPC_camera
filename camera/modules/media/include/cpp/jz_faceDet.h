/*
 * jz_facedet.h
 *
 */

#ifndef __JZ_FACEDET_H__
#define __JZ_FACEDET_H__

#include <imp_common.h>
#include <imp_osd.h>
#include "net.h"

#include <vector>  // 由于没有post_process。需要这里导入
#include <cmath>
#include <algorithm>
 // using namespace std;

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

    typedef struct objbox {
        float x0;
        float y0;
        float x1;
        float y1;
        float score;
        int clsid;
    } objbox;

    int jz_faceDet_init(jzdl::BaseNet* faceDetModel);
    int jz_faceDet_start(jzdl::BaseNet* faceDetModel, std::vector<objbox>& face_list, uint8_t* image, int sensor_sub_width, int sensor_sub_height);
    int jz_faceDet_exit(jzdl::BaseNet* faceDetModel);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __JZ_FACEDET_H__ */
