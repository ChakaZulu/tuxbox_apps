#ifndef __epgdata__
#define __epgdata__


#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <string>
#include <stdio.h>
#include <unistd.h>

#include "../driver/framebuffer.h"
#include "../driver/rcinput.h"
#include "../driver/fontrenderer.h"
#include "../widget/color.h"
#include "../helpers/settings.h"
#include "../options.h"

#include "sectionsdMsg.h"

#include "pthread.h"

#include <string>
#include <vector>

using namespace std;


#define SA struct sockaddr
#define SAI struct sockaddr_in



struct EPGData
{
  char title[100];
  char info1[600];
  char info2[3000];
  char date[20];
  char start[10];
  char end[10];
  char done[6];
};


class CEpgData
{
	private:
		CFrameBuffer		*frameBuffer;
		FontsDef			*fonts;
		CRCInput			*rcInput;
		EPGData				epgData;
		SNeutrinoSettings	*settings;

		int				ox, oy, sx, sy;
		int				emptyLineCount;
		vector<string>	epgText;

		void GetEPGData( string channelName );
		void addTextToArray( string text );
		void processTextToArray( char* text );
		void showText( int startPos);

	public:

		CEpgData();
		void start(CFrameBuffer	*FrameBuffer, FontsDef *Fonts, CRCInput* RcInput, SNeutrinoSettings *Settings );
		void show( string channelName );
		void hide();
};


#endif
