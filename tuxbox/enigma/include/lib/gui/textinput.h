#ifndef __LIB_GUI_TEXTINPUT_H__
#define __LIB_GUI_TEXTINPUT_H__

#include <lib/gui/ebutton.h>
#include <stack>

class eTextInputField: public eButton
{
	int curPos;
	unsigned int maxChars;
	int eventHandler( const eWidgetEvent &);
	void redrawWidget( gPainter *target, const eRect &area );
	void drawCursor();
	void updated();
	void nextChar();
	int lastKey;
	bool editMode;
	eString oldText;
	eString oldHelpText;
	eString editHelpText;
	eRect cursorRect, capsRect;
	eTimer nextCharTimer;
	eString useableChars;
	unsigned int nextCharTimeout;
	bool capslock;
	eString isotext;
	eLabel *editLabel;
	std::stack< std::pair<int,int> > scroll;
	void lostFocus();
public:
	eTextInputField( eWidget* parent, eLabel *descr=0, const char *deco="eNumber" );
	void setMaxChars( int i ) { maxChars = i; }
	void setUseableChars( const char* );
	void setNextCharTimeout( unsigned int );
	void setEditHelpText( eString str ) { editHelpText=str; }
	bool inEditMode() const { return editMode; }
};

#endif
