#ifndef __ebutton_h
#define __ebutton_h

#include <core/gui/elabel.h>
#include <core/gdi/grc.h>
#include <core/gui/decoration.h>

/**
 * \brief A widget which acts like a button.
 */
class eButton: public eLabel
{
	eLabel*	tmpDescr; // used for LCD with description
protected:
	eDecoration deco, deco_selected;
	eRect crect, crect_selected;    // this eRects holds the real client sizes when decoration is used
	gColor focusB, focusF, normalB, normalF;
	eString descr;
	int eventHandler(const eWidgetEvent &event);
	void gotFocus();
	void lostFocus();
public:
	/**
	 * \brief the "selected" signal.
	 *
	 * This signals is emitted when OK is pressed.
	 */
	Signal0<void> selected;

	Signal1<bool, eString&> selected_id;
	
	/**
	 * \brief Constructs a button.
	 *
	 * \param descr is for use with lcd
	 */
	eButton(eWidget *parent, eLabel* descr=0, int takefocus=1, bool loadDeco=true);

	void redrawWidget(gPainter *target, const eRect &area);
};

#endif
