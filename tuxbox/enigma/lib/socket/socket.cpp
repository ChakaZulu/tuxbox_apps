#include <sys/ioctl.h>
#include <asm/ioctls.h>
#include <unistd.h>
#include <errno.h>

#include <core/socket/socket.h>

void eSocketBuffer::removeblock()
{
	ASSERT(!buffer.empty());
	eSocketBufferData &b=buffer.front();
	delete[] b.data;
	buffer.pop_front();
	ptr=0;
}

eSocketBuffer::eSocketBufferData &eSocketBuffer::addblock()
{
	eSocketBufferData s;
	s.data=new __u8[allocationsize];
	s.len=0;
	buffer.push_back(s);
	return buffer.back();
}

eSocketBuffer::~eSocketBuffer()
{
	clear();
}

void eSocketBuffer::clear()
{
	while (!buffer.empty())
		removeblock();
}

int eSocketBuffer::size() const
{
	int total=0;
	for (std::list<eSocketBufferData>::const_iterator i(buffer.begin()); i != buffer.end(); ++i)
		total+=i->len;
	total-=ptr;
	return total;
}

int eSocketBuffer::empty() const
{
	return buffer.empty();
}

int eSocketBuffer::peek(void *dest, int len) const
{
	__u8 *dst=(__u8*)dest;
	std::list<eSocketBufferData>::const_iterator i(buffer.begin());
	int p=ptr;
	int written=0;
	while (len)
	{	
		if (i == buffer.end())
			break;
		int tc=i->len-p;
		if (tc > len)
			tc = len;
	
		memcpy(dst, i->data+p, tc);
		dst+=tc;
		written+=tc;
	
		++i;
		p=0;
			
		len-=tc;
	}
	return written;
}

void eSocketBuffer::skip(int len)
{
	while (len)
	{
		ASSERT(! buffer.empty());
		int tn=len;
		if (tn > (buffer.front().len-ptr))
			tn=buffer.front().len-ptr;

		ptr+=tn;
		if (ptr == buffer.front().len)
			removeblock();
		len-=tn;
	}
}

int eSocketBuffer::read(void *dest, int len)
{
	__u8 *dst=(__u8*)dest;
	len=peek(dst, len);
	skip(len);
	return len;
}

void eSocketBuffer::write(const void *source, int len)
{
	const __u8 *src=(const __u8*)source;
	while (len)
	{
		int tc=len;
		if (buffer.empty() || (allocationsize == buffer.back().len))
			addblock();
		if (tc > allocationsize-buffer.back().len)
			tc=allocationsize-buffer.back().len;
		memcpy(buffer.back().data+buffer.back().len, src, tc);
		src+=tc;
		buffer.back().len+=tc;
		len-=tc;
	}
}

int eSocketBuffer::readfile(int fd, int len)
{
	int re=0;
	while (len)
	{
		int tc=len;
		int r;
		if (buffer.empty() || (allocationsize == buffer.back().len))
			addblock();
		if (tc > allocationsize-buffer.back().len)
			tc=allocationsize-buffer.back().len;
		r=::read(fd, buffer.back().data+buffer.back().len, tc);
		buffer.back().len+=r;
		len-=r;
		if (r < 0)
		{
			if (errno != EWOULDBLOCK)
				eDebug("read: %m");
			r=0;
		}
		re+=r;
		if (r != tc)
			break;
	}
	return re;
}

int eSocketBuffer::sendfile(int fd, int len)
{
	int written=0;
	int w;
	while (len && !buffer.empty())
	{	
		if (buffer.begin() == buffer.end())
			break;
		int tc=buffer.front().len-ptr;
		if (tc > len)
			tc = len;
	
		w=::write(fd, buffer.front().data+ptr, tc);
		if (w < 0)
		{
			if (errno != EWOULDBLOCK)
				eDebug("write: %m");
			w=0;
		}
		ptr+=w;
		if (ptr == buffer.front().len)
			removeblock();
		written+=w;	

		len-=w;
		if (tc != w)
			break;
	}
	return written;
}

int eSocketBuffer::searchchr(char ch) const
{
	std::list<eSocketBufferData>::const_iterator i(buffer.begin());
	int p=ptr;
	int c=0;
	while (1)
	{	
		if (i == buffer.end())
			break;
		while (p < i->len)
			if (i->data[p] == ch)
				return c;
			else
				c++, p++;

		++i;
		p=0;
	}
	return -1;
}

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
			if (readbuffer.readfile(getDescriptor(), bytesavail) != bytesavail)
				eDebug("readfile failed!");
			readyRead_();
		}
	} else if (what & eSocketNotifier::Write)
	{
		if (!writebuffer.empty())
		{
			bytesWritten_(writebuffer.sendfile(getDescriptor(), 65536));
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
