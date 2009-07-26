/*
 * $Id: stream2file.h,v 1.13 2009/07/26 17:00:23 rhabarber1848 Exp $
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
	STREAM2FILE_OK                        =  0,
	STREAM2FILE_BUSY                      = -1,
	STREAM2FILE_INVALID_DIRECTORY         = -2,
	STREAM2FILE_INVALID_PID               = -3,
	STREAM2FILE_PES_FILTER_FAILURE        = -4,
	STREAM2FILE_DVR_OPEN_FAILURE          = -5,
	STREAM2FILE_RECORDING_THREADS_FAILED  = -6,
};

enum stream2file_status_t
{
	STREAM2FILE_STATUS_RUNNING            =  0,
	STREAM2FILE_STATUS_IDLE               =  1,
	STREAM2FILE_STATUS_BUFFER_OVERFLOW    = -1,
	STREAM2FILE_STATUS_WRITE_OPEN_FAILURE = -2,
	STREAM2FILE_STATUS_WRITE_FAILURE      = -3,
	STREAM2FILE_STATUS_READ_FAILURE       = -4,
	STREAM2FILE_STATUS_RECORDING_THREADS_FAILED = -6
};

struct stream2file_status2_t
{
	stream2file_status_t status;
	char dir[100];
	char filename[512];
};

stream2file_error_msg_t start_recording(const char * const filename,
					const char * const info,
					int mode,
					const bool with_o_sync,
					const bool with_fdatasync,
					const unsigned long long splitsize,
					const unsigned int numpids,
					const unsigned short * const pids,
					const bool write_ts = true,
					const unsigned int ringbuffers = 20);
stream2file_error_msg_t stop_recording(void);

#endif
