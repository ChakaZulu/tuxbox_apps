/*
	nhttpd  -  DBoxII-Project

	Copyright (C) 2001/2002 Dirk Szymanski

	$Id: nhttpd.cpp,v 1.20 2005/08/31 17:54:25 yjogol Exp $

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
#include <csignal>

// system
#include <unistd.h>

// nhttpd
#include "webserver.h"
#include "debug.h"

//-------------------------------------------------------------------------

static CWebserver *webserver = NULL;

//-------------------------------------------------------------------------

static void sig_catch(int msignal)
{
	switch (msignal) {
	case SIGPIPE:
		aprintf("got signal PIPE, nice!\n");
		break;
	case SIGHUP:
		aprintf("got signal HUP, reading config\n");
		if (webserver)
			webserver->ReadConfig();
		break;
	default:
		aprintf("stop requested......\n");
		if (webserver) {
			webserver->Stop();
			delete webserver;
			webserver = NULL;
		}
		exit(EXIT_SUCCESS); //FIXME: return to main() some way...
	}
}

//-------------------------------------------------------------------------

static void version(FILE *dest)
{
	fprintf(dest, "nhttpd - Neutrino Webserver v%s\n", NHTTPD_VERSION);
}

//-------------------------------------------------------------------------

static void usage(FILE *dest)
{
	version(dest);
	fprintf(dest, "command line parameters:\n");
	fprintf(dest, "-d, --debug    enable debugging code (implies -f)\n");
	fprintf(dest, "-f, --fork     do not fork\n");
	fprintf(dest, "-h, --help     display this text and exit\n\n");
	fprintf(dest, "-v, --version  display version and exit\n");
}

//-------------------------------------------------------------------------

int main(int argc, char **argv)
{
	bool do_fork = true;

	for (int i = 1; i < argc; i++)
	{
		if ((!strncmp(argv[i], "-d", 2)) || (!strncmp(argv[i], "--debug", 7)))
		{
			CDEBUG::getInstance()->Debug = true;
			do_fork = false;
		}
		else if ((!strncmp(argv[i], "-f", 2)) || (!strncmp(argv[i], "--fork", 6)))
		{
			do_fork = false;
		}
		else if ((!strncmp(argv[i], "-h", 2)) || (!strncmp(argv[i], "--help", 6)))
		{
			usage(stdout);
			return EXIT_SUCCESS;
		}
		else if ((!strncmp(argv[i], "-v", 2)) || (!strncmp(argv[i],"--version", 9))) 
		{
			version(stdout);
			return EXIT_SUCCESS;
		}
		else
		{
			usage(stderr);
			return EXIT_FAILURE;
		}
	}

	signal(SIGPIPE, sig_catch);
	signal(SIGINT, sig_catch);
	signal(SIGHUP, sig_catch);
	signal(SIGTERM, sig_catch);

	aprintf("Neutrino HTTP-Server starting..\n");

	if (do_fork)
	{
		switch (fork()) {
		case -1:
			dperror("fork");
			return -1;
		case 0:
			break;
		default:
			return EXIT_SUCCESS;
		}

		if (setsid() == -1)
		{
			dperror("[nhttpd] Error setsid");
			return EXIT_FAILURE;
		}
	}

	if ((webserver = new CWebserver(CDEBUG::getInstance()->Debug)))
	{
		if (webserver->Start())
		{
			webserver->DoLoop();
			webserver->Stop();
		}
	}
	else
	{
		aprintf("Error initializing nhttpd\n");
		return EXIT_FAILURE;
	}

	webserver->Stop();
        delete webserver;

	return EXIT_SUCCESS; 
}

