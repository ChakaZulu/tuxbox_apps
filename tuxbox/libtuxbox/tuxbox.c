/*
  $Id: tuxbox.c,v 1.4 2003/02/18 18:41:46 obi Exp $
  
  $Log: tuxbox.c,v $
  Revision 1.4  2003/02/18 18:41:46  obi
  == 1.2
  please do not use segfaulting code for drivers of other branches.
  commit the tuxbox module into the other branch instead. thanks.

  Revision 1.2  2003/01/09 02:22:07  obi
  read from proc, not from getenv()

  bash does not like . /proc/bus/tuxbox which was the reason for copying
  dbox.sh to /tmp

  Revision 1.1  2003/01/04 23:38:46  waldi
  move libtuxbox

  Revision 1.4  2003/01/03 11:13:09  Jolt
  - Added tag defines
  - Renamed *manufacturer* to *vendor*
  - Added some caps

  Revision 1.3  2003/01/01 22:14:51  Jolt
  - Renamed files
  - Merged in tuxbox.h

  Revision 1.2  2003/01/01 21:55:41  Jolt
  Tuxbox info lib

  Revision 1.1  2003/01/01 21:30:10  Jolt
  Tuxbox info lib


*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tuxbox.h"

static unsigned int tuxbox_read_proc(char *search)
{
	
	FILE *file;
	char *line = NULL;
	size_t len = 0;
	int ret = 0;

	if (!search)
		return ret;

	file = fopen("/proc/bus/tuxbox", "r");

	if (!file) {
		perror("/proc/bus/tuxbox");
		return ret;
	}

	while (getline(&line, &len, file) != -1) {
		if (line) {
			if (!strncmp(line, search, strlen(search))) {
				char *value = strchr(line, '=');
				if (value) {
					ret = strtoul(&value[1], NULL, 0);
					break;
				}
			}
		}
	}

	fclose(file);

	if (line)
		free(line);

	return ret;
	
}

unsigned int tuxbox_get_capabilities(void)
{

	return tuxbox_read_proc(TUXBOX_TAG_CAPABILITIES);

}

unsigned int tuxbox_get_vendor(void)
{

	return tuxbox_read_proc(TUXBOX_TAG_VENDOR);

}

char *tuxbox_get_vendor_str(void)
{

	switch(tuxbox_get_vendor()) {
	
		case TUXBOX_VENDOR_NOKIA:
		
			return "Nokia";
			
		case TUXBOX_VENDOR_SAGEM:
		
			return "Sagem";
			
		case TUXBOX_VENDOR_PHILIPS:
			
			return "Philips";
			
		case TUXBOX_VENDOR_DREAM_MM:
		
			return "Dream Multimedia TV";
			
		default:
		
			return "Unknown";
			
	}

}

unsigned int tuxbox_get_model(void)
{

	return tuxbox_read_proc(TUXBOX_TAG_MODEL);

}

char *tuxbox_get_model_str(void)
{

	switch(tuxbox_get_model()) {
	
		case TUXBOX_MODEL_DBOX2:
		
			return "D-BOX2";

		case TUXBOX_MODEL_DREAMBOX_DM7000:
		
			return "DM 7000";
			
		case TUXBOX_MODEL_DREAMBOX_DM5600:
		
			return "DM 5600";

		default:
		
			return "Unknown";
		
	}

}

unsigned int tuxbox_get_version(void)
{

	return tuxbox_read_proc(TUXBOX_TAG_VERSION);

}
