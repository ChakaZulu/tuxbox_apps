#ifndef __serversocket_h
#define __serversocket_h

#include "socket.h"

class eServerSocket: public eSocket
{
	public:
		eSocketNotifier *n;
		eSocket *socket;

		eServerSocket(int port);
		virtual ~eServerSocket();
		bool ok();

	protected:
		virtual void newConnection(int socket)=0;

	private:
		void incomingConnection(int handle);		
		int okflag;
};

#endif /* __serversocket_h */
