#include <lib/gui/actions.h>

#include <xmltree.h>
#include <lib/base/eerror.h>
#include <lib/dvb/edvb.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/system/nconfig.h>

eAction::eAction(eActionMap &map, char *identifier, char *description, int priority)
	: description(description), identifier(identifier), map(&map), priority(priority)
{
	map.add(this);
}

eAction::~eAction()
{
	map->remove(this);
}

/*eAction::keylist &eAction::getKeyList()
{
	std::map<eString, keylist>::iterator it=keys.find( eActionMapList::getInstance()->getCurrentStyle() );
	if ( it != keys.end() )
		return it->second;
	it = keys.find("default);
	return it->second;
}*/

int eAction::containsKey(const eRCKey &key, const eString &style ) const
{
	std::map<eString, keylist>::const_iterator it=keys.find( style );
	if ( it != keys.end() )
	{
		if (it->second.find(key)!=it->second.end())
			return 1;
	}
	return 0;
}

// ---------------------------------------------------------------------------

void eActionMap::findAction(eActionPrioritySet &list, const eRCKey &key, void *context, const eString &style) const
{
	for (std::list<eAction*>::const_iterator i=actions.begin(); i!=actions.end(); ++i)
	{
		if ((*i)->containsKey(key, style))
			list.insert(eAction::directedAction(context, *i));
	}
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

void eActionMap::loadXML(eRCDevice *device, std::map<std::string,int> &keymap, const XMLTreeNode *node, const eString& style)
{
	for (XMLTreeNode *xaction=node->GetChild(); xaction; xaction=xaction->GetNext())
	{
		if (strcmp(xaction->GetType(), "action"))
		{
			eFatal("illegal type %s, expected 'action'", xaction->GetType());
			continue;
		}
		const char *name=xaction->GetAttributeValue("name");
		eAction *action=0;
		if (name)
			action=findAction(name);
		if (!action)
		{
			eWarning("please specify a valid action with name=. valid actions are:");
			for (actionList::iterator i(actions.begin()); i != actions.end(); ++i)
				eWarning("  %s (%s)", (*i)->getIdentifier(), (*i)->getDescription());
			eFatal("but NOT %s", name);
			continue;
		}
		const char *code=xaction->GetAttributeValue("code");
		int icode=-1;
		if (!code)
		{
			const char *key=xaction->GetAttributeValue("key");
			if (!key)
			{
				eFatal("please specify a number as code= or a defined key with key=.");
				continue;
			}
			std::map<std::string,int>::iterator i=keymap.find(std::string(key));
			if (i == keymap.end())
			{
				eFatal("undefined key %s specified!", key);
				continue;
			}
			icode=i->second;
		} else
			sscanf(code, "%x", &icode);
		const char *flags=xaction->GetAttributeValue("flags");
		if (!flags || !*flags)
			flags="b";
		if (strchr(flags, 'm'))
			action->insertKey( style, eRCKey(device, icode, 0) );
		if (strchr(flags, 'b'))
			action->insertKey( style, eRCKey(device, icode, eRCKey::flagBreak) );
		if (strchr(flags, 'r'))
			action->insertKey( style, eRCKey(device, icode, eRCKey::flagRepeat) );
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
	char * tmp;
	if ( eConfig::getInstance()->getKey("/ezap/rc/style", tmp ) )
		currentStyle="default";
	else
	{
		currentStyle=tmp;
		delete [] tmp;
	}
	eDebug("currentStyle=%s", currentStyle.c_str() );	
	xmlfiles.setAutoDelete(1);
}

eActionMapList::~eActionMapList()
{
	eConfig::getInstance()->setKey("/ezap/rc/style", currentStyle.c_str() ) ;

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
	{
//		eDebug("cannot open %s", filename);
		return -1;
	}

	XMLTreeParser *parser=new XMLTreeParser("ISO-8859-1");
	char buf[2048];
	
	int done;
	do
	{
		unsigned int len=fread(buf, 1, sizeof(buf), in);
		done=len<sizeof(buf);
		if (!parser->Parse(buf, len, done))
		{
			eFatal("parse error: %s at line %d",
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
		eFatal("not a rcdefaults file.");
		return -1;
	}
	

	xmlfiles.push_back(parser);
	
	return 0;
}

XMLTreeNode *eActionMapList::searchDevice(const eString &id)
{
	for (ePtrList<XMLTreeParser>::iterator parser(xmlfiles.begin()); parser != xmlfiles.end(); ++parser)
	{
		XMLTreeNode *node=parser->RootNode();
	
		for (node=node->GetChild(); node; node=node->GetNext())
			if (!strcmp(node->GetType(), "device"))
			{
				const char *identifier=node->GetAttributeValue("identifier");
				if (!identifier)
				{
					eFatal("please specify an remote control identifier!");
					continue;
				}
				if (id == identifier)
					return node;
			}
	}
	return 0;
}
	
int eActionMapList::loadDevice(eRCDevice *device)
{
	XMLTreeNode *node=searchDevice(device->getIdentifier());
	if (!node)
		node=searchDevice("generic");
	if (!node)
	{
		eFatal("couldn't load key bindings for device %s", device->getDescription());
		return -1;
	}

	std::map<std::string,int> keymap;
		
	for (XMLTreeNode *xam=node->GetChild(); xam; xam=xam->GetNext())
		if (!strcmp(xam->GetType(), "actionmap"))
		{
			eActionMap *am=0;
			const char *name=xam->GetAttributeValue("name");
			if (name)
				am=findActionMap(name);
			if (!am)
			{
				eWarning("please specify a valid actionmap name (with name=)");
				eWarning("valid actionmaps are:");
				for (actionMapList::iterator i(actionmaps.begin()); i != actionmaps.end(); ++i)
					eWarning("  %s", i->first);
				eWarning("end.");
				eFatal("invalid actionmap: \"%s\"", name?name:"");
				continue;
			}
			const char *style=xam->GetAttributeValue("style");
			if (style)
			{
				const char *descr=xam->GetAttributeValue("descr");
				std::map<eString,eString>::iterator it = existingStyles.find(style);
				if ( it == existingStyles.end() ) // not in map..
				{
					if (descr)
						existingStyles[style]=descr;
					else
						existingStyles[style]=style;
				}
				else if ( descr && existingStyles[style] == style )
					existingStyles[style]=descr;
				am->loadXML(device, keymap, xam, style );
			}
			else
				am->loadXML( device, keymap, xam );
		} else if (!strcmp(xam->GetType(), "keys"))
		{
			for (XMLTreeNode *k=xam->GetChild(); k; k=k->GetNext())
			{
				if (!strcmp(k->GetType(), "key"))
				{
					const char *name=k->GetAttributeValue("name");
					if (name)
					{
						const char *acode=k->GetAttributeValue("code");
						int code=-1;
						if (acode)
							sscanf(acode, "%x", &code);
						else
						{
							acode=k->GetAttributeValue("icode");
							sscanf(acode, "%d", &code);
						}
						
						if (code != -1)
							keymap.insert(std::pair<std::string,int>(name, code));
						else
							eFatal("no code specified for key %s!", name);
					} else
						eFatal("no name specified in keys!");
				}
			}
		}
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

eAutoInitP0<eActionMapList> init_eActionMapList(eAutoInitNumbers::lowlevel, "eActionMapList");
