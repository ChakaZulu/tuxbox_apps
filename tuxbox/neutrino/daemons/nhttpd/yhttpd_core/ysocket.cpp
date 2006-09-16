//=============================================================================
// YHTTPD
// Socket Class : Basic Socket Operations 
//=============================================================================

#include <cstring>

// system
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
// yhttpd
#include "yhttpd.h"
#include "ysocket.h"
#include "ylogging.h"
// system
#ifdef Y_CONFIG_HAVE_SENDFILE
#include <sys/sendfile.h>
#endif
#ifdef Y_CONFIG_USE_OPEN_SSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#endif

//=============================================================================
// Initialization of static variables
//=============================================================================
#ifdef Y_CONFIG_USE_OPEN_SSL
SSL_CTX 	*CySocket::SSL_ctx;
std::string	CySocket::SSL_pemfile;
std::string	CySocket::SSL_CA_file;
#endif

//=============================================================================
// Constructor & Destructor & Initialization
//=============================================================================
CySocket::CySocket()
	: sock(0)
{
#ifdef Y_CONFIG_USE_OPEN_SSL
	ssl = NULL;
#endif
	BytesSend =0;
	sock = socket(AF_INET,SOCK_STREAM,0);
	init();
}
//-----------------------------------------------------------------------------
CySocket::~CySocket()
{
#ifdef Y_CONFIG_USE_OPEN_SSL
	if(isSSLSocket && ssl != NULL)
		SSL_free(ssl);
#endif
}
//-----------------------------------------------------------------------------
// initialize
//-----------------------------------------------------------------------------
void CySocket::init(void)
{
	handling 	= false;
	isOpened	= false;
	addr_len 	= sizeof(addr);
	memset(&addr, 0, addr_len);
#ifdef Y_CONFIG_USE_OPEN_SSL
	isSSLSocket 	= false;
#endif
}
#ifdef Y_CONFIG_USE_OPEN_SSL
//-----------------------------------------------------------------------------
// Initialize this socket as a SSL-socket
//-----------------------------------------------------------------------------
bool CySocket::initAsSSL(void)
{
	isSSLSocket = true;
	if (NULL == (ssl = SSL_new(CySocket::SSL_ctx)))	// create SSL-socket
	{
		aprintf("ySocket:SSL Error: Create SSL_new : %s\n", ERR_error_string(ERR_get_error(), NULL) );
		return false;
	}
	SSL_set_accept_state(ssl);			// accept connection
	if(1 != (SSL_set_fd(ssl, sock)))		// associate socket descriptor
	if (NULL == (ssl = SSL_new(CySocket::SSL_ctx)))
	{
		aprintf("ySocket:SSL Error: Create SSL_new : %s\n", ERR_error_string(ERR_get_error(), NULL) );
		return false;
	}
	return true;
}
//-----------------------------------------------------------------------------
// static: initialize the SSL-Library and the SSL ctx object.
// Read and assoziate the keyfile.
//-----------------------------------------------------------------------------
bool CySocket::initSSL(void)
{
	SSL_load_error_strings();	// Load SSL Error Strings
	SSL_library_init();		// Load SSL Library
	if (0 == RAND_status())		// set Random
	{
		aprintf("ySocket:SSL got no rand\n");
		return false;
	}
	if((SSL_ctx = SSL_CTX_new(SSLv23_server_method())) == NULL) // create master ctx
	{
		aprintf("ySocket:SSL Error: Create SSL_CTX_new : %s\n", ERR_error_string(ERR_get_error(), NULL) );
		return false;
	}
	if(SSL_pemfile == "")
	{
		aprintf("ySocket:SSL Error: no pemfile given\n");
		return false;
	}
	if(SSL_CA_file != "")		// have a CA?
		if(1 != SSL_CTX_load_verify_locations(SSL_ctx, SSL_CA_file.c_str(), NULL))
		{
			aprintf("ySocket:SSL Error: %s CA-File:%s\n",ERR_error_string(ERR_get_error(), NULL), SSL_CA_file.c_str());
			return false;
		}
	if(SSL_CTX_use_certificate_file(SSL_ctx, SSL_pemfile.c_str(), SSL_FILETYPE_PEM) < 0)
	{
		aprintf("ySocket:SSL Error: %s PEM-File:%s\n",ERR_error_string(ERR_get_error(), NULL), SSL_pemfile.c_str());
		return false;
	}

	if(SSL_CTX_use_PrivateKey_file(SSL_ctx, SSL_pemfile.c_str(), SSL_FILETYPE_PEM) < 0)
	{
		aprintf("ySocket:SSL Error: Private Keys: %s PEM-File:%s\n",ERR_error_string(ERR_get_error(), NULL), SSL_pemfile.c_str());
		return false;
	}
	if(SSL_CTX_check_private_key(SSL_ctx) != 1)
	{
		aprintf("ySocket:SSL Error: Private Keys not compatile to public keys: %s PEM-File:%s\n",ERR_error_string(ERR_get_error(), NULL), SSL_pemfile.c_str());
		return false;
	}
	return true;
}
#endif
//=============================================================================
// Socket handling
//=============================================================================
void CySocket::close(void)
{
	if(sock != 0 && sock != INVALID_SOCKET)
		::close(sock);
	sock = 0;
	isOpened = false;
}
//-----------------------------------------------------------------------------
void CySocket::shutdown(void)
{
	if(sock != 0 && sock != INVALID_SOCKET)
		::shutdown(sock,SHUT_RDWR);
}
//-----------------------------------------------------------------------------
bool CySocket::listen(int port, int max_connections)
{
	if(sock == INVALID_SOCKET)
		return false;
		
	// set sockaddr for listening
	sockaddr_in sai;
  	memset(&sai, 0, sizeof(sai));
	sai.sin_family 		= AF_INET;		// Protocol
	sai.sin_addr.s_addr 	= htonl(INADDR_ANY); 	// No Filter
	sai.sin_port 		= htons(port);		// Listening Port

	set_reuse_port();				// Re-Use Port
	set_reuse_addr();				// Re-Use IP
	if(bind(sock, (sockaddr *)&sai, sizeof(sockaddr_in)) != SOCKET_ERROR)
		if(::listen(sock, max_connections) == 0)
			return true;
	return false;
}
//-----------------------------------------------------------------------------
CySocket* CySocket::accept()
{
	init();
	SOCKET newSock = ::accept(sock, (sockaddr *) &addr, &addr_len);
	if(newSock == INVALID_SOCKET)
	{
		dperror("accept: invalid socket\n");
		return NULL;
	}
	CySocket *new_ySocket = new CySocket(newSock);
	if(new_ySocket != NULL)
	{
		new_ySocket->setAddr(addr);
#ifdef TCP_CORK
		new_ySocket->set_option(SOL_TCP, TCP_CORK);
#endif	
	}
	isOpened = true;
	handling = true;
	return new_ySocket;
}
//-----------------------------------------------------------------------------
std::string CySocket::get_client_ip(void)
{
	return inet_ntoa(addr.sin_addr);
}

//-----------------------------------------------------------------------------
void CySocket::setAddr(sockaddr_in _addr)
{
	addr = _addr;
}

//-----------------------------------------------------------------------------
// Set Socket Option (return = false = error)
//-----------------------------------------------------------------------------
bool CySocket::set_option(int typ, int option)
{
	int on = 1;
	return (setsockopt(sock, typ, option, (char *)&on, sizeof(on)) >= 0);
}

//-----------------------------------------------------------------------------
// Set Re-Use Option for Port.
//-----------------------------------------------------------------------------
void CySocket::set_reuse_port()
{
#ifdef SO_REUSEPORT
	if(!set_option(SOL_SOCKET, SO_REUSEPORT))
		dperror("setsockopt(SO_REUSEPORT)\n");
#endif
}

//-----------------------------------------------------------------------------
// Set Re-Use Option for Address.
//-----------------------------------------------------------------------------
void CySocket::set_reuse_addr()
{
#ifdef SO_REUSEADDR
	if(!set_option(SOL_SOCKET, SO_REUSEADDR))
		dperror("setsockopt(SO_REUSEADDR)\n");
#endif
}
//=============================================================================
// Send and receive
//=============================================================================
//-----------------------------------------------------------------------------
// Read a buffer (normal or SSL)
//-----------------------------------------------------------------------------
int CySocket::Read(char *buffer, unsigned int length)
{
#ifdef Y_CONFIG_USE_OPEN_SSL
	if(isSSLSocket)
		return SSL_read(ssl, buffer, length);
	else
#endif
		return ::read(sock, buffer, length);
}
//-----------------------------------------------------------------------------
// Send a buffer (normal or SSL)
//-----------------------------------------------------------------------------
int CySocket::Send(char const *buffer, unsigned int length)
{
	unsigned int len = 0;
#ifdef Y_CONFIG_USE_OPEN_SSL
	if(isSSLSocket)
		len = SSL_write(ssl, buffer, length);
	else
#endif
		len = ::send(sock, buffer, length, MSG_NOSIGNAL);
	if(len >= 0)
		BytesSend += len;
	return len;
}
//-----------------------------------------------------------------------------
// BASIC Send File over Socket for FILE*
// fd is an opened FILE-Descriptor
//-----------------------------------------------------------------------------
int CySocket::SendFile(int filed)
{
#ifdef Y_CONFIG_HAVE_SENDFILE
	// does not work with SSL !!!
	off_t start = 0;
	off_t end = lseek(filed,0,SEEK_END);
	int written = 0;
	if((written = ::sendfile(sock,filed,&start,end)) == -1)
	{
		perror("sendfile failed\n");
   		return false;
	}
	else
		BytesSend += written;
#else
	char sbuf[1024];
	unsigned int r=0;
	while ((r=read(filed, sbuf, 1024)) > 0) 
	{
    		if (Send(sbuf, r) < 0)
    		{
			perror("sendfile failed\n");
      			return false;
      		}
  	}
#endif // Y_CONFIG_HAVE_SENDFILE
	log_level_printf(9,"<Sock:SendFile>: Bytes:%ld\n", BytesSend);  
	return true;
}
//-----------------------------------------------------------------------------
// Receive File over Socket for FILE* filed
// read/write in small blocks (to save memory).
// usind sleep for sync input
// fd is an opened FILE-Descriptor
//-----------------------------------------------------------------------------
//TODO: Write upload Progress Informations into a file
unsigned int CySocket::ReceiveFileGivenLength(int filed, unsigned int _length)
{
	unsigned int _readbytes = 0;
	char buffer[RECEIVE_BLOCK_LEN];
	int retries=0;

	do
	{
		// check bytes in Socket buffer
		u_long readarg = 0;
		if(ioctl(sock, FIONREAD, &readarg) != 0)
			break;
		if(readarg > RECEIVE_BLOCK_LEN)		// enough bytes to read
			readarg = RECEIVE_BLOCK_LEN;	// read only given length

      		if(readarg == 0) 			// nothing to read: sleep
      		{
      			retries++;
      			if(retries >NON_BLOCKING_MAX_RETRIES)
      				break;
      			sleep(1);
      		}
      		else
      		{
			int bytes_gotten = Read(buffer, readarg);
			if(bytes_gotten <= 0)		// ERROR Code gotten
				break;		
			_readbytes += bytes_gotten;
	    		if (write(filed, buffer, bytes_gotten) != bytes_gotten)
	    		{
				perror("write file failed\n");
	      			return 0;
	      		}
      			retries = 0;
      			if(bytes_gotten < NON_BLOCKING_TRY_BYTES) // to few bytes gotten: sleep
      				sleep(1);
      		}
		log_level_printf(8,"Receive Block length:%d all:%d\n",_readbytes, _length);
	}
	while(_readbytes + RECEIVE_BLOCK_LEN < _length);
	return _readbytes;
}
//-----------------------------------------------------------------------------
// read all data avaiable on Socket
//-----------------------------------------------------------------------------
std::string CySocket::ReceiveBlock()
{
	std::string result;
	char buffer[RECEIVE_BLOCK_LEN];
 
	while(true)
	{
		// check bytes in Socket buffer
		u_long readarg = 0;
		if(ioctl(sock, FIONREAD, &readarg) != 0)
			break;
		if(readarg == 0) 			// nothing to read
			break;
		if(readarg > RECEIVE_BLOCK_LEN)		// need more loops
			readarg = RECEIVE_BLOCK_LEN;
		int bytes_gotten = Read(buffer, readarg);
		if(bytes_gotten <= 0)			// ERROR Code gotten
			break;
		std::string tmp;
		tmp.assign(buffer, bytes_gotten);
		result += tmp;				//TODO: use append?
	}
	//TODO:raus:aprintf("rblock:(%s)\n",result.c_str());
	return result;
}

//-----------------------------------------------------------------------------
// Read on line (Ends with CRLF) or maximum MAX_LINE_BUFFER chars
//-----------------------------------------------------------------------------
std::string CySocket::ReceiveLine()
{
	char buffer[MAX_LINE_BUFFER];
	int bytes_gotten = 0;
	std::string result="";
	while(true)
	{
		// read one char		
		if(Read(buffer+bytes_gotten, 1) == 1)
			if(buffer[bytes_gotten] == '\n')
				break;
		if(bytes_gotten < MAX_LINE_BUFFER-1)
			bytes_gotten ++;
		else
			break;
	}
	buffer[++bytes_gotten] = '\0';
	result.assign(buffer, bytes_gotten);
	return result;
}
