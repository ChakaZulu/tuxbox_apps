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
#ifdef HAVE_TRIPLEDRAGON
#include <zapit/td-demux-compat.h>
#include <tddevices.h>
#else
#if HAVE_DVB_API_VERSION >= 3
#include <linux/dvb/dmx.h>
#define DMXDEV	"/dev/dvb/adapter0/demux0"
#define DVRDEV	"/dev/dvb/adapter0/dvr0"
#else
#include <ost/dmx.h>
#define DMXDEV	"/dev/dvb/card0/demux0"
#define DVRDEV	"/dev/dvb/card0/dvr0"
#endif
#endif

/*
 * some definition
 */


#define TS_LEN			188
#define TS_SYNC_BYTE		0x47
#define TS_BUF_SIZE		(TS_LEN * 2048)		/* fix dmx buffer size */

#define AVERAGE_OVER_X_MEASUREMENTS   10


class BitrateCalculator
{
	protected:
		int 				pid;
#ifdef HAVE_TRIPLEDRAGON
		demux_pes_para			flt;
#else
#if HAVE_DVB_API_VERSION >= 3
		struct dmx_pes_filter_params	flt;
#else
		struct dmxPesFilterParams	flt;
#endif
		struct pollfd			pfd;
		struct timeval 			tv,last_tv, first_tv;
		long				b;
		long				packets_bad;
		u_char 	 			buf[TS_BUF_SIZE];
#endif
		int 				dmxfd;
		unsigned int			buffer[AVERAGE_OVER_X_MEASUREMENTS];
		unsigned int			buffer2[240];
		int				counter;
		int				counter2;
		unsigned int			sum;
		unsigned int			sum2;
		bool				first_round;
		bool				first_round2;

	public:
#ifdef HAVE_TRIPLEDRAGON
		BitrateCalculator(int inPid);
#elif HAVE_DVB_API_VERSION >= 3
		BitrateCalculator(int inPid, dmx_output_t flt_output = DMX_OUT_TS_TAP);
#else
		BitrateCalculator(int inPid, dmxOutput_t flt_output = DMX_OUT_TS_TAP);
#endif
		virtual unsigned int calc(unsigned int &long_average);
		int sync_ts (u_char *buf, int len);
		int ts_error_count (u_char *buf, int len);
		virtual ~BitrateCalculator();
};

#ifndef HAVE_TRIPLEDRAGON
class BitrateCalculatorRadio : public BitrateCalculator
{
	public:
		BitrateCalculatorRadio (int inPid);
		virtual ~BitrateCalculatorRadio();
		virtual unsigned int calc(unsigned int &long_average);
};
#else
/* lame hack */
#define BitrateCalculatorRadio BitrateCalculator
#endif

#endif
