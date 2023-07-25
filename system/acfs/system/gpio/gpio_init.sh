#!/bin/sh

#speak
echo 7 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio7/direction
echo 0 > /sys/class/gpio/gpio7/active_low
#set gpio value
echo 0 > /sys/class/gpio/gpio7/value

#LED
#gpio9 green, gpio10 red
echo 9 > /sys/class/gpio/export
echo 10 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio9/direction
echo out > /sys/class/gpio/gpio10/direction
echo 0 > /sys/class/gpio/gpio9/active_low
echo 0 > /sys/class/gpio/gpio10/active_low
#set gpio value
echo 1 > /sys/class/gpio/gpio9/value
echo 0 > /sys/class/gpio/gpio10/value

#ir light
echo 59 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio59/direction
echo 0 > /sys/class/gpio/gpio59/active_low
#set gpio value
echo 0 > /sys/class/gpio/gpio59/value

#reset button
echo 50 > /sys/class/gpio/export
echo in > /sys/class/gpio/gpio50/direction
echo 1 > /sys/class/gpio/gpio50/active_low
#get gpio value
cat /sys/class/gpio/gpio50/value

#SD POWER
echo 49 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio49/direction
echo 0 > /sys/class/gpio/gpio49/active_low
#set gpio value
echo 1 > /sys/class/gpio/gpio49/value

#IRCUT
/system/gpio/ircut/gpio_init


#WIFI_CHIP_EN
#echo 60 > /sys/class/gpio/export
#echo out > /sys/class/gpio/gpio60/direction
#echo 0 > /sys/class/gpio/gpio60/active_low
#set gpio value
#echo 1 > /sys/class/gpio/gpio60/value


#WIFI Power
#echo 63 > /sys/class/gpio/export
#echo out > /sys/class/gpio/gpio63/direction
#echo 0 > /sys/class/gpio/gpio63/active_low
#set gpio value, 0 effective
#echo 0 > /sys/class/gpio/gpio63/value
