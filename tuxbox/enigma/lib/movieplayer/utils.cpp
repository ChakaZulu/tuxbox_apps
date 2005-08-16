#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <netinet/in.h>
#include <lib/base/estring.h>
#include <lib/base/buffer.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PRIVATE_STREAM1  0xBD
#define PRIVATE_STREAM2  0xBF

#define AUDIO_STREAM_S   0xC0
#define AUDIO_STREAM_E   0xDF

#define VIDEO_STREAM_S   0xE0
#define VIDEO_STREAM_E   0xEF

#define TS_SIZE          188
#define IN_SIZE		 65424

int tcpOpen(eString serverIP, int serverPort)
{
	struct sockaddr_in ads;
	socklen_t adsLen;
	int fd = -1;
	int rc = -1;

	bzero((char *)&ads, sizeof(ads));
	ads.sin_family = AF_INET;
	ads.sin_addr.s_addr = inet_addr(serverIP.c_str());
	ads.sin_port = htons(serverPort);
	adsLen = sizeof(ads);

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) != -1)
	{
		if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
			eDebug("[MOVIEPLAYER] tcpOpen: fcntl failed.");
		int retry = 100;
		while (retry > 0)
		{
			rc = connect(fd, (struct sockaddr *)&ads, adsLen);
			if (rc < 0)
			{
				usleep(10000);
				retry--;
			}
			else
				retry = 0;
		}
		if (rc < 0)
		{
			close(fd);
			fd = -1;
		}
		else
			eDebug("[MOVIEPLAYER] tcpOpen: socket fd = %d", fd);
	}

	return fd;
}

int tcpRequest(int fd, char *ioBuffer, int maxLength)
{
	int rc = -1;
	char *p = ioBuffer;
	char temp[512];

	int wr = write(fd, ioBuffer, strlen(ioBuffer));
	eDebug("[MOVIEPLAYER] tcpRequest: fd = %d, wr = %d", fd, wr);
	int retry = 100;
	int rd = -1;
	while (retry > 0)
	{
		rd = read(fd, temp, sizeof(temp) - 1);
		if (rd <= 0)
		{
			retry--;
			usleep(10000);
		}
		else
		{
			int l = maxLength - (p - ioBuffer);
			strncpy(p, temp, l);
			if (l < rd)
			{
				p += l;
				retry = 0;
			}
			else
			{
				p += rd;
				retry = 100;
			}
			rc = 0;
		}
	}
	*p = '\0';
	eDebug("[MOVIEPLAYER] tcpRequest: rd = %d, rc = %d, response = %s", p - ioBuffer, rc, ioBuffer);
	return rc;
}

#define PID_MASK_HI 0x1F
uint16_t get_pid(uint8_t *pid)
{
	uint16_t pp = 0;

	pp = (pid[0] & PID_MASK_HI) << 8;
	pp |= pid[1];

	return pp;
}

void find_avpids(eIOBuffer *tsBuffer, int *vpid, int *apid, int *ac3)
{
	unsigned char buffer[IN_SIZE];
	int count;
	int i;
	int offset = 0;
	int bufferSize = tsBuffer->size();
	
	*apid = -1; *vpid = -1; *ac3 = -1;
        while (bufferSize > 0)
	{
		int toRead = (bufferSize < IN_SIZE) ? bufferSize : IN_SIZE;
		count = tsBuffer->read(buffer, toRead);
		bufferSize -= count;
		tsBuffer->write(buffer, count);
//		eDebug("[MOVIEPLAYER] find_avpids: bufferSize = %d, toRead = %d, count = %d, tsBuffer.size() = %d", bufferSize, toRead, count, tsBuffer->size());
		for (i = 0; i < (count - 7) && (*apid == -1 || *vpid == -1); i++)
		{
			if (buffer[i] == 0x47)
			{
				if (buffer[i + 1] & 0x40)
				{
					offset = 0;
					if (buffer[3 + i] & 0x20) //adapt field?
						offset = buffer[4 + i] + 1;
					switch (buffer[i + 7 + offset])
					{
						case VIDEO_STREAM_S ... VIDEO_STREAM_E:
						{
							*vpid = get_pid(buffer + i + 1);
							break;
						}
						case AUDIO_STREAM_S ... AUDIO_STREAM_E:
						{
							*apid = get_pid(buffer + i + 1);
							*ac3 = 0;
							break;
						}
						case PRIVATE_STREAM1:
						{
							*apid = get_pid(buffer + i + 1);
							*ac3 = 1;
							break;
						}
					}
				}
				i += 187;
			}
		}
	}
	eDebug("[MOVIEPLAYER] found apid: 0x%04X, vpid: 0x%04X, ac3: %d", *apid, *vpid, *ac3);
}
