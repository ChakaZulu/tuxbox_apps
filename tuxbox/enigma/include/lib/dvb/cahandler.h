#ifndef __DVB_CAHANDLER_H_
#define __DVB_CAHANDLER_H_

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <lib/base/eptrlist.h>
#include <lib/dvb/dvbservice.h>

class CAService: public Object
{
	int sock, clilen, lastPMTVersion;
	struct sockaddr_un servaddr;
	eServiceReferenceDVB me;
	unsigned int state;
	unsigned char *capmt;
	eTimer retry;
public:
	const eServiceReferenceDVB &getRef() const { return me; }
	void sendCAPMT();
	void Connect();
	CAService( const eServiceReferenceDVB &service )
		:lastPMTVersion(-1), me(service), state(0), capmt(NULL), retry(eApp)
	{
		CONNECT(retry.timeout, CAService::sendCAPMT);
		Connect();
//		eDebug("[eDVBCAHandler] new service %s", service.toString().c_str() );
	}
	~CAService()
	{
		delete [] capmt;
		::close(sock);
//		eDebug("[eDVBCAHandler] leave service %s", me.toString().c_str() );
	}
	void buildCAPMT( PMT *pmt );
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
			it->buildCAPMT(pmt);
	}
	eDVBCAHandler();
	~eDVBCAHandler();
};

#endif // __DVB_CAHANDLER_H_
