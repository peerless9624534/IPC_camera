echo 1 > /sys/class/gpio/gpio58/value
echo 0 > /sys/class/gpio/gpio57/value
sleep "1"
echo 0 > /sys/class/gpio/gpio58/value
