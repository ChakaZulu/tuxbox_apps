#ifndef __core_dvb_servicefile_h
#define __core_dvb_servicefile_h

#include <core/dvb/service.h>
#include <core/dvb/servicecache.h>
#include <list>

class eServiceFileHandler: public eServiceHandler
{
	eServiceCache<eServiceFileHandler> cache;
	static eServiceFileHandler *instance; 
public:

	Signal2<void,void*,const eString &> fileHandlers;
	void addReference(void *node, const eServiceReference &ref);
	
	static eServiceFileHandler *getInstance() { return instance; }
	void loadNode(eServiceCache<eServiceFileHandler>::eNode &node, const eServiceReference &ref);
	eService *createService(const eServiceReference &node);
	eService *lookupService(const eServiceReference&);
	
	eServiceFileHandler();
	~eServiceFileHandler();

		// service list functions
	void enterDirectory(const eServiceReference &dir, Signal1<void,const eServiceReference&> &callback);
	void leaveDirectory(const eServiceReference &dir);
};

#endif

