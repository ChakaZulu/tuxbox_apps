#ifndef __channellist__
#define __channellist__
//
// $Id: channellist.h,v 1.11 2001/09/18 11:48:43 fnbrd Exp $
//
// $Log: channellist.h,v $
// Revision 1.11  2001/09/18 11:48:43  fnbrd
// Changed some parameter to const string&
//
// Revision 1.10  2001/09/14 16:18:46  field
// Umstellung auf globale Variablen...
//
// Revision 1.9  2001/09/13 10:12:41  field
// Major update! Beschleunigtes zappen & EPG uvm...
//
// Revision 1.8  2001/08/20 13:10:27  tw-74
// cosmetic changes and changes for variable font size
//
// Revision 1.7  2001/08/20 01:51:12  McClean
// channellist bug fixed - faster channellist response
//
// Revision 1.6  2001/08/20 01:26:54  McClean
// stream info added
//
// Revision 1.5  2001/08/16 23:19:18  McClean
// epg-view and quickview changed
//
// Revision 1.4  2001/08/15 17:01:56  fnbrd
// Added id and log
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

class CChannelList
{

	struct channel
	{
		int	key;
		int	number;
		string	name;
		string  currentEvent;
	};


	unsigned int		selected;
	unsigned int		tuned;
	unsigned int		liststart;
	unsigned int		listmaxshow;
	unsigned int		numwidth;
	int			fheight; // Fonthoehe Channellist-Inhalt
	int			theight; // Fonthoehe Channellist-Titel

	int			key;
	string			name;
	vector<channel*>	chanlist;

	int 			width;
	int 			height;
	int 			x;
	int 			y;

	void paintItem(int pos);
	void paint();
	void paintHead();
	void hide();

	public:
    CChannelList(int Key=-1, const std::string& Name="");
	~CChannelList();
	void addChannel(int key, int number, const std::string& name);
	void setName(const std::string& Name);
	int getKey(int);
	const std::string& getActiveChannelName();
	int getActiveChannelNumber();

	void zapTo(int pos);
	bool showInfo(int pos);
	void updateEvents(void);
	void numericZap(int key);
	void exec();
	void quickZap(int key);
};


#endif


