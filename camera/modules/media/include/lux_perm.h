/*
 * lux_perm.h
 *
 * 周界检测
 * 
 */

#ifndef LUX_PERM_H
#define LUX_PERM_H

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

/*
 * 周界信息结构体
 */
typedef struct
{
    unsigned int x; /**< 周界信息，只有4个点 */
    unsigned int y;
    unsigned int width;
    unsigned int height;
} LUX_IVS_PERM_INPUT_POINT;


#define MAX_PERM_AREA_POINTS_NUM      (6)     //最大点数
//Detection area structure
typedef struct{
    int pointX;    //Starting point x  [0-100]
    int pointY;    //Starting point Y  [0-100]
}ALARM_POINT;

typedef struct{
    int iPointNum;   //区域点数 //至少3点且不在一条直线上
    ALARM_POINT alarmPonits[MAX_PERM_AREA_POINTS_NUM];
}LUX_ALARM_AREA_ST;


int LUX_IVS_Perm_Init();
int LUX_IVS_Perm_Exit();
void LUX_IVS_Perm_Json2St_2(CHAR_X * p_alarm_zone, LUX_ALARM_AREA_ST* strAlarmAreaInfo);
int LUX_IVS_Perm_ConfigArea(LUX_IVS_PERM_INPUT_POINT *pPoint);
int LUX_IVS_Perm_ConfigArea2(LUX_ALARM_AREA_ST *alarmArea);
int LUX_IVS_Perm_Start();
int LUX_IVS_Perm_Stop();

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* LUX_PERM_H */
