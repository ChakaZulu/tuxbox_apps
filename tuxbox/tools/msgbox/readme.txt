####################################################################################
####                        MsgBox Version 1.68  
####                 Messagebox zum Info-Anzeige aus Scripten
####                                
####              Das New-Tuxwetter-Team: SnowHead und Worschter
####################################################################################

Was ist die Messagebox?
----------------------------------
Die MessageBox dient beim Ausf�hren von Scripten zur Anzeige von Informationen auf dem
Bildschirm und zur Abfrage von Entscheidungen des Nutzers �ber bis zu drei frei beschrift-
bare Tasten. Die Nummer der gedr�ckten Taste (1..16) oder 0 bei Timeout ohne gedr�ckte
Taste wird als Returnwert zur�ckgegeben und kann vom Script �ber die Variable "$?" ausge-
wertet werden.


Installation
----------------------------------
Es wird nur die Datei "msgbox" ben�tigt. Abh�ngig vom Image-Typ ist diese entweder in
/bin/ (bei JFFS-Only) oder /var/bin/ (bei CRAMFS und SQUASHFS) zu kopieren und mit den
Rechten 755 zu versehen. Nun kann sie aus eigenen Scripten heraus verwendet werden. Eine
spezielle Busybox ist f�r die Verwendung von "msgbox" nicht erforderlich.


Anwendung
----------------------------------
Der Aufruf der MessageBox erfolgt �ber die Ausf�hrung von "msgbox" mit entsprechenden Pa-
rametern. Im Folgenden werden nun die m�glichen Parameter beschrieben.
Das wichtigste ist nat�rlich der anzuzeigende Text. Dieser kann entweder �ber die Kommando-
zeile oder aus einer Datei �bergeben werden. Die Art der Textanzeige (Message oder Popup)
wird �ber die Schl�sselw�rter "msg=" f�r Message und "popup=" f�r Popup festgelegt.
Um nun einen Text von der Kommandozeile als Popup anzuzeigen, erfolgt der Aufruf in dieser
Form:

  msgbox popup="Auszugebender Text"
  
f�r eine Message entsprechend:

  msgbox msg="Auszugebender Text"
  
Der Text mu� dabei in Hochkommas stehen. Soll der anzuzeigende Text aus einer Datei ausgele-
sen werden, mu� der Aufruf f�r ein Popup

  msgbox popup=Dateiname
  
und f�r eine Message

  msgbox msg=Dateiname
  
lauten. Die Erkennung, da� es sich um eine Datei handelt, erfolgt am Zeichen "/" am Anfang des
Dateinamens. Dieser Dateiname kann in Hochkommas gesetzt werden, es ist aber nicht zwingend er-
forderlich.
Wird ein Text als Message angezeigt, ist das an dem OK-Button am unteren Fensterrand erkennbar.
Bei einem Popup wird dieser Button nicht angezeigt. Beide Anzeigearten lassen sich jedoch durch
Bet�tigen der Tasten "HOME" oder "OK" auf der Fernbedienung schlie�en.

Das Verhalten der Messagebox kann �ber einige zu�tzliche Parameter gesteuert werden:

  title="Fenstertitel"
  
Der in Hochkommas gestellte Text wird als Titel f�r die angezeigte Box verwendet. Wird die-
ser Parameter nicht angegeben, verwendet das msgbox den Titel "Information". Um Platz zu spa-
ren kann die Titelzeile auch komplett ausgeschaltet werden. Das geschieht mit dem Parameter
title="none" (Kleinschreibung beachten!).

  size=nn

Die Zahl "nn" wird als Fontgr��e f�r den anzuzeigenden Text verwendet. Je gr��er diese Zahl 
ist, um so weniger Text pa�t nat�rlich auf die volle Bildschirmbreite. Die Fenstergr��e der 
Box wird dabei automatisch entsprechend der Zeilenl�nge und der Zeilenanzahl gesetzt. Ohne
Parameter wird standardm��ig die Fontgr��e 36 verwendet.

  timeout=nn
  
Mit diesem Parameter kann festgelegt werden, nach welcher Zeit sich die Box von selbst wieder
schlie�en soll, wenn sie nicht durch einen Tastendruck auf der Fernbedienung geschlossen wird.
Ohne Parameter schlie�t sich die Box bei einer Message nach 5 Minuten und bei einem Popup nach
der Timeoutzeit, welche in den Neutrino-Einstellungen f�r die Info-Zeile festgelegt wurde. Diese
Zeit wird durch einen beliebigen tastendruck (au�er OK und HOME) neu gestartet. Soll die Message-
box ohne Timeout unbegrenzt offenbleiben, ist als Wert f�r den Timeout "timeout=-1" anzugeben.

In der Funktion als MessageBox ("msg=...") k�nnen bis zu 16 Tasten angezeigt und die Auswahl
des Nutzers abgefragt werden. Anzahl und Beschriftung der Tasten werden �ber den Parameter

  select="Label 1[,Label 2[,...]]" 

festgelegt. Dabei k�nnen 1 bis 16 Tasten mit den entsprechenden Texten (z.B. "Label 1") erzeugt
werden. Die einzelnen Label-Bezeichner k�nnen von beliebig vielen Kommas angef�hrt und auch ge-
folgt werden. Ausgewertet werden nur die nichtleeren Bereiche zwischen zwei Kommas.
Die Breite der Tasten und damit auch des gesamten Fensters richtet sich nach der l�ngsten Tas-
tenbezeichnung und der Anzahl der Tasten. Ohne Angabe des "select="-Parameters wird eine Taste 
mit dem Label "OK" erzeugt.

Da bei der �bergabe aus Scripten f�r den Paramter "select=" bei leeren Variablen auch zwei Kom-
mas aufeinanderfolgen k�nnen, werden solche �bergaben normalerweise ignoriert. Also w�rden bei
einem Parameter "select=Eintrag1,,Eintrag3" zwei Buttons angezeigt werden. Im Normalfall w�rde
bei Auswahl von "Eintrag3" als R�ckgabewert "2" �bergeben werden. Soll jedoch die Zuordnung zu
den Variablen erhalten bleiben, kann mit dem Parameter

  absolute=1
  
festgelegt werden, da� als R�ckgabewert die absolute Position des Eintrages in der Select-Liste
zur�ckgegeben wird. Bei "Eintrag3" w�re das also "3". Der Defaultwert f�r "absolute" ist "0".

Um die sinnvollste Taste bereits beim Start selektieren zu k�nnen, kann mit dem Parameter

  default=n
  
die Nummer der Taste (1..16) �bergeben werden, welche unmittelbar nach Anzeige der Messagebox
selektiert sein soll und nur noch mit OK best�tigt werden braucht. Dabei ist zu beachten, da�
bei "absolute=1" auch der Defaultwert absolut abgegeben werden mu�.

Um anzugeben, wieviel Tasten in einer Zeile angezeigt werden sollen, wird der Parameter

  order=n
  
�bergeben. Sind zum Beispiel 12 Tasten vereinbart und order wird mit 4 angegeben, werden 3 Reihen
zu je vier Tasten erzeugt. Dabei ist jedoch das maximal ausf�llbare Bildschirmformat zu ber�ck-
sichtigen. Bei mehreren Zeilen kann zus�tzlich zu den Links-/Rechts-Tasten mit den Hoch-/Runter-
Tasten zwischen den Zeilen navigiert werden.

Um die gew�hlte Taste im Script leichter auswerten zu k�nnen, kann zus�tzlich zum oben beschrie-
benen R�ckgabewert auch der Text der gew�hlten Taste �ber die Konsole ausgegeben werden. Das wird
mit dem Parameter

  echo=n
  
geregelt. Ist n=1, wird statt der Versionsmeldung am Programmstart am Ende des Programms der Label
der gew�hlten Taste auf der Konsole ausgegeben. In diesem Fall ist die Auswertung des Ergebnisses
abweichend zu der oben beschriebenen Aufrufsyntax statt �ber "$?" in der Form

  auswahl=`msgbox .... echo=1`
  
m�glich. Der Label der gew�hlten Taste kann dann �ber $auswahl ausgewertet werden. Bei Timeout oder
Abbruch bleibt $auswahl leer.

�ber die Mute-Taste der Fernbedienung kann die MessageBox zeitweilig ausgeblendet werden. Einmal
Dr�cken der Mute-Taste blendet die Box aus, ein weiterer Druck auf Mute blendet sie wieder ein.

  hide=n
  
Was nach dem Ausblenden der MessageBox angezeigt wird, h�ngt vom Parameter "hide" ab. Bei 0 wird
der Druck auf die Mute-Taste ignoriert und die Box wird nicht ausgeblendet, bei 1 wird ein gel�sch-
ter Bildschirm angezeigt (nur das Fernsehbild ist zu sehen), bei 2 wird der Bildschirminhalt ange-
zeigt, welcher vor dem Start der Messagebox zu sehen war (Men�s usw.). Defaultwert ist "1".
Wurde als Textparameter eine Datei �bergeben, wird diese vor dem Einblenden neu eingelesen. Somit
werden w�hrend des Ausblendens in dieser Datei vorgenommene �nderungen nach dem Einblenden aktuell
angezeigt. W�hrend die Box ausgeblendet ist, werden alle Tastendr�cke au�er der Mute-Taste ignoriert.
Um aus dem Script heraus �berpr�fen zu k�nnen, ob die MessageBox ausgeblendet ist, wird von der Mes-
sageBox f�r die Zeit, in der sie ausgeblendet ist, die Flagdatei "/tmp/.msgbox_hidden" erzeugt.

Um das Verhalten bei bereits Angezeigten Men�s oder Meldungen zu steuern, dient der Parameter

  refresh=n
  
Mit n=0 werden vor dem Start der MessageBox angezeigte Men�s oder Infos gel�scht (nur die Messa-
geBox ist sichtbar) und beim Beenden der MessageBox der Bildschirm gel�scht.
Mit n=1 blendet sich die MessageBox �ber bereits angezeigte Infos ein, l�scht den Bildschirm beim
Beenden aber komplett.
Mit n=2 werden vor dem Start der MessageBox angezeigte Men�s oder Infos gel�scht (nur die Messa-
geBox ist sichtbar), die vorher abgezeigten Infos aber beim Beenden der Messagebox wiederhergestellt.
n=3 kombiniert 1 und 2, die MessageBox blendet sich �ber vorher angezeigte Infos ein und stellt beim
Beenden den Bildschirmzustand wieder her, welcher vor dem Start der MessageBox aktuell war.
Dieser Parameter kann entfallen, in diesem Fall wird standardm��ig mit refresh=3 gearbeitet.

Normalerweise wird die MessageBox auf dem Bildschirm seit der Version 2.60 jede Sekunde aufgefrischt.
Sollte das bie bestimmten Anwendungen st�ren, kann dieser zyklische Refresh ausgeschaltet werden.
Dazu wird der Parameter

  cyclic=0
  
�bergeben. Damit wird die Box nur noch ein Mal beim Aufruf und der �nderung des Inhaltes einer ange-
zeigten Datei dargestellt. Der Defaultwert f�r cyclic ist 1.

Formatierung des �bergebenen Textes
-----------------------------------
Um die Darstellung des Textes ansprechender gestalten zu k�nnen, werden bestimmte Formatsteuer-
zeichen im �bergebenen Text unterst�tzt. Allen Steuerzeichen gemeinsam ist der Beginn mit dem
Zeichen "~". Dieses kommt im normalen Text nicht vor und leitet daher immer einen Formatierungs-
befehl ein. Folgende Formatierungen k�nnen ausgef�hrt werden:

  ~l    Diese Zeile auf Links-Anschlag schieben
  ~c    Diese Zeile zentrieren
  ~r    Diese Zeile auf Rechtsanschlag schieben
  ~t    Tabulator
  ~Tnnn nachfolgenden Text auf absoluter Position nnn beginnen (nur im Messagetext zul�ssig)
  ~s    Separator (eine waagerechte Linie �ber die gesamte Textbreite auf Zeilenmitte
  ~R    nachfolgenden Text rot darstellen, gilt bis zum Zeilenende oder einem neuen Farbbefehl
  ~G    nachfolgenden Text gr�n darstellen, gilt bis zum Zeilenende oder einem neuen Farbbefehl
  ~B    nachfolgenden Text blau darstellen, gilt bis zum Zeilenende oder einem neuen Farbbefehl
  ~Y    nachfolgenden Text gelb darstellen, gilt bis zum Zeilenende oder einem neuen Farbbefehl
  ~F    nachfolgenden Text blinkend darstellen, gilt bis zum Zeilenende oder einem neuen Farbbefehl
  ~S    nachfolgenden Text in Standardfarbe darstellen


Sonderzeichen �ber die Kommandozeile
------------------------------------
Da Linux keine �bergabe von Sonder- und Steuerzeichen �ber die Kommandozeile unterst�tzt, k�nnen
die wichtigsten Sonderzeichen �ber die Nutzung des Formatsteuerzeichens sowohl aus Scripten als
auch von der Kommandozeile dargestellt werden. Aktuell werden folgende Sonder- und Steuerzeichen
unterst�tzt:

  ~n    neue Zeile (nur von der Kommandozeile)
  ~a    �
  ~o    �
  ~u    �
  ~A    �
  ~O    �
  ~U    �
  ~z    �
  ~d    � (degree)
  
  
Die Wirkung der Formatierungen kann man sich anhand des beiliegenden Beispieltextes anschauen.
Die Datei "msgbox.txt" nach /tmp/ kopieren und anschlie�end �ber Telnet eingeben:

  msgbox title="Beispieltext anzeigen" msg=/tmp/msgbox.txt
  
Der Parameter "title" kann hier nat�rlich auch weggelassen werden. ;-) Aber denkt bitte daran,
da� bei einem Aufruf �ber Telnet Neutrino auch weiterhin auf die Fernbedienung reagiert. Das 
ist kein Fehler der Messagbox. Bei einem Aufruf aus einem Script heraus, welches �ber die Plug-
in-Verwaltung gestartet wurde, tritt dieser Effekt dann nicht mehr auf.

Wird "msgbox" mit falschen oder v�llig ohne Parameter aufgerufen, wird im Log eine Liste der
unterst�tzten Parameter ausgegeben.


Also, viel Spa� und viel Erfolg

Das New-Tuxwetter-Team
SnowHead und Worschter