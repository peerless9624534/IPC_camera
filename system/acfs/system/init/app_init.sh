#!/bin/sh

mdev -s
/system/gpio/gpio_init.sh

wpa_supplicant -D nl80211 -i wlan0 -c /system/init/wpa_supplicant.conf -B

/system/bin/media_sample&
