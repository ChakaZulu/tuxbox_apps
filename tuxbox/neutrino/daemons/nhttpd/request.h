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

//#include <string>
//#include <vector>

#include "helper.h"
#include "webserver.h"
#include "upload.h"

using namespace std;

#define SA struct sockaddr
#define SAI struct sockaddr_in

enum Method_Typ {M_UNKNOWN=0,M_POST = 1,M_GET = 2,M_PUT = 3};


class TWebserverRequest
{

private:
	char * rawbuffer;
	int rawbuffer_len;

	void ParseHeader(char * t, int len);
	void RewriteURL();

	int OpenFile(char *path, char *filename);
	void SendOpenFile(int );
	TString *Boundary;

public:
 	int sock_fd;
	SAI servaddr;
	int Socket;

	void SocketWrite( char* text);
	void SocketWriteLn( char* text);
	void SocketWriteData( char* data, long length );
//	void SocketWriteLn( char * text);
	bool SendFile(char *path,char *filename);

	int Method;
	TString *URL;
	TString *Path;
	TString *Filename;
	TString *FileExt;
	TString *Param_String;
	TString *ContentType;

	TParameterList *ParameterList;
	TParameterList *HeaderList;
	
	int HttpStatus;

	class TWebserver * Parent;

	TWebserverRequest(TWebserver *server);
	~TWebserverRequest();
	bool GetRawRequest(int socket);
	bool ParseRequest();
	bool ParseParams(char *param_string);
	bool ParseFirstLine(char * zeile, int len);
	void Send404Error();
	void Send500Error();
	void SendPlainHeader(char *contenttype = NULL);
	bool HandleUpload(char * Name);
	void PrintRequest();
	bool SendResponse();
	bool EndRequest();
	TUpload *Upload;
	friend class TWebDbox;
//	friend class TUpload;
};
#endif
