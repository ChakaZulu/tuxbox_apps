/*
 * $Id: debug.h,v 1.1 2003/07/17 01:07:13 obi Exp $
 *
 * Copyright (C) 2002, 2003 Andreas Oberritter <obi@saftware.de>
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

#ifndef __dvb_debug_debug_h__
#define __dvb_debug_debug_h__

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if 1

/*
 * private definitions
 */

#define __DVB_DEBUG(file, prefix, fmt, args...)					\
	do {									\
		fprintf(file, "(" prefix ") [%s:%d] %s\n\t\"" fmt "\"\n",	\
			__FILE__ , __LINE__ , __PRETTY_FUNCTION__ , ## args);	\
	} while (0)

#define __DVB_MAINTAINER	"Andreas Oberritter <obi@saftware.de>"



/*
 * public definitions
 * ------------------
 * prefixes as in XFree86:
 *
 * (--) probed, (**) from config file, (==) default setting,
 * (++) from command line, (!!) notice, (II) informational,
 * (WW) warning, (EE) error, (NI) not implemented, (??) unknown
 */

#define DVB_ERROR(s)		__DVB_DEBUG(stderr, "EE", "%s: %s", s, strerror(errno))
#define DVB_INFO(fmt, args...)	__DVB_DEBUG(stdout, "II", fmt, ## args)
#define DVB_WARN(fmt, args...)	__DVB_DEBUG(stderr, "WW", fmt, ## args)

#define DVB_FATAL(fmt, args...)									\
	do {											\
		__DVB_DEBUG(stderr, "EE", fmt, ## args);					\
		fprintf(stderr, "EXECUTION STOPPED - PLEASE REPORT IMMEDIATELY TO %s!\n",	\
			__DVB_MAINTAINER); exit(EXIT_FAILURE);					\
	} while (0)

#define DVB_FOP(cmd, args...) ({			\
	int _r;						\
	if (fd >= 0) { if ((_r = ::cmd(fd, args)) < 0)	\
		DVB_ERROR(#cmd"(fd, "#args")"); }	\
	else { _r = fd; }				\
	_r;						\
})

#else

#define DVB_ERROR(s)
#define DVB_INFO(fmt, args...)
#define DVB_WARN(fmt, args...)
#define DVB_FATAL(fmt, args...)
#define DVB_FOP(cmd, args...) ::cmd(fd, args)

#endif

#endif /* __dvb_debug_debug_h__ */
