#ifndef __socket_h
#define __socket_h

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

#include <lib/base/ebase.h>
#include <lib/base/eerror.h>
#include <lib/base/estring.h>
#include <libsig_comp.h>
#include <lib/base/buffer.h>

class eSocket: public Object
{
	int mystate;
private:
	int socketdesc;
	eIOBuffer readbuffer;
	eIOBuffer writebuffer;
	int writebusy;
protected:
	eSocketNotifier	*rsn;
	virtual void notifier(int);
public:
	eSocket();
	eSocket(int socket);
	~eSocket();
	int connectToHost(eString hostname, int port);
	int getDescriptor();
	int writeBlock(const char *data, unsigned int len);
	int setSocket(int blub);
	int bytesToWrite();
	int readBlock(char *data, unsigned int maxlen);
	int bytesAvailable();
	bool canReadLine();
	eString readLine();
	void close();
			
	enum State {	Idle, HostLookup, Connecting,
			Listening, Connection, Closing };
	int state();
	
	Signal0<void> connectionClosed_;
	Signal0<void> connected_;
	Signal0<void> readyRead_;
	Signal1<void,int> bytesWritten_;
	Signal1<void,int> error_;
};

#endif /* __socket_h */
