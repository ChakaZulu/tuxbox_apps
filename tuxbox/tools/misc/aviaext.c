/*
 * Tool for testing the infamous Avia extension device
 *
 * $Id: aviaext.c,v 1.1 2004/07/03 01:45:25 carjay Exp $
 *
 * Copyright (C) 2004 Carsten Juttner <carjay@gmx.met>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <dbox/aviaEXT.h>

#define AVIAEXT_DEV "/dev/dbox/aviaEXT"
const char *digon = "Digital audio output turned on\n";
const char *digoff = "Digital audio output turned off\n";
const char *digset = "Digital audio output state (0=off,1=on):\n";

void usage(const char *s)
{
	printf ("%s version 1.0\n"
			"commandline tool for the aviaEXT-module\n\n"
			"Usage: %s <command>\n"
			"Commands:\n"
			"    --help      : displays this text\n\n"
			"    --iec-on    : turn optical output on\n"
			"    --iec-off   : turn optical output off\n"
			"    --iec-state : returns state of IEC\n\n",s,s);
	exit(0);
}

int main (int argc, char **argv)
{
	int i,fd;
	unsigned int cmd=0,param;
	int *ptr = NULL;
	const char *msg = NULL;
	
	if ((fd = open(AVIAEXT_DEV,O_RDWR))<0){
		if (errno==ENOENT){
			fprintf (stderr,"%s does not exist, did you forget to load the aviaEXT module?\n",AVIAEXT_DEV);
		} else {
			perror ("aviaext: error opening /dev/dbox/aviaEXT");
		}
		return 1;
	}
		
	if (argc<2){
		usage(argv[0]);
	}
	for (i=1; i<argc; i++){
		if (!strncmp(argv[i],"--help",6)){
			usage(argv[0]);
		} else if (!strncmp(argv[i],"--iec-on",8)){
			msg = digon;
			cmd = AVIA_EXT_IEC_SET;
			param = 1;
		} else if (!strncmp(argv[i],"--iec-off",9)){
			msg = digoff;
			cmd = AVIA_EXT_IEC_SET;
			param = 0;
		} else if (!strncmp(argv[i],"--iec-state",9)){
			msg = digset;
			cmd = AVIA_EXT_IEC_GET;
			ptr = &param;
		} else {
			printf ("unknown command: %s\n",argv[i]);
		}
	}
	if (cmd){
		int res;
		if (ptr)
			res = ioctl(fd,cmd,ptr);
		else
			res = ioctl(fd,cmd,param);
		if (res<0){
			perror("aviaext: ioctl");
			return 1;
		};
		if (msg)
			printf (msg);
		if (ptr)
			printf ("%d\n",*ptr);
	}
	return 0;
}
