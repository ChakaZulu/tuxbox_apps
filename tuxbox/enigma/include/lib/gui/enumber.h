#ifndef __enumber_h
#define __enumber_h

#include "ewidget.h"
#include "elabel.h"
#include "grc.h"

class eNumber: public eWidget
{
	Q_OBJECT
private:
	void redrawNumber(gPainter *, int n, const QRect &rect);
	void redrawWidget(gPainter *, const QRect &rect);
	QRect getNumberRect(int n);
	int eventFilter(const eWidgetEvent &event);
	int number[4];
	int len, space, active;
	gColor cursor, normal;
	int have_focus;
	int min, max, digit, maxdigits, isactive;
	QString descr;
	eLabel* tmpDescr; // used for description Label in LCD
signals:
	void selected(int *number);
protected:
	void keyUp(int key);
	void keyDown(int key);
	void gotFocus();
	void lostFocus();
public:
	eNumber(eWidget *parent, int len, int min, int max, int maxdigits, int *init, int isactive=0, eLabel* descr=0, int grabfocus=1);
	~eNumber();
	int getNumber(int f=0) { if ((f>=0) && (f<len)) return number[f]; return -1; }
};

#endif
