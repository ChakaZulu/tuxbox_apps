#include <lib/dvb/service.h>
#include <lib/dvb/dvb.h>
#include <lib/system/init.h>

eServiceHandler::eServiceHandler(int id): id(id)
{
}

eServiceHandler::~eServiceHandler()
{
}

eService *eServiceHandler::createService(const eServiceReference &node)
{
	return 0;
}

int eServiceHandler::play(const eServiceReference &service)
{
	return -1;
}

int eServiceHandler::serviceCommand(const eServiceCommand &cmd)
{
	return -1;
}

PMT *eServiceHandler::getPMT()
{
	return 0;
}

void eServiceHandler::setPID(const PMTEntry *)
{
	return;
}

SDT *eServiceHandler::getSDT()
{
	return 0;
}

EIT *eServiceHandler::getEIT()
{
	return 0;
}

int eServiceHandler::getFlags()
{
	return 0;
}

int eServiceHandler::getState()
{
	return 0;
}

int eServiceHandler::getAspectRatio()
{
	return 0;
}

int eServiceHandler::getErrorInfo()
{
	return 0;
}

int eServiceHandler::stop()
{
	return 0;
}

int eServiceHandler::getPosition(int)
{
	return -1;
}

void eServiceHandler::enterDirectory(const eServiceReference &dir, Signal1<void,const eServiceReference&> &callback)
{
	return;
}

void eServiceHandler::leaveDirectory(const eServiceReference &dir)
{
	return;
}

eService *eServiceHandler::addRef(const eServiceReference &service)
{
	return 0;
}

void eServiceHandler::removeRef(const eServiceReference &service)
{
}

eString eServiceHandler::getInfo(int id)
{
	return "";
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
	{
		currentServiceHandler->stop();
		return 0;
	}
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

int eServiceInterface::play(const eServiceReference &s)
{
	if (switchServiceHandler(s.type))
	{
		eWarning("couldn't play service type %d\n", s.type);
		return -1;
	}
	service=s;
	addRef(s);
	return currentServiceHandler->play(s);
}

int eServiceInterface::stop()
{
	if (!currentServiceHandler)
		return -1;
	removeRef(service);
	int res=currentServiceHandler->stop();
	conn.disconnect();
	currentServiceHandler=0;
	service=eServiceReference();
	return res;
}

void eServiceInterface::enterDirectory(const eServiceReference &dir, Signal1<void,const eServiceReference&> &callback)
{
	for (std::map<int,eServiceHandler*>::iterator i(handlers.begin()); i != handlers.end(); ++i)
		i->second->enterDirectory(dir, callback);
}

void eServiceInterface::leaveDirectory(const eServiceReference &dir)
{
	for (std::map<int,eServiceHandler*>::iterator i(handlers.begin()); i != handlers.end(); ++i)
		i->second->leaveDirectory(dir);
}

eService *eServiceInterface::addRef(const eServiceReference &service)
{
	eServiceHandler *handler=getServiceHandler(service.type);
	if (handler)
		return handler->addRef(service);
	else
		return 0;
}

void eServiceInterface::removeRef(const eServiceReference &service)
{
	eServiceHandler *handler=getServiceHandler(service.type);
	if (handler)
		return handler->removeRef(service);
}

eAutoInitP0<eServiceInterface> i_eServiceInteface(5, "eServiceInterface");
