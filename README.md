# chip-i2c-thingspeak

A simple Linux app for the C.H.I.P. single-board computer (http://getchip.com/)
It gathers data using the I2C ports. It assumes an AXP209 PMU at port 0, and a Si7021 temperature/humidity sensor at port 1. You can change them in config.h. Then it submits the data (actually just builds an URL which would submit the data) to ThingSpeak (https://thingspeak.com/)

# Compiling
$ make config.h

Edit config.h and specify your API keys for ThingSpeak. If needed, change the I2C port numbers.

$ make

# Running

$ ./report.sh

This script will submit your data once every 15 seconds. You need wget installed.

# Auto-start on boot

Add the following to /etc/rc.local before "exit 0":

/usr/sbin/i2cset -f -y 0 0x34 0x82 0xff

su -c '/home/chip/chip-i2c-thingspeak/report.sh &' chip
