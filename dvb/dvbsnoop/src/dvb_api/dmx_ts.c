/*
$Id: dmx_ts.c,v 1.8 2003/10/24 22:17:18 rasc Exp $

 -- (c) 2001 rasc
 -- Transport Streams
 --  For more information please see:
 --  ISO 13818 (-1) and ETSI 300 468

 -- Verbose Level >= 1

$Log: dmx_ts.c,v $
Revision 1.8  2003/10/24 22:17:18  rasc
code reorg...

Revision 1.7  2003/10/16 19:02:28  rasc
some updates to dvbsnoop...
- small bugfixes
- tables updates from ETR 162

Revision 1.6  2003/05/28 01:35:01  obi
fixed read() return code handling

Revision 1.5  2003/01/07 00:43:58  obi
set buffer size to 256kb

Revision 1.4  2002/11/01 20:38:40  Jolt
Changes for the new API

Revision 1.3  2002/08/17 20:36:12  obi
no more compiler warnings

Revision 1.2  2001/10/06 18:19:18  Toerli
Steuerzeichen entfernt. rasc wuerdest du mal bitte nen gescheiten unix-konformen Editor verwenden... windows editoren sind ungeeignet

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/


#include "dvbsnoop.h"
#include "dmx_ts.h"
#include "misc/cmdline.h"
#include "misc/hexprint.h"
#include "misc/pkt_time.h"
#include "ts/tslayer.h"



#define TS_BUF_SIZE  (256 * 1024)
#define READ_BUF_SIZE (188)          /* TS RDSIZE is fixed !! */




int  doReadTS (OPTION *opt)

{
  int     fd_dmx = 0, fd_dvr = 0;
  u_char  buf[READ_BUF_SIZE]; /* data buffer */
  long    count;
  int     i;
  char    *f;
  int     fileMode;


  

  if (opt->inpPidFile) {
  	f        = opt->inpPidFile;
        fileMode  = 1;
  } else {
  	f        = opt->devDvr;
        fileMode  = 0;
  } 


  if((fd_dvr = open(f,O_RDONLY)) < 0){
      perror(f);
      return -1;
  }


  


  /*
   -- init demux
  */

  if (!fileMode) {
    struct dmx_pes_filter_params flt;

    if((fd_dmx = open(opt->devDemux,O_RDWR)) < 0){
        perror(opt->devDemux);
	close (fd_dvr);
        return -1;
    }


    ioctl (fd_dmx,DMX_SET_BUFFER_SIZE, TS_BUF_SIZE);
    memset (&flt, 0, sizeof (struct dmx_pes_filter_params));

    flt.pid = opt->pid;
    flt.input  = DMX_IN_FRONTEND;
    flt.output = DMX_OUT_TS_TAP;
    flt.pes_type = DMX_PES_OTHER;
    flt.flags = 0;

    if ((i=ioctl(fd_dmx,DMX_SET_PES_FILTER,&flt)) < 0) {
      perror ("DMX_SET_PES_FILTER failed: ");
      return -1;
    }

    if ((i=ioctl(fd_dmx,DMX_START,&flt)) < 0) {
      perror ("DMX_START failed: ");
      return -1;
    }

}




/*
  -- read packets for pid
*/

  count = 0;
  while (1) {
    long   n;


    n = read(fd_dvr,buf,sizeof(buf));


    // -- error or eof?
    if (n == -1) perror("read");
    if (n < 0)  continue;
    if (n == 0) {
	if (!fileMode) continue;	// DVRmode = no eof!
	else break;			// filemode eof 
    }



    count ++;

    if (opt->binary_out) {

       // direct write to FD 1 ( == stdout)
       write (1, buf,n);

    } else {

       indent (0);

       out_nl (1,"");
       out_nl (1,"---------------------------------------------------------");
       out_nl (1,"TS-Packet: %08ld   PID: %u (0x%04x), Length: %d (0x%04x)",
		count, opt->pid,opt->pid,n,n);
       out_receive_time (1, opt);
       out_nl (1,"---------------------------------------------------------");


       if (opt->printhex) {
           printhex_buf (0,buf, n);
           out_NL(0);
       }


       // decode protocol
       if (opt->printdecode) {
          decodeTS_buf (buf,n ,opt->pid);
          out_nl (3,"==========================================================");
          out_nl (3,"");
       }
    } // bin_out



    // Clean Buffer
//    if (n > 0 && n < sizeof(buf)) memset (buf,0,n+1); 


    // count packets ?
    if (opt->packet_count > 0) {
       if (--opt->packet_count == 0) break;
    }


  } // while



  /*
    -- Stop Demux
  */

  if (!fileMode) {
     ioctl (fd_dmx, DMX_SET_FILTER, 0);
     ioctl (fd_dmx, DMX_STOP, 0);

     close(fd_dmx);
  }

  close(fd_dvr);
  return 0;
}





