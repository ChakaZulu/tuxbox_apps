#ifndef __core_dvb_servicestructure_h
#define __core_dvb_servicestructure_h

#include <core/dvb/service.h>
#include <core/dvb/servicecache.h>

class eServiceStructureHandler: public eServiceHandler
{
	eServiceCache<eServiceStructureHandler> cache;
public:
	void loadNode(eServiceCache<eServiceStructureHandler>::eNode &node, const eServiceReference &ref);
	eService *createService(const eServiceReference &node);
	eService* lookupService(const eServiceReference&);
	
	eServiceStructureHandler();
	~eServiceStructureHandler();

		// service list functions
	void enterDirectory(const eServiceReference &dir, Signal1<void,const eServiceReference&> &callback);
	void leaveDirectory(const eServiceReference &dir);
};

#endif

