#include <enigma_ci.h>

#include <lib/base/i18n.h>

#include <lib/driver/rc.h>
#include <lib/gui/elabel.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/enumber.h>
#include <lib/gui/eskin.h>
#include <lib/gdi/font.h>

#include <lib/system/econfig.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/dvbci.h>

eCImmi::eCImmi(eWidget *parent): eWidget(parent)
{
}

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
	int x;
	
	x=mmi->answer->getNumber();
	eDebug("Answer Ok pressed val:%d",x);
	

	DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::mmi_answ,0));
}

enigmaCImmi::enigmaCImmi(): eWindow(0), mmi(0)
{
	int fd=eSkin::getActive()->queryValue("fontsize", 8); //20

	DVBCI=eDVB::getInstance()->DVBCI;

	setText(_("Common Interface Module - mmi"));
	move(ePoint(50, 70));
	resize(eSize(600, 420));

	tt=new eLabel(this);
	tt->setAlign(eTextPara::dirCenter);
	tt->setText("");
	tt->move(ePoint(20,10));
	tt->resize(eSize(560,fd+4));

	stt=new eLabel(this);
	stt->setFlags(RS_WRAP);
	stt->setAlign(eTextPara::dirCenter);
	stt->setText("");
	stt->move(ePoint(5,40));
	stt->resize(eSize(590,(fd+4)*2));
			
	bt=new eLabel(this);
	bt->setAlign(eTextPara::dirCenter);
	bt->setText("");
	bt->move(ePoint(20,250));
	bt->resize(eSize(560,fd+4));

	cistate=new eLabel(this);
	cistate->setText("ci-status: waitung for module");
	cistate->move(ePoint(20,280));
	cistate->resize(eSize(460,fd+4));
	
	abort=new eButton(this);
	abort->setText(_("abort"));

	abort->move(ePoint(310, 280));
	abort->resize(eSize(150, 30));

	abort->setHelpText(_("leave ci mmi"));
	abort->loadDeco();

	CONNECT(abort->selected, enigmaCImmi::abortPressed);		

	lentrys=new eListBox<eListBoxMenuEntry>(this);
	lentrys->setName("MenuEntrys");
	lentrys->move(ePoint(20, 100));
	lentrys->resize(eSize(560, (fd+4)*6));
	lentrys->setFlags(eListBoxBase::flagNoPageMovement);
	lentrys->hide();
	CONNECT(lentrys->selected, enigmaCImmi::entrySelected);		

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

long enigmaCImmi::LengthField(unsigned char *lengthfield,long maxlength,int *fieldlen)
{
	int ByteCount = (int)(lengthfield[0]&0x7f);
	long Length	= (long)ByteCount;
	int indexField, indexVar;
	unsigned char *tmp;
	
	if(lengthfield[0] & 0x80)
	{
		*fieldlen = ByteCount + 1;
		if(ByteCount > maxlength - 1)
			return -1;
		
		Length = 0;	
		tmp = (unsigned char *) &Length;
		indexField = ByteCount;
		
		for(indexVar = 0; indexVar < ByteCount; indexVar++)
		{
			tmp[3-indexVar]=lengthfield[indexField--];
		}
	}
	else
		*fieldlen = 1;
	
	return Length;				
}

#if 1
void enigmaCImmi::getmmi(const char *data)
{
#define TAG_LENGTH	3
#define MAX_LENGTH_BYTES	4
	int fd=eSkin::getActive()->queryValue("fontsize", 8); //20

	const unsigned char TAG_MMI_MENU_LAST[]={0x9F,0x88,0x09};
	const unsigned char TAG_MMI_MENU_MORE[]={0x9F,0x88,0x0A};
	const unsigned char TAG_MMI_LIST_LAST[]={0x9F,0x88,0x0C};
	const unsigned char TAG_MMI_LIST_MORE[]={0x9F,0x88,0x0D};
	const unsigned char TAG_MMI_MENU_ANSW[]={0x9F,0x88,0x0B};
	const unsigned char TAG_MMI_TEXT_LAST[]={0x9F,0x88,0x03};
	const unsigned char TAG_MMI_TEXT_MORE[]={0x9F,0x88,0x04};
	const unsigned char TAG_MMI_ENQ[]      ={0x9F,0x88,0x07};

	int rp=5;
	eDebug("new mmi message received");
	for(int i=0;i<data[0];i++)
		printf("%02x ",data[i]);
	printf("\n");

	lentrys->clearList();		
	lentrys->hide();
	
	if(mmi)
		delete mmi;

	mmi=new eCImmi(this);
	mmi->move(ePoint(20, 100));
	mmi->resize(eSize(560,(fd+4)*6));
	mmi->show();

	if(memcmp(data+rp,TAG_MMI_ENQ,TAG_LENGTH)==0)
	{
		eDebug("mmi_enq_last");
		rp+=3;
		int LengthBytes;
		int size=LengthField((unsigned char*)data+rp, MAX_LENGTH_BYTES, &LengthBytes);
		rp += LengthBytes;

		int blind=data[rp++] & 1;		//blind_answer
		
		int nrcount=data[rp];
		if(nrcount>8)
			nrcount=8;
		
		int valinit[nrcount];
		memset(valinit,0,sizeof(valinit));		
		mmi->answer=new eNumber(mmi,nrcount,0,9,1,valinit,0,0);
		mmi->answer->move(ePoint(((560/2)-(20*(nrcount)/2)),30));
		mmi->answer->resize(eSize(20*(nrcount),fd+6));
		mmi->answer->setHelpText(_("mmi input field"));
		mmi->answer->loadDeco();
		if(blind)
			mmi->answer->setFlags(eNumber::flagHideInput);
		mmi->answer->show();
		rp++;		//answer text len

		char text[size-1];
		memset(text,0,size-1);
		memcpy(text,data+rp,size-2);
		eDebug("TEXT:%s",text);

		eLabel *answertext;
		answertext=new eLabel(mmi);
		answertext->setAlign(eTextPara::dirCenter);
		answertext->move(ePoint(20,0));
		answertext->resize(eSize(520,fd+4));
		answertext->setText(text);
		answertext->show();
		
		eButton *ok;
		ok=new eButton(mmi);
		ok->setText(_("OK"));
		ok->move(ePoint(235, 70));
		ok->resize(eSize(90, fd+4));
		ok->setHelpText(_("send data to ci"));
		ok->loadDeco();
		ok->show();

		CONNECT(ok->selected, enigmaCImmi::answokPressed);		

		rp+=size;
	}
	else
	
	if(memcmp(data+rp,TAG_MMI_MENU_LAST,TAG_LENGTH)==0 ||
		 memcmp(data+rp,TAG_MMI_LIST_LAST,TAG_LENGTH)==0)	
	{	
		eDebug("mmi_menu_last");
		lentrys->show();

		rp+=3;
		
		int LengthBytes;
		int size=LengthField((unsigned char*)data+rp, MAX_LENGTH_BYTES, &LengthBytes);
		
		rp += LengthBytes;
			
		unsigned char choices=data[rp++];
		eDebug("Size: %x Choices: %d",size,choices);
		
		int currElement=0;
		int endpos=rp+size;
		
		while(rp<endpos)	
		{
			if(memcmp(data+rp,TAG_MMI_TEXT_LAST,TAG_LENGTH)==0)	
			{
				eDebug("MMI_TEXT_LAST");	
				rp+=3;
				int LengthBytes;
				int size=LengthField((unsigned char*)data+rp, MAX_LENGTH_BYTES, &LengthBytes);
				rp += LengthBytes;
				
				char text[size+2];
				memset(text,0,size+2);
				memcpy(text+1,data+rp,size);
				text[0]=0x20;
				eDebug("TEXT:%s",text);
				currElement++;
				
				if(currElement==1)
					tt->setText(text);					
				if(currElement==2)
					stt->setText(text);					
				if(currElement==3)
					bt->setText(text);					
				
				if(currElement>3)
				{
					lentrys->beginAtomic();	
					eListBoxMenuEntry *e=new eListBoxMenuEntry(lentrys,text,currElement-3);
					lentrys->endAtomic();	
				}
				rp += size;
			}
			else	
			{
				eDebug("unknown MMI_TAG:%02x%02x%02x",data[rp],data[rp+1],data[rp+2]);
				rp+=3;
				int LengthBytes;
				int size=LengthField((unsigned char*)data+rp, MAX_LENGTH_BYTES, &LengthBytes);
		
				rp += LengthBytes + size;
			}
			if(rp>endpos)
				break;
		}
	}
	else if(memcmp(data+rp,TAG_MMI_MENU_MORE,TAG_LENGTH)==0)	
	{	
		eDebug("mmi_menu_more");
	}
}
#else
void enigmaCImmi::getmmi(const char *data)
{
	int entry=1;
	eDebug("new mmi message received");

	lentrys->clearList();		
	lentrys->hide();
	answ->hide();
	answok->hide();
	headansw->hide();
	
	for(int i=1;i<data[0];i++)
		printf("%02x ",data[i]);
	printf("\n");

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
#endif
enigmaCI::enigmaCI(): eWindow(0)
{
	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	DVBCI=eDVB::getInstance()->DVBCI;

	setText(_("Common Interface Module"));
	move(ePoint(150, 136));
	cresize(eSize(350, 220));

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

