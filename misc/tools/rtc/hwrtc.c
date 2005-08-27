/*
 * $Id: hwrtc.c,v 1.1 2005/08/27 01:56:22 chakazulu Exp $
 *
 * Reads/Sets time from/to a RTC.
 *
 * Copyright (C) 200 Michael Schuele 'ChakaZulu' <tuxbox@mschuele.de>
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

#include <stdio.h>     /* perror() */
#include <string.h>    /* memcpy() */
#include <unistd.h>    /* close()  */
#include <stdlib.h>    /* getenv() */

#include <time.h>      /* struct tm */
#include <sys/ioctl.h> /* ioctl()   */

#include <linux/rtc.h> /* rtc ioctls         */
#include <sys/time.h>  /* set/gettimeofday() */

/* open() */
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

/* the clock device */
static const char RTC_DEVICE[] = "/dev/dbox/clock";

/*
 * Converts a broken-down time to calendar time
 * without changes due to timezone settings.
 *
 * from 'man timegm'
 */
time_t my_timegm(struct tm *tm) {

	time_t ret;
	char *tz;
	
	tz = getenv("TZ");
	setenv("TZ", "", 1);
	tzset();
	ret = mktime(tm);
	if (tz)
		setenv("TZ", tz, 1);
	else
		unsetenv("TZ");
	tzset();
	return ret;
}

/*
 * Reads local system time, converts it to UTC and updates the RTC.
 */
void systohw() {
	int file;
	struct tm systime;
	time_t systime_local;

	if ((file = open(RTC_DEVICE,O_WRONLY)) < 0) {
		perror(RTC_DEVICE);
		return;
	}
	time(&systime_local);
	gmtime_r(&systime_local,&systime);

	/*
	  printf("setting new hardware time:\n");
	  printf("%02d:%02d:%02d, %04d-%02d-%02d\n weekday %d yearday %d, isdst: %d\n",
	  systime.tm_hour,systime.tm_min,systime.tm_sec,
	  systime.tm_year+1900,systime.tm_mon+1,systime.tm_mday,
	  systime.tm_wday,systime.tm_yday,systime.tm_isdst);
	  
	  printf("setting new hardware time:\n");
	  printf("%02d:%02d:%02d, %04d-%02d-%02d\n weekday %d yearday %d, isdst: %d\n",
	  clock.tm_hour,clock.tm_min,clock.tm_sec,
	  clock.tm_year+1900,clock.tm_mon+1,clock.tm_mday,
	  clock.tm_wday,clock.tm_yday,clock.tm_isdst);
	*/

	// !! struct tm == struct rtc_time
	if (ioctl(file,RTC_SET_TIME,&systime) < 0) {
		perror("setting time failed");
	}
	
	close(file);
}

/*
 * Reads the RTC time and updates the system's time
 */
void hwtosys() {
	int file;
	struct tm new_systime;
	struct timeval new_systime_local;

	if ((file = open(RTC_DEVICE,O_WRONLY)) < 0) {
		perror(RTC_DEVICE);
		return;
	}
	
	// !! struct tm == struct rtc_time
	if (ioctl(file,RTC_RD_TIME,&new_systime) < 0) {
		perror("reading time failed");
		return;
	}
	/*
	  printf("setting new system time:\n");
	  printf("%02d:%02d:%02d, %04d-%02d-%02d\n weekday %d yearday %d, isdst: %d\n",
	  clock.tm_hour,clock.tm_min,clock.tm_sec,
	  clock.tm_year+1900,clock.tm_mon+1,clock.tm_mday,
	  clock.tm_wday,clock.tm_yday,clock.tm_isdst);
	*/
	
	new_systime_local.tv_sec = my_timegm(&new_systime);
	new_systime_local.tv_usec = 0;

	settimeofday(&new_systime_local,NULL);
	close(file);
}

/*
 * Sets the system and RTC time to the given values
 */
void settime(char *date, char *time) {
	struct timeval new_tv;
	struct tm new;
	sscanf(date,"%04d-%02d-%02d",&new.tm_year,&new.tm_mon,&new.tm_mday);
	//printf("%04d-%02d-%02d\n",new.tm_year,new.tm_mon,new.tm_mday);
	sscanf(time,"%02d:%02d:%02d",&new.tm_hour,&new.tm_min,&new.tm_sec);
	//printf("%02d:%02d:%02d\n",new.tm_hour,new.tm_min,new.tm_sec);

	new.tm_year -= 1900;
	new.tm_mon  -= 1;

	new_tv.tv_sec  = mktime(&new);
	new_tv.tv_usec = 0;
	//printf("%d\n",new_tv.tv_sec);
	settimeofday(&new_tv,NULL);
	systohw();
}

int main(int argc, char* argv[]) {

	if (argc == 2 && !strcmp(argv[1],"hwtosys"))
		hwtosys();
	else if (argc == 2 && !strcmp(argv[1],"systohw"))
		systohw();
	else if (argc == 4 && !strcmp(argv[1],"settime"))
		settime(argv[2],argv[3]);
	else {
		printf("\nRead/Write RTC time\n$Version$ $Author: chakazulu $\n\n");
		printf("Usage: <rtctest> command\n");
		printf("\ncommand:\n");
		printf("hwtosys\t\t - set system time from rtc\n");
		printf("systohw\t\t - set rtc from system time\n");
		printf("settime <time>\t - set system and rtc time to given value\n");
		printf("  <time> \t - yyyy-mm-dd hh:mm:ss\n\n");
		return 0;
	}

	return 0;
}
