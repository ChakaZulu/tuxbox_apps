#include <sys/ioctl.h>
#include <asm/ioctls.h>
#include <unistd.h>
#include <errno.h>

#include <core/socket/socket.h>

void eSocket::close()
{
	if (writebuffer.empty())
	{
		delete rsn;
		rsn=0;
		::close(socketdesc);
		socketdesc=-1;
		mystate=Idle;
		connectionClosed_();
	} else
		mystate=Closing;
}

eString eSocket::readLine()
{
	int size=readbuffer.searchchr('\n');
	if (size == -1)
		return eString();
	size++; // ich will auch das \n
	char buffer[size+1];
	buffer[size]=0;
	readbuffer.read(buffer, size);
	return eString(buffer);
}

bool eSocket::canReadLine()
{
	return readbuffer.searchchr('\n') != -1;
}

int eSocket::bytesAvailable()
{
	return readbuffer.size();
}

int eSocket::readBlock(char *data, unsigned int maxlen)
{
	return readbuffer.read(data, maxlen);
}

int eSocket::bytesToWrite()
{
	return writebuffer.size();
}

int eSocket::state()
{
	return mystate;
}

int eSocket::setSocket(int blub)
{
#if 0
	eDebug("[SOCKET] setting socket to non-blocking");
#endif
	fcntl(socketdesc, F_SETFL, O_NONBLOCK);
	rsn=new eSocketNotifier(eApp, getDescriptor(), 
		eSocketNotifier::Read|eSocketNotifier::Priority|
		eSocketNotifier::Hungup);
	CONNECT(rsn->activated, eSocket::notifier);
	return 0;
}

void eSocket::notifier(int what)
{
	if (what & eSocketNotifier::Read)
	{
		int bytesavail;
		if (ioctl(getDescriptor(), FIONREAD, &bytesavail)<0)
			eDebug("FIONREAD failed.\n");
		else
		{
			if (!bytesavail)  // does the REMOTE END has closed the connection? (no Hungup here!)
			{
				writebuffer.clear();
				close();
				return;
			}
			if (readbuffer.fromfile(getDescriptor(), bytesavail) != bytesavail)
				eDebug("fromfile failed!");
			readyRead_();
		}
	} else if (what & eSocketNotifier::Write)
	{
		if (!writebuffer.empty())
		{
			bytesWritten_(writebuffer.tofile(getDescriptor(), 65536));
			if (writebuffer.empty())
			{
				rsn->setRequested(rsn->getRequested()&~eSocketNotifier::Write);
				if (mystate == Closing)
				{
					eDebug("ok, we can close down now.");
					close();		// warning, we might get destroyed after close.
					return;
				}
			}
		} else
			eDebug("got ready to write, but nothin in buffer. strange.");
	} else if (what & eSocketNotifier::Priority)
	{
		eDebug("socket: urgent data available!");
	} else if (what & eSocketNotifier::Hungup)
	{
		eDebug("socket: hup");
		writebuffer.clear();
		close();
	}
}

int eSocket::writeBlock(const char *data, unsigned int len)
{
	int w=len;
	if (writebuffer.empty())
	{
		int tw=::write(getDescriptor(), data, len);
		if ((tw < 0) && (errno != EWOULDBLOCK))
			eDebug("write: %m");
		
		if (tw < 0)
			tw = 0;
		data+=tw;
		len-=tw;
	}
	if (len)
		writebuffer.write(data, len);

	if (!writebuffer.empty())
		rsn->setRequested(rsn->getRequested()|eSocketNotifier::Write);
	return w;
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
	mystate=Connection;
	connected_();
	return(0);
}

eSocket::eSocket(): readbuffer(32768), writebuffer(32768)
{
	socketdesc=socket(AF_INET, SOCK_STREAM, 0);
#if 0
	eDebug("[SOCKET]: initalized socket %d", socketdesc);
#endif
	setSocket(socketdesc);
}

eSocket::eSocket(int socket): readbuffer(32768), writebuffer(32768) 
{
	socketdesc=socket;
	setSocket(socketdesc);
	mystate=Connection;
}

eSocket::~eSocket()
{
	if (rsn)
		delete rsn;
	if(socketdesc>=0)
		::close(socketdesc);
}
