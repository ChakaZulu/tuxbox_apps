#ifndef EVENTLIST_HPP
#define EVENTLIST_HPP
//
// $Id: eventlist.h,v 1.6 2001/09/20 17:02:16 field Exp $
//
// $Log: eventlist.h,v $
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

class EventList
{
  private:
    struct event {
      unsigned long long id;
      time_t    startzeit;
      string    datetimeduration;
      string    name;
    };

    void removeAllEvents(void);
    void readEvents(const std::string& channelname); // I really don't like handling names
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
    void exec(const string& channelname);
};


#endif // EVENTLIST_HPP








