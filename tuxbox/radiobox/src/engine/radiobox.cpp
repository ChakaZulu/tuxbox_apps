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
//#define RADIOBOX_CPP

#include <file.h>
#include <global.h>
#include <radiobox.h>
#include <playlist.h>
#include <statehandler.h>
#include <lcdd.h>
#include <rbxinput.h>


#include <iostream> 
#include <dirent.h>
#include <unistd.h>



#define DEFGLOB






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


CRadioBox::CRadioBox() : working( true )
{
	this->audioplayer = CAudioPlayer::getInstance();
	this->audioplayer->init();
	
	statehandler = new CMainMenu();

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

	CStateHandler::DumpAllSTHNames();

	std::cout << "run" << std::endl;

	int divider = 0;

	while( working )
	{
		

		if( NULL == statehandler )
			throw "No state handler in slot!";

		if( divider++ == 1 )
		{
			statehandler->Show();
			divider = 0;
		}
		

		HandleKeys();

		CStateHandler* tmp = statehandler->GetSubHandler();

		if( tmp )
		{
			std::cout << "push to stack handler [" << statehandler << "]" << std::endl;
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


void CRadioBox::HandleKeys()
{
	CRBXInput::KEYS	key;
	bool	keypressed; // pressed or not
	
	CRBXInput::getInstance()->ReadKeys( key, keypressed );

	if( CRBXInput::POWER == key && false == keypressed )
	{
		CLCD::getInstance()->setMode( CLCD::MODE_SHUTDOWN, "Shutdown" );
		CRBXInput::getInstance()->WriteTranslations();
		working = false;
	}

	statehandler->HandleKeys( key, keypressed );

}



