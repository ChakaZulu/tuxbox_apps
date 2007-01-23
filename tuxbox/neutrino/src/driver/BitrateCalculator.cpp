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

#include "BitrateCalculator.h"

#define dprintf(fmt, args...) { if (0) { printf(fmt, ## args); fflush(stdout); } }

/*
  -- Print receive time of Packet

*/

static unsigned long timeval_to_ms(const struct timeval *tv)
{
	return (tv->tv_sec * 1000) + ((tv->tv_usec + 500) / 1000);
}

long delta_time_ms (struct timeval *tv, struct timeval *last_tv)
{
	return timeval_to_ms(tv) - timeval_to_ms(last_tv);
}

BitrateCalculator::BitrateCalculator(int inPid)
{
	pid = inPid;
	printf("PID: %u (0x%04x)\n", pid, pid);

	// -- open DVR device for reading
	pfd.events = POLLIN | POLLPRI;
	if((pfd.fd = open(DVRDEV, O_RDONLY|O_NONBLOCK)) < 0){
		printf("error on %s\n", DVRDEV);
		return;
	}

	if ((dmxfd=open(DMXDEV, O_RDWR)) < 0) {
		printf("error on %s\n", DMXDEV);
		close(pfd.fd);
		return;
	}

	ioctl (dmxfd,DMX_SET_BUFFER_SIZE, sizeof(buf));
	flt.pid = pid;
	flt.input = DMX_IN_FRONTEND;
	flt.output = DMX_OUT_TS_TAP;
	flt.pes_type = DMX_PES_OTHER;
	flt.flags = DMX_IMMEDIATE_START;
	if (ioctl(dmxfd, DMX_SET_PES_FILTER, &flt) < 0) {
		printf("error on DMX_SET_PES_FILTER");
		close(pfd.fd);
		close(dmxfd);
		return;
	}

	ioctl (dmxfd, DMX_START);
	gettimeofday (&first_tv, NULL);
	last_tv.tv_sec	=  first_tv.tv_sec;
	last_tv.tv_usec	=  first_tv.tv_usec;
	last_avg_tv	= first_tv;
	b_total		= 0;
	packets_total	= 0;
 	packets_bad	= 0;
}

BitrateCalculator::~BitrateCalculator(void)
{
	// -- packets stats
	printf("PID: %u (0x%04x)\n", pid, pid);
	printf("   bad/total packets: %ld/%ld (= %1.1Lf%%)\n",
		packets_bad, packets_total,
                (((long double) packets_bad)*100)/packets_total );
	printf("   Avrg: %5lu.%03lu kbit/s\n",
		last_avg.kb_sec, last_avg.b_sec);

	if (ioctl(dmxfd, DMX_STOP) < 0) {
		printf("error at DMX_STOP");
	}
	close(dmxfd);
	close(pfd.fd);
}

unsigned long BitrateCalculator::getAverage(void)
{
	unsigned long long avgbit_s = 0;
	long  d_tim_ms;

	gettimeofday (&tv, NULL);
	// -- average bandwidth
	d_tim_ms = delta_time_ms (&tv, &last_avg_tv);
	if (d_tim_ms <= 0) d_tim_ms = 1;   //  ignore usecs 

	avgbit_s = ((b_total * 8000ULL) + ((unsigned long long)d_tim_ms / 2ULL))
		   / (unsigned long long)d_tim_ms;

	last_avg.kb_sec = (unsigned long) (avgbit_s / 1000ULL);
	last_avg.b_sec  = (unsigned long) (avgbit_s % 1000ULL);

	dprintf("   (Avrg: %5lu.%03lu kbit/s)\n", last_avg.kb_sec, last_avg.b_sec);

	b_total = 0; // restart average measurement
	last_avg_tv.tv_sec  =  tv.tv_sec;
	last_avg_tv.tv_usec =  tv.tv_usec;
	return (unsigned long) (avgbit_s / 1000ULL);
}

unsigned long BitrateCalculator::calc(void)
{
	int 			b_len, b_start;
	unsigned long long 	bit_s = 0;
	long  			d_tim_ms;
	int   			packets;
	int 			timeout = 100;

	b_len = 0;
	b_start = 0;
	if (poll(&pfd, 1, timeout) > 0) {
		if (pfd.revents & POLLIN) {

			b_len = read(pfd.fd, buf, sizeof(buf));
			gettimeofday (&tv, NULL);
			
			if (b_len >= TS_LEN) {
				b_start = sync_ts (buf, b_len);
			} else {
				b_len = 0;
			}

			b = b_len - b_start;
			if (b == 0) return 0;
			if (b < 0) {
			   printf("error on read");
			   return 0;
			}

			b_total += b;

			// -- calc bandwidth
			packets = b/TS_LEN;
			packets_total += packets;

			// output on different verbosity levels 
			// -- current bandwidth
			d_tim_ms = delta_time_ms (&tv, &last_tv);
			if (d_tim_ms <= 0) d_tim_ms = 1;   //  ignore usecs 

			dprintf("packets read: %3d/(%ld)   d_time: %2ld.%03ld s  = ",
				packets, packets_total, d_tim_ms / 1000UL, d_tim_ms % 1000UL);

			// -- current bandwidth in kbit/sec
			// --- cast to unsigned long long so it doesn't overflow as
			// --- early, add time / 2 before division for correct rounding
			bit_s = (((unsigned long long)b * 8000ULL) + ((unsigned long long)d_tim_ms / 2ULL))
				   / (unsigned long long)d_tim_ms;

			dprintf("%5llu.%03llu kbit/s", bit_s / 1000ULL, bit_s % 1000ULL);

			// -- bad packet(s) check in buffer
			int bp = ts_error_count (buf+b_start, b);
			packets_bad += bp;
			dprintf(" [bad: %d]\n", bp);

			last_tv.tv_sec  =  tv.tv_sec;
			last_tv.tv_usec =  tv.tv_usec;
		}
	}
	return (unsigned long) (bit_s / 1000ULL);
}



//
// -- sync TS stream (if not already done by firmware)
//

int BitrateCalculator::sync_ts (u_char *buf, int len)
{
	int  i;

	// find TS sync byte...
	// SYNC ...[188 len] ...SYNC...
	
	for (i=0; i < len; i++) {
		if (buf[i] == TS_SYNC_BYTE) {
		   if ((i+TS_LEN) < len) {
		      if (buf[i+TS_LEN] != TS_SYNC_BYTE) continue;
		   }
		   break;
		}
	}
	return i;
}




//
//  count error packets (ts error bit set, if passed thru by firmware)
//  we are checking a synced buffer with 1..n TS packets
//  so, we have to check every TS_LEN the error bit
//  return: error count
//

int BitrateCalculator::ts_error_count (u_char *buf, int len) 
{
	int error_count = 0;

	while (len > 0) {

		// check  = getBits(buf, 0, 8, 1);
		if (*(buf+1) & 0x80) error_count++;

		len -= TS_LEN;
		buf += TS_LEN;

	}
	return error_count;
}
