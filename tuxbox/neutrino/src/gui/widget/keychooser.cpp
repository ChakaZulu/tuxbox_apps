#include "keychooser.h"
#include "../global.h"

CKeyChooser::CKeyChooser( int* Key, string title, string Icon )
	: CMenuWidget(title, Icon)
{
	key = Key;
	keyChooser = new CKeyChooserItem("key setup", key);
	keyDeleter = new CKeyChooserItemNoKey(key);

	addItem( new CMenuSeparator(CMenuSeparator::STRING, " ") );
	addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	addItem( new CMenuForwarder("back") );
	addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	addItem( new CMenuForwarder("setup new", true, "", keyChooser) );
	addItem( new CMenuForwarder("set none", true, "", keyDeleter) );
}


CKeyChooser::~CKeyChooser()
{
	delete keyChooser;
	delete keyDeleter;
}


void CKeyChooser::paint()
{
	CMenuWidget::paint();
	
	string text = "current key: " + CRCInput::getKeyName(*key);
	g_Fonts->menu->RenderString(x+ 10, y+ 65, width, text.c_str(), COL_MENUCONTENT);
}

//*****************************
CKeyChooserItem::CKeyChooserItem(string Name, int *Key)
{
	name = Name;
	key = Key;
	width = 350;
	hheight = g_Fonts->menu_title->getHeight();
	mheight = g_Fonts->menu->getHeight();
	height = hheight+2*mheight;
	x=((720-width) >> 1) -20;
	y=(576-height)>>1;
}


int CKeyChooserItem::exec(CMenuTarget* parent, string)
{
	if (parent)
	{
		parent->hide();
	}
	paint();

	int rckey = g_RCInput->getKey(100);
	if(rckey!=CRCInput::RC_timeout)
	{//timeout, don't set the key
		*key = rckey;
	}

	hide();
	return CMenuTarget::RETURN_REPAINT;
}

void CKeyChooserItem::hide()
{
	g_FrameBuffer->paintBoxRel(x, y, width, height, COL_BACKGROUND);
}

void CKeyChooserItem::paint()
{

	g_FrameBuffer->paintBoxRel(x, y, width, hheight, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+ 10, y+ hheight, width, name.c_str(), COL_MENUHEAD);
	g_FrameBuffer->paintBoxRel(x, y+ hheight, width, height-hheight, COL_MENUCONTENT);

	//paint msg...
	g_Fonts->menu->RenderString(x+ 10, y+ hheight+ mheight, width, "please press the new key", COL_MENUCONTENT);
	g_Fonts->menu->RenderString(x+ 10, y+ hheight+ mheight* 2, width, "wait a few seconds for cancel", COL_MENUCONTENT);
}


