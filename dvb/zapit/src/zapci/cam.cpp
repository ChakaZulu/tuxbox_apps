#ifdef DVBS
#define USE_EXTERNAL_CAMD
#endif
#ifndef USE_EXTERNAL_CAMD

#include <fcntl.h>
#include <ost/ca.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <unistd.h>

#include <string>

#include "cam.h"

int get_caver ()
{
	uint8_t cmd[4];
	int step = 0;
	int camfd;
	
	char buffer[128];
	std::string cam_says;
	int v_pos;
	int pt;
	struct pollfd cam_pfd;

	cam_reset();
	usleep(10000);
	camfd=open("/dev/dbox/cam0", O_RDWR);
  
	if (camfd < 0)
	{
		perror("[cam.cpp] open");
		return -1;
	}
  
	cmd[0] = 0x50;
	cmd[1] = 0x81;
	cmd[2] = 0xF1;
	cmd[3] = 0x4E;

	if (write(camfd, &cmd, 4) < 0)
	{
		perror("[cam.cpp] write");
		close(camfd);
		return -1;
	}

	while (step < 10)
	{
		cam_pfd.fd = camfd;
		cam_pfd.events = POLLIN;
		cam_pfd.revents = 0;
      
		pt = poll(&cam_pfd, 1, 5000);
  
		switch (pt)
		{
		case -1:
			perror("[cam.cpp] poll");
			close(camfd);
			return -1;
		case 0:
			printf("[cam.cpp] poll timeout\n");
			close(camfd);
			return 128;
		}
  	
		memset(&buffer,0,sizeof(buffer));
		step++;

		if (read(camfd, &buffer, sizeof(buffer)) < 0)
		{
			perror("[cam.cpp] read");
			usleep(500);
			continue;
		}

		cam_says = buffer;
		v_pos = cam_says.find("01.01.00");
	
		if (v_pos >= 0)
		{
			close(camfd);

			switch (*cam_says.substr(v_pos + 9, 1).c_str())
			{
				case 'E':
					return 16;
				case 'D':
					return 32;
				case 'F':
					return 64;
				default:
					return 128;
			}
		}
	}

	close(camfd);
	return -1;
}

int get_caid ()
{
	ca_msg_t ca_msg;
	char cmd = 0x03;
	int caid = 0;
	int retries = 0;

	int step = 0;
	int camfd;

	char buffer[128];
	int i;
	int len;
	int csum;

	int newcaid;

	while ((caid == 0) && (retries < 3))
	{
		if (retries > 0)
		{
			usleep(100000);
			printf("[cam.cpp] trying to read CAID, try #%d\n", retries + 1);
		}
  
		camfd = open("/dev/ost/ca0", O_RDWR);
  
		if(camfd < 0)
		{
			perror("[cam.cpp] open");
			return -1;
		}
  
		cam_reset();
		usleep(10000);
  
		/* init ca message */
		ca_msg.index = 0;
		ca_msg.type = 0;
  
		writecam((unsigned char*) &cmd,1);
		while (step < 10)
		{
			step++;
			ca_msg.length = 4;

			if (ioctl(camfd, CA_GET_MSG, &ca_msg) < 0)
			{
				perror("[cam.cpp] CA_GET_MSG");
				break;
			}

			len = ca_msg.length;
      
			if (len <= 0)
			{
				usleep(500);
				//printf("Step: %d\n", step);
				continue;
			}
			else
			{
				memcpy(buffer,ca_msg.msg,ca_msg.length);
			}

			if ((buffer[0] != 0x6F) || (buffer[1] != 0x50))
			{
				printf("[cam.cpp] out of sync! %02x %02x %02x %02x\n", buffer[0], buffer[1], buffer[2], buffer[3]);
				break;
			}

			len = buffer[2] & 0x7F;

			ca_msg.length = len;

			if (ioctl(camfd, CA_GET_MSG, &ca_msg) < 0)
			{
				perror("[cam.cpp] CA_GET_MSG");
				break;
			}

			if ((int)ca_msg.length != len)
			{
				printf("[cam.cpp] invalid length");
				break;
			}
      
			memcpy(buffer + 4, ca_msg.msg, ca_msg.length);
      
			csum = 0;
			for (i = 0; i < len + 4; i++)
				csum ^= buffer[i];

			if (csum)
			{
				printf("[cam.cpp] checksum failed. packet was: ");
				for (i=0; i<len+4; i++) printf("%02x ", buffer[i]);
				printf("\n");
				continue;
			}
      
			if (buffer[3] == 0x23)
			{
				if ((unsigned)buffer[4] == 0x83)
				{
					newcaid = (buffer[6]<<8)|buffer[7];

					if (newcaid != caid)
					{
						printf("[zapit] CAID is: %04X\n", newcaid);
						close(camfd);
						return newcaid;
					}
				}
			}
			else
			{
				printf("[zapit] cam: no CAID found!\n");
			}
		}
		close(camfd);
		retries++;
	}
	return 0;
}
#endif /* DVBS */
