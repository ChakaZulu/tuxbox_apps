#ifndef __ebutton_h
#define __ebutton_h

#include <lib/gui/elabel.h>
#include <lib/gdi/grc.h>

/**
 * \brief A widget which acts like a button.
 */
class eButton: public eLabel
{
	eLabel*	tmpDescr; // used for LCD with description
protected:
	gColor focusB, focusF, normalB, normalF;
	eString descr;
	int eventHandler(const eWidgetEvent &event);
	void gotFocus();
	void lostFocus();
public:
	/**
	 * \brief Constructs a button.
	 *
	 * \param descr is for use with lcd
	 */
	eButton(eWidget *parent, eLabel* descr=0, int takefocus=1, const char *deco="eButton" );
	/**
	 * \brief the "selected" signal.
	 *
	 * This signals is emitted when OK is pressed.
	 */
	Signal0<void> selected;
	Signal1<void, eButton*> selected_id;
};

#endif
