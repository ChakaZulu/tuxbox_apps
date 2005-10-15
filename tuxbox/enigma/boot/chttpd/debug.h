/*
 * $Id: debug.h,v 1.1 2005/10/15 11:35:47 digi_casi Exp $
 *
 * (C) 2005 by digi_casi <digi_casi@tuxbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __chttpd_debug_h__
#define __chttpd_debug_h__

#include <pthread.h>
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
	do { CDEBUG::getInstance()->printf("[chttpd] " fmt, ## args); } while (0)

#define dprintf(fmt, args...) \
	do { CDEBUG::getInstance()->debugprintf("[chttpd] " fmt, ## args); } while (0)

#define lprintf(fmt, args...) \
	do { CDEBUG::getInstance()->logprintf("[chttpd] " fmt, ## args); } while (0)

#define dperror(str) \
	do { perror("[chttpd] " str); } while (0)

#endif /* __chttpd_debug_h__ */
