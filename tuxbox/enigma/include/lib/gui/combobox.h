#ifndef __E_COMBOBOX__
#define __E_COMBOBOX__

#include <core/gui/listbox.h>
#include <core/gui/ebutton.h>

class eComboBox: public eButton
{
	eListBox<eListBoxEntryText> listbox;
	eButton button;
	gPixmap *pm;
	int entries;
	eListBoxEntryText *current;
	void onOkPressed();
	void onEntrySelected( eListBoxEntryText* );	
	void onSelChanged( eListBoxEntryText* ); // für setzen des HelpTextes ( Statusbar )
	int eventHandler( const eWidgetEvent& );
	eString oldHelpText;
	int setProperty( const eString&, const eString& );
public:
	enum	{		OK = 0,		ERROR=1,		E_ALLREADY_SELECTED = 2,		E_COULDNT_FIND = 4,		E_INVALID_ENTRY = 8	};
	Signal1< void, eListBoxEntryText* > selchanged;	
	Signal2< void, eComboBox*, eListBoxEntryText* > selchanged_id;
	enum { /*flagVCenter=64 in eLabel*/ flagSorted=128, flagShowEntryHelp=256 };
	eComboBox(eWidget* parent, int OpenEntries, const char *deco="eComboBox" );
	void removeEntry( eListBoxEntryText* );
	void removeEntry( int );
	void removeEntry( void* );
	int setCurrent( eListBoxEntryText* );
	int setCurrent( int );
	int setCurrent( void* );
	int getCount() { return listbox.getCount(); }
	void clear() { listbox.clearList(); }
	eListBoxEntryText* getCurrent();
	operator eListBox<eListBoxEntryText>*()	{	return &listbox; }
};


#endif // __E_COMBOBOX__
