/*
 * $Id: saa.c,v 1.13 2005/05/26 19:14:43 carjay Exp $
 * 
 * Test tool for the SAA 7126H/7127H-driver
 *
 * Copyright (C) 2000-2001 Gillem <htoa@gmx.net>
 * Copyright (C) 2004 Carsten Juttner <carjay@gmx.net>
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
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <dbox/saa7126_core.h>

#define SAA7126_DEVICE "/dev/dbox/saa0"

#define VERSION "0.3"
void help(char *prog_name) {
 	printf("Version %s\n",VERSION);
 	printf("Usage: %s <options>\n\n",prog_name);
 	printf("Switches:\n"
		"-h, --help            help\n"
		"-o, --power-save <X>  power save mode\n"
		"                      none get power save state\n"
		"                      0    power save off\n"
		"                      1    power save on\n"
		"-r, --rgb             rgb mode\n"
		"-f, --fbas            fbas mode\n"
		"-s, --svideo          svideo mode\n"
		"-y, --yuv-cvbs        yuv+cvbs mode\n"
		"    --yuv-vbs         yuv+vbs mode\n"
		"-m, --mode            get current mode\n"
		"-p, --pal             pal mode\n"
		"-n, --ntsc            ntsc mode\n"
		"-i, --input <X>       input control\n"
		"                      MP1      = 1\n"
		"                      MP2      = 2\n"
		"                      CSYNC    = 4\n"
		"                      DEMOFF   = 8\n"
		"                      SYMP     = 16\n"
		"                      COLORBAR = 128\n"
		" -w, --wss <x>        widescreen signalling\n"
		"                      none get widescreen signalling state\n"
		"                      0    4:3 full format\n"
		"                      1    14:9 center letterbox\n"
		"                      2    14:9 top letterbox\n"
		"                      3    16:9 center letterbox\n"
		"                      4    16:9 top letterbox\n"
		"                      5    >16:9 center letterbox\n"
		"                      6    4:3 with 14:9 center letterbox\n"
		"                      7    16:9 full format (anamorphic)\n"
		"                      8    turned off\n"
		" -t, --ttx <x>        teletext VBI reinsertion\n"
		"                      none get teletext reinsertion state\n"
		"                      0    turned off\n"
		"                      1    turned on\n");
}

int read_powersave()
{
	int arg=0;
	int fd;

	if((fd = open(SAA7126_DEVICE,O_RDWR|O_NONBLOCK)) < 0){
		perror("SAA DEVICE: ");
		return -1;
	}

	if ( (ioctl(fd,SAAIOGPOWERSAVE,&arg) < 0)){
		perror("IOCTL: ");
		close(fd);
		return -1;
	}

	printf("SAA7126 POWER STATE: ");

	if(arg)
		printf("ON\n");
	else
		printf("OFF\n");

	close(fd);

	return 0;
}

static int read_wss(void)
{
	int arg=0;
	int fd;

	if((fd = open(SAA7126_DEVICE,O_RDWR|O_NONBLOCK)) < 0){
		perror("SAA DEVICE: ");
		return -1;
	}

	if ( (ioctl(fd,SAAIOGWSS,&arg) < 0)){
		perror("IOCTL: ");
		close(fd);
		return -1;
	}
	close(fd);

	printf("SAA7126 WSS STATE: ");

	if (arg==8)
		printf("8 (OFF)\n");
	else 
		printf("%d\n",arg);

	return 0;
}

static int read_ttx(void)
{
	int arg=0;
	int fd;

	if((fd = open(SAA7126_DEVICE,O_RDWR|O_NONBLOCK)) < 0){
		perror("SAA DEVICE: ");
		return -1;
	}

	if ( (ioctl(fd,SAAIOGTTX,&arg) < 0)){
		perror("IOCTL: ");
		close(fd);
		return -1;
	}
	close(fd);

	printf("SAA7126 TTX STATE: ");

	if(arg)
		printf("ON\n");
	else
		printf("OFF\n");
	return 0;
}

static int read_mode(void)
{
	int arg=0;
	int fd;

	if((fd = open(SAA7126_DEVICE,O_RDWR|O_NONBLOCK)) < 0){
		perror("SAA DEVICE: ");
		return -1;
	}

	if ( (ioctl(fd,SAAIOGMODE,&arg) < 0)){
		perror("IOCTL: ");
		close(fd);
		return -1;
	}
	close(fd);

	printf("SAA7126 MODE: ");
	switch (arg){
	case SAA_MODE_RGB:
		printf("RGB\n");
		break;
	case SAA_MODE_FBAS:
		printf("FBAS\n");
		break;
	case SAA_MODE_SVIDEO:
		printf("SVIDEO\n");
		break;
	case SAA_MODE_YUV_V:
		printf("YUV+VBS\n");
		break;
	case SAA_MODE_YUV_C:
		printf("YUV+CVBS\n");
		break;
	default:
		printf("Unknown mode: %d.\n",arg);
		break;
	}
	return 0;
}

int main(int argc, char **argv)
{
	int fd;
	int count=1;
	int mode;
	int arg;

	if (argc < 2){
		help(argv[0]);
		return 0;
	}

	if ((strcmp("-h",argv[count]) == 0) || (strcmp("--help",argv[count]) == 0)) {
		help(argv[0]);
		return 0;
	} 
	else if ((strcmp("-r",argv[count]) == 0) || (strcmp("--rgb",argv[count]) == 0)) {
	  arg = SAA_MODE_RGB;
	  mode = SAAIOSMODE;
	}
	else if ((strcmp("-f",argv[count]) == 0) || (strcmp("--fbas",argv[count]) == 0)) {
		arg = SAA_MODE_FBAS;
		mode = SAAIOSMODE;
	}
	else if ((strcmp("-s",argv[count]) == 0) || (strcmp("--svideo",argv[count]) == 0)) {
		arg = SAA_MODE_SVIDEO;
		mode = SAAIOSMODE;
	}
	else if ((strcmp("-y",argv[count]) == 0) || (strcmp("--yuv-cvbs",argv[count]) == 0)) {
		arg = SAA_MODE_YUV_C;
		mode = SAAIOSMODE;
	}
	else if (strcmp("--yuv-vbs",argv[count]) == 0) {
		arg = SAA_MODE_YUV_V;
		mode = SAAIOSMODE;
	}
	else if ((strcmp("-m",argv[count]) == 0) || (strcmp("--mode",argv[count]) == 0)) {
		read_mode();
		return 0;
	}
	else if ((strcmp("-p",argv[count]) == 0) || (strcmp("--pal",argv[count]) == 0)) {
		arg = SAA_PAL;
		mode = SAAIOSENC;
	}
	else if ((strcmp("-n",argv[count]) == 0) || (strcmp("--ntsc",argv[count]) == 0)) {
		arg = SAA_NTSC;
		mode = SAAIOSENC;
	}
	else if ((strcmp("-i",argv[count]) == 0) || (strcmp("--input",argv[count]) == 0)) {
		if(argc<3){
			help(argv[0]);
			return 0;
	 	}
		arg = atoi(argv[count+1]);
		mode = SAAIOSINP;
	}
	else if ((strcmp("-o",argv[count]) == 0) || (strcmp("--power-save",argv[count]) == 0)) {
		if(argc<3){
			read_powersave();
			return 0;
		} else {
			arg = atoi(argv[count+1]);
			mode = SAAIOSPOWERSAVE;
		}
	}
	else if ((strcmp("-w",argv[count]) == 0) || (strcmp("--wss",argv[count]) == 0) ) {
		if(argc<3){
			read_wss();
			return 0;
		} else {
			arg = atoi(argv[count+1]);
			mode = SAAIOSWSS;
		}
	}
	else if ((strcmp("-t",argv[count]) == 0) || (strcmp("--ttx",argv[count]) == 0) ) {
		if (argc<3){
			read_ttx();
			return 0;
		} else {
			arg = atoi(argv[count+1]);
			mode = SAAIOSTTX;
		}
	}
	else {
		help(argv[0]);
		return 0;
	}

	if((fd = open(SAA7126_DEVICE,O_RDWR|O_NONBLOCK)) < 0){
		perror("SAA DEVICE: ");
		return -1;
	}

	if ( (ioctl(fd,mode,&arg) < 0)){
		perror("IOCTL: ");
		return -1;
	}

	close(fd);
	return 0;
}
