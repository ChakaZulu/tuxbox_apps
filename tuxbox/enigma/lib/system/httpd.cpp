#include <qfile.h>
#include <sys/socket.h>
#include <error.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include "edvb.h"
#include "httpd.h"
#include <string>

void eHTTPGarbage::doGarbage()
{
	eDebug("garbage");
	for (ePtrList<eHTTPConnection>::iterator l(*conn); l != conn->end(); ++l)
		eDebug("%x", *l);
	delete conn;
	conn=0;
	eDebug("garbage ist vorbei");
}

void eHTTPGarbage::destruct(eHTTPConnection *c)
{
	if (!conn)
	{
		conn=new ePtrList<eHTTPConnection>;
		conn->setAutoDelete(true);
	}
	conn->push_back(c);
	garbage.start(0, 1);
}

eHTTPGarbage::eHTTPGarbage():garbage(eApp)
{
	instance=this;
	conn=0;
	CONNECT(garbage.timeout, eHTTPGarbage::doGarbage);
}

eHTTPGarbage::~eHTTPGarbage()
{
	if (conn)
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
	eString error="unknown error";
	switch (errcode)
	{
	case 400: error="Bad Request"; break;
	case 403: error="Forbidden"; break;
	case 404: error="Not found"; break;
	case 500: error="Internal server error"; break;
	}
	connection->code_descr=error;
	connection->code=errcode;
	
	connection->local_header["Content-Type"]=std::string("text/html");
}

int eHTTPError::doWrite(int w)
{
	eString html;
	html+="<html><head><title>Error "+eString().setNum(connection->code)+"</title></head>"+
		"<body><h1>Error "+eString().setNum(errcode)+": "+connection->code_descr+"</h1></body></html>\n";
	connection->writeBlock(html.c_str(), html.length());
	return -1;
}

eHTTPConnection::eHTTPConnection(int socket, eHTTPD *parent): QSocket(parent), parent(parent)
{
	CONNECT(this->readyRead_ , eHTTPConnection::readData);
	CONNECT(this->bytesWritten_ , eHTTPConnection::bytesWritten);
	CONNECT(this->error_ , eHTTPConnection::error);
	setSocket(socket);
//	QHostAddress me=address(), he=peerAddress();

	buffersize=64*1024;
	dying=0;
	localstate=stateWait;
	remotestate=stateRequest;
	data=0;
}

eHTTPConnection::eHTTPConnection(const char *host, int port): QSocket(0), parent(0)
{
	CONNECT(this->readyRead_ , eHTTPConnection::readData);
	CONNECT(this->bytesWritten_ , eHTTPConnection::bytesWritten);
	CONNECT(this->error_ , eHTTPConnection::error);
	CONNECT(this->connected_ , eHTTPConnection::hostConnected);	

	connectToHost(host, port);
	dying=0;

	localstate=stateWait;
	remotestate=stateWait;
	
	buffersize=64*1024;
	data=0;
}

void eHTTPConnection::hostConnected()
{
	if (processLocalState())
		die();
}

void eHTTPConnection::start()
{
	if (localstate==stateWait)
	{
		localstate=stateRequest;
		if (processLocalState())
			die();
	}
}

eHTTPConnection *eHTTPConnection::doRequest(const char *uri, int *error)
{
	if (error)
		*error=0;

	char *defaultproto="http";
	std::string proto, host, path;
	int port=80;
	
	int state=0; // 0 proto, 1 host, 2 port 3 path
	
	while (*uri)
	{
		switch (state)
		{
		case 0:
			if (!strncmp(uri, "://", 3))
			{
				state=1;
				uri+=3;
			} else if ((*uri=='/') || (*uri==':'))
			{
				host=proto;
				state=1;
				proto=defaultproto;
			} else
				proto.push_back(*uri++);
			break;
		case 1:
			if (*uri=='/')
				state=3;
			else if (*uri==':')
			{
				state=2;
				port=0;
				uri++;
			} else
				host.push_back(*uri++);
			break;
		case 2:
			if (*uri=='/')
				state=3;
			else
			{
				if (!isdigit(*uri))
				{
					port=-1;
					state=3;
				} else
				{
					port*=10;
					port+=*uri++-'0';
				}
			}
			break;
		case 3:
			path.push_back(*uri++);
		}
	}
	
	if (state==0)
	{
		path=proto;
		proto=defaultproto;
	}

	eDebug("proto: '%s', host '%s', path '%s', port '%d'", proto.c_str(), host.c_str(), path.c_str(), port);

	if (!host.size())
	{
		eDebug("no host given");
		if (error)
			*error=ENOENT;
		return 0;
	}
	
	if (strcmp(proto.c_str(), "http"))
	{
		eDebug("invalid protocol (%s)", proto.c_str());
		if (error)
			*error=EINVAL;
		return 0;
	}
	
	if (port == -1)
	{
		eDebug("invalid port");
		if (error)
			*error=EINVAL;
		return 0;
	}
	
	if (!path.size())
		path="/";

	eHTTPConnection *c=new eHTTPConnection(host.c_str(), 80);
	c->request="GET";
	c->requestpath=path.c_str();
	return c;
}

void eHTTPConnection::die()
{
	if (!dying)
	{
		dying=1;
		emit closing();
		eHTTPGarbage::getInstance()->destruct(this);
		eDebug("destruct ok");
	}
}

void eHTTPConnection::readData()
{
	if (processRemoteState())
		die();
}

void eHTTPConnection::bytesWritten(int)
{
	if (processLocalState())
		die();
}

int eHTTPConnection::processLocalState()
{
	switch (state())
	{
	case Connection:
		break;
	default:
		return 0;
	}
	int done=0;
	while (!done)
	{
		switch (localstate)
		{
		case stateWait:
			eDebug("local wait");
			done=1;
			break;
		case stateRequest:
		{
			eDebug("local request");
			eString req=request+" "+requestpath+" "+httpversion+"\r\n";
			writeBlock(req.c_str(), req.length());
			localstate=stateHeader;
			remotestate=stateResponse;
			break;
		}
		case stateResponse:
		{
			writeString( (httpversion + " " + eString().setNum(code)+" " + code_descr + "\r\n").c_str() );
			localstate=stateHeader;
			local_header["Connection"]="close";
			break;
		}
		case stateHeader:
			eDebug("local header");
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
			eDebug("local data");
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
				localstate=stateWait;
			break;
		case stateDone:
			localstate=stateClose;
			break;
		case stateClose:
			close();		// bye, bye, remote
			if (state() == Idle)
			{
				eDebug("state: IDLE!");
				return 1;
			}
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
			eDebug("remote stateWait");
			char buffer[1024];
			while (bytesAvailable())
				readBlock(buffer, 1024);
			done=1;
			break;
		}
		case stateRequest:
		{
			eString line;
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
			eDebug("remote stateHeader");
			eString line;
			if (!getLine(line))
			{
				done=1;
				break;
			}
			if (!line.length())
			{
				localstate=stateResponse;		// can be overridden by dataSource
				for (ePtrList<eHTTPPathResolver>::iterator i(parent->resolver); i != parent->resolver.end(); ++i)
				{
					if ((data=i->getDataSource(request, requestpath, this)))
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
				eString name=line.left(del), value=line.mid(del+2);
				remote_header[std::string(name)]=std::string(value);
				const char *ct="Content-Type";
			}
			done=1;
			break;
		}
		case stateData:
		{
			eDebug("remote stateData");
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
			eDebug("remote stateDone");
			remotestate=stateClose;
			break;
		case stateClose:
			eDebug("remote stateClose");
			remotestate=stateWait;
			abort=1;
			break;
		default:
			eDebug("bla wrong state");
			done=1;
		}
	}
	return 0;
}

void eHTTPConnection::writeString(const char *string)
{
	writeBlock(string, strlen(string));
}

int eHTTPConnection::getLine(eString &line)
{
	if (!canReadLine())
		return 0;

	line = (const char*) readLine();

	line.erase(line.length()-1);

	if (line[(line.length()-1)] == '\r')
		line.erase(line.length()-1);

	return 1;
}

void eHTTPConnection::gotError()
{
	eDebug("ich hab nen ERROR");
	die();
}

eHTTPD::eHTTPD(Q_UINT16 port, int backlog, QObject *parent, const char *name): QServerSocket(port, backlog, parent, name)
{
	new eHTTPGarbage;
	if (!ok())
		eDebug("[NET] httpd server FAILED on port %d", port);
	else
		eDebug("[NET] httpd server started on port %d", port);
	resolver.setAutoDelete(true);
}

eHTTPConnection::~eHTTPConnection()
{
	eDebug("~eHTTPConnection");
	if (state()!=Idle)
		qFatal("~eHTTPConnection, status still %d", state());
	eDebug("data %x", data);
	if (data)
		delete data;
	eDebug("ok");
}

void eHTTPD::newConnection(int socket)
{
	eHTTPConnection *conn=new eHTTPConnection(socket, this);
	CONNECT(conn->connectionClosed_ , eHTTPD::oneConnectionClosed);
	CONNECT(conn->delayedCloseFinished_ , eHTTPD::oneConnectionClosed);	
}

void eHTTPD::oneConnectionClosed()
{
	eHTTPGarbage::getInstance()->destruct((eHTTPConnection*)sender());
}
