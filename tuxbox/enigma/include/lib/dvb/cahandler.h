#ifndef __DVB_CAHANDER_H_
#define __DVB_CAHANDLER_H_

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <map>
#include <lib/dvb/dvbservice.h>

class CAService
{
	int sock, clilen, lastPMTVersion;
	struct sockaddr_un servaddr;
	eServiceReferenceDVB me;
public:
	CAService( const eServiceReferenceDVB &service )
		:lastPMTVersion(-1), me(service)
	{
		memset(&servaddr, 0, sizeof(struct sockaddr_un));
		servaddr.sun_family = AF_UNIX;
		strcpy(servaddr.sun_path, "/tmp/camd.socket");
		clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);
		sock = socket(PF_UNIX, SOCK_STREAM, 0);
		connect(sock, (struct sockaddr *) &servaddr, clilen);
//		eDebug("[eDVBCAHandler] new service %s", service.toString().c_str() );
	}
	~CAService()
	{
		::close(sock);
//		eDebug("[eDVBCAHandler] leave service %s", me.toString().c_str() );
	}
	void sendCAPMT( PMT *pmt );
};

class eDVBCAHandler: public eDVBCaPMTClient, public Object
{
	std::map<eServiceReferenceDVB, CAService*> services;
	void leaveTransponder( eTransponder* );
public:
	void enterService( const eServiceReferenceDVB &service)
	{
		std::map<eServiceReferenceDVB, CAService*>::iterator it(services.find(service));
		if ( it == services.end() )
			services[service] = new CAService( service );
	}
	void leaveService( const eServiceReferenceDVB &service )
	{
		std::map<eServiceReferenceDVB, CAService*>::iterator it(services.find(service));
		if ( it != services.end() )
		{
			delete it->second;
			services.erase(it);
		}
	}
	void handlePMT( const eServiceReferenceDVB &service, PMT *pmt )
	{
		std::map<eServiceReferenceDVB, CAService*>::iterator it(services.find(service));
		if ( it != services.end() )
			it->second->sendCAPMT(pmt);
	}
	eDVBCAHandler();
	~eDVBCAHandler();
};

#endif // __DVB_CAHANDLER_H_
