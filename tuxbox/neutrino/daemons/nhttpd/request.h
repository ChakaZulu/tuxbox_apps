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

#include "webserver.h"

using namespace std;

typedef map<string,string> CStringList;

#define SA struct sockaddr
#define SAI struct sockaddr_in

enum Method_Typ {M_UNKNOWN=0,M_POST = 1,M_GET = 2,M_PUT = 3,M_HEAD = 4};


class CWebserverRequest
{

private:
	bool RequestCanceled;
	string rawbuffer;
	int rawbuffer_len;

	void SplitParameter(string param_str);

	void RewriteURL();

	int OpenFile(string path, string filename);
	void SendOpenFile(int );

	string Boundary;
	
	long tmplong;
	int tmpint;
	string tmpstring;

	bool CheckAuth();
	

public:
// 	int				sock_fd;
//	SAI				servaddr;
	string			Client_Addr;
	int				Socket;
	unsigned long	RequestNumber;

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

	bool ParseFile(string file,CStringList params);
	string ParseLine(string line,CStringList params);


	int Method;
	string Host;
	string URL;
	string Path;
	string Filename;
	string FileExt;
	string Param_String;
	string ContentType;

	CStringList ParameterList;
	CStringList HeaderList;
	map<int,string> boundaries;

	
	int HttpStatus;

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
//	TUpload *Upload;
	friend class TWebDbox;
//	friend class TUpload;
};
#endif
