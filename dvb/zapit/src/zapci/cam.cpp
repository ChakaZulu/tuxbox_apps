#ifndef DVBS
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
	camfd = open(CAM_DEV, O_RDWR);
  
	if (camfd < 0)
	{
		perror("[cam.cpp] " CAM_DEV);
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
  
		camfd = open(CA_DEV, O_RDWR);
  
		if(camfd < 0)
		{
			perror("[cam.cpp] " CA_DEV);
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

int _writecamnu (uint8_t cmd, uint8_t *data, uint8_t len)
{
	int camfd;
	uint8_t buffer[256];
	int csum = 0;
	int i;
	int pt;
	struct pollfd cam_pfd;
	bool output = false;

	if((camfd = open(CAM_DEV, O_RDWR)) < 0)
	{
		perror("[cam.cpp] " CAM_DEV);
		close(camfd);
		return -1;
	}

	buffer[0] = 0x6E;
	buffer[1] = 0x50;
	buffer[2] = (len + 1) | ((cmd != 0x23) ? 0x80 : 0);
	buffer[3] = cmd;

	memcpy(buffer + 4, data, len);

	len += 4;

	for (i = 0; i < len; i++)
		csum ^= buffer[i];

	buffer[len++]=csum;

	if (write(camfd, buffer + 1, len - 1) <= 0)
	{
		perror("[zapit] cam write");
		close(camfd);
		return -1;
	}

	if (buffer[4] == 0x03)
	{
		close(camfd);
		return 0; // Let get_caid read the caid;
	}

#if 0
	if (buffer[4] == 0x84)
	{
  		close(camfd);
		return; //Setting emmpid. No answer expected.
	}
#endif

	if (buffer[4] == 0x0d)
		output = true;

	if (output)
	{
		printf("[cam.cpp] sending to cam: ");
		for (i = 0; i < len; i++) printf("%02X ", buffer[i]);
		printf("\n");
	}

	cam_pfd.fd = camfd;
	cam_pfd.events = POLLIN;
	cam_pfd.revents = 0;

	pt = poll(&cam_pfd, 1, 2000);

	if (!pt)
	{
		printf("[cam.cpp] poll timeout\n");
		close(camfd);
		return -1;
	}

	if (read(camfd, &buffer, sizeof(buffer)) < 0)
	{
		perror("[cam.cpp] read");
		close(camfd);
		return -1;
	}

	if (output)
	{
		printf("[cam.cpp] answer: ");
		for (i = 0; i < buffer[2] + 4; i++) printf("%02X ", buffer[i]);
		printf("\n");
	}

	close(camfd);
	return 0;
}

int writecam (uint8_t *data, uint8_t len)
{
	return _writecamnu(0x23, data, len);
}

int descramble (uint16_t original_network_id, uint16_t service_id, uint16_t unknown, uint16_t ca_system_id, dvb_pid_t ecm_pid, pids *decode_pids)
{
	uint8_t buffer[100];
	uint8_t i;
	uint8_t p;

	buffer[0] = 0x0D;
	buffer[1] = original_network_id >> 8;
	buffer[2] = original_network_id & 0xFF;
	buffer[3] = service_id >> 8;
	buffer[4] = service_id & 0xFF;
	buffer[5] = unknown >> 8;
	buffer[6] = unknown & 0xFF;
	buffer[7] = ca_system_id >> 8;
	buffer[8] = ca_system_id & 0xFF;
	buffer[9] = ecm_pid >> 8;
	buffer[10] = ecm_pid & 0xFF;
	buffer[11] = decode_pids->count_vpids + decode_pids->count_apids;

	p = 12;

	for(i = 0; i < decode_pids->count_vpids; i++)
  	{
		buffer[p++] = decode_pids->vpid >> 8;
		buffer[p++] = decode_pids->vpid & 0xFF;
		buffer[p++] = 0x80;
		buffer[p++] = 0;
	}

	for(i = 0; i < decode_pids->count_apids; i++)
	{
		buffer[p++] = decode_pids->apids[i].pid >> 8;
		buffer[p++] = decode_pids->apids[i].pid & 0xFF;
		buffer[p++] = 0x80;
		buffer[p++] = 0;
	}

	return writecam(buffer, p);
}

int cam_reset ()
{
	uint8_t buffer[1];
	buffer[0] = 0x9;
	return writecam(buffer, sizeof(buffer));
}

int setemm (uint16_t unknown, uint16_t ca_system_id, dvb_pid_t emm_pid)
{
	uint8_t buffer[7];
	buffer[0] = 0x84;
	buffer[1] = unknown >> 8;
	buffer[2] = unknown & 0xFF;
	buffer[3] = ca_system_id >> 8;
	buffer[4] = ca_system_id & 0xFF;
	buffer[5] = emm_pid >> 8;
	buffer[6] = emm_pid & 0xFF;
	return writecam(buffer, sizeof(buffer));
}

#endif /* DVBS */

