/*
	nhttpd  -  DBoxII-Project

	Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'

	$Id: debug.h,v 1.6 2003/03/14 07:20:01 obi Exp $

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

#ifndef __nhttpd_debug_h__
#define __nhttpd_debug_h__

// system
#include <pthread.h>

// nhttpd
#include "request.h"

class CDEBUG
{
	protected:
		pthread_mutex_t Log_mutex;
		char *buffer;
		FILE *Logfile;

		static CDEBUG *instance;

		CDEBUG(void);
		~CDEBUG(void);

	public:
		bool Debug;
		bool Log;
		bool Verbose;

		static CDEBUG *getInstance(void);
		static void deleteInstance(void);

		void printf(const char *fmt, ...);
		void debugprintf(const char *fmt, ...);
		void logprintf(const char *fmt, ...);
		void LogRequest(CWebserverRequest *Request);
};

#define aprintf(fmt, args...) \
	do { CDEBUG::getInstance()->printf("[nhttpd] " fmt, ## args); } while (0)

#define dprintf(fmt, args...) \
	do { CDEBUG::getInstance()->debugprintf("[nhttpd] " fmt, ## args); } while (0)

#define lprintf(fmt, args...) \
	do { CDEBUG::getInstance()->logprintf("[nhttpd] " fmt, ## args); } while (0)

#define dperror(str) \
	do { perror("[nhttpd] " str); } while (0)

#endif /* __nhttpd_debug_h__ */
