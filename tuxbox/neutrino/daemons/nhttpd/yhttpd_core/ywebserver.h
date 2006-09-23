//=============================================================================
// YHTTPD
// Webserver Class : Until now: exact one instance 
//-----------------------------------------------------------------------------
// The Webserver Class creates one "master" Listener Socket on given Port.
// If a Connection is accepted, a new Socket ist used. The Webserver creates
// a new Connection-Class witch is (normaly) Threaded. The Connection-Class
// handles Request and Response.
// For HTTP/1.1 permanent Connections (keep-alive) socket-multiplexing using
// FD_SET and select() is implemented. Sockets are stored in SocketList and
// can be reused for the connected client (to reduce socket handling overhead).
// Reuse of connections is often done by pipelinig (read->send->read->send ...)
//=============================================================================
#ifndef __yhttpd_ywebserver_h__
#define __yhttpd_ywebserver_h__

// system
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>

// yhttpd
#include "yconfig.h"
#include "ytypes_globals.h"
#include "ysocket.h"

//-----------------------------------------------------------------------------
class CWebserver; //forward declaration

//-----------------------------------------------------------------------------
// Arguments for Connection-Thread
//-----------------------------------------------------------------------------
typedef struct
{
	CySocket 	*ySock;			// Connection "Slave" Socket
	CWebserver 	*WebserverBackref;	// Backreference to Webserver
	bool		is_treaded;		// Use threading?
	int		thread_number;		// Number of Thread (informational use)
} TWebserverConnectionArgs;

//-----------------------------------------------------------------------------
// until now: One Instance for one hosted Web
//-----------------------------------------------------------------------------
class CWebserver
{
private:
	static pthread_mutex_t	mutex;
	pthread_t 	Connection_Thread_List[HTTPD_MAX_CONNECTIONS]; //Thread-List per webserver
	fd_set 		master;   		// master file descriptor list for select()
	fd_set 		read_fds; 		// read file descriptor list for select()
	fd_set 		write_fds; 		// read file descriptor list for select()
	int 		fdmax;        		// maximum file descriptor number
	CySocket 	*SocketList[HTTPD_MAX_CONNECTIONS];	// List of concurrent hadled connections
protected:
	bool 		terminate;		// flag: indicate to terminate the Webserver
	CySocket 	listenSocket;		// Master Socket for listening
	unsigned int 	port;			// Port to listen on
	void 		handle_connection(CySocket *newSock);// Create a new Connection Instance and handle Connection
	int 		AcceptNewConnectionSocket();
	int 		GetExistingConnectionSocket(SOCKET sock);
	void 		CloseConnectionSocketsByTimeout();
public:
	static bool 	is_threading;		// Use Threading for new Connections

	// constructor & destructor
	CWebserver();
	~CWebserver(void);

	void 		init(unsigned int _port, bool _is_threading) // Initialize Webserver Settings
				{port=_port; is_threading=_is_threading;}
	bool 		run(void);		// Start the Webserver
	void 		stop(void)		// Stop the Webserver
				{terminate=true;}; 

	void 		clear_Thread_List_Number(int number)// Set Entry(number)to NULL in Threadlist 
				{if(number <HTTPD_MAX_CONNECTIONS)Connection_Thread_List[number]=(pthread_t)NULL;}
	void		close_socket(SOCKET thisSocket);	// closes socket by application
	void		addSocketToMasterSet(SOCKET fd); 
};

#endif // __yhttpd_ywebserver_h__
