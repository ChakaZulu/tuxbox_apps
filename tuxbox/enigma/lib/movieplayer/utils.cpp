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

#define PRIVATE_STREAM1  0xBD
#define PRIVATE_STREAM2  0xBF

#define AUDIO_STREAM_S   0xC0
#define AUDIO_STREAM_E   0xDF

#define VIDEO_STREAM_S   0xE0
#define VIDEO_STREAM_E   0xEF

#define TS_SIZE          188
#define IN_SIZE          TS_SIZE*10

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
		curl_easy_setopt (curl, CURLOPT_USERPWD, "admin:admin");
	curl_easy_setopt (curl, CURLOPT_FAILONERROR, true);
	httpres = curl_easy_perform (curl);
	eDebug("[MOVIEPLAYER] HTTP result: %d", httpres);
	curl_easy_cleanup (curl);
	return httpres;
}

#define PID_MASK_HI 0x1F
uint16_t get_pid(uint8_t *pid)
{
	uint16_t pp = 0;

	pp = (pid[0] & PID_MASK_HI)<<8;
	pp |= pid[1];

	return pp;
}

void find_avpids(int fd, int *vpid, int *apid, int *ac3)
{
	unsigned char buffer[IN_SIZE];
	int count;
	int i;
	int offset = 0;
	
	*apid = -1; *vpid = -1; *ac3 = -1;
        while (*apid == -1 || *vpid == -1)
	{
		count = read(fd, buffer, IN_SIZE);
		if (count <= 0) 
			return;
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
}
