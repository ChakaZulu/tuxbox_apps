#ifndef __enumber_h
#define __enumber_h

#include <core/gui/ewidget.h>
#include <core/gui/decoration.h>

class eLabel;
class gPainter;

/**
 * \brief A widget to enter a number.
 */
class eNumber: public eWidget
{
private:
	eDecoration deco, deco_selected;
	eRect crect, crect_selected;    // this eRects holds the real client sizes when decoration is used
	void redrawNumber(gPainter *, int n, const eRect &rect);
	void redrawWidget(gPainter *, const eRect &rect);
	eRect getNumberRect(int n);
	int eventHandler(const eWidgetEvent &event);
	int number[16];
	int len, space, space_selected, active;
	gColor cursorB, cursorF, normalB, normalF;
	int have_focus;
	int min, max, digit, maxdigits, isactive;
	int flags;
	int base;
	eString descr;
	eLabel* tmpDescr; // used for description Label in LCD
protected:
	int keyDown(int key);
	void gotFocus();
	void lostFocus();
public:
	Signal1<void, int*> selected;
	Signal0<void> numberChanged;
	eNumber(eWidget *parent, int len, int min, int max, int maxdigits, int *init, int isactive=0, eLabel* descr=0, int grabfocus=1, int DrawDeco=1);
	~eNumber();
	int getNumber(int f) { if ((f>=0) && (f<len)) return number[f]; return -1; }
	void setNumber(int f, int n);

	void setLimits(int min, int max);
	void setNumberOfFields(int n);
	void setMaximumDigits(int n);
	enum
	{
		flagDrawPoints=1,
		flagDrawBoxes=2
	};
	void setFlags(int flags);
	void setBase(int base);
	
	void setNumber(int n);
	int getNumber();
};

#endif
