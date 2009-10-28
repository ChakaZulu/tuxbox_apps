/******************************************************************************
 *	rcsim - rcsim.c
 *                                                                            
 *	simulates the remote control, sends the requested key
 *                                                                            
 *	(c) 2003 Carsten Juttner (carjay@gmx.net)
 *	(c) 2009 Stefan Seyfried, add code to use the neutrino socket instead
 *			of the input subsystem for dreambox / tripledragon
 *  									      
 * 	This program is free software; you can redistribute it and/or modify
 * 	it under the terms of the GNU General Public License as published by
 * 	The Free Software Foundation; either version 2 of the License, or
 * 	(at your option) any later version.
 *
 * 	This program is distributed in the hope that it will be useful,
 * 	but WITHOUT ANY WARRANTY; without even the implied warranty of
 * 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * 	GNU General Public License for more details.
 *
 * 	You should have received a copy of the GNU General Public License
 * 	along with this program; if not, write to the Free Software
 * 	Foundation, 51 Franklin Street, Fifth Floor Boston, MA 02110-1301, USA.
 *
 ******************************************************************************
 * $Id: rcsim.c,v 1.7 2009/10/28 19:50:56 seife Exp $
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <error.h>

/* if you want use HAVE_XX_HARDWARE, better include config.h :-) */
#include "config.h"

#ifdef HAVE_DBOX_HARDWARE
#define EVENTDEV "/dev/input/event0"
#else
/* dreambox and tripledragon do not use a "normal" input device, so we cannot
   (ab-)use the event repeating function of it. use the neutrino socket instead. */
#include <sys/socket.h>
#include <sys/un.h>
#define NEUTRINO_SOCKET "/tmp/neutrino.sock"

/* those structs / values are stolen from libeventserver */
struct eventHead
{
	unsigned int eventID;
	unsigned int initiatorID;
	unsigned int dataSize;
};

enum initiators
{
	INITID_CONTROLD,
	INITID_SECTIONSD,
	INITID_ZAPIT,
	INITID_TIMERD,
	INITID_HTTPD,
	INITID_NEUTRINO,
	INITID_GENERIC_INPUT_EVENT_PROVIDER
};
#endif

/* compatibility stuff */

#ifndef KEY_TOPLEFT
#define KEY_TOPLEFT	0x1a2
#endif

#ifndef KEY_TOPRIGHT
#define KEY_TOPRIGHT	0x1a3
#endif

#ifndef KEY_BOTTOMLEFT
#define KEY_BOTTOMLEFT	0x1a4
#endif

#ifndef KEY_BOTTOMRIGHT
#define KEY_BOTTOMRIGHT	0x1a5
#endif

enum {	// not defined in input.h but used like that, at least in 2.4.22
	KEY_RELEASED = 0,
	KEY_PRESSED,
	KEY_AUTOREPEAT
};

struct key{
	char *name;
	unsigned long code;
};

static const struct key keyname[] = {
	{"KEY_0", 		KEY_0},
	{"KEY_1", 		KEY_1},
	{"KEY_2", 		KEY_2},
	{"KEY_3", 		KEY_3},
	{"KEY_4", 		KEY_4},
	{"KEY_5", 		KEY_5},
	{"KEY_6", 		KEY_6},
	{"KEY_7", 		KEY_7},
	{"KEY_8", 		KEY_8},
	{"KEY_9", 		KEY_9},
	{"KEY_RIGHT",		KEY_RIGHT},
	{"KEY_LEFT",		KEY_LEFT},
	{"KEY_UP",		KEY_UP},
	{"KEY_DOWN",		KEY_DOWN},
	{"KEY_OK",		KEY_OK},
	{"KEY_MUTE",		KEY_MUTE},
	{"KEY_POWER",		KEY_POWER},
	{"KEY_GREEN",		KEY_GREEN},
	{"KEY_YELLOW",		KEY_YELLOW},
	{"KEY_RED",		KEY_RED},
	{"KEY_BLUE",		KEY_BLUE},
	{"KEY_VOLUMEUP",	KEY_VOLUMEUP},
	{"KEY_VOLUMEDOWN",	KEY_VOLUMEDOWN},
	{"KEY_HELP",		KEY_HELP},
	{"KEY_SETUP",		KEY_SETUP},
	{"KEY_TOPLEFT",		KEY_TOPLEFT},
	{"KEY_TOPRIGHT", 	KEY_TOPRIGHT},
	{"KEY_BOTTOMLEFT", 	KEY_BOTTOMLEFT},
	{"KEY_BOTTOMRIGHT", 	KEY_BOTTOMRIGHT},
	{"KEY_HOME",		KEY_HOME},
	{"KEY_PAGEDOWN",	KEY_PAGEDOWN},
	{"KEY_PAGEUP",		KEY_PAGEUP}
};

void usage(char *n){
	unsigned int keynum = sizeof(keyname)/sizeof(struct key);
	unsigned int i;
	printf ("rcsim v1.1\nUsage: %s <keyname> [<time>] [<repeat>]\n"
		"       <keyname> is an excerpt of the 'KEY_FOO'-names in <linux/input.h>,\n"
		"             only the keys on the dbox2-remote control are supported\n"
		"       <time> is how long a code is repeatedly sent,\n"
		"              unit is seconds, default is 0 = sent only once\n"
		"       <repeat> what time is waited until a new code is sent\n"
		"                (if <time> is greater than 0), unit is milliseconds,\n"
		"		 default is 500\n\n"
		"       Example:\n"
		"                 %s KEY_1\n"
		"                        ; KEY_1 sent once\n"
		"                 %s KEY_OK 2 250\n"
		"                        ; KEY_OK sent every 250ms for 2 seconds\n\n"
		"       Keys:\n",n,n,n);
	for (i=0;i<keynum;){
		printf ("                 %-20s",keyname[i++].name);
		if (i<keynum)
			printf ("%s\n",keyname[i++].name);
		else
			printf ("\n");
	}
}

/* we could also use the neutrino socket on the dbox, but this needs more testing.
   so leave it as is for now */
#ifdef HAVE_DBOX_HARDWARE
int push(int ev, unsigned int code, unsigned int value)
{
	struct input_event iev;
	iev.type=EV_KEY;
	iev.code=code;
	iev.value=value;
	return write (ev,&iev,sizeof(iev));
}
#else
int push(int ev, unsigned int code, unsigned int value)
{
	struct eventHead eh;
	struct sockaddr_un servaddr;
	int clilen, fd;
	const char *errmsg;

	/* ev is unused - stupid compiler... */
	fd = ev;

	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, NEUTRINO_SOCKET);
	clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		perror("cannot open socket " NEUTRINO_SOCKET);
		return fd;
	}

	if (connect(fd, (struct sockaddr*) &servaddr, clilen) < 0)
	{
		errmsg = "connect " NEUTRINO_SOCKET;
		goto error;
	}

	eh.initiatorID = INITID_GENERIC_INPUT_EVENT_PROVIDER;
	eh.eventID = 0; // data field
	eh.dataSize = sizeof(int);
	if (value == KEY_AUTOREPEAT)
		code |= 0x0400; // neutrino:CRCInput::RC_repeat
	if (value == KEY_RELEASED)
		code |= 0x0800; // neutrino:CRCInput::RC_release

	if (write(fd, &eh, sizeof(eh)) < 0)
	{
		errmsg = "write() eventHead";
		goto error;
	}

	if (write(fd, &code, sizeof(code)) < 0)
	{
		errmsg = "write() event";
		goto error;
	}
	close(fd);
	return 0;

 error:
	perror(errmsg);
	close(fd);
	return -1;
}
#endif

int main (int argc, char **argv){
	int evd;
	unsigned long sendcode=KEY_0;
	unsigned int keys = sizeof(keyname)/sizeof(struct key);
	unsigned long time=0;
	unsigned long reptime=500;
	unsigned int offset;

	if (argc<2||argc>4){
		usage(argv[0]);
		return 1;
	}

	if (argc==2)
		if (!strncmp(argv[1],"--help",6)||!strncmp(argv[1],"-h",2)){
			usage(argv[0]);
			return 1;
		}

	for (offset=0;offset<keys;offset++){
		if (!strcmp(argv[1],keyname[offset].name)){
			sendcode = keyname[offset].code;
			break;
		}
	}

	if (offset==keys){
		printf ("keyname '%s' not found in list\n",argv[1]);
		return 1;
	}

	if (argc==4){
		reptime=atol (argv[3]);
	}

	if (argc>=3){
		time=(atol (argv[2])*1000)/reptime;
	}

#ifdef HAVE_DBOX_HARDWARE
	evd=open (EVENTDEV,O_RDWR);
	if (evd<0){
		perror ("opening event0 failed");
		return 1;
	}
#else
	evd = -1; // close(-1) does not harm... ;)
#endif
	printf ("sending key %s for %ld seconds\n",keyname[offset].name,(reptime*time)/1000);
	if (push (evd,sendcode,KEY_PRESSED)<0){
		perror ("writing 'key_pressed' event failed");
		close (evd);
		return 1;
	}
	while (time--){
		usleep(reptime*1000);
		if (push (evd,sendcode,KEY_AUTOREPEAT)<0){
			perror ("writing 'key_autorepeat' event failed");
			close (evd);
			return 1;
		}
	}
	if (push (evd,sendcode,KEY_RELEASED)<0){
		perror ("writing 'key_released' event failed");
		close (evd);
		return 1;
	}
	close (evd);
	return 0;
}
