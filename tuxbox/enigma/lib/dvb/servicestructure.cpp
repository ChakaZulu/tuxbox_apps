#include <core/dvb/servicestructure.h>
#include <core/system/init.h>

eServiceStructureHandler::eServiceStructureHandler(): eServiceHandler(eServiceReference::idStructure), cache(*this)
{
	if (eServiceInterface::getInstance()->registerHandler(id, this)<0)
		eFatal("couldn't register serviceHandler %d", id);
	cache.addPersistentService(eServiceReference(eServiceReference::idStructure, eServiceReference::isDirectory, 0), new eService(0, "root node"));
}

eServiceStructureHandler::~eServiceStructureHandler()
{
	eServiceInterface::getInstance()->unregisterHandler(id);
}

void eServiceStructureHandler::enterDirectory(const eServiceReference &dir, Signal1<void,const eServiceReference&> &callback)
{
	cache.enterDirectory(dir, callback);
}

void eServiceStructureHandler::leaveDirectory(const eServiceReference &dir)
{
	cache.leaveDirectory(dir);
}

void eServiceStructureHandler::loadNode(eServiceCache<eServiceStructureHandler>::eNode &n, const eServiceReference &r)
{
}
 
eService *eServiceStructureHandler::createService(const eServiceReference &node)
{
	eFatal("structure should create: %d.%d", node.type, node.data[0]);
	return 0;
}

eService* eServiceStructureHandler::lookupService(const eServiceReference &c)
{
	return cache.lookupService(c);
}

eAutoInitP0<eServiceStructureHandler> i_eServiceStructureHandler(6, "eServiceStructureHandler");
