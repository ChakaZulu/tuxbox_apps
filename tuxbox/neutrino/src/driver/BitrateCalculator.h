/*
	Neutrino-GUI  -   DBoxII-Project


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

	heavily ripped from:
	DVBSNOOP
	a dvb sniffer  and mpeg2 stream analyzer tool
	http://dvbsnoop.sourceforge.net/

	(c) 2001-2006   Rainer.Scherg@gmx.de (rasc)

*/

#ifndef _BitrateCalculator_h
#define _BitrateCalculator_h

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <global.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <linux/dvb/dmx.h>

/*
 * some definition
 */


#define TS_LEN			188
#define TS_SYNC_BYTE		0x47
#define TS_BUF_SIZE		(TS_LEN * 2048)		/* fix dmx buffer size */

#define AVERAGE_OVER_X_MEASUREMENTS   10

#define DMXDEV	"/dev/dvb/adapter0/demux0"
#define DVRDEV	"/dev/dvb/adapter0/dvr0"

class BitrateCalculator
{
	protected:
		int 				pid;
		struct pollfd			pfd;
		struct dmx_pes_filter_params	flt;
		int 				dmxfd;
		struct timeval 			tv,last_tv, first_tv;
		long				b;
		long				packets_bad;
		unsigned int			buffer[AVERAGE_OVER_X_MEASUREMENTS];
		unsigned int			buffer2[240];
		int				counter;
		int				counter2;
		unsigned int			sum;
		unsigned int			sum2;
		u_char 	 			buf[TS_BUF_SIZE];
		bool				first_round;
		bool				first_round2;

	public:
		BitrateCalculator(int inPid, dmx_output_t flt_output = DMX_OUT_TS_TAP);
		virtual unsigned int calc(unsigned int &long_average);
		int sync_ts (u_char *buf, int len);
		int ts_error_count (u_char *buf, int len);
		virtual ~BitrateCalculator();
};

class BitrateCalculatorRadio : public BitrateCalculator
{
	public:
		BitrateCalculatorRadio (int inPid);
		virtual ~BitrateCalculatorRadio();
		virtual unsigned int calc(unsigned int &long_average);
};

#endif
