#ifndef __src__lib__gui__guiactions_h__
#define __src__lib__gui__guiactions_h__

#include <lib/system/init.h>
#include <lib/base/i18n.h>
#include <lib/gui/actions.h>

struct cursorActions
{
	eActionMap map;
	eAction up, down, left, right, ok, cancel;
	cursorActions(): 
		map("cursor", "Cursor"),
		up(map, "up", _("up"), eAction::prioWidget),
		down(map, "down", _("down"), eAction::prioWidget),
		left(map, "left", _("left"), eAction::prioWidget),
		right(map, "right", _("right"), eAction::prioWidget),
		ok(map, "ok", _("ok"), eAction::prioWidget),
		cancel(map, "cancel", _("cancel"), eAction::prioWidget)
	{
	}
};

extern eAutoInitP0<cursorActions> i_cursorActions;

struct focusActions
{
	eActionMap map;
	eAction up, down, left, right;
	focusActions(): 
		map("focus", "Focus"),
		up(map, "up", _("up"), eAction::prioGlobal),
		down(map, "down", _("down"), eAction::prioGlobal),
		left(map, "left", _("left"), eAction::prioGlobal),
		right(map, "right", _("right"), eAction::prioGlobal)
	{
	}
};

extern eAutoInitP0<focusActions> i_focusActions;

struct listActions
{
	eActionMap map;
	eAction pageup, pagedown;
	listActions():
		map("list", "Listen"),
		pageup(map, "pageup", _("page up"), eAction::prioWidget+1),
		pagedown(map, "pagedown", _("page down"), eAction::prioWidget+1)
	{
	}
};

extern eAutoInitP0<listActions> i_listActions;


#endif
