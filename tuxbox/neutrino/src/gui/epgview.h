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
		EPGData			epgData;

		int			ox, oy, sx, sy;
		int			emptyLineCount, info1_lines;
		vector<string>		epgText;
		int			topheight,topboxheight;
		int			botheight,botboxheight;
		int			medlineheight,medlinecount;

		void GetEPGData( string channelName, unsigned int onid_tsid, unsigned long long id, time_t* startzeit );
		void addTextToArray( string text );
		void processTextToArray( char* text );
		void showText( int startPos, int ypos );

	public:

		CEpgData();
		void start( );
		void show( string channelName, unsigned int onid_tsid, unsigned long long id = 0, time_t* startzeit = NULL);
		void hide();
};


#endif

