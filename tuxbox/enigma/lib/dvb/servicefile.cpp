#include <core/dvb/servicefile.h>
#include <core/system/init.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

eServiceFileHandler *eServiceFileHandler::instance;

eServiceFileHandler::eServiceFileHandler(): eServiceHandler(eServiceReference::idFile), cache(*this)
{
	if (eServiceInterface::getInstance()->registerHandler(id, this)<0)
		eFatal("couldn't register serviceHandler %d", id);
	instance=this;
	cache.addPersistentService(eServiceReference(eServiceReference::idFile, eServiceReference::isDirectory, "/"), new eService(0, "root"));
	cache.addPersistentService(eServiceReference(eServiceReference::idFile, eServiceReference::isDirectory, "/mnt/"), new eService(0, "harddisk"));
}

eServiceFileHandler::~eServiceFileHandler()
{
	eServiceInterface::getInstance()->unregisterHandler(id);
}

void eServiceFileHandler::loadNode(eServiceCache<eServiceFileHandler>::eNode &node, const eServiceReference &ref)
{
	switch (ref.type)
	{
	case eServiceReference::idStructure:
		switch (ref.data[0])
		{
		case 0:	// root
			cache.addToNode(node, eServiceReference(eServiceReference::idFile, eServiceReference::isDirectory, "/"));
			cache.addToNode(node, eServiceReference(eServiceReference::idFile, eServiceReference::isDirectory, "/mnt/"));
			break;
		}
		break;
	case eServiceReference::idFile:
	{
		DIR *d=opendir(ref.path.c_str());
		if (!d)
			return;
		while (struct dirent *e=readdir(d))
		{
			if (!(strcmp(e->d_name, ".") && strcmp(e->d_name, "..")))
				continue;
			eString filename;
			
			filename=ref.path;
			filename+=e->d_name;
			
			struct stat s;
			if (stat(filename.c_str(), &s)<0)
				continue;
		
			if (S_ISDIR(s.st_mode))
				filename+="/";
				
			if (S_ISDIR(s.st_mode))
			{
				eServiceReference service(eServiceReference::idFile, eServiceReference::isDirectory, filename);
				service.data[0]=!!S_ISDIR(s.st_mode);
				cache.addToNode(node, service);
			} else
				fileHandlers((void*)&node, filename);
		}
		break;
	}
	default:
		break;
	}
}

void eServiceFileHandler::addReference(void *node, const eServiceReference &ref)
{
	cache.addToNode(*(eServiceCache<eServiceFileHandler>::eNode*)node, ref);
}

eService *eServiceFileHandler::createService(const eServiceReference &node)
{
	if (node.type == id)
		return new eService(0, eString(eString("[ ") + node.path + " ]").c_str());
	eServiceHandler *handler=eServiceInterface::getInstance()->getServiceHandler(node.type);
	if (!handler)
		return 0;
	return handler->createService(node);
}

eService *eServiceFileHandler::lookupService(const eServiceReference& service)
{
	return cache.lookupService(service);
}

void eServiceFileHandler::enterDirectory(const eServiceReference &dir, Signal1<void,const eServiceReference&> &callback)
{
	cache.enterDirectory(dir, callback);
}

void eServiceFileHandler::leaveDirectory(const eServiceReference &dir)
{
	cache.leaveDirectory(dir);
}

eAutoInitP0<eServiceFileHandler> i_eServiceFileHandler(6, "eServiceFileHandler");
