#ifndef __picmanager__
#define __picmanager__

#include <lib/gui/ewindow.h>
#include <lib/gui/statusbar.h>
#include <lib/gui/listbox.h>
#include <lib/gui/echeckbox.h>

class ePicViewerStyleSelector: public eListBoxWindow<eListBoxEntryText>
{
	int ssel;
public:
	ePicViewerStyleSelector(int ssel=0);
	int eventHandler( const eWidgetEvent &event );
	void entrySelected( eListBoxEntryText* e );
};

class ePicViewerSettings: public eWindow
{
private:

	eButton *ok, *abort;
	eStatusBar *statusbar;
	eCheckbox *sort, *wrap, *start, *subdirs, *busy;
	eListBox<eListBoxEntryText> *timeout;

	void fieldSelected(int *number);
	void okPressed();
	void abortPressed();
public:
	ePicViewerSettings();
	~ePicViewerSettings();
};
#endif
