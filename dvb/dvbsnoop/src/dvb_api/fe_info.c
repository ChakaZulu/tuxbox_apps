/*
$Id: fe_info.c,v 1.4 2004/03/21 18:02:45 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de  (rasc)


 -- FrontProcessor Info
 --  DVB-API 




$Log: fe_info.c,v $
Revision 1.4  2004/03/21 18:02:45  rasc
corrections

Revision 1.3  2004/03/21 13:20:07  rasc
more -feinfo, some restructs on FE code

Revision 1.2  2004/03/21 00:40:40  rasc
no message

Revision 1.1  2004/03/21 00:38:45  rasc
Query FrontEnd Info  (option: -s feinfo)




*/


#include <stdint.h>
#include <sys/time.h>
#include <errno.h>


#include "dvbsnoop.h"
#include "fe_info.h"
#include "fe_misc.h"
#include "misc/cmdline.h"
#include "misc/output.h"

#include "dmx_error.h"
#include "dvb_api.h"







#if DVB_API_VERSION == 1

  // -- not supported in DVB-API 1
  
  int  do_FE_Info (OPTION *opt)
  {
	 fprintf (stderr,"FE_info function not supported in DVB-API 1\n");
	 return 1;
  }


#else




int  do_FE_Info (OPTION *opt)

{
  int        fd_fe = 0;
  int        err;
  u_long     d;
  struct dvb_frontend_info fi;




  if (opt->inpPidFile) {
	fprintf (stderr,"Error: FileMode not possible...\n");
	return -1;
  } 


  if((fd_fe = open(opt->devFE,O_RDONLY)) < 0){
     IO_error(opt->devFE);
     return -1;
  }



   indent (0);
   out_NL (2);
   out_nl (2,"---------------------------------------------------------");
   out_nl (2,"FrontEnd Info...");
   out_nl (2,"---------------------------------------------------------");
   out_NL (2);


   out_nl (3,"Basic apabilities:");
   indent(+1);

   err = read_FEInfo(fd_fe, &fi);
   if (err) return 1;
 

   fi.name[127] = '\0';		// be save...
   out_nl (3,"Name: \"%s\"",fi.name);


   {
     char   *s;
     char   *sf;

     s  = "";
     sf = "";
     switch (fi.type) {
	case FE_QPSK:   s = "QPSK (DVB-S)"; sf = "MHz";  break;
	case FE_QAM:	s = "QAM (DVB-C)";  sf = "kHz";  break;
	case FE_OFDM:	s = "OFDM (DVB-T)"; sf = "kHz";  break;
	default:	s = "unkonwn"; break;
     }
     out_nl (3,"Frontend-type:       %s", s);

     out_nl (3,"Frequency (min):     %d.%03d %s", fi.frequency_min / 1000, fi.frequency_min % 1000, sf);
     out_nl (3,"Frequency (max):     %d.%03d %s", fi.frequency_max / 1000, fi.frequency_max % 1000, sf);
     out_nl (3,"Frequency stepsiz:   %d.%03d %s", fi.frequency_stepsize / 1000, fi.frequency_stepsize % 1000, sf);
     out_nl (3,"Frequency tolerance: %d.%03d %s", fi.frequency_tolerance/ 1000, fi.frequency_tolerance% 1000, sf);

   }


   d = 1000000L;
   out_nl (3,"Symbol rate (min):     %d.%06d MSym/s", fi.symbol_rate_min / d, fi.symbol_rate_min % d);
   out_nl (3,"Symbol rate (max):     %d.%06d MSym/s", fi.symbol_rate_max / d, fi.symbol_rate_max % d);
   out_nl (3,"Symbol rate tolerance: %d ppm", fi.symbol_rate_tolerance);
   

   out_nl (3,"Notifier delay: %d ms", fi.notifier_delay);



   out_nl (3,"Frontend capabilities:");
      indent (+1);
      if (fi.caps == FE_IS_STUPID)  		out_nl (3,"stupid FE");
      if (fi.caps &  FE_CAN_INVERSION_AUTO)  	out_nl (3,"auto inversion");
      if (fi.caps &  FE_CAN_FEC_1_2)  		out_nl (3,"FEC 1/2");
      if (fi.caps &  FE_CAN_FEC_2_3)  		out_nl (3,"FEC 2/3");
      if (fi.caps &  FE_CAN_FEC_3_4)  		out_nl (3,"FEC 3/4");
      if (fi.caps &  FE_CAN_FEC_4_5)  		out_nl (3,"FEC 4/5");
      if (fi.caps &  FE_CAN_FEC_5_6)  		out_nl (3,"FEC 5/6");
      if (fi.caps &  FE_CAN_FEC_6_7)  		out_nl (3,"FEC 6/7");
      if (fi.caps &  FE_CAN_FEC_7_8)  		out_nl (3,"FEC 7/8");
      if (fi.caps &  FE_CAN_FEC_AUTO)	  	out_nl (3,"FEC AUTO");
      if (fi.caps &  FE_CAN_QPSK)	  	out_nl (3,"QPSK");
      if (fi.caps &  FE_CAN_QAM_16)	  	out_nl (3,"QAM 16");
      if (fi.caps &  FE_CAN_QAM_32)	  	out_nl (3,"QAM 32");
      if (fi.caps &  FE_CAN_QAM_64)	  	out_nl (3,"QAM 64");
      if (fi.caps &  FE_CAN_QAM_128)	  	out_nl (3,"QAM 128");
      if (fi.caps &  FE_CAN_QAM_256)	  	out_nl (3,"QAM 256");
      if (fi.caps &  FE_CAN_QAM_AUTO)	  	out_nl (3,"QAM AUTO");
      if (fi.caps &  FE_CAN_TRANSMISSION_MODE_AUTO)	out_nl (3,"auto transmission mode");
      if (fi.caps &  FE_CAN_BANDWIDTH_AUTO)		out_nl (3,"auto bandwidth");
      if (fi.caps &  FE_CAN_GUARD_INTERVAL_AUTO)	out_nl (3,"auto guard interval");
      if (fi.caps &  FE_CAN_HIERARCHY_AUTO)	out_nl (3,"auto hierarchy");
      if (fi.caps &  FE_CAN_CLEAN_SETUP)	out_nl (3,"clean setup");
      indent (-1);

   indent (-1);






  close(fd_fe);
  return 0;
}





#endif  // DVB-API Check





