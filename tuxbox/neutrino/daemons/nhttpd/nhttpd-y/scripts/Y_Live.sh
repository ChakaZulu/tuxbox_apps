#!/bin/sh
# -----------------------------------------------------------
# Live (yjogol)
# $Date: 2005/08/31 18:23:49 $
# $Revision: 1.1 $
# -----------------------------------------------------------

. /share/tuxbox/neutrino/httpd-y/scripts/_Y_Globals.sh
. $y_path_scripts/_Y_Webserver_Check.sh
. $y_path_scripts/_Y_Library.sh

# -----------------------------------------------------------
# $1=Bouquet //Bouquets-Liste als HTML-Options. Welche aktiv ist kann nicht festgestellt werden.
# -----------------------------------------------------------
buildHTMLbouquets()
{
	buildHTML=`wget -O - -q $y_url_control/getbouquets|sed -e 's/^. /<option value=&>/g'|sed -e 's/ *$/<\/option>/g'|sed -e "s/value=$1/& selected/g"`
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

# -----------------------------------
# Main
# -----------------------------------
case "$1" in
	panel)
		# switch to TV
		wget -O - -q "$y_url_control/setmode?tv" >/dev/null
		# SPTS on
		wget -O - -q "$y_url_control/system?setAViAExtPlayBack=spts" >/dev/null
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

		echo "$buildHTML"
		echo "" >$y_tmp
		;;

	zapto)
		wget -O - -q "$y_url_control/zapto?$2" >/dev/null
		cat $y_wait_live
		;;
	ie)
		url=`buildStreamingURL`
		buildHTML=`sed -e s/Y_URL/$url/g $y_path_httpd/Y_Live_IE_tmpl.htm`
		echo "$buildHTML"
		;;
	moz)
		url=`buildStreamingURL`
		cmd="sed -e s/Y_URL/$url/g $y_path_httpd/Y_Live_Moz_tmpl.htm"
		buildHTML=`$cmd`
		echo $buildHTML ;;

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



