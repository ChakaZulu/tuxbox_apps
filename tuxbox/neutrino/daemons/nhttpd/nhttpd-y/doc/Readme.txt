=================================================================
	Y-Web ins CVS / Dokumentation V0.2
=================================================================

=================================================================
	Orte im CVS
=================================================================
/apps/tuxbox/neutrino/daemons/nhttpd/data-y //Neuer Ornder
	dorthin alles was jetzt unter var/httpd ist & Anpassungen

/apps/tuxbox/neutrino/daemons/nhttpd/y-web //Neuer Ornder
	dorthin:
	- Y-Web.conf
/apps/tuxbox/neutrino/daemons/nhttpd/doc //Neuer Ornder
	- Y-Web-API.txt
	- Y-Web-doc.txt
	- Y-Web-Changelog.txt
	- Y-Web_CVS.txt

=================================================================
	Image bauen
=================================================================
Voraussetzungen:
- busybox braucht sed
=================================================================
	Dateien (Was kommt wohin)
=================================================================
-----------------------------------------------------------------
	Geänderte Dateien (gab es schon vor Y-Web)
-----------------------------------------------------------------
WebServer ("gepatchter")
	/bin/nhttpd (ist schon im CVS, kommt noch ein Update)

-----------------------------------------------------------------
	Neue Dateien
-----------------------------------------------------------------
*** WebSeiten (Alternativ httpd komplett ersetzen)

	Image: 	/share/tuxbox/neutrino/httpd-y/*	
	image: 	/share/tuxbox/neutrino/httpd-y/images/*
    bisher:	/var/httpd/*
    
*** Y-Skripts aus 
	Image: 	/share/tuxbox/neutrino/httpd-y/scripts/*
	bisher:	/var/tuxbox/plugins/*
		alle Y* ausführbar
		alle _Y* lesbar
	Alles in /<PulbicDocRoot>/* hat vorrang

*** Konfigurationsdateien
	/var/tuxbox/config/Y-Web.conf
	bleibt unverändert

-----------------------------------------------------------------
	Konfiguration
-----------------------------------------------------------------
WebServer-Konfiguration (Verweis auf neues Web)
	/var/tuxbox/config/nhttpd.conf
	Zeile: 		PrivatDocRoot=/share/tuxbox/neutrino/httpd
	Ändern in:	PrivatDocRoot=/share/tuxbox/neutrino/httpd-y

*** _Y_Globals.sh anpassen
	CVS:
	y_path_plugins="/<PrivatDocRoot>/scripts" (<..> ersetzen)
	y_path_httpd="/<PrivatDocRoot>"  (<..> ersetzen)
**** Y_*.sh
	Includes ind den Skripten *.sh müssen angepasst sein
	(Ist alles standardmäßig auf httpd-y)

	ggf. müssen weitere Pfade angepasst werden





