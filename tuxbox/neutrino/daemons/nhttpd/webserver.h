#ifndef __webserver__
#define __webserver__

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <config.h>

//#include <string>
//#include <vector>

#include "helper.h"
#include "pthread.h"
#include "timer.h"

using namespace std;

#define SA struct sockaddr
#define SAI struct sockaddr_in

class TWebDbox;
class TWebserverRequest;
class TTimerList;

struct Tmconnect
{
	int sock_fd;
	SAI servaddr;
};

//----------------------------------------------------------------------
class TWebserver
{
	int	Port;
	int	ListenSocket;
	TString *DocumentRoot;
	pthread_t		Thread1;
	pthread_t		timerthread;
	bool THREADS;
	TTimerList * TimerList;

public:
		bool DEBUG;

		TWebDbox * WebDbox;
		TWebserver();
		~TWebserver();
		
		bool Init(int port,char * documentroot,bool debug, bool threads);
		bool Start();
		void DoLoop();
		void Stop();
		void Debug(char * text){if(DEBUG) Ausgabe(text);};
		void Ausgabe(char *text){if(text) printf("[httpd] %s\n",text);};
		
		int SocketConnect(Tmconnect * con,int Port);

	friend class TWebserverRequest;
	friend class TWebDbox;
};


#endif
