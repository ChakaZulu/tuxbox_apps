#include "actions.h"

eAction::eAction(eActionMap &map, char *identifier, char *description)
		: map(&map),  description(description), identifier(identifier)
{
	map.add(this);
}

eAction::~eAction()
{
	map->remove(this);
}

const eAction *eActionMap::findAction(const eRCKey &key) const
{
	for (std::list<const eAction*>::const_iterator i=actions.begin(); i!=actions.end(); ++i)
		if ((*i)->containsKey(key))
			return *i;
}

eActionMapList *eActionMapList::instance;

eActionMap *eActionMapList::findActionMap(const char *id) const
{
	for (const_iterator i(begin()); i!=end(); ++i)
		if (!strcmp((*i)->getIdentifier(), id))
			return *i;
	return 0;
}
