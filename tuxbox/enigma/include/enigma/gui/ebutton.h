#ifndef __ebutton_h
#define __ebutton_h

#include <core/gui/elabel.h>
#include <core/gdi/grc.h>

/**
 * \brief A widget which acts like a button.
 */
class eButton: public eLabel
{
	gColor focus, normal;
	eLabel*	tmpDescr; // used for LCD with description
protected:
	eString descr;
	int eventHandler(const eWidgetEvent &event);
	void gotFocus();
	void lostFocus();
public:
	/**
	 * \brief the "selected" signal.
	 *
	 * This signal is emitted when OK is pressed.
	 */
	Signal0<void> selected;
	
	/**
	 * \brief Constructs a button.
	 *
	 * \param descr is for use with lcd
	 */
	eButton(eWidget *parent, eLabel* descr=0, int takefocus=1);
};

#endif
