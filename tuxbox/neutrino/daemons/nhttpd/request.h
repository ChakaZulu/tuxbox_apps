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
#include <vector>
#include <map>

#include "helper.h"
#include "webserver.h"
#include "upload.h"

using namespace std;

typedef map<string,string> CParameterList;

#define SA struct sockaddr
#define SAI struct sockaddr_in

enum Method_Typ {M_UNKNOWN=0,M_POST = 1,M_GET = 2,M_PUT = 3};


class CWebserverRequest
{

private:
	char * rawbuffer;
	int rawbuffer_len;
	void ParseHeader(char * t, int len);
//	void SplitParameter(char *parameter,int len,char seperator='=');
	void SplitParameter(string param_str);

	void RewriteURL();

	int OpenFile(const char *path, const char *filename);
	void SendOpenFile(int );
	TString *Boundary;

public:
 	int sock_fd;
	SAI servaddr;
	SAI	cliaddr;
	int Socket;

	void SocketWrite( char* text);
	void SocketWriteLn( char* text);
	void SocketWriteData( char* data, long length );
	void SocketWrite(string text){SocketWrite( (char *)text.c_str());}
	void SocketWriteLn(string text){SocketWriteLn( (char *)text.c_str());}
	bool SendFile(char *path,char *filename);

	void SendHTMLFooter();
	void SendHTMLHeader(char * Titel);

	int Method;
	string URL;
	string Path;
	string Filename;
	string FileExt;
	string Param_String;
	string ContentType;

	CParameterList ParameterList;
	TParameterList *HeaderList;
	
	int HttpStatus;

	class TWebserver * Parent;

	CWebserverRequest(TWebserver *server);
	~CWebserverRequest();
	bool GetRawRequest(int socket);
	bool ParseRequest();
	bool ParseParams(string param_string);
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
