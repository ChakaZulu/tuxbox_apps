#ifndef __httpd_h
#define __httpd_h

#include <asm/types.h>
#include <qfile.h>
#include <qlist.h>
#include <qserversocket.h>
#include <qmap.h>

class eHTTPConnection;
class eHTTPDataSource;
class eHTTPD;

class eHTTPPathResolver
{
public:
	virtual ~eHTTPPathResolver() {}; 
	virtual eHTTPDataSource *getDataSource(QString request, QString path, const eHTTPConnection *conn)=0;
};

class eHTTPDataSource
{
public:
	virtual ~eHTTPDataSource();
	virtual int getCode()=0;
	virtual int writeData(eHTTPConnection *conn)=0;
	virtual int haveData(eHTTPConnection *conn)=0;
};

class eHTTPError: public eHTTPDataSource
{
	int errcode;
public:
	eHTTPError(int errcode);
	~eHTTPError() { } 
	int getCode();
	int writeData(eHTTPConnection *conn);
	int haveData(eHTTPConnection *conn);
};

class eHTTPConnection: public QSocket
{
	Q_OBJECT
	void doResponse();
	void processResponse(QString request, QString path);
	void doError(int error);
	eHTTPDataSource *data;
	eHTTPD *parent;
private slots:
	void readData();
	void gotError();
public slots:
	void deleteMyself();
public:
	enum
	{
		stateRequest, stateOptions, stateData, stateDataIn
	};
	int httpstate;
	__u8 *content;
	int content_length, content_read;
	eHTTPConnection(int socket, eHTTPD *parent);
	~eHTTPConnection();
	
	QString request, requestpath, httpversion;
	int is09;
	QMap<QString,QString> options;
};

class eHTTPD: public QServerSocket
{
	Q_OBJECT
	friend class eHTTPConnection;
	QList<eHTTPPathResolver> resolver;
private slots:
	void oneConnectionClosed();
public:
	eHTTPD(Q_UINT16 port, int backlog=0, QObject *parent=0, const char *name=0);
	void newConnection(int socket);

	void addResolver(eHTTPPathResolver *r) { resolver.append(r); }
	void removeResolver(eHTTPPathResolver *r) { resolver.remove(r); }
};

#endif
