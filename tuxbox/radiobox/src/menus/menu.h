#ifndef __MENU_H__
#define __MENU_H__

#include <statehandler.h>

#include <list>
#include <fstream>

#define CMENU	"CMenu"

class CMenu : public CStaticMenu
{
private:
	void DoAction( std::string _action );

	struct config
	{
		std::string	line;
		std::ifstream file;
	
		config( std::string _filename )
		{
			file.open( _filename.c_str() );
		}

		bool next() 
		{  
			if( file.is_open() )
			{
				return std::getline( file, line );
			}

			return false;
		}
	};

	struct menu_entry
	{
		menu_entry* parent;
		std::string handler;
		std::string text;
		std::list<menu_entry*> submenu;
		void Free();
		void ReadConfig( config& _cfg, int _level );
		void ParseLine( std::string _line, int& _level, std::string& _sthname, std::string& _sthtext );
		menu_entry( menu_entry* _parent = NULL ) : parent(_parent) {}
	};

	menu_entry*	menu;
	menu_entry*	curent;
	
	bool owner;
	void SetMenu( menu_entry* _menu );

public:
	void LoadMenu();
	CMenu();
	virtual ~CMenu();
	std::string GetName() { return CMENU; }
};



#endif /* __MENU_H__ */
