#include "edvb.h"
#include "setupaudio.h"
#include "ewindow.h"
#include "elistbox.h"
#include "ebutton.h"
#include "rc.h"
#include "enigma.h"

eZapAudioSetup::eZapAudioSetup():eWindow(0)
{
	setText("Audio Setup");
	move(QPoint(150, 136));
 	resize(QSize(220, 290));
	eListbox *list=new eListbox(this, eListbox::tBorder, eZap::FontSize);
	QString s;

	if (eDVB::getInstance()->config.getKey("/elitedvb/audio/useAC3", useAC3))
		useAC3=0;

	ok=new eButton(this);
	ok->setText("[save]");
	ok->move(QPoint(10, 200));
	ok->resize(QSize(90, eZap::FontSize+4));
	connect(ok, SIGNAL(selected()), SLOT(okPressed()));

	abort=new eButton(this);
	abort->setText("[abort]");
	abort->move(QPoint(120, 200));
	abort->resize(QSize(90, eZap::FontSize+4));
	connect(abort, SIGNAL(selected()), SLOT(abortPressed()));

	connect(new eListboxEntryText(list, useAC3?"AC3:  On":"AC3:  Off"), SIGNAL(selected(eListboxEntry*)), SLOT(sel_AC3(eListboxEntry*)));
	list->move(QPoint(10, 0));
	list->resize(QSize(200, 200));
}


eZapAudioSetup::~eZapAudioSetup()
{
}

void eZapAudioSetup::sel_AC3(eListboxEntry *lbe)
{
	useAC3=!useAC3;
	((eListboxEntryText*)lbe)->setText(useAC3?"AC3:  On":"AC3:  Off");
	lbe->parent->redraw();
}

void eZapAudioSetup::okPressed()
{
	eDVB::getInstance()->useAC3=useAC3;
	eDVB::getInstance()->config.setKey("/elitedvb/audio/useAC3", useAC3);
  close(1);
}

void eZapAudioSetup::abortPressed()
{
	close(0);
}


int eZapAudioSetup::eventFilter(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::keyDown:
		switch(event.parameter)
		{
		case eRCInput::RC_RIGHT:
			focusNext(0);
			return 1;
			break;
		case eRCInput::RC_LEFT:
			focusNext(1);
			return 1;
			break;
		}
	}
	return 0;
}
