/*
	webserver  -   DBoxII-Project

	Copyright (C) 2001/2002 Dirk Szymanski

	$ID$

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
#define NHTTPD_VERSION "1.3"

#include <signal.h>
 
#include <sys/types.h>
#include <stdio.h>

#include "webserver.h"
#include "webdbox.h"
#include "debug.h"

#include "config.h"

using namespace std;

#define SA struct sockaddr
#define SAI struct sockaddr_in

//-------------------------------------------------------------------------

CWebserver* webserver;


//-------------------------------------------------------------------------

void sig_catch(int)
{
        aprintf("stop requested......");
        webserver->Stop();
        delete(webserver);
        exit(0);
}
//-------------------------------------------------------------------------

int main(int argc, char **argv)
{
	bool debug = false;
	bool do_fork = true;

	int i;

	if (argc > 1)
	{
		for(i = 1; i < argc; i++)
		{

			if (strncmp(argv[i], "-d", 2) == 0)
			{
				debug = true;
				do_fork = false;
			}
			else 

			if (strncmp(argv[i], "-f", 2) == 0)
			{
				do_fork = false;
			}
			else if (strncmp(argv[i],"--version", 9) == 0) 
			{
				printf("nhttp - Neutrino Webserver\n");
				printf("Version: %s\n", NHTTPD_VERSION);
				return 0;
			}
			else if ((strncmp(argv[i], "--help", 6) == 0) || (strncmp(argv[i], "-h", 2) == 0))
			{
				printf("nhttpd parameters:\n");
				printf("-d\t\tdebug\n");
				printf("-f\t\tdo not fork\n");
				printf("--version\tversion\n");
				printf("--help\t\tthis text\n\n");
				return 0;
			}
		}
	}

	signal(SIGINT,sig_catch);
	signal(SIGHUP,sig_catch);
	signal(SIGTERM,sig_catch);

	aprintf("Neutrino HTTP-Server starting..\n");

	if (do_fork)
	{
		switch (fork())
		{
		case -1:
			dperror("fork");
			return -1;
		case 0:
			break;
		default:
			return 0;
		}

		if (setsid() == -1)
		{
			dperror("setsid");
			return -1;
		}
	}

	if ((webserver = new CWebserver(debug)) != NULL)
	{
			if (webserver->Start())
			{
				if(debug) printf("httpd gestartet\n");
				webserver->DoLoop();
				webserver->Stop();
			}
	}
	else
	{
		aprintf("Error initializing httpd");
		return -1;
	}

	return 0; 
}

