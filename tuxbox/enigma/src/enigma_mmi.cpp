#include <src/enigma_mmi.h>
#include <src/enigma_main.h>
#include <lib/gui/elabel.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/enumber.h>
#include <lib/gui/ewindow.h>
#include <lib/gui/statusbar.h>
#include <lib/gui/eskin.h>
#include <lib/gdi/font.h>

#define TAG_LENGTH 3
#define MAX_LENGTH_BYTES 4

enigmaMMI::enigmaMMI()
	:eWindow(1), mmi_messages(eApp, 1), open(0),
	responseTimer(eApp), delayTimer(eApp), closeTimer(eApp)
{
	eDebug("[enigmaMMI] created successfully");
	cmove( ePoint(150,140) );
	cresize( eSize(450,100) );
	eSize csize = getClientSize();
	lText = new eLabel(this);
	lText->move(ePoint(10,10));
	lText->resize(eSize( csize.width(), 180 ));
	lText->setAlign(eTextPara::dirCenter);
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
/*	for (int i=0; i < len; i++)
		eDebugNoNewLine("%x ", data[i]);
	eDebug("");*/
	mmi_messages.send( eMMIMsg( dest, len ) );
}

int enigmaMMI::eventHandler( const eWidgetEvent &e )
{
	switch (e.type)
	{
		case eWidgetEvent::execBegin:
			show();
			mmi_messages.start();
			beginExec();
			return 1;
		case eWidgetEvent::execDone:
			hide();
			conn.disconnect();
			responseTimer.stop();
			endExec();
			return 1;
		default:
			break;
	}
	return eWindow::eventHandler(e);
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

void enigmaMMI::showWaitForAnswer(int ret)
{
//	if ( ret != -1 )
	{
		if ( conn.connected() )
		{
			show();
			responseTimer.start(5000,true);
		}
	}
}

void enigmaMMI::hideWaitForAnswer()
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
			closeTimer.start(0,true);
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
		int LengthBytes;
		int size=LengthField((unsigned char*)data+rp, MAX_LENGTH_BYTES, &LengthBytes);
		rp+=LengthBytes;
		if ( *(data+rp) == 1 ) // timeout is set
		{
			if ( size > 1 )
			{
				eDebug("timed mmi_close");
				int delay = *(data+rp+1);
				delay = delay ? delay * 1000 : 1;
//			eDebug("start closeTimer %d", delay );
				closeTimer.start( delay, true );
			}
			else
			{
				eDebug("invalid close tag... 9f 88 00 LEN 01 .. but no timeout is given!!");
				closeMMI();
			}
		}
		else
		{
			eDebug("immediate mmi_close");
			closeMMI();
		}
		rp+=size;
	}
	else if( !memcmp(data+rp,TAG_MMI_ENQ,TAG_LENGTH) )
	{
		eDebug("mmi_enq_last");
		closeTimer.stop();

		if ( open )
		{
			open->hide();
			open->close(-2);
			// we must to delay exec'uting the next mmi window while
			// the open mmi window is still in execution... open->close()
			// set only the app_exit_loop boolean in the mainloop... but
			// this takes no effect while the mainloop is busy...
			scheduledData = data;
			delayTimer.start(0,true);
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

//		eDebug("TEXT:%s",text);
		hideWaitForAnswer();
		eMMIEnqWindow wnd(this->text, text, nrcount, blind );
		open = &wnd;
		int ret = wnd.exec();
		open = 0;

		unsigned char *buf = new unsigned char[ret == -1 ? 2 : 2 + nrcount];

		buf[1] = ret == -1 ? 0 : 1; // answer ok.. or user canceled

		buf[0] = buf[1] ? nrcount+1 : 1;  // length

		// when user have cancelled only one byte is answered to the ci

		eString atext = wnd.getAnswer();  // get Answer from number

		for (int i=0; i < buf[0]-1; ++i )  // copy user input to answer
			buf[2+i] = atext[i];

		sendAnswer( ENQAnswer, 815, buf );

		showWaitForAnswer(ret);
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
			// we must to delay exec'uting the next mmi window while
			// the open mmi window is still in execution... open->close()
			// set only the app_exit_loop boolean in the mainloop... but
			// this takes no effect while the mainloop is busy...
			scheduledData = data;
			delayTimer.start(0,true);
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

				char text[size+1];
				memset(text,0,size+1);
				memcpy(text,data+rp,size);
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
		hideWaitForAnswer();
		eMMIListWindow wnd(text, titleText, subTitleText, bottomText, entrys );
		open = &wnd;
		int ret = wnd.exec();
		open = 0;
		if ( ret > -2 )
		{
			if ( ret == -1 )
				sendAnswer( LISTAnswer, 0, 0 );
			else
				sendAnswer( LISTAnswer, wnd.getSelected(), 0 );
			showWaitForAnswer(ret);
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

eMMIEnqWindow::eMMIEnqWindow( const eString &titleBarText, const eString &text, int num, bool blind )
	:num(num)
{
	cmove( ePoint(100,140) );
	cresize( eSize(520,280) );
	setText(titleBarText);

	int valinit[num];
	memset(valinit,0,sizeof(valinit));
	input=new eNumber(this,num,0,9,1,valinit,0,0);
	input->move(ePoint(((520/2)-(15*(num)/2)),10));
	input->resize(eSize(15*(num),35));
	input->setHelpText(_("input all digits or press ok to send data"));
	input->loadDeco();
	if(blind)
		input->setFlags(eNumber::flagHideInput);

	int newHeight = size.height() - getClientSize().height() + 115;

	if ( text )
	{
		newHeight+=10;
		title = new eLabel(this);
		title->move(ePoint(0,10));
		title->resize(eSize(getClientSize().width(), 100));
		title->setText(text);
		title->setAlign(eTextPara::dirCenter);
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
	statusbar->move( ePoint(0, clientrect.height()-50 ) );
	statusbar->resize( eSize( clientrect.width(), 50) );
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
	eString ret;
	ret="";
	for ( int i=0; i < num; i++ )
		ret += (char)(input->getNumber( i )+0x30);
//	eDebug("ret = %s", ret.c_str() );
	return ret;
}

eMMIListWindow::eMMIListWindow(const eString & titleBarText, const eString &titleTextT, const eString &subtitleTextT, const eString &bottomTextT, std::list< std::pair< eString, int> > &entrys )
	:eListBoxWindow<eListBoxEntryText>(titleBarText, entrys.size() > 8 ? 8 : entrys.size() , 520, false)
{
	cmove(ePoint(120, 130));

	for ( std::list< std::pair<eString,int> >::iterator it( entrys.begin() ); it != entrys.end(); ++it )
		new eListBoxEntryText( &list, it->first, (void*) it->second );

	int newHeight = height();

	if ( titleTextT )
	{
		newHeight+=10;
		title = new eLabel(this);
		title->setFlags( RS_WRAP );
		title->move(ePoint(0,10));
		title->resize(eSize(getClientSize().width(), 100));
		title->setText(titleTextT);
		title->setAlign(eTextPara::dirCenter);
		eSize size = title->getSize();
		size.setHeight( title->getExtend().height()+10) ;
		title->resize( size );
		newHeight += size.height();

		ePoint pos = list.getPosition();
		pos += ePoint( 0, 10 + size.height() );
		list.move( pos );
	}

	if ( subtitleTextT )
	{
		newHeight+=10;
		subtitle = new eLabel(this);
		subtitle->setFlags( RS_WRAP );
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
		subtitle->setAlign(eTextPara::dirCenter);
		eSize size = subtitle->getSize();
		size.setHeight( subtitle->getExtend().height()+10) ;
		subtitle->resize( size );
		newHeight += size.height();

		ePoint pos = list.getPosition();
		pos += ePoint( 0, 10 + size.height() );
		list.move( pos );
	}

	if ( bottomTextT )
	{
		newHeight += 10;
		bottomText = new eLabel(this);
		bottomText->setFlags( RS_WRAP );
		bottomText->move(ePoint(0,list.getPosition().y()+list.getSize().height()+10));
		bottomText->resize(eSize(getClientSize().width(), 100));
		bottomText->setText(bottomTextT);
		bottomText->setAlign(eTextPara::dirCenter);
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

