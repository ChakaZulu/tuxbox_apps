#include "serversocket.h"

bool eServerSocket::ok()
{
	return okflag;
}

void eServerSocket::incomingConnection(int handle)
{
	int clientfd, clientlen;
	struct sockaddr_in client_addr;

#if 0
	eDebug("[SERVERSOCKET] incoming connection! handle: %d", handle);
#endif

	clientlen=sizeof(client_addr);
	clientfd=accept(socket->getDescriptor(),
			(struct sockaddr *) &client_addr,
			(socklen_t*)&clientlen);
	if(clientfd<0)
		eDebug("[SERVERSOCKET] error on accept()");

	newConnection(clientfd);
}

eServerSocket::eServerSocket(int port)
{
	struct sockaddr_in serv_addr;

	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=INADDR_ANY;
	serv_addr.sin_port=htons(port);

	okflag=1;
	socket=new eSocket();
#if 0
	eDebug("[SERVERSOCKET] enigma-http-server bound on port %d", port);
#endif

	int val=1;

	setsockopt(socket->getDescriptor(), SOL_SOCKET, SO_REUSEADDR, &val, 4);

	if(bind(socket->getDescriptor(),
		(struct sockaddr *) &serv_addr,
		sizeof(serv_addr))<0)
	{
		eDebug("[SERVERSOCKET] ERROR on bind() (%m)");
		okflag=0;
	}
	listen(socket->getDescriptor(), 0);
	n=new eSocketNotifier(eApp, socket->getDescriptor(), eSocketNotifier::http);
	CONNECT(n->activated, eServerSocket::incomingConnection);
}

eServerSocket::~eServerSocket()
{
#if 0
	eDebug("[SERVERSOCKET] destructed");
#endif
	delete n;
}
