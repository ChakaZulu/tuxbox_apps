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
		typename std::map<eServiceReference,eNode>::iterator i=cache.find(parent);
		eNode *node=0;
		if (i == cache.end())
		{
			node=&cache.insert(std::pair<eServiceReference,eNode>(parent, eNode())).first->second;
			factory.loadNode(*node, parent);
		} else
			node=&i->second;
		node->addRef();
		for (std::list<eServiceReference>::iterator i(node->content.begin()); i != node->content.end(); ++i)
		{
			callback(*i);
		}
	}

	void leaveDirectory(const eServiceReference &parent)
	{
		typename std::map<eServiceReference,eNode>::iterator i=cache.find(parent);
		if (i == cache.end())
			eDebug("leaveDirectory on non-cached directory!");
		else
		{
			eNode &node=i->second;
			if (node.removeRef() <= 0)
				cache.erase(i);
		}
	}

	void addPersistentService(const eServiceReference &serviceref, eService *service)
	{
		eCachedService c;
		c.refcnt=-1;
		c.service=service;
		services.insert(std::pair<eServiceReference, eCachedService>(serviceref, c));
	}
	
	int deleteService(const eServiceReference &dir, const eServiceReference &serviceref)
	{
		typename std::map<eServiceReference,eCachedService>::iterator c=services.find(serviceref);
		if (c == services.end())
			return 0;
		if (c->second.refcnt)
			return -1;
		services.erase(c);
		
		typename std::map<eServiceReference,eNode>::iterator i=cache.find(dir);
		if (i != cache.end())
			for (std::list<eServiceReference>::iterator a(i->second.content.begin()); a != i->second.content.end();)
				if (*a == serviceref)
					a = i->second.content.erase(a);
				else
					++a;
		return 0;
	}

	eService *addRef(const eServiceReference &serviceref)
	{
		if (services.find(serviceref) == services.end())  // service not exist in cache ?
		{
			eCachedService c;   // create new Cache Entry
			c.refcnt=1;					// currently one Object holds a reference to the new cache entry
			c.service=factory.createService(serviceref);
			if (!c.service)
			{
				eDebug("createService failed!");
				return 0;
			}
			services.insert(std::pair<eServiceReference,eCachedService>(serviceref, c));
			return c.service;
		} else
		{
			eCachedService &c=services.find(serviceref)->second;
			if (c.refcnt != -1)
				c.refcnt++;
			return c.service;
		}
	}
	
	void removeRef(const eServiceReference &serviceref)
	{
		typename std::map<eServiceReference,eCachedService>::iterator c=services.find(serviceref);
		if (c == services.end())
		{
			eDebug("removeRef on non-existing service!");
			return;
		}
		if ((c->second.refcnt != -1) && ! --c->second.refcnt)
		{
			delete c->second.service;
			services.erase(c);
		}
	}
};

#endif
