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

	// Revision 1.1  11.02.2002 20:20  dirch
	// Revision 1.2  22.03.2002 20:20  dirch

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
	PublicDocumentRoot = NULL;
	PrivateDocumentRoot = NULL;
	TimerList = NULL;
	DEBUG=false;
}
//-------------------------------------------------------------------------
TWebserver::~TWebserver()
{
	if(ListenSocket)
		Stop();

	if(PrivateDocumentRoot)
		delete PrivateDocumentRoot;
	if(PublicDocumentRoot)
		delete PublicDocumentRoot;

	if(WebDbox)
		delete WebDbox;
	if(TimerList)
		delete TimerList;
}
//-------------------------------------------------------------------------
bool TWebserver::Init(int port,char * publicdocumentroot,bool debug,bool verbose,bool threads)
{
	Port=port;
	DEBUG = debug;
	THREADS = threads;
	VERBOSE = verbose;
	PrivateDocumentRoot = new TString(PRIVATEDOCUMENTROOT);
	PublicDocumentRoot = new TString(publicdocumentroot);
	WebDbox = new TWebDbox(this);
	if(DEBUG) printf("WebDbox initialized\n");
	return true;
}
//-------------------------------------------------------------------------
/*
void * TimerThread(void  *timerlist)
{
bool ende = false;
int SleepTime = 10;
TTimerList *TimerList = (TTimerList *) timerlist;

	do
	{
		if(TimerList)
			if(TimerList->Count > 0)
			{
				TimerList->ProcessList();
				TimerList->PrintList();
			}
		sleep(SleepTime);
	}while(!ende);
	pthread_exit((void *)NULL);
//	pthread_detach(pthread_self());

}
*/
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
				printf("%d. Versuch, warte weitere 5 Sekunden\n",i++);
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
				
//	if (pthread_create (&timerthread, NULL, TimerThread, (void *)TimerList) != 0 )
//		perror("[nhttpd]: pthread_create(TimerThread)");

	return true;
}

//-------------------------------------------------------------------------
void * WebThread(void * request)
{
TWebserverRequest *req = (TWebserverRequest *)request;
	if(req->Parent->DEBUG) printf("*********** Thread %X gestartet\n",(int)pthread_self());
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
	if(req->Parent->DEBUG) printf("*********** Thread %X beendet\n",(int)pthread_self());
	pthread_exit((void *)NULL);
//	if(req->Parent->DEBUG) printf("*********** Thread %ld gelöscht\n",(int)pthread_self());
//	pthread_detach(pthread_self());
}
//-------------------------------------------------------------------------
void TWebserver::DoLoop()
{
socklen_t			clilen;
SAI					cliaddr;
TWebserverRequest	*req;
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
        if(DEBUG) printf("server: got connection from %ld\n", cliaddr.sin_addr);
            
		req = new TWebserverRequest(this);
		memcpy(&(req->cliaddr),&cliaddr,sizeof(cliaddr));
		if(req->GetRawRequest(sock_connect))
		{

			if(THREADS)
			{
				if(DEBUG) printf("Create Thread\n");
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
					if(DEBUG) printf("Response gesendet\n");
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
