#include <string>

#include <settings.h>
#include <configfile.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#define RADIOBOX_CONFIG			CONFIGDIR "/radiobox/radiobox.conf"
#define RADIOBOX_PLAYLISTROOT	CONFIGDIR "/radiobox/playlists"
#define RADIOBOX_LIBRARYROOT	CONFIGDIR "/radiobox/library"

void CRadioboxSettings::Load( )
{
	configfile.loadConfig( RADIOBOX_CONFIG );

	{
		int po = configfile.getInt32( "playorder" );
		switch( po )
		{
			case PO_Random: playorder = PO_Random; break;
			case PO_Normal: playorder = PO_Normal; break;
			case PO_RepeatAll: playorder = PO_RepeatAll; break;
			case PO_RepeatFile: playorder = PO_RepeatFile; break;
			default: playorder = PO_Random;
		}
	}

	playlist_root = configfile.getString( "playlist_root", RADIOBOX_PLAYLISTROOT );
	library_root = configfile.getString( "library_root", RADIOBOX_LIBRARYROOT );

}


void CRadioboxSettings::Save(  )
{

	configfile.setInt32( "playorder", int( playorder ) );
	configfile.setString( "playlist_root", playlist_root );
	configfile.setString( "library_root", library_root );

	configfile.saveConfig( RADIOBOX_CONFIG );
}

void CRadioboxSettings::Defaults()
{
	playorder = PO_Random;
	playlist_root = RADIOBOX_PLAYLISTROOT;
	library_root = RADIOBOX_LIBRARYROOT;
}


CRadioboxSettings::CRadioboxSettings( ) :
configfile( '=' )
{
	Defaults();
	
	Load();
}

CRadioboxSettings::~CRadioboxSettings( )
{
	Save();
}