//=============================================================================
// YHTTPD
// Response
//=============================================================================

#ifndef __yhttpd_response_h__
#define __yhttpd_response_h__

// c++
#include <string>
// yhttpd
#include "yconfig.h"
#include "ytypes_globals.h"
#include "yhook.h"

// forward declaration
class CWebserver;
class CWebserverConnection;

//-----------------------------------------------------------------------------
class CWebserverResponse
{
private:
//	int SocketSendFile(int fd);

protected:
	int OpenFile(std::string path, std::string filename);
	std::string GetFileName(std::string path, std::string filename);
	bool WriteData(char const *data, long length);
	// response data
	time_t 		obj_last_modified;	// Sendfile: date of last file modification
	long 		obj_content_len;	// Length of ResponseBody (Sendfile: Length of file)
	std::string	redirectURI;		// URI for redirection else: empty

public:
	class CWebserver *Webserver;
	class CWebserverConnection *Connection;
	
	// con/destructors
	CWebserverResponse();
	CWebserverResponse(CWebserver *pWebserver);

	// response control
	bool SendResponse(void);

	// output methods
	void printf(const char *fmt, ...);
	bool Write(char const *text);
	bool WriteLn(char const *text);
	bool Write(const std::string text) { return Write(text.c_str()); }
	bool WriteLn(const std::string text) { return WriteLn(text.c_str()); }

	// Headers
	void SendError(HttpResponseType responseType) {SendHeader(responseType, false, "text/html");}
	void SendHeader(HttpResponseType responseType, bool cache=false, std::string ContentType="text/html");
	void SendHTMLFooter(void);
	void SendHTMLHeader(std::string Titel);
	void SendPlainHeader(std::string contenttype = "text/plain"){SendHeader(HTTP_OK, false, contenttype);}
//	void SendObjectMoved(char const *URI){SendObjectMoved(std::string(URI));}
	void SendObjectMoved(std::string URI);
	bool SendFile(const std::string path, const std::string filename);

	// Helpers
	std::string GetContentType(std::string ext);
};

#endif /* __yhttpd_response_h__ */
