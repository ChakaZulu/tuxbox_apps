#ifndef __channellist__
#define __channellist__
//
// $Id: channellist.h,v 1.5 2001/08/16 23:19:18 McClean Exp $
//
// $Log: channellist.h,v $
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

		int					key;
		string				name;
		vector<channel*>	chanlist;

		int width;
		int height;
		int x;
		int y;

		void paintItem(CFrameBuffer* fb, FontsDef *Fonts, int pos );
		void paint(CFrameBuffer* fb, FontsDef *Fonts);
		void hide(CFrameBuffer* fb);

	public:

		CChannelList(int Key=-1, string Name="");
		~CChannelList();

		void addChannel(int key, int number, string name);
		void setName(string Name);
		int getKey(int);
		string getActiveChannelName();
		int getActiveChannelNumber();

		void zapTo(CRemoteControl *remoteControl, CInfoViewer *infoViewer, int pos);
		bool showInfo(CInfoViewer *infoViewer, int pos);
		void updateEvents(void);
		void numericZap(CFrameBuffer *frameBuffer, FontsDef *fonts, CRCInput *rcInput, CRemoteControl *remoteControl, CInfoViewer *infoViewer, int key);
		void exec(CFrameBuffer *frameBuffer, FontsDef *fonts, CRCInput *rcInput, CRemoteControl *remoteControl, CInfoViewer *infoViewer, SNeutrinoSettings* settings);
		void quickZap(CFrameBuffer *frameBuffer, FontsDef *fonts, CRCInput *rcInput, CRemoteControl *remoteControl, CInfoViewer *infoViewer, SNeutrinoSettings* settings, int key);

};


#endif
