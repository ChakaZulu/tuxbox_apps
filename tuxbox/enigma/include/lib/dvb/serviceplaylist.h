#ifndef __core_dvb_serviceplaylist_h
#define __core_dvb_serviceplaylist_h

#include <core/dvb/service.h>
#include <list>

struct ePlaylistEntry
{
	enum
	{
		PlaylistEntry=1, SwitchTimerEntry=2, RecTimerEntry=4,	
		stateWaiting=32, stateRunning=64,	statePaused=128, stateFinished=256, stateError=512,
		errorNoSpaceLeft=1024, errorUserAborted=2048, errorZapFailed=4096, errorOutdated=8192,
		boundFile=16384, typeSmartTimer=32768
	};

	eServiceReference service;
	union
	{
		int current_position;
		int event_id;
	};
	time_t time_begin;
	int duration,
			type;  // event type and current state of timer events...
	ePlaylistEntry(const eServiceReference &ref)
		:service(ref), current_position(-1), time_begin(-1), duration(-1), type(PlaylistEntry)
	{ }
	ePlaylistEntry(const eServiceReference &ref, int current_position)
		:service(ref), current_position(current_position), time_begin(-1), duration(-1), type(PlaylistEntry)
	{ }
	ePlaylistEntry(const eServiceReference &ref, int time_begin, int duration, int event_id=-1, int type=SwitchTimerEntry )
		:service(ref), event_id(event_id), time_begin(time_begin), duration(duration), type(type)
	{ }
	operator eServiceReference &() { return service; }
	operator const eServiceReference &() const { return service; }
	bool operator == (const eServiceReference &r) const
	{
			return r == service;
	}
	bool operator == (const ePlaylistEntry &e) const
	{
		if ( type == PlaylistEntry )
			return e.service == service;
		else if ( e.event_id != -1 && event_id != -1 )
			return e.service == service && e.event_id == event_id;
		else
			return e.service == service && e.time_begin == time_begin;
	}
};

class ePlaylist: public eService
{
public:
	std::list<ePlaylistEntry> list;
	std::list<ePlaylistEntry>::iterator current;

	int load(const char *filename);
	int save(const char *filename);
	
	int deleteService(std::list<ePlaylistEntry>::iterator it);
	int moveService(std::list<ePlaylistEntry>::iterator it, std::list<ePlaylistEntry>::iterator before);

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

	eService *addRef(const eServiceReference &service);
	void removeRef(const eServiceReference &service);

		// playlist functions
	eServiceReference newPlaylist(const eServiceReference &parent=eServiceReference(), const eServiceReference &serviceref=eServiceReference());
	void removePlaylist(const eServiceReference &service);
};

#endif
