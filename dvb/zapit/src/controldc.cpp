/*
        This program is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 2 of the License, or
        (at your option) any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program; if not, write to the Free Software
        Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// Copyright 2006, 2009 Bengt Martensson, "Barf"
// http://www.bengt-martensson.de/dbox2

/*
  This program has (at least) two uses:

  1. To experiment with and to debug controld interactivelly,

  2. To enable the user to change settings like video output format
     and volume in scripts, without "going behind the back of
     controld" (this would be the case if using the switch command).

  Its usage is believed to be obvious from the usage message.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <controldclient/controldclient.h>

#define CHECKARG(cmdname) if (arg == NO_ARG) {			\
    fprintf(stderr, "%s: This command takes an argument\n", cmdname);	\
    exit(1);								\
} 

#define GETCOMMAND_VOLUME(cmdname, cmd) if (!strcasecmp(argv[1], cmdname)) {\
  result = the_controld.cmd(vt); \
} else

#define GETCOMMAND(cmdname, cmd) if (!strcasecmp(argv[1], cmdname)) {\
  result = the_controld.cmd(); \
} else

#define SETCOMMAND(cmdname, cmd) if (!strcasecmp(argv[1], cmdname)) {\
    CHECKARG(cmdname);						     \
  the_controld.cmd(arg); \
} else

#define SETCOMMAND_VOLUME(cmdname, cmd) if (!strcasecmp(argv[1], cmdname)) {\
    CHECKARG(cmdname);							\
  the_controld.cmd(arg, vt); \
} else

#define DOCOMMAND_VOLUME(cmdname, cmd) if (!strcasecmp(argv[1], cmdname)) {\
  the_controld.cmd(vt); \
} else

#define DOCOMMAND(cmdname, cmd) if (!strcasecmp(argv[1], cmdname)) {\
  the_controld.cmd(); \
} else

#define NOTIMPLEMENTED(cmdname, cmd) if (!strcasecmp(argv[1], cmdname)) {\
  fprintf(stderr, "%s is not implemented, sorry\n", cmdname); \
  exit(1); \
} else

static const char NO_ARG = 127;
static const char NO_RESULT = 127;

void usage(char *);

int
main(int argc, char *argv[]) {
  CControldClient the_controld;
  CControld::volume_type vt = CControld::TYPE_UNKNOWN;
  char *myname = argv[0];
  char arg = NO_ARG;
  char result = NO_RESULT;

  if ((argc >= 2) && (argv[1][0] == '-')) {
    switch (argv[1][1]) {
#ifdef HAVE_DBOX_HARDWARE
    case 'a':
      vt = CControld::TYPE_AVS;
      break;
#endif
    case 'o':
      vt = CControld::TYPE_OST;
      break;
#ifdef ENABLE_LIRC
    case 'l':
      vt = CControld::TYPE_LIRC;
      break;
#endif
    default:
      vt = CControld::TYPE_UNKNOWN;
    }
    argc--;
    argv++;
  }

  if (argc < 2) {
    usage(myname);
    return 1;
  }

  if (argc >= 3) {
    arg = char(atoi(argv[2]));
  }
    
  GETCOMMAND_VOLUME("getVolume", getVolume)
  SETCOMMAND_VOLUME("setVolume", setVolume)
  GETCOMMAND("getVideoOutput", getVideoOutput)
  SETCOMMAND("setVideoOutput", setVideoOutput)
  SETCOMMAND_VOLUME("setMute", setMute)
  GETCOMMAND_VOLUME("getMute", getMute)
  DOCOMMAND_VOLUME("Mute", Mute)
  DOCOMMAND_VOLUME("UnMute", UnMute)
  SETCOMMAND("setVideoFormat", setVideoFormat)
  GETCOMMAND("getVideoFormat", getVideoFormat)
  GETCOMMAND("getAspectRatio", getAspectRatio)
  SETCOMMAND("setVCROutput", setVCROutput)
  GETCOMMAND("getVCROutput", getVCROutput)
  NOTIMPLEMENTED("setBoxType", setBoxType)
  GETCOMMAND("getBoxType", getBoxType)
  SETCOMMAND("setScartMode", setScartMode)
  GETCOMMAND("getScartMode", getScartMode)
  SETCOMMAND("videoPowerDown", videoPowerDown)
  GETCOMMAND("getVideoPowerDown", getVideoPowerDown)
  SETCOMMAND("setRGBCsync", setRGBCsync)
  GETCOMMAND("getRGBCsync", getRGBCsync)
  DOCOMMAND("shutdown", shutdown)
  DOCOMMAND("saveSettings", saveSettings)
  NOTIMPLEMENTED("registerEvent", registerEvent)
  NOTIMPLEMENTED("unRegisterEvent", unRegisterEvent)
  
    usage(myname);

  if (result != NO_RESULT) 
    printf("%d\n", (int) result);
}

void usage(char *myname) {
  fprintf(stderr, 
	  "Usage: %s [-VOLTYPE] COMMAND [ARGUMENT]\n"
	  "where VOLTYPE = "
#ifdef HAVE_DBOX_HARDWARE
	  " -a (avs),"
#endif
	  " -o (ost)"
#ifdef ENABLE_LIRC
	  ", or -l (lirc),"
#endif
	  "\n"
	  "and COMMAND is one of\n"
	  "getVolume\n"
	  "setVolume  VOLUME\n"
	  "getVideoOutput\n"
	  "setVideoOutput [0,1,2,3,4] (cvbs, rgb+cvbs, svideo, yuv-vbs, yuv+cvbs)\n"
	  "setMute [0,1]\n"
	  "getMute\n"
	  "Mute\n"
	  "UnMute\n"
	  "setVideoFormat [0,1,2,3] (auto, 16:9, 4:3, 4:3-PS)\n"
	  "getVideoFormat\n"
	  "getAspectRatio\n"
	  "setVCROutput [0,1] (cvbs, svideo)\n"
	  "getVCROutput\n"
	  "setBoxType [0,1,2,3,4,5]\n"
	  "getBoxType\n"
	  "setScartMode [0,1]\n"
	  "getScartMode\n"
	  "videoPowerDown [0,1]\n"
	  "getVideoPowerDown\n"
	  "setRGBCsync [0,...,31]\n"
	  "getRGBCsync\n"
	  "shutdown\n"
	  "saveSettings.\n",
	  myname);
}
