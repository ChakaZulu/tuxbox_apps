#include "keychooser.h"

CKeyChooser::CKeyChooser( int* Key, string title, FontsDef *Fonts, string Icon )
	: CMenuWidget(title, Fonts, Icon)
{
	key = Key;
	keyChooser = new CKeyChooserItem("key setup", fonts, key);
	keyDeleter = new CKeyChooserItemNoKey(key);

	addItem( new CMenuSeparator(CMenuSeparator::STRING, " ", fonts) );
	addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	addItem( new CMenuForwarder("back", fonts) );
	addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	addItem( new CMenuForwarder("setup new", fonts, true, "", keyChooser) );
	addItem( new CMenuForwarder("set none", fonts, true, "", keyDeleter) );
}


CKeyChooser::~CKeyChooser()
{
	delete keyChooser;
	delete keyDeleter;
}


void CKeyChooser::paint(CFrameBuffer* frameBuffer)
{
	CMenuWidget::paint(frameBuffer);
	
	string text = "current key: " + CRCInput::getKeyName(*key);
	fonts->menu->RenderString(x+10,y+65, width, text.c_str(), COL_MENUCONTENT);
}

//*****************************
CKeyChooserItem::CKeyChooserItem(string Name, FontsDef *Fonts, int *Key)
{
	name = Name;
	fonts = Fonts;
	key = Key;
	width = 350;
	hheight = fonts->menu_title->getHeight();
	mheight = fonts->menu->getHeight();
	height = hheight+2*mheight;
	x=((720-width) >> 1) -20;
	y=(576-height)>>1;
}


int CKeyChooserItem::exec(CFrameBuffer* frameBuffer, CRCInput *rcInput,CMenuTarget* parent, string)
{
	if (parent)
	{
		parent->hide(frameBuffer);
	}
	paint( frameBuffer );

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

void CKeyChooserItem::paint(CFrameBuffer* frameBuffer)
{

	frameBuffer->paintBoxRel(x,y, width,hheight, COL_MENUHEAD);
	fonts->menu_title->RenderString(x+10,y+hheight, width, name.c_str(), COL_MENUHEAD);
	frameBuffer->paintBoxRel(x,y+hheight, width,height-hheight, COL_MENUCONTENT);

	//paint msg...
	fonts->menu->RenderString(x+10,y+hheight+mheight, width, "please press the new key", COL_MENUCONTENT);
	fonts->menu->RenderString(x+10,y+hheight+mheight*2, width, "wait a few seconds for cancel", COL_MENUCONTENT);
}

