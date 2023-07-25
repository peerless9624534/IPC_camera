#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/prctl.h> 

#include <lux_hal_led.h>
#include <lux_iniparse.h>
#include <comm_def.h>


#define TAG       "lux_hal_led"


typedef struct 
{
    int  bUsed;
    int  fdLed;
    char colour[16];
    int  flashStatus;
    int  preStatus;    
    pthread_t threadId;
    int  pthreadRuning;
} LUX_HAL_LED_ST;

LUX_HAL_LED_ST  g_ledInfo[LEX_HAL_LED_MAX] =
{
    {
        .colour = "green",
        .flashStatus = LUX_HAL_LED_ON,
    },
    {
        .colour = "red",
        .flashStatus = LUX_HAL_LED_OFF,
    },
};

/**
 * @description: led灯开关
 * @param [in] colour  灯的颜色
 * @param [in] enable 开启或关闭
 * @return 0 成功， 非零失败
 */
static int LUX_HAL_LedOnOff(LUX_HAL_LED_EN colour, int enable)
{
    char on[1]  = {'1'};
    char off[1] = {'0'};

    if(enable)
    {
        write(g_ledInfo[colour].fdLed, on, 1);
    }
    else
    {
        write(g_ledInfo[colour].fdLed, off, 1);
    }

    return 0;
}


static void *Therad_LedFlash(void *args)
{
    int	ret = -1;
    LUX_HAL_LED_EN colour = (LUX_HAL_LED_EN)args;
    int timeInterval;
    char threadName[32] = {0};

    sprintf(threadName, "led_%s", g_ledInfo[colour].colour);
    prctl(PR_SET_NAME, threadName);
    nice(15);//

    while(1)
    {
        if(!g_ledInfo[colour].bUsed)
        {
            sleep(1);
            continue;
        }
        /*led 处于常量或者常灭状态*/
        if(g_ledInfo[colour].flashStatus < LUX_HAL_LED_1_HZ)
        {
            ret = LUX_HAL_LedOnOff(colour, g_ledInfo[colour].flashStatus);
            if (0 != ret)
            {
                printf(" %s %s:[%d]LUX_HAL_LedOnOff failed.\n", TAG, __FUNCTION__, __LINE__);
		        return (void *)-1;
	        }
            g_ledInfo[colour].preStatus = g_ledInfo[colour].flashStatus;
            usleep(300 * 1000);
        }
        /*闪烁状态*/
        else if ((g_ledInfo[colour].flashStatus >= LUX_HAL_LED_1_HZ) && (g_ledInfo[colour].flashStatus < LUX_HAL_LED_BUTTON))
        {
            /*计算闪烁时间间隔*/
            timeInterval = 1000 * 1000 / ((g_ledInfo[colour].flashStatus - 1) * 2);

            ret = LUX_HAL_LedOnOff(colour, 1);
            if (0 != ret)
            {
                printf(" %s %s:[%d]LUX_HAL_LedOnOff failed.\n", TAG, __FUNCTION__, __LINE__);
                return (void *)-1;
            }
            usleep(timeInterval);

            ret = LUX_HAL_LedOnOff(colour, 0);
            if (LUX_OK != ret)
            {
                printf(" %s %s:[%d]LUX_HAL_LedOnOff failed.\n", TAG, __FUNCTION__, __LINE__);
                return (void *)-1;
            }

            g_ledInfo[colour].preStatus = g_ledInfo[colour].flashStatus;
            usleep(timeInterval);
        }
        else
        {
            usleep(300 * 1000);
        }
    }

    LUX_HAL_LedOnOff(colour, 0);

	return (void *)0;

}

/**
 * @description: 设置led灯的状态
 * @param [in] colour  灯的颜色
 * @param [in] status  led灯状态
 * @return 0 成功， 非零失败
 */
int LUX_HAL_LedSetStatus(LUX_HAL_LED_EN colour, LUX_HAL_LED_STATUS_EN status)
{
    if( colour >= LEX_HAL_LED_MAX || status >= LUX_HAL_LED_BUTTON )
    {
        printf(" %s %s:[%d]LUX_HAL_LedSetStatus failed error parameter.\n", TAG, __FUNCTION__, __LINE__);
        return -1;
    }

    g_ledInfo[colour].preStatus   = g_ledInfo[colour].flashStatus;
    g_ledInfo[colour].flashStatus = status;

    /*if(0 == g_ledInfo[colour].bUsed)
    {
        printf(" %s %s:[%d] error, %s Uninitialized \n", TAG, __FUNCTION__, __LINE__, g_ledInfo[colour].colour);
        return -1;
    }*/

    return 0;
}


/**
 * @description: led初始化
 * @return 0 成功， 非零失败
 */
int LUX_HAL_LedInit(void)
{
    int ret = -1;
    int i = 0;
    char gpioNum[64] = {0};
    char tmpPath[128] = {0};

    for(i = 0; i < LEX_HAL_LED_MAX; i++)
    {
        if(g_ledInfo[i].bUsed)
        {     
            continue;
        }

        /*从初始化配置文件中读取配置的gpio口*/
        ret = LUX_INIParse_GetString(START_INI_CONFIG_FILE_NAME, "led", g_ledInfo[i].colour, gpioNum);
        if(LUX_OK != ret)
        {
            printf(" %s %s:[%d] LUX_INIParse_GetString failed\n", TAG, __FUNCTION__, __LINE__);
            return -1;
        }

        sprintf(tmpPath, "/sys/class/gpio/%s/value", gpioNum);

        g_ledInfo[i].fdLed = open(tmpPath, O_RDWR);
        if(g_ledInfo[i].fdLed < 0)
        {
            printf(" %s %s:[%d] open %s error !\n", TAG, __FUNCTION__, __LINE__, tmpPath);
            return -1;
        }

        g_ledInfo[i].pthreadRuning = 1;
        if(0 != pthread_create(&g_ledInfo[i].threadId, NULL, Therad_LedFlash, (void*)i))
        {
            printf("%s:%d pthread_create error!\n", __func__, __LINE__);
            g_ledInfo[i].pthreadRuning = 0;
            return -1;
        }
        g_ledInfo[i].bUsed = 0;
        LUX_HAL_LedOnOff(i, 0);
    }

    return 0;

}

int LUX_HAL_LedEnable(uint8_t enable)
{
    int i;
    for(i = 0; i < LEX_HAL_LED_MAX; i++)
    {
        g_ledInfo[i].bUsed = enable;
        LUX_HAL_LedOnOff(i, 0);
    }
}

/**
 * @description: led去初始化
 * @return 0 成功， 非零失败
 */
void LUX_HAL_LedDeinit(void)
{
    int i;

    for(i = 0; i < LEX_HAL_LED_MAX; i++)
    {
        if(g_ledInfo[i].bUsed)
        {  
            /*线程取消函数使用确保线程中有系统调用如sleep等,否则线程不会退出*/
            //pthread_cancel(g_ledInfo[i].threadId);
            g_ledInfo[i].pthreadRuning = 0;
            pthread_join(g_ledInfo[i].threadId, NULL);
            close(g_ledInfo[i].fdLed);
            g_ledInfo[i].bUsed = 0;
        }
    }

    return;
}
