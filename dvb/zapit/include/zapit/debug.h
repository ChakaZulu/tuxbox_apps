/*
 * $Id: debug.h,v 1.2 2002/12/02 22:22:46 thegoodguy Exp $
 *
 * (C) 2002 by Andreas Oberritter <obi@tuxbox.org>
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

#ifndef __zapit_debug_h__
#define __zapit_debug_h__


#include <errno.h>
#include <stdio.h>
#include <string.h>


#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#define DEBUG	1
#endif


/* zapit.cpp */
extern int debug;


#define DBG(fmt, args...)					\
	do {							\
		if (debug)					\
			fprintf(stdout, "[%s:%s:%d] " fmt "\n",	\
				__FILE__, __FUNCTION__,		\
				__LINE__ , ## args);		\
	} while (0)


#define ERROR(str)						\
	do {							\
		fprintf(stderr, "[%s:%s:%d] %s: %s\n",		\
			__FILE__, __FUNCTION__,			\
			__LINE__, str, strerror(errno));	\
	} while (0)


#ifdef DEBUG


#define INFO(fmt, args...)					\
	do {							\
		fprintf(stdout, "[%s:%s:%d] " fmt "\n",		\
			__FILE__, __FUNCTION__,			\
			__LINE__ , ## args);			\
	} while (0)


#define WARN(fmt, args...)					\
	do {							\
		fprintf(stderr, "[%s:%s:%d] " fmt "\n",		\
			__FILE__, __FUNCTION__,			\
			__LINE__ , ## args);			\
	} while (0)


#else /* DEBUG */


#define INFO(fmt, args...)
#define WARN(fmt, args...)


#endif /* DEBUG */


#endif /* __zapit_debug_h__ */
