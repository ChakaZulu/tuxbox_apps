Hier ist die erste Alpha-Version von Plugin mit angepassten Daemon.
Diese Version dient dem Vorschau auf dem Projektzustand und Entwicklungsrichtung.


Folgende Fuktionien sind noch nicht implementiert:
 - Screensaver ;-)
 - Zahleneditor für die numerische Werte.
 - SAVE-Anzeige, wenn Config-Datei gespeichert wird.

Plugin-Bedienung:
 Das Plugin verändert die Farbpalette und deswegen wird die Uhranzeige ausgeblendet.
 Standardsprache ist Deutsch. Englische Anzeige erreicht man durch drücken von "?"
 Die Flip-Flop Werte (Uhranzeige, Uhrtyp, SC-Datum) werden durch "OK" Taste umgeschaltet,
 die "OK"-Taste soll zukünftig bei numerischen Werten Zahleneditor starten.
 Numerische Werte können zur Zeit mit "+"/"-"-Tasten inkremetiert/dekrementiert werden;
 diese Tasten toggeln auch die FF-Werte.
 Die "d-box"-Taste startet das Sichern der Config-Datei mit den aktuellen Werten.
 Das Plugin hat keinen Timeout zur Beendigung nach längerer Inaktivität.
 Verlassen (kein Einfluss auf das Speichern der Config-Datei) wird mit "Home"-Taste bewirkt.

Offene Fragen:
 Syslog/Console der Fehler- und Statusmeldugen beim Plugin (kein Parameter für Syslog)

Nicht getestet:
 Enigma-Umgebung, alle DM und syslog-Funktionalität der Daemons.
