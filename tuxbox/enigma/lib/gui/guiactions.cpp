#include <lib/system/init.h>
#include <lib/gui/guiactions.h>

eAutoInitP0<cursorActions> i_cursorActions(5, "cursor actions");
eAutoInitP0<focusActions> i_focusActions(5, "focus actions");
eAutoInitP0<listActions> i_listActions(5, "list actions");
eAutoInitP0<shortcutActions> i_shortcutActions(5, "shortcut actions");
