#include <lib/system/init.h>
#include <lib/gui/guiactions.h>

eAutoInitP0<cursorActions> i_cursorActions(3, "cursor actions");
eAutoInitP0<focusActions> i_focusActions(3, "focus actions");
eAutoInitP0<listActions> i_listActions(3, "list actions");
eAutoInitP0<shortcutActions> i_shortcutActions(3, "shortcut actions");
