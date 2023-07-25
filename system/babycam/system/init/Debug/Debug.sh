#!/bin/sh

cp /system/init/Debug/app_init_debug.sh /system/init/app_init.sh
echo 4 > /system/init/Debug/IsDebug.txt
sleep 1
reboot

