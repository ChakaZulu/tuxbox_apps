/*
$Id: fe_info.c,v 1.1 2004/03/21 00:38:45 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de  (rasc)


 -- FrontProcessor Info
 --  DVB-API 




$Log: fe_info.c,v $
Revision 1.1  2004/03/21 00:38:45  rasc
Query FrontEnd Info  (option: -s feinfo)




*/


#include <fcntl.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>


#include "dvbsnoop.h"
#include "fe_info.h"
#include "misc/cmdline.h"
#include "misc/output.h"

#include "dvb_api.h"
#include "dmx_error.h"







#if DVB_API_VERSION == 1

  // -- not supported in DVB-API 1
  
  int  do_FE_Info (OPTION *opt)
  {
	 fprintf (stderr,"FE_info function not supported in DVB-API 1\n");
	 return 1;
  }


#else





static int read_FEInfo(int f, struct dvb_frontend_info *fi);



int  do_FE_Info (OPTION *opt)

{
  int        fd_fe = 0;
  int        err;
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


   err = read_FEInfo(fd_fe, &fi);
   if (err) return 1;
 

   fi.name[127] = '\0';		// be save...
   out_nl (1,"Name: \"%s\"",fi.name);


   {
     char   *s;
     char   *sf;
     u_long d;


     s  = "";
     sf = "kHz";
     d  = 1;
     switch (fi.type) {
	case FE_QPSK:   s = "QPSK (DVB-S)"; sf = "MHz"; d = 1000L;  break;
	case FE_QAM:	s = "QAM (DVB-C)"; break;
	case FE_OFDM:	s = "OFDM (DVB-T)"; break;
	default:	s = "unkonwn"; break;
     }
     out_nl (1,"Frontend-type: %s", s);


     out_nl (1,"Frequency (min):     %d.%d %s", fi.frequency_min/d, fi.frequency_min%d, sf);
     out_nl (1,"Frequency (max):     %d.%d %s", fi.frequency_max/d, fi.frequency_max%d, sf);

     out_nl (1,"Frequency stepsize:  %d.%d %s", fi.frequency_stepsize/d, fi.frequency_stepsize%d, sf);
     out_nl (1,"Frequency tolerance: %d", fi.frequency_tolerance);

   }


   out_nl (1,"Symbol rate (min) : %d", fi.symbol_rate_min);
   out_nl (1,"Symbol rate (max) : %d", fi.symbol_rate_max);
   out_nl (1,"Symbol rate tolerance : %d ppm", fi.symbol_rate_tolerance);
   

   out_nl (1,"Notifier delay: %d ms", fi.notifier_delay);



   out_nl (1,"Frontend capabilities:");
   indent (+1);

   if (fi.caps == FE_IS_STUPID)  		out_nl (1,"stupid FE");
   if (fi.caps &  FE_CAN_INVERSION_AUTO)  	out_nl (1,"auto inversion");
   if (fi.caps &  FE_CAN_FEC_1_2)  		out_nl (1,"FEC 1/2");
   if (fi.caps &  FE_CAN_FEC_2_3)  		out_nl (1,"FEC 2/3");
   if (fi.caps &  FE_CAN_FEC_3_4)  		out_nl (1,"FEC 3/4");
   if (fi.caps &  FE_CAN_FEC_4_5)  		out_nl (1,"FEC 4/5");
   if (fi.caps &  FE_CAN_FEC_5_6)  		out_nl (1,"FEC 5/6");
   if (fi.caps &  FE_CAN_FEC_6_7)  		out_nl (1,"FEC 6/7");
   if (fi.caps &  FE_CAN_FEC_7_8)  		out_nl (1,"FEC 7/8");
   if (fi.caps &  FE_CAN_FEC_AUTO)	  	out_nl (1,"FEC AUTO");
   if (fi.caps &  FE_CAN_QPSK)	  		out_nl (1,"QPSK");
   if (fi.caps &  FE_CAN_QAM_16)	  	out_nl (1,"QAM 16");
   if (fi.caps &  FE_CAN_QAM_32)	  	out_nl (1,"QAM 32");
   if (fi.caps &  FE_CAN_QAM_64)	  	out_nl (1,"QAM 64");
   if (fi.caps &  FE_CAN_QAM_128)	  	out_nl (1,"QAM 128");
   if (fi.caps &  FE_CAN_QAM_256)	  	out_nl (1,"QAM 256");
   if (fi.caps &  FE_CAN_QAM_AUTO)	  	out_nl (1,"QAM AUTO");
   if (fi.caps &  FE_CAN_TRANSMISSION_MODE_AUTO)	out_nl (1,"auto transmission mode");
   if (fi.caps &  FE_CAN_BANDWIDTH_AUTO)	out_nl (1,"auto bandwidth");
   if (fi.caps &  FE_CAN_GUARD_INTERVAL_AUTO)	out_nl (1,"auto guard interval");
   if (fi.caps &  FE_CAN_HIERARCHY_AUTO)	out_nl (1,"auto hierarchy");
   if (fi.caps &  FE_CAN_CLEAN_SETUP)		out_nl (1,"clean setup");

   indent (-1);


  close(fd_fe);
  return 0;
}





/*
 * -- read frontend info
 */

static int read_FEInfo(int f, struct dvb_frontend_info *fi)
{
  int err = 0;


  err = ioctl(f, FE_GET_INFO, fi);
  if (err < 0) {
	IO_error ("frontend ioctl");
  	return -1;
  }
  return 0;
}




#endif  // DVB-API Check








//
// read_FE_get_frontend...
//
// int ioctl(int fd, int request = FE GET FRONTEND, struct dvb frontend parameters *p);
// int ioctl(int fd, int request = QPSK GET EVENT, struct dvb frontend event *ev);

