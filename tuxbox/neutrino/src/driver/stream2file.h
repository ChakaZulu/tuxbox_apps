/*
 * $Id: stream2file.h,v 1.4 2004/05/04 11:34:40 thegoodguy Exp $
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

enum stream2file_error_msg_t
{
	STREAM2FILE_OK                 =  0,
	STREAM2FILE_BUSY               = -1,
	STREAM2FILE_INVALID_DIRECTORY  = -2,
	STREAM2FILE_INVALID_PID        = -3,
	STREAM2FILE_PES_FILTER_FAILURE = -4,
	STREAM2FILE_DVR_OPEN_FAILURE   = -5
};

stream2file_error_msg_t start_recording(const char * const filename,
					const char * const info,
					const unsigned long long splitsize,
					const unsigned int numpids,
					const unsigned short * const pids,
					const bool write_ts = true);
stream2file_error_msg_t stop_recording(void);

#endif

