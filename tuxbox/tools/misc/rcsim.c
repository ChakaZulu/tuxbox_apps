/******************************************************************************
 *	rcsim - rcsim.c
 *                                                                            
 *	simulates the remote control, sends the requested key
 *                                                                            
 *	(c) 2003 Carsten Juttner (carjay@gmx.net)
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
 * 	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ******************************************************************************
 * $Id: rcsim.c,v 1.5 2005/01/19 02:18:09 carjay Exp $
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

#define EVENTDEV "/dev/input/event0"

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

int send (int ev, unsigned int code, unsigned int value){
	struct input_event iev;
	iev.type=EV_KEY;
	iev.code=code;
	iev.value=value;
	return write (ev,&iev,sizeof(iev));
}

int main (int argc, char **argv){
	int evd;
	unsigned long sendcode=KEY_0;
	unsigned int keys = sizeof(keyname)/sizeof(struct key);
	unsigned long time=0;
	unsigned long reptime=500;
	int offset;

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

	evd=open (EVENTDEV,O_RDWR);
	if (evd<0){
		perror ("opening event0 failed");
		return 1;
	}
	printf ("sending key %s for %ld seconds\n",keyname[offset].name,(reptime*time)/1000);
	if (send (evd,sendcode,KEY_PRESSED)<0){
		perror ("writing 'key_pressed' event failed");
		close (evd);
		return 1;
	}
	while (time--){
		usleep(reptime*1000);
		if (send (evd,sendcode,KEY_AUTOREPEAT)<0){
			perror ("writing 'key_autorepeat' event failed");
			close (evd);
			return 1;
		}
	}
	if (send (evd,sendcode,KEY_RELEASED)<0){
		perror ("writing 'key_released' event failed");
		close (evd);
		return 1;
	}
	close (evd);
	return 0;
}
