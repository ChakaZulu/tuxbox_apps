/*
$Id: ts2secpes.c,v 1.2 2004/04/15 04:08:49 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de



 -- Transport Stream Sub-Decode  PES / SECTION

   


$Log: ts2secpes.c,v $
Revision 1.2  2004/04/15 04:08:49  rasc
no message

Revision 1.1  2004/04/15 03:40:39  rasc
new: TransportStream sub-decoding (ts2PES, ts2SEC)  [-tssubdecode]
checks for continuity errors, etc. and decode in TS enclosed sections/pes packets



*/




#include "dvbsnoop.h"
#include "ts2secpes.h"
#include "sections/sectables.h"
#include "pes/pespacket.h"
#include "misc/packet_mem.h"
#include "misc/output.h"





#define TS_SUBDEC_BUFFER   (128*1024)
enum  { TSD_no_error = 0,
	TSD_error, TSD_continuity_error, TSD_scrambled_error,
	TSD_pid_change, TSD_mem_error};


typedef struct _TS_SUBDEC {
	int	mem_handle;
	int	pid;
	int     invalid;		// content is invalid
	int     continuity_counter;	// 4 bit max !!
} TS_SUBDEC;


static  TS_SUBDEC  tsd;



//------------------------------------------------------------ 

//
// -- init TS sub decoding buffer
// -- return: < 0: fail
//
int ts2SecPesInit (void)
{
  tsd.mem_handle = packetMem_acquire (TS_SUBDEC_BUFFER);
  tsd.pid = -1;
  tsd.invalid = TSD_error;
  tsd.continuity_counter = -1;
  return tsd.mem_handle;
}




//
// -- free TS sub decoding buffer
//
void ts2SecPesFree (void)
{
  packetMem_free (tsd.mem_handle);
}






//
// -- add TS data 
// -- return: 0 = fail
//
int ts2SecPes_AddPacketStart (int pid, int cc, u_char *b, u_int len)
{

    // -- duplicate packet ?
    if ((pid == tsd.pid) && (cc == tsd.continuity_counter)) {
	    return 1;
    }

    tsd.invalid = TSD_no_error;
    tsd.pid = pid;
    tsd.continuity_counter = cc;
    packetMem_clear (tsd.mem_handle);
    if (! packetMem_add_data (tsd.mem_handle,b,len)) {
	tsd.invalid = TSD_mem_error;
	return 0;
    }
    
    return 1;
}


int ts2SecPes_AddPacketContinue (int pid, int cc, u_char *b, u_int len)
{

    // -- duplicate packet?  (this would be ok, due to ISO13818-1)
    if ((pid == tsd.pid) && (cc == tsd.continuity_counter)) {
	    return 1;
    }

    // -- pid change in stream? (without packet start)
    // -- This is currently not supported   $$$ TODO
    if ((tsd.invalid == TSD_no_error) && (pid != tsd.pid)) {
	tsd.invalid = TSD_pid_change;
    }

    // -- discontinuity error in packet ?
    if ((tsd.invalid == TSD_no_error) && (cc != (++tsd.continuity_counter%16))) {
	tsd.invalid = TSD_continuity_error;
    }

    if (tsd.invalid == TSD_no_error) {
	if (!packetMem_add_data (tsd.mem_handle,b,len) ) {
		tsd.invalid = TSD_mem_error;
	} else {
	  	return 1;
	}
    }

    return 0;
}



//------------------------------------------------------------ 


//
// -- TS  SECTION/PES  subdecoding
// -- check TS buffer and push data to sub decoding buffer
// -- on new packet start, output old packet data
//
void ts2SecPes_subdecode (u_char *b, int len)
{
    u_int  transport_error_indicator;		
    u_int  payload_unit_start_indicator;		
    u_int  pid;		
    u_int  transport_scrambling_control;		
    u_int  continuity_counter;		
    u_int  adaptation_field_control;

 


 transport_error_indicator	 = getBits (b, 0, 8, 1);
 payload_unit_start_indicator	 = getBits (b, 0, 9, 1);
 pid				 = getBits (b, 0,11,13);
 transport_scrambling_control	 = getBits (b, 0,24, 2);
 adaptation_field_control	 = getBits (b, 0,26, 2);
 continuity_counter		 = getBits (b, 0,28, 4);



 len -= 4;
 b   += 4;


 // -- skip adaptation field
 if (adaptation_field_control & 0x2) {
	int n;

	n = b[0] + 1;
	b += n;
	len -= n;
 }


 // -- push data to subdecoding collector buffer
 // -- on packet start, output collected data of buffer
 if (adaptation_field_control & 0x1) {

	// -- payload buffering/decoding

	// -- oerks, this we cannot use
	if (transport_scrambling_control || transport_error_indicator) {
		tsd.invalid = TSD_scrambled_error;
		return;
	}

	// -- if payload_start, check PES/SECTION
	if (payload_unit_start_indicator) {

		// -- output data of prev. collected packets
		if (packetMem_length (tsd.mem_handle) > 0) {
			ts2SecPes_Output_subdecode ();
		}

		// -- first buffer data
		ts2SecPes_AddPacketStart (pid, continuity_counter, b, (u_long)len);

	} else {

		// -- add more data
		ts2SecPes_AddPacketContinue (pid, continuity_counter, b, (u_long)len);

	}
 }

}



//
// -- TS  SECTION/PES  subdecoding  output
//
void ts2SecPes_Output_subdecode (void)
{

     indent (+1);
     out_NL (3);
     out_nl (3,"TS-Packet(s) sub-decoding:");
     out_nl (3,"==========================");


     if (tsd.invalid != TSD_no_error) {
   	char *s = "";

	switch (tsd.invalid) {
	   case TSD_error:  		s = "unknown packet error"; break;
	   case TSD_continuity_error:  	s = "packet continuity error"; break;
	   case TSD_scrambled_error:  	s = "packet scrambled or packet error"; break;
	   case TSD_pid_change:  	s = "PID change in TS stream"; break;
	   case TSD_mem_error:  	s = "subdecoding buffer (allocation) error"; break;
	}
     	out_nl (3,"Packet cannot be sub-decoded: %s",s);
        out_NL (3);

     } else {
   	u_char *b;
	u_int  len;

	b   = packetMem_buffer_start (tsd.mem_handle);
	len = (u_int) packetMem_length (tsd.mem_handle);

	if (b && len) {

	    // -- PES or SECTION
	    if (b[0]==0x00 && b[1]==0x00 && b[2]==0x01 && b[3]>=0xBC) {

		out_nl (3,"TS contains PES stream...");
	    	indent (+1);
		decodePES_buf (b, len, tsd.pid);
	    	indent (-1);

	    } else {
		int pointer = b[0]+1;
		b += pointer;

		out_nl (3,"TS contains Section...");
	    	indent (+1);
		decodeSections_buf (b, len-pointer, tsd.pid);
	    	indent (-1);
	    }
	    out_NL (3);
	    out_NL (3);

	}


     }

     indent (-1);
}





