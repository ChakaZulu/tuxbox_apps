#ifndef __picviewer_h
#define __picviewer_h

#include <lib/gui/ewindow.h>
#include <lib/gui/listbox.h>
#include <lib/gui/statusbar.h>

class ePicViewerStyleSelector: public eListBoxWindow<eListBoxEntryText>
{
	int ssel;
public:
	ePicViewerStyleSelector(int ssel=0);
	int eventHandler(const eWidgetEvent &event);
	void entrySelected(eListBoxEntryText* e);
};
#endif
