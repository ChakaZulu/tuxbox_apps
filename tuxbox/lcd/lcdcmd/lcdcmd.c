/*
 *
 *
 *   lcdcmd.c - lcd dim function (dbox-II-project)
 *
 *   Homepage: http://dbox2.elxsi.de
 *
 *   Copyright (C) 2002 Voldemort (voldemort@wizardnet.de)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 */

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#include "dbox/lcd-ks0713.h"
#include "dbox/fp.h"

#define VERSION "0.3"

static int fd;
static int fp;

static void help (char *prog_name) {
  printf("%s Version %s\n",prog_name,VERSION);
  printf("Usage: %s <switches>\n\n",prog_name);
  printf("Switches:\n"
  "-h,     --help            help\n"
  "-aon,   --aon              set LCD autodim on\n"
  "-aoff,  --aoff             set LCD autodim off\n"
  "-d,     --dim <value>      set LCD (value 0-255)\n"
  "-c,     --contrast <value> set LCD contrast (value 0-63)\n"
  "-on,    --on               set LCD on\n"
  "-off,   --off              set LCD off\n"
  "-r,     --reverse          set LCD in reverse mode\n"
  "-n,     --normal           set LCD in normal mode\n");
  exit(0);
}

static int _static (int i) {
    if (i)
    {
	ioctl(fd,LCD_IOCTL_SIR,&i);
    }
    else
    {
	ioctl(fd,LCD_IOCTL_SIR,&i);
	ioctl(fd,LCD_IOCTL_SIRC,&i);
    }
    return 0;
}

static int eon (int i) {
    if( ioctl(fd,LCD_IOCTL_EON,&i) < 0 ) {
	return 1;
    }
    return 0;
}

static int dim (int i) {
    if( ioctl(fp,FP_IOCTL_LCD_DIMM,&i) < 0 ) {
        return 1;
    }
    return 0;
}


static int on (int i) {
    if(i == 1) {
	if(dim(150) < 0 ) {
    	    return 1;
	}
    }	
    else
    {	
	if( dim(0) < 0 ) {
    	    return 1;
	}
    }
    if( ioctl(fd,LCD_IOCTL_ON,&i) < 0 ) {
        return 1;
    }
    return 0;
}

static int con (int i) {
    if( ioctl(fd,LCD_IOCTL_SRV,&i) < 0 ) {
        return 1;
    }
    return 0;
}

static int autodimm (int i){
    if( ioctl(fp,FP_IOCTL_LCD_AUTODIMM,&i) < 0 ) {
	return 1;
    }
return 0;
}

static int reverse (int i){
    if( ioctl(fd,LCD_IOCTL_REVERSE,&i) < 0 ) {
	return 1;
    }
return 0;
}

static int power (int i) {
    if (i)
    {
	eon(0);
	_static(1);
	on(1);
	dim(150);
    }
    else
    {
	dim(0);
	eon(1);
	_static(0);   
	on(0); 
    }
    return 0;
}

int main(int argc, char **argv) {
    int count;
    if((fp = open("/dev/dbox/fp0",O_RDWR)) < 0)
    {
	perror("Error: FP device (/dev/dbox/fp0) not found");
	return -1;
    }
    if((fd = open("/dev/dbox/lcd0",O_RDWR)) < 0)
    {
	perror("Error: LCD device (/dev/dbox/lcd0) not found");
	close(fp);
	return -1;
    }
    if (argc < 2)  help(argv[0]);

    for(count=1;count<argc;count++)
    {

	/* -h or --help */
	if ((strcmp("-h",argv[count]) == 0) || (strcmp("--help",argv[count]) == 0))
	{
	    help(argv[0]);
	}

	/* -d or --dim */
	else if ((strcmp("-d",argv[count]) == 0) || (strcmp("--dim",argv[count]) == 0))
	{
	    if (argc == count+1)
	    {
		printf("No dim arg given\n");
		exit(1);
	    }
	    else if ((argv[count+1][0] < 0x30) || (argv[count+1][0] > 0x39))
	    {
		printf("No dim arg given\n");
		exit(1);
	    }
	    else
	    {
		count++;
		if (dim(atoi(argv[count]))) perror ("dim failed");
	    }
	}

	/* -c or --contrast */
	else if ((strcmp("-c",argv[count]) == 0) || (strcmp("--contrast",argv[count]) == 0))
	{
	    if (argc == count+1)
	    {
		printf("No contrast arg given\n");
		exit(1);
	    }
	    else if ((argv[count+1][0] < 0x30) || (argv[count+1][0] > 0x39))
	    {
		printf("No contrast arg given\n");
		exit(1);
	    }
	    else
	    {
		count++;
		if (con(atoi(argv[count]))) perror ("contrast failed");
	    }
	}

	/* -aon or --aon */
	else if ((strcmp("-aon",argv[count]) == 0) || (strcmp("--aon",argv[count]) == 0)) {
	    if (autodimm(1))	perror ("autodimm on failed");
	}

	/* -aoff or --aoff */
	else if ((strcmp("-aoff",argv[count]) == 0) || (strcmp("--aoff",argv[count]) == 0)) {
	    if (autodimm(0))	perror ("autodimm off failed");
	}

	/* -pon or --pon */
	else if ((strcmp("-pon",argv[count]) == 0) || (strcmp("--pon",argv[count]) == 0)) {
	    if (power(1))	perror ("power on failed");
	}

	/* -poff or --poff */
	else if ((strcmp("-poff",argv[count]) == 0) || (strcmp("--poff",argv[count]) == 0)) {
	    if (power(0))	perror ("power off failed");
	}

	/* -on or --on */
	else if ((strcmp("-on",argv[count]) == 0) || (strcmp("--on",argv[count]) == 0)) {
	    if (on(1))	perror ("on failed");
	}

	/* -off or --off */
	else if ((strcmp("-off",argv[count]) == 0) || (strcmp("--off",argv[count]) == 0)) {
	    if (on(0))	perror ("off failed");
	}

	/* -n or --normal */
	else if ((strcmp("-n",argv[count]) == 0) || (strcmp("--normal",argv[count]) == 0)) {
	    if (reverse(0))	perror ("normal failed");
	}

	/* -r or --reverse */
	else if ((strcmp("-r",argv[count]) == 0) || (strcmp("--reverse",argv[count]) == 0)) {
	    if (reverse(1))	perror ("reverse failed");
	}

	else
	{
	    help(argv[0]);
	}
    }
    close(fd);
    close(fp);
    return 0;
}
