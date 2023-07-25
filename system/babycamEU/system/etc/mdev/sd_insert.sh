#!/bin/sh
echo "@@@@@@@@ sd card insert! @@@@@@@@" > /dev/console
if [ -e "/dev/mmcblk0p1" ]; then
mkdir /mnt/sdcard
mount -rw /dev/mmcblk0p1 /mnt/sdcard
fi
if [ -e "/dev/mmcblk0p2" ]; then
mkdir /system/sdcard2
mount -rw /dev/mmcblk0p2 /system/sdcard2
fi

