#ifndef __serversocket_h
#define __serversocket_h

#include "socket.h"

class eServerSocket: public eSocket
{
	public:
		eSocketNotifier *n;
		eSocket *socket;

		eServerSocket(int port);
		~eServerSocket();
		bool ok();
		
	private:
		int okflag;
		void incomingConnection(int handle);

};

#endif /* __serversocket_h */
