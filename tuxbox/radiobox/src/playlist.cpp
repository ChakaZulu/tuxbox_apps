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
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <playlist.h>
#include <file.h>
#include <audioplay.h>
#include <audiofile.h>

#include <sstream>
#include <iomanip>
void CPlayList::Play( int _idx )
{
	std::string location;
	

//	current = _idx;

	try
	{
		location = SearchDirectoryEntry( _idx );
	}		
	catch( EPlayList _e )
	{
		/* bypass deleted records */
		if( EPlayList::deleted != _e.code )
			throw _e;
	}

	if( false == IsFileExists( location ) )
	{
		std::cout << "File does not exists! [" << location << "]" << std::endl;
		return;
	}
	
	std::cout << _idx << " Playing " << location << std::endl;
	
	CAudiofile	thefile( location, CFile( location ).getType() );
	
	CBaseDec::GetMetaDataBase( &thefile, true );

	CAudioPlayer::getInstance()->play( thefile );
}

CBaseDec::State CPlayList::GetState(  )
{
	return CAudioPlayer::getInstance()->getState();
}

void CPlayList::Play()
{
	std::cout << "Play songs in normal order" << std::endl;
	current = (size_t)-1;
	state = STATE_PLAY;
}

void CPlayList::PlayEntry( int _idx )
{
	current = _idx;
	state = STATE_PLAYENTRY;
	std::cout << "playing entry " << current << std::endl;
	Play( current );	
}


void CPlayList::PlayRandom()
{
	std::cout << "Play songs in random order" << std::endl;
	current = 0;
	rand_X = 0;
	state = STATE_PLAYRND;
	PrepareRandomizer();		
	std::cout << "rand_MAX " <<  rand_MAX << std::endl;

}

void CPlayList::Next()
{
	if( CAudioPlayer::getInstance()->getState() != CBaseDec::STOP )
		CAudioPlayer::getInstance()->stop();
}

void CPlayList::Prev()
{
	if( CAudioPlayer::getInstance()->getState() == CBaseDec::STOP )
		return;

	if( 5 < CAudioPlayer::getInstance()->getTimePlayed() )
	{
		std::cout << "rewind back" << std::endl;
		CAudioPlayer::getInstance()->rev( CAudioPlayer::getInstance()->getTimePlayed() );
		return;
	}
		

	switch( state )
	{
	case STATE_STOP:
		return;
	case STATE_PLAY:
		std::cout << "normal play go back 2" << std::endl;
		current -= 2;
		break;
	case STATE_PLAYRND:
		std::cout << "scroll" << std::endl;
		{ 
			size_t i = 0;
			size_t save_current = current;
			current = 0;
			for( i = 0; i < rand_MAX; i++ )
				GetRandomIdx();
			current = save_current-2;
		}
		break;
	case STATE_PAUSE:
	case STATE_COLLECTING:
		return;
	}

	CAudioPlayer::getInstance()->stop();
}

CPlayList::CPlayList( const std::string _filename, const bool _create )
: playlistname( _filename )
{
	bool exists = IsFileExists( _filename );
	
	this->norecords = 0;
	this->nofiles = 0;
	
	if( false == exists || true == _create )
	{
		playlist = fopen( _filename.c_str(), "wb" );
		
		WriteHeader( true );
		
		fclose( playlist );
	}
	
	playlist = fopen( _filename.c_str(), "r+" );
	
	if( NULL == playlist )
		throw EPlayList( EPlayList::noaccess );
	
	/* read header information */
	if( exists )
	{
		Load();
	}

}

int	CPlayList::GetRandomIdx()
{
	if( current == nofiles )
		return nofiles+1;

	rand_Xnext = (rand_A * rand_Xnext + rand_B) % rand_MAX;
	
	while( rand_Xnext >= nofiles )
	{
		rand_Xnext = (rand_A * rand_Xnext + rand_B) % rand_MAX;
	}

/*
	std::cout << "current = " << current << std::endl;
	std::cout << "rand_X = " << rand_X << std::endl;
	std::cout << "rand_Xnext = " << rand_Xnext << std::endl;
*/

	return rand_X = rand_Xnext;
}

void CPlayList::PrepareRandomizer()
{
	rand_MAX = 1;

	srand ( time(NULL) );

	for( ; rand_MAX <= 65536 && nofiles > rand_MAX; rand_MAX *= 2 );

	rand_A = 4 * (rand()%2024 + 128) + 1;

	rand_B = rand()%2024 + 128;

	if( 0 == rand_B%2 ) 
		rand_B++;

	rand_Xnext = rand()%rand_MAX;
}

	
void CPlayList::Delete( int _idx )
{
	std::cout << "Delete is not implamented" << std::endl;
}

void CPlayList::IncreaseCounter( int _idx )
{
	std::cout << "IncreaseCounter is not implamented" << std::endl;
}

void CPlayList::PlayFile( const std::string& _filename )
{
	std::cout << "play File is not implamented" << std::endl;
}
	
void CPlayList::PlayURL( const std::string& _location )
{
	std::cout << "play URL is not implamented" << std::endl;
}


bool CPlayList::IsFileExists( const std::string& _filename )
{
	return !access( _filename.c_str(), R_OK );
}

std::string EPlayList::GetText()
{
	switch( code )
	{
		case filenotfound:
			return "File not found";
		case nospace:
			return "No space left on device"; 
		case couldntcreatefile:
			return "Couldn't create file";
		case couldntopenurl:
			return "Couldn't open url";
		case noaccess:
			return "No access";
		case filenotopen:
			return "File not found";
		case corrupt:
			return "File is corrupt";
		case unknown:
			return "Unknown";
		case idxoutofbound:
			return "Index out of bounds";
		case deleted:
			return "Record marked as deleted";
	};
	
	return "Unknown";
}

// void CPlayList::AddFile( const std::string& _filename )
// {
// 	this->AddRecord( _filename, file );	
// }

void CPlayList::AddDir( const std::string& _dirname )
{
	/* poll the directory and collect data */
	struct dirent* entry = NULL;
	size_t	files = 0;
	
	DIR* dir = opendir( _dirname.c_str() );
	
 	std::cout << "Open direcory" << std::endl;
	
	if( NULL == dir )
		throw EPlayList( EPlayList::noaccess );
	
 	std::cout << "Read direcory" << std::endl;

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
			std::cout << "AddDir " << __LINE__ << std::endl;
			if( S_ISREG( status.st_mode ) )
			{
				/* regular file, add it to playlist */
				
				/* check filename extension, has to be mp3  */
				
				std::cout << "AddDir " << __LINE__ << std::endl;
		
				switch( CFile( fullname ).getType() )
				{ 
					case CFile::FILE_MP3 :
					case CFile::STREAM_AUDIO :
						std::cout << "AddDir " << __LINE__ << std::endl;
						files++;
					default:
						std::cout << "AddDir " << __LINE__ << std::endl;
				}
			}
			else if( S_ISDIR( status.st_mode ) )
			{
				std::cout << "AddDir " << __LINE__ << std::endl;
				if( strcmp( ".", entry->d_name ) && strcmp( "..", entry->d_name ) )
				{
					/* recursive add subfolders */
					std::cout << "AddDir " << __LINE__ << std::endl;
					AddDir( fullname );	
				}
			}
		}
		std::cout << "AddDir " << __LINE__ << std::endl;
		entry = readdir( dir );
	}
	std::cout << "AddDir " << __LINE__ << std::endl;
	closedir( dir );
	this->nofiles += files;
		std::cout << "AddDir " << __LINE__ << std::endl;
	AppendRecord( _dirname, files );
}

std::string	CPlayList::GetLocation( size_t _idx )
{
	if( _idx > 0 && _idx < nofiles )
		return SearchDirectoryEntry( _idx );
	else
		return "";
}

// void CPlayList::AddRecord( const std::string& _location, size_t _nfiles )
// {
// 	unsigned int i;
// 	size_t	start_pos;
// 
// 	size_t	length = _location.size();
// 
// 	playlist.seekp( 0, std::fstream::end );
// 	
// 	start_pos = playlist.tell();
// 	
// 	/* write delete flag */
// 	playlist << '\0';
// 	
// 	/* write counter */
// 	playlist << '\0';
// 
// 	/* write count of files */
// 	playlist << _nfiles
// 	
// 	playlist << length;
// 	
// 	if( playlist.fail() )
// 		throw EPlayList( EPlayList::nospace );
// 
// 	for( i = 0; i < _location.size(); i++ )
// 	{
// 		playlist << _location[i];	
// 	}
// 	
// 	if( playlist.fail() )
// 		throw EPlayList( EPlayList::nospace );
// 	
// 	this->norecords++;
// 
// 	this->WriteHeader();	
// }

/* load operator */
void CPlayList::Load( )
{
	fseek( playlist, 0, SEEK_SET );

	ReadCounter( this->norecords );
	ReadCounter( this->nofiles );
}	

void CPlayList::WriteHeader( bool _with_idx )
{
	fseek( playlist, 0, SEEK_SET );

	WriteCounter( this->norecords );
	WriteCounter( this->nofiles );

	int i;
	
	for( i = 0; _with_idx && i < MAXIDX; i++ )
	{
		WriteCounter( 0 );
		WriteCounter( 0 );			
	}			
}

char CPlayList::EncodeRecType( rectype _type )
{
	switch( _type )
	{
		case file: return 1;
		case url: return 0;			
	}
	throw EPlayList( EPlayList::unknown );
}


CPlayList::rectype CPlayList::DecodeRecType( char _type )
{
	switch( _type )
	{
		case 1: return file;
		case 0: return url;
	}
	throw EPlayList( EPlayList::unknown );
}

// void CPlayList::AddURL( const std::string& _url )
// {
// 	this->AddRecord( _url, url );	
// }

// void CPlayList::GetRecord( const int _idx, 	std::string& _location, rectype& _type )
// {
// 	char 	delflag;
// 	char 	type;
// 	char 	counter;
// 	size_t	length;
// 	
// 	size_t	rec_starplaylistt;
// 	// goto to position withing index
// 	playlist.seekp( sizeof( size_t ) + ( _idx * 4 ), std::fstream::beg );
// 
// 	playlist >> rec_start;
// 
// 	//goto record begin	
// 	playlist.seekp( rec_start, std::fstream::beg );
// 	
// //	std::cout << "Go to " << GetRecordBegin( _idx ) << "with record len " << RECSIZE;
// 	
// 	playlist >> delflag >> counter >> type >> length;
// 	
// 	if( playlist.fail() )
// 		throw EPlayList( EPlayList::unknown );
// 
// 	if( delflag )
// 		throw EPlayList( EPlayList::deleted );
// 		
// 	_type = DecodeRecType( type );
// 	
// 	int i = 0;
// 	
// //	std::cout << std::endl << "collect:";
// 	
// 	for( i = 0; i < length; i++ )
// 	{
// 		char buff;
// 		playlist >> std::noskipws >> buff;
// //		std::cout << buff;
// 		if( buff == 0 || playlist.eof() ) 
// 		{
// //			std::cout << " done ";
// //			if( playlist.eof() )
// //				std::cout << "by eof ";
// 			break;
// 		}
// 		if( playlist.fail() )
// 			throw EPlayList( EPlayList::unknown );
// 		
// 		_location += buff;
// 	}
// //	std::cout << std::endl;
// 	playlist.clear();	
// }


// size_t CPlayList::GetRecordBegin( const size_t _idx )
// {
// 	if( _idx > this->norecords )
// 	{
// 		std::cout << "Index is out of bounds [idx = "<< _idx << ", norecords = " << norecords << "]" << std::endl;
// 		throw EPlayList( EPlayList::idxoutofbound );
// 	}
// 	
// 	return GetHeaderLenght() + this->recsize * _idx;
// }

void CPlayList::Stop()
{
	state = STATE_STOP;
	CAudioPlayer::getInstance()->stop();
}

void CPlayList::FF( size_t _seconds )
{

}

void CPlayList::REW( size_t _seconds )
{

}

void CPlayList::Pause()
{
	if( state != STATE_PAUSE )
	{
		paused_state = state;
		state = STATE_PAUSE;
	}
	else
		state = paused_state;

	CAudioPlayer::getInstance()->pause();
}

void CPlayList::DoAction()
{

	switch( state )
	{
	case STATE_STOP:
// 		std::cout << "DoAction::STOP" << std::endl;
		return;
	case STATE_PLAYENTRY:
// 		std::cout << "DoAction::PLAYENTRY" << std::endl;
		if( CAudioPlayer::getInstance()->getState() == CBaseDec::STOP )
		{
			state = STATE_STOP;
		}
		break;
	case STATE_PLAY:
// 		std::cout << "DoAction::PLAY" << std::endl;
		if( CAudioPlayer::getInstance()->getState() == CBaseDec::STOP )
		{
			std::cout << "Stopped" << std::endl;
			try
			{
				std::cout << "Try playing next item" << std::endl;
				Play( ++current );
				std::cout << "Play: " << GetCurrentLocation() << std::endl;
			}
			catch( EPlayList _e )
			{
				switch( _e.code )
				{
				case EPlayList::idxoutofbound:
					this->state = STATE_STOP;
					return;
				default:
					throw _e;
				}
			}
		}
		return;
	case STATE_PLAYRND:
// 		std::cout << "DoAction::PLAYRND" << std::endl;
		if( CAudioPlayer::getInstance()->getState() == CBaseDec::STOP )
		{
			std::cout << "Stopped" << std::endl;
			try
			{
				std::cout << "Try playing next random item" << std::endl;
				Play( GetRandomIdx() );
				current++;
				std::cout << "current = " << current << std::endl;
			}
			catch( EPlayList _e )
			{
				switch( _e.code )
				{
				case EPlayList::idxoutofbound:
					this->state = STATE_STOP;
					return;
				default:
					throw _e;
				}
			}
		}
		return;
	case STATE_PAUSE:
// 		std::cout << "DoAction::PAUSE" << std::endl;
		return;
	case STATE_COLLECTING:
// 		std::cout << "DoAction::COLLECTED" << std::endl;
		return;
	}

// 	std::cout << "DoAction end" << std::endl;

}

int CPlayList::GetPositionPercents()
{
	if( 0 == CAudioPlayer::getInstance()->getTimeTotal() )
	{
		return -1;
	}
	
	return CAudioPlayer::getInstance()->getTimePlayed() * 100 / CAudioPlayer::getInstance()->getTimeTotal();
}

std::string	CPlayList::GetTimePlayed()
{
	int tmp = CAudioPlayer::getInstance()->getTimePlayed();

	int minutes  = tmp / 60 /* seconds in a hour */;; ;

	int seconds = tmp%60;

	std::ostringstream ss;

//	if( days )
//		ss << days << ":";
	
//	if( hours )
//		ss << hours << ":";

	ss << std::setfill( '0' ) << std::setw(2) << minutes << std::setw( 1 ) << ":";
	ss << std::setfill( '0' ) << std::setw(2) << seconds <<  std::setw( 1 );

	return std::string( ss.str() );
}

std::size_t CPlayList::GetCurrent()
{
	switch( state )
	{
	case STATE_PLAYENTRY:
	case STATE_PLAY:
		return current;
	case STATE_PLAYRND:
		return rand_X;
	case STATE_PAUSE:
		switch( paused_state )
		{
			case STATE_PLAYENTRY:
			case STATE_PLAY:
				return current;
			case STATE_PLAYRND:
				return rand_X;
			default:
				return 0;
		}
	case STATE_COLLECTING:
	case STATE_STOP:
		return 0;
	}

}



void CPlayList::AppendIndex( size_t _filecount, size_t _dirpos )
{

	size_t pos = 
					sizeof( size_t ) + //norecords  
					sizeof( size_t ) + //nofiles
					this->norecords * 
					(
						sizeof( size_t ) +		//nofiles
						sizeof( size_t ) 		//position
					);

	this->norecords++;

	fseek( playlist, pos, SEEK_SET );

	WriteCounter( _filecount );
	WriteCounter( _dirpos );

	WriteHeader();
}

void CPlayList::AppendRecord( std::string _dirname, size_t _filecount )
{
	
// 	std::cout << "Directory = " 	<< _dirname << std::endl;
// 	std::cout << "filecount = " 	<< _filecount << std::endl;

	static size_t __pos = 0;
	
// 	std::cout << "old pos = " 	<< __pos << std::endl;

	fseek( playlist, 0, SEEK_END );

	
	size_t	pos = ftell( playlist );
	
// 	std::cout << "new pos = " 	<< __pos << std::endl;
	
	__pos = pos;

	size_t i = 0;
	
	const char* text = _dirname.c_str();

	for( i = 0; i < _dirname.size(); i++ )
	{
		fputc( text[i], playlist );
	}
	//write terminator
		fputc( '\0', playlist );

	AppendIndex( _filecount, pos );
}
	
std::string CPlayList::GetDirectoryEntry( size_t _pos, size_t _count, size_t _idx )
{
	fseek( playlist, _pos, SEEK_SET );
		
// 	std::cout << "Search dirname at " << std::hex << _pos << std::dec << std::endl;

	std::string dirname = "";

	int buffer = 0;
	while( buffer != EOF ) 
	{
		
		buffer = fgetc( playlist );
// 		std::cout << ":" << buffer;
		if( '\0' == buffer  ) 
		{
			break;
		}
		dirname += buffer;
	}
		

// 	std::cout << "\nDirname = " << dirname << std::endl;

	///////////////////////////////////////////////

	struct dirent* entry = NULL;
	size_t	files = 0;
	
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
		return "";

	}
	
	while( entry != NULL )
	{
		struct stat status;
		std::string fullname;
		
		fullname += dirname + "/" + entry->d_name; 
		
// 		std::cout << "get entry status " << fullname << std::endl;
		
		if( 0 == stat( fullname.c_str(), &status ) )
		{
			if( S_ISREG( status.st_mode ) )
			{
				/* check filename extension, has to be mp3  */
				
				if
				( 
					CFile::FILE_MP3 == CFile( fullname ).getType() ||
					CFile::STREAM_AUDIO == CFile( fullname ).getType()
				)
				{
					if( _count + files == _idx )
					{
						std::cout << "File " << fullname << "\n";
						std::cout << "DName " << entry->d_name << std::endl;
						return fullname;
					}						
					
					files++;
				}
			}
		}
		entry = readdir( dir );
	}

	closedir( dir );
	
	return "";
	
}
/* reads all indexes, counts numbedr of files and returns 
position of directory where the file with _idx is */
std::string CPlayList::SearchDirectoryEntry( size_t _idx )
{
	static size_t		last_idx = _idx;
	static std::string	last_filename = "";
	
	if( last_idx == _idx )
		return last_filename;

	last_idx = _idx;

// 	std::cout << "sizeof( size_t ) = " << sizeof( size_t ) << std::endl;

	fseek( playlist, sizeof( size_t ) * 2, SEEK_SET );
	
	size_t count = 0;
	size_t files = 0;
	size_t pos = 0;

// 	std::cout << "scan index for " << _idx <<  std::endl;

	while(1)
	{
		ReadCounter( files );
		ReadCounter( pos );

/*		std::cout << "count " << count <<  std::endl;
		std::cout << "files " << files <<  std::endl;
		std::cout << "pos " << std::hex << pos << std::dec <<  std::endl;*/
		
		if( (files + count) > _idx )
		{
// 			std::cout << "count + files > _idx" <<  std::endl;
			last_filename = GetDirectoryEntry( pos, count, _idx );
			return last_filename;
		}
	
		count += files;
	}
	
// 	std::cout << "Index out of bounds!" <<  std::endl;
	throw EPlayList( EPlayList::corrupt );
	
	return "";	
}

void CPlayList::WriteCounter( size_t _counter )
{
	fwrite( &_counter, sizeof( size_t ), 1, playlist );
}

void	 CPlayList::ReadCounter( size_t& _counter )
{
	fread( &_counter, sizeof( size_t ), 1, playlist );

}

