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

#include <string>

#include "pthread.h"
#include "config.h"
#include "../../libconfigfile/configfile.h"
#include "../neutrinoNG/neutrinoMessages.h"
#include "../libevent/eventserver.h"

using namespace std;

#define SA struct sockaddr
#define SAI struct sockaddr_in
#define PRIVATEDOCUMENTROOT "/share/tuxbox/neutrino/httpd"
#define PUBLICDOCUMENTROOT "/var/tmp/httpd"

class TWebDbox;
class TWebserverRequest;

struct Tmconnect
{
	int sock_fd;
	SAI servaddr;
};

//----------------------------------------------------------------------
class CWebserver
{
	int				Port;
	int				ListenSocket;
	string			PrivateDocumentRoot;
	string			PublicDocumentRoot;
	pthread_t		Thread1;
	pthread_t		timerthread;
	bool			THREADS;
	bool			NewGui;


public:
	bool			STOP;
	bool			DEBUG;
	bool			VERBOSE;
	bool			MustAuthenticate;
	TWebDbox		*WebDbox;
	CEventServer	EventServer;
	CConfigFile		*Config;

	CWebserver();
	~CWebserver();

	bool Init(bool debug);
	bool Start();
	void DoLoop();
	void Stop();
	void Debug(char * text){if(DEBUG) Ausgabe(text);};
	void Ausgabe(char *text){if(text) printf("[httpd] %s\n",text);};

	int SocketConnect(Tmconnect * con,int Port);

	friend class CWebserverRequest;
	friend class TWebDbox;
};


#endif
