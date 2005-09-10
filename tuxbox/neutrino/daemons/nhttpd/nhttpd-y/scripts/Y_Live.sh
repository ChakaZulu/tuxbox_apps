#!/bin/sh
# -----------------------------------------------------------
# Live (yjogol)
# $Date: 2005/09/10 12:51:31 $
# $Revision: 1.4 $
# -----------------------------------------------------------

. /share/tuxbox/neutrino/httpd-y/scripts/_Y_Globals.sh
. $y_path_scripts/_Y_Webserver_Check.sh
. $y_path_scripts/_Y_Library.sh

# -----------------------------------------------------------
# $1=Bouquet //Bouquets-Liste als HTML-Options. Welche aktiv ist kann nicht festgestellt werden.
# -----------------------------------------------------------
buildHTMLbouquets()
{
	buildHTML=`wget -O - -q $y_url_control/getbouquets|sed -e 's/^\([^ ]*\) \(.*$\)/<option value=\1>\2<\/option>/g'|sed -e "s/value=$1/& selected/g"`
	echo "$buildHTML"
}
# -----------------------------------------------------------
# $1=Bouquet // Channel Liste
# -----------------------------------------------------------
buildHTMLbouquetChannels()
{
	buildHTML=`wget -O - -q $y_url_control/getbouquet?bouquet=$1|sed -e 's/^\([^ ]*\) \([^ ]*\) \(.*$\)/<option value=\2>\1 \3<\/option>/g'`
	echo "$buildHTML"
}
# -----------------------------------------------------------
live_lock()
{
	wget -O - -q "$y_url_control/lcd?lock=1&clear=1&rect=10,10,110,50,1,0&xpos=20&ypos=27&size=22&font=2&text=%20%20%20%20Y-Web%0A%20%20LiveView&update=1" >/dev/null
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
		# SPTS on
		wget -O - -q "$y_url_control/system?setAViAExtPlayBack=pes" >/dev/null
}

# -----------------------------------------------------------
live_getmode()
{
	mode=`wget -O - -q "$y_url_control/getmode"`
	mode=`echo "$mode"|sed -e "s/tv.*/tv/1"`
	echo "$mode"
}
# -----------------------------------------------------------
live_buildpage()
{
	mode=`live_getmode`
	if [ "$mode" = "tv" ]
	then
		prepare_tv
		url=`buildStreamingURL`
		case "$2" in
			ie)
				buildHTML=`sed -e s/Y_URL/$url/g $y_path_httpd/Y_Live_IE_tmpl.htm` ;;
			moz)
				buildHTML=`sed -e s/Y_URL/$url/g $y_path_httpd/Y_Live_Moz_tmpl.htm` ;;
		esac
		echo "$buildHTML"
	else
		prepare_radio
		url=`buildStreamingAudioRawURL`
#		echo "url:($url) m3u:($y_tmp_m3u)"
		echo "$url" > $y_tmp_m3u
		echo "$url" > $y_tmp_m4u
		cat $y_path_httpd/Y_Live_Radio.htm
	fi
}

live_buildpanel()
{
	# actual Bouquet
	actualBouquet=`wget -O - -q $y_url_control/getbouquet?actual`
	if [ "$2" = "" ]
	then
		bouquet="$actualBouquet"
	else
		bouquet="$2"
	fi
		bouquets=`buildHTMLbouquets $bouquet`
	echo "$bouquets" >$y_tmp
	buildHTML=`sed "/Y_Bouquets/r $y_tmp" $y_path_httpd/Y_Live_panel_tmpl.htm`
		channels=`buildHTMLbouquetChannels $bouquet`
	actualChannel=`wget -O - -q $y_url_control/zapto`
	channels=`echo $channels|sed -e "s/$actualChannel/& selected/g"`
	echo "$channels" >$y_tmp
	buildHTML=`echo "$buildHTML"|sed "/Y_Channels/r $y_tmp"`

	mode=`live_getmode`
	if [ "$mode" = "tv" ]
	then
		ymode="Radio"
	else
		ymode="TV"
	fi
	buildHTML=`echo "$buildHTML"|sed -e "s/Y_mode/$ymode/g"`

	echo "$buildHTML"
	echo "" >$y_tmp
}
# -----------------------------------
# Main
# -----------------------------------
case "$1" in
	panel)
		live_buildpanel	;;

	zapto)
		wget -O - -q "$y_url_control/zapto?$2" >/dev/null
		cat $y_wait_live
		;;

	switchto)
		if [ "$2" = "Radio" ]
		then
			wget -O - -q "$y_url_control/setmode?radio" >/dev/null
		else
			wget -O - -q "$y_url_control/setmode?tv" >/dev/null
		fi
		cat $y_path_httpd/Y_LiveView.htm
		;;

	live)
		live_buildpage $*
		;;

	url)
		url=`buildStreamingRawURL`
		echo "$url" ;;

	audio-url)
		url=`buildStreamingAudioRawURL`
		echo "$url" ;;

	buildHTMLbouquets)
		buildHTMLbouquets ;;

	live_lock)
		live_lock ;;

	live_unlock)
		live_unlock ;;

	*)
		echo "Parameter falsch: $*" ;;
esac



