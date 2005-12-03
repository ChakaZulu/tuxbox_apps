#!/bin/sh
# -----------------------------------------------------------
# Y Library (yjogol)
# $Date: 2005/12/03 14:39:26 $
# $Revision: 1.6 $
# -----------------------------------------------------------

# ===========================================================
# Streaming URL
# ===========================================================

buildLocalIP()
{
    localIP=`ifconfig eth0|sed -n '/inet addr/p'|sed -e 's/^.*inet addr://g' -e 's/ .*//g'`
	echo "$localIP"
}

# -----------------------------------------------------------
# Streaming URL für sed
# -----------------------------------------------------------
buildStreamingURL()
{
	localIP=`buildLocalIP`
	pids=`wget -O - -q "$y_url_control/yweb?video_stream_pids=0"`
    echo "http:\/\/$localIP:31339\/0,$pids"
}

# -----------------------------------------------------------
# Streaming URL
# -----------------------------------------------------------
buildStreamingRawURL()
{
	localIP=`buildLocalIP`
	pids=`wget -O - -q "$y_url_control/yweb?video_stream_pids=0"`
    echo "http://$localIP:31339/0,$pids"
}

# -----------------------------------------------------------
# Audio: Streaming URL für sed
# -----------------------------------------------------------
buildStreamingAudioURL()
{
	localIP=`buildLocalIP`
	Y_APid=`wget -O - -q "$y_url_control/yweb?radio_stream_pid"`
    echo "http:\/\/$localIP:31338\/$Y_APid"
}

# -----------------------------------------------------------
# Streaming URL
# -----------------------------------------------------------
buildStreamingAudioRawURL()
{
	localIP=`buildLocalIP`
	Y_APid=`wget -O - -q "$y_url_control/yweb?radio_stream_pid"`
    echo "http://$localIP:31338/$Y_APid"
}

# -----------------------------------
# UNIX ($msg) Text als HTML ausgeben
# noch sehr unschoen
# -----------------------------------

y_format_message_html()
{
	tmp="<html><head><meta http-equiv='Content-Type' content='text/html; charset=windows-1252'>"
	tmp="$tmp <link rel='stylesheet' type='text/css' href='/Y_Main.css'></head>"
	tmp="$tmp <body><div class='y_work_box'><pre>\n$msg\n</pre></div></body></html>"
	echo "$tmp"
}
y_format_message_html2()
{
	tmp="<html><head><meta http-equiv='Content-Type' content='text/html; charset=windows-1252'>"
	tmp="$tmp <link rel='stylesheet' type='text/css' href='/Y_Main.css'></head>"
	tmp="$tmp <body>$msg</body></html>"
	echo "$tmp"
}
# ===========================================================
# config-Dateien - lesen / schreiben
# (Zeilenformat: VarName=VarValue)
# ===========================================================
cfg=""
# -----------------------------------------------------------
# config-Datei lesen/cachen (Inhalt in $cfg)
# $1=config-Filename
# -----------------------------------------------------------
config_open()
{
	cfg=""
	cfg=`cat $1`
}
# -----------------------------------------------------------
# config-Datei schreiben (Inhalt in $cfg)
# $1=config-Filename
# -----------------------------------------------------------
config_write()
{
	echo "$cfg" >$1
}
# -----------------------------------------------------------
# Variablenwert zurueckgeben (vorher open)
# $1=VarName
# -----------------------------------------------------------
config_get_value()
{
	cmd="sed -n /^$1=/p"
	tmp=`echo "$cfg" | $cmd`
	cmd="sed -e s/^$1=//1"
	tmp=`echo "$tmp" | $cmd`
	echo $tmp
}
# -----------------------------------------------------------
# Variablenwert zurueckgeben (ohne open)
# $1=config-Filename
# $2=VarName
# -----------------------------------------------------------
config_get_value_direct()
{
	config_open $1
	config_get_value $2
}
# -----------------------------------------------------------
# Variablenwert setzen (vorher open)
# $1=VarName)
# $2=VarValue
# -----------------------------------------------------------
config_set_value()
{
	tmp=`echo "$cfg" | sed -n "/^$1=.*/p"`
	if [ "$tmp" = "" ]
	then
		cfg=`echo -e "$cfg\n$1=$2"`
	else
		cmd="sed -e s/^$1=.*/$1=$2/g"
		cfg=`echo "$cfg" | $cmd`
	fi
}
# -----------------------------------------------------------
# Variablenwert zurueckgeben (ohne open)
# $1=config-Filename
# $2=VarName)
# $3=VarValue
# -----------------------------------------------------------
config_set_value_direct()
{
	config_open $1
	config_set_value $2 $3
	config_write $1
}
# -----------------------------------------------------------
# Reboot
# -----------------------------------------------------------
yreboot()
{

    killall -9 nhttpd
    killall -9 controld
    killall zapit
    sleep 1
    killall -9 camd2
    killall -9 timerd
    killall -9 sectionsd
    sleep 1
    reboot
}

# -----------------------------------------------------------
# Message - nmsg (waiting - press ok on rc)
# -----------------------------------------------------------
msg_nmsg()
{
    wget -O - -q "$y_url_control/message?nmsg=$1"
}

# -----------------------------------------------------------
# Message - popup (closes autom.)
# -----------------------------------------------------------
msg_popup()
{
    wget -O - -q "$y_url_control/message?popup=$1"
}

# -----------------------------------------------------------
# create Y_Web.conf if does not exists
# -----------------------------------------------------------
check_Y_Web_conf()
{
	if ! [ -e $y_config_Y_Web ]
	then
		echo "skin=Tuxbox"  >$y_config_Y_Web
		echo "slavebox="	>>$y_config_Y_Web
		echo "live_resolution_w=384"	>>$y_config_Y_Web
		echo "live_resolution_wh=288"	>>$y_config_Y_Web
	fi
}
