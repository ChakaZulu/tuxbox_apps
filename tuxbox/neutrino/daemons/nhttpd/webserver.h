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
	bool			THREADS;
	bool			NewGui;


public:
	CConfigFile		*Config;
// config vars / switches
	bool			STOP;
	bool			DEBUG;
	bool			VERBOSE;
	bool			MustAuthenticate;
	
	string			PrivateDocumentRoot;
	string			PublicDocumentRoot;
	string			Zapit_XML_Path;


	TWebDbox		*WebDbox;
	CEventServer	*EventServer;

	CWebserver(bool debug);
	~CWebserver();

	bool Init(bool debug);
	bool Start();
	void DoLoop();
	void Stop();

	int SocketConnect(Tmconnect * con,int Port);

	friend class CWebserverRequest;
	friend class TWebDbox;

};


#endif
