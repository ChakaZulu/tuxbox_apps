#!/bin/sh
# -----------------------------------------------------------
# Y Library (yjogol)
# $Date: 2005/09/23 16:26:02 $
# $Revision: 1.2 $
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
	echo "$msg" | sed 's/$/<br>/g' >$y_tmp
	buildHTML=`sed "/Y_msg/r $y_tmp" $y_path_httpd/Y_Tools_Format_tmpl.htm`
	echo "$buildHTML"
	echo "" >$y_tmp
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
	cmd="sed -e s/^.*=//1"
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
		cfg=`echo "$cfg\n$1=$2"`
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
    wget -O - -q "$y_url_control/control/message?nmsg=$1"
}

# -----------------------------------------------------------
# Message - popup (closes autom.)
# -----------------------------------------------------------
msg_popup()
{
    wget -O - -q "$y_url_control/control/message?popup=$1"
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
		echo "live_resolution=1"	>>$y_config_Y_Web
	fi
}
