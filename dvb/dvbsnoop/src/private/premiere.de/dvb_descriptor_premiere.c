/*
$Id: dvb_descriptor_premiere.c,v 1.1 2004/11/03 21:01:02 rasc Exp $ 


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)



 -- Private DVB Descriptors  Premiere.de



$Log: dvb_descriptor_premiere.c,v $
Revision 1.1  2004/11/03 21:01:02  rasc
 - New: "premiere.de" private tables and descriptors (tnx to Peter.Pavlov, Premiere)
 - New: cmd option "-privateprovider <provider name>"
 - New: Private provider sections and descriptors decoding
 - Changed: complete restructuring of private descriptors and sections


*/


#include "dvbsnoop.h"
#include "dvb_descriptor_premiere.h"
#include "strings/dvb_str.h"
#include "misc/hexprint.h"
#include "misc/output.h"




/*
 *
 * Private DVB descriptors
 * User Space: Premiere privat (www.premiere.de)
 *
 * basic code snipset provided by  Peter.Pavlov, Premiere 2004-10
 *  
 */





/*
   0xF0  Content Order Descriptor
   PTS 60 04 101    v 1.0.1 17.06.2004
*/

void descriptor_PRIVATE_PremiereDE_ContentOrder (u_char *b)
{
 int  tag, len;

  tag		 = b[0];
  len       	 = b[1];
 
  out_nl (4,"--> Premiere Content Order descriptor ");

  b+=2;
  out_SB_NL (5,"Order_number_length: ", *b);
  print_text_468A (4, "Order_number: ", b+1, *b);
 
  b += *b + 1;
  out_SB_NL (5,"Order_price_length: ", *b);
  print_text_468A (4, "Order_price: ", b+1, *b);
 
  b += *b + 1;
  out_SB_NL (5,"Order_phone_number_length: ", *b);
  print_text_468A (4, "Order_phone_number: ", b+1, *b);
 
  b += *b + 1;
  out_SB_NL (5,"SMS_order_information_length: ", *b);
  print_text_468A (4, "SMS_order_information: ", b+1, *b);
 
  b += *b + 1;
  out_SB_NL (5,"URL_order_information_length: ", *b);
  print_text_468A (4, "URL_order_information: ", b+1, *b);

}
 



 
/*
 	Premiere privat
   0xF1  Parent Information Descriptor
   PTS 60 04 101    v 1.0.1 17.06.2004
*/
 
void descriptor_PRIVATE_PremiereDE_ParentalInformation (u_char *b)
{
  int  tag, len;
 
  tag		 = b[0];
  len       	 = b[1];
 
  out_nl (4,"--> Premiere Parental Information descriptor ");

  b += 2;
  out_SB_NL (5,"rating: ", *b);
  out_nl    (4,"Controll_time_t1: %02x:%02x:%02x", b[1], b[2], b[3]);
  out_nl    (4,"Controll_time_t2: %02x:%02x:%02x", b[4], b[5], b[6]);
 
  b += 7;
  out_SB_NL (5,"Parental_information_length: ", *b);
  print_text_468A (4, "Parental_information: ", b+1, *b);
 
}
 


/*
 	Premiere privat
   0xF2  Content Transmition Descriptor
   PTS 60 04 101    v 1.0.1 17.06.2004
*/

#if 0
 
// Adaptierte Routine 

void descriptor_PRIVATE_PremiereDE_ContentTransmition (u_char *b)
{
  int  len, str_tim_len;
  u_int time_MJD, time_UTC;

 
  // tag	 = b[0];
  len       	 = b[1];


  out_nl (4,"--> Premiere Content Transmition descriptor ");

  // $$$ TODO  this descriptor works wrong!!
 out_nl (4,"DEBUG: Premiere original...");
  out_nl (1,"--> TODO: Decoding currently wrong?????? ");



  outBit_Sx_NL  (4,"transport_stream_ID: ",	b,  16, 16);
  outBit_S2x_NL (4,"original_network_id: ",	b,  32, 16,
			(char *(*)(u_long)) dvbstrOriginalNetwork_ID);
  outBit_S2Tx_NL(4,"service_ID: ",		b,  48, 16,
			" --> refers to PMT program_number"); 
 
 
  b += 8;
  len -= 6;

  out_NL(4);
  while (len>0) {

	out (4,"Start_date: ");
  	time_MJD = getBits (b, 0, 0, 16);
	print_time_mjd (4, time_MJD);
	out_NL (4);
 	b+=2;

  	str_tim_len = outBit_Sx_NL (5,"start_time_loop_length: ", b,  0, 8);
	b++;

 	len -= 3;

	indent (+1);
 	for(; str_tim_len>0; str_tim_len-=3) {
  		// originally provides : time_UTC = getBits (b, 0, 8, 24);
		// this may be wrong!
  		out (4,"Start_time: ");
  		time_UTC = getBits (b, 0, 0, 24);
  		print_time_utc (4, time_UTC);
  		out_NL (4);

		b   += 3;
		len -= 3;
		printf ("DEBUG:  len = %d\n",len);   // $$$ DEBUG TODO  
 	}
	out_NL (4);
	indent (-1);
  }

}


#else

// Original Routine von Premiere

void descriptor_PRIVATE_PremiereDE_ContentTransmition (u_char *b)
{
 int  tag, len, str_tim_len;
 u_int transport_stream_id, original_network_id, service_id;
 u_int time_MJD, time_UTC;

 tag		 = b[0];
 len       	 = b[1];

 out_nl (4,"--> Premiere Content Transmition descriptor ");
 out_nl (4,"DEBUG: Premiere original...");

 transport_stream_id		 = getBits (b, 0, 16, 16);
 original_network_id		 = getBits (b, 0, 32, 16);
 service_id			 = getBits (b, 0, 48, 16);

 out_SW_NL  (4,"Transport_stream_ID: ",transport_stream_id);
 out_S2W_NL (4,"Original_network_ID: ",original_network_id,
	dvbstrOriginalNetwork_ID(original_network_id));
 out_S2W_NL (4,"Service_ID: ",service_id,
      " --> refers to PMT program_number");

 b += 8;
 len -= 6;
 while(len>0)
 	{
 	time_MJD = getBits (b, 0, 0, 16);
	b+=2;
	len -= *b+3;
 	out_SB_NL (5,"start_time_length: ", *b);
	for(str_tim_len=*b; str_tim_len>0; str_tim_len-=3, b+=3)
 		{
 		time_UTC = getBits (b, 0, 8, 24);
 		out (4,"Start_time: ");
 		print_time40 (4, time_MJD, time_UTC);
 		out_NL (4);
		}
	b++;
 	out_NL (4);
	}
}

#endif

