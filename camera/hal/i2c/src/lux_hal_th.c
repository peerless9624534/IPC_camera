/* 
 * 温湿度传感器功能
 * 作者：panloyz
 */

#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "lux_hal_th.h"

#define LUX_HAL_TH_I2C_BUS  "/dev/i2c-1"
#define LUX_HAL_TH_I2C_ADDR 0x44

void LUX_HAL_GetTemperatureAndHumidity(LUX_HAL_TH_PARAM *pThParam)
{
    if (NULL == pThParam)
    {
        printf("%s:%d error null ptr\n", __func__, __LINE__);
        return ;
    }
#ifdef CONFIG_BABY
    // Create I2C bus
    int file;
    if ((file = open(LUX_HAL_TH_I2C_BUS, O_RDWR)) < 0)
    {
        printf("Failed to open [%s] bus.\n", LUX_HAL_TH_I2C_BUS);
        return ;
    }
    // Get I2C device, SHT31 I2C address is 0x44(68)
    ioctl(file, I2C_SLAVE, LUX_HAL_TH_I2C_ADDR);

    // Send high repeatability measurement command
    // Command msb, command lsb(0x2C, 0x06)
    char config[2] = {0};
    config[0] = 0x2C;
    config[1] = 0x06;
    write(file, config, 2);
    usleep(5000);   /* sleep 5ms */

    // Read 6 bytes of data
    // temp msb, temp lsb, temp CRC, humidity msb, humidity lsb, humidity CRC
    unsigned char data[6] = {0};
    if (read(file, data, 6) != 6)
    {
        printf("Error : Input/output Error \n");
    }
    else
    {
        // Convert the data
        pThParam->cTemp = (((data[0] << 8) + data[1]) * 175.0) / 65535.0 - 45.0;
        pThParam->fTemp = (((data[0] << 8) + data[1]) * 315.0) / 65535.0 - 49.0;
        pThParam->humidity = (((data[3] << 8) + data[4])) * 100.0 / 65535.0;

        // Output data to screen
        //printf("Temperature in Celsius : %.2f C \n", pThParam->cTemp);
        //printf("Temperature in Fahrenheit : %.2f F \n", pThParam->fTemp);
        //printf("Relative Humidity is : %.2f RH \n", pThParam->humidity);
    }
    close(file);
#endif
    return ;
}
