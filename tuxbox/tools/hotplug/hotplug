#!/bin/sh

exec &1>/dev/console
FIRMWARE_DIR="/var/tuxbox/ucodes"
SYSFS="/sys"


if [ $FIRMWARE != ""  ]; then
    case $ACTION in
	add)
#	    echo "firmware add"
	    if [ ! -e $SYSFS/$DEVPATH/loading ]; then
		sleep 1
	    fi
	    if [ -f $FIRMWARE_DIR/$FIRMWARE ]; then
#		echo "firmware $FIRMWARE_DIR/$FIRMWARE loading"
		echo 1 > $SYSFS/$DEVPATH/loading
		cp $FIRMWARE_DIR/$FIRMWARE $SYSFS/$DEVPATH/data
		echo 0 > $SYSFS/$DEVPATH/loading
	    else
#		echo "firmware $FIRMWARE_DIR/$FIRMWARE not found"
		echo -1 > $SYSFS/$DEVPATH/loading
	    fi
	    ;;
    esac
else
#    echo cmd line: $@
#    echo env :
#    export
if [ -e /dev/.udev ]; then
    /sbin/udev $@
fi
fi
