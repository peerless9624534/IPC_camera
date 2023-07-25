#!/bin/sh
echo "@@@@@@@@ sd card remove! @@@@@@@@" #> /dev/console
umount -l /system/sdcard*
rm –rf /system/sdcard*

