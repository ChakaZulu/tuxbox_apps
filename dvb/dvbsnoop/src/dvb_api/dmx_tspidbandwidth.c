/*
$Id: dmx_tspidbandwidth.c,v 1.4 2004/01/01 20:09:23 rasc Exp $


 DVBSNOOP
 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)




$Log: dmx_tspidbandwidth.c,v $
Revision 1.4  2004/01/01 20:09:23  rasc
DSM-CC INT/UNT descriptors
PES-sync changed, TS sync changed,
descriptor scope
other changes

Revision 1.3  2003/12/20 05:44:20  obi
- use more exact division,
- use unsigned long long for calculations because of overflows on high bandwidth pids,
- display kbit/s instead of kb/s to avoid confusion

Revision 1.2  2003/12/15 20:09:48  rasc
no message

Revision 1.1  2003/12/14 23:42:00  rasc
new: bandwidth usage reporting for a PID



*/






#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <unistd.h>



#include "dvbsnoop.h"
#include "misc/cmdline.h"
#include "misc/output.h"
#include "misc/pkt_time.h"

#include "dvb_api.h"
#include "dmx_tspidbandwidth.h"



/*
 * some definition
 */


#define TS_LEN			188
#define TS_SYNC_BYTE		0x47
#define TS_BUF_SIZE		(TS_LEN * 2048)


static int sync_ts (u_char *buf, int len);




int ts_pidbandwidth (OPTION *opt)
{

  u_char 	 buf[TS_BUF_SIZE];
  struct pollfd  pfd;
  struct dmx_pes_filter_params flt;
  int 		 dmxfd;
  struct timeval tv,last_tv, first_tv;
  int		 pid;
  long           b;
  long           packets_total;
  unsigned long long      b_total;



   pid = opt->pid;



   indent (0);
   out_nl (2,"");
   out_nl (2,"---------------------------------------------------------");
   out_nl (2,"PID bandwidth statistics...");
    if (opt->packet_count == 0) 
	out_nl (2,"PID: %u (0x%04x)", pid, pid);
    else 
	out_nl (2,"PID: %u (0x%04x)    - max packet count: %ld ", pid, pid, opt->packet_count);
   out_nl (2,"---------------------------------------------------------");



	// -- open DVR device for reading
   	pfd.events = POLLIN | POLLPRI;
	if((pfd.fd = open(opt->devDvr,O_RDONLY|O_NONBLOCK)) < 0){
		perror(opt->devDvr);
		return -1;
   	}



	if ((dmxfd=open(opt->devDemux,O_RDWR)) < 0) {
		perror(opt->devDemux);
		return -1;
	}
	ioctl (dmxfd,DMX_SET_BUFFER_SIZE, sizeof(buf));

		flt.pid = pid;
		flt.input = DMX_IN_FRONTEND;
		flt.output = DMX_OUT_TS_TAP;
		flt.pes_type = DMX_PES_OTHER;
		flt.flags = DMX_IMMEDIATE_START;
		if (ioctl(dmxfd, DMX_SET_PES_FILTER, &flt) < 0) {
			perror("DMX_SET_PES_FILTER");
			return -1;
		}



	gettimeofday (&first_tv, NULL);
	last_tv.tv_sec  =  first_tv.tv_sec;
	last_tv.tv_usec =  first_tv.tv_usec;

	b_total = 0;
	packets_total = 0;

	while (1) {
		int b_len, b_start;

		// -- we will poll the PID in 2 secs intervall
		int timeout = 2000;

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
				if (b <= 0) continue;

				b_total += b;



				// -- calc bandwidth

				{
				   unsigned long long  bit_s;
				   long  d_tim_ms;
				   int   packets;

				   packets = b/TS_LEN;
				   packets_total += packets;


				   // output on different verbosity levels 
				   // -- current bandwidth
				   d_tim_ms = delta_time_ms (&tv, &last_tv);
				   if (d_tim_ms <= 0) d_tim_ms = 1;   //  ignore usecs 

				   out (3, "packets read: %3d/(%ld)   d_time: %2ld.%03ld s  = ",
					packets, packets_total, d_tim_ms / 1000UL, d_tim_ms % 1000UL);

				   /* cast to unsigned long long so it doesn't overflow as early,
				    * add time / 2 before division for correct rounding */
				   bit_s = (((unsigned long long)b * 8000ULL) + ((unsigned long long)d_tim_ms / 2ULL)) / (unsigned long long)d_tim_ms;

				   out (1, "%5llu.%03llu kbit/s", bit_s / 1000UL, bit_s % 1000UL);

				   // -- average bandwidth
				   d_tim_ms = delta_time_ms (&tv,&first_tv);
				   if (d_tim_ms <= 0) d_tim_ms = 1;   //  ignore usecs 

				   bit_s = ((b_total * 8000ULL) + ((unsigned long long)d_tim_ms / 2ULL)) / (unsigned long long)d_tim_ms;

				   out (2, "   (Avrg: %5llu.%03llu kbit/s)", bit_s / 1000UL, bit_s % 1000UL);

				   out_NL (1);

				}


				last_tv.tv_sec  =  tv.tv_sec;
				last_tv.tv_usec =  tv.tv_usec;



				// count packets ?
				if (opt->packet_count > 0) {
					opt->packet_count -= b/TS_LEN;
					if (opt->packet_count <= 0) break;
				}


			}
		}

	}



	if (ioctl(dmxfd, DMX_STOP) < 0)
		perror("DMX_STOP");
	close(dmxfd);
	close(pfd.fd);


  return 0;

}



static int sync_ts (u_char *buf, int len)
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


