/*
$Id: dmx_pes.c,v 1.25 2004/02/15 22:22:28 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)


 -- PE Streams
 --  For more information please see:
 --  ISO 13818 (-1) and ETSI 300 468





$Log: dmx_pes.c,v $
Revision 1.25  2004/02/15 22:22:28  rasc
cmd option: -hexdumpbuffer -nohexdumpbuffer

Revision 1.24  2004/01/25 22:36:52  rasc
minor changes & enhancments

Revision 1.23  2004/01/11 22:49:40  rasc
PES restructured

Revision 1.22  2004/01/06 14:06:08  rasc
no message

Revision 1.21  2004/01/06 03:13:25  rasc
TS prints PES/Section ID on payload_start

Revision 1.20  2004/01/02 16:40:36  rasc
DSM-CC  INT/UNT descriptors complete
minor changes and fixes

Revision 1.19  2004/01/02 02:37:54  rasc
pes sync bugfix

Revision 1.18  2004/01/02 00:00:37  rasc
error output for buffer overflow

Revision 1.17  2004/01/01 20:09:23  rasc
DSM-CC INT/UNT descriptors
PES-sync changed, TS sync changed,
descriptor scope
other changes

Revision 1.16  2003/12/30 14:05:37  rasc
just some annotations, so I do not forget these over Sylvester party...
(some alkohol may reformat parts of /devbrain/0 ... )
cheers!

Revision 1.15  2003/12/28 22:53:40  rasc
some minor changes/cleanup

Revision 1.14  2003/12/28 14:00:25  rasc
bugfix: section read from input file
some changes on packet header output

Revision 1.13  2003/12/15 20:09:48  rasc
no message

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
#include "misc/print_header.h"

#include "pes/pespacket.h"
#include "dvb_api.h"
#include "dmx_error.h"
#include "dmx_pes.h"


#define PES_BUF_SIZE  (256 * 1024)
#define READ_BUF_SIZE (2 * 64 * 1024)  // larger (64KB + 6 Bytes) !!!




static long pes_UnsyncRead (int fd, u_char *buf, u_long len);
static long pes_SyncRead   (int fd, u_char *buf, u_long len, u_long *skipped_bytes);





int  doReadPES (OPTION *opt)

{
  int     fd;
  u_char  buf[READ_BUF_SIZE]; 		/* data buffer */
  u_char  *b;				/* ptr to packet start */
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
      IO_error(f);
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
      IO_error ("DMX_SET_PES_FILTER failed: ");
      return -1;
    }

  }




/*
  -- read PES packet for pid
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
	b = buf;
    } else {
    	n = pes_UnsyncRead(fd,buf,sizeof(buf));
	b = buf;
    }


    // -- error or eof?
    if (n < 0) {
	int err;
	
	err = IO_error("read");
	// if (err == ETIMEDOUT) break;		// Timout, abort
	continue;
    }

    if (n == 0) {
	if (dmxMode) continue;	// dmxmode = no eof!
	else break;		// filemode eof 
    }


    count ++;

    if (opt->binary_out) {

       // direct write to FD 1 ( == stdout)
       write (1, b, n);

    } else {

       indent (0);
       print_packet_header (opt, "PES", opt->pid, count, n, skipped_bytes);


       if (opt->buffer_hexdump) {
           printhex_buf (0, b, n);
           out_NL(0);
       }


       // decode protocol
       if (opt->printdecode) {
          decodePES_buf (b, n ,opt->pid);
          out_nl (3,"==========================================================");
          out_NL (3);
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
    long    n1,n2;
    long    l;
    u_long  sync;

    

    // -- simple PES sync... seek for 0x000001 (PES_SYNC_BYTE)
    // -- $$$ Q: has this to be byteshifted or bit shifted???
    //
    // ISO/IEC 13818-1:
    // -- packet_start_code_prefix -- The packet_start_code_prefix is
    // -- a 24-bit code. Together with the stream_id that follows it constitutes
    // -- a packet start code that identifies the beginning of a packet.
    // -- The packet_start_code_prefix  is the bit string '0000 0000 0000 0000
    // -- 0000 0001' (0x000001).
    // ==>   Check the stream_id with "dvb_str.c", if you do changes!
 

    *skipped_bytes = 0;
    sync = 0xFFFFFFFF;
    while (1) {
	u_char c;

    	n1 = read(fd,buf+3,1);
	if (n1 <= 0) return n1;			// error or strange, abort

	// -- byte shift for packet_start_code_prefix
	// -- sync found? 0x000001 + valid PESstream_ID
	// -- $$$ check this if streamID defs will be enhanced by ISO!!!

	c = buf[3];
	sync = (sync << 8) | c;
	if ( (sync & 0xFFFFFF00) == 0x00000100 ) {
		if (c >= 0xBC)  break;	
		// $$$ TODO
		// if (c == 0xBB)  ; // system_header_start
		// if (c == 0xBA)  ; // pack_header_start
		// if (c == 0xB9)  ; // MPEG_program_end
	}

	(*skipped_bytes)++;			// sync skip counter
    }


    // -- Sync found!

    *skipped_bytes -= 3;
    buf[0] = 0x00;   // write packet_start_code_prefix to buffer
    buf[1] = 0x00;
    buf[2] = 0x01;
    // buf[3] = streamID == recent read


    // -- read more 2 bytes (packet length)
    // -- read rest ...

    n1 = read(fd,buf+4,2);
    if (n1 == 2) {
        l = (buf[4]<<8) + buf[5];		// PES packet size...
	n1 = 6; 				// 4+2 bytes read
	// $$$ TODO    if len == 0, special unbound length

	if (l > 0) {
           n2 = read(fd, buf+6, (unsigned int) l );
           n1 = (n2 < 0) ? n2 : n1+n2;
	}
    }


    return n1;
}








