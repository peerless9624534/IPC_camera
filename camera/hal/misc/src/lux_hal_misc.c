#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <lux_iniparse.h>
#include <lux_hal_misc.h>

#define TAG     "lux_hal_misc"

/**
 * @description: 红外灯亮灭控制
 * @param [in] enable 开启或关闭
 * @return 0 成功， 非零失败，返回错误码
 */
int LUX_Hal_Misc_IRLightOnOff(int enable)
{
	int fd;
    int ret = -1;
	char on[1]  = {'1'};
    char off[1] = {'0'};
    char gpioNum[32] = {0};
    char tmpPath[64] = {0};

    /*从初始化配置文件中读取配置的gpio口*/
    ret = LUX_INIParse_GetString(START_INI_CONFIG_FILE_NAME, "misc", "irled", gpioNum);
    if(LUX_OK != ret)
    {
        printf(" %s %s:[%d] LUX_INIParse_GetString failed\n", TAG, __FUNCTION__, __LINE__);
        return -1;
    }

    sprintf(tmpPath, "/sys/class/gpio/%s/value", gpioNum);

	fd = open(tmpPath, O_RDWR);
	if(fd < 0) {
		 printf("%s %s:[%d] open error!\n", TAG, __FUNCTION__, __LINE__);
		return -1;
	}

    if (enable)
    {
        write(fd, on, 1);
    }
    else
    {
        write(fd, off, 1);
    }

    close(fd);

	return 0;
}


/**
 * @description: 红外滤光片切换开关
 * @param [in] enable 开启或关闭
 * @return 0 成功， 非零失败，返回错误码
 */
 int LUX_ISP_SetIRCUT(BOOL_X enable)
{
	int fd57, fd58;
	char on[1]  = {'1'};
    char off[1] = {'0'};
    INT_X platform_type = LUX_PLATFORM_TYPE_AC;

    /* 配置文件读取平台类型信息 */
	if (LUX_INIParse_GetInt(START_INI_CONFIG_FILE_NAME, "platform", "platform_type", &platform_type)) {
		printf("%s LUX_INIParse_GetInt failed\n", TAG);
		return -1;
	}

	fd58 = open("/sys/class/gpio/gpio57/value", O_RDWR);
	if(fd58 < 0) {
        printf("%s %s:[%d] open /sys/class/gpio57/value error !", TAG, __FUNCTION__, __LINE__);
		return -1;
	}

	fd57 = open("/sys/class/gpio/gpio58/value", O_RDWR);
	if(fd57 < 0) {
		printf("%s %s:[%d] open /sys/class/gpio58/value error !", TAG, __FUNCTION__, __LINE__);
		return -1;
	}

	if (platform_type == LUX_PLATFORM_TYPE_AC) {
		/* 57=1，58=0：关闭IRCUT；57=0，58=1：开启IRCUT； */
	    if (enable)
	    {
	        write(fd57, on, 1);
	        write(fd58, off, 1);

	    }
	    else
	    {
	        write(fd57, off, 1);
	        write(fd58, on, 1);
	    }
	}else if (platform_type == LUX_PLATFORM_TYPE_PTZ) {
		/* 57=0，58=1：关闭IRCUT；57=1，58=0：开启IRCUT； */
	    if (enable)
	    {
			write(fd57, off, 1);
	        write(fd58, on, 1);

	    }
	    else
	    {
	        write(fd57, on, 1);
	        write(fd58, off, 1);

	    }
	}
	else {
		printf("%s Invaild platform type\n", TAG);
		return -1;
	}

	usleep(160*1000);

    write(fd57, off, 1);
    write(fd58, off, 1);

    close(fd57);
	close(fd58);

	return 0;
}

/**
 * @description: 扬声器开关控制
 * @param [in] enable 开启或关闭
 * @return 0 成功， 非零失败，返回错误码
 */
int LUX_Hal_Misc_SpeakerOnOff(int enable)
{
    int fd;
    int ret = -1;
    char gpioNum[32] = {0};
    char tmpPath[64] = {0};

    /*从初始化配置文件中读取配置的gpio口*/
    ret = LUX_INIParse_GetString(START_INI_CONFIG_FILE_NAME, "misc", "speaker", gpioNum);
    if (LUX_OK != ret)
    {
        printf(" %s %s:[%d] LUX_INIParse_GetString failed\n", TAG, __FUNCTION__, __LINE__);
        return -1;
    }

    sprintf(tmpPath, "/sys/class/gpio/%s/value", gpioNum);

    fd = open(tmpPath, O_RDWR);
    if (fd < 0)
    {
        printf("%s %s:[%d] open error!\n", TAG, __FUNCTION__, __LINE__);
        return -1;
    }

    /* 写入到gpio的value */
    write(fd, (enable ? "1" : "0"), 1);

    close(fd);

    return 0;
}

/**
 * @description: 获取复位按键状态
 * @param [in] enable 开启或关闭
 * @return 0 成功， 非零失败，返回错误码
 */
int LUX_Hal_Misc_ResetKey_GetStatus(void)
{
    int fd;
    int ret = -1;
    char status = 0;
    char gpioNum[32] = {0};
    char tmpPath[64] = {0};

    /*从初始化配置文件中读取配置的gpio口*/
    ret = LUX_INIParse_GetString(START_INI_CONFIG_FILE_NAME, "misc", "reset", gpioNum);
    if (LUX_OK != ret)
    {
        printf(" %s %s:[%d] LUX_INIParse_GetString failed\n", TAG, __FUNCTION__, __LINE__);
        return 0;
    }

    sprintf(tmpPath, "/sys/class/gpio/%s/value", gpioNum);

    fd = open(tmpPath, O_RDWR);
    if (fd < 0)
    {
        printf("%s %s:[%d] open[%s] error!\n", TAG, __FUNCTION__, __LINE__, tmpPath);
        return 0;
    }
    read(fd, &status, 1);
    close(fd);

    return (status == '1') ? 1 : 0;
}
