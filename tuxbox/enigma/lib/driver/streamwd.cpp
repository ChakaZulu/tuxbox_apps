#include "streamwd.h"
#include "eavswitch.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "ost/video.h"
#include "edvb.h"

eStreamWatchdog *eStreamWatchdog::instance;

	/* hier mal bitte support fuer das event device oder so einbauen */

eStreamWatchdog::eStreamWatchdog()
{
	connect(&timer, SIGNAL(timeout()), SLOT(checkstate()));
	timer.start(1000);
	last=-1;

	if (!instance)
		instance=this;

}

eStreamWatchdog *eStreamWatchdog::getInstance()
{
	return instance;
}

void eStreamWatchdog::checkstate()
{
	int isanamorph=0;
	FILE *bitstream=fopen("/proc/bus/bitstream", "rt");
	if (bitstream)
	{
	    char buffer[100];
	    int aspect=0;
	    while (fgets(buffer, 100, bitstream))
	    {
	    	if (!strncmp(buffer, "A_RATIO: ", 9))
	    		aspect=atoi(buffer+9);
	    }
	    fclose(bitstream);
	    switch (aspect)
	    {
		case 1:
		case 2:
			isanamorph=0;
		break;
		case 3:
		case 4:
			isanamorph=1;
	    }
	}

	if (last != isanamorph)
	{
	    last=isanamorph;
 	
	    emit AspectRatioChanged(isanamorph);
	
	    int fd;
	    if ((fd = open("/dev/ost/video0",O_RDWR)) <= 0)
	    {
  		perror("open");
		return;
	    }
    
	    int videoDisplayFormat=VIDEO_LETTER_BOX;
	    int doanamorph=0;
    
	    unsigned int pin8;
	    eDVB::getInstance()->config.getKey("/elitedvb/video/pin8", pin8);
        
    
	    switch (pin8)
	    {
		case 0:
   		    doanamorph=0;
   		    videoDisplayFormat=VIDEO_LETTER_BOX;
   		break;
   		case 1:
   		    doanamorph=0;
   		    videoDisplayFormat=VIDEO_PAN_SCAN;
   		break;
		case 2:
   		    doanamorph=isanamorph;
   		    videoDisplayFormat=VIDEO_CENTER_CUT_OUT;
   		break;
	    }
		
	    if (ioctl(fd, VIDEO_SET_DISPLAY_FORMAT, videoDisplayFormat))
	    {
    		perror("VIDEO SET DISPLAY FORMAT:");
    		 return;
 	    }
	    close(fd);
    
	    eAVSwitch::getInstance()->setAspectRatio(doanamorph?r169:r43);
	}
}


eStreamWatchdog::~eStreamWatchdog()
{
	if (instance==this)
		instance=0;
}

void eStreamWatchdog::reloadSettings()
{
	last=-1;
	checkstate();
}

