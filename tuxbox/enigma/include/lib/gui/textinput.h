#ifndef __LIB_GUI_TEXTINPUT_H__
#define __LIB_GUI_TEXTINPUT_H__

#include <lib/gui/ebutton.h>
#include <stack>
#include <map>
#include <vector>

class eTextInputFieldHelpWidget;

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
	bool capslock, swapNum;
	eString isotext;
	eLabel *editLabel;
	std::stack< std::pair<int,int> > scroll;
	static std::map< eString, std::vector<std::pair< eString,eString > > > keymappings;
	eTextInputFieldHelpWidget *helpwidget;
	void lostFocus();
	void updateHelpWidget();
public:
	eTextInputField( eWidget* parent, eLabel *descr=0, eTextInputFieldHelpWidget* hlp=0, const char *deco="eNumber" );
	static void loadKeyMappings();
	void setMaxChars( int i ) { maxChars = i; }
	void setUseableChars( const char* );
	void setNextCharTimeout( unsigned int );
	void setEditHelpText( eString str ) { editHelpText=str; }
	bool inEditMode() const { return editMode; }
};

class eTextInputFieldHelpWidget : public eWidget
{
	friend class eTextInputField;
	void redrawWidget(gPainter *target, const eRect & area );
	eLabel *keys[12];
public:
	eTextInputFieldHelpWidget(eWidget *parent);
};

#endif
