#ifndef eventlist_hpp
#define eventlist_hpp
//
// $Id: eventlist.h,v 1.1 2001/09/18 10:50:30 fnbrd Exp $
//
// $Log: eventlist.h,v $
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
      long long id;
      string name;
      time_t starttime;
      unsigned lengthInSeconds;
    };
    void removeAllEvents(void);

	unsigned int		selected;
	unsigned int		tuned;
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

	void paintItem(int pos);
	void paint();
	void paintHead();
	void hide();

  public:
	EventList(int Key=-1, string Name="");
	~EventList();
	void setName(string Name);
//	int getKey(int);
//	string getActiveChannelName();
//	int getActiveChannelNumber();

//	void zapTo(int pos);
//	bool showInfo(int pos);
    void readEvents(const char *channelname); // I really don't like handling names
//	void numericZap(int key);
    void exec(const char *channelname);
//	void quickZap(int key);
};


#endif








