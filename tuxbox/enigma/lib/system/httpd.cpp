#include <qobject.h>
#include <qfile.h>
#include <qtimer.h>
#include <sys/socket.h>
#include <error.h>
#include <errno.h>
#include <time.h>
#include "edvb.h"
#include "httpd.h"

eHTTPDataSource::~eHTTPDataSource()
{
}

eHTTPError::eHTTPError(int errcode): errcode(errcode)
{
}

int eHTTPError::getCode()
{
	return errcode;
}

int eHTTPError::writeData(eHTTPConnection *conn)
{
	QString error="unknown error";
	switch (errcode)
	{
	case 400: error="Bad Request"; break;
	case 403: error="Forbidden"; break;
	case 404: error="Not found"; break;
	case 500: error="Internal server error"; break;
	}
	QString html;
	if (!conn->is09)
		html="Content-Type: text/html\r\n\r\n";
	html+="<html><head><title>Error "+QString().setNum(errcode)+"</title></head>"+
		"<body><h1>Error "+QString().setNum(errcode)+": "+error+"</h1></body></html>\n";
	conn->writeBlock(html, html.length());
	return -1;
}

int eHTTPError::haveData(eHTTPConnection *conn)
{
	return 0;	// pech
}

eHTTPConnection::eHTTPConnection(int socket, eHTTPD *parent): QSocket(parent), parent(parent)
{
	connect(this, SIGNAL(readyRead()), SLOT(readData()));
	connect(this, SIGNAL(error(int)), SLOT(gotError()));
	setSocket(socket);
	QHostAddress me=address(), he=peerAddress();
	httpstate=stateRequest;
	data=0;
}

void eHTTPConnection::doResponse()
{
	if (!data)
		qFatal("doResponse - and no data set!");
	QString res="HTTP/1.1 "+QString().setNum(data->getCode())+" OK\r\n";
	if (!is09)
		res+="Server: EliteDVB httpd\r\nConnection: close\r\n";
	httpstate=stateData;
	writeBlock((const char*)res.latin1(), res.length());
	int rc;
	while ((rc=data->writeData(this))>0);
	writeBlock("\r\n", 2);
	if (rc<0)
		close();
	if (!rc)
		qFatal("res==0 nyi");
	delete data;
	data=0;
	httpstate=stateRequest;
}

void eHTTPConnection::processResponse(QString request, QString path)
{
	qDebug("[HTTP] %s %s", (const char*)request, (const char*)path);
	
	for (QListIterator<eHTTPPathResolver> i(parent->resolver); i.current(); ++i)
	{
		if ((data=i.current()->getDataSource(request, path, this)))
			break;
	}

	if (!data)
		doError(404);	// not found

	doResponse();
}

void eHTTPConnection::doError(int error)
{
	data=new eHTTPError(error);
}

void eHTTPConnection::readData()
{
	qDebug("read read");
	while (bytesAvailable())
	{
		if (httpstate==stateData)
		{
			if (data)
				data->haveData(this);
			else
			{
				doError(400);
				doResponse();
			}
			break;
		} else
		{
			QString line;
			if (httpstate!=stateDataIn)
			{
				if (!canReadLine())
					break;
				line=readLine();
				line.truncate(line.length()-1);
				if (line[line.length()-1]=='\r')
					line.truncate(line.length()-1);
			}
			if (httpstate==stateRequest)
			{
				int del[2];
				del[0]=line.find(" ");
				del[1]=line.find(" ", del[0]+1);
				if (del[0]==-1)
				{
					doError(400);
					doResponse();
					continue;
				}
				request=line.left(del[0]);
				requestpath=line.mid(del[0]+1, (del[1]==-1)?-1:(del[1]-del[0]-1));
				if (del[1]!=-1)
				{
					is09=0;
					httpversion=line.mid(del[1]+1);
				} else
					is09=1;
				if (is09)
				{
					content_length=0;
					content_read=0;
					content=0;
					httpstate=stateDataIn;
				} else
					httpstate=stateOptions;
			} else if (httpstate==stateOptions)
			{
				if (!line.length())
				{
					content_length=0;
					QString acontent_length=options["Content-Length"];
					if (acontent_length)
						content_length=atoi(acontent_length);
					content=new __u8[content_length];
					content_read=0;
					httpstate=stateDataIn;
				} else
				{
					int del=line.find(": ");
					QString name=line.left(del), value=line.mid(del+2);
					options.insert(name, value);
				}
			}
			if (httpstate==stateDataIn)
			{
				if (content_length != content_read)
				{
					int r=bytesAvailable();
					if (r>(content_length-content_read))
						r=content_length-content_read;
					content_read+=readBlock((char*)content+content_read, r);
					qDebug("now have %d/%d bytes of content", content_read, content_length);
				}

				if (content_length == content_read)
				{
					processResponse(request, requestpath);
					delete content;
					content=0;
				}
			}
		}
	}
}

void eHTTPConnection::gotError()
{
	close();
}

void eHTTPConnection::deleteMyself()
{
	delete this; // selbstmord
}

eHTTPD::eHTTPD(Q_UINT16 port, int backlog, QObject *parent, const char *name): QServerSocket(port, backlog, parent, name)
{
	if (!ok())
		qDebug("[NET] httpd server FAILED on port %d", port);
	else
		qDebug("[NET] httpd server started on port %d", port);
	resolver.setAutoDelete(true);
}

eHTTPConnection::~eHTTPConnection()
{
	delete data;
}

void eHTTPD::newConnection(int socket)
{
	eHTTPConnection *conn=new eHTTPConnection(socket, this);
	connect(conn, SIGNAL(connectionClosed()), SLOT(oneConnectionClosed()));
	connect(conn, SIGNAL(delayedCloseFinished()), SLOT(oneConnectionClosed()));
}

static int bla=0;

void eHTTPD::oneConnectionClosed()
{
	delete sender();	// das hier ist SEHR SEHR riskant. don't try at home.
}
