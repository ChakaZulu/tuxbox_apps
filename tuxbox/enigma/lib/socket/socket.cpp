#include "socket.h"

#include <sys/ioctl.h>
#include <asm/ioctls.h>
#include <unistd.h>
#include <asm/errno.h>

void eSocket::close()
{
	if(socketdesc)
		close(socketdesc);
}

void eSocket::close(int socket)
{
	if(socketdesc)
		close(socketdesc);
}

eString eSocket::readLine()
{
	int m=0, n=0;  // bytes read
	int maxlen=4095;
	char buf[4096];
	char *t=(char*) buf;

	while(*t!='\n' && n<maxlen)
	{
		if(m>0) {
			t++;
			n++;
		}
		if(n<maxlen)
			m=read(getDescriptor(), (void*) t, 1);
	}
	buf[n+1]='\0';
	return eString(buf);
}

bool eSocket::canReadLine()
{
	if(bytesAvailable())
		return(1);
	else
		return(0);
}

int eSocket::bytesAvailable()
{
	int bytesavail;
	ioctl(getDescriptor(), FIONREAD, &bytesavail);
	return bytesavail;
}

int eSocket::readBlock(char *data, unsigned int maxlen)
{
	return read(getDescriptor(), (void*)data, maxlen); 
}

int eSocket::bytesToWrite()
{
	return(0);
}

int eSocket::state()
{
	return Connection;
}

int eSocket::setSocket(int blub)
{
#if 0
	eDebug("[SOCKET] setting socket to non-blocking");
#endif
	fcntl(socketdesc, F_SETFL, O_NONBLOCK);
	rsn=new eSocketNotifier(eApp, getDescriptor(), eSocketNotifier::http);
	CONNECT(rsn->activated, eSocket::readit);
	return 0;
}


void eSocket::readit(int)
{
#if 0
	eDebug("[SOCKET] readit called");
#endif
	if(!bytesAvailable())
	{
		delete rsn;
#if 0
		eDebug("[SOCKET] closing...");
#endif
		connectionClosed_();
	}
	else
	{
		readyRead_();
	}
}

void eSocket::writeit(int)
{
#if 0
	eDebug("[SOCKET] writeit called");
#endif
	bytesWritten_(0);
}

int eSocket::writeBlock(const char *data, unsigned int len)
{
	rsn->stop();
	int byteswritten=send(getDescriptor(), data, len, MSG_NOSIGNAL);
	rsn->start();
	return byteswritten;
}

int eSocket::getDescriptor()
{
	return socketdesc;
}

int eSocket::connectToHost(eString hostname, int port)
{
	struct sockaddr_in	serv_addr;
	struct hostent		*server;

	if(!socketdesc){
		error_(-1);
		return(-1);
	}
	server=gethostbyname(hostname.c_str());
	if(server==NULL)
	{
		eDebug("can't resolve %s", hostname.c_str());
		error_(-1);
		return(-1);
	}
	bzero(	(char*)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	bcopy(	(char*)server->h_addr,
		(char*)&serv_addr.sin_addr.s_addr,
		server->h_length);
	serv_addr.sin_port=htons(port);
	if(connect(socketdesc, (const sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
	{
		eDebug("can't connet to host: %s", hostname.c_str());
		error_(-1);
		return(-1);
	}
	connected_();
	return(0);
}

eSocket::eSocket()
{
	socketdesc=0;
	socketdesc=socket(AF_INET, SOCK_STREAM, 0);
#if 0
	eDebug("[SOCKET]: initalized socket %d", socketdesc);
#endif
}

eSocket::eSocket(int socket)
{
	socketdesc=socket;
}

eSocket::~eSocket()
{
	delete rsn;
	if(socketdesc)
		close(socketdesc);
#if 0
	eDebug("[SOCKET]: destructed %d", socketdesc);
#endif
}
