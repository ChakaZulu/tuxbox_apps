/*
$Id: dvbsnoop.h,v 1.1 2001/09/30 13:05:20 rasc Exp $

 -- dvbsnoop
 -- a dvb sniffer tool
 -- mainly for me to learn the dvb streams

   (c) rasc

$Log: dvbsnoop.h,v $
Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/

#define VERSION  "0.7b"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <ost/dmx.h>


//typedef  unsigned long   u_long;
//typedef  unsigned int    u_int;
//typedef  unsigned char   u_char;


#include "helper.h"
#include "dvb_str.h"
#include "output.h"







#define DEMUX_DEVICE "/dev/ost/demux0"
#define DVR_DEVICE   "/dev/ost/dvr0"



