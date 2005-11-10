#!/bin/sh
# -----------------------------------------------------------
# Flashing Library (yjogol)
# $Date: 2005/11/10 19:47:00 $
# $Revision: 1.7 $
# -----------------------------------------------------------

. ./_Y_Globals.sh
. ./_Y_Library.sh

# -----------------------------------------------------------
# Image -helper - build form $1=HTML-Template
# -----------------------------------------------------------
image_sub_build_form()
{
	mtd_partition_list=`cat /proc/mtd|sed -n 2,7p|sed -e 's/^.* \"//g'|sed -e 's/\"$//g'`
	ymtd0=`echo "$mtd_partition_list"|sed -n 1p`
	ymtd1=`echo "$mtd_partition_list"|sed -n 2p`
	ymtd2=`echo "$mtd_partition_list"|sed -n 3p`
	ymtd3=`echo "$mtd_partition_list"|sed -n 4p`
	ymtd4=`echo "$mtd_partition_list"|sed -n 5p`
	ymtd5=`echo "$mtd_partition_list"|sed -n 6p`

	buildPage=`sed -e 's/Y_mtd0/'"$ymtd0"'/g' $1 | sed -e 's/Y_mtd1/'"$ymtd1"'/g' | sed -e 's/Y_mtd2/'"$ymtd2"'/g' | sed -e 's/Y_mtd3/'"$ymtd3"'/g' | sed -e 's/Y_mtd4/'"$ymtd4"'/g' | sed -e 's/Y_mtd5/'"$ymtd5"'/g'`
	echo "$buildPage"
}
# -----------------------------------------------------------
# Image Backup - build form
# -----------------------------------------------------------
image_backup_build_form()
{
	rm /tmp/*.img
	image_sub_build_form "$y_path_httpd/Y_Tools_Flash_Menue.htm"
}
# -----------------------------------------------------------
# Image Backup - build form
# -----------------------------------------------------------
image_flash_build_form()
{
	if [ -s "$y_upload_file" ]
	then
		image_sub_build_form "$y_path_httpd/Y_Tools_Image_Flash_Menue.htm"
	else
		msg="Upload-Problem.<br>Bitte nochmal hochladen."
		y_format_message_html
	fi
}

# -----------------------------------
# Flash-Backup ($1=mtd Nummer)
# -----------------------------------
backup_mtd()
{
	rm /tmp/*.img
	cat /dev/mtd/$1 > /tmp/flash_mtd$1.img
}

# -----------------------------------
# Sende Download-Page ($1=mtd Nummer)
# -----------------------------------
response_download_page()
{
	cmd="sed -e s/Y_URL/\/tmp\/flash_mtd$1.img/g $y_path_httpd/Y_Flash-backup_tmpl.htm"
	buildHTML=`$cmd`
	echo $buildHTML
}

# -----------------------------------------------------------
# Flash ($1=mtd Nummer) Upload-File
# -----------------------------------------------------------
flash_mtd()
{
	rm /tmp/*.img
	msg_nmsg "Image%20wird%20geflasht!"
	eraseall /dev/mtd/$1 >/dev/null
	if [ -s "$y_upload_file" ]
	then
		cat "$y_upload_file" > /dev/mtd/$1
		msg_nmsg "Reboot"
		msg="geflasht ... reboot ..."
		y_format_message_html
		yreboot
	else
		msg="Upload-Problem.<br>Bitte nochmal hochladen."
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
# 1-fstype, 2-ip, 3-dir, 4-localdir, 5-mac, 6-opt1, 7-opt2, 8-usr, 9-psw
do_mount()
{
echo "$*"
}

a()
{
	fstype="nfs"
	if [ "$1" = "1" ]
	then
		if [ "$1" = "2" ]
		then
			fstype="ftpfs"
		else
			fstype="cifs"
		fi
	fi
	
	opt=""
	if [ "$6" != "" ]
	then
		opt="$6"
	fi
	
	if [ "$7" != "" ]
	then
		if [ "$opt" != "" ]
		then
			opt="$opt,$7"
		else
			opt="$7"
		fi
	fi

#	if [ "$8" != "" ]
#	then
#		if [ "$opt" != "" ]
#		then
#			opt="$opt,user=$8"
#		else
#			opt="user=$8"
#		fi
#	fi
#
#	if [ "$9" != "" ]
#	then
#		if [ "$opt" != "" ]
#		then
#			opt="$opt,pass=$9"
#		else
#			opt="user=$9"
#		fi
#	fi
		
	echo "mount -t nfs $2:$3 $4 -o $opt $ip:$3 $4"
#	mount -t $fstype -o $opt $ip:$3 $4
}

# -----------------------------------------------------------
# 1: directory 2: append [true|false] 3+: cmd
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


# -----------------------------------
# Main
# -----------------------------------

case "$1" in
	image_backup_build_form)
		image_backup_build_form ;;

	image_flash_build_form)
		image_flash_build_form ;;

	image_backup)
		backup_mtd $2
		response_download_page $2 ;;

	image_flash)
		flash_mtd $2 ;;

	image_list)
		msg=`cat /proc/mtd`
		y_format_message_html ;;

	test_upload)
		if [ -s "$y_upload_file" ]
		then
			a=`ls -l /tmp`
			msg="Upload erfolgreich<br><br>$a"
		else
			msg="Upload-Problem.<br>Bitte nochmal hochladen."
		fi
		y_format_message_html ;;

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
		
	cmd)
		shift 1
		do_cmd $* ;;
		
	installer)
		shift 1
		do_installer $* ;;
		
	*)
		echo "Parameter falsch: $*" ;;
esac



