/*
$Id: dmx_tspidscan.c,v 1.2 2003/12/09 18:27:48 rasc Exp $


 DVBSNOOP
 a dvb sniffer  and mpeg2 stream analyzer tool

 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de


 Brute force scan all pids on a transponder
 scanpids principle is based on the sourcefile getpids.c from 'obi'


$Log: dmx_tspidscan.c,v $
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
#include "misc/output.h"

#include "dmx_tspidscan.h"



/*
 * some definition
 */

// timeout in ms
// TimoutHIGH will be used on PIDs < 0x20
#define PID_TIMEOUT_LOW		220
#define PID_TIMEOUT_HIGH	30100

// max filters (will be checked dynamically)
#define MAX_PID_FILTER		64

// highest pid
#define MAX_PID                 0x1FFF

#define TS_LEN			188
#define TS_SYNC_BYTE		0x47
#define TS_BUF_SIZE		(TS_LEN * 2048)


static struct {
	int	pid;
	int	timeout;
} specialPids[] = {
	{ 0x00,	  2000 },	// PAT
	{ 0x01,	  2000 },	// CAT
	{ 0x02,	 10100 },	// TSDT
	{ 0x10,	 10100 },	// NIT, etc
	{ 0x11,	 10100 },	// BAT, etc
	{ 0x12,	 10100 },	// EIT, etc.
	{ 0x13,	 10100 },	// RST, etc.
	{ 0x14,	 30100 },	// TOT, etc.
	{ 0x15,	  2100 },	// 
	{ 0x1C,	  2100 },	// 
	{ 0x1D,	  2100 },	// 
	{ 0x1E,	  2100 },	// 
	{ 0x1F,	  2100 },	// DIT
	{ 0x1FFC, 10100 },	// ATSC
	{ 0x1FFD, 10100 },	// ATSC
	{ 0x1FFF, 10100 },	// ATSC
	{-1,	 0 }		// ende
};



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
  int		pid_found;




   indent (0);

   out_nl (1,"");
   out_nl (1,"---------------------------------------------------------");
   out_nl (1,"PID-SCAN...");
   out_nl (1,"$$$ EXPERIMENTAL...");
   out_nl (1,"---------------------------------------------------------");



  // alloc pids
  pidArray = (int *) malloc ( (MAX_PID+1) * sizeof(int) );
  	if (!pidArray) {
		perror("malloc");
		return -1;
	}

  	for (i=0; i <= MAX_PID ; i++) 
		pidArray[i] = 0;


   dmxfd = (int *) malloc(sizeof(int) * MAX_PID_FILTER);
	if (!dmxfd) {
		free (pidArray);
		perror("malloc");
		return -1;
	}

	for (i = 0; i < MAX_PID_FILTER; i++)
		dmxfd[i] = -1;


   // $$$ TODO seems to skip lots of packets...
   // $$$ do the following scan block, if found mark some low-repetition pids
   // $$$ as do be rescanned...

   pid = 0;
   while (pid <= MAX_PID) {

	pid_low = pid;
	   
	// -- open DVR device for reading
   	pfd.events = POLLIN | POLLPRI;
   	if((pfd.fd = open(opt->devDvr,O_RDONLY|O_NONBLOCK)) < 0){
		perror(opt->devDvr);
		free (pidArray);
		free (dmxfd);
		return -1;
   	}


	// set multi PID filter

	for (i = 0; (i < MAX_PID_FILTER) && (pid <= MAX_PID); i++) {
		if (dmxfd[i] < 0) {
			if ((dmxfd[i]=open(opt->devDemux,O_RDWR)) < 0) 
				break;
		}

		ioctl (dmxfd[i],DMX_SET_BUFFER_SIZE, sizeof(buf));

		flt.pid = pid++;
		flt.input = DMX_IN_FRONTEND;
		flt.output = DMX_OUT_TS_TAP;
		flt.pes_type = DMX_PES_OTHER;
		flt.flags = DMX_IMMEDIATE_START|DMX_ONESHOT;
		if (ioctl(dmxfd[i], DMX_SET_PES_FILTER, &flt) < 0) {
			perror("DMX_SET_PES_FILTER");
			break;
		}
	}

	out_nl (9,"scanning pid   0x%04x to 0x%04x",pid_low, pid-1);


	// -- calc timeout;
	// -- on lower pids: higher timeout
	// -- (e.g. TOT/TDT will be sent within 30 secs)

	timeout = PID_TIMEOUT_LOW;
	if ( (pid_low) < 0x20) timeout = PID_TIMEOUT_HIGH;



	out_nl (9,"... timeout %ld ms",timeout);
	if (poll(&pfd, 1, timeout) > 0) {
		if (pfd.revents & POLLIN) {
			int len; 
			len = read(pfd.fd, buf, sizeof(buf)/2);
			out_nl (9,"... read %d bytes",len);
			if (len >= TS_LEN) {
				pid_found = analyze_ts_pid (buf, len);
			}
		}
	}



	for (i = 0; i < MAX_PID_FILTER; i++) {
		if (dmxfd[i] >= 0) {
			if (ioctl(dmxfd[i], DMX_STOP) < 0)
				perror("DMX_STOP");
			close(dmxfd[i]);
			dmxfd[i] = -1;
		}
	}


	close(pfd.fd);
   }



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

out_SW_NL (3," ---PID found: ",pid);
			if (pidArray[pid] == 0) {
				pidArray[pid] = 1;
				out_SW_NL (3,"PID found: ",pid);
				found = 1;
			}

		}
	}

  	return found;
}




