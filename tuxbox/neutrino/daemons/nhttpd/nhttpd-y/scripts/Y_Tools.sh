#!/bin/sh
# -----------------------------------------------------------
# Flashing Library (yjogol)
# $Date: 2005/12/21 18:10:07 $
# $Revision: 1.9 $
# -----------------------------------------------------------

. ./_Y_Globals.sh
. ./_Y_Library.sh

# -----------------------------------------------------------
# Image Backup - build form
# -----------------------------------------------------------
image_upload()
{
	if [ -s "$y_upload_file" ]
	then
		msg="<b>Image upload ok</b><br>"
		msg="$msg <script language='JavaScript' type='text/javascript'>window.setTimeout('parent.do_image_upload_ready()',1000)</script>"
	else
		msg="Upload-Problem.<br>Bitte nochmal hochladen."
		msg="$msg <script language='JavaScript' type='text/javascript'>window.setTimeout('parent.do_image_upload_ready_error()',1000)</script>"
	fi
	y_format_message_html
}
# -----------------------------------
# Flash-Backup ($1=mtd Nummer)
# -----------------------------------
image_backup_mtd()
{
	rm /tmp/*.img
	cat /dev/mtd/$1 > /tmp/flash_mtd$1.img
}
# -----------------------------------
# Sende Download-Page ($1=mtd Nummer)
# -----------------------------------
image_backup_download_page()
{
	msg="<div class='y_work_box'><b>Das Image wurde erstellt.</b><p>"
	msg="$msg <a type='application/octet-stream' href='/tmp/flash_mtd$1.img'><u>Download</u></a><br><br>"
	msg="$msg <a href='/control/exec?Y_Tools&image_delete'><u>Download fertig. Image in /tmp loeschen.</u></a></p></div>"
	msg="$msg  <script language='JavaScript' type='text/javascript'>parent.do_image_download_ready()</script>"
	y_format_message_html
}
# -----------------------------------
image_delete_download_page()
{
	rm -r /tmp/*.img
	msg="<div class='y_work_box'><b>Die Image-Datei in tmp wurde geloescht.</b></div>"
	y_format_message_html
}
# -----------------------------------------------------------
# Flash ($1=mtd Nummer) Upload-File
# -----------------------------------------------------------
flash_mtd()
{
simulate="false"

	rm /tmp/*.img
	if [ -s "$y_upload_file" ]
	then
		echo "" >/tmp/e.txt
		msg_nmsg "Image%20wird%20geflasht!"
		if [ "$simulate" != "true" ]
		then
			fcp -v "$y_upload_file" /dev/mtd/$1 >/tmp/e.txt
		else #simulation/DEMO
			i="0"
			while test $i -le 10
			do
				p=`expr $i \* 10`
				b=`expr $i \* 63`
				b=`expr $b / 10`
				echo "\rDEMO: Erasing blocks: $b/63 ($p%)" >>/tmp/e.txt
				i=`expr $i + 1`	
				sleep 1
			done
			i="0"
			while test $i -le 20
			do
				p=`expr $i \* 5`
				b=`expr $i \* 8064`
				b=`expr $b / 20`
				echo "\rDEMO: Writing data: $b k/8064k ($p%)" >>/tmp/e.txt
				i=`expr $i + 1`	
				sleep 2
			done
			i="0"
			while test $i -le 5
			do
				p=`expr $i \* 20`
				b=`expr $i \* 8064`
				b=`expr $b / 5`
				echo "\rDEMO: Verifying data: $b k/8064k ($p%)" >>/tmp/e.txt
				i=`expr $i + 1`	
				sleep 1
			done
		fi
		msg_nmsg "flashen%20fertig.%20Reboot..."
		msg="geflasht ... bitte jetzt box neu starten ..."
		msg="$msg <script language='JavaScript' type='text/javascript'>window.setTimeout('parent.do_image_flash_ready()',1000)</script>"
		y_format_message_html
	else
		msg="Upload-Problem.<br>Bitte nochmal hochladen."
		msg="$msg <script language='JavaScript' type='text/javascript'>window.setTimeout('parent.do_image_flash_ready()',1000)</script>"
		y_format_message_html
	fi
}
# -----------------------------------------------------------
# copies Uploadfile to $1
# -----------------------------------------------------------
upload_copy()
{
	if [ -s "$y_upload_file" ]
	then
		cp "$y_upload_file" "$1"
	else
		msg="Upload-Problem.<br>Bitte nochmal hochladen."
	fi
}
# -----------------------------------------------------------
bootlogo_upload()
{
	msg="Boot-Logo neu gesetzt"
	upload_copy "$y_boot_logo"
	y_format_message_html
}
# -----------------------------------------------------------
bootlogo_lcd_upload()
{
	msg="Boot-Logo-LCD neu gesetzt"
	upload_copy "$y_boot_logo_lcd"
	y_format_message_html
}
# -----------------------------------------------------------
ucodes_upload()
{
	msg="$1 hochgeladen<br><a href='/Y_Settings_ucodes.htm'><u>naechste Datei</u></a>"
	upload_copy "$y_path_ucodes/$1"
	y_format_message_html
}
# -----------------------------------------------------------
zapit_upload()
{
	msg="$1 hochgeladen<br><a href='/Y_Settings_zapit.htm'><u>naechste Datei</u></a>"
	upload_copy "$y_path_zapit/$1"
	y_format_message_html
}
# -----------------------------------------------------------
# Mount from Neutrino-Settings $1=nr
# -----------------------------------------------------------
do_mount()
{
	config_open $y_config_neutrino
	fstype=`config_get_value "network_nfs_type_$1"`
	ip=`config_get_value "network_nfs_ip_$1"`
	local_dir=`config_get_value "network_nfs_local_dir_$1"`
	dir=`config_get_value "network_nfs_dir_$1"`
	options1=`config_get_value "network_nfs_mount_options1_$1"`
	options2=`config_get_value "network_nfs_mount_options2_$1"`
	username=`config_get_value "network_nfs_username_$1"`
	password=`config_get_value "network_nfs_password_$1"`

	# check options
	if [ "$options1" = "" ]
	then
		options1=options2
		options2=""
	fi

	# default options
	if [ "$options1" = "" ]
	then
		if [ "$options2" = "" ]
		then
			if [ "$fstype" = "0" ]
			then
				options1="ro,soft,udp"
				options2="nolock,rsize=8192,wsize=8192"
			fi
			if [ "$fstype" = "1" ]
			then
				options1="ro"
			fi
		fi
	fi
	# build mount command
	case "$fstype" in
		0) #nfs
			cmd="mount -t nfs $ip:$dir $local_dir -o $options1"
			;;
		1)
			cmd="mount -t cifs //$ip/$dir $local_dir -o username=$username,password=$password,unc=//$ip/$dir,$options1";
			;;
		2)
			cmd="lufsd none $local_dir -o fs=ftpfs,username=$username,password=$password,host=$ip,root=/$dir,$options1";
			;;
		default)
			echo "mount type not supported"
	esac
	
	if [ "$options2" != "" ]
	then
		cmd="$cmd,$options2"
	fi

	res=`$cmd`
	echo "$res"
	echo "view mounts"
	m=`mount`
	msg="mount cmd:$cmd<br><br>res=$res<br>view Mounts;<br>$m"
	y_format_message_html
}
# -----------------------------------------------------------
# unmount $1=local_dir
# -----------------------------------------------------------
do_unmount()
{
	umount $1
}
# -----------------------------------------------------------
# Execute shell command
# 1: directory 2: append [true|false] 3+: cmd
# -----------------------------------------------------------
do_cmd()
{
	cd $1
	pw1="$1"
	app="$2"
	shift 2
	
	if [ "$1" = "cd" ]
	then
		cd $2
	else
		tmp=`$*` #Execute command
	fi
	pw=`pwd`
	echo '<html><body><form name="o"><textarea name="ot">'
	echo "$pw1>$*"
	echo "$tmp"
	echo '</textarea></form>'
	echo '<script language="JavaScript" type="text/javascript">'
	if [ "$app" = "true" ]
	then
		echo 'parent.document.co.cmds.value += document.o.ot.value;'
	else
		echo 'parent.document.co.cmds.value = document.o.ot.value;'
	fi
	echo "parent.set_pwd('$pw');"
	echo 'parent.setCaretToEnd(parent.document.co.cmds);'
	echo 'parent.document.f.cmd.focus();'
	echo '</script></body></html>'
}
# -----------------------------------------------------------
# yInstaller
# un-tar uploaded file to /tmp. Execute included install.sh
# -----------------------------------------------------------
do_installer()
{
	# clean up
	if [ -s "$y_out_html" ]
	then
		rm $y_out_html
	fi
	
	if [ -s "$y_upload_file" ]
	then
		# unpack /tmp/upload.tmp
		cd $y_path_tmp
		tar -xf "$y_upload_file"
		rm $y_upload_file
		if [ -s "$y_install" ] #look for install.sh
		then		
			chmod 755 $y_install
			o=`$y_install` # execute
			rm -f $y_install # clean up
			if [ -s "$y_out_html" ] #html - output?
			then	
				echo '<html><head><link rel="stylesheet" type="text/css" href="/Y_Main.css">'
				echo "<meta http-equiv='refresh' content='0; $y_out_html'></head>"
				echo "<body><a href='$y_out_html'>Falls automatische Weiterleitung nicht geht.</a>"
				echo '</body></html>'
#				cat $y_out_html
			else
				echo '<html><head><link rel="stylesheet" type="text/css" href="/Y_Main.css"></head>'
				echo '<body>'
				echo "$o"
				echo '</body></html>'
			fi
		else
			msg="$y_install nicht gefunden"
			y_format_message_html
		fi
	else
		msg="Upload-Problem.<br>Bitte nochmal hochladen."
		y_format_message_html
	fi
}
# -----------------------------------------------------------
# view /proc/$1 Informations
# -----------------------------------------------------------
proc()
{
	msg=`cat /proc/$1`
	msg="<b>proc: $1</b><br><br>$msg"
	y_format_message_html
}
# -----------------------------------------------------------
# wake up $1=MAC
# -----------------------------------------------------------
wol()
{
	msg=`etherwake $1`
	msg="<b>Wake on LAN $1</b><br><br>$msg"
	y_format_message_html
}
# -----------------------------------------------------------
# wake up $1=MAC
# -----------------------------------------------------------
dofbshot()
{
	rm -r /tmp/a.png
	fbshot /tmp/a.png >/dev/null
	msg="<img src='' name="fb" id="fb">"
	msg="$msg <script language='JavaScript' type='text/javascript'>document.fb.src='/tmp/a.png?hash=' + Math.random();window.setTimeout('parent.do_ready()',1000)</script>"
	y_format_message_html2
}
# -----------------------------------------------------------
# Main
# -----------------------------------------------------------
case "$1" in
	image_upload)
		image_upload ;;

	image_backup) # $2=mtd#
		image_backup_mtd $2
		image_backup_download_page $2 ;;

	image_flash)
		flash_mtd $2 ;;

	image_flash_free_tmp)
		rm -r /tmp/*.img
		;;
	image_delete)
		image_delete_download_page
		;;
	bootlogo_upload)
		bootlogo_upload	;;

	bootlogo_lcd_upload)
		bootlogo_lcd_upload	;;

	ucodes_upload)
		ucodes_upload $2 ;;

	zapit_upload)
		zapit_upload $2 ;;

	kernel-stack)
		msg=`dmesg`
		y_format_message_html ;;
	ps)
		msg=`ps -c`
		y_format_message_html ;;
	free)
		f=`free`
		p=`df -h`
		msg="RAM Speichernutzung\n-------------------\n$f\n\nPartitionen\n-------------------\n$p"
		y_format_message_html ;;

	yreboot)
		yreboot
		echo "Reboot..." ;;

	check_yWeb_conf)
		check_Y_Web_conf ;;
		
	rcsim)
		rcsim $2 >/dev/null ;;

	domount)
		shift 1
		do_mount $* ;;
		
	dounmount)
		shift 1
		do_unmount $* ;;
		
	cmd)
		shift 1
		do_cmd $* ;;
		
	installer)
		shift 1
		do_installer $* ;;
		
	proc)
		shift 1
		proc $* ;;
	
	wol)
		shift 1
		wol $* ;;
	
	dofbshot)
		dofbshot
		;;
	
	*)
		echo "Parameter falsch: $*" ;;
esac



