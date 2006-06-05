/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <iostream> 
#include <lcdd.h>
#include <file.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>

#include <radiobox.h>
#include <playlist.h>
#include <statehandler.h>

#define DEFGLOB
#include <global.h>
__setup globals;


#define FONTDIR "/share/fonts"
#define PLAYLISTDIR "/var/"




extern int errno;

const char* predefined_lcd_font[2][6] = 
{
	{FONTDIR "/12.pcf.gz", "Fix12", FONTDIR "/14B.pcf.gz", "Fix14", FONTDIR "/15B.pcf.gz", "Fix15"},
	{FONTDIR "/md_khmurabi_10.ttf", "MD King KhammuRabi", NULL, NULL,  NULL, NULL}
};

CRadioBox* CRadioBox::GetInstance()
{
	static CRadioBox* instance = NULL;

	if( instance == NULL )
	{
		instance = new CRadioBox();
	}
	return instance;
}

void CRadioBox::PushHandler( CStateHandler* _handler )
{
	handlers.push_back( _handler );

	std::cout << "after push stackhandler size = " << handlers.size() << std::endl;
}


CStateHandler* CRadioBox::PopHandler()
{
	std::cout << "before pop stackhandler size = " << handlers.size() << std::endl;


	if( 0 == handlers.size() )
		return NULL;

	CStateHandler* hdl = handlers.back();
	
	handlers.pop_back();

	return hdl;
}


CRadioBox::CRadioBox()
{
	this->audioplayer = CAudioPlayer::getInstance();
	this->audioplayer->init();
	
	statehandler = new CMainMenu();

	std::cout << "Open lirc" << std::endl;

	OpenLircd();
}

void CRadioBox::Run()
{

	//INITIALIZATION
	CLCD::getInstance()->init(
					predefined_lcd_font[0][0], 
					predefined_lcd_font[0][1],
					predefined_lcd_font[0][2],
					predefined_lcd_font[0][3],
					predefined_lcd_font[0][4],
					predefined_lcd_font[0][5] );	

	std::cout << "run" << std::endl;

	int divider = 0;

	while( 1 )
	{
		
		ReadKeys();

		if( NULL == statehandler )
			throw "No state handler in slot!";

		if( divider++ == 1 )
		{
			statehandler->Show();
			divider = 0;
		}
		
		if( POWER == key && false == keypressed )
		{
			CLCD::getInstance()->setMode( CLCD::MODE_SHUTDOWN, "Shutdown" );
			break;
		}
		
	
		statehandler->HandleKeys( key, keypressed );

		CStateHandler* tmp = statehandler->GetSubHandler();

		if( tmp )
		{
			std::cout << "push to stak handler [" << statehandler << "]" << std::endl;
			PushHandler( statehandler );
			statehandler = tmp;
			std::cout << "new handler [" << statehandler << "]" << std::endl;
		}

		if( statehandler->HasToBeRemoved() )
		{
			std::cout << "remove handler [" << statehandler << "]" << std::endl;
			delete statehandler;

			statehandler = PopHandler();

			std::cout << "get last handler handler [" << statehandler << "]" << std::endl;

			if( NULL == statehandler )
			{
				std::cout << "No more statehandlers in stack!" << std::endl;
			}
		}
	}	
}


void CRadioBox::ReadKeys()
{
	static KEYS lastkey = NOKEY;

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	ReadFromLircd();


	if( NOKEY != this->key )
	{
		keypressed = true;
		lastkey = this->key;
	}
	else
	{
		if( NOKEY != lastkey )
		{
			this->key = lastkey;
			keypressed = false;
		}
		lastkey = NOKEY;
	}

	rcinput.getMsg(&msg, &data, 1 );
		
	if( msg != CRCInput::RC_nokey && msg != CRCInput::RC_timeout )
	{
		this->key = TranslateKey( (int)msg );
		this->keypressed = !data;
	}
}

void CRadioBox::ReadFromLircd()
{
	if( -1 == lircd ) return;
		
	fd_set		read_sock;
	struct 		timeval	tv;
	int 		rc = 0;

	key = NOKEY;

	tv.tv_sec	= 0;
	tv.tv_usec	= 0;

	FD_ZERO( &read_sock );
	FD_SET( lircd, &read_sock );

	rc = select( lircd + 1, &read_sock, NULL, NULL, &tv );

	if( 0 == rc )
	{
		return; // nothing  to read from lircd
	}

	if( -1 == rc )
	{
		//something wrong, don't read from lircd anymore
		close( lircd );
		lircd = -1;
		return;
	}
	
	std::string	instring;
	char		buffer[128];

//	std::cout << "read data from socket" << std::endl;

	while( 0 < ( rc = recv( lircd, buffer, 128, 0 ) ) )
	{
		int i;

		for( i = 0; i < rc; i++ )
		{
			instring += buffer[i];
		}
	
		if( rc < 128 ) break;
	}

	if( 0 == rc )
	{
		//lircd closed connection or some problems, don't read any more from lircd

		close( lircd );
		lircd = -1;
		return;
	}
	else
	if( rc < 0 && errno != EAGAIN )
	{
		std::cout << "Error on lircd socket while reading" << std::endl;
		close( lircd );
		lircd = -1;
		return;
	}
	
	if( 0 == instring.size() )
		return;

	std::cout << "Data read from lircd:" << std::endl << instring << std::endl;

//	Parse strings

	int i = 0;
	int cursor = 0;
	int part = 0;
	std::string::size_type	start = 0;
	std::string::size_type	end = 0;

	while( end < instring.size() )
	{
		for( int part = 0; part < 4; part++ )
		{
			std::string sub;
			std::string delim = (part == 3)?"\n":" ";

			end = instring.find( delim, start );
			sub = instring.substr( start, end - start );
			
//			std::cout << "part: " << part << std::endl;

			switch( part )
			{
				case 0: /* internal code */
				case 1: /* repeat count */
				case 3: /* rc name */
					break;
				case 2: /* button name */
					this->key = TranslateKey( sub );
//					std::cout << sub << " translated = " << this->key << std::endl;
					break;
			}

			start = ++end;			
		}

	}

}

void CRadioBox::OpenLircd()
{
	struct sockaddr_un addr;

	addr.sun_family=AF_UNIX;
	strcpy(addr.sun_path, "/dev/lircd");
	lircd = socket(AF_UNIX,SOCK_STREAM,0);
	if( -1 == lircd )
	{
		std::cout << "couldn't open lircd" << std::endl;
		return;
	};

	long opts = fcntl( lircd, F_GETFL );

	if( opts <  0 )  
	{
		std::cout << "couldn't get sock options " << std::endl;
		lircd = -1;
		return;
	}
			
	opts |= O_NONBLOCK;
			
	if( 0 > fcntl( lircd, F_SETFL, opts ) ) 
	{
		std::cout << "couldn't set sock options " << std::endl;
		lircd = -1;
		return;
	}

	if( connect(lircd,(struct sockaddr *)&addr,sizeof(addr))==-1 )
	{
		std::cout << "error lircd connecting" << std::endl;
		lircd = -1;
		return;
	};



	return;	
}

CRadioBox::KEYS CRadioBox::TranslateKey( std::string _key )
{
	if( _key == "POWER" )
		return POWER;
	else
	if( _key == "OPEN" )
		return OPEN;
	else
	if( _key == "MENU" )
		return MENU;
	else
	if( _key == "TITLE" )
		return TITLE;
	else
	if( _key == "DISPLAY" )
		return DISPLAY;
	else
	if( _key == "SELECT" )
		return SELECT;
	else
	if( _key == "ZOOM" )
		return ZOOM;
	else
	if( _key == "RETURN" )
		return RETURN;
	else
	if( _key == "UP" )
		return UP;
	else
	if( _key == "LEFT" )
		return LEFT;
	else
	if( _key == "RIGHT" )
		return RIGHT;
	else
	if( _key == "DOWN" )
		return DOWN;
	else
	if( _key == "PLAY" )
		return PLAY;
	else
	if( _key == "STOP" )
		return STOP;
	else
	if( _key == "PREV" )
		return PREV;
	else
	if( _key == "NEXT" )
		return NEXT;
	else
	if( _key == "REW" )
		return REW;
	else
	if( _key == "FF" )
		return FF;
	else
	if( _key == "SUBTITLE" )
		return SUBTITLE;
	else
	if( _key == "AUDIO" )
		return AUDIO;
	else
	if( _key == "ANGLE" )
		return ANGLE;
	else
	if( _key == "SEARCH" )
		return SEARCH;
	else
	if( _key == "PROGRAM" )
		return PROGRAM;
	else
	if( _key == "AB" )
		return AB;
	else
	if( _key == "TIME" )
		return TIME;
	else
	if( _key == "ONE" )
		return ONE;
	else
	if( _key == "TWO" )
		return TWO;
	else
	if( _key == "THREE" )
		return THREE;
	else
	if( _key == "CLEAR" )
		return CLEAR;
	else
	if( _key == "FOUR" )
		return FOUR;
	else
	if( _key == "FIVE" )
		return FIVE;
	else
	if( _key == "SIX" )
		return SIX;
	else
	if( _key == "TEN" )
		return TEN;
	else
	if( _key == "SEVEN" )
		return SEVEN;
	else
	if( _key == "REPEAT" )
		return REPEAT;
	else
	if( _key == "EIGHT" )
		return EIGHT;
	else
	if( _key == "NINE" )
		return NINE;
	else
	if( _key == "ZERO" )
		return ZERO;

	std::cout << "UNKNOWN STR! :" << _key << std::endl;
	return UNKNOWN;
}


CRadioBox::KEYS CRadioBox::TranslateKey( int _key )
{
	switch( _key )
	{
		case CRCInput::RC_standby:	return POWER;
		case CRCInput::RC_up:	return UP;
		case CRCInput::RC_down:	return DOWN;
	}

	std::cout << "UNKNOWN INT!" << std::endl;
	return UNKNOWN;
}
