/*
 * ssd_kickquiltDet.h
 *
 */

#ifndef __SSD_KICKQUILTDET_H__
#define __SSD_KICKQUILTDET_H__

#include <imp_common.h>
#include <imp_osd.h>
#include "net.h"

#include <vector> // 由于没有post_process。需要这里导入
#include <cmath>
#include <algorithm>
using namespace std;

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

    typedef struct
    {
        float x0;
        float y0;
        float x1;
        float y1;
        float score;
        int clsid;
    } kq_objbox;

    int ssd_kickquiltDet_init(jzdl::BaseNet *kickquiltDetModel);
    int ssd_kickquiltDet_start(jzdl::BaseNet *kickquiltDetModel, vector<kq_objbox> &kickquilt_list, uint8_t *image, int sensor_sub_width, int sensor_sub_height);
    int ssd_kickquiltDet_exit(jzdl::BaseNet *kickquiltDetModel);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __SSD_KICKQUILTDET_H__ */
