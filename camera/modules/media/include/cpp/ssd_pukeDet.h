/*
 * ssd_pukeDet.h
 *
 */

#ifndef __SSD_PUKEDET_H__
#define __SSD_PUKEDET_H__

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
    } pk_objbox;

    int ssd_pukeDet_init(jzdl::BaseNet *pukeDetModel);
    int ssd_pukeDet_start(jzdl::BaseNet *pukeDetModel, vector<pk_objbox> &puke_list, uint8_t *image, int sensor_sub_width, int sensor_sub_height);
    int ssd_pukeDet_exit(jzdl::BaseNet *pukeDetModel);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __SSD_PUKEDET_H__ */
