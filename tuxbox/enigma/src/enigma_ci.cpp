#ifndef DISABLE_CI

#include <enigma_ci.h>
#include <enigma.h>
#include <lib/base/i18n.h>
#include <lib/driver/rc.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/dvbci.h>
#include <lib/gui/elabel.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/enumber.h>
#include <lib/gui/textinput.h>
#include <lib/gui/ewindow.h>
#include <lib/gui/statusbar.h>
#include <lib/gui/eskin.h>
#include <lib/gdi/font.h>
#include <lib/system/econfig.h>
#include <lib/system/info.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>

std::map<eDVBCI*,enigmaMMI*> enigmaMMI::exist;

enigmaCI::enigmaCI()
{
	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	DVBCI=eDVB::getInstance()->DVBCI;
	
	if( eSystemInfo::getInstance()->hasCI() > 1 )
	{
		setText(_("Common Interface Modules"));
		move(ePoint(160, 80));
		cresize(eSize(350, 310));
		DVBCI2=eDVB::getInstance()->DVBCI2;
	}
	else
	{
		setText(_("Common Interface Module"));
		move(ePoint(160, 136));
		cresize(eSize(350, 180));
	}

	reset=new eButton(this);
	reset->setText(_("Reset"));
	reset->move(ePoint(10, 13));
	reset->resize(eSize(330, fd+10));
	reset->setHelpText(_("reset the Common Interface module"));
	reset->loadDeco();

	CONNECT(reset->selected, enigmaCI::resetPressed);

	init=new eButton(this);
	init->setText(_("Init"));
	init->move(ePoint(10, 53));
	init->resize(eSize(330, fd+10));
	init->setHelpText(_("send the ca-pmt to CI"));
	init->loadDeco();

	CONNECT(init->selected, enigmaCI::initPressed);		

	app=new eButton(this);
	app->setText(_("waiting for module"));
	app->move(ePoint(10, 93));
	app->resize(eSize(330, fd+10));
	app->setHelpText(_("enter Common Interface menu (mmi)"));
	app->loadDeco();

	CONNECT(app->selected, enigmaCI::appPressed);		

	if( eSystemInfo::getInstance()->hasCI() > 1 )
	{
		reset2=new eButton(this);
		reset2->setText(_("Reset"));
		reset2->move(ePoint(10, 143));
		reset2->resize(eSize(330, fd+10));
		reset2->setHelpText(_("reset the Common Interface module"));
		reset2->loadDeco();

		CONNECT(reset2->selected, enigmaCI::reset2Pressed);		

		init2=new eButton(this);
		init2->setText(_("Init"));
		init2->move(ePoint(10, 183));
		init2->resize(eSize(330, fd+10));
		init2->setHelpText(_("send the ca-pmt to CI"));
		init2->loadDeco();

		CONNECT(init2->selected, enigmaCI::init2Pressed);		

		app2=new eButton(this);
		app2->setText(_("waiting for module"));
		app2->move(ePoint(10, 223));
		app2->resize(eSize(330, fd+10));
		app2->setHelpText(_("enter Common Interface menu (mmi)"));
		app2->loadDeco();

		CONNECT(app2->selected, enigmaCI::app2Pressed);		
	}

	status = new eStatusBar(this);	
	status->move( ePoint(0, clientrect.height()-30) );
	status->resize( eSize( clientrect.width(), 30) );
	status->loadDeco();

	CONNECT(DVBCI->ci_progress, enigmaCI::updateCIinfo);
	DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::init));

	if( eSystemInfo::getInstance()->hasCI() > 1 )
	{
		CONNECT(DVBCI2->ci_progress, enigmaCI::updateCI2info);
		DVBCI2->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::init));
	}
}

enigmaCI::~enigmaCI()
{
	if (status)
		delete status;
}

void enigmaCI::updateCIinfo(const char *buffer)
{
	eDebug("new info %s",buffer);
	app->setText(buffer);
}

void enigmaCI::updateCI2info(const char *buffer)
{
	eDebug("new info %s",buffer);
	app2->setText(buffer);
}

void enigmaCI::resetPressed()
{
	app->setText(_("resetting....please wait"));
	DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::reset));
}

void enigmaCI::reset2Pressed()
{
	app->setText(_("resetting....please wait"));
	DVBCI2->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::reset));
}

void enigmaCI::initPressed()
{
	DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::init));
}

void enigmaCI::init2Pressed()
{
	DVBCI2->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::init));
}

void enigmaCI::appPressed()
{
	hide();
	DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::mmi_begin));
	enigmaMMI::getInstance(DVBCI)->exec();
	DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::mmi_end));
	show();
}

void enigmaCI::app2Pressed()
{
	hide();
	DVBCI2->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::mmi_begin));
	enigmaMMI::getInstance(DVBCI2)->exec();
	DVBCI2->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::mmi_end));
	show();
}

#define TAG_LENGTH 3
#define MAX_LENGTH_BYTES 4

enigmaMMI::enigmaMMI(eDVBCI *ci)
	:eWindow(1), ci(ci), mmi_messages(eApp, 1), open(0),
	responseTimer(eApp), delayTimer(eApp), closeTimer(eApp)
{
	eDebug("[enigmaMMI] created successfully");
	cmove( ePoint(150,140) );
	cresize( eSize(450,100) );
	setText(_("Common Interface Module - mmi"));
	eSize csize = getClientSize();
	lText = new eLabel(this);
	lText->move(ePoint(10,10));
	lText->resize(eSize( csize.width(), 180 ));
	lText->setText(_("waiting for CI answer..."));
	lText->setAlign(eTextPara::dirCenter);
	int newHeight = size.height() - getClientSize().height() + lText->getExtend().height() + 10 + 20;
	resize( eSize( size.width(), newHeight ) );
	CONNECT( mmi_messages.recv_msg, enigmaMMI::handleMessage );
	CONNECT(responseTimer.timeout, eWidget::reject);
	CONNECT(delayTimer.timeout, enigmaMMI::haveScheduledData );
	CONNECT(closeTimer.timeout, enigmaMMI::closeMMI );
}

void enigmaMMI::handleMessage( const eMMIMsg &msg )
{
	if ( handleMMIMessage( msg.data ) )
		delete [] msg.data;
}

void enigmaMMI::gotMMIData( const char* data, int len )
{
	char *dest = new char[len];
	memcpy( dest, data, len );
	mmi_messages.send( eMMIMsg( dest, len ) );
}

int enigmaMMI::eventHandler( const eWidgetEvent &e )
{
	switch (e.type)
	{
		case eWidgetEvent::execBegin:
			show();
			responseTimer.start(30000);
			mmi_messages.start();
			conn = CONNECT(ci->ci_mmi_progress, enigmaMMI::gotMMIData );
			return 1;
		case eWidgetEvent::execDone:
			hide();
			conn.disconnect();
			responseTimer.stop();
			return 1;
		default:
			break;
	}
	return eWindow::eventHandler(e);
}

enigmaMMI* enigmaMMI::getInstance( eDVBCI* ci )
{
	std::map<eDVBCI*, enigmaMMI*>::iterator it = exist.find(ci);
	if ( it == exist.end() )
		exist[ci]=new enigmaMMI(ci);
	return exist[ci];
}

long LengthField(unsigned char *lengthfield,long maxlength,int *fieldlen)
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

void enigmaMMI::showWaitForCIAnswer(int ret)
{
	if ( ret != -1 )
	{
		if ( conn.connected() )
		{
			show();
			responseTimer.start(10000,true);
		}
	}
}

void enigmaMMI::hideWaitForCIAnswer()
{
	if ( conn.connected() )
	{
		hide();
		responseTimer.stop();
	}
}

void enigmaMMI::haveScheduledData()
{
	// must create a copy of the pointer on lokal stack !!
	const char * data = scheduledData;
	if ( handleMMIMessage(data) )
		delete [] data;
}

void enigmaMMI::closeMMI()
{
//	eDebug("closeMMI");
	if ( open )
	{
		open->hide();
		open->close(-2);
//		eDebug("open->hide()\nopen->close(-2)");
		if ( conn.connected() )
		{
//			eDebug("conn is connected... start closeTimer");
			closeTimer.start(2,true);
		}
	}
	else if ( conn.connected() )
	{
//		eDebug("close(0)");
		close(0);
	}
}

bool enigmaMMI::handleMMIMessage(const char *data)
{
	const unsigned char TAG_MMI_CLOSE[]={0x9F,0x88,0x00};
	const unsigned char TAG_MMI_DISPLAY_CONTROL[]={0x9F,0x88,0x01};
	const unsigned char TAG_MMI_TEXT_LAST[]={0x9F,0x88,0x03};
	const unsigned char TAG_MMI_TEXT_MORE[]={0x9F,0x88,0x04};

	const unsigned char TAG_MMI_ENQ[]      ={0x9F,0x88,0x07};

	const unsigned char TAG_MMI_MENU_LAST[]={0x9F,0x88,0x09};
	const unsigned char TAG_MMI_MENU_MORE[]={0x9F,0x88,0x0A};

	const unsigned char TAG_MMI_LIST_LAST[]={0x9F,0x88,0x0C};
	const unsigned char TAG_MMI_LIST_MORE[]={0x9F,0x88,0x0D};

	int rp=0;

	while ( data[rp] != 0x9F || data[rp+1] != 0x88 )
		rp++;

	if( !memcmp(data+rp,TAG_MMI_CLOSE,TAG_LENGTH) )
	{
		rp += 3;
		if ( *(data+rp) ) // timeout is set
		{
			int delay = *(data+rp+1);
			delay = delay ? delay * 1000 : 1;
//			eDebug("start closeTimer %d", delay );
			closeTimer.start( delay, true );
		}
		else
			closeMMI();
	}
	else if( !memcmp(data+rp,TAG_MMI_ENQ,TAG_LENGTH) )
	{
		eDebug("mmi_enq_last");
		closeTimer.stop();

		if ( open )
		{
			open->hide();
			open->close(-2);
			// we must delay executing of the next mmi window while
			// the open mmi window is still executed... open->close()
			// set only the app_exit_loop boolean in the mainloop... but
			// this takes not effect while the mainloop is busy...

			scheduledData = data;
			delayTimer.start(2,true);
			return false;
		}

		rp+=3;

		int LengthBytes;
		int size=LengthField((unsigned char*)data+rp, MAX_LENGTH_BYTES, &LengthBytes);

		rp+=LengthBytes;

		int blind=data[rp++] & 1;  //blind_answer
		eDebug("blind = %d", blind );

		uint nrcount=data[rp++];
		if ( nrcount > 32 )
			nrcount = 32;

		char text[size-1];
		memset(text,0,size-1);
		memcpy(text,data+rp,size-2);

		eDebug("TEXT:%s",text);

		eMMIEnqWindow wnd(text, nrcount, blind );
		open = &wnd;
		int ret = wnd.exec();
		open = 0;

		unsigned char buf[ret == -1 ? 2 : 2 + nrcount];
		buf[1] = ret == -1 ? 0 : 1; // answer ok.. or user canceled
		buf[0] = buf[1] ? nrcount : 1;  // length
		// when user have cancelled only one byte is answered to the ci

		eString atext = wnd.getAnswer();  // get Answer from number

		for (uint i=0; i < nrcount; ++i )  // copy user input to answer
			buf[2+i] = atext[i];

		ci->messages.send( eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::mmi_enqansw, buf));

		showWaitForCIAnswer(ret);

		rp+=size;
	}
	else if( memcmp(data+rp,TAG_MMI_MENU_LAST,TAG_LENGTH)==0 ||
		 memcmp(data+rp,TAG_MMI_LIST_LAST,TAG_LENGTH)==0)
	{
		eDebug("mmi_menu_last");
		closeTimer.stop();
		if ( open )
		{
			open->hide();
			open->close(-2);
			// we must delay executing of the next mmi window while
			// the open mmi window is still executed... open->close()
			// set only the app_exit_loop boolean in the mainloop... but
			// this takes not effect while the mainloop is busy...

			scheduledData = data;
			delayTimer.start(2,true);
			return false;
		}

		rp+=3;

		int LengthBytes;
		int size=LengthField((unsigned char*)data+rp, MAX_LENGTH_BYTES, &LengthBytes);

		rp += LengthBytes;

		unsigned char choices=data[rp++];

		eDebug("Size: %x Choices: %d",size,choices);

		int currElement=0;
		int endpos=rp+size;

		eString titleText, subTitleText, bottomText;
		std::list< std::pair<eString, int> > entrys;
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
					titleText=text;
				if(currElement==2)
					subTitleText=text;
				if(currElement==3)
					bottomText=text;

				if(currElement>3)
				{
					eDebug("new entry text %s", text);
					entrys.push_back( std::pair<eString, int>( text, currElement-3 ) );
				}
				rp += size;
			}
			else if(memcmp(data+rp,TAG_MMI_TEXT_MORE,TAG_LENGTH)==0)
				eDebug("mmi_text_more.. unhandled yet");
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
		hideWaitForCIAnswer();
		eMMIListWindow wnd(titleText, subTitleText, bottomText, entrys );
		open = &wnd;
		int ret = wnd.exec();
		open = 0;
		eDebug("ret = %d",ret);
		if ( ret > -2 )
		{
			if ( ret == -1 )
				ci->messages.send( eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::mmi_menuansw,0));
			else
				ci->messages.send( eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::mmi_menuansw,wnd.getSelected()));
			showWaitForCIAnswer(ret);
		}
	}
	else if(!memcmp(data+rp,TAG_MMI_MENU_MORE,TAG_LENGTH))
		eDebug("mmi_menu_more.. unhandled yet");
	else if(!memcmp(data+rp,TAG_MMI_LIST_MORE,TAG_LENGTH))
		eDebug("mmi_list_more.. unhandled yet");
	else if(!memcmp(data+rp,TAG_MMI_DISPLAY_CONTROL,TAG_LENGTH))
		eDebug("DISPLAY CONTROL .. still answered in dvbci");
	else
	{
		eDebug("unknown MMI_TAG:%02x%02x%02x",data[rp],data[rp+1],data[rp+2]);
		eDebug("CLOSE");
		closeMMI();
	}

	return true;
}

eString &removeSpaces( eString &s )
{
	unsigned int length = s.length();
	while ( length && s[0] == 32 )
	{
		s.erase(0,1);
		--length;
	}
	while ( length && s[length-1] == 32 )
		s.erase(--length,1);
	return s;
}

eMMIEnqWindow::eMMIEnqWindow( eString text, int num, bool blind )
	:num(num)
{
	cmove( ePoint(120,140) );
	cresize( eSize(520,250) );
	setText(_("Common Interface Module - mmi"));

	int valinit[num];
	memset(valinit,0,sizeof(valinit));
	input=new eNumber(this,num,0,9,1,valinit,0,0);
	input->move(ePoint(((520/2)-(15*(num)/2)),10));
	input->resize(eSize(15*(num),35));
	input->setHelpText(_("input all digits or press ok to send data"));
	input->loadDeco();
	if(blind)
		input->setFlags(eNumber::flagHideInput);

	int newHeight = size.height() - getClientSize().height() + 100;

	if ( removeSpaces(text) )
	{
		newHeight+=10;
		title = new eLabel(this);
		title->setAlign(eTextPara::dirCenter);
		title->move(ePoint(0,10));
		title->resize(eSize(getClientSize().width(), 100));
		title->setText(text);
		eSize size = title->getSize();
		size.setHeight( title->getExtend().height()+10) ;
		title->resize( size );
		newHeight += size.height();

		ePoint pos = input->getPosition();
		pos += ePoint( 0, 10 + size.height() );
		input->move( pos );
	}

	resize( eSize( getSize().width(), newHeight ) );

	eStatusBar *statusbar=new eStatusBar(this);
	statusbar->move( ePoint(0, clientrect.height()-30 ) );
	statusbar->resize( eSize( clientrect.width(), 30) );
	statusbar->loadDeco();
	CONNECT( input->selected, eMMIEnqWindow::okPressed );
}

void eMMIEnqWindow::okPressed(int*)
{
	accept();
}

int eMMIEnqWindow::eventHandler( const eWidgetEvent &e )
{
	switch (e.type)
	{
		case eWidgetEvent::execBegin:
			show();
			return 1;
		case eWidgetEvent::execDone:
			hide();
			return 1;
		default:
			break;
	}
	return eWindow::eventHandler(e);
}

eString eMMIEnqWindow::getAnswer()
{
	static eString ret;
	ret="";
	for ( int i=0; i < num; i++ )
		ret += (char)(input->getNumber( i )+0x30);
	return ret;
}

eMMIListWindow::eMMIListWindow(eString titleTextT, eString subtitleTextT, eString bottomTextT, std::list< std::pair< eString, int> > &entrys )
	:eListBoxWindow<eListBoxEntryText>(_("Common Interface Module - mmi"), entrys.size() > 8 ? 8 : entrys.size() , 520, false)
{
	cmove(ePoint(120, 130));

	for ( std::list< std::pair<eString,int> >::iterator it( entrys.begin() ); it != entrys.end(); ++it )
		new eListBoxEntryText( &list, removeSpaces(it->first), (void*) it->second );

	int newHeight = height();

	if ( removeSpaces(titleTextT) )
	{
		newHeight+=10;
		title = new eLabel(this);
		title->setFlags( RS_WRAP );
		title->setAlign(eTextPara::dirCenter);
		title->move(ePoint(0,10));
		title->resize(eSize(getClientSize().width(), 100));
		title->setText(titleTextT);
		eSize size = title->getSize();
		size.setHeight( title->getExtend().height()+10) ;
		title->resize( size );
		newHeight += size.height();

		ePoint pos = list.getPosition();
		pos += ePoint( 0, 10 + size.height() );
		list.move( pos );
	}

	if ( removeSpaces(subtitleTextT) )
	{
		newHeight+=10;
		subtitle = new eLabel(this);
		subtitle->setFlags( RS_WRAP );
		subtitle->setAlign(eTextPara::dirCenter);
		if ( title )
		{
			ePoint pos = title->getPosition();
			pos+=ePoint(0,title->getSize().height() );
			subtitle->move(pos);
		}
		else
			subtitle->move(ePoint(0,10));
		subtitle->resize(eSize(getClientSize().width(), 100));
		subtitle->setText(subtitleTextT);
		eSize size = subtitle->getSize();
		size.setHeight( subtitle->getExtend().height()+10) ;
		subtitle->resize( size );
		newHeight += size.height();

		ePoint pos = list.getPosition();
		pos += ePoint( 0, 10 + size.height() );
		list.move( pos );
	}

	if ( removeSpaces(bottomTextT) )
	{
		newHeight += 10;
		bottomText = new eLabel(this);
		bottomText->setFlags( RS_WRAP );
		bottomText->setAlign(eTextPara::dirCenter);
		bottomText->move(ePoint(0,list.getPosition().y()+list.getSize().height()+10));
		bottomText->resize(eSize(getClientSize().width(), 100));
		bottomText->setText(bottomTextT);
		eSize size = bottomText->getSize();
		size.setHeight( bottomText->getExtend().height()+10 ) ;
		bottomText->resize( size );
		newHeight+=size.height();
	}

	if ( titleTextT || bottomTextT || subtitleTextT )
		resize( eSize( width, newHeight + 10 ));

	CONNECT( list.selected, eMMIListWindow::entrySelected );
}

void eMMIListWindow::entrySelected( eListBoxEntryText *e )
{
	close(e?0:-1);
}

int eMMIListWindow::eventHandler( const eWidgetEvent &e )
{
	switch (e.type)
	{
		case eWidgetEvent::execBegin:
			show();
			return 1;
		case eWidgetEvent::execDone:
			hide();
			return 1;
		default:
			break;
	}
	return eWindow::eventHandler(e);
}

#endif // DISABLE_CI
