#ifndef __DVB_CAHANDLER_H_
#define __DVB_CAHANDLER_H_

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <lib/base/eptrlist.h>
#include <lib/dvb/dvbservice.h>

class CAService
{
	int sock, clilen, lastPMTVersion;
	struct sockaddr_un servaddr;
	eServiceReferenceDVB me;
public:
	const eServiceReferenceDVB &getRef() const { return me; }
	CAService( const eServiceReferenceDVB &service )
		:lastPMTVersion(-1), me(service)
	{
		memset(&servaddr, 0, sizeof(struct sockaddr_un));
		servaddr.sun_family = AF_UNIX;
		strcpy(servaddr.sun_path, "/tmp/camd.socket");
		clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);
		sock = socket(PF_UNIX, SOCK_STREAM, 0);
		connect(sock, (struct sockaddr *) &servaddr, clilen);
		fcntl(sock, F_SETFL, O_NONBLOCK);
		int val=1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, 4);
//		eDebug("[eDVBCAHandler] new service %s", service.toString().c_str() );
	}
	~CAService()
	{
		::close(sock);
//		eDebug("[eDVBCAHandler] leave service %s", me.toString().c_str() );
	}
	void sendCAPMT( PMT *pmt );
};

static bool operator==( const CAService *caservice, const eServiceReferenceDVB &service )
{
	return caservice->getRef() == service;
}

class eDVBCAHandler: public eDVBCaPMTClient, public Object
{
	ePtrList<CAService> services;
	void leaveTransponder( eTransponder* );
public:
	void enterService( const eServiceReferenceDVB &service)
	{
		ePtrList<CAService>::iterator it =
			std::find(services.begin(), services.end(), service );
		if ( it == services.end() )
			services.push_back(new CAService( service ));
	}
	void leaveService( const eServiceReferenceDVB &service )
	{
		ePtrList<CAService>::iterator it =
			std::find(services.begin(), services.end(), service );
		if ( it != services.end() )
			services.erase(it);
	}
	void handlePMT( const eServiceReferenceDVB &service, PMT *pmt )
	{
		ePtrList<CAService>::iterator it =
			std::find(services.begin(), services.end(), service );
		if ( it != services.end() )
			it->sendCAPMT(pmt);
	}
	eDVBCAHandler();
	~eDVBCAHandler();
};

#endif // __DVB_CAHANDLER_H_
