#ifndef __infoview__
#define __infoview__

#include "../driver/framebuffer.h"
#include "../driver/fontrenderer.h"
#include "../widget/color.h"
#include "../helpers/settings.h"
#include "../options.h"

#include "pthread.h"
#include "sectionsdMsg.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/timeb.h>
#include <time.h>

#include <string>

using namespace std;


#define SA struct sockaddr
#define SAI struct sockaddr_in


class CInfoViewer
{
	private:
		CFrameBuffer		*frameBuffer;
		FontsDef			*fonts;
		SNeutrinoSettings	*settings;

		int					intTimer;
		int					intShowDuration;
		pthread_t			thrViewer;

		
		int					InfoHeightY;
		int					BoxEndX;
		int					BoxEndY;
		int					BoxStartX;
		int					BoxStartY;

		string				CurrentChannel;
		bool				epgReady;

		char				running[50];
		char				next[50];
		char				runningStart[10];
		char				nextStart[10];
		char				runningDuration[10];
		char				nextDuration[10];
		char				runningPercent;
		
		static void * InfoViewerThread (void *arg);
		bool getEPGData( string channelName );
		void showData();
	public:

        CInfoViewer();

        void start(CFrameBuffer *FrameBuffer, FontsDef *Fonts, SNeutrinoSettings *Settings );

        void showTitle( int ChanNum, string Channel, bool reshow=false );
        void killTitle();

	bool isActive();	

        void setDuration( int Duration );
};


#endif
