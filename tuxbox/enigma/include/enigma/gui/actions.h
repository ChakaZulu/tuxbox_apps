#ifndef __actions_h
#define __actions_h

#include <string.h>
#include <string>
#include <list>
#include <functional>
#include <set>

#include "rc.h"

class eActionMap;

class eAction
{
	typedef std::set<eRCKey> keylist;
	char *description, *identifier;
	std::set<eRCKey> keys;
	eActionMap *map;
public:
	eAction(eActionMap &map, char *identifier, char *description);
	~eAction();
	const char *getDescription() const { return description; }
	const char *getIdentifier() const { return identifier; }
	
	keylist &getKeyList() { return keys; }
	
	int containsKey(const eRCKey &key) const
	{
		if (keys.find(key)!=keys.end())
			return 1;
		return 0;
	}
};

class eActionMap
{
	typedef std::list<const eAction*> actionList;
	actionList actions;
	const char *identifier, *description;
public:
	eActionMap(const char *identifier, char *description);
	~eActionMap();
	void add(const eAction *action)
	{
		actions.push_back(action);
	}
	void remove(const eAction *action)
	{
		actions.remove(action);
	}
	const eAction *findAction(const eRCKey &key) const;
	const char *getDescription() const { return description; }
	const char *getIdentifier() const { return identifier; }
	void reloadConfig();
	void saveConfig();
};

class eActionMapList
{
public:

	struct lstr
	{
		bool operator()(const char *a, const char *b) const
		{
			return strcmp(a, b)<0;
		}
	};
	typedef std::map<const char*,eActionMap*,lstr> actionMapList;
	
	actionMapList actionmaps;

	static eActionMapList *instance;
public:
	eActionMapList();
	~eActionMapList();
	void addActionMap(const char *, eActionMap *);
	void removeActionMap(const char *);
	eActionMap *findActionMap(const char *id) const;
	actionMapList &getActionMapList() { return actionmaps; }

	static eActionMapList *getInstance() { return instance; }
};

#endif
