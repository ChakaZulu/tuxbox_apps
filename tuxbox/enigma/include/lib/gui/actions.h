#ifndef __actions_h
#define __actions_h

#include <string.h>
#include <string>
#include <list>
#include <functional>

#include "rc.h"

class eActionMap;

class eAction
{
	char *description, *identifier;
	std::list<const eRCKey*> keys;
	eActionMap *map;
public:
	eAction(eActionMap &map, char *identifier, char *description);
	~eAction();
	const char *getDescription() const { return description; }
	const char *getIdentifier() const { return identifier; }
	int containsKey(const eRCKey &key) const
	{
		for (std::list<const eRCKey*>::const_iterator i(keys.begin()); i!=keys.end(); ++i)
			if ((**i) > key)
				return 1;
		return 0;
	}
};

class eActionMap
{
	std::list<const eAction*> actions;
	const char *identifier, *description;
public:
	eActionMap(const char *identifier, char *description)
			: identifier(identifier), description(description)
	{
	}
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
};

class eActionMapList: public std::list<eActionMap*>
{
	static eActionMapList *instance;
public:
	eActionMapList();
	~eActionMapList();
	eActionMap *findActionMap(const char *id) const;
	static eActionMapList *getInstance() { return instance; }
};

#endif
