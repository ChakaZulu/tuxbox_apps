#ifndef __core_dvb_servicecache_h
#define __core_dvb_servicecache_h

#include <map>
#include <list>
#include <core/dvb/service.h>

class eService;

template <class Factory>
class eServiceCache
{
	struct eCachedService
	{
		int refcnt;
		eService *service;
	};
	struct eNode
	{
		int refcnt;
		int addRef()
		{
			return ++refcnt;
		}
		int removeRef()
		{
			return --refcnt;
		}
		std::list<eServiceReference> content;
		eNode()
		{
			refcnt=0;
		}
	};
	std::map<eServiceReference,eCachedService> services;
	std::map<eServiceReference,eNode> cache;
	Factory &factory;
public:
	eServiceCache(Factory &factory): factory(factory)
	{
	}
	void addToNode(eNode &node, const eServiceReference &ref)
	{
		node.content.push_back(ref);
	}
	void enterDirectory(const eServiceReference &parent, Signal1<void, const eServiceReference&> &callback)
	{
		eNode &node=cache[parent];
		if (node.addRef() == 1)
			factory.loadNode(node, parent);
		for (std::list<eServiceReference>::iterator i(node.content.begin()); i != node.content.end(); ++i)
		{
			if (services.find(*i) == services.end())
			{
				eCachedService c;
				c.refcnt=1;
				c.service=factory.createService(*i);
				if (!c.service)
				{
					eDebug("createService failed!");
					continue;
				}
				services.insert(std::pair<eServiceReference,eCachedService>(*i, c));
			} else if (services.find(*i)->second.refcnt != -1)
				services.find(*i)->second.refcnt++;
			callback(*i);
		}
	}
	eService *lookupService(const eServiceReference &service)
	{
		typename std::map<eServiceReference,eCachedService>::iterator i;
		if ((i=services.find(service)) != services.end())
			return i->second.service;
		else
			return 0;
	}
	void leaveDirectory(const eServiceReference &parent)
	{
		eNode &node=cache[parent];
		if (node.removeRef() <= 0)
		{
			for (std::list<eServiceReference>::iterator i(node.content.begin()); i != node.content.end(); ++i)
			{
				typename std::map<eServiceReference,eCachedService>::iterator c=services.find(*i);
				if (c == services.end())
					continue;
				if ((c->second.refcnt != -1) && ! --c->second.refcnt)
					services.erase(c);
			}
			cache.erase(parent);
		}
	}
	void addPersistentService(const eServiceReference &serviceref, eService *service)
	{
		eCachedService c;
		c.refcnt=-1;
		c.service=service;
		services.insert(std::pair<eServiceReference, eCachedService>(serviceref, c));
	}
};

#endif
