/*
	LCD-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean', Georg Lukas
	Homepage: http://dbox.cyberphoria.org/



	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
 
	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __lcdd__
#define __lcdd__

#include <netinet/in.h>

#define SA struct sockaddr
#define SAI struct sockaddr_in

#define LCDD_PORT 1510

#define LCDD_VERSION 3

/* Aaaaaalso gut, damit ihr lcdd-Benutzer wisst, was ihr machen könnt, hier
 * eine Kurze Übersicht der Befehle.
 *
 * LC_CHANNEL   zeigt param3 als Kanalname an
 * LC_VOLUME    erwartet in param einen Wert 0..100 und zeigt das als Balken an
 * LC_MUTE      erwartet in param LC_MUTE_ON oder _OFF und zeigt das an
 * LC_SET_MODE  ändert den Anzeigemodus entsprechend dem lcdd_mode in param
 * LC_MENU_MSG  ändert Zeile param im Menü zu param3
 * LC_POWEROFF  zeigt shutdown-logo und beendet lcdd.
 */
enum lcdd_cmd {
	LC_CHANNEL = 1,
	LC_VOLUME = 2,
	LC_MUTE,
	LC_SET_MODE,
	LC_MENU_MSG,
	LC_POWEROFF,
};

/* param für LC_MUTE-Befehl */
#define LC_MUTE_OFF 0
#define LC_MUTE_ON 1

/* param für LC_SET_MODE-Befehl,
 *
 * LCDM_TV        Normaler Anzeigemodus von Kanal (gross), Lautstärke und Zeit
 * LCDM_MENU      Menü-Ansicht (param=Zeilennummer; p3=Inhalt)
 * LCDM_SAVER     Startet einen Screenhack der eigenen Wahl (irgendwann mal)
 * LCDM_POWEROFF  Setzt shutdown-Logo, beendet aber NICHT den lcdd.
 */
enum lcdd_mode {
	LCDM_TV,
	LCDM_MENU,
	LCDM_SAVER,
	LCDM_POWEROFF,
};

struct lcdd_msg {
  unsigned char version;
  unsigned char cmd;
  unsigned char param;
  unsigned short param2;
  char param3[30];
  // XXX +++ WARNING +++ XXX
  // changed interface
  //char param4[10][30];
}__attribute((packed));


#endif // __lcdd__
