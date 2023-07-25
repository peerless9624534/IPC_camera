#!/bin/sh

if [ -f /system/etc/sensor/gc2053-t31.bin ]; then
	insmod /system/ko/tx-isp-t31.ko
    insmod /system/ko/sensor_gc2053_t31.ko  data_interface=1
elif [ -f /system/etc/sensor/sc4335-t31.bin ]; then
	insmod /system/ko/tx-isp-t31.ko isp_clk=200000000
	insmod /system/ko/sensor_sc4335_t31.ko
elif [ -f  /system/ko/sc2335-t31.bin ]; then
	insmod /system/ko/tx-isp-t31.ko
	insmod /system/ko/sensor_sc2335_t31.ko
else
	insmod /system/ko/tx-isp-t31.ko
    insmod /system/ko/sensor_gc2053_t31.ko  data_interface=1
fi

insmod /system/ko/sample_pwm_core.ko
insmod /system/ko/sample_pwm_hal.ko
insmod /system/ko/eeprom_at24.ko

insmod /system/ko/avpu.ko
insmod /system/ko/sinfo.ko
insmod /system/ko/audio.ko  spk_gpio=-1 alc_mode=0
insmod /system/ko/8189fs.ko
insmod /system/ko/sample_motor.ko
