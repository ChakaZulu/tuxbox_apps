#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>

#include "rc.h"

int fd=-1;
int lastpressed = KEY_UNKNOWN;

int RCOpen()
{
  fd=open("/dev/input/event0", O_RDONLY);
  if (fd<0)
  {
    perror("/dev/input/event0");
    return fd;
  }
  return 0;
}

int RCGet()
{
	struct input_event rc_ev;
	int rd;
	unsigned char new_key = 0;
	do {
		rd = read(fd, &rc_ev, sizeof(struct input_event));
		if (rc_ev.value == !0){
			if (rc_ev.code != lastpressed){	// new button was pressed
				lastpressed = rc_ev.code;
				new_key = 1;
			}
		} 
		else if (rc_ev.value == 0){	// (some) button was released, accept same key again
			lastpressed = KEY_UNKNOWN;
		}
	}
	while(!new_key);

	if (rd < (int) sizeof(struct input_event)){
		perror("RCGet()");
		return -1;
 	}
	return rc_ev.code;
}

int RCPoll()
{
  return -1;
}

int RCClose()
{
  if (fd>=0)
    close(fd);
  return 0;
}
