/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.
	

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

#ifndef EVENTLIST_HPP
#define EVENTLIST_HPP
//
// $Id: eventlist.h,v 1.11 2001/11/26 02:34:04 McClean Exp $
//
// $Log: eventlist.h,v $
// Revision 1.11  2001/11/26 02:34:04  McClean
// include (.../../stuff) changed - correct unix-formated files now
//
// Revision 1.10  2001/11/15 11:42:41  McClean
// gpl-headers added
//
// Revision 1.9  2001/10/14 14:30:47  rasc
// -- EventList Darstellung ueberarbeitet
// -- kleiner Aenderungen und kleinere Bugfixes
// -- locales erweitert..
//
// Revision 1.8  2001/10/04 19:28:44  fnbrd
// Eventlist benutzt ID bei zapit und laesst sich per rot wieder schliessen.
//
// Revision 1.7  2001/09/21 14:33:39  field
// Eventlist - ok/? vertauscht, epg-Breite flexibel
//
// Revision 1.6  2001/09/20 17:02:16  field
// event-liste zeigt jetzt auch epgs an...
//
// Revision 1.5  2001/09/20 11:55:58  fnbrd
// removed warning.
//
// Revision 1.4  2001/09/18 20:20:26  field
// Eventlist in den Infov. verschoben (gelber Knopf), Infov.-Anzeige auf Knoepfe
// vorbereitet
//
// Revision 1.3  2001/09/18 14:58:20  field
// Eventlist verbessert
//
// Revision 1.2  2001/09/18 11:34:42  fnbrd
// Some changes.
//
// Revision 1.1  2001/09/18 10:50:30  fnbrd
// Eventlist, quick'n dirty
//
//

#include "driver/framebuffer.h"
#include "driver/fontrenderer.h"
#include "driver/rcinput.h"
#include "daemonc/remotecontrol.h"
#include "helpers/infoviewer.h"
#include "helpers/settings.h"
#include "menue.h"
#include "color.h"

#include <string>
#include <vector>

using namespace std;

struct epg_event {
    unsigned long long id;
    time_t    startzeit;
    string    description;
};

class EventList
{
  private:
    struct event {
        epg_event   epg;
        string      datetime1_str;
        string      datetime2_str;
        string      duration_str;
    };

    void removeAllEvents(void);
    void readEvents(unsigned onidSid, const std::string& channelname); // I really don't like handling names
    unsigned int		selected;
    unsigned int                current_event;
    unsigned int		liststart;
    unsigned int		listmaxshow;
    unsigned int		numwidth;
    int			fheight; // Fonthoehe Channellist-Inhalt
    int			fheight1,fheight2;
    int			fwidth1,fwidth2;
    int			theight; // Fonthoehe Channellist-Titel

    int			key;
    string			name;
    vector<event*>	evtlist;

    int 			width;
    int 			height;
    int 			x;
    int 			y;

    void paintItem(unsigned pos);
    void paint();
    void paintHead();
    void hide();

  public:
    EventList();
    ~EventList();
    void exec(unsigned onidSid, const string& channelname);
};


#endif // EVENTLIST_HPP








