/*
$Id: dmx_sect.c,v 1.1 2001/09/30 13:05:20 rasc Exp $

 -- (c) 2001 rasc
 --  Sections Streams
 --  For more information please see:
 --  ISO 13818 (-1) and ETSI 300 468

 -- Verbose Level >= 1


$Log: dmx_sect.c,v $
Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/


#include "dvbsnoop.h"
#include "cmdline.h"
#include "hexprint.h"
#include "pkt_time.h"
#include "dmx_sect.h"
#include "sectables.h"



#define SECT_BUF_SIZE (64*1024)




int  doReadSECT (OPTION *opt)

{
  int     fd;
  u_char  buf[SECT_BUF_SIZE]; /* data buffer */
  long    count;
  int     i;




  if((fd = open(opt->devDemux,O_RDWR)) < 0){
      perror(opt->devDemux);
      return -1;
  }


  /*
   -- init demux
  */

{
  struct dmxSctFilterParams flt;
  memset (&flt, 0, sizeof (struct dmxSctFilterParams));
  flt.pid = opt->pid;
  flt.filter.filter[0] = opt->filter;
  flt.filter.mask[0] = opt->mask;
  flt.timeout = 60000;
  flt.flags = DMX_IMMEDIATE_START;
  if (opt->crc) flt.flags |= DMX_CHECK_CRC;

  if ((i=ioctl (fd, DMX_SET_FILTER, &flt)) < 0) {
    perror ("DMX_SET_FILTER failed: ");
    return -1;
  }

}




/*
  -- read packets for pid
*/

  count = 0;
  while (1) {
    long   n;


    n = read(fd,buf,sizeof(buf));

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
       out_nl (1,"SECT-Packet: %08ld   PID: %u (0x%04x), Length: %d (0x%04x)",
		count, opt->pid,opt->pid,n,n);
       out_receive_time (1, opt);
       out_nl (1,"---------------------------------------------------------");


       if (opt->printhex) {
           printhex_buf (0,buf, n);
           out_NL(0);
       }


       // decode protocol
       if (opt->printdecode) {
          decodeSections_buf (buf,n ,opt->pid);
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
  return;
}








