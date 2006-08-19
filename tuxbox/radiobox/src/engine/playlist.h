#ifndef __PLAYLIST_H__
#define __PLAYLIST_H__

/***************************************************************************
 *            playlist.h
 *
 *  Sat Apr  9 23:45:03 2005
 *  Copyright  2005  User
 *  Email
 ****************************************************************************/

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


#include <basedec.h>

#include <stdio.h>
#include <string>


#define MAXLOCATION	51200 00 00 0c
#define STRHEADER "radiobox"
#define HDRLEN 8
#define RECSIZE ( MAXLOCATION + 1 + 1 + 1 ) /* 1 - deleted flag, 1 - times played, 1 - file/url flag */

/* exception for playlist */

#define MAXIDX 	(5000)

struct EPlayList
{

	enum eplcode
	{
		filenotfound,
		nospace,
		couldntcreatefile,
		couldntopenurl,
		noaccess,
		filenotopen,
		corrupt,
		unknown,
		idxoutofbound,
		deleted
	};

	eplcode code;
	EPlayList( eplcode _code ) { this->code = _code; };
	
	std::string GetText();
};

class CPlayList
{
public:
	enum STATES
	{
		STATE_STOP = 0, /* stopped */
		STATE_PAUSE, /* paused */
		STATE_PLAYENTRY, /* playing entry */
		STATE_PLAY, /* playing normal order */
		STATE_PLAYRND, /* playing random order */
		STATE_COLLECTING /* collection filenames for playlist */
	};
	
	struct INDEX
	{
		size_t		filecount;
		size_t		dirpos;
	};

private:
	size_t			norecords;
	size_t			nofiles;

	FILE*			playlist;

private:

	STATES 	state;
	STATES	paused_state;
	size_t	current;

	unsigned int rand_MAX;
	unsigned int rand_X;
	unsigned int rand_Xnext;
	unsigned int rand_A;
	unsigned int rand_B;

public:
	
	enum rectype
	{
		url,
		file		
	};

	CPlayList( const std::string _filename, const bool _create = false );

//	void 		AddFile( const std::string& _filename );
	void 		AddDir( const std::string& _dirname );
//	void 		AddURL( const std::string& _url );

	void		PlayRandom();
	void		PlayEntry( int _idx );
	void		Play();

	void		Next();
	void		Prev();

	void		Stop();
	void		FF( size_t _seconds );
	void		REW( size_t _s__PLAYLIST_H__econds );
	void		Pause();

	void 		DoAction();

	static bool	IsFileExists( const std::string& _filename );

/***********************************************************************/

	std::string		GetName() { return playlistname; }
	std::size_t		GetSize() { return nofiles; }
	std::size_t		GetCurrent();
	std::string		GetLocation( size_t _idx );
	std::string		GetCurrentLocation() { return GetLocation( GetCurrent() ); }
	int 			GetPositionPercents();
	CBaseDec::State	CPlayList::GetState();
	bool			CPlayList::IsStopped() { return state == STATE_STOP; };
	std::string		GetTimePlayed();


private:
	std::string	playlistname;

/*
	size_t 	GetHeaderLenght() { return 8 + sizeof( size_t ) + sizeof( size_t ); };
	size_t 	GetFileLength() { return GetHeaderLenght() + recsize * norecords; };
	size_t 	GetRecordBegin( const size_t _idx );

	void 	AddRecord( const std::string& _location, rectype _type );
	void	GetRecord( const int _idx, 	std::string& _location, rectype& _type );

*/
	void	PrepareRandomizer();
	int 	GetRandomIdx();

	void	Delete( int _idx );
	void	IncreaseCounter( int _idx );
	
	void	Play( int _idx );
	void	PlayFile( const std::string& _filename );
	void	PlayURL( const std::string& _location );

	void	 Load( );
	
	char	EncodeRecType( rectype _type );
	rectype DecodeRecType( char _type );	


	void 	WriteHeader( bool with_idx = false );
	void		AppendIndex( size_t _filecount, size_t _dirpos );
	void		AppendRecord( std::string _dirname, size_t _filecount );
	
	std::string	GetDirectoryEntry( size_t _pos, size_t _count, size_t _idx );
	std::string SearchDirectoryEntry( size_t _idx );

	void		WriteCounter( size_t _counter );	
	void		ReadCounter( size_t& _counter );	
	
};


#endif //__PLAYLIST_H__
