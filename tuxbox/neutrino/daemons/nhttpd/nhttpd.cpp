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
#define NHTTPD_VERSION "1.2"

#include <signal.h>
 
#include <sys/types.h>
#include <stdio.h>

#include "webserver.h"
#include "webdbox.h"

using namespace std;

#define SA struct sockaddr
#define SAI struct sockaddr_in

//-------------------------------------------------------------------------

TWebserver* ws;


//-------------------------------------------------------------------------
void Ausgabe(char * OutputString)
{
	printf("[nhttpd] %s\n",OutputString);
}
//-------------------------------------------------------------------------

void sig_catch(int)
{
        Ausgabe("stop requested......");
        ws->Stop();
        delete(ws);
        exit(0);
}
//-------------------------------------------------------------------------

int main(int argc, char **argv)
{
bool debug = false;
bool verbose = false;
bool threads = true;
bool forken = false;

	
	if(argc > 1)
	{
		for(int i = 1; i < argc;i++)
		{
			if(strcmp(argv[i],"-d") == 0)
				debug = true;
			if(strcmp(argv[i],"-v") == 0)
				verbose = true;
			if(strcmp(argv[i],"-t") == 0)
				threads = false;
			if(strcmp(argv[i],"-f") == 0)
				forken = true;
			if( (strcmp(argv[i],"-version") == 0) || (strcmp(argv[i],"--version") == 0) ) 
			{
				printf("nhttp - Neutrino Webserver\nVersion: %s\n",NHTTPD_VERSION);
				return 0;
			}
			if( (strcmp(argv[i],"--help") == 0) || (strcmp(argv[i],"-h") == 0) )
			{
				printf("nhttpd Parameter:\n -d\t\tdebug Mode\n -v\t\tverbose Mode\n -t\t\tmultithreaded ausschalten\n -f\t\tforken\n -version\tversion\n --help\t\tdieser Text\n\n");
				return 0;
			}
		}
	}
	if(debug)
		printf("Starte %s\n",threads?"threaded":"nicht threaded");
	signal(SIGINT,sig_catch);	
	signal(SIGHUP,sig_catch);
	signal(SIGKILL,sig_catch);
	signal(SIGTERM,sig_catch);

	Ausgabe("Neutrino HTTP-Server starting..\n");

	if(forken)
		if (fork()) 
		{ 
			printf("nhttpd forked\n");
			exit(0); 
		}

	if((ws = new TWebserver()) != NULL)
	{
		if(ws->Init(80,"/share/tuxbox/neutrino/httpd",debug,verbose,threads))
		{
			if(ws->Start())
			{
				printf("httpd gestartet\n");
				ws->DoLoop();
				ws->Stop();
			}
		}
		else
			Ausgabe("Error initializing httpd");
		delete ws;
	}
	else
		Ausgabe("Error while creating httpd");		
	return 0; 
}
