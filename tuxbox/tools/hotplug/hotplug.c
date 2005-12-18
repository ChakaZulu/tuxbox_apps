/* $Id: hotplug.c,v 1.2 2005/12/18 14:40:41 carjay Exp $

   Hotplug written in C to speed things up
   
   for the non-believers: loading f.ex. the cam-module
   takes about 2.2 seconds using hotplug.sh and 0.7 secs in C.
   That's 3 times faster.

   Copyright (C) 2005 Carsten Juttner <carjay@gmx.net>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2, as 
   published by the Free Software Foundation.
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define FW_PATH "/var/tuxbox/ucodes/"
#define SYS_PATH "/sys"
static char firmware_path[PATH_MAX] = FW_PATH;
static char sysfs_path[PATH_MAX] = SYS_PATH;
static const int sysfs_base_len = sizeof(SYS_PATH-1);

#ifdef DEBUG
static FILE *outhandle;
/* we're started by the kernel, 
       so we cannot write to stderr */
static void init_debug()
{
	outhandle = fopen("/dev/console","wb");
	if (!outhandle)
		return;
}
static void fini_debug()
{
	if (outhandle)
		fclose(outhandle);
}
static void output_debug(char *msg, ...)
{
	if (!outhandle)
		return;
	va_list ap;
	va_start(ap,msg);
	vfprintf(outhandle, msg, ap);
	va_end(ap);
}
#else
#define init_debug()
#define fini_debug()
#define output_debug(...)
#endif

static int filecopy(int out, int in)
{
	char block[32*4096]; /* 128 kiByte */
	ssize_t todo;
	struct stat st;
	
	if (fstat(in,&st)<0) {
		output_debug("hotplug: getting size of firmware file failed\n");
		return -1;
	}
	
	if (!st.st_size) {
		output_debug("hotplug: firmware has 0 bytes\n");
		return 0;
	}
	output_debug("hotplug: firmware has %d bytes\n",st.st_size);

	todo = st.st_size;
	do {
		ssize_t wr_off = 0;
		ssize_t readsize=todo;
		if (todo>sizeof(block))
			readsize = sizeof(block);
		ssize_t res = read(in,block,readsize);
		if (res<=0){
			output_debug("hotplug: error reading firmware\n");
			return -1;
		} else {
			todo -= res;
		}
		do {
			ssize_t written = write(out,block+wr_off,res);
			if (written<=0){
				output_debug("hotplug: error writing firmware\n");
				return -1;
			} else {
				res -= written;
				wr_off += written;
			}
			
		} while (res);
		
	} while (todo);

	return 0;
}

static int action_add(char *fw_name)
{
	int ret = -1;
	int hloading;
	int hfin,hfout;
	int retry_count = 0;
	static const char ack[3]="01\xff"; /* '0','1','-1' */
#define ACK_0 	0
#define ACK_1 	1
#define ACK_ERR 2

	char *devpath_env = getenv("DEVPATH");
	output_debug("hotplug: add-action called for dev \"%s\" \"%s\"\n",devpath_env?devpath_env:"unknown",fw_name);

	if (!devpath_env) {
		output_debug("hotplug: add-action called with an empty devpath\n");
		return ret;
	}
	
	strncat(sysfs_path,devpath_env,PATH_MAX);
	strncat(sysfs_path,"/loading",PATH_MAX);

	while (access(sysfs_path,F_OK)){
		usleep(100000);
		if (++retry_count==20) {
			output_debug("hotplug: waiting for \"%s\" timed out\n",sysfs_path);
			return ret;
		}
	}

	hloading = open(sysfs_path,O_WRONLY);
	if (hloading<0) {
		output_debug("hotplug: opening loading-file \"%s\" failed\n",sysfs_path);
		return ret;
	}

	write(hloading,&ack[ACK_1],1);
	strncat(firmware_path,fw_name,PATH_MAX);

	hfin=open(firmware_path,O_RDONLY);
	if (!hfin) {
		output_debug("hotplug: opening firmware \"%s\" failed\n",firmware_path);
		goto hfin_out;
	}
	
	sysfs_path[sysfs_base_len]=0x00; /* truncate again */
	strncat(sysfs_path,devpath_env,PATH_MAX);
	strncat(sysfs_path,"/data",PATH_MAX);

	hfout=open(sysfs_path,O_WRONLY);
	if (!hfout) {
		output_debug("hotplug: opening data file \"%s\" failed\n",sysfs_path);
		goto hfout_out;
	}
	if (filecopy(hfout,hfin)) {
		output_debug("hotplug: error copying file \"%s\" to \"%s\"\n",firmware_path,sysfs_path);
		goto filecopy_out;
	}
	write(hloading,&ack[ACK_0],1);
	
	output_debug("hotplug: successfully wrote firmware file \"%s\" to \"%s\"\n",firmware_path,sysfs_path);
	ret = 0;
	
filecopy_out:
	close(hfout);
hfout_out:
	close(hfin);
hfin_out:
	if (ret)
		write(hloading,&ack[ACK_ERR],1);

	close(hloading);
	return ret;
}

int main( int argc, char **argv )
{
	int ret = 0;
	char *action_env = getenv("ACTION");
	init_debug();
	if (action_env && !strncmp( action_env,"add", 3)) {
		char *firmware_env = getenv("FIRMWARE");
		if (firmware_env && *firmware_env != 0x00) {
			ret = action_add(firmware_env);
		}
	}

	fini_debug();
	return ret;
}
