/*
$Id: dmx_ts.c,v 1.1 2001/09/30 13:05:20 rasc Exp $

 -- (c) 2001 rasc
 -- Transport Streams
 --  For more information please see:
 --  ISO 13818 (-1) and ETSI 300 468

 -- Verbose Level >= 1

$Log: dmx_ts.c,v $
Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/


#include "dvbsnoop.h"
#include "cmdline.h"
#include "hexprint.h"
#include "pkt_time.h"
#include "dmx_ts.h"
#include "tslayer.h"



#define TS_BUF_SIZE  (1024*1024)
#define READ_BUF_SIZE (188)          /* TS RDSIZE is fixed !! */




int  doReadTS (OPTION *opt)

{
  int     fd, fd_dvr;
  u_char  buf[READ_BUF_SIZE]; /* data buffer */
  long    count;
  int     i;





  if((fd_dvr = open(opt->devDvr,O_RDONLY)) < 0){
      perror(opt->devDvr);
      return -1;
  }


  if((fd = open(opt->devDemux,O_RDWR)) < 0){
      perror(opt->devDemux);
      return -1;
  }
  


  /*
   -- init demux
  */

{
  struct dmxPesFilterParams flt;

  ioctl (fd,DMX_SET_BUFFER_SIZE, TS_BUF_SIZE);
  memset (&flt, 0, sizeof (struct dmxPesFilterParams));

  flt.pid = opt->pid;
  flt.input  = DMX_IN_FRONTEND;
  flt.output = DMX_OUT_TS_TAP;
  flt.pesType = DMX_PES_OTHER;
  flt.flags = 0;

  if ((i=ioctl(fd,DMX_SET_PES_FILTER,&flt)) < 0) {
    perror ("DMX_SET_PES_FILTER failed: ");
    return -1;
  }

  if ((i=ioctl(fd,DMX_START,&flt)) < 0) {
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


    /*
      -- error ?
    */

    if (n <= 0) {
        fprintf (stderr,"Error on read: %ld\n",n);
        continue;
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

  ioctl (fd, DMX_SET_FILTER, 0);
  ioctl (fd, DMX_STOP, 0);


  close(fd);
  close(fd_dvr);
  return;
}





