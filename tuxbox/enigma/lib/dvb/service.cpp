#include "service.h"
#include <core/dvb/dvb.h>
#include <core/system/init.h>

eServiceHandler::eServiceHandler(int id): id(id)
{
}

eServiceHandler::~eServiceHandler()
{
}

eServiceInterface *eServiceInterface::instance;

eServiceInterface *eServiceInterface::getInstance()
{
	return instance;
}

void eServiceInterface::handleServiceEvent(const eServiceEvent &evt)
{
	serviceEvent(evt);
}

int eServiceInterface::switchServiceHandler(int id)
{
	if (currentServiceHandler && (currentServiceHandler->getID() == id))
		return 0;
	stop();
	eServiceHandler *handler=getServiceHandler(id);
	if (!handler)
		return -1;
	
	conn=CONNECT(handler->serviceEvent, eServiceInterface::handleServiceEvent);
	currentServiceHandler=handler;
	return 0;
}

eServiceInterface::eServiceInterface()
{
	currentServiceHandler = 0;
	if (!instance)
		instance = this;
}

eServiceInterface::~eServiceInterface()
{
	if (instance == this)
		instance = 0;
	stop();
}

int eServiceInterface::registerHandler(int id, eServiceHandler *handler)
{
	if (handlers.count(id))
		return -1;
	handlers.insert(std::pair<int,eServiceHandler*>(id, handler));
	return 0;
}

int eServiceInterface::unregisterHandler(int id)
{
	std::map<int,eServiceHandler*>::iterator i=handlers.find(id);
	if (i == handlers.end())
		return -1;
	if (i->second == currentServiceHandler)
		stop();
	handlers.erase(i);
	return 0;
}

eServiceHandler *eServiceInterface::getServiceHandler(int id)
{
	std::map<int,eServiceHandler*>::iterator i=handlers.find(id);
	if (i == handlers.end())
		return 0;
	return i->second;
}

eService *eServiceInterface::lookupService(const eServiceReference &service)
{
	eServiceHandler *handler=getServiceHandler(service.type);
	return handler->lookupService(service);
}

int eServiceInterface::play(const eServiceReference &s)
{
	if (switchServiceHandler(s.type))
	{
		eWarning("couldn't play service type %d\n", s.type);
		return -1;
	}
	service=s;
	return currentServiceHandler->play(s);
}

int eServiceInterface::stop()
{
	if (!currentServiceHandler)
		return -1;
	int res=currentServiceHandler->stop();
	conn.disconnect();
	currentServiceHandler=0;
	return res;
}

eAutoInitP0<eServiceInterface> i_eServiceInteface(5, "eServiceInterface");
