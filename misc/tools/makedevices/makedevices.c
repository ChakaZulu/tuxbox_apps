/*  $Id
    A small hack to create all needed device nodes in rcS.
    Ten times faster than doing it in a script.

    Copyright (C) 2007 Stefan Seyfried
    All rights reserved.

    This program is released under the GNU General Public License
    (GPL) Version 2, not any other version of that license.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define C S_IFCHR
#define B S_IFBLK

struct deventry {
	const char *name;
	const int type;
	const char major;
	const char minor;
};

struct deventry devices[] = {
	{ "mem",	C, 1, 1 },
	{ "kmem",	C, 1, 2 },
	{ "null",	C, 1, 3 },
	{ "zero",	C, 1, 5 },
	{ "full",	C, 1, 7 },
	{ "random",	C, 1, 8 },
	{ "urandom",	C, 1, 9 },
	{ "tty0",	C, 4, 0 },
	{ "tty1",	C, 4, 1 },
	{ "tty2",	C, 4, 2 },
	{ "tty3",	C, 4, 3 },
	/* different major / minor number than on 2.4... */
	{ "tts/0",	C, 204, 46 },
	{ "tts/1",	C, 204, 47 },
	{ "tty",	C, 5, 0 },
	{ "console",	C, 5, 1 },
	{ "ptmx",	C, 5, 2 },
	{ "loop0",	B, 7, 0 },
	{ "loop1",	B, 7, 1 },
	{ "loop2",	B, 7, 2 },
	{ "loop3",	B, 7, 3 },
	{ "loop4",	B, 7, 4 },
	{ "loop5",	B, 7, 5 },
	{ "loop6",	B, 7, 6 },
	{ "loop7",	B, 7, 7 },
	/* "4 partitions should be enough for everybody!" */
	{ "hda",	B, 3, 0 },
	{ "hda1",	B, 3, 1 },
	{ "hda2",	B, 3, 2 },
	{ "hda3",	B, 3, 3 },
	{ "hda4",	B, 3, 4 },
	{ "vc/0",	C, 7, 0 },
	{ "vc/1",	C, 7, 1 },
	{ "vc/2",	C, 7, 2 },
	{ "vc/3",	C, 7, 3 },
	{ "vc/4",	C, 7, 4 },
	{ "vc/5",	C, 7, 5 },
	{ "vc/6",	C, 7, 6 },
	{ "input/mouse0",	C, 13, 32 },
	{ "input/mice",		C, 13, 63 },
	{ "input/event0",	C, 13, 64 },
	{ "sound/mixer",	C, 14, 0 },
	{ "sound/dsp",		C, 14, 3 },
	{ "sound/mixer1",	C, 14, 16 },
	{ "fb0",	C, 29, 0 },
	{ "mtdblock/0",	B, 31, 0 },
	{ "mtdblock/1",	B, 31, 1 },
	{ "mtdblock/2",	B, 31, 2 },
	{ "mtdblock/3",	B, 31, 3 },
	{ "mtdblock/4",	B, 31, 4 },
	{ "mtdblock/5",	B, 31, 5 },
	{ "v4l/video0",	C, 81, 0 },
	{ "i2c/0",	C, 89, 0 },
	{ "mtd/0",	C, 90, 0 },
	{ "mtd/1",	C, 90, 2 },
	{ "mtd/2",	C, 90, 4 },
	{ "mtd/3",	C, 90, 6 },
	{ "mtd/4",	C, 90, 8 },
	{ "mtd/5",	C, 90, 10 },
	{ "mtd/0ro",	C, 90, 1 },
	{ "mtd/1ro",	C, 90, 3 },
	{ "mtd/2ro",	C, 90, 5 },
	{ "mtd/3ro",	C, 90, 7 },
	{ "mtd/4ro",	C, 90, 9 },
	{ "mtd/5ro",	C, 90, 11 },
	{ "dvb/adapter0/video0",	C, 212, 0 },
	{ "dvb/adapter0/audio0",	C, 212, 1 },
	{ "dvb/adapter0/frontend0",	C, 212, 3 },
	{ "dvb/adapter0/demux0",	C, 212, 4 },
	{ "dvb/adapter0/dvr0",		C, 212, 5 },
	{ "dvb/adapter0/ca0",		C, 212, 6 },
	{ "dvb/adapter0/net0",		C, 212, 7 },
	{ "dvb/adapter0/ca1",		C, 212, 22 },
	{ NULL, 0, 0, 0 }
};

int main(void)
{
	int i = 0;
	char devname[100];
	while (devices[i].name) {
		strcpy(devname, "/dev/");
		strcat(devname, devices[i].name);
		if (mknod(devname, devices[i].type|0600,
			  makedev(devices[i].major,devices[i].minor)))
			fprintf(stderr, "mknod %s: %s\n", devname,
							strerror(errno));
		i++;
	}
	return 0;
}
