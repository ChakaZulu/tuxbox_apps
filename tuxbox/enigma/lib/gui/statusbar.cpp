#include "statusbar.h"

#include <core/system/init.h>
#include <core/gui/eskin.h>
#include <core/gdi/font.h>

eStatusBar::eStatusBar( eWidget* parent, const char *deco)
	:eDecoWidget(parent, 0, deco), flags(0), client(this), current(0)
{
	client.setFont( eSkin::getActive()->queryFont("eStatusBar") );
	client.setForegroundColor ( eSkin::getActive()->queryColor("eStatusBar.foreground") );
	client.setBackgroundColor ( eSkin::getActive()->queryColor("eStatusBar.background") );
	client.setFlags( RS_FADE );
	initialize();
}


void eStatusBar::initialize()
{
	if ( !(flags & flagOwnerDraw) )
	{
		if (parent)
			CONNECT( parent->focusChanged, eStatusBar::update );
	}
	if (flags & flagVCenter)
		client.setFlags(eLabel::flagVCenter);
}

void eStatusBar::update( const eWidget* p )
{
	if (p)
	{
		current = p;
		invalidate( clientrect );
	}
}

int eStatusBar::getFlags() const	
{			
	return flags;	
}

void eStatusBar::setFlags( int fl )	
{
	flags = fl;
	initialize();
}

int eStatusBar::setProperty(const eString &prop, const eString &value)
{
	if (prop=="ownerDraw")
		flags |= flagOwnerDraw;
	else if (prop=="vCenter")
		flags |= flagVCenter;
	else
		return eDecoWidget::setProperty(prop, value);

	initialize();
	
	return 0;
}

void eStatusBar::redrawWidget(gPainter *target, const eRect& where)
{
/*	eDebug("redrawWidget eStatusbar");
	eDebug("where left = %i, top = %i, width = %i, height = %i", where.left(), where.top(), where.width(), where.height() );
	eDebug("left = 0, top = 0, width = %i, bottom = %i", width(), height() );*/
	if ( deco && where.contains( eRect(0, 0, width(), height() ) ) )
		deco.drawDecoration(target, ePoint(width(), height()));
/*	else
		eDebug("do not redraw Deco");*/
	
	if ( (!(flags & flagOwnerDraw)) && current && where.contains( clientrect ) )
		client.setText( current->getHelpText() );
/*	else
		eDebug("do not redraw Deco");*/
}

int eStatusBar::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
		case eWidgetEvent::changedSize:
			if (deco)
				clientrect.setRect( deco.borderLeft, deco.borderTop, width() - (deco.borderLeft+deco.borderRight), height() - (deco.borderTop+deco.borderBottom) );

			client.move( ePoint(0,0) );
			client.resize( clientrect.size() );

			invalidate();
		break;

		default:
		break;
	}
	return eDecoWidget::eventHandler(event);
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
