/*
 * $Id: stream2file.h,v 1.2 2004/05/03 15:24:38 thegoodguy Exp $
 *
 * (C) 2004 by thegoodguy <thegoodguy@berlios.de>
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

#ifndef __stream2file_h__
#define __stream2file_h__

bool start_recording(const char * const filename,
		     const char * const info,
		     const unsigned long long splitsize,
		     const unsigned int numpids,
		     const unsigned short * const pids);
bool stop_recording(void);

#endif

