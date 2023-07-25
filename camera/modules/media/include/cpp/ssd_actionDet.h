/*
 * ssd_actionDet.h
 *
 */

#ifndef __SSD_ACTIONDET_H__
#define __SSD_ACTIONDET_H__

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
    } act_objbox;

    int ssd_actionDet_init(jzdl::BaseNet *actionDetModel);
    int ssd_actionDet_start(jzdl::BaseNet *actionDetModel, vector<act_objbox> &action_list, uint8_t *image, int sensor_sub_width, int sensor_sub_height);
    int ssd_actionDet_exit(jzdl::BaseNet *actionDetModel);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __SSD_ACTIONDET_H__ */
