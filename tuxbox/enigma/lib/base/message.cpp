#include "message.h"
#include <unistd.h>

eMessagePump::eMessagePump()
{
	pipe(fd);
}

eMessagePump::~eMessagePump()
{
	close(fd[0]);
	close(fd[1]);
}

int eMessagePump::send(const void *data, int len)
{
	return ::write(fd[1], data, len)<0;
}

int eMessagePump::recv(void *data, int len)
{
	unsigned char*dst=(unsigned char*)data;
	while (len)
	{
		int r=::read(fd[0], dst, len);
		if (r<0)
			return r;
		dst+=r;
		len-=r;
	}
	return 0;
}

int eMessagePump::getInputFD() const
{
	return fd[1];
}

int eMessagePump::getOutputFD() const
{
	return fd[0];
}
