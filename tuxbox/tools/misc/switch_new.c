/*
 *   switch.c - audio/video switch tool (dbox-II-project)
 *
 *   Homepage: http://dbox2.elxsi.de
 *
 *   Copyright (C) 2001 Gillem (htoa@gmx.net)
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
 *
 *   $Log: switch_new.c,v $
 *   Revision 1.2  2001/03/25 13:57:24  gillem
 *   - update includes
 *
 *   Revision 1.1  2001/03/20 21:16:00  gillem
 *   - switch rewrite
 *
 *
 *   $Revision: 1.2 $
 *
 */

/* ---------------------------------------------------------------------- */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <argp.h>

#include "dbox/avs_core.h"

/* ---------------------------------------------------------------------- */

const char *argp_program_version = "switch 0.1";
const char *argp_program_bug_address = "<htoa@gmx.net>";
static char doc[] = "audio/video switch";

/* The options we understand. */
static struct argp_option options[] = {
  {"mute",                'm', 0,          0,  "Mute" },
  {"show",                's', 0,          0,  "Show current settings" },
  {"unmute",              'u', 0,          0,  "Unmute" },

  {"volume",              'l', "VOL",      0,  "Set volume" },
  {"zc-detector",         'z', "ON/OFF",   0,  "Set zero cross detector" },
  {"video-fs-ctrl",       'f', "<0-3>",    0,  "Set video function switch control" },
  {"ycmix",               'y', "ON/OFF",   0,  "Set y/c mix" },
  {"video-fb-ctrl",       'b', "<0-3>",    0,  "Set video fast blanking control" },
  {"logic",               'c', "ON/OFF",   0,  "Set logic" },
  {"route-video",         'v', "SRC:DEST", 0,  "Route video" },
  {"route-audio",         'a', "SRC:DEST", 0,  "Route audio" },

  {0,0,0,0, "Routing sources:\nTV,VCR,AUX" },
  {0,0,0,0, "Routing TV destinations:\nDE1,DE2,VCR,AUX,DE3,DE4,DE5,VM1" },
  {0,0,0,0, "Routing VCR destinations:\nDE1,DE2,VCR,AUX,DE3,VM1,VM2,VM3" },
  {0,0,0,0, "Routing AUX destinations:\nDE1,VM1,VCR,AUX,DE2,VM2,VM3,VM4" },
  { 0 }
};

struct arguments
{
  int verbose;
};

/* ---------------------------------------------------------------------- */

static error_t parse_opt (int key, char *arg, struct argp_state *state);

int fd;

/* ---------------------------------------------------------------------- */

/* Our argp parser. */
static struct argp argp = { options, parse_opt, 0, doc };

/* ---------------------------------------------------------------------- */

int show_type()
{
	int i;

	if (ioctl(fd,AVSIOGTYPE,&i)< 0)
	{
		perror("AVSIOGTYPE:");
		return -1;
	}

	printf("Type: ");

	switch(i)
	{

		case CXA2092:
			printf("CXA2092");
			break;
		case CXA2126:
			printf("CXA2126");
			break;
		default:
			printf("unknown");
			break;
	}

	printf("\n");

	return 0;
}

int show_ycm()
{
	int i;

	if (ioctl(fd,AVSIOGYCM,&i)< 0)
	{
		perror("AVSIOGYCM:");
		return -1;
	}

	printf("YCM: %d\n",i);

	return 0;
}

int show_zcd()
{
	int i;

	if (ioctl(fd,AVSIOGZCD,&i)< 0)
	{
		perror("AVSIOGZCD:");
		return -1;
	}

	printf("ZCD: %d\n",i);
	return 0;
}

int show_fnc()
{
	int i;

	if (ioctl(fd,AVSIOGFNC,&i)< 0)
	{
		perror("AVSIOGFNC:");
		return -1;
	}

	printf("FNC: %d\n",i);
	return 0;
}

int show_fblk()
{
	int i;

	if (ioctl(fd,AVSIOGFBLK,&i)< 0)
	{
		perror("AVSIOGFBLK:");
		return -1;
	}

	printf("FBLK: %d\n",i);
	return 0;
}

/* ---------------------------------------------------------------------- */

int mute()
{
	int i;

	i=AVS_MUTE;

	if (ioctl(fd,AVSIOSMUTE,&i)< 0)
	{
		perror("AVSIOSMUTE:");
		return -1;
	}

	return 0;
}

int unmute()
{
	int i;

	i=AVS_UNMUTE;

	if (ioctl(fd,AVSIOSMUTE,&i)< 0)
	{
		perror("AVSIOSMUTE:");
		return -1;
	}

	return 0;
}

/* ---------------------------------------------------------------------- */

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
	struct arguments *arguments = state->input;

	switch (key)
	{
		case 's':
			show_type();
			show_zcd();
			show_fnc();
			show_ycm();
			show_fblk();
			break;

		case 'm':
	        mute();
			break;

		case 'u':
	        unmute();
			break;

		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

/* ---------------------------------------------------------------------- */

int main (int argc, char **argv)
{
	int count,i;
	struct arguments arguments;

	if ((fd = open("/dev/dbox/avs0",O_RDWR)) <= 0)
	{
		perror("open");
		return -1;
	}

	argp_parse (&argp, argc, argv, 0, 0, &arguments);

	close(fd);

	return 0;
}

/* ---------------------------------------------------------------------- */
