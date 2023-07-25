#!/bin/sh
echo "@@@@@@@@ sd card insert! @@@@@@@@" #> /dev/console
if [ -e "/dev/$MDEV" ]; then
mkdir /system/sdcard
mount -rw /dev/$MDEV /system/sdcard
fi

