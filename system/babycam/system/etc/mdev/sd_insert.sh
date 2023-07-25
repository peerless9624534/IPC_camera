#!/bin/sh
echo "@@@@@@@@ sd card insert! $MDEV $ACTION @@@@@@@@" > /dev/console
case $MDEV in
mmcblk0p1)
    MOUNTPOINT=/mnt/sdcard
    ;;
mmcblk0p2)
    MOUNTPOINT=/system/sdcard2
    ;;
*)
    exit 0
    ;;
esac
mkdir -p $MOUNTPOINT
mount -t vfat /dev/$MDEV $MOUNTPOINT -o rw,errors=continue


