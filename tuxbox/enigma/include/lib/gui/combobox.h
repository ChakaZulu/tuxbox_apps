#ifndef __E_COMBOBOX__
#define __E_COMBOBOX__

#include <lib/gui/listbox.h>
#include <lib/gui/ebutton.h>

class eComboBox: public eButton
{
	eListBox<eListBoxEntryText> listbox;
	eLabel button; // the small buttin with arrow png...
	gPixmap *pm;
	int entries;
	eListBoxEntryText *current;
	void onOkPressed();
	void onEntrySelected( eListBoxEntryText* );	
	void onSelChanged( eListBoxEntryText* ); // für setzen des HelpTextes ( Statusbar )
	int eventHandler( const eWidgetEvent& );
	eString oldHelpText;
	int setProperty( const eString&, const eString& );
	void redrawWidget(gPainter *target, const eRect &rc);
public:
	void setOpenWidth( int w ) { listbox.resize( eSize(w, listbox.getSize().height()) ); }
	enum	{		OK = 0,		ERROR=1,		E_ALLREADY_SELECTED = 2,		E_COULDNT_FIND = 4,		E_INVALID_ENTRY = 8	};
	Signal1< void, eListBoxEntryText* > selchanged;	
	Signal2< void, eComboBox*, eListBoxEntryText* > selchanged_id;
	enum { /*flagVCenter=64 in eLabel*/ flagSorted=128, flagShowEntryHelp=256 };
	eComboBox(eWidget* parent, int OpenEntries=5, eLabel* desc=0, const char *deco="eComboBox" );
	void removeEntry( eListBoxEntryText* );
	void removeEntry( int );
	void removeEntry( void* );
	void sort() { listbox.sort(); }
	int setCurrent( eListBoxEntryText* );
	int setCurrent( int );
	int setCurrent( void* );
	int getCount() { return listbox.getCount(); }
	int moveSelection ( int dir )	{	return listbox.moveSelection( dir ); }
	void clear() { listbox.clearList(); }
	eListBoxEntryText* getCurrent();
	operator eListBox<eListBoxEntryText>*()	{	return &listbox; }

	template <class Z>
	int forEachEntry(Z ob)
	{
		return listbox.forEachEntry(ob);
	}
};


#endif // __E_COMBOBOX__
