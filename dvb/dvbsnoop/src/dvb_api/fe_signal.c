/*
$Id: fe_signal.c,v 1.3 2004/01/04 22:03:21 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de  (rasc)


 -- Signal Statistics  Current Frequency/Transponder
 --  DVB-API 




$Log: fe_signal.c,v $
Revision 1.3  2004/01/04 22:03:21  rasc
time for a version leap

Revision 1.2  2004/01/03 16:40:12  rasc
no message

Revision 1.1  2004/01/03 15:40:45  rasc
simple frontend signal status query added "-s signal"



*/


#include <fcntl.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>


#include "dvbsnoop.h"
#include "fe_signal.h"
#include "misc/cmdline.h"
#include "misc/output.h"
#include "misc/pkt_time.h"

#include "dvb_api.h"
#include "dmx_error.h"




typedef struct _fe_signal {
	int16_t   strength;
	int16_t   snr;
	uint32_t  ber;
	uint32_t  ublocks;
	fe_status_t status;
} FE_SIGNAL;

typedef struct _fe_signal_cap {
	int  ber;
	int  snr;
	int  strength;
	int  status;
	int  ublocks;
} FE_SIG_CAP;



static int capability_Check (int f, int cap);
static int read_Signal(int f, FE_SIGNAL *s, FE_SIG_CAP *cap);
static void out_status_detail (int v, fe_status_t s);




int  do_SignalStrength (OPTION *opt)

{
  int        fd_fe = 0;
  struct timeval tv,last_tv, first_tv;
  u_long     count;
  FE_SIGNAL  s;
  FE_SIG_CAP has;

 


  if (opt->inpPidFile) {
	fprintf (stderr,"Error: FileMode not possible...\n");
	return -1;
  } 


  if((fd_fe = open(opt->devFE,O_RDONLY)) < 0){
     IO_error(opt->devFE);
     return -1;
  }



  // -- check capabilities
 
  has.ber 	= capability_Check (fd_fe, FE_READ_BER);
  has.snr	= capability_Check (fd_fe, FE_READ_SNR);
  has.strength	= capability_Check (fd_fe, FE_READ_SIGNAL_STRENGTH);
  has.status	= capability_Check (fd_fe, FE_READ_STATUS);
  has.ublocks	= capability_Check (fd_fe, FE_READ_UNCORRECTED_BLOCKS);



   indent (0);
   out_NL (2);
   out_nl (2,"---------------------------------------------------------");
   out_nl (2,"Transponder/Frequency signal strength statistics...");
   if (opt->packet_count > 0) {
	   out_nl (2,"max cycle count: %ld ", opt->packet_count);
   }
   out_nl (9,"Capabilities: BER: %d  SNR: %d  SIG: %d  STAT: %d  UBLK: %d",
		   has.ber, has.snr, has.strength, has.status, has.ublocks);
   out_nl (2,"---------------------------------------------------------");




   gettimeofday (&first_tv, NULL);
   last_tv.tv_sec  =  first_tv.tv_sec;
   last_tv.tv_usec =  first_tv.tv_usec;



  count = 0;
  while (1) {
	int  err;
	int  d_tim_ms;


	count++;
	out (6, "cycle: %lu  ",count);

	gettimeofday (&tv, NULL);
   	d_tim_ms = delta_time_ms (&tv, &last_tv);
	if (d_tim_ms <= 0) d_tim_ms = 1;   //  ignore usecs 
	last_tv.tv_sec  =  tv.tv_sec;
	last_tv.tv_usec =  tv.tv_usec;

	out (6, "d_time: %ld.%03ld s  ", d_tim_ms / 1000UL, d_tim_ms % 1000UL);

				   

	err = read_Signal (fd_fe, &s, &has);
	    if (err == -1) return -1;
//	    if (err == -2) {
//		    out_nl (1," No signal...");
//		    continue;
//	    }

	    // & 0xFFFF necessary, due to interface transformations??
	if (has.strength)  out (1,"Sig: %u  ", s.strength & 0xFFFFL);
	if (has.snr)	   out (2,"SNR: %u  ", s.snr & 0xFFFFL);
	if (has.ber)	   out (3,"BER: %lu  ",s.ber);
	if (has.ublocks)   out (4,"UBLK: %lu  ",s.ublocks);
	if (has.status) {
		out (4,"Stat: 0x%02x ",s.status);
		out_status_detail (5,s.status);
	}
	out_NL(1);


	if (d_tim_ms == 0) usleep (1000);	// don't kill the system


	// count cycles? ?
	if (opt->packet_count && (opt->packet_count <= count)) break;

  } // while



  close(fd_fe);
  return 0;
}



/*
 * check capability of device function
 * return: 0/1 (has_capability)
 */

static int capability_Check (int f, int cap)
{
  u_long  dummy;

  if (ioctl(f,cap,&dummy) < 0) {
	if (errno == ENOSYS)  return 0;
  }

  return 1;
}




/*
 * -- read signal strength parameter 
 */

static int read_Signal(int f, FE_SIGNAL *s, FE_SIG_CAP *cap)
{
  int err = 0;

  s->strength = 0;
  s->ber = 0xFFFFFFFF;
  s->snr = 0;
  s->status = 0;
  s->ublocks = 0;


  if (cap->strength) {
  	err = ioctl(f,FE_READ_SIGNAL_STRENGTH,&s->strength);
  }

  if (cap->ber && ! err) {
  	err = ioctl(f,FE_READ_BER,&s->ber);
  }

  if (cap->snr && ! err) {
  	err = ioctl(f,FE_READ_SNR,&s->snr);
  }

  if (cap->status && ! err) {
  	err = ioctl(f,FE_READ_STATUS,&s->status);
  }

  if (cap->ublocks && ! err) {
  	err = ioctl(f,FE_READ_UNCORRECTED_BLOCKS,&s->ublocks);
  }


  if (err < 0) {
//  	if (errno == ENOSIGNAL)  return -2;
	IO_error ("frontend ioctl");
  	return -1;
  }
  return 0;
}



static void out_status_detail (int v,fe_status_t s)
{
	out (v,"[");
#ifdef DVB_API_1
	if (s & FE_HAS_SIGNAL) 	out (v,"SIG ");
	if (s & FE_HAS_LOCK) 	out (v,"LOCK ");
#else
	if (s & FE_HAS_SIGNAL) 	out (v,"SIG ");
	if (s & FE_HAS_CARRIER)	out (v,"CARR ");
	if (s & FE_HAS_VITERBI)	out (v,"VIT ");
	if (s & FE_HAS_SYNC) 	out (v,"SYNC ");
	if (s & FE_HAS_LOCK) 	out (v,"LOCK ");
	if (s & FE_TIMEDOUT) 	out (v,"TIMOUT ");
	if (s & FE_REINIT) 	out (v,"REINIT ");
#endif
	out (v,"]");
}



// Annotation:
//
// DVB_API 3:
// FE_HAS_SIGNAL = 0x01, /* found something above the noise level
// FE_HAS_CARRIER = 0x02, /* found a DVB signal */
// FE_HAS_VITERBI = 0x04, /* FEC is stable */
// FE_HAS_SYNC = 0x08, /* found sync bytes */
// FE_HAS_LOCK = 0x10, /* everything's working... */
// FE_TIMEDOUT = 0x20, /* no lock within the last Ÿ2 seconds
// FE_REINIT = 0x40 /* frontend was reinitialized, */
//
// DVB_API: 1
// FE_HAS_POWER = 0x01    the frontend is powered up and is ready to be used
// FE_HAS_SIGNAL = 0x02   the frontend detects a signal above the normal noise level
// FE_SPECTRUM_INV = 0x04 spectrum inversion is enabled/was necessary for lock
// FE_HAS_LOCK = 0x08 	  frontend successfully locked to a DVB signal
// TUNER_HAS_LOCK = 0x80 the tuner has a frequency lock




