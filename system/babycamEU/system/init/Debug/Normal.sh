#!/bin/sh

cp /system/init/Debug/app_init.sh /system/init/app_init.sh
echo 0 > /system/init/Debug/IsDebug.txt

sleep 1
reboot

