/*
	nhttpd  -  DBoxII-Project

	Copyright (C) 2001/2002 Dirk Szymanski

	$Id: webserver.cpp,v 1.28 2006/03/29 15:31:55 yjogol Exp $

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


*/

// c++
#include <cerrno>

// system
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h> 
#include <sys/types.h>
#include <unistd.h>

// tuxbox
#include <config.h>
#include <configfile.h>

// nhttpd
#include "webserver.h"
#include "request.h"
#include "webdbox.h"
#include "debug.h"

#define NHTTPD_CONFIGFILE CONFIGDIR "/nhttpd.conf"

struct Cmyconn
{
	socklen_t	clilen;
	std::string	Client_Addr;
	int		Socket;
	CWebserver	*Parent;	
};

static pthread_mutex_t ServerData_mutex;
static unsigned long Requests = 0;

//-------------------------------------------------------------------------

CWebserver::CWebserver(bool debug)
{
	Port = 0;
	ListenSocket = -1;
	PublicDocumentRoot = "";
	PrivateDocumentRoot = "";
	STOP = false;
	NewGui = false;
	CDEBUG::getInstance()->Debug = debug;
	AuthUser = "";
	AuthPassword = "";
 
	ReadConfig();

	WebDbox = new CWebDbox(this);
	dprintf("WebDbox initialized\n");
}

//-------------------------------------------------------------------------

CWebserver::~CWebserver(void)
{
	if (ListenSocket != -1)
		Stop();

	if (WebDbox)
		delete WebDbox;
}

//-------------------------------------------------------------------------

void CWebserver::ReadConfig(void)
{
	CConfigFile *Config = new CConfigFile(',');

	Config->loadConfig(NHTTPD_CONFIGFILE);

	Port = Config->getInt32("Port", 80);
	THREADS = Config->getBool("THREADS", true);
	CDEBUG::getInstance()->Verbose = Config->getBool("VERBOSE", false);
	CDEBUG::getInstance()->Log = Config->getBool("LOG", false);
	MustAuthenticate = Config->getBool("Authenticate", false);
	PrivateDocumentRoot = Config->getString("PrivatDocRoot", PRIVATEDOCUMENTROOT);
	PublicDocumentRoot = Config->getString("PublicDocRoot", PUBLICDOCUMENTROOT);
	HostedDocumentRoot = Config->getString("HostedDocRoot", HOSTEDDOCUMENTROOT);
	NewGui = Config->getBool("NewGui", true);
	Zapit_XML_Path = Config->getString("Zapit_XML_Path", "/var/tuxbox/config/zapit");
	AuthUser = Config->getString("AuthUser", "root");
	AuthPassword = Config->getString("AuthPassword", "dbox2");
	NoAuthClient = Config->getString("NoAuthClient", "");

	if (Config->getUnknownKeyQueryedFlag() == true)
		Config->saveConfig(NHTTPD_CONFIGFILE);

	delete Config;
}

//-------------------------------------------------------------------------

bool CWebserver::Init(bool /*debug*/)
{
	return true;
}

//-------------------------------------------------------------------------

bool CWebserver::Start(void)
{
	SAI servaddr;

	if ((Port < 1024) && (geteuid() != 0))
	{
		aprintf("cannot bind to port %d without superuser privilleges. aborting.\n", Port);
		return false;
	}

	//network-setup
	ListenSocket = socket(AF_INET, SOCK_STREAM, 0);

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(Port);

        SetSockOpts();

	if (bind(ListenSocket, (SA *)&servaddr, sizeof(servaddr)) == -1)
	{
		perror("bind");
		Stop();
		return false;
	}

	if (listen(ListenSocket, 5) != 0)
	{
		perror("listen");
		Stop();
		return false;
	}

	dprintf("Server started\n");

	return true;
}

//-------------------------------------------------------------------------

void *WebThread(void *args)
{
	CWebserverRequest *req;
	Cmyconn *myconn = (Cmyconn *) args;

	if (!myconn) {
		aprintf("WebThread called without arguments!\n");
		pthread_exit(NULL);
	}

	pthread_detach(pthread_self());

	req = new CWebserverRequest(myconn->Parent);
	req->Client_Addr = myconn->Client_Addr;
	req->Socket = myconn->Socket;

	if (req->GetRawRequest())
	{
		dprintf("++ Thread 0x06%X gestartet\n", (int) pthread_self());
		
		if (req->ParseRequest())
		{
			req->SendResponse();
			req->PrintRequest();
			req->EndRequest();
		}
		else
		{
			dperror("Error while parsing request\n");
		}

		dprintf("-- Thread 0x06%X beendet\n",(int)pthread_self());
	}

	delete req;
	delete myconn;

	pthread_exit(NULL);
}

//-------------------------------------------------------------------------

void CWebserver::DoLoop(void)
{
	socklen_t clilen;
	SAI cliaddr;
	CWebserverRequest *req;
	int sock_connect;
	int thread_num = 0;
	int t = 1;
	pthread_t Threads[30];

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_mutex_init(&ServerData_mutex, NULL);

	clilen = sizeof(cliaddr);

	while (!STOP)
	{
		memset(&cliaddr, 0, sizeof(cliaddr));
		
		// accepting requests
		if ((sock_connect = accept(ListenSocket, (SA *) &cliaddr, &clilen)) == -1)
		{
			perror("Error in accept");
			continue;
		}

		setsockopt(sock_connect, SOL_TCP, TCP_CORK, &t, sizeof(t));

		// request from client arrives
		dprintf("nhttpd: got connection from %s\n", inet_ntoa(cliaddr.sin_addr));

		if (THREADS) // Multi Threaded
		{
			// prepare Cmyconn struct
			Cmyconn *myconn = new Cmyconn;
			myconn->clilen = clilen;
			myconn->Client_Addr = inet_ntoa(cliaddr.sin_addr);
			myconn->Socket = sock_connect;
			myconn->Parent = this;
			
			// start WebThread
			if (pthread_create(&Threads[thread_num], &attr, WebThread, (void *)myconn) != 0)
				dperror("pthread_create(WebThread)");
			if (thread_num == 20) // testing ??
				thread_num = 0;
			else
				thread_num++;
		}
		else // Single Threaded
		{
			// create new request
			req = new CWebserverRequest(this);
			req->Socket = sock_connect;
			req->Client_Addr = inet_ntoa(cliaddr.sin_addr);
			req->RequestNumber = Requests++;
			
			if (req->GetRawRequest()) //read request from client
			{
				if (req->ParseRequest()) // parse it
				{
					// send the proper response
					req->SendResponse();
					// and print if wanted
					req->PrintRequest();
				}
				else {
					dperror("Error while parsing request");
				}
			}
			
			// end the request
			req->EndRequest();
			
			delete req;
			req = NULL;
		}
	}
}

//-------------------------------------------------------------------------

void CWebserver::Stop(void)
{
	if (ListenSocket != -1)
	{
		close(ListenSocket);
		ListenSocket = -1;
	}
}

//-------------------------------------------------------------------------

int CWebserver::SocketConnect(Tmconnect *con, int Port)
{
	char rip[] = "127.0.0.1";

	con->sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	memset(&con->servaddr, 0, sizeof(SAI));
	con->servaddr.sin_family = AF_INET;
	con->servaddr.sin_port = htons(Port);
	inet_pton(AF_INET, rip, &con->servaddr.sin_addr);

#ifdef HAS_SIN_LEN
	servaddr.sin_len = sizeof(servaddr); // needed ???
#endif

	if (connect(con->sock_fd, (SA *)&con->servaddr, sizeof(con->servaddr)) == -1)
	{
		aprintf("[nhttp]: connect to socket %d failed\n",Port);
		return -1;
	}
	else {
		return con->sock_fd;
	}
}

//-------------------------------------------------------------------------

void CWebserver::SetSockOpts(void)
{
	// if no valid socket, return
	if (ListenSocket < 0)
		return;

#ifdef SO_REUSEADDR
	// Most important socket opt for us: SO_REUSEADDR, so bindings
	// on a port after fast restart of webserver do not fail
	int opt = 1;
	if (setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
		fprintf(stderr, "setsockopt(SO_REUSEADDR): %s\n", strerror(errno));
#endif
}

