#include "actions.h"
#include "init.h"
#include "nconfig.h"
#include "edvb.h"

eAction::eAction(eActionMap &map, char *identifier, char *description)
		: map(&map),  description(description), identifier(identifier)
{
	map.add(this);
}

eAction::~eAction()
{
	map->remove(this);
}

// ---------------------------------------------------------------------------

const eAction *eActionMap::findAction(const eRCKey &key) const
{
	for (std::list<const eAction*>::const_iterator i=actions.begin(); i!=actions.end(); ++i)
		if ((*i)->containsKey(key))
			return *i;
}

eActionMap::eActionMap(const char *identifier, char *description): 
		identifier(identifier), description(description)
{
	eActionMapList::getInstance()->addActionMap(identifier, this);
} 

eActionMap::~eActionMap()
{
	eActionMapList::getInstance()->removeActionMap(identifier);
}

void eActionMap::reloadConfig()
{
#if 0
	NConfig &nc=eDVB::getInstance()->config;
	std::string path="/ezap/rc/keymaps/";
	path+=identifier;
	path+="/";
	
	for (actionList::iterator i(actions.begin()); i!=actions.end(); ++i)
	{
		std::string key=path+i->getIdentifier();
		qDebug("loading %s", key.c_str());
		char *value;
		int len;
		int err;
		if (!(err=nc.getKey(value, len)))
		{
			qDebug("have data!");
		} else
			qDebug("error %d", err);
	}
#endif
}

void eActionMap::saveConfig()
{
#if 0
	NConfig &nc=eDVB::getInstance()->config;
	std::string path="/ezap/rc/keymaps/";
	path+=identifier;
	path+="/";
	
	for (actionList::iterator i(actions.begin()); i!=actions.end(); ++i)
	{
		std::string key=path+i->second.getIdentifier();
		qDebug("saving %s", key.c_str());
		std::string value;
		
		eAction::keyList &kl=i->second.getKeyList();
		
		for (eAction::keylist::iterator kli(kl.begin()); kli!=kl.end(); ++kli)
		{
			eRCKey &key=kli->second;
			qDebug("%s, %x, %x", key.producer->getIdentifier(), key.code, key.flags);
			value+=key.producer->getIdentifier();
			value+=0;

			value+=(key.code>>24)&0xFF;
			value+=(key.code>>16)&0xFF;
			value+=(key.code>>8)&0xFF;
			value+=key.code&0xFF;

			value+=(key.flags>>24)&0xFF;
			value+=(key.flags>>16)&0xFF;
			value+=(key.flags>>8)&0xFF;
			value+=key.flags&0xFF;
		}
		nc.setKey(key.c_str(), value.data(), value.size());
	}
#endif
}

// ---------------------------------------------------------------------------

eActionMapList *eActionMapList::instance;

eActionMapList::eActionMapList()
{
	if (!instance)
		instance=this;
}

eActionMapList::~eActionMapList()
{
	if (instance==this)
		instance=0;
}

void eActionMapList::addActionMap(const char *id, eActionMap *am)
{
	actionmaps.insert(std::pair<const char*,eActionMap*>(id,am));
}

void eActionMapList::removeActionMap(const char *id)
{
	actionmaps.erase(id);
}
    
eActionMap *eActionMapList::findActionMap(const char *id) const
{
	std::map<const char *,eActionMap*>::const_iterator i;
	i=(actionmaps.find(id));
	if (i==actionmaps.end())
		return 0;
	return i->second;
}

eAutoInitP0<eActionMapList> init_eActionMapList(1, "eActionMapList");
