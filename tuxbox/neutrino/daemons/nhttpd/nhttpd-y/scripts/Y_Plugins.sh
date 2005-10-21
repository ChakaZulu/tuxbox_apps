#!/bin/sh
# -----------------------------------------------------------
# Plugins (yjogol)
# $Date: 2005/10/21 13:05:22 $
# $Revision: 1.7 $
# -----------------------------------------------------------

. ./_Y_Globals.sh
. ./_Y_Library.sh

# ===========================================================
# Plugin : VNC
# ===========================================================
vnc_set()
{
	if [ "$#" -ne 4 ]
	then
		echo "zu wenig Parameter ($*)"
	else
		config_open $y_config_vnc
		config_set_value 'server' $1
		config_set_value 'port' $2
		config_set_value 'passwd' $3
		config_set_value 'scale' $4
		config_write $y_config_vnc
		msg="VNC Parameter uebernommen"
		y_format_message_html
	fi
}

# ===========================================================
# Settings : Skins
# ===========================================================
# -----------------------------------------------------------
# Skin Liste
# -----------------------------------------------------------
skin_get()
{
	check_Y_Web_conf
	active_skin=`config_get_value_direct $y_config_Y_Web 'skin'`
	html_option_list=""
	skin_list=`find $y_path_httpd -name 'Y_Main-*'`
	for f in $skin_list
	do
		skin=`echo "$f"|sed -e s/^.*Y_Main-//g|sed -e s/.css//g`
		if [ "$skin" = "$active_skin" ]
		then
			selec="selected"
		else
			selec=""
		fi
		opt="<option $selec value='$skin'>$skin</option>"
		html_option_list="$html_option_list $opt"
	done
	echo "$html_option_list"
}

# -----------------------------------------------------------
# Skin setzen : css ueberschreiben  $1=Skin-Name
# -----------------------------------------------------------
skin_set()
{
	cd $y_path_httpd
	cp Y_Main-$1.css Y_Main.css
	if [ -e global-$1.css ]
	then
		cp global-$1.css global.css
	else
		cp global-Standard.css global.css
	fi
	config_set_value_direct $y_config_Y_Web 'skin' $1

	msg="Skin geaendert - Jetzt Browser Refresh/Aktualisierung ausfuehren"
	y_format_message_html
}
# ===========================================================
# Settings : WebServer (nhttpd)
# ===========================================================
nhttpd_set()
{
	if [ "$#" -ne 3 ]
	then
		echo "zu wenig Parameter ($*)"
	else
		config_open $y_config_nhttpd
		config_set_value 'AuthPassword' $1
		config_set_value 'AuthUser' $2
		config_set_value 'Authenticate' $3
#		config_set_value 'Port' $4
		config_write $y_config_nhttpd
		msg="<b>nhttpd Parameter uebernommen</b><br>Die Aenderungen werden erst nach dem Neustart der Box uebernommen<br><br><a href="/control/exec?Y_Reboot"><b><u>Reboot</u></b></a>"
		y_format_message_html
	fi
}

# ===========================================================
# Settings : yWeb
# ===========================================================
yWeb_set()
{
		config_open $y_config_Y_Web
		config_set_value 'live_resolution_w' $1
		config_set_value 'live_resolution_h' $2
		config_set_value 'slavebox' $3
		
		config_write $y_config_Y_Web
		msg="<b>Parameter uebernommen</b><br>Wenn die Aufloesung geaendert wurde, dann Browser Refresh"
		y_format_message_html
}

# -----------------------------------------------------------
# Main
# -----------------------------------------------------------

case "$1" in
	vnc_set)
        if [ "$#" -ne 5 ]
        then
            echo "vnc_set: zu wenig Parameter ($*)"
        else
		vnc_set $2 $3 $4 $5
	fi ;;

	nhttpd_set)
        	shift 1
		nhttpd_set $* ;;

	skin_set)
		skin_set $2 ;;

	skin_get)
		skin_get ;;

	yWeb_set)
		shift 1
		yWeb_set $* ;;

	*)
		echo "Parameter falsch: $*" ;;
esac

