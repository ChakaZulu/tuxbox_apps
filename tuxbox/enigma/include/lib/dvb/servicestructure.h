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
	
	eServiceStructureHandler();
	~eServiceStructureHandler();

		// service list functions
	void enterDirectory(const eServiceReference &dir, Signal1<void,const eServiceReference&> &callback);
	void leaveDirectory(const eServiceReference &dir);
	eService* addRef(const eServiceReference&);
	void removeRef(const eServiceReference&);
	
	enum { modeRoot, modeTV, modeRadio, modeFile, modeFavourite };
	static eServiceReference getRoot(int mode)
	{
		return eServiceReference(eServiceReference::idStructure, eServiceReference::flagDirectory|eServiceReference::shouldSort, mode);
	}
};

#endif
