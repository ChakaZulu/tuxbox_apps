#include <setup_rfmod.h>

#include <lib/base/i18n.h>

#include <lib/driver/rc.h>
#include <lib/driver/rfmod.h>
#include <lib/gui/elabel.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/emessage.h>
#include <lib/gui/eskin.h>
#include <lib/gdi/font.h>
#include <lib/system/econfig.h>

eZapRFmodSetup::eZapRFmodSetup(): eWindow(0)
{
	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	setText(_("RF-Modulator Setup"));
	move(ePoint(150, 86));
	cresize(eSize(390, 320));
		
	TestPatternEnable=new eCheckbox(this);
	TestPatternEnable->setText(_("Test Pattern"));
	TestPatternEnable->move(ePoint(80, 20));
	TestPatternEnable->resize(eSize(200, 40));
	TestPatternEnable->setHelpText(_("enable test pattern"));
	TestPatternEnable->loadDeco();
	CONNECT(TestPatternEnable->selected, eZapRFmodSetup::TestPatternEnable_selected);		

	SO=0;
	eConfig::getInstance()->getKey("/elitedvb/rfmod/so",SO);
	SoundEnable=new eCheckbox(this);
	SoundEnable->setText(_("Sound enable"));
	SoundEnable->move(ePoint(80, 60));
	SoundEnable->resize(eSize(200, 40));
	SoundEnable->setHelpText(_("enable Sound"));
	SoundEnable->loadDeco();
	if(!SO)
		SoundEnable->setCheck(1);
	CONNECT(SoundEnable->selected, eZapRFmodSetup::SoundEnable_selected);		
		
	eLabel *sscl=new eLabel(this);
	sscl->setText("Sound Subcarrier:");
	sscl->move(ePoint(40,105));
	sscl->resize(eSize(200,40));

	SFD=5500;
	eConfig::getInstance()->getKey("/elitedvb/rfmod/sfd",SFD);
	SoundSubcarrier=new eListBox<eListBoxEntryText>(this,sscl);
	SoundSubcarrier->loadDeco();
	SoundSubcarrier->setFlags(eListBox<eListBoxEntryText>::flagNoUpDownMovement);
	SoundSubcarrier->move(ePoint(220, 105));
	SoundSubcarrier->resize(eSize(100,34));
	eListBoxEntryText* sscentrys[4];
	sscentrys[0]=new eListBoxEntryText(SoundSubcarrier,_("4.5 MHz"),(void*)4500,eTextPara::dirCenter);
	sscentrys[1]=new eListBoxEntryText(SoundSubcarrier,_("5.5 MHz"),(void*)5500,eTextPara::dirCenter);
	sscentrys[2]=new eListBoxEntryText(SoundSubcarrier,_("6.0 MHz"),(void*)6000,eTextPara::dirCenter);
	sscentrys[3]=new eListBoxEntryText(SoundSubcarrier,_("6.5 MHz"),(void*)6500,eTextPara::dirCenter);

	for(int i=0;i<4;i++)
	{
		if((int)sscentrys[i]->getKey() == SFD)
		{
			SoundSubcarrier->setCurrent(sscentrys[i]);
			break;
		}
	}
	SoundSubcarrier->setHelpText(_("change sound subcarrier frequency"));
	CONNECT(SoundSubcarrier->selchanged, eZapRFmodSetup::SoundSubcarrier_selected);		

	eLabel *cl=new eLabel(this);
	cl->setText("Channel:");
	cl->move(ePoint(40,145));
	cl->resize(eSize(200,40));

	DIV=2080;
	eConfig::getInstance()->getKey("/elitedvb/rfmod/div",DIV);
	Channel=new eListBox<eListBoxEntryText>(this,cl);
	Channel->loadDeco();
	Channel->setFlags(eListBox<eListBoxEntryText>::flagNoUpDownMovement);
	Channel->move(ePoint(220, 145));
	Channel->resize(eSize(100,34));
		
	eListBoxEntryText* clentrys[49];

	for(int i=21;i<70;i++)
	{
		clentrys[i]=new eListBoxEntryText(Channel,eString().sprintf("%d",i),(void*)i,eTextPara::dirCenter);
	}

	Channel->setCurrent(clentrys[0]);
	Channel->setHelpText(_("change channel"));

	eLabel *fl=new eLabel(this);
	fl->setText("Fine Tune:");
	fl->move(ePoint(40,185));
	fl->resize(eSize(200,40));
	
	FineTune=new eListBox<eListBoxEntryText>(this,fl);
	FineTune->loadDeco();
	FineTune->setFlags(eListBox<eListBoxEntryText>::flagNoUpDownMovement);
	FineTune->move(ePoint(220, 185));
	FineTune->resize(eSize(100,34));
		
	eListBoxEntryText* flentrys[81];

	for(int i=-40;i<41;i++)
	{
		if(i>0)
			flentrys[i]=new eListBoxEntryText(FineTune,eString().sprintf("+%d",i),(void*)i,eTextPara::dirCenter);
		else
			flentrys[i]=new eListBoxEntryText(FineTune,eString().sprintf("%d",i),(void*)i,eTextPara::dirCenter);
	}

	FineTune->setCurrent(flentrys[0]);
	FineTune->setHelpText(_("250Khz steps"));
	CONNECT(FineTune->selchanged, eZapRFmodSetup::FineTune_selected);		
	
	ok=new eButton(this);
	ok->setText(_("save"));
	ok->setShortcut("green");
	ok->setShortcutPixmap("green");

	ok->move(ePoint(20, 230));
	ok->resize(eSize(170, 40));
	ok->setHelpText(_("save settings and leave rf setup"));
	ok->loadDeco();
	CONNECT(ok->selected, eZapRFmodSetup::okPressed);		

	abort=new eButton(this);
	abort->setText(_("abort"));
	abort->move(ePoint(210, 230));
	abort->resize(eSize(170, 40));
	abort->setHelpText(_("leave rf setup (no changes are saved)"));
	abort->loadDeco();
	CONNECT(abort->selected, eZapRFmodSetup::abortPressed);

	status = new eStatusBar(this);	
	status->move( ePoint(0, clientrect.height()-30) );
	status->resize( eSize( clientrect.width(), 30) );
	status->loadDeco();
}

eZapRFmodSetup::~eZapRFmodSetup()
{
	if (status)
		delete status;
}

void eZapRFmodSetup::okPressed()
{
	close(1);
}

void eZapRFmodSetup::abortPressed()
{
	close(0);
}

void eZapRFmodSetup::TestPatternEnable_selected()
{
	eRFmod::getInstance()->setTPEN((int)1);		

	eMessageBox box(_("if you can read this your rfmod will not work."),_("Test Pattern"),eMessageBox::iconWarning|eMessageBox::btOK);

	box.show();
	box.exec();
	box.hide();

	TestPatternEnable->setCheck(0);
	eRFmod::getInstance()->setTPEN((int)0);		
}

void eZapRFmodSetup::SoundEnable_selected()
{
	int val;
	
	if(SoundEnable->isChecked())
		val=0;
	else
		val=1;		
		
	eRFmod::getInstance()->setSO(val);		

}

void eZapRFmodSetup::SoundSubcarrier_selected(eListBoxEntryText* entry)
{
	eRFmod::getInstance()->setSFD((int)SoundSubcarrier->getCurrent()->getKey());
}

void eZapRFmodSetup::Channel_selected(eListBoxEntryText* entry)
{
	eRFmod::getInstance()->setSO((int)entry->getKey());
}

void eZapRFmodSetup::FineTune_selected(eListBoxEntryText* entry)
{
	int tempDIV=DIV+(int)FineTune->getCurrent()->getKey();
	//DIV+=(int)FineTune->getCurrent()->getKey();
	eDebug("DIV:%d val:%d\n",DIV,(int)FineTune->getCurrent()->getKey());
	eRFmod::getInstance()->setDivider(tempDIV);
}
