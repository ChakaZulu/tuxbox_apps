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

eServiceHandlerDVB::eServiceHandlerDVB(): eServiceHandler(eServiceReference::idDVB), cache(*this)
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

	cache.addPersistentService(
			eServiceReference(eServiceReference::idDVB, eServiceReference::isDirectory, -1),
			new eService(eServiceReference::idDVB, "DVB - bouquets")
		);
	cache.addPersistentService(
			eServiceReference(eServiceReference::idDVB, eServiceReference::isDirectory, -2), 
			new eService(eServiceReference::idDVB, "DVB - all services")
		);
}

eServiceHandlerDVB::~eServiceHandlerDVB()
{
	if (eServiceInterface::getInstance()->unregisterHandler(id)<0)
		eFatal("couldn't unregister serviceHandler %d", id);
}

eService *eServiceHandlerDVB::lookupService(const eServiceReference &service)
{
	eService *s=cache.lookupService(service);
	if (s)
		return s;
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
	Signal1<void,const eServiceReference&> &callback;
	eServiceHandlerDVB_addService(Signal1<void,const eServiceReference&> &callback): callback(callback)
	{
	}
	void operator()(const eServiceReference &service)
	{
		callback(service);
	}
};

void eServiceHandlerDVB::enterDirectory(const eServiceReference &ref, Signal1<void,const eServiceReference&> &callback)
{
	eDebug("enter directory...");
	switch (ref.type)
	{
	case eServiceReference::idDVB:
		switch (ref.data[0])
		{
		case -2:
			eDVB::getInstance()->settings->getTransponders()->forEachServiceReference(eServiceHandlerDVB_addService(callback));
			break;
		case -3:
		{
			eBouquet *b=eDVB::getInstance()->settings->getBouquet(ref.data[1]);
			if (!b)
				break;
			for (std::list<eServiceReferenceDVB>::iterator i(b->list.begin());  i != b->list.end(); ++i)
				callback(*i);
			break;
		}
		default:
			break;
		}
	default:
		break;
	}
	cache.enterDirectory(ref, callback);
}

eService *eServiceHandlerDVB::createService(const eServiceReference &node)
{
	switch (node.data[0])
	{
	case -3:
	{
		eBouquet *b=eDVB::getInstance()->settings->getBouquet(node.data[1]);
		if (!b)
			return 0;
		return new eService(eServiceID(0), b->bouquet_name.c_str());
	}
	}
	return 0;
}

void eServiceHandlerDVB::loadNode(eServiceCache<eServiceHandlerDVB>::eNode &node, const eServiceReference &ref)
{
	eDebug("loadNode...");
	switch (ref.type)
	{
	case eServiceReference::idStructure:
		if (!ref.data[0])
		{
			eDebug("r00t");
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::isDirectory, -1, 0));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::isDirectory, -2, 0));
		}
		break;
	case eServiceReference::idDVB:
		switch (ref.data[0])
		{
		case -1:
		{
			ePtrList<eBouquet> &list=*eDVB::getInstance()->settings->getBouquets();
			for (ePtrList<eBouquet>::iterator i(list.begin()); i != list.end(); ++i)
				cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::isDirectory, -3, i->bouquet_id));
			break;
		}
		}
		break;
	}
}

void eServiceHandlerDVB::leaveDirectory(const eServiceReference &dir)
{
	cache.leaveDirectory(dir);
}

eAutoInitP0<eServiceHandlerDVB> i_eServiceHandlerDVB(6, "eServiceHandlerDVB");
