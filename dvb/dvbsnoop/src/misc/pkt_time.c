/*
$Id: pkt_time.c,v 1.6 2003/12/20 05:11:42 obi Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 mainly for me to learn about dvb streams, mpeg2, mhp, dsm-cc, ...

 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de



$Log: pkt_time.c,v $
Revision 1.6  2003/12/20 05:11:42  obi
simplified timeval to ms conversion

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

static unsigned long timeval_to_ms(const struct timeval *tv)
{
	return (tv->tv_sec * 1000) + ((tv->tv_usec + 500) / 1000);
}

void  out_receive_time (int verbose, OPTION *opt)

{
 struct timeval           tv;
 time_t                   t;
 long                     ms;
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
	    ms = timeval_to_ms(&tv);
            out (verbose,"Time (delta) received: %0ld.%03ld (sec)\n", ms / 1000, ms % 1000);
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
	return timeval_to_ms(tv) - timeval_to_ms(last_tv);
}


