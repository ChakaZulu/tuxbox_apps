#ifndef __actions_h
#define __actions_h

#include <string.h>
#include <list>
#include <string>
#include <functional>
#include <set>

#include <include/libsig_comp.h>
#include <core/driver/rc.h>

class eActionMap;

/**
 * \brief An action
 *
 * An action is an action of the user. It might raise by a keypress or otherwise. bla.
 */
class eAction
{
	typedef std::set<eRCKey> keylist;
	char *description, *identifier;
	std::set<eRCKey> keys;
	eActionMap *map;
	int priority;
	friend struct priorityComperator;
public:
	typedef std::pair<void*,eAction*> directedAction;
	/**
	 * \brief Functor to compare by priority.
	 *
	 * This is useful in sorted priority lists.
	 */
	struct priorityComperator
	{
		bool operator()(const directedAction &a, const directedAction &b)
		{
			return a.second->priority > b.second->priority;
		}
	};

	enum
	{
		prioGlobal=0, prioDialog=10, prioWidget=20, prioDialogHi=30
	};
	/**
	 * \param priority The priority of this action. 0 should be used for
	 * global actions, 10 for dialog-local actions, 20 for widget-local actions,
	 * 30 for dialog-hi-priority actions.
	 */
	eAction(eActionMap &map, char *identifier, char *description, int priority=0);
	~eAction();
	const char *getDescription() const { return description; }
	const char *getIdentifier() const { return identifier; }
	int getPriority() const { return priority; }
	
	Signal0<void> handler;
	
	keylist &getKeyList() { return keys; }
	
	int containsKey(const eRCKey &key) const
	{
		if (keys.find(key)!=keys.end())
			return 1;
		return 0;
	}
};

typedef std::multiset<eAction::directedAction,eAction::priorityComperator> eActionPrioritySet;

class XMLTreeNode;

class eActionMap
{
	typedef std::list<eAction*> actionList;
	actionList actions;
	const char *identifier, *description;
public:
	eActionMap(const char *identifier, char *description);
	~eActionMap();
	void add(eAction *action)
	{
		actions.push_back(action);
	}
	void remove(eAction *action)
	{
		actions.remove(action);
	}
	void findAction(eActionPrioritySet &list, const eRCKey &key, void *context) const;
	eAction *findAction(const char *id) const;
	const char *getDescription() const { return description; }
	const char *getIdentifier() const { return identifier; }
	void reloadConfig();
	void loadXML(eRCDevice *device, std::map<std::string,int> &keymap, const XMLTreeNode *node);
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
	int loadXML(const char *filename);
	eActionMap *findActionMap(const char *id) const;
	actionMapList &getActionMapList() { return actionmaps; }

	static eActionMapList *getInstance() { return instance; }
};

#endif
