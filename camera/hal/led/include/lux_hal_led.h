/*
 * lux_audio.h
 *
 * led
 */

#ifndef __LUX_HAL_LED_H__
#define __LUX_HAL_LED_H__

#include "comm_def.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

typedef enum
{
    LUX_HAL_LED_GREEN,
    LUX_HAL_LED_RED,
    LEX_HAL_LED_MAX,
} LUX_HAL_LED_EN;

typedef enum
{
    LUX_HAL_LED_OFF,        /*常灭*/
    LUX_HAL_LED_ON,         /*常亮*/
    LUX_HAL_LED_1_HZ,       /*闪烁*/
    LUX_HAL_LED_2_HZ,
    LUX_HAL_LED_3_HZ,
    LUX_HAL_LED_4_HZ,
    LUX_HAL_LED_BUTTON,
} LUX_HAL_LED_STATUS_EN;

/**
 * @description: 设置led灯的状态
 * @param [in] colour  灯的颜色
 * @param [in] status  led灯状态
 * @return 0 成功， 非零失败
 */
int LUX_HAL_LedSetStatus(LUX_HAL_LED_EN colour, LUX_HAL_LED_STATUS_EN status);

/**
 * @description: led初始化
 * @return 0 成功， 非零失败
 */
int LUX_HAL_LedInit(void);

/**
 * @description: led去初始化
 * @return 0 成功， 非零失败
 */
//void LUX_HAL_LedDeinit(void);
int LUX_HAL_LedEnable(UINT8_X enable);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __LUX_HAL_LED_H__ */
