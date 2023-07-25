#!/bin/sh

cp /system/init/backup/media_config.ini /system/init/
cp /system/init/backup/tuya_config.ini /system/init/
cp /system/init/backup/hostapd.conf /system/init/
cp /system/init/backup/udhcpd.conf /system/init/
cp /system/init/backup/wpa_supplicant.conf /system/init/
cp /system/init/Debug/app_init.sh /system/init/app_init.sh

rm /system/etc/first_set_wifi
rm /system/bin/unnormalReboot.txt
rm /system/init/Debug/IsDebug.txt
rm /system/bin/audio/Mycradlesong.pcm

rm /system/tuya/*
rm /system/init/event/*
