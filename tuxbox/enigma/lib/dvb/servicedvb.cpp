#include "servicedvb.h"
#include <core/dvb/edvb.h>
#include <core/dvb/dvbservice.h>
#include <core/system/init.h>
#include <core/driver/streamwd.h>

int eServiceHandlerDVB::getID() const
{
	return eServiceReference::idDVB;
}

void eServiceHandlerDVB::scrambledStatusChanged(bool scrambled)
{
	int oldflags=flags;
	if (scrambled)
		flags|=flagIsScrambled;
	else
		flags&=~flagIsScrambled;
	if (oldflags != flags)
		serviceEvent(eServiceEvent(eServiceEvent::evtFlagsChanged) );
}

void eServiceHandlerDVB::switchedService(const eServiceReferenceDVB &, int err)
{
	int oldstate=state;
	error = err;
	if (error)
		state=stateError;
	else
		state=statePlaying;
	if (state != oldstate)
		serviceEvent(eServiceEvent(eServiceEvent::evtStateChanged));

	serviceEvent(eServiceEvent(eServiceEvent::evtStart));
}

void eServiceHandlerDVB::gotEIT(EIT *, int)
{
	serviceEvent(eServiceEvent(eServiceEvent::evtGotEIT));
}

void eServiceHandlerDVB::gotSDT(SDT *)
{
	serviceEvent(eServiceEvent(eServiceEvent::evtGotSDT));
}

void eServiceHandlerDVB::gotPMT(PMT *)
{
	serviceEvent(eServiceEvent(eServiceEvent::evtGotPMT));
}

void eServiceHandlerDVB::leaveService(const eServiceReferenceDVB &)
{
	serviceEvent(eServiceEvent(eServiceEvent::evtStop));
}

void eServiceHandlerDVB::aspectRatioChanged(int isanamorph)
{
	aspect=isanamorph;
	serviceEvent(eServiceEvent(eServiceEvent::evtAspectChanged));
}

eServiceHandlerDVB::eServiceHandlerDVB(): eServiceHandler(eServiceReference::idDVB)
{
	if (eServiceInterface::getInstance()->registerHandler(id, this)<0)
		eFatal("couldn't register serviceHandler %d", id);

	CONNECT(eDVB::getInstance()->scrambled, eServiceHandlerDVB::scrambledStatusChanged);
	CONNECT(eDVB::getInstance()->switchedService, eServiceHandlerDVB::switchedService);
	CONNECT(eDVB::getInstance()->gotEIT, eServiceHandlerDVB::gotEIT);
	CONNECT(eDVB::getInstance()->gotSDT, eServiceHandlerDVB::gotSDT);
	CONNECT(eDVB::getInstance()->gotPMT, eServiceHandlerDVB::gotPMT);
	CONNECT(eDVB::getInstance()->leaveService, eServiceHandlerDVB::leaveService);
	CONNECT(eStreamWatchdog::getInstance()->AspectRatioChanged, eServiceHandlerDVB::aspectRatioChanged);

	// structure[eServiceReference(eServiceReference::idDVB, -1, eServiceID())]=new eService("DVB - bouquets");
	// structure[eServiceReference(eServiceReference::idDVB, -2, eServiceID())]=new eService("DVB - ALL services");
	
	// structure[eServiceReference(eServiceReference::idDVB, -3, eServiceID(bouquet_id))]=new eService("");
}

eServiceHandlerDVB::~eServiceHandlerDVB()
{
	if (eServiceInterface::getInstance()->unregisterHandler(id)<0)
		eFatal("couldn't unregister serviceHandler %d", id);
}

eService *eServiceHandlerDVB::lookupService(const eServiceReference &service)
{
	eTransponderList *tl=eTransponderList::getInstance();
	if (!tl)
		return 0;
	return tl->searchService(service);
}

int eServiceHandlerDVB::play(const eServiceReference &service)
{
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (service.type != eServiceReference::idDVB)
		return -1;
	if (sapi)
		return sapi->switchService((const eServiceReferenceDVB&)service);
	return -1;
}

PMT *eServiceHandlerDVB::getPMT()
{
	return eDVB::getInstance()->getPMT();
}

void eServiceHandlerDVB::setPID(const PMTEntry *e)
{
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (sapi)
	{
		sapi->setPID(e);
		sapi->setDecoder();
	}
}

SDT *eServiceHandlerDVB::getSDT()
{
	return eDVB::getInstance()->getSDT();
}

EIT *eServiceHandlerDVB::getEIT()
{
	return eDVB::getInstance()->getEIT();
}

int eServiceHandlerDVB::getFlags()
{
	return flags;
}

int eServiceHandlerDVB::getAspectRatio()
{
	return aspect;
}

int eServiceHandlerDVB::getState()
{
	return state;
}

int eServiceHandlerDVB::getErrorInfo()
{
	return error;
}

int eServiceHandlerDVB::stop()
{
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();

	if (sapi)
		sapi->switchService(eServiceReferenceDVB());

	return 0;
}

struct eServiceHandlerDVB_addService
{
	Signal0<void,const eServiceReference&> &callback;
	eServiceHandlerDVB_addService(Signal0<void,const eServiceReference&> &callback): callback(callback)
	{
	}
	void operator()(const eServiceReference &service)
	{
//		callback(service);
	}
};

void eServiceHandlerDVB::enterDirectory(const eServiceReference &dir, Signal0<void,const eServiceReference&> &callback)
{
	switch (dir.type)	
	{
	case eServiceReference::idStructure:
//		callback(eServiceReference(eServiceReference::idDVB, -1, eServiceID(0)));
		break;
	case eServiceReference::idDVB:
		if (dir.data[0] == -1)
		{

		}
		break;
	}
}

void eServiceHandlerDVB::leaveDirectory(const eServiceReference &dir)
{
	// noch nix.
}

eAutoInitP0<eServiceHandlerDVB> i_eServiceHandlerDVB(6, "eServiceHandlerDVB");
