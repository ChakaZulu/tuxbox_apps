#include <enigma_mmisocket.h>
#include <enigma.h>
#include <unistd.h>
#include <string.h>
#include <lib/base/eerror.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/gdi/font.h>

int eSocketMMIHandler::send_to_mmisock( void* buf, size_t len)
{
	int ret = write(connfd, buf, len);
	if ( ret < 0 )
		eDebug("[eSocketMMIHandler] write (%m)");
	else if ( (uint)ret != len )
		eDebug("[eSocketMMIHandler] only %d bytes sent.. %d bytes should be sent", ret, len );
	else
		return 0;
	return ret;
}

eSocketMMIHandler::eSocketMMIHandler()
	:connfd(-1), connsn(0), sockname("/tmp/mmi.socket"), name(0)
{
	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	unlink(sockname);
	strcpy(servaddr.sun_path, sockname);
	clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);
	if ((listenfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		eDebug("[eSocketMMIHandler] socket (%m)");
		return;
	}
	if (bind(listenfd, (struct sockaddr *) &servaddr, clilen) < 0)
	{
		eDebug("[eSocketMMIHandler] bind (%m)");
		return;
	}
	if (listen(listenfd, 5) < 0)
	{
		eDebug("[eSocketMMIHandler] listen (%m)");
		return;
	}
	listensn = new eSocketNotifier( eApp, listenfd, POLLIN ); // POLLIN
	listensn->start();
	CONNECT( listensn->activated, eSocketMMIHandler::listenDataAvail );
	eDebug("[eSocketMMIHandler] created successfully");
	CONNECT( eZapSetup::setupHook, eSocketMMIHandler::setupOpened );
}

void eSocketMMIHandler::setupOpened( eSetupWindow* setup, int *entrynum )
{
	if ( !connsn )  // no mmi connection...
		return;
	eListBox<eListBoxEntryMenu> *list = setup->getList();
	CONNECT((
			new eListBoxEntryMenu(list,
				eString().sprintf("%s Menu", name).c_str(),
				eString().sprintf("(%d) open %s menu", ++(*entrynum), name)
				))->selected, eSocketMMIHandler::initiateMMI);
}

#define CMD_SET_NAME "\x01\x02\x03\x04"

void eSocketMMIHandler::listenDataAvail(int what)
{
	switch(what)
	{
		case POLLIN:
		{
			if ( connsn )
				return;
			char msgbuffer[256];
			connfd=accept(listenfd, (struct sockaddr *) &servaddr, (socklen_t *) &clilen);
			ssize_t length = read(connfd, msgbuffer, sizeof(msgbuffer));
//			eDebug("got %d bytes", length);
			if ( !length || ( length == -1 && errno != EAGAIN ) )
			{
				eDebug("Connection error.. close connection");
				closeConn();
			}
			else if ( (length < 4) || strncmp(msgbuffer, CMD_SET_NAME, 4) )
			{
				eDebug("CMD_SET_NAME not found.. close connection");
				closeConn();
				break;
			}
			length-=4;
			if ( !length )
			{
				eDebug("CMD_SET_NAME failed.. no more characters for name left");
				closeConn();
				break;
			}
			name = new char[length+1];
			memcpy(name, msgbuffer+4, length);
			name[length]=0;
			fcntl(connfd, F_SETFL, O_NONBLOCK);
			int val=1;
			setsockopt(connfd, SOL_SOCKET, SO_REUSEADDR, &val, 4);
			connsn = new eSocketNotifier( eApp, connfd, POLLIN|POLLHUP|POLLERR );
			CONNECT( connsn->activated, eSocketMMIHandler::connDataAvail );
		}
	}
}

void eSocketMMIHandler::connDataAvail(int what)
{
	switch(what)
	{
		case POLLIN:
		case POLLPRI:
		{
			char msgbuffer[4096];
			ssize_t length = read(connfd, msgbuffer, sizeof(msgbuffer));
			if ( !length || ( length == -1 && errno != EAGAIN ) )
			{
				closeConn();
				break;
			}
			if ( eSocketMMI::getInstance(this)->connected() )
				eSocketMMI::getInstance(this)->gotMMIData(msgbuffer, length);
			else
			{
/*				eServiceHandler *handler = eServiceInterface::getInstance()->getService();
				if ( handler && handler->getFlags() & eServiceHandler::flagIsScrambled && */
//				if ( eApp->looplevel() == 1 && ( !currentFocus || currentFocus == eZapMain::getInstance() ) )
				{
					if ( eZapMain::getInstance()->isVisible() )
						eZapMain::getInstance()->hide();
					eSocketMMI::getInstance(this)->gotMMIData(msgbuffer, length);
				}
			}
			break;
		}
		default:
		case POLLERR:
		case POLLHUP:
			closeConn();
			eSocketMMI::getInstance(this)->gotMMIData("\x9f\x88\x00\x00",4);
			break;
	}
}

void eSocketMMIHandler::closeConn()
{
	if ( connfd != -1 )
	{
		close(connfd);
		connfd=-1;
	}
	if ( connsn )
	{
		delete connsn;
		connsn=0;
	}
	if ( name )
	{
		delete [] name;
		name=0;
	}
}

eSocketMMIHandler::~eSocketMMIHandler()
{
	closeConn();
	delete listensn;
	unlink(sockname);
}

void eSocketMMIHandler::initiateMMI()
{
	int ret=0;
	{
		unsigned char buf[]="\x9F\x88\x01";  // DISPLAY CONTROL
		ret = send_to_mmisock(buf, 3);
	}
	if ( ret )
		return;
	eWindow *setup = (eWindow*)currentFocus;
	setup->hide();
	eSocketMMI::getInstance(this)->exec();
	{
		unsigned char buf[]="\x9F\x88\x00";  // CLOSE MMI
		send_to_mmisock(buf, 3);
	}
	setup->show();
}

extern long LengthField(unsigned char *lengthfield,long maxlength,int *fieldlen);

std::map<eSocketMMIHandler*,eSocketMMI*> eSocketMMI::exist;

eSocketMMI* eSocketMMI::getInstance( eSocketMMIHandler *handler  )
{
	std::map<eSocketMMIHandler*, eSocketMMI*>::iterator it = exist.find(handler);
	if ( it == exist.end() )
		exist[handler]=new eSocketMMI(handler);
	return exist[handler];
}

eSocketMMI::eSocketMMI( eSocketMMIHandler *handler )
	:handler(handler)
{
	setText(eString().sprintf("%s - mmi", handler->getName()));
	lText->setText(eString().sprintf("waiting for %s answer...", handler->getName()));
	int newHeight = size.height() - getClientSize().height() + lText->getExtend().height() + 10 + 20;
	resize( eSize( size.width(), newHeight ) );
}

void eSocketMMI::sendAnswer( AnswerType ans, int param, unsigned char *data )
{
	switch(ans)
	{
		case ENQAnswer:
		{
			unsigned char buffer[ data[0]+4 ];
			memcpy(buffer,"x9f\x88\x08", 3);
			memcpy(buffer+3, data, data[0]+1 );
			for (int i=0; i < data[0]+4; i++ )
				eDebugNoNewLine("%02x ", buffer[i]);
			eDebug("");
			handler->send_to_mmisock( buffer, data[0]+4 );
			break;
		}
		case LISTAnswer:
		case MENUAnswer:
		{
			unsigned char buffer[5];
			memcpy(buffer,"\x9f\x88\x0B\x1",4);
			buffer[4]=param&0xff;
			handler->send_to_mmisock( buffer, 5 );
			break;
		}
	}
}

void eSocketMMI::beginExec()
{
	conn = CONNECT(handler->mmi_progress, enigmaMMI::gotMMIData );
}

eAutoInitP0<eSocketMMIHandler> init_eSocketMMIHandler(eAutoInitNumbers::osd-2, "socket mmi handler");
