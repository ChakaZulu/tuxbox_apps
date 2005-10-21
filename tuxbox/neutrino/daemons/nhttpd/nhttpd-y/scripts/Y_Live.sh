#!/bin/sh
# -----------------------------------------------------------
# Live (yjogol)
# $Date: 2005/10/21 13:05:22 $
# $Revision: 1.10 $
# -----------------------------------------------------------

. ./_Y_Globals.sh
. ./_Y_Library.sh

# -----------------------------------------------------------
live_lock()
{
	wget -O - -q "$y_url_control/lcd?lock=1&clear=1&rect=10,10,110,50,1,0&xpos=20&ypos=27&size=22&font=2&text=%20%20%20%20yWeb%0A%20%20LiveView&update=1" >/dev/null
	wget -O - -q "$y_url_control/rc?lock" >/dev/null
	wget -O - -q "$y_url_control/zapto?stopplayback" >/dev/null
}
# -----------------------------------------------------------
live_unlock()
{
	wget -O - -q "$y_url_control/lcd?lock=0" >/dev/null
	wget -O - -q "$y_url_control/rc?unlock"  >/dev/null
	wget -O - -q "$y_url_control/zapto?startplayback" >/dev/null
}
# -----------------------------------------------------------
prepare_tv()
{
	# SPTS on
	wget -O - -q "$y_url_control/system?setAViAExtPlayBack=spts" >/dev/null
}
# -----------------------------------------------------------
prepare_radio()
{
	# SPTS off
	wget -O - -q "$y_url_control/system?setAViAExtPlayBack=pes" >/dev/null
}

# -----------------------------------
# Main
# -----------------------------------
case "$1" in

	zapto)
		if [ "$2" != "" ]
		then
			wget -O - -q "$y_url_control/zapto?$2" >/dev/null
		fi
		;;

	switchto)
		if [ "$2" = "Radio" ]
		then
			wget -O - -q "$y_url_control/setmode?radio" >/dev/null
		else
			wget -O - -q "$y_url_control/setmode?tv" >/dev/null
		fi
		;;

	url)
		url=`buildStreamingRawURL`
		echo "$url" ;;

	audio-url)
		url=`buildStreamingAudioRawURL`
		echo "$url" ;;

	live_lock)
		live_lock ;;

	live_unlock)
		live_unlock ;;

	dboxIP)
		buildLocalIP ;;

	prepare_radio)
		prepare_radio
		url=`buildStreamingAudioRawURL`
		echo "$url" > $y_tmp_m3u
		echo "$url" > $y_tmp_m4u
		;;

	prepare_tv)
		prepare_tv
		;;

	*)
		echo "Parameter falsch: $*" ;;
esac



