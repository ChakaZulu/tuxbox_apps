#include <statehandler.h>

#include <iostream> 
#include <fstream> 
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
#include <global.h>
#include <flashtool.h>

#define PLAYLISTDIR g_settings.playlist_root


/**************************************************************/

CSelectPlayList::CSelectPlayList() 	
{ 
	std::cout << "Select playlist::create()" << std::endl;
	LoadPlaylists( PLAYLISTDIR ); 
	if( 0 == playlists.size() )
		throw "No playlists were found in "  "/var/"  " location.";
}


/**************************************************************/

void CSelectPlayList::HandleKeys( CRBXInput::KEYS _key, bool _pressed )
{
	if( _pressed ) return; /* handle it only if key released is */

	switch( _key )
	{
		case CRBXInput::POWER:
			break;
		case CRBXInput::PLAY:
			this->subhandler = new CPlayPLRandom( new CPlayList( std::string( PLAYLISTDIR ) + playlists[menu_top + menu_selected], false ) );
			break;
		case CRBXInput::SELECT:
//			this->subhandler = new CPlayListEntries( std::string( PLAYLISTDIR ) + playlists[menu_top + menu_selected] );
			break;
		case CRBXInput::UP:
			menu_selected --;
			break;
		case CRBXInput::DOWN:
			menu_selected ++;
			break;
		case CRBXInput::MENU:
			break;
		default:
			break;
	}

	if( menu_selected == playlists.size() || menu_selected == NUM_MENU_LINES )
	{
		if( NUM_MENU_LINES < playlists.size() - menu_top )
		{
			menu_top++;
		}
	
		menu_selected = (playlists.size() > NUM_MENU_LINES )?NUM_MENU_LINES-1:playlists.size()-1;
	}
	else
	if( menu_selected < 0 )
	{
		if( 0 < menu_top )
		{
			menu_top--;
		}
	
		menu_selected = 0;
	}


}

/**************************************************************/

void CSelectPlayList::Show()
{
	unsigned int i;
	
	CLCD::getInstance()->ClearMenu();
	CLCD::getInstance()->setMenuTitle( "Playlists" );


	for( i = 0; i < playlists.size() && i < NUM_MENU_LINES; i++ )
	{
		CLCD::getInstance()->setMenuLine( playlists[i + menu_top] );

	}

	CLCD::getInstance()->setMenuCursors( ( menu_top > 0 ), ( NUM_MENU_LINES < playlists.size() - menu_top ) );
	CLCD::getInstance()->setMenuSelected( menu_selected );
	CLCD::getInstance()->ShowMenu(0);
}

/**************************************************************/

//polls directory with _dirname and collect names of playlists 
void CSelectPlayList::LoadPlaylists( std::string _dirname )
{
	/* poll the directory and collect data */
	struct dirent* entry = NULL;
	
	DIR* dir = opendir( _dirname.c_str() );
	
//	std::cout << "Open direcory" << std::endl;
	
	if( NULL == dir )
		return;
	
//	std::cout << "Read direcory" << std::endl;

	if( NULL == ( entry = readdir( dir ) ) )
	{
		/* perhaps some errors, 
		but currently no checks, just exit, 
		assume empty folder */
		return;
	}
	
	while( entry != NULL )
	{
		struct stat status;
		std::string fullname;
		
		fullname += _dirname + "/" + entry->d_name; 
		
		std::cout << "get entry status " << fullname << std::endl;
		
		if( 0 == stat( fullname.c_str(), &status ) )
		{
			if( S_ISREG( status.st_mode ) )
			{
				/* regular file, add it to playlist */
				
				/* check filename extension, has to be mp3  */
				
			
				if( CFile::FILE_RADIOBOX == CFile( fullname ).getType() )
				{
					this->playlists.push_back( CFile( fullname ).getFileName() );
				}
				else
					std::cout << 
						"Baypass file with " << 
						fullname << 
						" extension" << std::endl;
			}
		}
	
		entry = readdir( dir );
	}
		
	closedir( dir );

}

/**************************************************************/

void CPlayListEntries::Show()
{
	unsigned int i;

	CLCD::getInstance()->ClearMenu();
	CLCD::getInstance()->setMenuTitle( playlist->GetName() );


	for( i = 0; i < playlist->GetSize() && i < NUM_MENU_LINES; i++ )
	{
		CLCD::getInstance()->setMenuLine( basename( playlist->GetLocation( i + menu_top ).c_str() ) );
	}

	CLCD::getInstance()->setMenuCursors( ( menu_top > 0 ), ( NUM_MENU_LINES < playlist->GetSize() - menu_top ) );
	CLCD::getInstance()->setMenuSelected( menu_selected );
	CLCD::getInstance()->ShowMenu(0);
}

/**************************************************************/

void CPlayListEntries::HandleKeys( CRBXInput::KEYS _key, bool _pressed )
{
	if( _pressed ) return; /* handle it only if key released is */

	switch( _key )
	{
		case CRBXInput::POWER:
			break;
		case CRBXInput::PLAY:
			this->subhandler = new CPlayLocation( playlist, menu_top + menu_selected );
			break;
		case CRBXInput::UP:
			menu_selected --;
			break;
		case CRBXInput::DOWN:
			menu_selected ++;
			break;
		case CRBXInput::DISPLAY:
			break;
		case CRBXInput::MENU:
			remove = true;;
			break;
		default:
			break;
	}

	if( menu_selected == playlist->GetSize() || menu_selected == NUM_MENU_LINES )
	{
		if( NUM_MENU_LINES < playlist->GetSize() - menu_top )
		{
			menu_top++;
		}
	
		menu_selected = (playlist->GetSize() > NUM_MENU_LINES )?NUM_MENU_LINES-1:playlist->GetSize()-1;
	}
	else
	if( menu_selected < 0 )
	{
		if( 0 < menu_top )
		{
			menu_top--;
		}
	
		menu_selected = 0;
	}
}

/**************************************************************/

CPlayLocation::CPlayLocation( CPlayList* _playlist, size_t _idx )
: playlist( _playlist ), idx( _idx ) 
{ 
	if( NULL == playlist )
		throw "CPlayLocation: Wrong parameter!";

	frame = 0;

	playlist->PlayEntry( idx ); 
}

/**************************************************************/

void CPlayLocation::Show()
{
//	sleep( 1 );
	frame = CLCD::getInstance()->ShowPlayingFile( CFile( playlist->GetCurrentLocation() ).getFileName(),  playlist->GetPositionPercents(), playlist->GetTimePlayed(), frame );

	if( CBaseDec::STOP == playlist->GetState() )
		remove = true;
}

/**************************************************************/

void CPlayLocation::HandleKeys( CRBXInput::KEYS _key, bool _pressed )
{
	if( _pressed ) return; /* handle it only if key released is */

	switch( _key )
	{
		case CRBXInput::POWER:
			break;
		case CRBXInput::PLAY:
			playlist->Pause();			
			break;
		case CRBXInput::UP:
			break;
		case CRBXInput::STOP:
			playlist->Stop();
			remove = true;
			break;
		case CRBXInput::DOWN:
			break;
		case CRBXInput::MENU:
			break;
		default:
			break;
	}

}

/**************************************************************/

CPlayPLRandom::CPlayPLRandom( CPlayList* _playlist )
: playlist( _playlist ) 
{ 
	if( NULL == playlist )
		throw "CPlayLocation: Wrong parameter!";

	frame = 0;
	
	if( g_settings.playorder == CRadioboxSettings::PO_Random )
		playlist->PlayRandom( ); 
	if( g_settings.playorder  == CRadioboxSettings::PO_Normal )
		playlist->Play( ); 
	
}

/**************************************************************/

void CPlayPLRandom::Show()
{
//	sleep( 1 );
	static size_t lastplayed = 	0;

	if( lastplayed != playlist->GetCurrent() )
	{
		frame = 0;
		lastplayed = playlist->GetCurrent();
	}

	frame = CLCD::getInstance()->ShowPlayingFile( CFile( playlist->GetCurrentLocation() ).getFileName(),  playlist->GetPositionPercents(), playlist->GetTimePlayed(), frame );

	playlist->DoAction();	

	if( playlist->IsStopped() )
		remove = true;

//	if( CBaseDec::STOP == playlist->GetState() )
//		remove = true;


}

/**************************************************************/

void CPlayPLRandom::HandleKeys( CRBXInput::KEYS _key, bool _pressed )
{
	if( _pressed ) return; /* handle it only if key released is */

	switch( _key )
	{
		case CRBXInput::POWER:
			break;
		case CRBXInput::PLAY:
			playlist->Pause();
			break;
		case CRBXInput::UP:
			break;
		case CRBXInput::STOP:
			playlist->Stop();
			remove = true;
			break;
		case CRBXInput::NEXT:
			playlist->Next();
			break;
		case CRBXInput::PREV:
			std::cout << "random handler: prev" << std::endl;
			playlist->Prev();
			break;
		case CRBXInput::DOWN:
			break;
		case CRBXInput::MENU:
			break;
		default:
			break;
	}

}

/**************************************************************/

CSelectLocation::CSelectLocation()
{
	try
	{
		LoadLocations();
	}
	catch( EPlayList e )
	{
		if( e.code == EPlayList::noaccess )
		{
			this->subhandler = new CMessageBox( "Cannot access media!" );
		}
	}
}

/**************************************************************/

void CSelectLocation::LoadLocations( )
{
	std::string dirname = g_settings.library_root;
	struct dirent* entry = NULL;
	
	DIR* dir = opendir( dirname.c_str() );
	
// 	std::cout << "Open direcory" << std::endl;
	
	if( NULL == dir )
		throw EPlayList( EPlayList::noaccess );
	
// 	std::cout << "Read direcory" << std::endl;

	if( NULL == ( entry = readdir( dir ) ) )
	{
		/* perhaps some errors, 
		but currently no checks, just exit, 
		assume empty folder */
		return;

	}
	
	while( entry != NULL )
	{
		struct stat status;
		std::string fullname;
		
		fullname += dirname + "/" + entry->d_name; 
		
// 		std::cout << "get entry status " << fullname << std::endl;
		
		if( 0 == stat( fullname.c_str(), &status ) )
		{
			if( S_ISDIR( status.st_mode ) )
			{
				if( strcmp( ".", entry->d_name ) && strcmp( "..", entry->d_name ) )
					locations.push_back( entry->d_name );
			}
		}

		entry = readdir( dir );
	}

	closedir( dir );

#if 0
	bool exists = CPlayList::IsFileExists( _locs_conf );
	std::string	loc;
	
	if( false == exists )
		throw "no locs.conf found!";

	std::fstream locs_conf;
	
	locs_conf.open( _locs_conf.c_str(), 
			std::fstream::in | 
			std::fstream::binary );
	
	if( false == locs_conf.is_open() )
		throw "Couldn't open loc.conf";
	
	/* read locations */
	while(1)
	{
		char buff;
		
		locs_conf >> std::noskipws >> buff;
//		std::cout << buff;
		if( buff == 0 || locs_conf.eof() ) 
		{
//			std::cout << " done ";
//			if( playlist.eof() )
//				std::cout << "by eof ";
			if( loc.size() )
				locations.push_back( loc );
			
			break;
		}

		if( locs_conf.fail() )
			throw EPlayList( EPlayList::unknown );

		if( 0x0a == buff )
		{
			std::cout << "\nAdd location: " << loc << std::endl;
			this->locations.push_back( loc );
			loc = "";			
		}
		else
		{
			std::cout << buff;	
			loc += buff;
		}	
	}
	
	locs_conf.close();	
#endif
}

/**************************************************************/

void CSelectLocation::HandleKeys( CRBXInput::KEYS _key, bool _pressed )
{
	if( _pressed ) return; /* handle it only if key released is */

	switch( _key )
	{
		case CRBXInput::POWER:
			break;
		case CRBXInput::PLAY:
			this->subhandler = new CPlayPLRandom( GetPlayList( g_settings.library_root + "/" + locations[menu_top + menu_selected] ) );
			break;
		case CRBXInput::SELECT:
			std::cout << "get locations[menu_top + menu_selected] " << g_settings.library_root + "/" + locations[menu_top + menu_selected] << std::endl;
			this->subhandler = new CPlayListEntries( GetPlayList( g_settings.library_root + "/" + locations[menu_top + menu_selected] ) );
			break;
		case CRBXInput::UP:
			menu_selected --;
			break;
		case CRBXInput::DOWN:
			menu_selected ++;
			break;
		case CRBXInput::MENU:
			remove = true;
			break;
		case CRBXInput::DISPLAY:
			this->subhandler = new CPlayListOptions( locations[menu_top + menu_selected] );
			break;
		default:
			break;
	}

	if( menu_selected == locations.size() || menu_selected == NUM_MENU_LINES )
	{
		if( NUM_MENU_LINES < locations.size() - menu_top )
		{
			menu_top++;
		}
	
		menu_selected = (locations.size() > NUM_MENU_LINES )?NUM_MENU_LINES-1:locations.size()-1;
	}
	else
	if( menu_selected < 0 )
	{
		if( 0 < menu_top )
		{
			menu_top--;
		}
	
		menu_selected = 0;
	}


}
/**************************************************************/

CPlayList* CSelectLocation::GetPlayList( std::string _location )
{
	std::string plname = _location;

	unsigned int i;

	for( i = 0; i < plname.size(); i++ )
		if( plname[i] == '/' )
			plname[i] = '_';
	
	plname = PLAYLISTDIR + plname + ".playlist";
		
	std::cout << "Get playlist " << __LINE__ << std::endl;

	CPlayList* pl = new	CPlayList( plname, !CPlayList::IsFileExists( plname ) );
		
	std::cout << "Get playlist " << __LINE__ << std::endl;
	if( 0 == pl->GetSize() )
	{
		pl->AddDir( _location );
	}
	std::cout << "Get playlist " << __LINE__ << std::endl;

	return pl;	
}

/**************************************************************/

void CMenu::ResetMenu() 
{ 
	menu_top = 0; menu_selected = 0; 
	CLCD::getInstance()->ClearMenu();
}


/**************************************************************/

void CSelectLocation::Show()
{
	unsigned int i;
	
	CLCD::getInstance()->ClearMenu();
	CLCD::getInstance()->setMenuTitle( "Locations" );


	for( i = 0; i < locations.size() && i < NUM_MENU_LINES; i++ )
	{
		CLCD::getInstance()->setMenuLine( locations[i + menu_top] );

	}

	CLCD::getInstance()->setMenuCursors( ( menu_top > 0 ), ( NUM_MENU_LINES < locations.size() - menu_top ) );
	CLCD::getInstance()->setMenuSelected( menu_selected );
	CLCD::getInstance()->ShowMenu( 0 );
}

/**************************************************************/

CMainMenu::CMainMenu()
{
	this->CanBeRemoved = false;
	this->title = "Main Menu";
	entries.push_back( "Media" );
	entries.push_back( "Setup" );
}

void CMainMenu::DoAction( std::string _action )
{
	switch( sel )
	{
	case 0: this->subhandler = new CSelectLocation(); break;
	case 1: this->subhandler = new CSetupMenu(); break;
	}
}

/**************************************************************/
void CStaticMenu::Show()
{
	unsigned int i;
	
	CLCD::getInstance()->ClearMenu();
	CLCD::getInstance()->setMenuTitle( this->title );


	for( i = 0; i < entries.size() && i < NUM_MENU_LINES; i++ )
	{
		CLCD::getInstance()->setMenuLine( entries[i + menu_top] );
	}

	CLCD::getInstance()->setMenuCursors( ( menu_top > 0 ), ( NUM_MENU_LINES < entries.size() - menu_top ) );
	CLCD::getInstance()->setMenuSelected( menu_selected );
	CLCD::getInstance()->ShowMenu(0);
}

/**************************************************************/

void CStaticMenu::HandleKeys( CRBXInput::KEYS _key, bool _pressed )
{
	if( _pressed ) return; /* handle it only if key released is */

	switch( _key )
	{
		case CRBXInput::SELECT:
			{
				sel = menu_top + menu_selected;
				DoAction( entries[sel] );
			}	
			break;
		case CRBXInput::UP:
			menu_selected --;
			break;
		case CRBXInput::DOWN:
			menu_selected ++;
			break;
		case CRBXInput::MENU:
			remove = CanBeRemoved;
			break;
		default:
			break;
	}

	if( menu_selected == entries.size() || menu_selected == NUM_MENU_LINES )
	{
		if( NUM_MENU_LINES < entries.size() - menu_top )
		{
			menu_top++;
		}
	
		menu_selected = (entries.size() > NUM_MENU_LINES )?NUM_MENU_LINES-1:entries.size()-1;
	}
	else
	if( menu_selected < 0 )
	{
		if( 0 < menu_top )
		{
			menu_top--;
		}
	
		menu_selected = 0;
	}


}
/**************************************************************/

CSetupMenu::CSetupMenu()
{
	title = "Setup";
	entries.push_back( "Play Order" );
	entries.push_back( "Mounts" );
	entries.push_back( "Update Software" );
}

void CSetupMenu::DoAction( std::string _action )
{
	if( _action == "Play Order" )
		this->subhandler = new CSetupPlayOrder();
	if( _action == "Mounts" )
		this->subhandler = new CMountSetup();
	if( _action == "Update Software" )
		this->subhandler = new CSetupSoftware();
}

/**************************************************************/

CSetupPlayOrder::CSetupPlayOrder()
{
	title = "Play order";
	entries.push_back( "Normal" );
	entries.push_back( "Random" );
}

void CSetupPlayOrder::DoAction( std::string _action )
{
	if( _action == "Normal" )
		g_settings.playorder = CRadioboxSettings::PO_Normal;
	if( _action == "Random" )
		g_settings.playorder = CRadioboxSettings::PO_Random;

	g_settings.Save();

	//here setup play order
	remove = true;
}

/**************************************************************/

/**************************************************************/

CSetupSoftware::CSetupSoftware()
{
	title = "Update Software";
	entries.push_back( "Read Flash" );
	entries.push_back( "Write Flash" );
}

void CSetupSoftware::DoAction( std::string _action )
{
	if( _action == "Read Flash" )
		this->subhandler = new CSetupReadFlash();
	if( _action == "Write Flash" )
		this->subhandler = new CSetupWriteFlash();
	remove = true;
}

/**************************************************************/
/**************************************************************/

CSetupReadFlash::CSetupReadFlash( )
{
	title = "Read part";
	int i = 0;
	for( i = 0; i < CMTDInfo::getInstance()->getMTDCount(); i++ )
	{
		entries.push_back( CMTDInfo::getInstance()->getMTDName( i ) );
	}
}

void CSetupReadFlash::DoAction( std::string _action )
{
	std::string file = "/tmp/";
	int num = CMTDInfo::getInstance()->findMTDNumberByName( _action );

	file += _action;
	file += ".img";

	unsigned int i = 0;
	for ( i = 0; i < file.size(); i++ )
	{
		if( 
			file[i] == ' '
			|| file[i] == '('
			|| file[i] == ')' ) 
		{
			file[i] = '_';
		}
	}

	std::cout << "File : " << file << std::endl;	
	std::cout << "Partition : " << _action << std::endl;	
	std::cout << "MTD Device : " << CMTDInfo::getInstance()->getMTDFileName( num ) << std::endl;	

	if( num >= 0 )
	{
		CFlashTool ft;
		ft.setMTDDevice( CMTDInfo::getInstance()->getMTDFileName( num ) );
		if( false == ft.readFromMTD( file, 100 ) )
		{
			std::cout << ft.getErrorMessage() << std::endl;
		}
	}	

	remove = true;
}

/**************************************************************/
/**************************************************************/

CSetupWriteFlash::CSetupWriteFlash( )
{
	title = "Select img.";
	this->flag = false;
	/* poll the directory and collect data */
	struct dirent* entry = NULL;
	
	DIR* dir = opendir( "/tmp" );
	
	if( NULL == dir )
		return;
	
	if( NULL == ( entry = readdir( dir ) ) )
	{
		/* perhaps some errors, 
		but currently no checks, just exit, 
		assume empty folder */
		return;
	}
	
	while( entry != NULL )
	{
		struct stat status;
		std::string fullname;
		
		fullname += std::string("/tmp/") + std::string(entry->d_name); 
		
		if( 0 == stat( fullname.c_str(), &status ) )
		{
			if( S_ISREG( status.st_mode ) )
			{
				/* regular file, add it to playlist */
				
				/* check filename extension, has to be mp3  */
				
				if( CFile::FILE_IMAGE == CFile( fullname ).getType() )
				{
					std::cout << "file " << fullname << std::endl;
					entries.push_back( fullname );
				}
			}
		}

		entry = readdir( dir );
	}
		
	closedir( dir );
}

void CSetupWriteFlash::DoAction( std::string _action )
{
	if( false == flag )
	{
		this->entries.erase( entries.begin(), entries.end() );
		this->file 	= _action;
		this->flag 	= true;
		title = "Select part";
		int i = 0;
		for( i = 0; i < CMTDInfo::getInstance()->getMTDCount(); i++ )
		{
			entries.push_back( CMTDInfo::getInstance()->getMTDName( i ) );
		}
	}
	else
	{
		CFlashTool ft;
		int num = CMTDInfo::getInstance()->findMTDNumberByName( _action );
		std::cout << "File : " << file << std::endl;	
		std::cout << "Partition : " << _action << std::endl;	
		std::cout << "MTD Device : " << CMTDInfo::getInstance()->getMTDFileName( num ) << std::endl;	
		ft.setMTDDevice( CMTDInfo::getInstance()->getMTDFileName( num ) );
		if( false == ft.program( file, 100, 100 ) )
		{
			std::cout << ft.getErrorMessage() << std::endl;
		}		

		remove = true;
	}

}
/**************************************************************/
/**************************************************************/

CPlayListOptions::CPlayListOptions( std::string _location )
{
	this->location = _location;
	title = "Options";
	entries.push_back( "Return" );
	entries.push_back( "Rebuild playlist" );
}

void CPlayListOptions::DoAction( std::string _action )
{
	if( _action == "Rebuild playlist" )
	{
		//TODO Show here "Please Wait" Screen
		std::string plname = location;

		unsigned int i;

		for( i = 0; i < plname.size(); i++ )
			if( plname[i] == '/' )
				plname[i] = '_';
	
		plname = PLAYLISTDIR + plname + ".playlist";

		CPlayList pl( plname, true );
		pl.AddDir( location );
	}
	remove = true;
}

/**************************************************************/

CPlayListEntries::CPlayListEntries( CPlayList* _playlist )
: playlist( _playlist )
{  
	std::cout << "CPlayListEntries" << std::endl;
}

/**************************************************************/

CMessageBox::CMessageBox( std::string _msg, int _type, int _timeout )
{
	this->frame = 0;
	this->msg = _msg;
	this->type = _type;
	this->endtime = time( NULL ) + _timeout ;
	CLCD::getInstance()->showMessageBox( msg, type, frame );
}


void	CMessageBox::Show()
{
	CLCD::getInstance()->showMessageBox( msg, type, frame++ );
	if( time( NULL ) > this->endtime )
	{
		remove = true;
	}
	return;
}

void	CMessageBox::HandleKeys( CRBXInput::KEYS _key, bool _pressed )
{
	if( _pressed ) return; /* handle it only if key released is */

	if( _key != CRBXInput::NOKEY )
		remove = true;
}

