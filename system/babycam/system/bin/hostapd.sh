
mkdir -p /var/lib/misc
touch /var/lib/misc/udhcpd.leases
ifconfig wlan0 down
#echo "/lib/firmware/fw_43341_apsta.bin">/sys/module/bcmdhd/parameters/firmware_path
ifconfig wlan0 192.168.1.10 up
hostapd -B /system/init/hostapd.conf
udhcpd  /system/init/udhcpd.conf



