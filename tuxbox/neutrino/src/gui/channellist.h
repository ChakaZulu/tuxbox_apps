#ifndef __channellist__
#define __channellist__
//
// $Id: channellist.h,v 1.8 2001/08/20 13:10:27 tw-74 Exp $
//
// $Log: channellist.h,v $
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
	FontsDef		*fonts;
	int			fheight; // Fonthoehe Channellist-Inhalt
	int			theight; // Fonthoehe Channellist-Titel

	int			key;
	string			name;
	vector<channel*>	chanlist;

	int 			width;
	int 			height;
	int 			x;
	int 			y;

	void paintItem(CFrameBuffer* fb, int pos);
	void paint(CFrameBuffer* fb);
	void paintHead(CFrameBuffer* fb);
	void hide(CFrameBuffer* fb);

	public:
	CChannelList(SNeutrinoSettings *settings, int Key=-1, string Name="", FontsDef *cfont=NULL);
	~CChannelList();
	void addChannel(int key, int number, string name);
	void setName(string Name);
	int getKey(int);
	string getActiveChannelName();
	int getActiveChannelNumber();

	void zapTo(CRemoteControl *remoteControl, CInfoViewer *infoViewer, int pos);
	bool showInfo(CInfoViewer *infoViewer, int pos);
	void updateEvents(void);
	void numericZap(CFrameBuffer *frameBuffer, CRCInput *rcInput, CRemoteControl *remoteControl, CInfoViewer *infoViewer, int key);
	void exec(CFrameBuffer *frameBuffer, CRCInput *rcInput, CRemoteControl *remoteControl, CInfoViewer *infoViewer, SNeutrinoSettings* settings);
	void quickZap(CFrameBuffer *frameBuffer, CRCInput *rcInput, CRemoteControl *remoteControl, CInfoViewer *infoViewer, SNeutrinoSettings* settings, int key);
};


#endif
