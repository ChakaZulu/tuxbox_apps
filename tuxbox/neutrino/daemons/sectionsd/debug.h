/*
 * $Header: /cvs/tuxbox/apps/tuxbox/neutrino/daemons/sectionsd/debug.h,v 1.3 2008/01/05 18:02:11 seife Exp $
 *
 * Debug tools (sectionsd) - d-box2 linux project
 *
 * (C) 2003 by thegoodguy <thegoodguy@berlios.de>
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

#ifndef __sectionsd__debug_h__
#define __sectionsd__debug_h__


#include <stdio.h>


extern bool debug;

#define dprintf(fmt, args...) do { if (debug) { printdate_ms(stdout); printf(fmt, ## args); fflush(stdout); }} while (0)
#define dputs(str)            do { if (debug) { printdate_ms(stdout); puts(str);            fflush(stdout); }} while (0)

void printdate_ms(FILE* f);

#endif /* __sectionsd__debug_h__ */

