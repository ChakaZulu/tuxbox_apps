#ifndef __CORE_GUI_NUMBERACTIONS__
#define __CORE_GUI_NUMBERACTIONS__

#include <lib/gui/actions.h>
#include <lib/base/i18n.h>
#include <lib/system/init.h>

struct numberActions
{
	eActionMap map;
	eAction key0, key1, key2, key3, key4, key5, key6, key7, key8, key9, keyExt1, keyExt2, keyBackspace;
	numberActions():
		map("numbers", _("number actions")),
		key0(map, "0", _("key 0"), eAction::prioGlobal),
		key1(map, "1", _("key 1"), eAction::prioGlobal),
		key2(map, "2", _("key 2"), eAction::prioGlobal),
		key3(map, "3", _("key 3"), eAction::prioGlobal),
		key4(map, "4", _("key 4"), eAction::prioGlobal),
		key5(map, "5", _("key 5"), eAction::prioGlobal),
		key6(map, "6", _("key 6"), eAction::prioGlobal),
		key7(map, "7", _("key 7"), eAction::prioGlobal),
		key8(map, "8", _("key 8"), eAction::prioGlobal),
		key9(map, "9", _("key 9"), eAction::prioGlobal),
		keyExt1(map, "ext1", _("extended key1"), eAction::prioGlobal),
		keyExt2(map, "ext2", _("extended key2"), eAction::prioGlobal),
		keyBackspace(map, "backspace", _("backspace"), eAction::prioGlobal)
	{
	}
};
extern eAutoInitP0<numberActions> i_numberActions;
#endif

