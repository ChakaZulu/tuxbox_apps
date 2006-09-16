//=============================================================================
// YHTTPD
// Webserver Class : Until now: exact one instance 
//=============================================================================
// c++
#include <cerrno>
#include <csignal>

// system
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>

// tuxbox
#include <configfile.h>

// yhttpd
#include "yhttpd.h"
#include "ytypes_globals.h"
#include "ywebserver.h"
#include "ylogging.h"
#include "ysocket.h"
#include "yconnection.h"
#include "yrequest.h"

//=============================================================================
// Initialization of static variables
//=============================================================================
bool CWebserver::is_threading = true;

//=============================================================================
// Constructor & Destructor & Initialization
//=============================================================================
CWebserver::CWebserver()
{
	terminate = false;
	for(int i=0;i<HTTPD_MAX_CONNECTIONS;i++)
		Connection_Thread_List[i] = (pthread_t)NULL;
	for(int i=0;i < HTTPD_MAX_CONNECTIONS;i++) 
		SocketList[i] = NULL;
	FD_ZERO(&master);    // initialize FD_SETs
	FD_ZERO(&read_fds);
}
//-----------------------------------------------------------------------------
CWebserver::~CWebserver()
{
	ySock.close();
}
//=============================================================================
// Start Webserver. Main-Loop. 
//-----------------------------------------------------------------------------
// Wait for Connection and schedule ist to handle_connection()
// HTTP/1.1 should can handle "keep-alive" connections to reduce socket
// creation and handling. This is handled using FD_SET and select Socket 
// mechanism.
// select wait for socket-activity. Cases: 
//	1) get a new connection
//	2) re-use a socket
//	3) timeout: close unused sockets 
//-----------------------------------------------------------------------------
//	from RFC 2616:
//	8 Connections
//	8.1 Persistent Connections
//	8.1.1 Purpose
//	
//	   Prior to persistent connections, a separate TCP connection was
//	   established to fetch each URL, increasing the load on HTTP servers
//	   and causing congestion on the Internet. The use of inline images and
//	   other associated data often require a client to make multiple
//	   requests of the same server in a short amount of time. Analysis of
//	   these performance problems and results from a prototype
//	   implementation are available [26] [30]. Implementation experience and
//	   measurements of actual HTTP/1.1 (RFC 2068) implementations show good
//	   results [39]. Alternatives have also been explored, for example,
//	   T/TCP [27].
//	
//	   Persistent HTTP connections have a number of advantages:
//	
//	      - By opening and closing fewer TCP connections, CPU time is saved
//	        in routers and hosts (clients, servers, proxies, gateways,
//	        tunnels, or caches), and memory used for TCP protocol control
//	        blocks can be saved in hosts.
//	
//	      - HTTP requests and responses can be pipelined on a connection.
//	        Pipelining allows a client to make multiple requests without
//	        waiting for each response, allowing a single TCP connection to
//	        be used much more efficiently, with much lower elapsed time.
//	
//	      - Network congestion is reduced by reducing the number of packets
//	        caused by TCP opens, and by allowing TCP sufficient time to
//	        determine the congestion state of the network.
//	
//	      - Latency on subsequent requests is reduced since there is no time
//	        spent in TCP's connection opening handshake.
//	
//	      - HTTP can evolve more gracefully, since errors can be reported
//	        without the penalty of closing the TCP connection. Clients using
//	        future versions of HTTP might optimistically try a new feature,
//	        but if communicating with an older server, retry with old
//	        semantics after an error is reported.
//	
//	   HTTP implementations SHOULD implement persistent connections.
//=============================================================================
bool CWebserver::run(void)
{
	if(!ySock.listen(port, HTTPD_MAX_CONNECTIONS))
	{
		dperror("Socket cannot bind and listen. Abort.\n");
		return false;
	}

	// initialize values for select
	int listener = ySock.get_socket();
	int fdmax;        		// maximum file descriptor number
	int newfd;        		// newly accept()ed socket descriptor
    	struct timeval tv;		// timeout struct
	FD_SET(listener, &master);	// add the listener to the master set
	fdmax = listener;		// init max fd

	// main Webserver Loop
	while(!terminate)
	{
		CySocket *connectionSock = NULL;
		read_fds = master; 	// copy it
		tv.tv_sec = 5;		// Set timeout var for every call of select (it might be modified by select)
		tv.tv_usec=0;
		
		int fd=select(fdmax+1, &read_fds, NULL, NULL, &tv);	// wait for socket activity
		if(fd == -1)		// we got an error
		{
            		perror("select");
            		return false;
		}
		if(fd == 0)		// we got a timeout
		{
			for(int j=0;j < HTTPD_MAX_CONNECTIONS;j++) 
				if(SocketList[j] != NULL 			// here is a socket
					&& !SocketList[j]->handling) 		// it is not handled
				{	//TODO: check how long the Socket should be keept alive
					connectionSock = SocketList[j];
					int thisSocket = connectionSock->get_socket();
					connectionSock->close();
					FD_CLR(thisSocket, &master); // remove from master set
					delete SocketList[j]; 
					SocketList[j] = NULL;
					
				}
			continue;	// mail loop again	
		}
		for(int i = 0; i <= fdmax; i++)
		{
			if (FD_ISSET(i, &read_fds)) 	// here activity?
			{ // we got one!!
				if (i == listener)	// handle new connections
				{
					log_level_printf(6,"FD: new con fd:%d\n",i);
			 		if(!(connectionSock = ySock.accept() ))	//Blocking wait
					{
						dperror("Socket accept error. Continue.\n");
						delete connectionSock;
						continue;
					}
					else
					{	// Add Socket to List
						newfd = connectionSock->get_socket();
						FD_SET(newfd, &master); 	// add to master set
						if (newfd > fdmax)    		// keep track of the maximum
							fdmax = newfd;
						for(int j=0;j < HTTPD_MAX_CONNECTIONS;j++) 
							if(SocketList[j] == NULL)
							{
								SocketList[j]=connectionSock; // put it to list
								break;
							}
					}
				}
				else // Connection on an existing open Socket
				{
					for(int j=0;j < HTTPD_MAX_CONNECTIONS;j++) 
						if(SocketList[j] != NULL 			// here is a socket
							&& SocketList[j]->get_socket() == i 	// we know that socket
							&& !SocketList[j]->handling) 		// it is not handled (else a connection-thread is handing this socket)
						{
							connectionSock = SocketList[j];		// get socket to re-use
							log_level_printf(6,"FD: find con fd:%d\n",i);
							break;
						}
				}
			}
		}
		if(connectionSock != NULL)
		{
#ifdef Y_CONFIG_USE_OPEN_SSL
			if(Cyhttpd::ConfigList["SSL"]=="true")
				connectionSock->initAsSSL();	// make it a SSL-socket
#endif
			handle_connection(connectionSock);	// handle this activity
		}
	}
	return true;
}
//-----------------------------------------------------------------------------
// Close (FD_SET handled) Socket
// Clear it from SocketList 
//-----------------------------------------------------------------------------
void CWebserver::close_socket(SOCKET thisSocket)
{
	for(int j=0;j < HTTPD_MAX_CONNECTIONS;j++) 
		if(SocketList[j] != NULL 				// here is a socket
			&& SocketList[j]->get_socket() == thisSocket) 	// it is not handled
		{
			SocketList[j]->handling = false;
			SocketList[j]->close();				// close the socket
			FD_CLR(thisSocket, &master); 			// remove from master set
			delete SocketList[j]; 				// destroy ySocket
			SocketList[j] = NULL;				// free in list
		}
}
//-----------------------------------------------------------------------------
// A new Connection is established to newSock. Create a (threaded) Connection
// and handle the Request.
//-----------------------------------------------------------------------------
void CWebserver::handle_connection(CySocket *newSock)
{
	void *WebThread(void *args); //forward declaration
	// create arguments
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		
	TWebserverConnectionArgs *newConn = new TWebserverConnectionArgs;
	newConn->ySock = newSock;
	newConn->ySock->handling = true;
	newConn->WebserverBackref = this;
	newConn->is_treaded = is_threading;

	if(is_threading)
	{
		int index = -1;
		for(int i=0;i<HTTPD_MAX_CONNECTIONS;i++)
			if(Connection_Thread_List[i] == (pthread_t)NULL)
			{
				index = i;
				break;
			}
		if(index == -1)
		{
			dperror("Maximum Connection-Threads reached\n");
			return;
		}
		newConn->thread_number = index;
		if(pthread_create(&Connection_Thread_List[index], &attr, WebThread, (void *)newConn) != 0)
			dperror("Could not create Connection-Thread\n");		
	}
	else	// non threaded
		WebThread((void *)newConn);
}
//-------------------------------------------------------------------------
// Webserver Thread for each connection
//-------------------------------------------------------------------------
void *WebThread(void *args)
{
	CWebserverConnection *con;
	CWebserver *ws;
	TWebserverConnectionArgs *newConn = (TWebserverConnectionArgs *) args;
	ws = newConn->WebserverBackref;

	bool is_threaded = newConn->is_treaded;
	if(is_threaded)
		dprintf("++ Thread 0x06%X gestartet\n", (int) pthread_self());

	if (!newConn) {
		dperror("WebThread called without arguments!\n");
		if(newConn->is_treaded)
			pthread_exit(NULL);
	}

	if(is_threaded)
		pthread_detach(pthread_self());

	// (1) create & init Connection
	con = new CWebserverConnection(ws);
	con->Request.UrlData["clientaddr"] = newConn->ySock->get_client_ip(); // TODO:here?
	con->sock = newConn->ySock;		// give socket reference
	newConn->ySock->handling = true;	// dont handle this socket now be webserver main loop

	// (2) handle the connection
	con->HandleConnection();

	// (3) end connection handling
	con->sock->handling = false; 	// socket can be handled by webserver main loop (select) again
	if(!con->keep_alive)
		ws->close_socket(con->sock->get_socket());
	if(is_threaded)
		dprintf("-- Thread 0x06%X beendet\n",(int)pthread_self());

	// end thread
	if(is_threaded)
		ws->clear_Thread_List_Number(newConn->thread_number);

	delete con;
	delete newConn;
	if(is_threaded)
		pthread_exit(NULL);
	return NULL;
}

