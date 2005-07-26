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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#define PROG_STREAM_MAP  0xBC
#ifndef PRIVATE_STREAM1
#define PRIVATE_STREAM1  0xBD
#endif
#define PADDING_STREAM   0xBE
#ifndef PRIVATE_STREAM2
#define PRIVATE_STREAM2  0xBF
#endif
#define AUDIO_STREAM_S   0xC0
#define AUDIO_STREAM_E   0xDF
#define VIDEO_STREAM_S   0xE0
#define VIDEO_STREAM_E   0xEF
#define ECM_STREAM       0xF0
#define EMM_STREAM       0xF1
#define DSM_CC_STREAM    0xF2
#define ISO13522_STREAM  0xF3
#define PROG_STREAM_DIR  0xFF

#define TS_SIZE        188
#define TRANS_ERROR    0x80
#define PAY_START      0x40
#define TRANS_PRIO     0x20
#define PID_MASK_HI    0x1F

#define IN_SIZE TS_SIZE*10
#define IPACKS 2048


uint16_t get_pid(uint8_t *pid)
{
	uint16_t pp = 0;

	pp = (pid[0] & PID_MASK_HI)<<8;
	pp |= pid[1];

	return pp;
}

int tcpRequest(int fd, char *ioBuf, int maxLen)
{
	int r;

	write(fd, ioBuf, strlen(ioBuf));
	r = read(fd, ioBuf, maxLen);
	if (r >= 0) 
		ioBuf[r] = '\0';

	return r;
}

int tcpOpen(eString serverIP, int serverPort)
{
	struct sockaddr_in ads;
	socklen_t adsLen;
	int fd;

	bzero((char *)&ads, sizeof(ads));
	ads.sin_family = AF_INET;
	ads.sin_addr.s_addr = inet_addr(serverIP.c_str());
	ads.sin_port = htons(serverPort);
	adsLen = sizeof(ads);

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) != -1)
	{
		if (connect(fd, (struct sockaddr *)&ads, adsLen) == -1)
		{
			close(fd);
			fd = -1;
		}
	}

	return fd;
}

size_t CurlDummyWrite (void *ptr, size_t size, size_t nmemb, void *data)
{
	std::string *pStr = (std::string *)data;
	*pStr += (char *)ptr;
	return size *nmemb;
}

CURLcode sendGetRequest (const eString& url, eString& response, bool useAuthorization) 
{
	CURL *curl;
	CURLcode httpres;

	curl = curl_easy_init ();
	curl_easy_setopt (curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, CurlDummyWrite);
	curl_easy_setopt (curl, CURLOPT_FILE, (void *)&response);
	if (useAuthorization)
		curl_easy_setopt (curl, CURLOPT_USERPWD, "admin:admin");	/* !!! make me customizable */
	curl_easy_setopt (curl, CURLOPT_FAILONERROR, true);
	httpres = curl_easy_perform (curl);
	eDebug("[MOVIEPLAYER] HTTP result: %d", httpres);
	curl_easy_cleanup (curl);
	return httpres;
}


void find_avpids(int fd, uint16_t *vpid, uint16_t *apid)
{
	uint8_t buf[IN_SIZE];
	int count;
	int i;
	int off = 0;
	
	*apid = 0; *vpid = 0;
        while (*apid == 0 || *vpid == 0)
	{
		count = read(fd, buf, IN_SIZE);
		if (count <= 0) 
			return;
		for (i = 0; i < count-7; i++)
		{
			if (buf[i] == 0x47)
			{
				if (buf[i + 1] & 0x40)
				{
					off = 0;
					if (buf[3 + i] & 0x20)//adapt field?
						off = buf[4 + i] + 1;
					switch(buf[i + 7 + off])
					{
						case VIDEO_STREAM_S ... VIDEO_STREAM_E:
							*vpid = get_pid(buf + i + 1);
							break;
						case PRIVATE_STREAM1:
						case AUDIO_STREAM_S ... AUDIO_STREAM_E:
							*apid = get_pid(buf + i + 1);
							break;
					}
				}
				i += 187;
			}
			if (*apid != 0 && *vpid != 0) 
				break;
		}
	}
}

int is_audio_ac3(int fd)
{
	uint8_t buf[IN_SIZE];
	int count;
	int i;
	int off = 0;
	int is_ac3 = -1;

	while (is_ac3 == -1)
	{
		count = read(fd, buf, IN_SIZE);
		if (count <= 0)
			break;
		for (i = 0; i < count - 7; i++)
		{
			if (buf[i] == 0x47)
			{
				if (buf[i + 1] & 0x40)
				{
					off = 0;
					if ( buf[3 + i] & 0x20)//adapt field?
						off = buf[4 + i] + 1;
					switch (buf[i + 7 + off])
					{
						case PRIVATE_STREAM1:
							is_ac3 = 1;
							break;
						case AUDIO_STREAM_S ... AUDIO_STREAM_E:
							is_ac3 = 0;
							break;
					}
				}
				i += 187;
			}
			if (is_ac3 >= 0) 
				break;
		}
	}
	return is_ac3;
}
