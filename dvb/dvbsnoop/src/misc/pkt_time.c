/*
$Id: pkt_time.c,v 1.1 2001/09/30 13:05:20 rasc Exp $

 -- Print Packet receive time
 -- (c) 2001 rasc


$Log: pkt_time.c,v $
Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/


#include "dvbsnoop.h"
#include "cmdline.h"

#include "sys/time.h"
#include "unistd.h"
#include "time.h"



int  out_receive_time (int verbose, OPTION *opt)

{

 static struct timeval    last_tv = {0,0};
 static int               first_call = 1;
 struct timeval           tv;
 time_t                   t;
 long                     t_s,t_us;
 char                     tstr[128];


 if (first_call) {
    gettimeofday (&last_tv, NULL);
    first_call = 0;
 }


 switch (opt->time_mode) {

    case FULL_TIME:
            t = time (&t);
            strftime (tstr,sizeof(tstr)-1,"%a %d.%m.%Y  %H:%M:%S",
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

