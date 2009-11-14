#include <setup_osd.h>

#include <setup_osd_extra.h>

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
	void init_PluginOffsetScreen();
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
	init_PluginOffsetScreen();
}
void PluginOffsetScreen::init_PluginOffsetScreen()
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
	alpha = gFBDC::getInstance()->getAlpha();
	sAlpha = CreateSkinnedSlider("alpha","lalpha", 0, 512 );
	sAlpha->setIncrement( eSystemInfo::getInstance()->getAlphaIncrement() ); // Percent !
	sAlpha->setValue( alpha);
	CONNECT( sAlpha->changed, eZapOsdSetup::alphaChanged );

	brightness = gFBDC::getInstance()->getBrightness();
	sBrightness = CreateSkinnedSlider("brightness","lbrightness", 0, 255 );
	sBrightness->setIncrement( 5 ); // Percent !
	sBrightness->setValue( brightness);
	CONNECT( sBrightness->changed, eZapOsdSetup::brightnessChanged );

	gamma = gFBDC::getInstance()->getGamma();
	sGamma = CreateSkinnedSlider("contrast","lcontast", 0, 255 );
	sGamma->setIncrement( 5 ); // Percent !
	sGamma->setValue( gamma);
	CONNECT( sGamma->changed, eZapOsdSetup::gammaChanged );

	simpleMainMenu=CreateSkinnedCheckbox("simpleMainMenu",0,"/ezap/osd/simpleMainMenu");

	CONNECT( CreateSkinnedButton("skin")->selected, eZapOsdSetup::skinPressed );

	CONNECT( CreateSkinnedButton("pluginoffs")->selected, eZapOsdSetup::pluginPositionPressed );

	CONNECT(CreateSkinnedButton("ok")->selected, eZapOsdSetup::okPressed);

	CONNECT( CreateSkinnedButton("expert")->selected, eZapOsdSetup::expertPressed );

	BuildSkin("eZapOsdSetup");

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
		int ret = eMessageBox::ShowBox(_("You have to restart enigma to apply the new skin\nRestart now?"), _("Skin changed"), eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion, eMessageBox::btYes );
		if ( ret == eMessageBox::btYes )
		{
			if ( eZapMain::getInstance()->checkRecordState() )
				eZap::getInstance()->quit(2);
		}
	}
	show();
	setFocus(oldfocus);
}

void eZapOsdSetup::expertPressed()
{
	hide();
	eOSDExpertSetup setup;
#ifndef DISABLE_LCD
	setup.setLCD(LCDTitle, LCDElement);
#endif
	setup.show();
	setup.exec();
	setup.hide();
	eConfig::getInstance()->flush();
	show();
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
