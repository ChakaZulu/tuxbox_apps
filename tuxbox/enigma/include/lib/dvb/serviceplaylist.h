#ifndef __core_dvb_serviceplaylist_h
#define __core_dvb_serviceplaylist_h

#include <core/dvb/service.h>
#include <list>

struct ePlaylistEntry
{
	eServiceReference service;
	int current_position;
	time_t time_begin, time_end;
	
	ePlaylistEntry(const eServiceReference &ref): service(ref), current_position(-1), time_begin(-1), time_end(-1) { }
	ePlaylistEntry(const eServiceReference &ref, int current_position): service(ref), current_position(current_position), time_begin(-1), time_end(-1) { }
	ePlaylistEntry(const eServiceReference &ref, int time_begin, int time_end): service(ref), current_position(-1), time_begin(time_begin), time_end(time_end) { }
	operator eServiceReference &() { return service; }
	operator const eServiceReference &() const { return service; }
	bool operator == (const eServiceReference &r) const
	{
		return r == service;
	}
};

class ePlaylist: public eService
{
public:
	std::list<ePlaylistEntry> list;
	std::list<ePlaylistEntry>::iterator current;

	int load(const char *filename);
	int save(const char *filename);

	ePlaylist();
};

class eServicePlaylistHandler: public eServiceHandler
{
	static eServicePlaylistHandler *instance; 
	void addFile(void *node, const eString &filename);

	std::multimap<eServiceReference,eServiceReference> playlists;

public:
	enum { ID = 0x1001 } ;
	static eServicePlaylistHandler *getInstance() { return instance; }

	eService *createService(const eServiceReference &node);
	
	eServicePlaylistHandler();
	~eServicePlaylistHandler();

		// service list functions
	void enterDirectory(const eServiceReference &dir, Signal1<void,const eServiceReference&> &callback);
	void leaveDirectory(const eServiceReference &dir);

	int deleteService(const eServiceReference &dir, const eServiceReference &ref);
	int moveService(const eServiceReference &dir, const eServiceReference &ref, int dr);

	eService *addRef(const eServiceReference &service);
	void removeRef(const eServiceReference &service);

		// playlist functions
	eServiceReference newPlaylist(const eServiceReference &parent=eServiceReference(), const eServiceReference &serviceref=eServiceReference());
	void removePlaylist(const eServiceReference &service);
};

#endif
