/*
$Id: rcinput.h,v 1.7 2001/10/29 16:49:00 field Exp $

 Module  RemoteControle Handling

History:
 $Log: rcinput.h,v $
 Revision 1.7  2001/10/29 16:49:00  field
 Kleinere Bug-Fixes (key-input usw.)

 Revision 1.6  2001/10/11 21:00:56  rasc
 clearbuffer() fuer RC-Input bei Start,
 Klassen etwas erweitert...

 Revision 1.5  2001/10/01 20:41:08  McClean
 plugin interface for games - beta but nice.. :)

 Revision 1.4  2001/09/23 21:34:07  rasc
 - LIFObuffer Module, pushbackKey fuer RCInput,
 - In einige Helper und widget-Module eingebracht
   ==> harmonischeres Menuehandling
 - Infoviewer Breite fuer Channelsdiplay angepasst (>1000 Channels)


*/



#ifndef __MOD_rcinput__
#define __MOD_rcinput__

#include <dbox/fp.h>
#include <stdio.h>
#include <asm/types.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>

#include <utime.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "pthread.h"
#include "semaphore.h"

#include "ringbuffer.h"
#include "lifobuffer.h"

#include <string>

using namespace std;

class CRCInput
{
	private:

		int             fd;
		CRingBuffer     ringbuffer;
		CLIFOBuffer     LIFObuffer;
		pthread_t       thrInput;
		sem_t           waitforkey;

		struct timeval  tv_prev;

		__u16           prevrccode;

		int translate(int code);
		int getKeyInt();	//don't use!
		void start();

		
		static void * InputThread (void *arg);

	public:
		//rc-code definitions
		enum
		{
			RC_standby=0x10, RC_home=0x1F, RC_setup=0x18, RC_0=0x0, RC_1=0x1,
			RC_2=0x2, RC_3=0x3, RC_4=0x4, RC_5=0x5, RC_6=0x6, RC_7=0x7,
			RC_8=0x8, RC_9=0x9, RC_blue=0x14, RC_yellow=0x12, RC_green=0x11,
			RC_red=0x13, RC_page_up=0x54, RC_page_down=0x53, RC_up=0xC, RC_down=0xD,
			RC_left=0xB, RC_right=0xA, RC_ok=0xE, RC_plus=0x15, RC_minus=0x16,
			RC_spkr=0xF, RC_help=0x17, RC_top_left=27, RC_top_right=28, RC_bottom_left=29, RC_bottom_right=30,
			RC_timeout=-1, RC_nokey=-2
		};
		
		int getFileHandle(){return fd;}; //only used for plugins (games) !!
		void stopInput();
		void restartInput();

        int repeat_block;
		CRCInput();      //constructor - opens rc-device and starts needed threads
		~CRCInput();     //destructor - closes rc-device

		
		int  getKey(int Timeout=-1);     //get key from the input-device
		int  pushbackKey (int key);      // push key back in buffer (like ungetc)
		void clear (void);
		static string getKeyName(int);
};

#endif



