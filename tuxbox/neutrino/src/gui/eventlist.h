#ifndef EVENTLIST_HPP
#define EVENTLIST_HPP
//
// $Id: eventlist.h,v 1.8 2001/10/04 19:28:44 fnbrd Exp $
//
// $Log: eventlist.h,v $
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

#include "../driver/framebuffer.h"
#include "../driver/fontrenderer.h"
#include "../driver/rcinput.h"
#include "../daemonc/remotecontrol.h"
#include "../helpers/infoviewer.h"
#include "../helpers/settings.h"
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
        string      datetimeduration;
    };

    void removeAllEvents(void);
    void readEvents(unsigned onidSid, const std::string& channelname); // I really don't like handling names
    unsigned int		selected;
    unsigned int                current_event;
    unsigned int		liststart;
    unsigned int		listmaxshow;
    unsigned int		numwidth;
    int			fheight; // Fonthoehe Channellist-Inhalt
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








