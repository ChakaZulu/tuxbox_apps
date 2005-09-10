#!/bin/sh
# -----------------------------------------------------------
# Plugins (yjogol)
# $Date: 2005/09/10 12:50:31 $
# $Revision: 1.2 $
# -----------------------------------------------------------

. /share/tuxbox/neutrino/httpd-y/scripts/_Y_Globals.sh
. $y_path_scripts/_Y_Library.sh

# ===========================================================
# Plugin : VNC
# ===========================================================
vnc_build_form()
{
	config_open $y_config_vnc

	vnc_server=`config_get_value 'server'`
	vnc_port=`config_get_value 'port'`
	vnc_pw=`config_get_value 'passwd'`
	vnc_scale=`config_get_value 'scale'`

	buildPage=`sed -e s/Y_server/$vnc_server/g $y_path_httpd/Y_Plugins_VNC.htm | sed -e s/Y_port/$vnc_port/g | sed -e s/Y_password/$vnc_pw/g | sed -e s/Y_scale/$vnc_scale/g`
	echo "$buildPage"
}

# -----------------------------------------------------------
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
		echo "VNC Parameter uebernommen"
	fi
}

# -----------------------------------------------------------
# BETA: Hier wird noch gebastelt
# -----------------------------------------------------------
news_build_form()
{
	a=`cat /var/tuxbox/config/tuxnews/tuxnews.list`
#	echo "a:$a"
z=""
	echo "$a" | \
        for i in 0 1 2 3 4 5 6 7 8 9
        do
            x=read _line
z="$z $i $_line"
#			echo "ii:$_line"
#			echo "X:$x"
#            if [ "$x" ]
#            then
#				echo "i:$_line"
#            fi
        done
	echo "$z"
}
# ===========================================================
# Settings : Y-Web : Skins
# ===========================================================

# -----------------------------------------------------------
# Skin Liste
# -----------------------------------------------------------
skin_get()
{
	if ! [ -e $y_config_Y_Web ]
	then
		echo "skin=Tuxbox" >$y_config_Y_Web
	fi
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
		opt="<option $selec value='$skin'>$skin<\/option>"
#		echo "skin:($skin) opt:($opt)"
		html_option_list="$html_option_list $opt"
	done
	echo "$html_option_list"
}
# -----------------------------------------------------------
skin_build_form()
{
	opts=`skin_get`
	buildPage=`sed -e "s/Y_opts/$opts/g" $y_path_httpd/Y_Settings_Skin.htm`
	echo "$buildPage"
}

# -----------------------------------------------------------
# Skin setzen : css ueberschreiben
# $1=Skin-Name
# -----------------------------------------------------------
skin_set()
{
	cp $y_path_httpd/Y_Main-$1.css $y_path_httpd/Y_Main.css
	if [ -e $y_path_httpd/global-$1.css ]
	then
		cp $y_path_httpd/global-$1.css $y_path_httpd/global.css
	else
		cp $y_path_httpd/global-Standard.css $y_path_httpd/global.css
	fi
	config_set_value_direct $y_config_Y_Web 'skin' $1

	echo "Skin geaendert - Jetzt Browser Refresh/Aktualisierung ausfuehren"
}
# ===========================================================
# Settings : WebServer (nhttpd)
# ===========================================================
nhttpd_build_form()
{
	config_open $y_config_nhttpd

	yAuthPassword=`config_get_value 'AuthPassword'`
	yAuthUser=`config_get_value 'AuthUser'`
	yAuthenticate=`config_get_value 'Authenticate'`
#	yPort=`config_get_value 'Port'`

#	buildPage=`sed -e s/Y_authpassword/$yAuthPassword/g $y_path_httpd/Y_Settings_nhttpd.htm | sed -e s/Y_authuser/$yAuthUser/g | sed -e s/Y_authenticate/$yAuthenticate/g | sed -e s/Y_port/$yPort/g`
	buildPage=`sed -e s/Y_authpassword/$yAuthPassword/g $y_path_httpd/Y_Settings_nhttpd.htm | sed -e s/Y_authuser/$yAuthUser/g | sed -e s/Y_authenticate/$yAuthenticate/g`
	echo "$buildPage"
}

# -----------------------------------------------------------
nhttpd_set()
{
	if [ "$#" -ne 4 ]
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

# -----------------------------------------------------------
# Main
# -----------------------------------------------------------
. $y_path_scripts/_Y_Webserver_Check.sh

case "$1" in
	vnc_build_form)
		vnc_build_form ;;

	vnc_set)
        if [ "$#" -ne 5 ]
        then
            echo "vnc_set: zu wenig Parameter ($*)"
        else
			vnc_set $2 $3 $4 $5
		fi ;;
	news_build_form)
		news_build_form ;;

	news_set)
		;;

	nhttpd_build_form)
		nhttpd_build_form ;;

	nhttpd_set)
        if [ "$#" -ne 5 ]
        then
            echo "nhttpd_set: zu wenig Parameter ($*)"
        else
        	shift 1
			nhttpd_set $*
		fi ;;

	skin_build_form)
		skin_build_form ;;

	skin_set)
		skin_set $2 ;;

	*)
		echo "Parameter falsch: $*" ;;
esac

