#include <core/system/init.h>
#include "guiactions.h"

eAutoInitP0<cursorActions> i_cursorActions(5, "cursor actions");
eAutoInitP0<focusActions> i_focusActions(5, "focus actions");
eAutoInitP0<listActions> i_listActions(5, "list actions");
