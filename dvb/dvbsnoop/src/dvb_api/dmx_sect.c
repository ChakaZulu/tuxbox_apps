/*
$Id: dmx_sect.c,v 1.5 2003/05/28 01:35:01 obi Exp $

 -- (c) 2001 rasc
 --  Sections Streams
 --  For more information please see:
 --  ISO 13818 (-1) and ETSI 300 468

 -- Verbose Level >= 1


$Log: dmx_sect.c,v $
Revision 1.5  2003/05/28 01:35:01  obi
fixed read() return code handling

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
  struct dmx_sct_filter_params flt;
  memset (&flt, 0, sizeof (struct dmx_sct_filter_params));
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

    if (n == -1)
	perror("read");

    if (n <= 0)
	continue;



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
  return 0;
}








