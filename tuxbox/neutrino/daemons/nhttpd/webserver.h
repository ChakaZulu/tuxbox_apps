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
#include <pthread.h>

#include <configfile.h>
#include <neutrinoMessages.h>


using namespace std;

#define SA struct sockaddr
#define SAI struct sockaddr_in
#define PRIVATEDOCUMENTROOT "/share/tuxbox/neutrino/httpd"
#define PUBLICDOCUMENTROOT "/var/tmp/httpd"

class CWebDbox;
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

public:
// config vars / switches
	bool			STOP;
	bool			VERBOSE;
	bool			MustAuthenticate;
	bool			NewGui;

	string			PrivateDocumentRoot;
	string			PublicDocumentRoot;
	string			Zapit_XML_Path;

	string			AuthUser;
	string			AuthPassword;



	CWebDbox		*WebDbox;

	CWebserver(bool debug);
	~CWebserver();

	bool Init(bool debug);
	bool Start();
	void DoLoop();
	void Stop();

	int SocketConnect(Tmconnect * con,int Port);

	void SaveConfig();
	void ReadConfig();

	friend class CWebserverRequest;
	friend class TWebDbox;

};


#endif
