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

void enigmaCImmi::entrySelected(eListBoxMenuEntry *choice)
{
	if(!choice)
	{
		eDebug("no selection");
		return;
	}
	eDebug("menu_answ: %d",choice->getEntry());	

	DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::mmi_menuansw,choice->getEntry()));
}

void enigmaCImmi::answokPressed()
{
	eDebug("Answer Ok pressed");
	DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::mmi_answ,0));
}

enigmaCImmi::enigmaCImmi(): eWindow(0)
{
	int fd=eSkin::getActive()->queryValue("fontsize", 8); //20

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
	lentrys->hide();
	CONNECT(lentrys->selected, enigmaCImmi::entrySelected);		

	headansw=new eLabel(this);
	headansw->setAlign(eTextPara::dirCenter);
	headansw->setText("-- bottom text --");
	headansw->move(ePoint(20,70));
	headansw->resize(eSize(460,fd+4));
	headansw->hide();

	int valinit[4]={0,0,0,0};
	answ=new eNumber(this,4,0,9,1,valinit,0,0);
	answ->move(ePoint(20,110));
	answ->resize(eSize(460, fd+4));
	answ->setHelpText(_("mmi input field"));
	answ->loadDeco();
	answ->hide();

	answok=new eButton(this);
	answok->setText(_("OK"));
	answok->move(ePoint(200, 170));
	answok->resize(eSize(90, fd+4));
	answok->setHelpText(_("send data to ci"));
	answok->loadDeco();
	answok->hide();

	CONNECT(answok->selected, enigmaCImmi::answokPressed);		
	
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
	int entry=1;
	eDebug("new mmi message received");

	lentrys->clearList();		
	lentrys->hide();
	answ->hide();
	answok->hide();
	headansw->hide();
	
	//for(int i=1;i<data[0];i++)
	//	printf("%02x ",data[i]);
	//printf("\n");

	if(data[5] == 0x9F && data[6] == 0x88)
	{
		int menupos=0;
		if(data[7]== 0x07)		//t_enq
		{
			int len=data[8];
			int blind=data[9]&1;
			int answerlen=data[10];
			char buf[len-2];
			answ->show();
			answok->show();						
			headansw->show();
			memcpy(buf,data+11,len-2);
			buf[len-2]=0;			

			headansw->setText(buf);
	
		}
		else if(data[7]== 0x09 || data[7]==0xC)		//t_menu_last
		{
			int pos=10;
			int len=data[8];
			int choice=data[8];
			
			lentrys->show();
			
			if(len&0x80)
			{
				if((len&0x7f)==2)
				{
					len=data[10];
					pos=12;
				}
				else if((len&0x7f)==1)
				{
					len=data[9];
					pos=11;
				}
			}
			
			eDebug("entering t_menu_last len=%d",len);
			
			while(pos<len)
			{
				if(data[pos++]==0x9f && data[pos++]==0x88 && data[pos++]==0x03) //fixed lines
				{
					int len=data[pos++];
					char buffer[len+1];
					eDebug("entering text_last len=%d",len);

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
						eListBoxMenuEntry *e=new eListBoxMenuEntry(lentrys,buffer,entry++);
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

