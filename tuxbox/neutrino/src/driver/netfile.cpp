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
	- follow redirection errors (server error codes 302, 301)
	- support for automatic playlist processing (shoutcast pls files)
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

char err_txt[2048];		/* human readable error message */
static int debug = 0;		/* print debugging output or not */
static int enable_metadata = 0;	/* allow shoutcast meta data streaming */
static int cache_size = 196608;/* default cache size; can be overridden at */
				/* runtime with an option in the options file */

STATIC STREAM_CACHE cache[CACHEENTMAX];
STATIC STREAM_TYPE stream_type[CACHEENTMAX];

static int  ConnectToServer(char *hostname, int port);
static int  parse_response(URL *url, void *, CSTATE*);
static int  request_file(URL *url);
static void readln(int, char *);
static int  getCacheSlot(FILE *fd);
static int  push(FILE *fd, char *buf, long len);
static int  pop(FILE *fd, char *buf, long len);
static void CacheFillThread(void *url);
static void ShoutCAST_MetaFilter(STREAM_FILTER *);
static void ShoutCAST_DestroyFilter(void *a);
static STREAM_FILTER *ShoutCAST_InitFilter(int);
static void ShoutCAST_ParseMetaData(char *, CSTATE *);

static void getOpts(void);

/***************************************/
/* this is a simple options parser     */

void getOpts()
{
  char *dirs[] = { "/var/etc", ".", NULL };
  char buf[4096], *ptr;
  int i;
  FILE *fd = NULL;
  
  for(i=0; dirs[i] != NULL; i++)
  {
    sprintf(buf, "%s/.netfile", dirs[i]);
    fd = fopen(buf, "r");
    if(fd) break;
  }

  if(!fd) return;
  fread(buf, sizeof(char), 4095, fd);
  fclose(fd);
  
  if(strstr(buf, "debug=1"))
    debug = 1;

  if(strstr(buf, "enable meta"))
    enable_metadata = 1;

  if((ptr = strstr(buf, "cachesize=")))
    cache_size = atoi(strchr(ptr, '=') + 1);

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

  /* get the cache slot for this stream. A negative return value */
  /* indicates that no cache has been set up for this stream */
  slot = getCacheSlot(url->stream);

  switch(url->proto_version)
  {
  /* send a HTTP/1.0 request */
  case HTTP10:	{
  		  sprintf(str, "GET http://%s:%d%s\n", url->host, url->port, url->file);
		  dprintf(stderr, "> %s", str);
		  send(url->fd, str, strlen(str), 0);
		 }
		 break;

  /* send a HTTP/1.1 request */
  case HTTP11:	{
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
		  
		  rval = parse_response(url, NULL, NULL);

//		  dprintf(stderr, "server response parser: return value = %d\n", rval);

		  /* if the header indicated a zero length document or an */
		  /* error, then close the cache, if there is any */
		  if((slot >= 0) && (rval <= 0))
		    cache[slot].closed = 1;
		  
		  /* return on error */
		  if( rval < 0 )
		    return -1;

		  }
		  break;

  /* send a SHOUTCAST request */
  case SHOUTCAST:{
  		    int meta_int;
		  CSTATE tmp;

  		  sprintf(str, "GET %s HTTP/1.0\r\n", url->file);
		  dprintf(stderr, "> %s", str);
		  send(url->fd, str, strlen(str), 0);
		  sprintf(str, "Host: %s\r\n", url->host);
		  dprintf(stderr, "> %s", str);
		  send(url->fd, str, strlen(str), 0);

		  if(enable_metadata)
		  {
		    sprintf(str, "icy-metadata: 1\r\n");
		    dprintf(stderr, "> %s", str);
		    send(url->fd, str, strlen(str), 0);
		  }

		  sprintf(str, "User-Agent: %s\r\n\r\n", "RealPlayer/4.0");
		  dprintf(stderr, "> %s", str);
		  send(url->fd, str, strlen(str), 0);

		  if( (meta_int = parse_response(url, &id3, &tmp)) < 0)
		    return -1;
		    
		  if(meta_int)
		  {
		     /* hook in the filter function if there is meta */
		     /* data present in the stream */
		     cache[slot].filter_arg = ShoutCAST_InitFilter(meta_int);
		     cache[slot].filter = ShoutCAST_MetaFilter;

		      /* this is a *really bad* way to pass along the argument, */
		      /* since we convert the integer into a pointer instead of */
		      /* passing along a pointer/reference !*/
		     if(cache[slot].filter_arg->state)
		       bcopy(&tmp, cache[slot].filter_arg->state, sizeof(CSTATE));
		  }

		  /* push the created ID3 header into the stream cache */
		  push(url->stream, (char*)&id3, id3.len);

		  }
		  break;
  }

  /* now it get's nasty ;) */
  /* create a thread that continously feeds the cache */
  /* with the data it fetches from the network stream */
  /* but *ONLY* if there is a cache slot for this stream at all ! */
  /* HINT: in compatibility mode no cache is configured */
 
  if(slot >= 0)
  {
    pthread_attr_init(&cache[slot].attr);
    pthread_create(
  	&cache[slot].fill_thread, 
	&cache[slot].attr, 
	(void *(*)(void*))&CacheFillThread, (void*)&cache[slot]);
  }
  
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
  if(ptr) \
  { \
    ptr = strchr(ptr, ':'); \
    for(;!isalnum(*ptr);ptr++); \
    b = atoi(ptr); \
  } else b = -1; }

#define getHeaderStr(a,b) { \
  char *ptr; \
  ptr = strstr(header, a); \
  if(ptr) \
  { \
    ptr = strchr(ptr, ':'); \
    for(;!isalnum(*ptr);ptr++); \
    strcpy(b, ptr); \
    ptr = strchr(b, '\n'); \
    if(ptr) *ptr = 0; \
  } }

void readln(int fd, char *buf)
{
  for(recv(fd, buf, 1, 0); (buf && isalnum(*buf)); recv(fd, ++buf, 1, 0));
  if(buf) *buf = 0;
}

int parse_response(URL *url, void *opt, CSTATE *state)
{
  char header[2049], str[255];
  char *ptr, chr=0, lastchr=0;
  int hlen = 0, response;
  int meta_interval = 0, rval;
  int fd = url->fd;
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
  if(!ptr) return -1;

  response = atoi(strchr(ptr, ' ') + 1);

  switch(response)
  {
  case 200:	errno = 0;
  		  break;

  case 301:	/* 'file moved' error */
  case 302:	/* 'file not found' error */
  		  errno = ENOENT;
  		  strcpy(err_txt, ptr);
		  return -1 * response;
		  break;
		  
  case 404:	/* 'file not found' error */
  		  errno = ENOENT;
  		  strcpy(err_txt, ptr);
		  return -1;
		  break;
		  
  default:	errno = ENOPROTOOPT; 
  		  dprintf(stderr, "unknown server response code: %d\n", response);
  		  return -1;
  }

  /* use 'audio/mpeg' as default type, in case we are not told otherwise */
  strcpy(str, "audio/mpeg");
  getHeaderStr("Content-Type:", str);
  f_type(url->stream, str);
  
  /* if we got a content length from the server, i.e. rval >= 0 */
  /* then return now with the content length as return value */
  getHeaderVal("Content-Length:", rval);
   // dprintf(stderr, "file size: %d\n", rval);
  if(rval >= 0) return rval;

  /* yet another hack: this is only implemented to be able to fetch */
  /* the playlists from shoutcast */
  if(strstr(header, "Transfer-Encoding: chunked"))
  {
    readln(fd, str);
    sscanf(str, "%x", &rval);
    return rval;
  }

  /* no content length indication from the server ? Then treat it as stream */
  getHeaderVal("icy-metaint:", meta_interval);
  if(meta_interval < 0) meta_interval = 0;

  getHeaderStr("icy-genre:", state->genre);
  getHeaderStr("icy-name:", state->station);
  getHeaderStr("icy-url:", state->station_url);
  getHeaderVal("icy-br:", state->bitrate);

  /********************* dirty hack ***********************/
  /* we parse the stream header sent by the server and    */
  /* build based on this information an arteficial id3    */
  /* header that is pushed into the streamcache before    */
  /* any data from the stream is fed into the cache. This */
  /* makes the stream look like an MP3 and we have the    */
  /* station information in the display of the player :)) */
  
#define SSIZE(a) (\
   (((a) & 0x0000007f) << 0) | (((a) & 0x00003f80) << 1) | \
   (((a) & 0x001fc000) << 2) | (((a) & 0xfe000000) << 3))

#define FRAME(b,c) {\
  strcpy(id3frame.id, (b)); \
  strcpy(id3frame.base, (c)); \
  id3frame.size = strlen(id3frame.base); \
  fcnt = 11 + id3frame.size; }

  if(id3)
  {
    int cnt = 0, fcnt = 0;
    ID3_frame id3frame;
    uint32_t sz;
    char *ptr, station[2048], desc[2048], str[2048];

    bcopy("ID3", id3->magic, 3);
    id3->version[0] = 3;
    id3->version[1] = 0;

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

FILE *f_open(const char *filename, const char *acctype)
{
  URL url;
  FILE *fd;
  int i, compatibility_mode = 0;
  char *ptr = NULL, buf[4096], type[10];

  if(acctype)
    strcpy(type, acctype);
  
  /* read some options from the options-file */
  getOpts();
  
  /* test if compatibility mode has been requested. */
  /* e.g. "rbc" = read, binary, without stream caching */
  if(strchr(type, 'c'))
  {
    compatibility_mode = 1;
    *strchr(type, 'c') = 0;
  }
  
  dprintf(stderr, "f_open: %s %s\n", (compatibility_mode) ? "(compatibility mode)" : "", filename);

  /* set default protocol and port */
  bzero(&url, sizeof(URL));
  url.proto_version = HTTP11;
  url.port = 80;
  strcpy(url.url, filename);

  /* remove leading spaces */
  for (ptr = url.url; (ptr != NULL) && ((*ptr == ' ') || (*ptr == '	')); ptr++);

  if(ptr != url.url)
    strcpy(url.url, ptr);

  /* did we get an URL file as argument ? If so, then open */
  /* this file, read out the url and open the url instead */
#ifndef DISABLE_URLFILES
  if( strstr(filename, ".url") || strstr(filename, ".imu"))
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

  if (!ptr)
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
    else    strcpy(url.file, "/");

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
  case MODE_HTTP:	{
  			   int retries = 10;
			   
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
			       /* in compatibility mode we must not use our own stream cache */
			       /* because the application makes use of their own f*() calls */
			       /* which we can not replace by our own functions and thus they'll */
			       /* interfere with each other. All we can do is to open the stream */
			       /* and return a valid stream descriptor */
			       if(!compatibility_mode)
			       {
			         /* look for a free cache slot */
			         for(i=0; ((i<CACHEENTMAX) && (cache[i].cache != NULL)); i++);
			       
			         /* no free cache slot ? return an error */
			         if(i == CACHEENTMAX)
			         {
			           sprintf(err_txt, "no more free cache slots. Too many open files.\n");
				   return NULL;
			         }
			       
			         dprintf(stderr, "f_open: adding stream %x to cache[%d]\n", fd, i);

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
			         dprintf(stderr, "f_open: creating mutexes\n");
			         pthread_mutex_init(&cache[i].cache_lock, &cache[i].cache_lock_attr);
			         pthread_mutex_init(&cache[i].readable,   &cache[i].readable_attr);
			         pthread_mutex_init(&cache[i].writeable,  &cache[i].writeable_attr);

			         /* and set the empty cache to 'unreadable' */
			         dprintf(stderr, "f_open: locking read direction\n");
			         pthread_mutex_lock( &cache[i].readable );
			     
			         /* but writeable. */
			         dprintf(stderr, "f_open: unlocking write direction\n");
			         pthread_mutex_unlock( &cache[i].writeable );
			       }
			       
			       /* send the file request and check it'S revurn value */
			       /* if it failed, then close the stream */
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
			 
  case MODE_SCAST:	/* pseude transport mode; create the url to fetch the shoutcast */
  			 /* directory playlist, fetch it, parse it and try to open the   */
			 /* stream url in it until we find one that works. The we open   */
			 /* this one and return with the opened stream.                  */
			 /* CAUTION: this is a nasty nested thing, because we call       */
			 /* f_open() several times recursively - so the function should  */
			 /* better be free from any bugs ;)                              */
			 
			 /* create the correct url from the station number */
			 sprintf(url.url, "http://www.shoutcast.com/sbin/shoutcast-playlist.pls?rn=%s&file=filename.pls", url.host);

  case MODE_PLS:	{
  			    char *ptr=NULL, *ptr2, buf[4096], servers[25][1024];
			    int rval, i, retries = 15;

			    /* fetch the playlist from the shoutcast directory with our own */
			    /* url-capable f_open() call. We need the compatibility mode for */
			    /* this because we will read the data with the standard OS fread() call */
			    do
			    {
			      fd = f_open(url.url, "rc");
			    
			      if(fd)
			      {
			        /* read the playlist (we use the standard fopen() call from the */
			        /* operating system because we don't need/want stream caching for */
			        /* this operation */

			        rval = fread(buf, sizeof(char), 4096, fd);
			        f_close(fd);

			        ptr = strstr(buf, "http://"); 
			      }

			      retries--;
			    
			    } while(!ptr && retries > 0);
			    
			    /* extract the servers from the received playlist */
			    if(!ptr)
			    {
			      dprintf(stderr, "Ups! Playlist doesn't seem to contain any URL !\nbuffer:%s\n", buf);
			      sprintf(err_txt, "Ups! Playlist doesn't seem to contain any URL !");
			      return NULL;
			    }
			    
			    for( i=0; ((ptr != NULL) && (i<25)); ptr = strstr(ptr, "http://") )
			    {
			      strncpy(servers[i], ptr, 1023);
			      ptr2 = strchr(servers[i], '\n');
			      if(ptr2) *ptr2 = 0;
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
  default:	       fd = fopen(url.file, type);

			 /* a smarter solution would be to get this info from /etc/mime.types */

  			 if(strstr(url.file, ".ogg")) f_type(fd, "audio/ogg");
  			 if(strstr(url.file, ".mp3")) f_type(fd, "audio/mpeg");
  			 if(strstr(url.file, ".mp2")) f_type(fd, "audio/mpeg");
  			 if(strstr(url.file, ".mpa")) f_type(fd, "audio/mpeg");
  			 if(strstr(url.file, ".wav")) f_type(fd, "audio/wave");
  			 if(strstr(url.file, ".aif")) f_type(fd, "audio/aifc");
  			 if(strstr(url.file, ".snd")) f_type(fd, "audio/snd");

  			 break;
  }
  
  return fd;
}

int f_close(FILE *stream)
{
  int i, rval;
  
  /* at first, lookup the stream in the stream type table and remove it */
  for(i=0 ; (i<CACHEENTMAX) && (stream_type[i].stream != stream); i++);

  if(i < CACHEENTMAX)
    stream_type[i].stream = NULL;

  /* lookup the stream ID in the cache table */
  i = getCacheSlot(stream);

  /* no associated cache slot ? Simply close the stream */
  if(i < 0)
    return( fclose(stream) );

  if(cache[i].fd == stream)
  {
    dprintf(stderr, "f_close: removing stream %x from cache[%d]\n", (uint32_t)stream, i);
    
    cache[i].closed = 1;		/* indicate that the cache is closed */

    /* remove the cache looks */
    pthread_mutex_unlock( &cache[i].writeable );
    pthread_mutex_unlock( &cache[i].readable );
    pthread_mutex_unlock( &cache[i].cache_lock );

    /* wait for the fill thread to finish */
    dprintf(stderr, "f_close: waiting for fill tread to finish\n");
    pthread_join(cache[i].fill_thread, NULL);

    dprintf(stderr, "f_close: closing cache\n");
    rval = fclose(cache[i].fd);		/* close the stream */
    free(cache[i].cache);		/* free the cache */
    
    /* if this stream has a streamfilter, call it's destructor */
    if(cache[i].filter_arg)
    if(cache[i].filter_arg->destructor)
    {
      dprintf(stderr, "f_close: calling stream filter destructor\n");
      cache[i].filter_arg->destructor(cache[i].filter_arg);
      free(cache[i].filter_arg);
    }

    dprintf(stderr, "f_close: destroying mutexes\n");
    pthread_mutex_destroy(&cache[i].cache_lock);
    pthread_mutex_destroy(&cache[i].readable);
    pthread_mutex_destroy(&cache[i].writeable);
    
    /* completely blank out all data */
    bzero(&cache[i], sizeof(STREAM_CACHE));
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

  if(i < 0)
    return( ftell(stream) );

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

  if(i < 0)
  {
    rewind(stream);
    return;
  }

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

  if(i < 0)
    return( fseek(stream, offset, whence) );

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

  if(i < 0)
    return( fread(ptr, size, nitems, stream) );

  if(cache[i].fd == stream)
    rval = pop(stream, (char*)ptr, size * nitems);
  else
    rval = fread(ptr, size, nitems, stream);

  return rval;
}

char *f_type(FILE *stream, char *type)
{
  int i;
  
  /* lookup the stream in the stream type table */
  for(i=0 ; (i<CACHEENTMAX) && (stream_type[i].stream != stream); i++);
  
  /* if the stream could not be found, look for a free slot ... */
  if(i == CACHEENTMAX)
  {
    dprintf(stderr, "stream %x not in type table, ", stream);

    for(i=0 ; (i<CACHEENTMAX) && (stream_type[i].stream != NULL); i++);

    /* ... and copy the supplied type into the table */
    if(i < CACHEENTMAX)
    {
      if(type)
      {
        stream_type[i].stream = stream;
        strncpy(stream_type[i].type, type, 64);
        dprintf(stderr, "added entry (%s) for %x\n", type, stream);
      }
      return type;
    }
    else
      dprintf(stderr, "failed to add entry (%s)\n", type);

  }

  /* the stream is already in the table */
  else
  {
    dprintf(stderr, "stream %x lookup in type table succeded\n", stream);

    if(!type)
      return stream_type[i].type;
    else
    if(strstr(stream_type[i].type, type))
      return stream_type[i].type;
  }

  return NULL;
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

  for(i=0; (
  (i<CACHEENTMAX) && ((cache[i].fd != fd) || (!cache[i].cache)) 
  ); i++);
  
  return (i == CACHEENTMAX) ? -1 : i;
}

/* push a block of data into the stream cache */
int push(FILE *fd, char *buf, long len)
{
  int rval = 0, i, j;
  int blen = 0;

  i = getCacheSlot(fd);

  if(i < 0)
    return -1;

//  dprintf(stderr, "push: %d bytes to store [filled: %d of %d], stream: %x\n", len, cache[i].filled, CACHESIZE, fd);

  if(cache[i].fd == fd)
  {
    do
    {
      if(cache[i].closed)
        return -1;

      /* try to obtain write permissions for the cache */
      /* this will block if the cache is full */
      pthread_mutex_lock( &cache[i].writeable );

      if((cache[i].csize - cache[i].filled))
      {
        int amt[2];
        
	/* prevent any modification to the cache by other threads, */
	/* mainly by the pop() function, while we write data to it */
        pthread_mutex_lock( &cache[i].cache_lock );

        /* has the cache been closed while we were waiting for the */
	/* lock to open ? */
	if(cache[i].closed)
          return -1;

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
	  if(amt[j] > blen) amt[j] = blen;

	  if(amt[j])
	  {
	    bcopy(buf, cache[i].wptr, amt[j]);
	    cache[i].wptr = cache[i].cache + 
	    	(((int)(cache[i].wptr - cache[i].cache) + amt[j]) % cache[i].csize);

	    buf += amt[j];	/* adjust the target buffer pointer */
	    rval += amt[j];	/* increase the 'total bytes' counter */
	    blen -= amt[j];	/* decrease the block length counter */
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

  if(i < 0)
    return -1;
    
  dprintf(stderr, "pop: %d bytes requested [filled: %d of %d], stream: %x\n", 
  	len, cache[i].filled, CACHESIZE, (uint32_t)fd);

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
	  if(amt[j] > blen) amt[j] = blen;

	  if(amt[j])
	  {
	    dprintf(stderr, "pop(): rptr: 0x%08x, buf: 0x%08x, amt[%d]=%d, blen=%d, len=%d, rval=%d\n", 
		cache[i].rptr, buf, j, amt[j], blen, len, rval);

	    bcopy(cache[i].rptr, buf, amt[j]);

	    cache[i].rptr = cache[i].cache + 
	    	(((int)(cache[i].rptr - cache[i].cache) + amt[j]) % cache[i].csize);

	    buf += amt[j];	/* adjust the target buffer pointer */
	    rval += amt[j];	/* increase the 'total bytes' counter */
	    blen -= amt[j];	/* decrease the block length counter */
	    cache[i].filled -= amt[j];
	  }
	}
	
	dprintf(stderr, "pop: %d/%d/%d bytes read [filled: %d of %d], stream: %x\n", amt[0] + amt[1], rval, len, cache[i].filled, CACHESIZE, fd);

        /* if the cache is closed and empty, then */
	/* force the end condition to be met */
	if(cache[i].closed && (! cache[i].filled))
          break;//len = rval;

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
        dprintf(stderr, "pop: buffer underrun; cache empty - leaving cache locked\n");

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

  if(cache[i].filter_arg)
  if(cache[i].filter_arg->state)
    cache[i].filter_arg->state->buffered = 65536L * (long long)cache[i].filled / (long long)CACHESIZE;

  return rval;
}
  
void CacheFillThread(void *c)
{
  char *buf;
  STREAM_CACHE *cache = (STREAM_CACHE*)c;
  int rval, datalen;
  
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
    /* has a f_close() call in an other thread already closed the cache ? */
    datalen = CACHEBTRANS;

    rval = fread(buf, 1, datalen, cache->fd);
    
    /* if there is a filter function set up for this stream, then */
    /* we need to call it with the propper arguments */
    if(cache->filter)
    {
      cache->filter_arg->buf = buf;
      cache->filter_arg->len = &rval;
      cache->filter(cache->filter_arg);
      datalen = rval;
    }

    if( push(cache->fd, buf, rval) < 0)
      break;

  } while( (rval == datalen) && (!cache->closed) );

  /* close the cache if the stream disrupted */
  cache->closed = 1;
  pthread_mutex_unlock( &cache->writeable );
  pthread_mutex_unlock( &cache->readable );

  /* ... and exit this thread. */
  dprintf(stderr, "CacheFillThread: thread exited, stream %8x  \n", cache->fd);

  free(buf);
  pthread_exit(0);
}

/**************************** stream filter ******************************/

int f_status(FILE *stream, void (*cb)(void*))
{
  int i, rval = -1;
  
  if(!stream)
  {
    strcpy(err_txt, "NULL pointer as stream id\n");
    return -1;
  }
  
  /* lookup the stream ID in the cache table */
  i = getCacheSlot(stream);

  if(cache[i].fd == stream)
  {
    /* hook the users function into the steam filter */
    if(cache[i].filter_arg)
    {
      if(cache[i].filter_arg->state)
      {
        cache[i].filter_arg->state->cb = cb;
        rval = 0;
      }
      else
        strcpy(err_txt, "no cache[].filter_arg->state hook\n");
    }
    else
      strcpy(err_txt, "no cache[].filter_arg hook\n");
  }

  return rval;
}

/* parse the meta data block and copy all relevant */
/* information into the CSTATE structure */
void ShoutCAST_ParseMetaData(char *md, CSTATE *state)
{
#define SKIP(a) for(;(a && !isalnum(*a)); ++a);
  char *ptr;

  dprintf(stderr, "ShoutCAST_ParseMetaData(%s, %x)\n", md, state);
  
  ptr = strstr(md, "StreamTitle=");

  if(ptr)
  {
    /* look if there is a dash or a comma that separates artist and title */
    ptr = strstr(md, " - ");

    if(!ptr)
     ptr = strstr(md, ", ");
    
     
    /* no separator, simply copy everything into the 'title' field */
    if(!ptr)
    {
      ptr = strchr(md, '=');
      strncpy(state->title, ptr + 2, 4096);
      ptr = strchr(state->title, ';');      
      if(ptr) *(ptr - 1) = 0;
      state->artist[0] = 0;
    }
    else
    {
      SKIP(ptr);
      strcpy(state->title, ptr);
      ptr = strchr(state->title, ';');      
      if(ptr) *(ptr - 1) = 0;

      ptr = strstr(md, "StreamTitle=");
      ptr = strchr(ptr, '\'');
      strncpy(state->artist, ptr + 1, 4096);
      ptr = strstr(state->artist, " - ");
      if(!ptr)
        ptr = strstr(state->artist, ", ");

      if(ptr) *ptr = 0;
    }
    state->state = RUNNING;
  }
}

void ShoutCAST_DestroyFilter(void *a)
{
  STREAM_FILTER *arg = (STREAM_FILTER*)a;
  
  if(arg->state) free(arg->state), arg->state = NULL;
  if(arg->user)  free(arg->user),  arg->user  = NULL;
}

STREAM_FILTER *ShoutCAST_InitFilter(int meta_int)
{
  STREAM_FILTER *arg;
  
  arg = (STREAM_FILTER*)calloc(1, sizeof(STREAM_FILTER));

  /* allocate our private data space, hook it into the */
  /* stream filter structure and initialize the variables */
  if(!arg->user)
  {
    arg->user  =          calloc(1, sizeof(FILTERDATA));
    arg->state = (CSTATE*)calloc(1, sizeof(CSTATE));
    ((FILTERDATA*)arg->user)->meta_int = meta_int;
    arg->destructor = ShoutCAST_DestroyFilter;
  }  

  return arg;
}

void ShoutCAST_MetaFilter(STREAM_FILTER *arg)
{
  FILTERDATA *filterdata = (FILTERDATA*)arg->user;
  int meta_int = filterdata->meta_int;
  int      len = *arg->len;
  char    *buf = (char*)arg->buf;
  int meta_start;

#if 0
dprintf(stderr, "filter : cnt      : %d\n", filterdata->cnt);
dprintf(stderr, "filter : len      : %d\n", filterdata->len);
dprintf(stderr, "filter : stored   : %d\n", filterdata->stored);
dprintf(stderr, "filter : cnt + len: %d\n", filterdata->cnt + len);
dprintf(stderr, "filter : meta_int : %d\n", filterdata->meta_int);
#endif

  /* not yet all meta data has been processed */
  if(filterdata->stored < filterdata->len)
  {
    int bsize = (filterdata->len + 1) - filterdata->stored;
    
    /* if there is some meta data, extract it */
    /* there can be zero size blocks too */
    if(filterdata->len)
    {
      dprintf(stderr, "filter : ********* partitioned metadata block part 2 ******\n");

      bcopy(buf, filterdata->meta_data + filterdata->stored, bsize);

      ShoutCAST_ParseMetaData(filterdata->meta_data, arg->state);

      /* call the users callback function */
      if(arg->state->cb)
        arg->state->cb(arg->state);
      
      //dprintf(stderr, "filter : metadata : \n\n\n----------\n%s\n----------\n\n\n", filterdata->meta_data);

      /* remove the metadata and it's size indicator from the buffer */
      memmove(buf, buf + bsize, len - bsize);

      *arg->len -= bsize;
      filterdata->cnt = len - bsize;
      filterdata->stored = 0;
      filterdata->len = 0;
    }
    return;
  }
  
  if((filterdata->cnt < meta_int) && ((filterdata->cnt + len) <= meta_int))
  {
    /* do nothing; leave the data block and the length variable */
    /* untouched, update our private counter and return */
    filterdata->cnt += len;
    return;
  }

  /* does a meta data block start in the current block ? */
  if((filterdata->cnt <= meta_int) && ((filterdata->cnt + len) > meta_int))
  {
    meta_start = meta_int - filterdata->cnt;

    /* the first byte of the meta data block tells us how long it is */
    filterdata->len = 16 * (int)buf[meta_start];

    dprintf(stderr, "filter : ---> metadata %d bytes @ %d\n", filterdata->len, meta_start);

    /****************************************************************/
    /* case A: the meta data is completely within the current block */
    /****************************************************************/
    if((meta_start + filterdata->len) < len)
    {
      int b = meta_start + filterdata->len + 1;
      
      /* if there is some meta data, extract it; */
      /* there can be zero size blocks too */
      if(filterdata->len)
      {
        bzero(filterdata->meta_data, 4096);
        bcopy(buf + meta_start, filterdata->meta_data, filterdata->len);

        ShoutCAST_ParseMetaData(filterdata->meta_data, arg->state);
        
	/* call the users callback function */
        if(arg->state->cb)
          arg->state->cb(arg->state);
      
        //dprintf(stderr, "filter : metadata : \n\n\n----------\n%s\n----------\n\n\n", filterdata->meta_data);
      }
  
      /* remove the metadata and it's size indicator from the buffer */
      memmove(buf + meta_start, buf + b, len - b );

      /* adjust the buffersize */
      *arg->len -= filterdata->len + 1;
      filterdata->cnt = len - b;
      filterdata->stored = 0;
      filterdata->len = 0;
    }

    /************************************************************************/
    /* case B: the meta data is partitioned and continues in the next block */
    /************************************************************************/
    else
    {
      dprintf(stderr, "filter : ********* partitioned metadata block part 1 ******\n");

      /* if there is some meta data, extract it */
      /* there can be zero size blocks too */
      if(filterdata->len)
      {
        bzero(filterdata->meta_data, 4096);
        filterdata->stored = len - meta_start;
        bcopy(buf + meta_start, filterdata->meta_data, filterdata->stored);
      }

      *arg->len = meta_start;
      filterdata->cnt = 0;
    }
  }
}
