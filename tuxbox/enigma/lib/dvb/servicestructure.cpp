#include <core/dvb/servicestructure.h>
#include <core/system/init.h>
#include <core/base/i18n.h>

eServiceStructureHandler::eServiceStructureHandler(): eServiceHandler(eServiceReference::idStructure), cache(*this)
{
	if (eServiceInterface::getInstance()->registerHandler(id, this)<0)
		eFatal("couldn't register serviceHandler %d", id);
	cache.addPersistentService(eServiceReference(eServiceReference::idStructure, eServiceReference::flagDirectory|eServiceReference::shouldSort, modeRoot), new eService(0, _("root node")));
	cache.addPersistentService(eServiceReference(eServiceReference::idStructure, eServiceReference::flagDirectory|eServiceReference::shouldSort, modeTV), new eService(0, _("TV mode")));
	cache.addPersistentService(eServiceReference(eServiceReference::idStructure, eServiceReference::flagDirectory|eServiceReference::shouldSort, modeRadio), new eService(0, _("Radio Mode")));
	cache.addPersistentService(eServiceReference(eServiceReference::idStructure, eServiceReference::flagDirectory|eServiceReference::shouldSort, modeFile), new eService(0, _("File Mode")));
	cache.addPersistentService(eServiceReference(eServiceReference::idStructure, eServiceReference::flagDirectory|eServiceReference::shouldSort, modeFavourite), new eService(0, _("Favourites")));
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

eService* eServiceStructureHandler::addRef(const eServiceReference &c)
{
	return cache.addRef(c);
}

void eServiceStructureHandler::removeRef(const eServiceReference &c)
{
	cache.removeRef(c);
}

eAutoInitP0<eServiceStructureHandler> i_eServiceStructureHandler(6, "eServiceStructureHandler");
