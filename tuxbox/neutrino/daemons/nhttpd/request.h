/*      
        webserver  -   DBoxII-Project

        Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'

        $Id: request.h,v 1.21 2002/12/09 17:59:27 dirch Exp $

        License: GPL

        This program is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 2 of the License, or
        (at your option) any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program; if not, write to the Free Software
        Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/


#ifndef __webserver_request__
#define __webserver_request__

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <config.h>

#include <string>
#include <map>

#include "webserver.h"


using namespace std;

typedef map<string,string> CStringList;


enum Method_Typ {M_UNKNOWN=0,M_POST = 1,M_GET = 2,M_PUT = 3,M_HEAD = 4};


class CWebserverRequest
{

private:
	bool RequestCanceled;
	string rawbuffer;
	int rawbuffer_len;
	char *outbuf;
	string Boundary;
	
	long tmplong;
	int tmpint;
	string tmpstring;

	bool CheckAuth();
	string GetContentType(string ext);
	string GetFileName(string path, string filename);
	void SplitParameter(string param_str);
	void RewriteURL();
	int OpenFile(string path, string filename);


public:
	string			Client_Addr;
	int				Socket;
	unsigned long	RequestNumber;

	void printf ( const char *fmt, ... );

	bool SocketWrite( char* text);
	bool SocketWriteLn( char* text);
	bool SocketWriteData( char* data, long length );
	bool SocketWrite(string text){return SocketWrite( (char *)text.c_str());}
	bool SocketWriteLn(string text){return SocketWriteLn( (char *)text.c_str());}
	bool SendFile(string path,string filename);

	void SendHTMLFooter();
	void SendHTMLHeader(string Titel);
	void SendPlainHeader(string contenttype = "text/plain");
	void Send302(char *URI);
	void Send404Error();
	void Send500Error();

	bool Authenticate();

	bool ParseFile(string filename,CStringList params);
	string ParseLine(string line,CStringList params);

	int Method;
	int HttpStatus;

	string Host;
	string URL;
	string Path;
	string Filename;
	string FileExt;
	string Param_String;

	CStringList ParameterList;
	CStringList HeaderList;
	
	map<int,string> boundaries;

	class CWebserver * Parent;

	CWebserverRequest(CWebserver *server);
	~CWebserverRequest();

	bool GetRawRequest();
	bool ParseRequest();
	bool ParseParams(string param_string);
	bool ParseFirstLine(string zeile);
	bool ParseHeader(string header);
	bool ParseBoundaries(string bounds);
	void URLDecode(string &encodedString);
	bool HandleUpload();
	bool HandleUpload(char * Name);
	void PrintRequest();
	bool SendResponse();
	bool EndRequest();
	void SendOk();
	void SendError();

	friend class TWebDbox;
};
#endif
