#include <setup_osd.h>

#include <setupskin.h>
#include <enigma.h>
#include <enigma_main.h>
#include <lib/base/i18n.h>
#include <lib/dvb/edvb.h>
#include <lib/gdi/gfbdc.h>
#include <lib/gdi/font.h>
#include <lib/gui/eskin.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/guiactions.h>
#include <lib/gui/slider.h>
#include <lib/gui/statusbar.h>
#include <lib/gui/emessage.h>
#include <lib/system/econfig.h>
#include <lib/system/info.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>

class PluginOffsetScreen: public eWidget
{
	enum { posLeftTop, posRightBottom } curPos;
	eLabel *descr;
	int eventHandler( const eWidgetEvent & e );
	int left, top, right, bottom;
	void redrawLeftTop( gPainter *target );
	void redrawRightBottom( gPainter *target );
	void redrawWidget(gPainter *target, const eRect &where);
	gColor foreColor, backColor;
public:
	PluginOffsetScreen();
};

struct PluginOffsetActions
{
	eActionMap map;
	eAction leftTop, rightBottom, store;
	PluginOffsetActions()
		:map("PluginOffsetActions", _("PluginOffsetActions")),
		leftTop(map,"leftTop", _("enable set the leftTop Point of the rectangle")),
		rightBottom(map,"rightBottom", _("enable set the rightBottom Point of the rectangle")),
		store(map, "store", _("saves the current positions"))
	{
	}
};

eAutoInitP0<PluginOffsetActions> i_PluginOffsetActions(eAutoInitNumbers::actions, "tuxtxt offset actions");

int PluginOffsetScreen::eventHandler( const eWidgetEvent &event )
{
	switch ( event.type )
	{
		case eWidgetEvent::execBegin:
			eConfig::getInstance()->getKey("/enigma/plugins/needoffsets/left", left);
			eConfig::getInstance()->getKey("/enigma/plugins/needoffsets/top", top);
			eConfig::getInstance()->getKey("/enigma/plugins/needoffsets/right", right);
			eConfig::getInstance()->getKey("/enigma/plugins/needoffsets/bottom", bottom);
			invalidate();
			return 0;
		case eWidgetEvent::execDone:
			eConfig::getInstance()->setKey("/enigma/plugins/needoffsets/left", left);
			eConfig::getInstance()->setKey("/enigma/plugins/needoffsets/top", top);
			eConfig::getInstance()->setKey("/enigma/plugins/needoffsets/right", right);
			eConfig::getInstance()->setKey("/enigma/plugins/needoffsets/bottom", bottom);
			return 0;
		case eWidgetEvent::willShow:
			invalidate();
			return 0;
		case eWidgetEvent::evtAction:
			if (event.action == &i_PluginOffsetActions->leftTop)
			{
				curPos=posLeftTop;
				return 0;
			}
			else if (event.action == &i_PluginOffsetActions->rightBottom)
			{
				curPos=posRightBottom;
				return 0;
			}
			else if (event.action == &i_cursorActions->cancel)
			{
				close(0);
				return 0;
			}
			else if (event.action == &i_PluginOffsetActions->store)
			{
				close(0);
				return 0;
			}
			else if (event.action == &i_cursorActions->left)
			{
				if ( curPos == posLeftTop )
					left--;
				else if (curPos == posRightBottom )
					right--;
			}
			else if (event.action == &i_cursorActions->right)
			{
				if ( curPos == posLeftTop )
					left++;
				else if (curPos == posRightBottom )
					right++;
			}
			else if (event.action == &i_cursorActions->up)
			{
				if ( curPos == posLeftTop )
					top--;
				else if (curPos == posRightBottom )
					bottom--;
			}
			else if (event.action == &i_cursorActions->down)
			{
				if ( curPos == posLeftTop )
					top++;
				else if (curPos == posRightBottom )
					bottom++;
			}
			else
				break;
			if ( curPos == posLeftTop )
				invalidate( eRect( ePoint(left-1, top-1), eSize(102, 102) ) );
			else if ( curPos == posRightBottom )
				invalidate( eRect( ePoint(right-101, bottom-101), eSize(102, 102) ) );
			return 0;
		default:
			break;
	}
	return eWidget::eventHandler( event );
}

void PluginOffsetScreen::redrawLeftTop( gPainter *target )
{
	target->fill( eRect( ePoint( left, top ), eSize( 100, 3 ) ) );
	target->fill( eRect( ePoint( left, top ), eSize( 3, 100 ) ) );
}

void PluginOffsetScreen::redrawRightBottom( gPainter *target )
{
	target->fill( eRect( ePoint( right-3, bottom-100 ), eSize( 3, 100 ) ) );
	target->fill( eRect( ePoint( right-100, bottom-3 ), eSize( 100, 3 ) ) );
}

void PluginOffsetScreen::redrawWidget(gPainter *target, const eRect &where)
{
	target->setForegroundColor( foreColor );
	if ( where.intersects( eRect(	ePoint( left, top ), eSize( 100, 100 ) ) ) )
		redrawLeftTop( target );
	if ( where.intersects( eRect( ePoint( right-3, bottom-100 ), eSize( 3, 100 ) ) ) )
		redrawRightBottom( target );
}

PluginOffsetScreen::PluginOffsetScreen()
	:eWidget(0, 1), curPos( posLeftTop ),
		left(20), top(20), right( 699 ), bottom( 555 )
{
	foreColor = eSkin::getActive()->queryColor("eWindow.titleBarFont");
	setForegroundColor( foreColor );
	move(ePoint(0,0));
	resize(eSize(768,576));
	descr = new eLabel( this );
	descr->setFlags( eLabel::flagVCenter|RS_WRAP );
	descr->setForegroundColor( foreColor );
	descr->resize(eSize(568,300));
	descr->move(ePoint(100,100));
	descr->setText(_("here you can center the tuxtxt rectangle...\npress red to select the left top edge\npress green to select the right bottom edge\nuse the cursor keys to move the selected edges"));
	eSize ext = descr->getExtend();
	ext+=eSize(8,4);  // the given Size of the Text is okay... but the renderer sucks...
	descr->resize( ext );
	descr->move( ePoint( (width()/2)-(ext.width()/2) , (height()/2)-(ext.height()/2) ) );
	descr->show();
	addActionMap(&i_PluginOffsetActions->map);
	addActionMap(&i_cursorActions->map);
	addActionToHelpList( &i_PluginOffsetActions->leftTop );
	addActionToHelpList( &i_PluginOffsetActions->rightBottom );
	setHelpID(96);
}

eZapOsdSetup::eZapOsdSetup()
	:eWindow(0)
{
	init_eZapOsdSetup();
}

void eZapOsdSetup::init_eZapOsdSetup()
{
	setText(_("OSD Settings"));
	cmove(ePoint(140, 125));
	cresize(eSize(460, 350));

	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	alpha = gFBDC::getInstance()->getAlpha();
	eLabel* l = new eLabel(this);
	l->setText(_("Alpha:"));
	l->move(ePoint(20, 20));
	l->resize(eSize(110, fd+4));
	sAlpha = new eSlider( this, l, 0, 512 );
	sAlpha->setIncrement( eSystemInfo::getInstance()->getAlphaIncrement() ); // Percent !

	sAlpha->move( ePoint( 150, 20 ) );
	sAlpha->resize(eSize( 290, fd+4 ) );
	sAlpha->setHelpText(_("change the transparency correction"));
	sAlpha->setValue( alpha);
	CONNECT( sAlpha->changed, eZapOsdSetup::alphaChanged );

	brightness = gFBDC::getInstance()->getBrightness();
	l = new eLabel(this);
	l->setText(_("Brightness:"));
	l->move(ePoint(20, 60));
	l->resize(eSize(120, fd+4));
	sBrightness = new eSlider( this, l, 0, 255 );
	sBrightness->setIncrement( 5 ); // Percent !
	sBrightness->move( ePoint( 150, 60 ) );
	sBrightness->resize(eSize( 290, fd+4 ) );
	sBrightness->setHelpText(_("change the brightness correction"));
	sBrightness->setValue( brightness);
	CONNECT( sBrightness->changed, eZapOsdSetup::brightnessChanged );

	gamma = gFBDC::getInstance()->getGamma();
	l = new eLabel(this);
	l->setText(_("Contrast:"));
	l->move(ePoint(20, 100));
	l->resize(eSize(120, fd+4));
	sGamma = new eSlider( this, l, 0, 255 );
	sGamma->setIncrement( 5 ); // Percent !
	sGamma->move( ePoint( 150, 100 ) );
	sGamma->resize(eSize( 290, fd+4 ) );
	sGamma->setHelpText(_("change the contrast"));
	sGamma->setValue( gamma);
	CONNECT( sGamma->changed, eZapOsdSetup::gammaChanged );

	simpleMainMenu=new eCheckbox(this);
	simpleMainMenu->setText(_("Show Mainmenu like Listbox"));
	simpleMainMenu->setHelpText(_("show the Mainmenu in normal listbox style"));
	simpleMainMenu->move(ePoint(20,140));
	simpleMainMenu->resize(eSize(420,35));
	int bla=0;
	eConfig::getInstance()->getKey("/ezap/osd/simpleMainMenu", bla);
	simpleMainMenu->setCheck(bla);

	skin=new eButton(this);
	skin->setText(_("Change skin"));
	skin->setShortcut("blue");
	skin->setShortcutPixmap("blue");
	skin->move(ePoint(20, 190));
	skin->resize(eSize(205, 40));
	skin->loadDeco();
	skin->setHelpText(_("press ok to open skin selector"));
	CONNECT( skin->selected, eZapOsdSetup::skinPressed );

	pluginoffs=new eButton(this);
	pluginoffs->setText(_("TuxText position"));
	pluginoffs->setHelpText(_("here you can center the Tuxtxt (builtin videotext)"));
	pluginoffs->setShortcut("yellow");
	pluginoffs->setShortcutPixmap("yellow");
	pluginoffs->move(ePoint(235, 190));
	pluginoffs->resize(eSize(205, 40));
	pluginoffs->loadDeco();
	CONNECT( pluginoffs->selected, eZapOsdSetup::pluginPositionPressed );

	ok=new eButton(this);
	ok->setText(_("save"));
	ok->setShortcut("green");
	ok->setShortcutPixmap("green");
	ok->move(ePoint(20, 245));
	ok->resize(eSize(205, 40));
	ok->setHelpText(_("save changes and return"));
	ok->loadDeco();

	CONNECT(ok->selected, eZapOsdSetup::okPressed);

	statusbar=new eStatusBar(this);
	statusbar->move( ePoint(0, clientrect.height()-50 ) );
	statusbar->resize( eSize( clientrect.width(), 50) );
	statusbar->loadDeco();
	setHelpID(83);
}

eZapOsdSetup::~eZapOsdSetup()
{
}

void eZapOsdSetup::alphaChanged( int i )
{
	alpha = i;
	gFBDC::getInstance()->setAlpha(alpha);
}

void eZapOsdSetup::brightnessChanged( int i )
{
	brightness = i;
	gFBDC::getInstance()->setBrightness(brightness);
}

void eZapOsdSetup::gammaChanged( int i )
{
	gamma = i;
	gFBDC::getInstance()->setGamma(gamma);
}

void eZapOsdSetup::pluginPositionPressed()
{
	eWidget *oldfocus=focus;
	hide();
	PluginOffsetScreen scr;
	scr.show();
	scr.exec();
	scr.hide();
	show();
	setFocus(oldfocus);
}

void eZapOsdSetup::okPressed()
{
	eConfig::getInstance()->setKey("/ezap/osd/simpleMainMenu", simpleMainMenu->isChecked());
	gFBDC::getInstance()->saveSettings();
	eConfig::getInstance()->flush();
	close(1);
}

void eZapOsdSetup::skinPressed()
{
	eWidget *oldfocus=focus;
	hide();
	eSkinSetup setup;
	int res;
#ifndef DISABLE_LCD
	setup.setLCD(LCDTitle, LCDElement);
#endif
	setup.show();
	res=setup.exec();
	setup.hide();
	if (!res)
	{
		eMessageBox msg(_("You have to restart enigma to apply the new skin\nRestart now?"), _("Skin changed"), eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion, eMessageBox::btYes );
		msg.show();
		int ret = msg.exec();
		msg.hide();
		if ( ret == eMessageBox::btYes )
		{
			if ( eZapMain::getInstance()->checkRecordState() )
				eZap::getInstance()->quit(2);
		}
		msg.hide();
	}
	show();
	setFocus(oldfocus);
}

int eZapOsdSetup::eventHandler( const eWidgetEvent &e )
{
	switch (e.type)
	{
		case eWidgetEvent::execDone:
		{
			gFBDC::getInstance()->reloadSettings();
			break;
		}
		default:
			return eWindow::eventHandler( e );
	}
	return 1;
}
