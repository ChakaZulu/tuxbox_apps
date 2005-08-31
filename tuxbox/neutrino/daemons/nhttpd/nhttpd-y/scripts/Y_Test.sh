#!/bin/sh
# -----------------------------------------------------------
# Test - patched WebServer (yjogol)
# $Date: 2005/08/31 18:23:49 $
# $Revision: 1.1 $
# -----------------------------------------------------------

# -----------------------------------
# Textausgabe, wenn der gepatchte Server
# laeuft, dann wird MIME text/html
# ausgegeben. Dann Zeilenumbrueche
# selbst machen
# -----------------------------------
do_echo()
{
	echo "$1"
	if [ "$web_neu" -eq 1 ]
	then
		echo "<br>"
	fi
}

# -----------------------------------
# Main
# web_neu=0 alter nhttpd =1 neuer
# web_neu=2 neuer und DOS Ausgabe
# -----------------------------------
web_neu=1
if [ "$#" -eq 0 ]
then
	web_neu=0
fi
if [ "$1" = "dos" ]
then
	web_neu=2
fi

do_echo "WebServer Test V1.0 (yjogol)"
do_echo "============================"
do_echo "Beispiel-Aufruf fuer Test: http://dbox/control/exec?Y_Test&111&222&333"
do_echo "Anzahl uebergebene Argumente: $#\n\r"
do_echo "Alle Argumente..............: $*"
do_echo ""

if [ "$web_neu" -eq 0 ]
then
	do_echo "Der gepatchte WebServer laeuft nicht!!!"
	do_echo "Beim Boot muss /var/bin/nhttpd gestartet werden."
	do_echo "Bei JtG geht das mit /var/tuxbox/start.neutrino"
	do_echo "Bei aktuellen CVS Images ist /var/bin als erstes im Suchpfad. Sollte eigentlich gehen"
	do_echo "Suchpfad PATH = $PATH"
else
	do_echo "Der gepatchte WebServer laeuft. GO"
fi
