#include <qobject.h>
#include <qfile.h>
#include <qtimer.h>
#include <sys/socket.h>
#include <error.h>
#include <errno.h>
#include <time.h>
#include "edvb.h"
#include "httpd.h"


void eHTTPGarbage::doGarbage()
{
	delete conn;
	conn=0;
}

void eHTTPGarbage::destruct(eHTTPConnection *c)
{
	if (!conn)
	{
		conn=new QList<eHTTPConnection>;
		conn->setAutoDelete(true);
	}
	conn->append(c);
	garbage.start(0, 1);
}

eHTTPGarbage::eHTTPGarbage()
{
	connect(&garbage, SIGNAL(timeout()), this, SLOT(doGarbage()));
	instance=this;
	conn=0;
}

eHTTPGarbage::~eHTTPGarbage()
{
	delete conn;
	conn=0;
}


eHTTPGarbage *eHTTPGarbage::instance;

eHTTPDataSource::eHTTPDataSource(eHTTPConnection *c): connection(c)
{
}

eHTTPDataSource::~eHTTPDataSource()
{
}

void eHTTPDataSource::haveData(void *data, int len)
{
}

eHTTPError::eHTTPError(eHTTPConnection *c, int errcode): eHTTPDataSource(c), errcode(errcode)
{
	QString error="unknown error";
	switch (errcode)
	{
	case 400: error="Bad Request"; break;
	case 403: error="Forbidden"; break;
	case 404: error="Not found"; break;
	case 500: error="Internal server error"; break;
	}
	connection->code_descr=error;
	connection->code=errcode;
	
	connection->local_header["Content-Type"]="text/html";
}

int eHTTPError::doWrite(int w)
{
	QString html;
	html+="<html><head><title>Error "+QString().setNum(connection->code)+"</title></head>"+
		"<body><h1>Error "+QString().setNum(errcode)+": "+connection->code_descr+"</h1></body></html>\n";
	connection->writeBlock(html, html.length());
	return -1;
}

eHTTPConnection::eHTTPConnection(int socket, eHTTPD *parent): QSocket(parent), parent(parent)
{
	connect(this, SIGNAL(readyRead()), SLOT(readData()));
	connect(this, SIGNAL(bytesWritten(int)), SLOT(bytesWritten(int)));
	connect(this, SIGNAL(error(int)), SLOT(gotError()));
	setSocket(socket);
//	QHostAddress me=address(), he=peerAddress();

	buffersize=64*1024;
	dying=0;
	localstate=stateWait;
	remotestate=stateRequest;
	data=0;
}

void eHTTPConnection::readData()
{
	if (processRemoteState())
		if (!dying)
		{
			dying=1;
			eHTTPGarbage::getInstance()->destruct(this);
		}
}

void eHTTPConnection::bytesWritten(int)
{
	if (processLocalState())
		if (!dying)
		{
			dying=1;
			eHTTPGarbage::getInstance()->destruct(this);
		}
}

int eHTTPConnection::processLocalState()
{
	int done=0;
	while (!done)
	{
		switch (localstate)
		{
		case stateWait:
			done=1;
			break;
		case stateRequest:
		{
			QString req=request+" "+requestpath+" "+httpversion+"\r\n";
			writeBlock((const char*)req.latin1(), req.length());
			localstate=stateHeader;
			break;
		}
		case stateResponse:
		{
			writeString(httpversion + " "+QString().setNum(code)+" " + code_descr + "\r\n");
			localstate=stateHeader;
			local_header["Connection"]="close";
			break;
		}
		case stateHeader:
			for (std::map<std::string,std::string>::iterator cur=local_header.begin(); cur!=local_header.end(); ++cur)
			{
				writeString(cur->first.c_str());
				writeString(": ");
				writeString(cur->second.c_str());
				writeString("\r\n");
			}
			writeString("\r\n");
			if (request=="HEAD")
				localstate=stateDone;
			else
				localstate=stateData;
			break;
		case stateData:
			if (data)
			{
				int btw=buffersize-bytesToWrite();
				if (btw>0)
				{
					if (data->doWrite(btw)<0)
						localstate=stateDone;
					else
						done=1;
				} else
					done=1;
			} else
				localstate=stateDone;
			break;
		case stateDone:
			localstate=stateClose;
			break;
		case stateClose:
			close();		// bye, bye, remote
			if (State() == Idle)
				return 1;
			return 0;
			break;
		}
	}
	return 0;
}

int eHTTPConnection::processRemoteState()
{
	int abort=0, done=0;
	while (((!done) || bytesAvailable()) && !abort)
	{
		switch (remotestate)
		{
		case stateWait:
		{
			char buffer[1024];
			while (bytesAvailable())
				readBlock(buffer, 1024);
			done=1;
			break;
		}
		case stateRequest:
		{
			QString line;
			if (!getLine(line))
			{
				done=1;
				break;
			}

			int del[2];
			del[0]=line.find(" ");
			del[1]=line.find(" ", del[0]+1);
			if (del[0]==-1)
			{
				if (data)
					delete data;
				data=new eHTTPError(this, 400);
				localstate=stateResponse;
				remotestate=stateDone;
				if (processLocalState())
					return -1;
				break;
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
				remotestate=stateData;
				content_length_remaining=content_length_remaining=0;
				data=new eHTTPError(this, 400);	// bad request - not supporting version 0.9 yet
			} else
				remotestate=stateHeader;
			break;
		}
		case stateHeader:
		{
			QString line;
			if (!getLine(line))
			{
				done=1;
				break;
			}
			if (!line.length())
			{
				localstate=stateResponse;		// can be overridden by dataSource
				for (QListIterator<eHTTPPathResolver> i(parent->resolver); i.current(); ++i)
				{
					if ((data=i.current()->getDataSource(request, requestpath, this)))
						break;
				}

				if (!data)
				{
					if (data)
						delete data;
					data=new eHTTPError(this, 404);
				}

				content_length=0;
				if (remote_header.count("Content-Length"))
				{
					content_length=atoi(remote_header["Content-Length"].c_str());
					content_length_remaining=content_length;
				}
				if (content_length)
					remotestate=stateData;
				else
				{
					data->haveData(0, 0);
					remotestate=stateDone;
				}
				if (processLocalState())
					return -1;
			} else
			{
				int del=line.find(": ");
				QString name=line.left(del), value=line.mid(del+2);
				remote_header[std::string((const char*)name)]=value;
				const char *ct="Content-Type";
			}
			done=1;
			break;
		}
		case stateData:
		{
			ASSERT(data);
			char buffer[1024];
			int len;
			while (bytesAvailable())
			{
				int tr=1024;
				if (tr>content_length_remaining)
					tr=content_length_remaining;
				len=readBlock(buffer, tr);
				data->haveData(buffer, len);
				content_length_remaining-=len;
				if (!content_length_remaining)
				{
					char buffer[100]="Content-Type";
					data->haveData(0, 0);
					remotestate=stateDone;
					break;
				}
			}
			done=1;
			if (processLocalState())
				return -1;
			break;
		}
		case stateDone:
			remotestate=stateClose;
			break;
		case stateClose:
			remotestate=stateWait;
			abort=1;
			break;
		default:
			qDebug("bla wrong state");
			done=1;
		}
	}
	return 0;
}

void eHTTPConnection::writeString(const char *string)
{
	writeBlock(string, strlen(string));
}

int eHTTPConnection::getLine(QString &line)
{
	if (!canReadLine())
		return 0;
	line=readLine();
	line.truncate(line.length()-1);
	if (line[line.length()-1]=='\r')
		line.truncate(line.length()-1);
	return 1;
}

void eHTTPConnection::gotError()
{
/*	close();
	emit connectionClosed(); */
}

eHTTPD::eHTTPD(Q_UINT16 port, int backlog, QObject *parent, const char *name): QServerSocket(port, backlog, parent, name)
{
	new eHTTPGarbage;
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

void eHTTPD::oneConnectionClosed()
{
	delete sender();	// das hier ist SEHR SEHR riskant. don't try at home.
}
