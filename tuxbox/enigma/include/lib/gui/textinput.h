#ifndef __LIB_GUI_TEXTINPUT_H__
#define __LIB_GUI_TEXTINPUT_H__

#include <lib/gui/ebutton.h>

class eTextInputField: public eButton
{
	unsigned int curPos, maxChars;
	int eventHandler( const eWidgetEvent &);
	void redrawWidget( gPainter *target, const eRect &area );
	void drawCursor();
	void nextChar();
	int lastKey;
	bool editMode;
	eString oldText;
	eString oldHelpText;
	eTimer nextCharTimer;
	eString useableChars;
public:
	eTextInputField( eWidget* parent, eLabel *descr=0, const char *deco="eLabel" );
	void setMaxChars( int i ) { maxChars = i; }
	void setUseableChars( const char* );
};

#endif
