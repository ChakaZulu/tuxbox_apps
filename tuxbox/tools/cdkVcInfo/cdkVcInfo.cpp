/*
 * Tool for printing some image information during bootup.
 *
 * $Id: cdkVcInfo.cpp,v 1.5 2009/12/07 13:41:48 striper Exp $
 *
 * Copyright (C) 2006 the Tuxbox project http://www.tuxbox.org.
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

#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>

#define CONSOLE "/dev/vc/0"
#define VERSION_FILE "/.version"
#define char_delay_usec 500
#define BUFFERSIZE 255
#define BIGBUFFERSIZE 2000

int main() 
{  
  switch (fork()) {
  case -1:
    perror("[cdkVcInfo] fork");
    return -1;
    
  case 0:
    break;
    
  default:
    return 0;
  }

  if (setsid() < 0) {
    perror("[cdkVcInfo] setsid");
    return 1;
  }
		
  char buf[BUFFERSIZE] = "";
  int release_type = -1;
  int imageversion = 0;
  int imagesubver = 0;
  int year = 9999;
  int month = 99;
  int day = 99;
  int hour = 99;
  int minute = 99;
  char creator[BUFFERSIZE] = "-- unknown --";
  char imagename[BUFFERSIZE] = "-- unknown --";
	
  FILE* fv = fopen(VERSION_FILE, "r");
  if (fv) {
    while (fgets(buf, BUFFERSIZE, fv)) {
      sscanf(buf, "version=%1d%1d%2d%4d%2d%2d%2d%2d", 
	     &release_type, &imageversion, &imagesubver,
	     &year, &month, &day, &hour, &minute);
      sscanf(buf, "creator=%[^\n]", (char *) &creator);
      sscanf(buf, "imagename=%[^\n]", (char *) &imagename);
    }
    fclose(fv);
  }

  char message[BIGBUFFERSIZE];
  strcpy(message, "");
  sprintf(message,
	  "\n\n\n\n\n\n\n\t\t\t-------- Image Info --------\n\n"
	  "\t\t\t  Image Version     : %d.%02d\n"
	  "\t\t\t  Image Type        : %s\n"
	  "\t\t\t  Creation Date     : %d-%02d-%02d\n"
	  "\t\t\t  Creation Time     : %d:%02d\n"
	  "\t\t\t  Creator           : %s\n"
	  "\t\t\t  Image Name        : %s\n"
	  "\n\n\t\t\t\tLoading....\n",
	  imageversion, imagesubver, 
	  release_type == 0    ? "Release" 
	  : release_type == 1  ? "Snapshot" 
	  :  release_type == 2 ? "Internal"
	  : 			 "Unknown",
	  year, month, day,
	  hour, minute, creator, imagename);
  //printf("%s", message);

  FILE *fb = fopen(CONSOLE, "w" );
  if (fb == 0) {
    perror("[cdkVcInfo] fopen");
    exit(1);
  }

  for (unsigned int i = 0; i < strlen(message); i++) {
    fputc(message[i], fb);
    fflush(fb);
    usleep(char_delay_usec);
  }

  fclose(fb);
  exit(0);
}
