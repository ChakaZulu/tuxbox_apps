#include "statusbar.h"

#include <core/system/init.h>
#include <core/gui/eskin.h>

eStatusBar::eStatusBar( eWidget* parent, int fl)
	:eWidget(parent), flags( fl ), client(this)
{
	if ( !(flags & flagOwnerDraw) )
	{
		if (parent)
			CONNECT( parent->focusChanged, eStatusBar::update );
	}
	client.setFont( eSkin::getActive()->queryFont("eStatusBar") );
	client.setForegroundColor ( eSkin::getActive()->queryColor("eStatusBar.foreground") );
	client.setBackgroundColor ( eSkin::getActive()->queryColor("eStatusBar.background") );
}

void eStatusBar::update( const eWidget* p )
{
	if (p)
		client.setText( p->getHelpText() );
}

int eStatusBar::getFlags() const	
{			
	return flags;	
}

void eStatusBar::setFlags( int fl )	
{
	flags = fl;
	if (fl & flagLoadDeco)
		loadDeco();
}

void eStatusBar::loadDeco()
{
	if (!deco)
	{
		deco.load("eStatusBar");
		event(eWidgetEvent(eWidgetEvent::changedSize));
	}
}

int eStatusBar::setProperty(const eString &prop, const eString &value)
{
	if (prop=="loadDeco")
	{
		flags |= flagLoadDeco;
		loadDeco();
	}
	else if (prop=="ownerDraw")
		flags |= flagOwnerDraw;
	else
		return eWidget::setProperty(prop, value);
	
	return 0;
}

void eStatusBar::redrawBorder(gPainter *target, const eRect& where)
{
	if ( where.contains( eRect(0, 0, width(), height() ) ) )
		if (deco)
			deco.drawDecoration(target, ePoint(width(), height()));
}

void eStatusBar::redrawWidget(gPainter *target, const eRect& where)
{
	redrawBorder(target, where);		
}

int eStatusBar::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
		case eWidgetEvent::changedSize:
			if (deco)
			{
				client.move( ePoint(deco.borderLeft, deco.borderTop) );
				client.resize( eSize(width() - (deco.borderLeft+deco.borderRight), height() - (deco.borderTop+deco.borderBottom)) );
			}
			else
			{
				client.move( ePoint(0,0) );
				client.resize( eSize(width(), height()) );
			}
			invalidate();
		break;

		default:
		break;
	}
	return eWidget::eventHandler(event);
}

static eWidget *create_eStatusBar(eWidget *parent)
{
	return new eStatusBar(parent);
}

class eStatusBarSkinInit
{
public:
	eStatusBarSkinInit()
	{
		eSkin::addWidgetCreator("eStatusBar", create_eStatusBar);
	}
	~eStatusBarSkinInit()
	{
		eSkin::removeWidgetCreator("eStatusBar", create_eStatusBar);
	}
};

eAutoInitP0<eStatusBarSkinInit> init_eStatusBarSkinInit(3, "eStatusBar");
