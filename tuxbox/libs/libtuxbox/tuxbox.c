/*
  $Id: tuxbox.c,v 1.1 2003/01/04 23:38:46 waldi Exp $
  
  $Log: tuxbox.c,v $
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tuxbox.h"

unsigned int tuxbox_get_capabilities(void)
{

	char *value;

	if (!(value = getenv(TUXBOX_TAG_CAPABILITIES)))
		return 0;
		
	return strtoul(value, NULL, 0);

}

unsigned int tuxbox_get_vendor(void)
{

	char *value;

	if (!(value = getenv(TUXBOX_TAG_VENDOR)))
		return TUXBOX_VENDOR_UNKNOWN;
		
	return strtoul(value, NULL, 0);

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

	char *value;

	if (!(value = getenv(TUXBOX_TAG_MODEL)))
		return TUXBOX_MODEL_UNKNOWN;
		
	return strtoul(value, NULL, 0);

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

	char *value;

	if (!(value = getenv(TUXBOX_TAG_VERSION)))
		return 0;
		
	return strtoul(value, NULL, 0);

}
