/*
$Id: pkt_time.c,v 1.5 2003/12/14 23:38:46 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 mainly for me to learn about dvb streams, mpeg2, mhp, dsm-cc, ...

 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de



$Log: pkt_time.c,v $
Revision 1.5  2003/12/14 23:38:46  rasc
- bandwidth reporting for a PID

Revision 1.4  2003/11/26 16:27:46  rasc
- mpeg4 descriptors
- simplified bit decoding and output function

Revision 1.3  2003/10/26 19:06:27  rasc
no message

Revision 1.2  2001/10/02 21:52:44  rasc
- init der time_delta
- PES erweitert, PES arbeitet im read() noch nicht richtig!!
- muss tmbinc fragem, ob ich Mist baue, oder der Treiber (??)

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/


#include "dvbsnoop.h"
#include "cmdline.h"

#include "sys/time.h"
#include "unistd.h"
#include "time.h"


static struct timeval    last_tv = {0,0};


/*
  -- Print receive time of Packet

*/

void  out_receive_time (int verbose, OPTION *opt)

{
 struct timeval           tv;
 time_t                   t;
 long                     t_s,t_us;
 char                     tstr[128];



 switch (opt->time_mode) {

    case FULL_TIME:
            t = time (&t);
            strftime (tstr,sizeof(tstr)-1,"%a %Y-%m-%d  %H:%M:%S",
			localtime(&t));
            gettimeofday (&tv, NULL);
            out (verbose,"Time received: %s.%03ld\n", tstr, tv.tv_usec/1000 );
            break;

    case DELTA_TIME:
            gettimeofday (&tv, NULL);
            t_s  = tv.tv_sec  - last_tv.tv_sec;
            t_us = (tv.tv_usec - last_tv.tv_usec) / 1000;
            if (t_us < 0) {
                t_us += 1000;
                t_s--;
            }
            out (verbose,"Time (delta) received: %0ld.%03ld (sec)\n", t_s,t_us );
            last_tv.tv_sec  =  tv.tv_sec;
            last_tv.tv_usec =  tv.tv_usec;
            break;

    case NO_TIME:
    default:
            break;

 }

 return;
}




void  init_receive_time (void)

{
  gettimeofday (&last_tv, NULL);
}



long delta_time_ms (struct timeval *tv, struct timeval *last_tv)
{
	// - delta time
	long t_s, t_ms;
	

       	t_s  = tv->tv_sec  - last_tv->tv_sec;
	t_ms = (tv->tv_usec - last_tv->tv_usec) / 1000;
	if (t_ms < 0) {
		t_ms += 1000;
		t_s--;
	}

	return (long) t_s * 1000L + t_ms;
}


