#ifndef __socket_h
#define __socket_h

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

#include <core/base/ebase.h>
#include <core/base/eerror.h>
#include <core/base/estring.h>
#include <include/libsig_comp.h>


class eSocket: public Object
{
	private:
		int socketdesc;
	public:
		eSocketNotifier	*rsn;
		eSocket();
		eSocket(int socket);
		~eSocket();
		void writeit(int);
		void readit(int);
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
		void close(int socket);
				
		enum State {	Idle, HostLookup, Connecting,
				Listening, Connection, Closing };
		int state();
		
		Signal0<void> connectionClosed_;
		Signal0<void> delayedCloseFinished_;
		Signal0<void> connected_;
		Signal0<void> readyRead_;
		Signal1<void,int> bytesWritten_;
		Signal1<void,int> error_;
};

#endif /* __socket_h */
