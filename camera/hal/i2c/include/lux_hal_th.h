/*
 * lux_hal_th.h
 *
 * temperature & humidity
 */

#ifndef LUX_HAL_TH_H
#define LUX_HAL_TH_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

typedef struct
{
    float cTemp;    /* 摄氏度 */
    float fTemp;    /* 华氏度 */
    float humidity; /* 湿度 */
} LUX_HAL_TH_PARAM;

/* 获取温湿度参数 */
void LUX_HAL_GetTemperatureAndHumidity(LUX_HAL_TH_PARAM *pThParam);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LUX_HAL_TH_H */
