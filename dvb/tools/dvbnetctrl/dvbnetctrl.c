/*

   dvbnetctrl - a program for adding and removing a dvb net interface

   ripped from dvb_tune.c
   Copyright (C) Dave Chapman 2001-2004

   Copyright (C) Harald Küthe 2005

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
   Or, point your browser to http://www.gnu.org/copyleft/gpl.html

*/

// Linux includes:
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <unistd.h>

// DVB includes:
#include <linux/dvb/osd.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/video.h>
#include <linux/dvb/audio.h>
#include <linux/dvb/net.h>

#define DVB_NET_DEVICES_MAX	10	// in driver/dvb/drivers/media/dvb/dvb-core/dvb_net.h

int fd_demuxd;
int card=0;

char* frontenddev[4]={"/dev/dvb/adapter0/frontend0",
			"/dev/dvb/adapter1/frontend0",
			"/dev/dvb/adapter2/frontend0",
			"/dev/dvb/adapter3/frontend0"};
char* dvrdev[4]={"/dev/dvb/adapter0/dvr0",
		    "/dev/dvb/adapter1/dvr0",
		    "/dev/dvb/adapter2/dvr0",
		    "/dev/dvb/adapter3/dvr0"};
char* demuxdev[4]={"/dev/dvb/adapter0/demux0",
		    "/dev/dvb/adapter1/demux0",
		    "/dev/dvb/adapter2/demux0",
		    "/dev/dvb/adapter3/demux0"};

void set_dpid(ushort dpid)
{
	struct dmx_sct_filter_params sctFilterParams;

    if (dpid==0 || dpid==0xffff) {
        ioctl(fd_demuxd, DMX_STOP, dpid);
        return;
    }
    memset(&sctFilterParams.filter,0,sizeof(sctFilterParams.filter));
    sctFilterParams.pid = dpid;
	//sctFilterParams.filter.filter[0] = 0x3e;
    //sctFilterParams.filter.mask[0] = 0xff;
	sctFilterParams.timeout = 0;
    sctFilterParams.flags = DMX_IMMEDIATE_START;
    if (ioctl(fd_demuxd, DMX_SET_FILTER, &sctFilterParams) < 0)
        perror("set_dpid");
}


int main(int argc, char **argv)
{
//	int fd_dvr=0;
	int addpid=0;
	int delpid=0;
	int i;
	int add=0;
	int del=0;
	int show=0;

	if (argc==1) {
		fprintf(stderr,"Usage: dvbnetctrl [OPTIONS]\n\n");
	    fprintf(stderr,"Standard options:\n\n");
		fprintf(stderr,"-add dpid     Add network interface and receive MPE on PID dpid\n");
		fprintf(stderr,"-del dpid     Delete network interface on PID dpid\n");
		fprintf(stderr,"-show         Lists all DVB network interfaces and their PID\n");
		fprintf(stderr,"\n");
		return(-1);
	} else {
		for (i=1;i<argc;i++) {
			if (strcmp(argv[i],"-add")==0) { //
				i++;
				add++;
				addpid=atoi(argv[i]);
			} else if (strcmp(argv[i],"-del")==0) { //
				i++;
				del++;
				delpid=atoi(argv[i]);
			} else if (strcmp(argv[i],"-show")==0) { //
				i++;
				show++;
			}
		} // END for
	}
	if((fd_demuxd = open(demuxdev[card],O_RDWR)) < 0){
		fprintf(stderr,"FD %i: ",i);
		perror("DEMUX DEVICE: ");
		return -1;
	}

	if (add > 0 || del > 0 || show > 0) {
		char devnamen[80];
		int dev, fdn;
		struct dvb_net_if netif;
		int freenum=-1;

		dev = card;

		sprintf(devnamen,"/dev/dvb/adapter%d/net0",dev);
		//printf("Trying to open %s\n",devnamen);
		if((fdn = open(devnamen,O_RDWR|O_NONBLOCK)) < 0) {
			fprintf(stderr, "Failed to open DVB NET DEVICE");
		} else {
			if (del) {
				for (i=0; i<DVB_NET_DEVICES_MAX; i++)
				{
					netif.if_num = i;
					if (ioctl( fdn, NET_GET_IF, &netif) == 0)
					{
						printf("DVB network device num %d found on PID %d\n", i, netif.pid);
						if (netif.pid == delpid)
						{
							printf("PID matches\n");
		    	  			// Add the network interface
			    		  	if (ioctl( fdn, NET_REMOVE_IF, netif.if_num) == 0)
				    			printf("Successfully removed network device with PID %d\n", netif.pid);
							else
				    			perror("Error removing network device" );
							break;
						}
					}
				}
		  	}
	  		if (add) {
				for (i=0; i<DVB_NET_DEVICES_MAX; i++)
				{
					netif.if_num = i;
					if (ioctl( fdn, NET_GET_IF, &netif) == 0)
					{
						if (netif.pid == addpid)
						{
				    		printf("Error network device already exists\n");
							close (fdn);
							return(-1);
						}
					}
					else if (freenum<0) freenum = i; // always choosen the next free number
				}
				netif.if_num = freenum;
				netif.pid = addpid;
#ifdef __NET_ADD_IF_OLD
				netif.feedtype = DVB_NET_FEEDTYPE_MPE;
#endif
				// Add the network interface
		    	if (ioctl( fdn, NET_ADD_IF, &netif) == 0)
					printf("Successfully opened network device, please configure the dvb interface\n");
				else
					perror("NET_ADD_IF");
			}
			if (show) {
				for (i=0; i<DVB_NET_DEVICES_MAX; i++)
				{
					netif.if_num = i;
					if (ioctl( fdn, NET_GET_IF, &netif) == 0)
					{
				    	printf("DVB network device num %d found on PID %d\n", i, netif.pid);
					}
				}
			}
			close (fdn);
	    }
	}
	return(0);
}
