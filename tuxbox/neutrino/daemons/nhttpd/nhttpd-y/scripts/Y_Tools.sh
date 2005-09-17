#!/bin/sh
# -----------------------------------------------------------
# Flashing Library (yjogol)
# $Date: 2005/09/17 10:05:18 $
# $Revision: 1.2 $
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

# -----------------------------------
# Main
# -----------------------------------
#. ./_Y_Webserver_Check.sh

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
	*)
		echo "Parameter falsch: $*" ;;
esac



