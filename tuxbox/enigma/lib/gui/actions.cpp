#include "actions.h"
#include <core/base/eerror.h>
#include "init.h"
#include "nconfig.h"
#include "edvb.h"
#include <core/xml/xmltree.h>

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
	for (std::list<eAction*>::const_iterator i=actions.begin(); i!=actions.end(); ++i)
		if ((*i)->containsKey(key))
			return *i;
	return 0;
}

eAction *eActionMap::findAction(const char *id) const
{
	for (std::list<eAction*>::const_iterator i=actions.begin(); i!=actions.end(); ++i)
		if (!strcmp((*i)->getIdentifier(), id))
			return *i;
	return 0;
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

void eActionMap::loadXML(eRCDevice *device, const XMLTreeNode *node)
{
	for (XMLTreeNode *xaction=node->GetChild(); xaction; xaction=xaction->GetNext())
	{
		if (strcmp(xaction->GetType(), "action"))
		{
			eDebug("illegal type %s, expected 'action'", xaction->GetType());
			continue;
		}
		const char *name=xaction->GetAttributeValue("name");
		eAction *action=0;
		if (name)
			action=findAction(name);
		if (!action)
		{
			eDebug("please specify a valid action with name=. valid actions are:");
			for (actionList::iterator i(actions.begin()); i != actions.end(); ++i)
				eDebug("  %s (%s)", (*i)->getIdentifier(), (*i)->getDescription());
			continue;
		}
		const char *code=xaction->GetAttributeValue("code");
		if (!code)
		{
			eDebug("please specify a number as code=.");
			continue;
		}
		int icode=-1;
		sscanf(code, "%x", &icode);
		const char *flags=xaction->GetAttributeValue("flags");
		if (!flags || !*flags)
			flags="b";
		if (strchr(flags, 'm'))
			action->getKeyList().insert(eRCKey(device, icode, 0));
		if (strchr(flags, 'b'))
			action->getKeyList().insert(eRCKey(device, icode, eRCKey::flagBreak));
		if (strchr(flags, 'r'))
			action->getKeyList().insert(eRCKey(device, icode, eRCKey::flagRepeat));
	}
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

int eActionMapList::loadXML(const char *filename)
{
	FILE *in=fopen(filename, "rt");
	if (!in)
		return -1;

	XMLTreeParser *parser=new XMLTreeParser("ISO-8859-1");
	char buf[2048];
	
	int done;
	do
	{
		unsigned int len=fread(buf, 1, sizeof(buf), in);
		done=len<sizeof(buf);
		if (!parser->Parse(buf, len, done))
		{
			eDebug("parse error: %s at line %d",
				parser->ErrorString(parser->GetErrorCode()),
				parser->GetCurrentLineNumber());
			delete parser;
			parser=0;
			fclose(in);
			return -1;
		}
	} while (!done);
	fclose(in);

	XMLTreeNode *root=parser->RootNode();
	if (!root)
		return -1;
	if (strcmp(root->GetType(), "rcdefaults"))
	{
		eDebug("not an rcdefaults file.");
		return -1;
	}
	
	XMLTreeNode *node=parser->RootNode();
	
	for (node=node->GetChild(); node; node=node->GetNext())
		if (!strcmp(node->GetType(), "device"))
		{
			const char *identifier=node->GetAttributeValue("identifier");
			
			eRCDevice *device=0;
			if (identifier)
				device=eRCInput::getInstance()->getDevice(identifier);
			if (!device)
			{
				eDebug("please specify an remote control identifier!");
				continue;
			}
			
			for (XMLTreeNode *xam=node->GetChild(); xam; xam=xam->GetNext())
				if (!strcmp(xam->GetType(), "actionmap"))
				{
					eActionMap *am=0;
					const char *name=xam->GetAttributeValue("name");
					if (name)
						am=findActionMap(name);
					if (!am)
					{
						eDebug("please specify a valid actionmap name (with name=)");
						eDebug("valid actionmaps are:");
						for (actionMapList::iterator i(actionmaps.begin()); i != actionmaps.end(); ++i)
							eDebug("  %s", i->first);
						continue;
					}
					am->loadXML(device, xam);
				}
		}

	delete parser;
	
	return 0;
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
