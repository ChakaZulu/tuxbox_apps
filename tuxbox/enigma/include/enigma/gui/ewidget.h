#ifndef __ewidget_h
#define __ewidget_h

//#include <qobject.h>
#include <ebase.h>
#include "epoint.h"
#include "esize.h"
#include "erect.h"
#include <qlist.h>
#include "grc.h"
#include <list>
#include <libsig_comp.h>

class eWidgetEvent
{
public:
	enum eventType
	{
		keyUp, keyDown,
		willShow, willHide,
		execBegin, execDone,
		gotFocus, lostFocus,
		
		changedText, changedFont, changedForegroundColor, changedBackgroundColor,
		changedSize, changedPosition, changedPixmap,
	} type;
	int parameter;
	eWidgetEvent(eventType type, int parameter=0): type(type), parameter(parameter) { }
};

class eWidget: public Object
{
//	Q_OBJECT
	enum
	{
		stateShow=1
	};

public:// slots:
	void close(int result);
	void accept();
	void reject();
	std::list<eWidget*> childlist;
	
protected:
	eWidget *parent;
	QString name;
	ePoint position;
	eSize size;
	eRect clientrect;
	eRect clientclip;
	QList<eWidget> _focusList;
	eWidget *oldfocus;
	int takefocus;
	int state;
	
	gDC *target;

	inline eWidget *getTLW()
	{
		return parent?parent->getTLW():this;
	}
	inline eWidget *getNonTransparentBackground()
	{
		if (getBackgroundColor()!=-1)
			return this;
		return parent?parent->getNonTransparentBackground():this;
	}
	int result, in_loop, have_focus, just_showing;
	void takeFocus();
	void releaseFocus();

	void _willShow();
	void _willHide();
	
	virtual void willShow();
	virtual void willHide();
	
	virtual void setPalette();

	void willShowChildren();
	void willHideChildren();
	
	virtual int eventFilter(const eWidgetEvent &event);	/** 0 for 'no action taken' */

	virtual void keyDown(int rc);
	virtual void keyUp(int rc);
	
	virtual void gotFocus();
	virtual void lostFocus();
	
	virtual void recalcClientRect();
	void recalcClip();
	void checkFocus();

			// generic properties
	gFont font;
	QString text;
	gColor backgroundColor, foregroundColor;
	
	gPixmap *pixmap;

public:
	eWidget *LCDTitle;
	eWidget *LCDElement;
	eWidget *LCDTmp;

	inline ePoint getAbsolutePosition()
	{
		return (parent?(parent->getAbsolutePosition()+parent->clientrect.topLeft()+position):position);
	}
	inline ePoint getTLWPosition()
	{
		return (parent?(parent->getTLWPosition()+parent->clientrect.topLeft()+position):ePoint(0,0));
	}
	virtual void redrawWidget(gPainter *target, const eRect &area);
	virtual void eraseBackground(gPainter *target, const eRect &area);
	eWidget(eWidget *parent=0, int takefocus=0);
	virtual ~eWidget();
	QList<eWidget> *focusList() { return &_focusList; }

	void resize(eSize size);
	void move(ePoint position);
	eSize getSize() { return size; }
	ePoint getPosition() { return position; }
	eSize getClientSize() { return clientrect.size(); }
	eRect getClientRect() { return clientrect; }

	void redraw(eRect area=eRect());
	void invalidate(eRect area=eRect());
	int exec();
	
	void clear();
	
	void event(const eWidgetEvent &event);
	void show();
	void hide();
	int isVisible() { return (state&stateShow) && ((!parent) || parent->isVisible()); }
	
	void focusNext(int dir=0);
	
	void setFont(const gFont &font);
	void setText(const QString &label);
	const	QString& getText() { return text; }
	void setBackgroundColor(gColor color);
	void setForegroundColor(gColor color);
	void setPixmap(gPixmap *pmap);
	void setTarget(gDC *target);
	void setLCD(eWidget *lcdtitle, eWidget *lcdelement);
	void setName(const char *name);
	
	gColor getBackgroundColor() { return backgroundColor; }
	gColor getForegroundColor() { return foregroundColor; }
	
	int width() { return getSize().width(); }
	int height() { return getSize().height(); }
	
	gPainter *getPainter(eRect area=eRect());
	
	virtual int setProperty(const QString &prop, const QString &value);
	
	eWidget *search(const QString &name);
};

#endif
