#ifndef __httpd_h
#define __httpd_h

#include <asm/types.h>
#include <eptrlist.h>
#include <ebase.h>
#include <core/socket/socket.h>
#include <core/socket/serversocket.h>
#include <map>
#include <estring.h>
#include <eerror.h>

class eHTTPConnection;
class eHTTPDataSource;
class eHTTPD;

class eHTTPGarbage
{
	eTimer garbage;
	ePtrList<eHTTPConnection> *conn;
	static eHTTPGarbage *instance;
public:
	void doGarbage();
public:
	void destruct(eHTTPConnection *c);
	eHTTPGarbage();
	~eHTTPGarbage();
	static eHTTPGarbage *getInstance() { return instance; }
};

class eHTTPPathResolver
{
public:
	virtual ~eHTTPPathResolver() {}; 
	virtual eHTTPDataSource *getDataSource(eString request, eString path, eHTTPConnection *conn)=0;
};

class eHTTPDataSource
{
protected:
	eHTTPConnection *connection;
public:
	eHTTPDataSource(eHTTPConnection *c);
	virtual ~eHTTPDataSource();
	virtual void haveData(void *data, int len);
	virtual int doWrite(int bytes)=0;	// number of written bytes, -1 for "no more"
};

class eHTTPError: public eHTTPDataSource
{
	int errcode;
public:
	eHTTPError(eHTTPConnection *c, int errcode);
	~eHTTPError() { }
	void haveData();
	int doWrite(int bytes);
};

class eHTTPConnection: public eSocket
{
	void doError(int error);
	
	int getLine(eString &line);
	
	int processLocalState();
	int processRemoteState();
	void writeString(const char *data);
	
	eHTTPDataSource *data;
	eHTTPD *parent;
	
	int buffersize, dying;
private:
	void readData();
	void gotError(int);
	void bytesWritten(int);
	void hostConnected();
public:
	Signal0<void> closing;
	enum
	{
		/*
		
		< GET / HTTP/1.0
		< If-modified-since: bla
		<
		< Data
		> 200 OK HTTP/1.0
		> Content-Type: text/html
		>
		> Data
		*/
	
		stateWait, stateRequest, stateResponse, stateHeader, stateData, stateDone, stateClose
	};
	int localstate, remotestate;
	
	eHTTPConnection(int socket, eHTTPD *parent);
	eHTTPConnection(eString host, int port=80);
	void die();
	static eHTTPConnection *doRequest(const char *uri, int *error=0);
	void start();
	~eHTTPConnection();
	
		// stateRequest
	eString request, requestpath, httpversion;
	int is09;
	
		// stateResponse
	
	int code;
	eString code_descr;
	
	std::map<std::string,std::string> remote_header, local_header;
	
		// stateData
	int content_length, content_length_remaining;
};

class eHTTPD: public eServerSocket
{
	friend class eHTTPConnection;
	ePtrList<eHTTPPathResolver> resolver;
private:// slots:
	void oneConnectionClosed();
	static eHTTPD *instance;
	eHTTPConnection *conn;
	
public:
	eHTTPD(int port);
	void newConnection(int socket);

	static eHTTPD *getInstance() { return instance; }

	void addResolver(eHTTPPathResolver *r) { resolver.push_back(r); }
	void removeResolver(eHTTPPathResolver *r) { resolver.remove(r); }
};

#endif
