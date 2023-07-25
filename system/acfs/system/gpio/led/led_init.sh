echo 9 > /sys/class/gpio/export
echo 10 > /sys/class/gpio/export

echo out > /sys/class/gpio/gpio9/direction
echo out > /sys/class/gpio/gpio10/direction

echo 0 > /sys/class/gpio/gpio9/active_low
echo 0 > /sys/class/gpio/gpio10/active_low

echo 0 > /sys/class/gpio/gpio9/value
echo 0 > /sys/class/gpio/gpio10/value
