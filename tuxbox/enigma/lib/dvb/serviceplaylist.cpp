#include <core/dvb/serviceplaylist.h>
#include <core/dvb/servicefile.h>
#include <core/system/init.h>
#include <core/base/i18n.h>

/*
		eServicePlaylistHandler hooks into the file handler eServiceFileHandler
		and implements an "addFile" for various playlist formats.
		
		it implements also enterDirectory and leaveDirectory for reading
		the playlists.
		
		playlists are stored in eServiceFileHandler's cache in an eService-based
		structure (ePlaylist)
*/

ePlaylist::ePlaylist(): eService(0, "playlist")
{
}

int ePlaylist::load(const char *filename)
{
	eDebug("loading playlist... %s", filename);
	FILE *fp=fopen(filename, "rt");
	eString path=filename;
	service_name=eString("playlist: ") + path.mid(path.rfind('/')+1);
	path=path.left(path.rfind('/')+1);
	
	int entries=0;
	if (!fp)	
	{
		eDebug("failed to open.");
		return -1;
	}
	while (1)
	{
		char line[256];
		if (!fgets(line, 256, fp))
			break;
		line[strlen(line)-1]=0;
		if (strlen(line) && line[strlen(line)-1]=='\r')
			line[strlen(line)-1]=0;
		if (line[0]=='#')
			continue;
		
		eString filename=path + eString(line);
		
		eServiceReference ref;
		if (eServiceFileHandler::getInstance()->lookupService(ref, filename.c_str()))
		{
			entries++;
			list.push_back(ref);
		} else
			eDebug("mit %s kann niemand was anfangen...", filename.c_str());
	}
	service_name += eString().sprintf(_(" (%d entries)"), entries);
	fclose(fp);
	return 0;
}

void eServicePlaylistHandler::addFile(void *node, const eString &filename)
{
	if (filename.right(4).upper()==".M3U")
		eServiceFileHandler::getInstance()->addReference(node, eServiceReference(id, eServiceReference::isDirectory, filename));
}

eService *eServicePlaylistHandler::createService(const eServiceReference &node)
{
	ePlaylist *list=new ePlaylist();
	if (!node.path.empty())
	{
		if (!list->load(node.path.c_str()))
			return list;
		delete list;
		return 0;
	} else
		return list;
}

eServicePlaylistHandler::eServicePlaylistHandler(): eServiceHandler(0x1001)
{
	if (eServiceInterface::getInstance()->registerHandler(id, this)<0)
		eFatal("couldn't register serviceHandler %d", id);
	CONNECT(eServiceFileHandler::getInstance()->fileHandlers, eServicePlaylistHandler::addFile);
}

eServicePlaylistHandler::~eServicePlaylistHandler()
{
}

eService *eServicePlaylistHandler::addRef(const eServiceReference &service)
{
	return eServiceFileHandler::getInstance()->addRef(service);
}

void eServicePlaylistHandler::removeRef(const eServiceReference &service)
{
	return eServiceFileHandler::getInstance()->removeRef(service);
}

void eServicePlaylistHandler::enterDirectory(const eServiceReference &dir, Signal1<void,const eServiceReference&> &callback)
{
	if (dir.type == id)
	{
		ePlaylist *service=(ePlaylist*)addRef(dir);
		if (!service)
			return;
	
		for (std::list<eServiceReference>::iterator i(service->list.begin()); i != service->list.end(); ++i)
			callback(*i);
	
		removeRef(dir);
	}
}

void eServicePlaylistHandler::leaveDirectory(const eServiceReference &dir)
{
}

eAutoInitP0<eServicePlaylistHandler> i_eServicePlaylistHandler(7, "eServicePlaylistHandler");
