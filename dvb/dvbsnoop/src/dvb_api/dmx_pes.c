/*
$Id: dmx_pes.c,v 1.12 2003/12/10 22:54:11 obi Exp $

 -- PE Streams
 --  For more information please see:
 --  ISO 13818 (-1) and ETSI 300 468


 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de




$Log: dmx_pes.c,v $
Revision 1.12  2003/12/10 22:54:11  obi
more tiny fixes

Revision 1.11  2003/11/26 16:27:46  rasc
- mpeg4 descriptors
- simplified bit decoding and output function

Revision 1.10  2003/11/24 23:52:16  rasc
-sync option, some TS and PES stuff;
dsm_addr inactive, may be wrong - due to missing ISO 13818-6

Revision 1.9  2003/10/24 22:45:05  rasc
code reorg...

Revision 1.8  2003/10/24 22:17:18  rasc
code reorg...

Revision 1.7  2003/10/16 19:02:27  rasc
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
#include "misc/cmdline.h"
#include "misc/output.h"
#include "misc/hexprint.h"
#include "misc/pkt_time.h" 

#include "pes/pespacket.h"
#include "dmx_pes.h"


#define PES_BUF_SIZE  (256 * 1024)
#define READ_BUF_SIZE (2 * 64 * 1024)  // larger as 64KB !!




static long pes_UnsyncRead (int fd, u_char *buf, u_long len);
static long pes_SyncRead   (int fd, u_char *buf, u_long len, u_long *skipped_bytes);





int  doReadPES (OPTION *opt)

{
  int     fd;
  u_char  buf[READ_BUF_SIZE]; /* data buffer */
  long    count;
  int     i;
  char    *f;
  int     openMode;
  int     dmxMode;



  if (opt->inpPidFile) {
  	f        = opt->inpPidFile;
  	openMode = O_RDONLY;
        dmxMode  = 0;
  } else {
  	f        = opt->devDemux;
  	openMode = O_RDWR;
        dmxMode  = 1;
  }


  if((fd = open(f,openMode)) < 0){
      perror(f);
      return -1;
  }




  /*
   -- init demux
  */

  if (dmxMode) {
    struct dmx_pes_filter_params flt;

    ioctl (fd,DMX_SET_BUFFER_SIZE, PES_BUF_SIZE);
    memset (&flt, 0, sizeof (struct dmx_pes_filter_params));

    flt.pid = opt->pid;
    flt.input  = DMX_IN_FRONTEND;
    flt.output = DMX_OUT_TAP;
    flt.pes_type = DMX_PES_OTHER;
    flt.flags = DMX_IMMEDIATE_START;

    if ((i=ioctl(fd,DMX_SET_PES_FILTER,&flt)) < 0) {
      perror ("DMX_SET_PES_FILTER failed: ");
      return -1;
    }

  }




/*
  -- read packets for pid
*/

  count = 0;
  while (1) {
    long    n;
    u_long  skipped_bytes = 0;


    /*
      -- Read PES packet
     */

    if (opt->packet_header_sync) {
    	n = pes_SyncRead(fd,buf,sizeof(buf), &skipped_bytes);
    } else {
    	n = pes_UnsyncRead(fd,buf,sizeof(buf));
    }


    // -- error or eof?
    if (n == -1) perror("read");
    if (n < 0)  continue;
    if (n == 0) {
	if (dmxMode) continue;	// dmxmode = no eof!
	else break;		// filemode eof 
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
       if (skipped_bytes) {
	       out_nl (1,"Syncing PES... (%ld bytes skipped)",skipped_bytes);
       }
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
  if (dmxMode) {
    ioctl (fd, DMX_STOP, 0);
  }


  close(fd);
  return 0;
}




/*
  -- read PES packet (unsynced)
  -- return: len // read()-return code
*/

static long pes_UnsyncRead (int fd, u_char *buf, u_long len)

{
    long    n,n1;
    long    l;


    /*
     -- read first 6 bytes to get length of Packet
     -- read rest ...
    */

    n = read(fd,buf,6);
    if (n == 6) {
        l = (buf[4]<<8) + buf[5];		// PES packet size...

	if (l > 0) {
           if ( (l+6) > len) return -1;		// prevent buffer overflow

           n1 = read(fd, buf+6, (unsigned int) l );
           n = (n1 < 0) ? n1 : 6+n1;
	}
    }


    return n;
}




/*
  -- read PES packet (Synced)
  -- return: len // read()-return code
*/

static long pes_SyncRead (int fd, u_char *buf, u_long len, u_long *skipped_bytes)

{
    long    n,n1;
    long    l;

    

    // -- simple PES sync... seek for 0x000001 (PES_SYNC_BYTE)
    // -- $$$ Q: has this to be byteshifted or bit shifted???
    // -- $$$ to be improved:

    *skipped_bytes = 0;
    buf[0] = 0xff;				// illegal bytes
    buf[1] = 0xff;
    while (1) {
    	n = read(fd,buf+2,1);
	if (n <= 0) return n;			// error or strange, abort

	// -- sync found ? 0x000001
	if (buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x01) break;

	buf[0] = buf[1];			//  -- shift buffer  (byte-shifter)
	buf[1] = buf[2];
	(*skipped_bytes)++;			// sync skip counter
    }


    // -- Sync found!
    // -- read more 3 bytes (streamID and packet length)
    // -- read rest ...

    n = read(fd,buf+3,3);
    if (n == 3) {
        l = (buf[4]<<8) + buf[5];		// PES packet size...

	if (l > 0) {
           if ( (l+6) > len) {
		fprintf (stderr,"buffer to small on pes read (%ld)\n",l);
	   	return -1;		// prevent buffer overflow
	   }

           n1 = read(fd, buf+6, (unsigned int) l );
           n = (n1 < 0) ? n1 : 6+n1;		// we already read 3+3 bytes
	}
    }


    return n;
}



