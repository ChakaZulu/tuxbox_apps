#include "keychooser.h"

CKeyChooser::CKeyChooser( int* Key, string title, string Icon )
	: CMenuWidget(title, Icon)
{
	key = Key;
	keyChooser = new CKeyChooserItem("key setup", key);
	keyDeleter = new CKeyChooserItemNoKey(key);

	addItem( new CMenuSeparator(60) );
	addItem( new CMenuSeparator(11, CMenuSeparator::LINE) );
	addItem( new CMenuForwarder("back") );
	addItem( new CMenuSeparator(11, CMenuSeparator::LINE) );
	addItem( new CMenuForwarder("setup new", true, "", keyChooser) );
	addItem( new CMenuForwarder("set none", true, "", keyDeleter) );
}


CKeyChooser::~CKeyChooser()
{
	delete keyChooser;
	delete keyDeleter;
}


void CKeyChooser::paint(CFrameBuffer* frameBuffer, FontsDef *fonts)
{
	CMenuWidget::paint(frameBuffer,fonts);
	
	string text = "current key: " + CRCInput::getKeyName(*key);
	fonts->menu->RenderString(x+10,y+70, width, text.c_str(), COL_MENUCONTENT);
}

//*****************************
CKeyChooserItem::CKeyChooserItem(string Name, int *Key)
{
	name = Name;
	key = Key;
	width = 300;
	height = 105;
	x=((720-width) >> 1) -20;
	y=(576-height)>>1;
}


int CKeyChooserItem::exec(CFrameBuffer* frameBuffer, FontsDef *fonts, CRCInput *rcInput,CMenuTarget* parent, string)
{
	if (parent)
	{
		parent->hide(frameBuffer);
	}
	paint( frameBuffer, fonts );

	int rckey = rcInput->getKey(100); 
	if(rckey!=-1)
	{//timeout, don't set the key
		*key = rckey;
	}

	hide(frameBuffer);
	return CMenuTarget::RETURN_REPAINT;
}

void CKeyChooserItem::hide(CFrameBuffer* frameBuffer)
{
	frameBuffer->paintBoxRel(x,y, width,height, COL_BACKGROUND);
}

void CKeyChooserItem::paint(CFrameBuffer* frameBuffer, FontsDef *fonts)
{

	frameBuffer->paintBoxRel(x,y, width,30, COL_MENUHEAD);
	fonts->menu_title->RenderString(x+10,y+23, width, name.c_str(), COL_MENUHEAD);
	frameBuffer->paintBoxRel(x,y+30, width,height-30, COL_MENUCONTENT);

	//paint msg...
	fonts->menu->RenderString(x+10,y+60, width, "please press the new key", COL_MENUCONTENT);
	fonts->epg_info2->RenderString(x+10,y+90, width, "wait a few seconds for cancel", COL_MENUCONTENT);
}

