/******************************************************************************\
|	Neutrino-GUI  -   DBoxII-Project                                       |
|                                                                              |
|	Copyright (C) 2004 by Sanaia <sanaia at freenet dot de>                |
|	netfile - remote file access mapper                                    |
|                                                                              |
|                                                                              |
| description:                                                                 |
|	netfile - a URL capable wrapper for the fopen() call                   |
|                                                                              |
| usage:                                                                       |
|	include 'netfile.h' in your code as *LAST* file after all other        |
|	include files and add netfile.c to your sources. That's it. The        |
|	include file maps the common fopen() call onto a URL capable           |
|	version that handles files hosted on http servers like local ones.     |
|                                                                              |
|                                                                              |
| examples:                                                                    |
|                                                                              |
| automatic redirection example:                                               |
|                                                                              |
|	fd = fopen("remotefile.url", "r");                                     |
|	                                                                       |
|	This is a pretty straight forward implementation that should           |
|	even work with applications which refuse to accept filenames           |
|	starting with protocol headers (i.e. 'http://'). If the                |
|	given file ends with '.url', then the file is opened and read          |
|	and its content is assumed to be a valid URL which opened then.        |
|	This works for all protocols provided by this code, e.g. 'http://',    |
|	'icy://' and 'scast://'                                                |
|	All restrictions apply for the protocols themself as follows.          |
|                                                                              |
|                                                                              |
| http example:                                                                |
|                                                                              |
|	fd = fopen("http://find.me:666/somewhere/foo.bar", "r");               |
|	                                                                       |
|	This opens the specified file on a webserver (read only).              |
|	NOTE: the stream itself is bidirectional, you can write                |
|	into it (e.g. commands for communication with the server),             |
|	but this is neither supported nor recommended. The result              |
|	of such an action is rather undefined. You *CAN NOT* make              |
|	any changes to the file of the webserver !                             |
|	                                                                       |
|	                                                                       |
| shoutcast example:                                                           |
|                                                                              |
|	fd = fopen("scast://666", "r");                                        |
|	                                                                       |
|	This opens a shoutcast sation; all you need to know is the             |
|	station number. The query of the shoutcast directory and the           |
|	lookup of a working server is done automatically. The stream           |
|	is opened read-only.                                                   |
|	I recommend this for official shoutcast stations.                      |
|                                                                              |
| shoutcast example:                                                           |
|                                                                              |
|	fd = fopen("icy://find.me:666/funky/station/", "r");                   |
|	                                                                       |
|	This is a low level mechanism that can be used for all                 |
|	shoutcast servers, but it mainly is intended to be used for            |
|	private radio stations which are not listed in the official            |
|	shoutcast directory. The stream is opened read-only.                   |
|	                                                                       |
| NOTE: All network accesses are only possible read only. Although some        |
|       protocols could allow write access to the remote resource, this        |
|       is not (and rather unlikely to be anytime) implemented here.           |
|       All remote accesses are made through a FIFO caching mechanism that     |
|       uses read-ahead caching (as far as possible). The fopen() call starts  |
|       a background fetching thread that fills the cache up to the ceiling.   |
|                                                                              |
|                                                                              |
|	License: GPL                                                           |
|                                                                              |
|	This program is free software; you can redistribute it and/or modify   |
|	it under the terms of the GNU General Public License as published by   |
|	the Free Software Foundation; either version 2 of the License, or      |
|	(at your option) any later version.                                    |
|                                                                              |
|	This program is distributed in the hope that it will be useful,        |
|	but WITHOUT ANY WARRANTY; without even the implied warranty of         |
|	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          |
|	GNU General Public License for more details.                           |
|                                                                              |
|	You should have received a copy of the GNU General Public License      |
|	along with this program; if not, write to the Free Software            |
|	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.              |
|                                                                              |
\******************************************************************************/
#include "netfile.h"

/*
TODO:
	- follow redirection errors (server error code 302)
	- support for automatic playlist processing (shoutcast pls files)
	- stream filtering (e.g. separation of mp3 & metadata 
*/

#define STATIC /**/

#ifdef fopen
	#undef fopen
#endif
#ifdef fclose
	#undef fclose
#endif
#ifdef fread
	#undef fread
#endif
#ifdef ftell
	#undef ftell
#endif
#ifdef rewind
	#undef rewind
#endif
#ifdef fseek
	#undef fseek
#endif

char err_txt[2048];	/* human readable error message */
static int debug = 0;	/* print debugging output or not */

STATIC STREAM_CACHE cache[CACHEENTMAX];

static int  ConnectToServer(char *hostname, int port);
static int  parse_response(int fd, void *);
static int  request_file(URL *url);
static void readln(int, char *);
static int  getCacheSlot(FILE *fd);
static int  push(FILE *fd, char *buf, long len);
static int  pop(FILE *fd, char *buf, long len);
static void CacheFillThread(void *url);
static void ShoutCAST_MetaFilter(STREAM_FILTER *);
static void getOpts(void);

/***************************************/
/* this is a simple options parser     */

void getOpts()
{
	char *dirs[] = { "/var/etc", ".", NULL};
	char buf[4096];
	int i;
	FILE *fd = 0;

	for(i=0; dirs[i] != NULL; i++)
	{
		sprintf(buf, "%s/.netfile", dirs[i]);
		dprintf(stderr, "trying %s\n", buf);
		fd = fopen(buf, "r");
		if(fd) break;
	}

	if(!fd) return;
	fread(buf, sizeof(char), 4095, fd);
	fclose(fd);

	if(strstr(buf, "debug=1"))
		debug = 1;

}

/***************************************/
/* networking functions                */

int ConnectToServer(char *hostname, int port)
{
	struct hostent *host;
	struct sockaddr_in sock;
	int fd, addr;

	dprintf(stderr, "looking up hostname: %s\n", hostname);

	host = gethostbyname(hostname); 

	if(host == NULL)
	{
		herror(err_txt);
		return -1;
	}

	addr = htonl(*(int *)host->h_addr);

	dprintf(stderr, "connecting to %s [%d.%d.%d.%d], port %d\n", host->h_name, 
			  (addr & 0xff000000) >> 24,
			  (addr & 0x00ff0000) >> 16,
			  (addr & 0x0000ff00) >>  8,
			  (addr & 0x000000ff), port);

	fd = socket(AF_INET, SOCK_STREAM, 0);

	if(fd == -1)
	{
		strcpy(err_txt, strerror(errno));
		return -1;
	}

	bzero(&sock, sizeof(sock));
	bcopy(host->h_addr, (char*)&sock.sin_addr, host->h_length);

	sock.sin_family = AF_INET;
	sock.sin_port = htons(port);

	if( connect(fd, (struct sockaddr *)&sock, sizeof(sock)) == -1 )
	{
		strcpy(err_txt, strerror(errno));
		return -1;
	}

	return fd;
}

/*********************************************/
/* request a file from the HTTP server       */
/* the network stream must be opened already */
/* and the URL structure be initialized      */
/* properly !                                */

int request_file(URL *url)
{
	char str[255];
	int slot, rval;
	ID3 id3;

	slot = getCacheSlot(url->stream);

	switch(url->proto_version)
	{
		/* send a HTTP/1.0 request */
		case HTTP10: {
				sprintf(str, "GET http://%s:%d%s\n", url->host, url->port, url->file);
				dprintf(stderr, "> %s", str);
				send(url->fd, str, strlen(str), 0);
			}
			break;

			/* send a HTTP/1.1 request */
		case HTTP11: {
				sprintf(str, "GET %s HTTP/1.1\r\n", url->file);
				dprintf(stderr, "> %s", str);
				send(url->fd, str, strlen(str), 0);
				sprintf(str, "Host: %s\r\n", url->host);
				dprintf(stderr, "> %s", str);
				send(url->fd, str, strlen(str), 0);

				sprintf(str, "Connection: close\r\n");
				dprintf(stderr, "> %s", str);
				send(url->fd, str, strlen(str), 0);

				sprintf(str, "User-Agent: %s\r\n\r\n", "Mozilla/4.0");
				dprintf(stderr, "> %s", str);
				send(url->fd, str, strlen(str), 0);

				rval = parse_response(url->fd, NULL);

//		  dprintf(stderr, "server response parser: return value = %d\n", rval);

				/* if the header indicated a zero length document or an */
				/* error, then close the cache */
				if(rval <= 0)
					cache[slot].closed = 1;

				/* return on error */
				if( rval < 0 )
					return -1;

			}
			break;

			/* send a SHOUTCAST request */
		case SHOUTCAST:{
				int meta_int;
				static STREAM_FILTER f_arg;

				sprintf(str, "GET %s HTTP/1.0\r\n", url->file);
				dprintf(stderr, "> %s", str);
				send(url->fd, str, strlen(str), 0);
				sprintf(str, "Host: %s\r\n", url->host);
				dprintf(stderr, "> %s", str);
				send(url->fd, str, strlen(str), 0);
//		  sprintf(str, "icy-metadata: 1\r\n", url->host);
//		  dprintf(stderr, "> %s", str);
//		  send(url->fd, str, strlen(str), 0);
				sprintf(str, "User-Agent: %s\r\n\r\n", "RealPlayer/4.0");
				dprintf(stderr, "> %s", str);
				send(url->fd, str, strlen(str), 0);

				if( (meta_int = parse_response(url->fd, &id3)) < 0)
					return -1;

				//meta_int = 16000;

				if(meta_int)
				{
					/* hook in the filter function if there is meta */
					/* data present in the stream */
					cache[slot].filter = ShoutCAST_MetaFilter;
					cache[slot].filter_arg = &f_arg;

					/* this is a *really bad* way to pass along the argument, */
					/* since we convert the integer into a pointer instead of */
					/* passing along a pointer/reference !*/
					f_arg.arg = (void*)meta_int;
				}

				/* push the created ID3 header into the stream cache */
				push(url->stream, (char*)&id3, id3.len);

			}
			break;
	}

	/* now it get's nasty ;) */
	/* create a thread that continously feeds the cache */
	/* with the data it fetches from the network stream */

	pthread_attr_init(&cache[slot].attr);
	pthread_mutex_init(&cache[slot].cache_lock, &cache[slot].cache_lock_attr);
	pthread_create(
					  &cache[slot].fill_thread, 
					  &cache[slot].attr, 
					  (void *(*)(void*))&CacheFillThread, (void*)&cache[slot]);

	/* now we do not care any longer about fetching new data,*/
	/* but we can not be shure that the cache is filled with */
	/* enough stuff ! */

	return 0;
}

/***************************************/
/* parse the server response (header)  */
/* and translate possible errors into  */
/* local syserror numbers              */

#define getHeaderVal(a,b) { \
  char *ptr; \
  ptr = strstr(header, a); \
  if(ptr) b = atoi( strchr(ptr, ' ') + 1); \
  else b = -1; }

void readln(int fd, char *buf)
{
	for(recv(fd, buf, 1, 0); (buf && isalnum(*buf)); recv(fd, ++buf, 1, 0));
	if(buf) *buf = 0;
}

int parse_response(int fd, void *opt)
{
	char header[2049], str[255];
	char *ptr, chr=0, lastchr=0;
	int hlen = 0, response;
	int meta_interval = 0, rval;
	ID3 *id3 = (ID3*)opt;

	bzero(header, 2048);
	ptr = header;

	/* extract the http header from the stream */
	do
	{
		recv(fd, ptr, 1, 0);

		/* skip all 'CR' symbols */
		if(*ptr != 13)
		{
			lastchr = chr; chr = *ptr++; hlen++;
		}

	} while((hlen < 2048) && ( ! ((chr == 10) && (lastchr == 10))));

	*ptr = 0;

	dprintf(stderr, "----------\n%s----------\n", header);

	/* parse the header fields */
	ptr = strstr(header, "HTTP/1.1");

	if(!ptr)
		ptr = strstr(header, "HTTP/1.0");

	if(!ptr)
		ptr = strstr(header, "ICY");

	/* no valid HTTP/1.1 or SHOUTCAST response */
	if(!ptr)	return -1;

	response = atoi(strchr(ptr, ' ') + 1);

	switch(response)
	{
		case 200: errno = 0;
			break;

		case 301: /* 'file permanently moved' error */
		case 404: /* 'file not found' error */
			errno = ENOENT;
			strcpy(err_txt, ptr);
			return -1;
			break;

		default:  errno = ENOPROTOOPT; 
			dprintf(stderr, "unknown server response code: %d\n", response);
			return -1;
	}

	/* if we got a content length from the server, i.e. rval >= 0 */
	/* then return now with the content length as return value */
	getHeaderVal("Content-Length:", rval);
//    dprintf(stderr, "file size: %d\n", rval);
	if(rval >= 0) return rval;

	/* yet another hack: this is only implemented to be able to fetch */
	/* the playlists from shoutcast */
	if(strstr(header, "Transfer-Encoding: chunked"))
	{
		readln(fd, str);
		sscanf(str, "%x", &rval);
//    dprintf(stderr, "%s %d\n", str, rval);
		return rval;
	}

	/* no content length indication from the server ? Then treat it as stream */
	getHeaderVal("icy-metaint:", meta_interval);
	if(meta_interval < 0) meta_interval = 0;

	/********************* dirty hack ***********************/
	/* we parse the stream header sent by the server and    */
	/* build based on this information an arteficial id3    */
	/* header that is pushed into the streamcache before    */
	/* any data from the stream is fed into the cache. This */
	/* makes the stream look like an MP3 and we have the    */
	/* station information in the display of the player :)) */

	if(id3)
	{
		int cnt = 0, fcnt = 0;
		ID3_frame id3frame;
		uint32_t sz;
		char *ptr, station[2048], desc[2048], str[2048];

		bcopy("ID3", id3->magic, 3);
		id3->version[0] = 3;
		id3->version[1] = 0;

#define SSIZE(a) (\
   (((a) & 0x0000007f) << 0) | (((a) & 0x00003f80) << 1) | \
   (((a) & 0x001fc000) << 2) | (((a) & 0xfe000000) << 3))

#define FRAME(b,c) {\
  strcpy(id3frame.id, (b)); \
  strcpy(id3frame.base, (c)); \
  id3frame.size = strlen(id3frame.base); \
  fcnt = 11 + id3frame.size; }

		ptr = strstr(header, "icy-name:");
		if(ptr)
		{
			ptr = strchr(ptr, ':') + 1;
			for(; ((*ptr == '-') || (*ptr == ' ')); ptr++);
			strcpy(station, ptr);
			*strchr(station, '\n') = 0;

			ptr = strchr(station, '-');
			if(ptr)
			{
				*ptr = 0;
				for(ptr++; ((*ptr == '-') || (*ptr == ' ')); ptr++);
				strcpy(desc, ptr);
			}

			FRAME("TPE1", station);
			id3frame.size = SSIZE(id3frame.size + 1);
			bcopy(&id3frame, id3->base + cnt, fcnt);
			cnt += fcnt;

			FRAME("TALB", desc);
			id3frame.size = SSIZE(id3frame.size + 1);
			bcopy(&id3frame, id3->base + cnt, fcnt);
			cnt += fcnt;
		}

		ptr = strstr(header, "icy-genre:");
		if(ptr)
		{
			ptr = strchr(ptr, ':') + 1;
			for(; ((*ptr == '-') || (*ptr == ' ')); ptr++);
			strcpy(str, ptr);
			*strchr(str, '\n') = 0;

			FRAME("TIT2", str);
			id3frame.size = SSIZE(id3frame.size + 1);
			bcopy(&id3frame, id3->base + cnt, fcnt);
			cnt += fcnt;
		}

		FRAME("COMM", "dbox streamconverter");
		id3frame.size = SSIZE(id3frame.size + 1);
		bcopy(&id3frame, id3->base + cnt, fcnt);
		cnt += fcnt;

		sz = 14 + cnt - 10;

		id3->size[0] = (sz & 0xfe000000) >> 21;
		id3->size[1] = (sz & 0x001fc000) >> 14;
		id3->size[2] = (sz & 0x00003f80) >> 7;
		id3->size[3] = (sz & 0x0000007f) >> 0;

		id3->len = 14 + cnt;
	}

	return meta_interval;
}

/******* wrapper functions for the f*() calls ************************/

#define transport(a,b,c) \
    if(strstr(url.url, a)) \
    { \
      url.access_mode = b; \
      url.proto_version = c; \
    }

FILE *f_open(const char *filename, const char *type)
{
	URL url;
	FILE *fd;
	int i;
	char *ptr = NULL, buf[4096];

	getOpts();			 /* read some options from the options-file */

	bzero(&url, sizeof(URL));
	strcpy(url.url, filename);
	url.port = 80;		 /* default port is 80 */
	url.proto_version = HTTP11; /* default protocol is HTTP/1.1 */

	/* remove leading spaces */
	for(ptr = url.url; (ptr != NULL) && ((*ptr == ' ') || (*ptr == '	')); ptr++);

	if(ptr != url.url)
		strcpy(url.url, ptr);

	/* did we get an URL file as argument ? If so, then open */
	/* this file, read out the url and open the url instead */
#ifndef DISABLE_URLFILES
//#warning using instant url file processing - this might not be what you want ! If you dont want this, define DISABLE_URLFILES=1
	if(
	  strstr(filename, ".url") || 
	  strstr(filename, ".imu")	  /* image URL - analog m3u */
	  )
	{
		fd = fopen(filename, "r");

		if(fd)
		{
			fread(buf, sizeof(char), 4095, fd);
			fclose(fd);

			ptr = strstr(buf, "://"); 

			if(!ptr)
			{
				dprintf(stderr, "Ups! File doesn't seem to contain any URL !\nbuffer:%s\n", buf);
				return NULL;
			}

			ptr = strchr(buf, '\n');
			if(ptr)
				*ptr = 0;

			sprintf(url.url, "%s", buf);
		}
		else
			return NULL;
	}
#endif

	/* now lets see what we have ... */

	ptr = strstr(url.url, "://");

	if(!ptr)
	{
		url.access_mode = MODE_FILE;
		strcpy(url.file, url.url);
		url.host[0] = 0;
		url.port = 0;
	}
	else
	{
		strcpy(url.host, ptr + 3);

		/* select the appropriate transport modes */
		transport("http",  MODE_HTTP, HTTP11);
		transport("icy",   MODE_HTTP, SHOUTCAST);
		transport("scast", MODE_SCAST, SHOUTCAST);

		/* if we fetch a playlist file, then set the access mode */
		/* that it will be parsed and processed automatically. If */
		/* it does not fail, then the call returns with an opened stream */

/* this currently results in an endless loop due to recursive calls of f_open()
	 if((url.access_mode == HTTP11) && (strstr(url.url, ".pls")))
	 {
		url.access_mode = MODE_PLS;
		url.proto_version = SHOUTCAST;
	 }
*/    

		/* extract the file path from the url */
		ptr = strchr(ptr + 3, '/');
		if(ptr) strcpy(url.file, ptr);
		else	  strcpy(url.file, "/");

		/* extract the host part from the url */
		ptr = strchr(url.host, '/');
		if(ptr) *ptr = 0;

		ptr = strrchr(url.host, ':');

		if(ptr)
		{
			url.port = atoi(ptr + 1);
			*ptr = 0;
		}
	}

	dprintf(stderr, "URL  to open: %s, access mode %s%s\n", 
			  url.url, 
			  (url.access_mode == MODE_HTTP) ? "HTTP" : 
			  (url.access_mode == MODE_SCAST) ? "SHOUTCAST" :
			  (url.access_mode == MODE_PLS) ? "PLAYLIST" : "FILE",
			  (url.access_mode != MODE_FILE) ? (
														  (url.proto_version == 0) ? "/1.0" : 
														  (url.proto_version == 1) ? "/1.1" :
														  (url.proto_version == 2) ? "/SHOUTCAST" : "") : ""
			 );
	dprintf(stderr, "FILE to open: %s, access mode: %d\n", url.file, url.access_mode);

	switch(url.access_mode)
	{
		case MODE_HTTP: 
			{
				int retries=10;
				do
				{
					url.fd = ConnectToServer(url.host, url.port);
					retries--;
				} while( url.fd < 0 && retries > 0);

				/* if the stream could not be opened, then indicate */
				/* an 'No such device or address' error */
				if(url.fd < 0)
				{
					fd = NULL;
					errno = ENXIO;
				}
				else
				{
					fd = fdopen(url.fd, "r+");

					url.stream = fd;

					if(!fd)
					{
						perror(err_txt);
					}
					else
					{
						/* look for a free cache slot */
						for(i=0; cache[i].cache; i++);

						cache[i].fd      = fd;
						cache[i].csize   = CACHESIZE;
						cache[i].cache   = (char*)malloc(cache[i].csize);
						cache[i].ceiling = cache[i].cache + CACHESIZE;
						cache[i].wptr    = cache[i].cache;
						cache[i].rptr    = cache[i].cache;
						cache[i].closed  = 0;
						cache[i].total_bytes_delivered = 0;
						cache[i].filter  = NULL;

						/* create the readable/writeable mutex locks */
						pthread_mutex_init(&cache[i].readable, &cache[i].readable_attr);
						pthread_mutex_init(&cache[i].writeable, &cache[i].writeable_attr);

						/* and set the empty cache to 'unreadable */
						pthread_mutex_lock( &cache[i].readable );

						/* but writeable. */
						pthread_mutex_unlock( &cache[i].writeable );

						/* did the request fail ? Then close the stream */
						if(request_file(&url) < 0)
						{
							/* we need out own f_close() function here, because everything */
							/* already has been set up and f_close() deinitializes it all correctly */
							f_close(url.stream);
							fd = NULL;
						}
					}
				}          
			}
			break;

		case MODE_SCAST:	 /* pseude transport mode; create the url to fetch the shoutcast */
			/* directory playlist, fetch it, parse it and try to open the   */
			/* stream url in it until we find one that works. The we open   */
			/* this one and return with the opened stream.                  */
			/* CAUTION: this is a nasty nested thing, because we call       */
			/* f_open() several times recursively - so the function should  */
			/* better be free from any bugs ;)                              */

			/* create the correct url from the station number */
			sprintf(url.url, "http://www.shoutcast.com/sbin/shoutcast-playlist.pls?rn=%s&file=filename.pls", url.host);
		case MODE_PLS:
			{
				char *ptr=NULL, *ptr2, buf[4096], servers[25][1024];
				int rval, i;
				int retries=15;


				/* fetch the playlist from the shoutcast directory with our own */
				/* url-capable f_open() call */
				do
				{
					fd = f_open(url.url, "r");

					if(!fd) break;

					/* read the playlist (we use the standard fopen() call from the */
					/* operating system because we don't need/want stream caching for */
					/* this operation */

					rval = fread(buf, sizeof(char), 4096, fd);
					f_close(fd);

					ptr = strstr(buf, "http://"); 
					retries--;
				} while(!ptr && retries > 0);
				/* extract the servers from the received playlist */
				if(!ptr)
				{
					fd = NULL;
					dprintf(stderr, "Ups! Playlist doesn't seem to contain any URL !\nbuffer:%s\n", buf);
					printf("Ups! Playlist doesn't seem to contain any URL !\nbuffer:%s\n", buf);
					return fd;
				}

				for( i=0; ((ptr != NULL) && (i<25)); ptr = strstr(ptr, "http://") )
				{
					strncpy(servers[i], ptr, 1023);
					ptr2 = strchr(servers[i], '\n');
					if(ptr2)	*ptr2 = 0;
					ptr = ptr2 + 1;
					dprintf(stderr, "server[%d]: %s\n", i, servers[i]);
					i++;
				}

				/* try to connect to all servers until we find one that */
				/* is willing to serve us */
				for(i=0, fd = NULL; ((fd == NULL) && (i<25)); i++)
				{
					sprintf(url.url, "icy%s", strstr(servers[i], "://"));
					fd = f_open(url.url, "r");
				}
			}
			break;

		case MODE_FILE: 
		default:         fd = fopen(url.file, type);
			break;
	}

	return fd;
}

int f_close(FILE *stream)
{
	int i, rval;

	/* lookup the stream ID in the cache table */
	i = getCacheSlot(stream);

	if(cache[i].fd == stream)
	{
		dprintf(stderr, "f_close: removing stream %x from the cache\n", stream);

		cache[i].closed = 1;		  /* indicate that tha cache is closed */
		pthread_join(cache[i].fill_thread, NULL);	/* wait for thread to terminate */
		rval = fclose(cache[i].fd);	  /* close the stream */
		free(cache[i].cache);	  /* free the cache */
		cache[i].csize = 0;		  /* set the cache size to zero */
		cache[i].fd = NULL;		  /* remove the stream ID from the cache */

		pthread_mutex_destroy(&cache[i].cache_lock);
		pthread_mutex_destroy(&cache[i].readable);
		pthread_mutex_destroy(&cache[i].writeable);
	}
	else
		rval = fclose(stream);

	return rval;
}

long f_tell(FILE *stream)
{
	int i;
	long rval;

	/* lookup the stream ID in the cache table */
	i = getCacheSlot(stream);

	if(cache[i].fd == stream)
		rval = cache[i].total_bytes_delivered;
	else
		rval = ftell(stream);

	return rval;
}

void f_rewind(FILE *stream)
{
	int i;

	/* lookup the stream ID in the cache table */
	i = getCacheSlot(stream);

	if(cache[i].fd == stream)
	{
		/* nothing to do */
	}
	else
		rewind(stream);
}

int f_seek(FILE *stream, long offset, int whence)
{
	int i, rval;

	/* lookup the stream ID in the cache table */
	i = getCacheSlot(stream);

	if(cache[i].fd == stream)
	{
		dprintf(stderr, "WARNING: program tries to seek on a stream !!\n");
		rval = -1;
	}
	else
		rval = fseek(stream, offset, whence);

	return rval;

}

size_t f_read (void *ptr, size_t size, size_t nitems, FILE *stream)
{
	int i, rval;

	/* lookup the stream ID in the cache table */
	i = getCacheSlot(stream);

	if(cache[i].fd == stream)
		rval = pop(stream, (char*)ptr, size * nitems);
	else
		rval = fread(ptr, size, nitems, stream);

	return rval;
}


/************************************************************************/
/* stream cache functions                                               */
/*                                                                      */
/* this section implements some functions to cache data from a          */
/* stream in a cache associated to the stream number. The cache         */
/* is created in the f_open() call and destroyed in the f_close()       */
/* call is the object to be opened is a network resource and not        */
/* a local file.                                                        */
/*                                                                      */
/* functions:                                                           */
/*            push(FILE *fd, char *buf, long len)                       */
/*                                  write the buffer pointed to by *buf */
/*                                  with len bytes into the cache for   */
/*                                  stream fd                           */ 
/*                                                                      */
/*            pop(FILE *fd, char *buf, long len)                        */
/*                                  fill the buffer pointed to by *buf  */
/*                                  with len bytes from the cache for   */
/*                                  stream fd                           */ 
/*                                                                      */
/*            getCacheSlot(FILE *fd)                                    */
/*                                                                      */
/*            CacheFillThread(void *c)                                  */
/*                                  feeds the cache with data from the  */
/*                                  stream                              */
/************************************************************************/

int getCacheSlot(FILE *fd)
{
	int i;

	for(i=0; ((i<CACHEENTMAX) && (cache[i].fd != fd)); i++);

	return i;
}

/* push a block of data into the stream cache */
int push(FILE *fd, char *buf, long len)
{
	int rval = 0, i, j;
	int blen = 0;

	i = getCacheSlot(fd);

	if(cache[i].closed)
		return -1;

//  dprintf(stderr, "push: %d bytes to store [filled: %d of %d], stream: %x\n", len, cache[i].filled, CACHESIZE, fd);

	if(cache[i].fd == fd)
	{
		do
		{
			if(cache[i].closed)
				return 0;

			/* try to obtain write permissions for the cache */
			/* this will block if the cache is full */
			pthread_mutex_lock( & cache[i].writeable );

			if((cache[i].csize - cache[i].filled))
			{
				int amt[2];

				/* prevent any modification to the cache by other threads, */
				/* mainly by the pop() function, while we write data to it */
				pthread_mutex_lock( &cache[i].cache_lock );

				/* block transfer length: get either what's there or */
				/* only as much as we need */
				blen = ((len - rval) > (cache[i].csize - cache[i].filled)) ? (cache[i].csize - cache[i].filled) : (len - rval);

				if(cache[i].wptr < cache[i].rptr)
				{
					amt[0] = cache[i].rptr - cache[i].wptr;
					amt[1] = 0;
				}
				else
				{
					amt[0] = cache[i].ceiling - cache[i].wptr;
					amt[1] = cache[i].rptr - cache[i].cache;
				}

				for(j=0; j<2; j++)
				{
					if(amt[j] > blen)	amt[j] = blen;

					if(amt[j])
					{
						bcopy(buf, cache[i].wptr, amt[j]);
						cache[i].wptr = cache[i].cache + 
											 (((int)(cache[i].wptr - cache[i].cache) + amt[j]) % cache[i].csize);

						buf += amt[j];	  /* adjust the target buffer pointer */
						rval += amt[j];  /* increase the 'total bytes' counter */
						blen -= amt[j];  /* decrease the block length counter */
						cache[i].filled += amt[j];
					}
				}

//	dprintf(stderr, "push: %d/%d bytes written [filled: %d of %d], stream: %x\n", amt[0] + amt[1], rval, cache[i].filled, CACHESIZE, fd);

				pthread_mutex_unlock( &cache[i].cache_lock );

				/* unlock the cache for read access, if it */
				/* contains some data */
				if(cache[i].filled)
					pthread_mutex_unlock( &cache[i].readable );
			}

			/* if there is still space in the cache, unlock */
			/* it again for further writing by anyone */
			if(cache[i].csize - cache[i].filled)
				pthread_mutex_unlock( & cache[i].writeable );
			else
				dprintf(stderr, "push: buffer overrun; cache full - leaving cache locked\n");

		} while(rval < len);
	}
	else
	{
		dprintf(stderr, "push: no cache present for stream %0x\n", fd);
		rval = -1;
	}

//  dprintf(stderr, "push: exitstate: [filled: %d of %d], stream: %x\n", cache[i].filled, CACHESIZE, fd);
	dprintf(stderr, "push: exitstate: [filled: %3.1f %%], stream: %x\r", 100.0 * (float)cache[i].filled / (float)cache[i].csize, fd);

	return rval;
}

int pop(FILE *fd, char *buf, long len)
{
	int rval = 0, i, j;
	int blen = 0;

	i = getCacheSlot(fd);

	dprintf(stderr, "pop: %d bytes requested [filled: %d of %d], stream: %x\n", len, cache[i].filled, CACHESIZE, fd);

	if(cache[i].fd == fd)
	{
		do
		{
			if(cache[i].closed && (!cache[i].filled) )
				return 0;

			/* try to obtain read permissions for the cache */
			/* this will block if the cache is empty */
			pthread_mutex_lock( & cache[i].readable );

			if(cache[i].filled)
			{
				int amt[2];

				/* prevent any modification to the cache by other threads, */
				/* mainly by the push() function, while we read data from it */
				pthread_mutex_lock( &cache[i].cache_lock );

				/* block transfer length: get either what's there or */
				/* only as much as we need */
				blen = ((len - rval) > cache[i].filled) ? cache[i].filled : (len - rval);

				/* if the cache is closed, just read the remaining data */
				if(cache[i].closed)
					blen = cache[i].filled, len = blen;

				if(cache[i].rptr < cache[i].wptr)
				{
					amt[0] = cache[i].wptr - cache[i].rptr;
					amt[1] = 0;
				}
				else
				{
					amt[0] = cache[i].ceiling - cache[i].rptr;
					amt[1] = cache[i].wptr - cache[i].cache;
				}

				for(j=0; j<2; j++)
				{
					if(amt[j] > blen)	amt[j] = blen;

					if(amt[j])
					{
						bcopy(cache[i].rptr, buf, amt[j]);
						cache[i].rptr = cache[i].cache + 
											 (((int)(cache[i].rptr - cache[i].cache) + amt[j]) % cache[i].csize);

						buf += amt[j];	  /* adjust the target buffer pointer */
						rval += amt[j];  /* increase the 'total bytes' counter */
						blen -= amt[j];  /* decrease the block lengt counter */
						cache[i].filled -= amt[j];
					}
				}

//	dprintf(stderr, "pop: %d/%d/%d bytes read [filled: %d of %d], stream: %x\n", amt[0] + amt[1], rval, len, cache[i].filled, CACHESIZE, fd);

				/* if the cache is closed and empty, then */
				/* force the end contition to be met */
				if(cache[i].closed && (! cache[i].filled))
					len = rval;

				/* allow write access again */
				pthread_mutex_unlock( &cache[i].cache_lock );

				/* unlock the cache for write access, if it */
				/* has some free space */
				if(cache[i].csize - cache[i].filled)
					pthread_mutex_unlock( &cache[i].writeable );
			}

			/* if there is still data in the cache, unlock */
			/* it again for further reading by anyone */
			if(cache[i].filled)
				pthread_mutex_unlock( & cache[i].readable );
			else
			{
				dprintf(stderr, "pop: buffer underrun; cache empty - leaving cache locked\n");
			}

		} while(rval < len);
	}
	else
	{
		dprintf(stderr, "pop: no cache present for stream %0x\n", fd);
		rval = -1;
	}

//  dprintf(stderr, "pop: exitstate: [filled: %d of %d], stream: %x\n", cache[i].filled, CACHESIZE, fd);
//  dprintf(stderr, "pop:  exitstate: [filled: %3.1f %%], stream: %x\n", 100.0 * (float)cache[i].filled / (float)cache[i].csize, fd);

	cache[i].total_bytes_delivered += rval;

	return rval;
}


void CacheFillThread(void *c)
{
	char *buf;
	STREAM_CACHE *cache = (STREAM_CACHE*)c;
	int rval/*, corr_len*/;

	if(cache->closed)
		return;

	dprintf(stderr, "CacheFillThread: thread started, using stream %8x\n", cache->fd);

	buf = (char*)malloc(CACHEBTRANS);

	if(!buf)
	{
		dprintf(stderr, "CacheFillThread: fatal error ! Could not allocate memory. Terminating.\n");
		exit(-1);
	}

	/* endless loop; read a block of data from the stream */
	/* and push it into the cache */
	do
	{
		rval = fread(buf, 1, CACHEBTRANS, cache->fd);

		/* if there is a filter function set up for this stream, then */
		/* we need to call it with the propper arguments */
		if(cache->filter)
		{
			cache->filter_arg->buf = buf;
			cache->filter_arg->len = rval;
			cache->filter(cache->filter_arg);
		}

		push(cache->fd, buf, rval);
		dprintf(stderr, "CacheFillThread: close? %d\n", cache->closed);
	} while( (rval == CACHEBTRANS) && (! cache->closed) );

	/* close the cache if the stream disrupted */
	cache->closed = 1;

	pthread_mutex_unlock( &cache->writeable );
	pthread_mutex_unlock( &cache->readable );

	/* ... and exit this thread. */
	dprintf(stderr, "CacheFillThread: thread exited, stream %8x  \n", cache->fd);

	free(buf);
	pthread_exit(0);
}

/* the following is still non-functional */
void ShoutCAST_MetaFilter(STREAM_FILTER *arg)
{
	int *cnt = (int*)arg->user;
	int meta_int = (int)arg->arg;

	/* allocate the space for the counter variable */
	if(!cnt)
		cnt = (int*)calloc(1, sizeof(int));

	if((*cnt + arg->len) > meta_int)
		;

	dprintf(stderr, "filter : buffer: %x, len: %d\n", arg->buf, arg->len, (int)arg);
	dprintf(stderr, "filter : meta interval %d (%d)\n", meta_int, *cnt);

	*cnt = *cnt + arg->len;
}
