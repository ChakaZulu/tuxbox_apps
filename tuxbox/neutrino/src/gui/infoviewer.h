#ifndef __infoview__
#define __infoview__

#include "../driver/rcinput.h"
#include "../driver/framebuffer.h"
#include "../driver/fontrenderer.h"
#include "../widget/color.h"
#include "../helpers/settings.h"
#include "../options.h"
#include "streaminfo.h"

#include "pthread.h"
#include "semaphore.h"
#include <sys/wait.h>
#include <signal.h>

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
		int					intShowDuration;

		pthread_t			thrViewer;
        pthread_cond_t      epg_cond;
        pthread_mutex_t     epg_mutex;

        pthread_t			thrLangViewer;

		int					InfoHeightY;
        int					InfoHeightY_Info;
        bool                ShowInfo_Info;
		int					BoxEndX;
		int					BoxEndY;
		int					BoxStartX;
		int					BoxStartY;
        int                 ButtonWidth;

        int                 ChanWidth;
        int                 ChanHeight;
        int                 ChanInfoX;

		string				CurrentChannel;
        char                *EPG_NotFound_Text;

		char				running[50];
		char				next[50];
		char				runningStart[10];
		char				nextStart[10];
		char				runningDuration[10];
		char				nextDuration[10];
		char				runningPercent;

		static void * InfoViewerThread (void *arg);
        static void * LangViewerThread (void *arg);
		bool getEPGData( string channelName );
		void showData();
        void showWarte();
        void showButtons();
	public:

        bool                is_visible;
        pthread_cond_t      lang_cond;

        CInfoViewer();

        void start();

        void showTitle( int ChanNum, string Channel, bool CalledFromNumZap = false );
        void killTitle();

        void setDuration( int Duration );
};


#endif


