#ifndef __remotecontrol__
#define __remotecontrol__

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <string>
#include <stdio.h>
#include <unistd.h>

#include <string>

using namespace std;

#define SA struct sockaddr
#define SAI struct sockaddr_in



struct st_rmsg
{
	unsigned char version;
 	unsigned char cmd;
	unsigned short param;
	unsigned short param2;
	char param3[30];
};

class CRemoteControl
{
		st_rmsg	remotemsg;
		void send();

	public:

		CRemoteControl();
		void zapTo(int key,  string chnlname );
		void shutdown();
	
};


#endif
