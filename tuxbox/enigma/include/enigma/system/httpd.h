#ifndef __httpd_h
#define __httpd_h

#include <asm/types.h>
#include <qfile.h>
#include <qlist.h>
#include <qserversocket.h>
#include <map>
#include <string>
#include <qtimer.h>

class eHTTPConnection;
class eHTTPDataSource;
class eHTTPD;

class eHTTPGarbage: public /*Q*/Object
{
//	Q_OBJECT
	QTimer garbage;
	QList<eHTTPConnection> *conn;
	static eHTTPGarbage *instance;
public:// slots:
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
	virtual eHTTPDataSource *getDataSource(QString request, QString path, eHTTPConnection *conn)=0;
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

class eHTTPConnection: public QSocket
{
//	Q_OBJECT
	void doError(int error);
	
	int getLine(QString &line);
	
	int processLocalState();
	int processRemoteState();
	void writeString(const char *string);
	
	eHTTPDataSource *data;
	eHTTPD *parent;
	
	int buffersize, dying;
private:// slots:
	void readData();
	void gotError();
	void bytesWritten(int);
	void hostConnected();
/*signals:
	void closing();*/
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
	eHTTPConnection(const char *host, int port=80);
	void die();
	static eHTTPConnection *doRequest(const char *uri, int *error=0);
	void start();
	~eHTTPConnection();
	
		// stateRequest
	QString request, requestpath, httpversion;
	int is09;
	
		// stateResponse
	
	int code;
	QString code_descr;
	
	std::map<std::string,std::string> remote_header, local_header;
	
		// stateData
	int content_length, content_length_remaining;
};

class eHTTPD: public QServerSocket, public Object
{
//	Q_OBJECT
	friend class eHTTPConnection;
	QList<eHTTPPathResolver> resolver;
private:// slots:
	void oneConnectionClosed();
public:
	eHTTPD(Q_UINT16 port, int backlog=0, QObject *parent=0, const char *name=0);
	void newConnection(int socket);

	void addResolver(eHTTPPathResolver *r) { resolver.append(r); }
	void removeResolver(eHTTPPathResolver *r) { resolver.remove(r); }
};

#endif
