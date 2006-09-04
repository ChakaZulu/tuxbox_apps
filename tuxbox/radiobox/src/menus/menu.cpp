#include <menu.h>
#include <iostream>
#include <global.h>

#define MENUCONFIG	CONFIGDIR "/radiobox/radiobox.menu"

CMenu::CMenu()
{
	std::cout << DBGINFO << std::endl;
	this->owner = false;
	this->menu = NULL;
}

void CMenu::DoAction( std::string _action )
{
	std::cout << DBGINFO << _action << std::endl;
	int counter = 0;	
	std::list<menu_entry*>::iterator i = menu->submenu.begin();
//	this->entries.clear();
	for(; i != menu->submenu.end(); i++, counter++ )
	{
		if( counter == sel )
		{
			if( (*i)->handler == CMENU )
			{
				std::cout << DBGINFO << std::endl;
				CMenu* m = new CMenu();
				m->SetMenu( (*i) );
				m->title = _action;
				subhandler = m;
				break;
			}
			else
			{
				subhandler = CStateHandler::CreateHandlerByName( (*i)->handler );
			}
		}
	}
}

/**************************************************************/

void CMenu::SetMenu( menu_entry* _menu )
{
	if( _menu == NULL )
		return;

	std::cout << DBGINFO << this << "," << _menu << std::endl;

	this->menu = _menu;
	std::list<menu_entry*>::iterator i = menu->submenu.begin();
//	this->entries.clear();
	for(; i != menu->submenu.end(); i++ )
	{
		std::cout << DBGINFO << (*i)->text << std::endl;
		this->entries.push_back( (*i)->text );
	}
}

void CMenu::LoadMenu()
{
	config cfg( MENUCONFIG );
	menu = new menu_entry();
	std::cout << "ReadConfig" << DBGINFO << std::endl;
	menu->ReadConfig( cfg, 0 );
	this->title = "Main Menu";
	SetMenu( menu );
}

CMenu::~CMenu()
{
	std::cout << DBGINFO << "Free CMENU" << std::endl;
/*	if( owner && menu )
	{
		menu->Free();
		delete menu;
	}*/
}

void CMenu::menu_entry::Free()
{
	std::list<menu_entry*>::iterator i = submenu.begin();
	
	for( ; i != submenu.end(); i++ )
	{
		if( (*i) )
		{
			(*i)->Free();
			delete (*i);
		}
	}
}

void CMenu::menu_entry::ReadConfig( config& _cfg, int _level )
{
	int curlevel = 0;
	std::string sthname;
	std::string sthtext;		

	std::string line;

	std::cout << "ReadConfig" << DBGINFO << std::endl;
	bool flag = _cfg.next();

	while( flag )
	{
		std::cout << "ReadConfig: " << _cfg.line << " " << DBGINFO << std::endl;
		ParseLine( _cfg.line, curlevel, sthname, sthtext );
		std::cout << "ReadConfig: " << curlevel << "," << sthname << "," << sthtext << " " << DBGINFO << std::endl;

		if( curlevel != _level ) return;

		menu_entry* me = new menu_entry( this );
		me->handler = sthname;
		me->text = sthtext;

		std::cout << "me.handler = " << me->handler << " " << DBGINFO << std::endl;

		if( me->handler == CMENU )
		{
			std::cout << "ReadConfig: recursion" << DBGINFO << std::endl;
			me->ReadConfig( _cfg, _level+1 );
		}
		else
		{
			flag = _cfg.next();
		}

//TODO		me->Validate();
		
		this->submenu.push_back( me );	
	}
}

void CMenu::menu_entry::ParseLine(  std::string _line, int& _level, std::string& _sthname, std::string& _sthtext )
{
	int i = 0;
	_level = 0;
	_sthname = "";
	_sthtext = "";
	for( i = 0; i < (int)_line.size() && _line[i] == '\t'; i++  ) _level++;
	for( ;i < (int)_line.size() && _line[i] != '\t'; i++  ) _sthname += _line[i];
	for( i++; i < (int)_line.size(); i++  ) _sthtext += _line[i];
}
