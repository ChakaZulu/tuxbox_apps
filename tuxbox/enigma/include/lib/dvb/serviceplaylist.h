#ifndef __core_dvb_serviceplaylist_h
#define __core_dvb_serviceplaylist_h

#include <core/dvb/service.h>
#include <list>

class ePlaylist: public eService
{
public:
	std::list<eServiceReference> list;
	int load(const char *filename);
	ePlaylist();
};

class eServicePlaylistHandler: public eServiceHandler
{
	static eServicePlaylistHandler *instance; 
	void addFile(void *node, const eString &filename);

public:
	static eServicePlaylistHandler *getInstance() { return instance; }

	eService *createService(const eServiceReference &node);
	
	eServicePlaylistHandler();
	~eServicePlaylistHandler();

		// service list functions
	void enterDirectory(const eServiceReference &dir, Signal1<void,const eServiceReference&> &callback);
	void leaveDirectory(const eServiceReference &dir);

	eService *addRef(const eServiceReference &service);
	void removeRef(const eServiceReference &service);
	
	
		// playlist functions
	eServiceReference newPlaylist();
};

#endif
