/*
$Id: dmx_pes.c,v 1.5 2003/01/07 00:43:58 obi Exp $

 -- (c) 2001 rasc
 -- PE Streams
 --  For more information please see:
 --  ISO 13818 (-1) and ETSI 300 468

 -- Verbose Level >= 1


$Log: dmx_pes.c,v $
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
#include "cmdline.h"
#include "hexprint.h"
#include "pkt_time.h"
#include "dmx_pes.h"
#include "pespacket.h"



#define PES_BUF_SIZE  (256 * 1024)
#define READ_BUF_SIZE (2 * 64 * 1024)  // larger as 64KB !!




int  doReadPES (OPTION *opt)

{
  int     fd;
  u_char  buf[READ_BUF_SIZE]; /* data buffer */
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
  struct dmx_pes_filter_params flt;

  ioctl (fd,DMX_SET_BUFFER_SIZE, PES_BUF_SIZE);
  memset (&flt, 0, sizeof (struct dmx_pes_filter_params));

  flt.pid = opt->pid;
  flt.input  = DMX_IN_FRONTEND;
  flt.output = DMX_OUT_TAP;
  flt.pes_type = DMX_PES_OTHER;
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
    long    n,n1;
    long    l;


   /*
     -- read first 6 bytes to get length of Packet
     -- read rest ...
   */

    n = read(fd,buf,6);
    if (n == 6) {
        l = (buf[4]<<8) + buf[5];
	if (l > 0) {
           n1 = read(fd, buf+6, (unsigned int) l );
           n = (n1 < 0) ? n1 : 6+n1;
	}
    }


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
       out_nl (1,"PES-Packet: %08ld   PID: %u (0x%04x), Length: %d (0x%04x)",
		count, opt->pid,opt->pid,n,n);
       out_receive_time (1, opt);
       out_nl (1,"---------------------------------------------------------");


       if (opt->printhex) {
           printhex_buf (0,buf, n);
           out_NL(0);
       }


       // decode protocol
       if (opt->printdecode) {
          decodePES_buf (buf,n ,opt->pid);
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
  return 0;
}





