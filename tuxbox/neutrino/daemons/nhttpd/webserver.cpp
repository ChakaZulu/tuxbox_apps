/*
	webserver  -   DBoxII-Project

	Copyright (C) 2001/2002 Dirk Szymanski


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

	$ID$

*/

#include "webserver.h"
#include "request.h"
#include "webdbox.h"
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/wait.h> 


//-------------------------------------------------------------------------
TWebserver::TWebserver()
{
	Port=0;
	ListenSocket = 0;
	PublicDocumentRoot = "";
	PrivateDocumentRoot = "";
	TimerList = NULL;
	DEBUG=false;
}
//-------------------------------------------------------------------------
TWebserver::~TWebserver()
{
	if(ListenSocket)
		Stop();

	if(WebDbox)
		delete WebDbox;
	if(TimerList)
		delete TimerList;
}
//-------------------------------------------------------------------------
bool TWebserver::Init(int port,string publicdocumentroot,bool debug,bool verbose,bool threads, bool auth)
{
	Port=port;
	DEBUG = debug;
	THREADS = threads;
	VERBOSE = verbose;
	MustAuthenticate = auth;
	PrivateDocumentRoot = PRIVATEDOCUMENTROOT;
	PublicDocumentRoot = publicdocumentroot;
	WebDbox = new TWebDbox(this);
	if(DEBUG) printf("WebDbox initialized\n");
	return true;
}
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
bool TWebserver::Start()
{
	SAI servaddr;

	//network-setup
	ListenSocket = socket(AF_INET, SOCK_STREAM, 0);

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(Port);
	TimerList = new TTimerList(this);
	
	if ( bind(ListenSocket, (SA *) &servaddr, sizeof(servaddr)) !=0)
	{
		int i = 1;
			do
			{
				printf("[nhttpd] bind to port %d failed...\n",Port);
				printf("%d. Versuch, warte 5 Sekunden\n",i++);
				sleep(5);
			}while(bind(ListenSocket, (SA *) &servaddr, sizeof(servaddr)) !=0);
//			return false;
	}

	if (listen(ListenSocket, 5) !=0)
	{
			Ausgabe("listen failed...");
			return false;
	}
	Debug("Server gestartet\n");

	// TimerThread starten
				
	return true;
}

//-------------------------------------------------------------------------
void * WebThread(void * request)
{
static int ThreadsCount = 0;
CWebserverRequest *req = (CWebserverRequest *)request;
	ThreadsCount++;
	while(ThreadsCount > 15)
	{
		printf("[nhttpd] Too many requests, waitin one sec\n");
		sleep(1);
	}
	if(req->Parent->DEBUG) printf("*********** Thread %d (%X) gestartet\n",ThreadsCount,(int)pthread_self());	
	if(req)
	{
		if(req->ParseRequest())
		{
			req->SendResponse();

			if(req->Parent->VERBOSE) req->PrintRequest();

			req->EndRequest();

		}
		else
			printf("Error while parsing request\n");

		delete req;
		if(req->Parent->DEBUG) printf("Nach delete req\n");
	}
	ThreadsCount--;
	if(req->Parent->DEBUG) printf("*********** Thread %X beendet, ThreadCount: %d\n",(int)pthread_self(),ThreadsCount);
	pthread_exit((void *)NULL);
//	if(req->Parent->DEBUG) printf("*********** Thread %ld gelöscht\n",(int)pthread_self());
//	pthread_detach(pthread_self());
}
//-------------------------------------------------------------------------
void TWebserver::DoLoop()
{
socklen_t			clilen;
SAI					cliaddr;
CWebserverRequest	*req;
int sock_connect;
int thread_num =0;
pthread_t Threads[10];


	clilen = sizeof(cliaddr);
	while(1)
	{
		memset(&cliaddr, 0, sizeof(cliaddr));

		if ((sock_connect = accept(ListenSocket, (SA *) &cliaddr, &clilen)) == -1) 
		{
                perror("Error in accept");
                continue;
        }
        if(DEBUG) printf("nhttpd: got connection from %s\n", inet_ntoa(cliaddr.sin_addr));
		req = new CWebserverRequest(this);
		memcpy(&(req->cliaddr),&cliaddr,sizeof(cliaddr));
		req->Socket = sock_connect;
		if(req->GetRawRequest())
		{

			if(THREADS)
			{
				if (pthread_create (&Threads[thread_num++], NULL, WebThread, (void *)req) != 0 )
					perror("[nhttpd]: pthread_create(WebThread)");
				if(thread_num == 10)
					thread_num = 0;
			}
			else
			{
				if(req->ParseRequest())
				{
				
					req->SendResponse();
					req->PrintRequest();
				}
				else
					Ausgabe("Error while parsing request");
				
				req->EndRequest();
				delete req;
				if(DEBUG) printf("Request beendet und gelöscht\n");
				req = NULL;
			}
		}
		else
			Ausgabe("Unable to read request");
	}
}

//-------------------------------------------------------------------------
void TWebserver::Stop()
{
	if(ListenSocket != 0)
	{
		Debug("ListenSocket closed\n");
		close( ListenSocket );
		ListenSocket = 0;
	}
}

//-------------------------------------------------------------------------

int TWebserver::SocketConnect(Tmconnect * con,int Port)
{
	char rip[]="127.0.0.1";

	con->sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	memset(&con->servaddr,0,sizeof(SAI));
	con->servaddr.sin_family=AF_INET;
	con->servaddr.sin_port=htons(Port);
	inet_pton(AF_INET, rip, &con->servaddr.sin_addr);

	#ifdef HAS_SIN_LEN
		servaddr.sin_len = sizeof(servaddr); // needed ???
	#endif


	if(connect(con->sock_fd, (SA *)&con->servaddr, sizeof(con->servaddr))==-1)
	{
		printf("[nhttp]: connect to socket %d failed",Port);
		perror("");
		return -1;
	}
	else
		return con->sock_fd;
}
