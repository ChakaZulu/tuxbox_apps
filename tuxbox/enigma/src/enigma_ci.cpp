#include <enigma_ci.h>

#include <lib/base/i18n.h>

#include <lib/driver/rc.h>
#include <lib/gui/elabel.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/eskin.h>
#include <lib/gdi/font.h>

#include <lib/system/econfig.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/dvbci.h>

enigmaCImmi::enigmaCImmi(): eWindow(0)
{
	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	DVBCI=eDVB::getInstance()->DVBCI;

	setText(_("Common Interface Module - mmi"));
	move(ePoint(100, 70));
	resize(eSize(500, 420));

	tt=new eLabel(this);
	tt->setAlign(eTextPara::dirCenter);
	tt->setText("-- title text --");
	tt->move(ePoint(20,10));
	tt->resize(eSize(460,fd+4));

	stt=new eLabel(this);
	stt->setAlign(eTextPara::dirCenter);
	stt->setText("-- sub title text --");
	stt->move(ePoint(20,40));
	stt->resize(eSize(460,fd+4));
		
	bt=new eLabel(this);
	bt->setAlign(eTextPara::dirCenter);
	bt->setText("-- bottom text --");
	bt->move(ePoint(20,220));
	bt->resize(eSize(460,fd+4));

	cistate=new eLabel(this);
	cistate->setText("ci-status: waitung for module");
	cistate->move(ePoint(20,280));
	cistate->resize(eSize(360,fd+4));
	
	abort=new eButton(this);
	abort->setText(_("abort"));
	abort->move(ePoint(350, 250));
	abort->resize(eSize(90, fd+4));
	abort->setHelpText(_("leave ci mmi"));
	abort->loadDeco();

	CONNECT(abort->selected, enigmaCImmi::abortPressed);		

	lentrys=new eListBox<eListBoxMenuEntry>(this);
	lentrys->setName("MenuEntrys");
	lentrys->move(ePoint(20, 70));
	lentrys->resize(eSize(460, (fd+4)*6));
	lentrys->setFlags(eListBoxBase::flagNoPageMovement);
	
	//for(int i=0;i<4;i++)
	//	eListBoxMenuEntry *e=new eListBoxMenuEntry(lentrys,"blub");

	status = new eStatusBar(this);	
	status->move( ePoint(0, clientrect.height()-30) );
	status->resize( eSize( clientrect.width(), 30) );
	status->loadDeco();
	
	CONNECT(DVBCI->ci_mmi_progress, enigmaCImmi::getmmi);		

	DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::mmi_begin));
}

enigmaCImmi::~enigmaCImmi()
{

	DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::mmi_end));

	if (status)
		delete status;
}

void enigmaCImmi::abortPressed()
{
	close(0);
}

void enigmaCImmi::getmmi(const char *data)
{
	eDebug("new mmi message received");
	
	//for(int i=1;i<data[0];i++)
	//	printf("%02x ",data[i]);
	//printf("\n");

	if(data[5] == 0x9F && data[6] == 0x88)
	{
		int menupos=0;
		if(data[7]== 0x09 || data[7]==0xC)		//t_menu_last
		{
			int pos=10;
			int len=data[8];
			int choice=data[8];
			
			if(len&0x80)
			{
				len=data[10];
				pos=12;
			}
			
			eDebug("entering t_menu_last");
			
			while(pos<len)
			{
				if(data[pos++]==0x9f && data[pos++]==0x88 && data[pos++]==0x03) //fixed lines
				{
					int len=data[pos++];
					char buffer[len+1];
					eDebug("entering text_last");

					memcpy(buffer,data+pos,len);
					buffer[len]=0;
					pos+=len;
					
					if(menupos==0)
						tt->setText(buffer);					
					if(menupos==1)
						stt->setText(buffer);					
					if(menupos==2)
						bt->setText(buffer);					
					if(menupos>2)
					{
						lentrys->beginAtomic();	
						eListBoxMenuEntry *e=new eListBoxMenuEntry(lentrys,buffer);
						lentrys->endAtomic();	

					}							
					menupos++;
				}	
			}	
		}	
	}	
}


enigmaCI::enigmaCI(): eWindow(0)
{
	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	DVBCI=eDVB::getInstance()->DVBCI;

	setText(_("Common Interface Module"));
	move(ePoint(150, 136));
	resize(eSize(350, 300));

	reset=new eButton(this);
	reset->setText(_("Reset"));
	reset->move(ePoint(10, 13));
	reset->resize(eSize(330, fd+10));
	reset->setHelpText(_("reset the common interface module"));
	reset->loadDeco();

	CONNECT(reset->selected, enigmaCI::resetPressed);		

	init=new eButton(this);
	init->setText(_("Init"));
	init->move(ePoint(10, 53));
	init->resize(eSize(330, fd+10));
	init->setHelpText(_("send the ca-pmt to ci"));
	init->loadDeco();

	CONNECT(init->selected, enigmaCI::initPressed);		

	app=new eButton(this);
	app->setText(_("waiting for module"));
	app->move(ePoint(10, 93));
	app->resize(eSize(330, fd+10));
	app->setHelpText(_("enter ci menu (mmi)"));
	app->loadDeco();

	CONNECT(app->selected, enigmaCI::appPressed);		

	ok=new eButton(this);
	ok->setText(_("ok"));
	ok->move(ePoint(20, 150));
	ok->resize(eSize(90, fd+4));
	ok->setHelpText(_("leave common interface menu"));
	ok->loadDeco();

	CONNECT(ok->selected, enigmaCI::okPressed);		

	status = new eStatusBar(this);	
	status->move( ePoint(0, clientrect.height()-30) );
	status->resize( eSize( clientrect.width(), 30) );
	status->loadDeco();
	
	CONNECT(DVBCI->ci_progress, enigmaCI::updateCIinfo);		

	DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::init));
}

enigmaCI::~enigmaCI()
{
	if (status)
		delete status;
}

void enigmaCI::updateCIinfo(const char *buffer)
{
	eDebug("new info %s",buffer);
	app->setText(_(buffer));
}

void enigmaCI::okPressed()
{
	close(1);
}

void enigmaCI::resetPressed()
{
	app->setText(_("resetting....please wait"));
	DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::reset));
}

void enigmaCI::initPressed()
{
	DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::init));
}

void enigmaCI::appPressed()
{
	hide();
	enigmaCImmi mmi;
	mmi.show();
	mmi.exec();
	mmi.hide();
	show();
}

