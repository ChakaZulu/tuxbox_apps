TuxCom:

Historie:
---------

25.07.2004 Version 1.4
 - Taskmanager eingebaut (über Info-Taste aufrufbar)
 - vor-/zurück-scrollen bei Kommandoausführung/Skriptausführung möglich
 - vor-/zurück-scrollen in Dateiansicht nicht mahr auf 100k-Dateien beschränkt
 - aktuell ausgewählte Datei merken bei Verlassen des Plugins
 - Tastaturunterstützung für DMM-Tastatur eingebaut
 - Verzögerung bei gedrückter Taste eingebaut
 - Bugfix: Workaround für Tastendruck-Fehler von Enigma
 - Bei Verweis-erstellen (Taste 0) wird automatisch der ausgewählte Dateiname vorgeschlagen

21.06.2004 Version 1.3
 - FTP-Client eingebaut
 - kleinere Fehler im Editor beseitigt
 - Texteingabe: Sprung zum nächsten Zeichen, wenn eine andere Ziffer gedrückt wird.
 - Texteingabe: letztes Zeichen wird entfernt wenn am Ende der Zeile volume- gedrückt wird.
 - Umschalten zwischen 4:3 und 16:9-Modus über Dream-Taste
 - Dateiansicht : Scrollen wie im Editor möglich (bei Dateien, die maximal 100k gross sind).

05.06.2004 Version 1.2a
 - BugFix: Fehlende Sonderzeichen bei Eingabe ergänzt.
 - Texteingabe im "SMS-Stil" eingebaut
 
29.05.2004 Version 1.2
 - Unterstützung zum Extrahieren aus "tar", "tar.Z", "tar.gz" und "tar.bz2" Archiven
   funktioniert leider im Original-Image 1.07.4 mit vielen Archiven nicht (zu alte BusyBox-Version :( )
 - Anzeige der aktuellen Zeilennummer im Editor
 - Positionierung anhand der TuxTxt-Koordinaten
 - grosse Schrift beim Editieren einer Zeile
 - Scrollen in Zeichen im Editiermodus an Enigma-Standard angepasst (hoch/runter vertauscht)
 - Versionsnummer über Info-Taste abrufbar
 - Sicherheitsabfrage, falls durch kopieren/verschieben bestehende Dateien überschrieben werden.

08.05.2004 Version 1.1a
 - BugFix: Keine angehängten Leerzeichen mehr beim Umbenennen von Dateien

02.05.2004 Version 1.1
 - einige Farbänderungen
 - Deutsche Texte eingebaut
 - Möglichkeit, Tasten gedrückt zu halten (hoch/runter, rechts/links, volume+/-, grüne Taste)
 - 3 Tranzparenzstufen 
 - Dateien markieren, sodass man mehrere Dateien auf einmal kopieren/verschieben oder löschen kann
 - Tranzparanzmodus wird jetzt durch die 'mute'- Taste gewechselt (analog zu TuxTxt) (grüne Taste wird zum Dateien markieren verwendet)

03.04.2004 Version 1.0 : 
   erste Veröffentlichung
   
     
  
Quellen:
--------
Ich habe Codeteile von T-Hydron's script-plugin (http://t-hydron.verkoyen.be/)
und LazyT's TuxTxt (aus dem CDK)  übernommen. 


Vorraussetzungen:
-----------------
 - Eine Dreambox 7000-S ( nicht auf anderen Modellen getested)
 - Firmware Version 1.07.x oder höher ( nicht auf älteren Versionen getested)

Installation:
-------------
Wie bei jedem Plugin, einfach tuxcom.so und tuxcom.cfg nach /var/tuxbox/plugins kopieren

Wenn die Font-Datei 'pakenham.ttf' nicht in /share/fonts/ liegt 
bitte diese in /var/tuxbox/config/enigma/fonts/ kopieren

Tasten:
---------------

links/rechts		linkes/rechtes Fenster wählen
hoch/runter 		nächsten/vorherigen Eintrag im aktuellen Fenster wählen
volume -/+		Eine Seite hoch/runter im aktuellen Fenster
ok			gewählte Datei ausführen / Verzeichnis wechseln im aktuellen Fenster / Archiv zum Lesen öffnen
1			Eigenschaften (Rechte) von gewählter Datei anzeigen/ändern
2			gewählte Datei umbenennen
3			gewählte Datei anzeigen
4			gewählte Datei bearbeiten
5			gewählte Datei von aktuellem Fenster ins andere Fenster kopieren
6			gewählte Datei von aktuellem Fenster ins andere Fenster verschieben
7			neues Verzeichnis in aktuellem Fenster erstellen
8			gewählte Datei löschen
9			neue Datei in aktuellem Fenster erstellen
0			symbolischen Verweis zur gewählten Datei im aktuellen Verzeichnis des anderen Fensters erstellen
rot			linux Kommando ausführen
grün			Datei markieren/Markierung aufheben
gelb			Sortierung der Einträge im aktuellen Fenster umkehren
blau			Ansicht aktualisieren
mute			Transparenzmodus wechseln
dream			Umschalten zwischen 4:3 und 16:9-Modus
info			Taskmanager / Versionsinformation

in Mitteilungsfenstern:

links/rechts		Auswahl ändern
ok			Auswahl bestätigen
rot/grün/gelb		Auswahl ändern

in Texteingabe:

links/rechts		Position wechseln
hoch/runter		Zeichen wechseln
ok			bestätigen
volume +		neues Zeichen einfügen
volume -		Zeichen entfernen
rot			Eingabe löschen
gelb			Wechseln zwischen Gross und Kleinbuchstaben
0..9			Zeichenauswahl im "SMS-Stil" ( wie in der Enigma Texteingabe)


in Eigenschaften:

hoch/runter		Auswahl ändern
ok			Recht gewähren/entziehen
rot			Änderungen bestätigen
grün			Änderungen verwerfen


in Editor:

links/rechts		Seite zurück/vor
hoch/runter		Zeile zurück/vor
ok			Zeile bearbeiten
volume +		Sprung zur 1. Zeile
volume -		Sprung zur letzten Zeile
rot			Zeile löschen
grün			Zeile einfügen

in Viewer:

ok, rechts		nächste Seite
links/rechts		Seite zurück/vor
hoch/runter		Zeile zurück/vor
volume +		Sprung zur 1. Zeile
volume -		Sprung zur letzten Zeile

in Taskmanager:

ok, rechts		nächste Seite
links/rechts		Seite zurück/vor
hoch/runter		Zeile zurück/vor
volume +		Sprung zur 1. Zeile
volume -		Sprung zur letzten Zeile
rot			Prozess beenden 

in allen Dialogen: 

lame			Dialog verlassen



Farben:
------------
Hintergrund: 
schwarz : aktuelles Verzeichnis hat nur Lesezugriff
blau    : aktuelles Verzeichnis hat Lese/Schreibzugriff

Dateiname:
weiss : Eintrag ist Verzeichnis
orange: Eintrag ist Verweis
gelb  : Eintrag ist ausführbar
grau  : Eintrag hat Schreibzugriff
grün  : Eintrag hat Lesezugriff


Nutzung des FTP-Client:
-----------------------
1.) Eine Datei mit der Endung .ftp erstellen. 
2.) Diese Datei editieren:
Folgende Einträge in dieser Datei sind möglich:
host=<ftp-Adresse>	(muss immer angegeben werden, z.B.: host=ftp.gnu.org)
user=<username> 	(optional)
pass=<password> 	(optional)
port=<ftpport>  	(optional, standardmässig 21)
dir=<Unterverzeichnis>	(optional, standardmässig /)
3.) Datei auswählen und OK drücken. 
Es wird eine FTP-Verbindung zur angegebenen Adresse aufgebaut.