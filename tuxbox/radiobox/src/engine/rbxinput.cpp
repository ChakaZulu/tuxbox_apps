#include <configfile.h>
#include <rbxinput.h>

#include <iostream>


#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define TRCONFIG	CONFIGDIR "/radiobox/rckeys.conf"

CRBXInput::CRBXInput()
{ 
	SetDefaultTranslations();	
	LoadNativeTranslations();
	OpenLircd(); 
}

CRBXInput::~CRBXInput()
{ 
	std::cout << "~CRBXInput()" << std::endl;
	WriteTranslations();
}


void CRBXInput::ReadKeys( KEYS& _key, bool& _keypressed )
{

	static KEYS lastkey = NOKEY;

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	ReadFromLircd( _key );


	if( NOKEY != _key )
	{
		_keypressed = true;
		lastkey = _key;
	}
	else
	{
		if( NOKEY != lastkey )
		{
			_key = lastkey;
			_keypressed = false;
		}
		lastkey = NOKEY;
	}

	rcinput.getMsg_ms(&msg, &data, 100 );
		
	if( msg != CRCInput::RC_nokey && msg != CRCInput::RC_timeout )
	{
		std::cout << "got something from rcinput!" << std::endl;
		_key = TranslateKey( (int)msg );
		_keypressed = !data;
	}

}

void CRBXInput::ReadFromLircd( KEYS& _key )
{
	if( -1 == lircd ) return;

//	std::cout << "read from LIRCD :" << std::endl;
		
	fd_set		read_sock;
	struct 		timeval	tv;
	int 		rc = 0;

	_key = NOKEY;

//	std::cout << "read data from lircd" << std::endl;

	tv.tv_sec	= 0;
	tv.tv_usec	= 0;

	FD_ZERO( &read_sock );
	FD_SET( lircd, &read_sock );

	rc = select( lircd + 1, &read_sock, NULL, NULL, &tv );

	if( 0 == rc )
	{
//		std::cout << "read from LIRCD 2:" << std::endl;
		return; // nothing  to read from lircd
	}

//	std::cout << "read from LIRCD 0:" << std::endl;

	if( -1 == rc )
	{
		//something wrong, don't read from lircd anymore
//		std::cout << "read from LIRCD 1:" << std::endl;
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
					_key = TranslateKey( sub );
//					std::cout << sub << " translated = " << this->key << std::endl;
					break;
			}

			start = ++end;			
		}

	}

}

void CRBXInput::OpenLircd()
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

CRBXInput::KEYS CRBXInput::TranslateKey( std::string _key )
{
	struct tr_pair 
	{
		const char* 		key;
		KEYS		value;
	} defs[] = 
	{
		{ "POWER",POWER },
		{ "OPEN",OPEN },
		{ "MENU",MENU },
		{ "TITLE",TITLE },
		{ "DISPLAY",DISPLAY },
		{ "SELECT",SELECT },
		{ "ZOOM",ZOOM },
		{ "RETURN",RETURN },
		{ "UP",UP },
		{ "LEFT",LEFT },
		{ "RIGHT",RIGHT },
		{ "DOWN",DOWN },
		{ "PLAY",PLAY },
		{ "STOP",STOP },
		{ "PREV",PREV },
		{ "NEXT",NEXT },
		{ "REW",REW },
		{ "FF",FF },
		{ "SUBTITLE",SUBTITLE },
		{ "AUDIO",AUDIO },
		{ "ANGLE",ANGLE },
		{ "SEARCH",SEARCH },
		{ "PROGRAM",PROGRAM },
		{ "AB",AB },
		{ "TIME",TIME },
		{ "ONE",ONE },
		{ "TWO",TWO },
		{ "THREE",THREE },
		{ "CLEAR",CLEAR },
		{ "FOUR",FOUR },
		{ "FIVE",FIVE },
		{ "SIX",SIX },
		{ "TEN",TEN },
		{ "SEVEN",SEVEN },
		{ "REPEAT",REPEAT },
		{ "EIGHT",EIGHT },
		{ "NINE",NINE },
		{ "ZERO",ZERO },
		{ "MUTE",MUTE },
		{ "MINUS",MINUS },
		{ "PLUS",PLUS },
		{ "RED",RED },
		{ "BLUE",BLUE },
		{ "YELLOW",YELLOW },
		{ "GREEN",GREEN },
		{ NULL,UNKNOWN }
	};
	
	int i = 0;

	while( defs[i].key )
	{
		if( 0 == strcmp( defs[i].key, _key.c_str() ) )
		{
			return defs[i].value;
		}
		i++;
	}
	
	return UNKNOWN;
}


//Native RC
void CRBXInput::LoadNativeTranslations()
{
	unsigned int i;
	CConfigFile	configfile( '=' );
	configfile.loadConfig( TRCONFIG );

	//i loop over all defined keys and check wheither there is a defined key value
	for( i = UNKNOWN + 1; i < NOKEY; i++ )
	{
		_trpair tr;
		tr.key = (KEYS)i;
		tr.rc = configfile.getInt32( GetKeyName( (KEYS)i ) );


		if( strcmp( "unknown", CRCInput::getSpecialKeyName( tr.rc ) ) )
		{
			bool flag = true;

			std::list<_trpair>::iterator t;
			for( t = translations.begin(); t != translations.end(); t++ )
			{
				if( t->key == (KEYS)i )
				{
					flag = false;

					t->rc = tr.rc;
					break;
				}
			}

			if( flag )
			{
				translations.push_back( tr );
			}
		}
	}
}

void CRBXInput::WriteTranslations()
{
	CConfigFile	configfile( '=' );
	configfile.loadConfig( TRCONFIG );

	std::list<_trpair>::iterator t;
	for( t = translations.begin(); t != translations.end(); t++ )
	{
		configfile.setInt32( GetKeyName( t->key ), t->rc );
	}
	std::cout << "Save config file with translations!" << std::endl;
	configfile.saveConfig( TRCONFIG );
}

void CRBXInput::SetDefaultTranslations()
{
	_trpair defs[] = 
	{
		{ POWER, CRCInput::RC_standby },
		{ UP, CRCInput::RC_up },
		{ DOWN, CRCInput::RC_down },
		{ POWER, CRCInput::RC_standby },
		{ POWER, CRCInput::RC_standby },
		{ POWER, CRCInput::RC_standby },
		{ POWER, CRCInput::RC_standby },
		{ ZERO, CRCInput::RC_0 },
		{ ONE, CRCInput::RC_1 },
		{ TWO, CRCInput::RC_2 },
		{ THREE, CRCInput::RC_3 },
		{ FOUR, CRCInput::RC_4 },
		{ FIVE, CRCInput::RC_5 },
		{ SIX, CRCInput::RC_6 },
		{ SEVEN, CRCInput::RC_7 },
		{ EIGHT, CRCInput::RC_8 },
		{ NINE, CRCInput::RC_9 },
		{ CLEAR, CRCInput::RC_backspace },
		{ DISPLAY, CRCInput::RC_home },
		{ REW, CRCInput::RC_page_up },
		{ LEFT, CRCInput::RC_left },
		{ RIGHT, CRCInput::RC_right },
		{ FF, CRCInput::RC_page_down },
		{ MUTE, CRCInput::RC_spkr },
		{ MINUS, CRCInput::RC_minus },
		{ PLUS, CRCInput::RC_plus },
		{ MENU, CRCInput::RC_setup },
		{ SELECT, CRCInput::RC_ok },
		{ RED, CRCInput::RC_red },
		{ GREEN, CRCInput::RC_green },
		{ STOP, CRCInput::RC_yellow },
		{ PLAY, CRCInput::RC_blue },
		{ UNKNOWN, 0 } };

	int i = 0;
	

	for( i = 0; defs[i].key != UNKNOWN; i++ )
	{
		translations.push_back( defs[i] );
	}
}

//Native RC
CRBXInput::KEYS CRBXInput::TranslateKey( int _key )
{
	std::list<_trpair>::iterator t;

	for( t = translations.begin(); t != translations.end(); t++ )
	{
		if( t->rc == _key ) 
		{
			return t->key;
		}
	}

	return UNKNOWN;
}

const char* CRBXInput::GetKeyName( KEYS _key )
{
	char* keys[] = {
		"POWER",
		"OPEN",
		"TITLE",
		"DISPLAY",
		"SELECT",
		"MENU",
		"ZOOM",
		"RETURN",
		"UP",
		"LEFT",
		"RIGHT",
		"DOWN",
		"PLAY",
		"STOP",
		"PREV",
		"NEXT",
		"REW",
		"FF",
		"SUBTITLE",
		"AUDIO",
		"ANGLE",
		"SEARCH",
		"PROGRAM",
		"REPEAT",
		"AB",
		"TIME",
		"ONE",
		"TWO",
		"THREE",
		"CLEAR",
		"FOUR",
		"FIVE",
		"SIX",
		"TEN",
		"SEVEN",
		"EIGHT",
		"NINE",
		"ZERO",
		"MUTE",
		"PLUS",
		"MINUS",
		"RED",
		"BLUE",
		"YELLOW",
		"GREEN" };

	return keys[ _key - 1 ];

}





