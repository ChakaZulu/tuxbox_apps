#include <lib/dvb/servicefile.h>
#include <lib/dvb/servicestructure.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/*
		eServiceFileHandler is some kind of a "multplexer", it gives
		you a filesystem structure (with ref.path = path), and calls
		"addFile" with every file. various file handlers can hook into
		this handler and parse files and return a eServiceReference.
		
		services itself are addRef'd first through the real handler (e.g.
		the mp3 handler), then reflected back into serviceFileHandler,
		then parsed by the cache (different file handlers to not need
		to have their own), calls to createService will be forwared to
		the module handlers.
*/

eServiceFileHandler *eServiceFileHandler::instance;

static const int dirflags=eServiceReference::isDirectory|eServiceReference::canDescent|eServiceReference::mustDescent|eServiceReference::shouldSort|eServiceReference::sort1;

eServiceFileHandler::eServiceFileHandler(): eServiceHandler(eServiceReference::idFile), cache(*this)
{
	if (eServiceInterface::getInstance()->registerHandler(id, this)<0)
		eFatal("couldn't register serviceHandler %d", id);
	instance=this;
	cache.addPersistentService(eServiceReference(eServiceReference::idFile, dirflags, "/"), new eService("root"));
	cache.addPersistentService(eServiceReference(eServiceReference::idFile, dirflags, "/hdd/"), new eService("harddisk"));
	
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
			case eServiceStructureHandler::modeRoot:
			case eServiceStructureHandler::modeFile:
			{
				cache.addToNode(node, eServiceReference(eServiceReference::idFile, dirflags, "/"));
				cache.addToNode(node, eServiceReference(eServiceReference::idFile, dirflags, "/hdd/"));
				// Add all links present in $CONFIGDIR/enigma/root/ to root-menu
				DIR *d=opendir(CONFIGDIR "/enigma/root/");
				if (d)
				{
					while (struct dirent64 *e=readdir64(d))
					{
						if (!(strcmp(e->d_name, ".") && strcmp(e->d_name, "..")))
							continue;
						eString filename;
						filename.sprintf("%s/enigma/root/%s", CONFIGDIR,e->d_name);
						struct stat64	 s;
						if (stat64(filename.c_str(), &s)<0)
							continue;
						if (S_ISDIR(s.st_mode))
						{
							filename+="/";
							eServiceReference service(eServiceReference::idFile, dirflags, filename);
							service.data[0]=!!S_ISDIR(s.st_mode);
							cache.addToNode(node, service);
						}	
					}
					closedir(d);
				}
				break;
			}
		}
		break;
	case eServiceReference::idFile:
	{
		DIR *d=opendir(ref.path.c_str());
		if (!d)
			return;
		while (struct dirent64 *e=readdir64(d))
		{
			if (!(strcmp(e->d_name, ".") && strcmp(e->d_name, "..")))
				continue;
			eString filename;
			
			filename=ref.path;
			filename+=e->d_name;
			
			struct stat64	 s;
			if (stat64(filename.c_str(), &s)<0)
				continue;
		
			if (S_ISDIR(s.st_mode))
				filename+="/";
				
			if (S_ISDIR(s.st_mode))
			{
				eServiceReference service(eServiceReference::idFile, dirflags, filename);
				service.data[0]=!!S_ISDIR(s.st_mode);
				cache.addToNode(node, service);
			} else
				fileHandlers((void*)&node, filename);
		}
		closedir(d);
		break;
	}
	default:
		break;
	}
}

void eServiceFileHandler::addReference(void *node, const eServiceReference &ref)
{
	if (!node)
		result=ref;	// super unthreadsafe und nichtmal reentrant.
	else
		cache.addToNode(*(eServiceCache<eServiceFileHandler>::eNode*)node, ref);
}

eService *eServiceFileHandler::createService(const eServiceReference &node)
{
	if (node.type == id)
	{
		int n=node.path.size()-2;
		if (n<0)
			n=0;
		eString path=node.path.mid(node.path.rfind("/", n)+1);
		if (!isUTF8(path))
			path=convertLatin1UTF8(path);
		return new eService(path.c_str());
	}
	eServiceHandler *handler=eServiceInterface::getInstance()->getServiceHandler(node.type);
	if (!handler)
		return 0;
	return handler->createService(node);
}

void eServiceFileHandler::enterDirectory(const eServiceReference &dir, Signal1<void,const eServiceReference&> &callback)
{
	cache.enterDirectory(dir, callback);
}

void eServiceFileHandler::leaveDirectory(const eServiceReference &dir)
{
	cache.leaveDirectory(dir);
}

eService *eServiceFileHandler::addRef(const eServiceReference &service)
{
	return cache.addRef(service);
}

void eServiceFileHandler::removeRef(const eServiceReference &service)
{
	return cache.removeRef(service);
}

int eServiceFileHandler::lookupService(eServiceReference &ref, const char *filename)
{
	result=eServiceReference();
	fileHandlers(0, filename);
	if (result)
	{
		ref=result;
		return 1;
	}
	return 0;
}

eAutoInitP0<eServiceFileHandler> i_eServiceFileHandler(eAutoInitNumbers::service+1, "eServiceFileHandler");
