/*
$Id: dmx_tspidscan.c,v 1.14 2004/01/13 21:04:20 rasc Exp $


 DVBSNOOP
 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)


 -- Brute force scan all pids on a transponder
 -- scanpids principle is based on the sourcefile getpids.c from 'obi'


$Log: dmx_tspidscan.c,v $
Revision 1.14  2004/01/13 21:04:20  rasc
BUGFIX: getbits overflow fixed...

Revision 1.13  2004/01/11 21:01:32  rasc
PES stream directory, PES restructured

Revision 1.12  2004/01/02 00:00:37  rasc
error output for buffer overflow

Revision 1.11  2004/01/01 20:09:23  rasc
DSM-CC INT/UNT descriptors
PES-sync changed, TS sync changed,
descriptor scope
other changes

Revision 1.10  2003/12/28 14:00:26  rasc
bugfix: section read from input file
some changes on packet header output

Revision 1.9  2003/12/15 22:41:28  rasc
pidscan improved, problems with max filters on demux

Revision 1.8  2003/12/15 22:29:27  rasc
pidscan improved, problems with max filters on demux

Revision 1.7  2003/12/15 20:09:48  rasc
no message

Revision 1.6  2003/12/14 18:29:56  rasc
no message

Revision 1.5  2003/12/10 23:18:10  rasc
improve pidscan

Revision 1.4  2003/12/10 20:07:15  rasc
minor stuff

Revision 1.3  2003/12/09 20:34:23  rasc
transponder pid-scan improved (should be sufficient now)

Revision 1.2  2003/12/09 18:27:48  rasc
pidscan on transponder

Revision 1.1  2003/12/07 23:36:13  rasc
pidscan on transponder
- experimental(!)


*/






#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <unistd.h>



#include "dvbsnoop.h"
#include "misc/cmdline.h"
#include "misc/helper.h"
#include "misc/output.h"

#include "dvb_api.h"
#include "dmx_error.h"
#include "dmx_tspidscan.h"



/*
 * some definition
 */

// min. buffer collect time before read/poll
// has to be below timeouts!!!
#define PID_TIME_WAIT		100

// timeout in ms
// TimoutHIGH will be used on PIDs < 0x20
#define PID_TIMEOUT_LOW		(290 - PID_TIME_WAIT)
#define PID_TIMEOUT_HIGH	(30100 - PID_TIME_WAIT)

// max filters (will be checked dynamically)
#define MAX_PID_FILTER		128

// highest pid
#define MAX_PID                 0x1FFF

#define TS_LEN			188
#define TS_SYNC_BYTE		0x47
#define TS_BUF_SIZE		(TS_LEN * 2048)



static int   *pidArray;
static int   analyze_ts_pid (u_char *buf, int len);



int ts_pidscan (OPTION *opt)
{

  u_char 	buf[TS_BUF_SIZE];
  struct pollfd pfd;
  struct dmx_pes_filter_params flt;
  int 		*dmxfd;
  int 		timeout;
  int		pid,pid_low;
  int    	i;
  int		filters;
  int		max_pid_filter;
  int		pid_found;
  int		rescan;




   indent (0);

   out_nl (2,"");
   out_nl (2,"---------------------------------------------------------");
   out_nl (2,"Transponder PID-Scan...");
   out_nl (2,"---------------------------------------------------------");



   //  -- max demux filters to use...
   max_pid_filter = MAX_PID_FILTER;
   if (opt->filter > 0) max_pid_filter = opt->filter;	// -f opt


   // alloc pids
   pidArray = (int *) malloc ( (MAX_PID+1) * sizeof(int) );
  	if (!pidArray) {
		IO_error("malloc");
		return -1;
	}

  	for (i=0; i <= MAX_PID ; i++) 
		pidArray[i] = 0;


   dmxfd = (int *) malloc(sizeof(int) * MAX_PID_FILTER);
	if (!dmxfd) {
		free (pidArray);
		IO_error("malloc");
		return -1;
	}

	for (i = 0; i < max_pid_filter; i++)
		dmxfd[i] = -1;



   pid = 0;
   while (pid <= MAX_PID) {

	pid_low = pid;

	do {
		pid = pid_low;
		rescan = 0;
	   
		// -- open DVR device for reading
	   	pfd.events = POLLIN | POLLPRI;
   		if((pfd.fd = open(opt->devDvr,O_RDONLY|O_NONBLOCK)) < 0){
			IO_error(opt->devDvr);
			free (pidArray);
			free (dmxfd);
			return -1;
   		}


		// -- set multi PID filter
		// -- try to get as many dmx filters as possible
		// -- error messages only if filter 0 fails

		filters = 0;
		for (i = 0; (i < max_pid_filter) && (pid <= MAX_PID); i++) {
			if (dmxfd[i] < 0) {
				if ((dmxfd[i]=open(opt->devDemux,O_RDWR)) < 0)  {
					// -- no filters???
					if (i == 0) IO_error(opt->devDemux);
					break;
				}
			}

			// ioctl (dmxfd[i],DMX_SET_BUFFER_SIZE, sizeof(buf));

			// -- skip already scanned pids (rescan-mode)
			while ( (pidArray[pid] != 0) && (pid < MAX_PID) ) pid++;
	
			flt.pid = pid;
			flt.input = DMX_IN_FRONTEND;
			flt.output = DMX_OUT_TS_TAP;
			flt.pes_type = DMX_PES_OTHER;
			flt.flags = DMX_IMMEDIATE_START;
			if (ioctl(dmxfd[i], DMX_SET_PES_FILTER, &flt) < 0) {
				if (i == 0) IO_error("DMX_SET_PES_FILTER");
				break;
			}
			pid ++;
			filters ++;
		}




		// -- ieek, no dmx filters available???
		// -- there is something terribly wrong here... - abort
		if (filters == 0) {

			pid = MAX_PID+1;	// abort criteria for loop

		} else {


			if (rescan) out (8,"re-");
			out (8,"scanning pid   0x%04x to 0x%04x",pid_low, pid-1);
			out (9,"  (got %d dmx filters) ",filters);
			out_NL (8);




			// -- calc timeout;
			// -- on lower pids: higher timeout
			// -- (e.g. TOT/TDT will be sent within 30 secs)
	
			timeout = PID_TIMEOUT_LOW;
			if ( (pid_low) < 0x20) timeout = PID_TIMEOUT_HIGH;

			// give read a chance to collect some pids
			usleep ((unsigned long) PID_TIME_WAIT);

			pid_found = 0;
			if (poll(&pfd, 1, timeout) > 0) {
				if (pfd.revents & POLLIN) {
					int len; 
					len = read(pfd.fd, buf, sizeof(buf));
					if (len >= TS_LEN) {
						pid_found = analyze_ts_pid (buf, len);
					}
				}
			}
	

			// rescan should to be done?
			if (pid_found) {
			  int x;
			  for (x=pid_low; x<pid; x++) {
				  if (pidArray[x] > 0) {
					  rescan = 1;
					  break;
				  }
			  }
			}

		} // if (filters==0)


		// -- close dmx, filters
		for (i = 0; i < max_pid_filter; i++) {
			if (dmxfd[i] >= 0) {
				ioctl(dmxfd[i], DMX_STOP);  // ignore any errors
				close(dmxfd[i]);
				dmxfd[i] = -1;
			}
		}


		close(pfd.fd);

	} while (rescan);
   } // while



  free (dmxfd);
  free (pidArray);
  return 0;

}



static int analyze_ts_pid (u_char *buf, int len)
{
	int  i;
	int  pid;
	int  found = 0;

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

	for (; i < len; i += TS_LEN) {
		if (buf[i] == TS_SYNC_BYTE) {
			pid	 = getBits (buf, i, 11, 13);

			pidArray[pid]++;
			if (pidArray[pid] == 1) {
				out_SW_NL (1,"PID found: ",pid);
				found = 1;
			}

		}
	}


  	return found;
}



/*
 * $$$ TODO 
 *  display PID content (Section Table, PES-streamID)
 */


