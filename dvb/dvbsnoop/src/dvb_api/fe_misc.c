/*
$Id: fe_misc.c,v 1.2 2004/03/21 18:02:45 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de  (rasc)


 -- FrontEnd routines...
 --  DVB-API 




$Log: fe_misc.c,v $
Revision 1.2  2004/03/21 18:02:45  rasc
corrections

Revision 1.1  2004/03/21 13:20:07  rasc
more -feinfo, some restructs on FE code



*/


#include <stdint.h>
#include <sys/time.h>
#include <errno.h>


#include "dvbsnoop.h"
#include "fe_misc.h"
#include "misc/output.h"

#include "dmx_error.h"
#include "dvb_api.h"






/*
 * check capability of device function
 * return: 0/1 (has_capability)
 */

int capability_Check (int f, int cap)
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

int read_Signal(int f, FE_SIGNAL *s, FE_SIG_CAP *cap)
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




/*
  -- DVI-API Frontend Status Flags -> verbose
  -- we do this API independent per flag...
  -- see history of dvb_api docs
 */

void out_status_detail (int v,fe_status_t s)
{
        out (v,"[");
#if DVB_API_VERSION == 1
        if (s & FE_HAS_SIGNAL)  out (v,"SIG ");
        if (s & FE_HAS_LOCK)    out (v,"LOCK ");
        if (s & FE_SPECTRUM_INV)out (v,"INV ");
#else
        if (s & FE_HAS_SIGNAL)  out (v,"SIG ");
        if (s & FE_HAS_CARRIER) out (v,"CARR ");
        if (s & FE_HAS_VITERBI) out (v,"VIT ");
        if (s & FE_HAS_SYNC)    out (v,"SYNC ");
        if (s & FE_HAS_LOCK)    out (v,"LOCK ");
        if (s & FE_TIMEDOUT)    out (v,"TIMOUT ");
        if (s & FE_REINIT)      out (v,"REINIT ");
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
// FE_TIMEDOUT = 0x20, /* no lock within the last 2 seconds
// FE_REINIT = 0x40 /* frontend was reinitialized, */
//
// DVB_API: 1
// FE_HAS_POWER = 0x01    the frontend is powered up and is ready to be used
// FE_HAS_SIGNAL = 0x02   the frontend detects a signal above the normal noise level
// FE_SPECTRUM_INV = 0x04 spectrum inversion is enabled/was necessary for lock
// FE_HAS_LOCK = 0x08 	  frontend successfully locked to a DVB signal
// TUNER_HAS_LOCK = 0x80 the tuner has a frequency lock






#if DVB_API_VERSION != 1

/*
 * -- read frontend info
 */

int read_FEInfo(int f, struct dvb_frontend_info *fi)
{
  int err = 0;


  err = ioctl(f, FE_GET_INFO, fi);
  if (err < 0) {
	IO_error ("frontend ioctl");
  	return -1;
  }
  return 0;
}



/*
 * -- read effective frontend params
 */

int read_FEParam(int f, struct dvb_frontend_parameters *p)
{
  int err = 0;


  err = ioctl(f, FE_GET_FRONTEND, p);
  if (err < 0) {
	IO_error ("frontend ioctl");
  	return -1;
  }
  return 0;
}




#endif




