#!/bin/sh
#APP_INIT_VERTION_003
mdev -s
/system/gpio/gpio_init.sh

wpa_supplicant -D nl80211 -i wlan0 -c /system/init/wpa_supplicant.conf -B

/system/init/checkwlan0

/system/bin/media_sample > /dev/null 2>&1 &

/system/bin/media_sample save > /dev/null &
#/system/init/app_wdt &
