 /***************************************************************************
	Neutrino-GUI  -   DBoxII-Project

 	Homepage: http://dbox.cyberphoria.org/

	$Id: movieinfo.cpp,v 1.20 2009/10/01 20:02:17 seife Exp $

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

	***********************************************************

	Module Name: movieinfo.cpp .

	Description: Implementation of the CMovieInfo class
	             This class loads, saves and shows the movie Information from the any .xml File on HD

	Date:	  Nov 2005

	Author: Günther@tuxbox.berlios.org
	Copyright (C) 2009 Stefan Seyfried

****************************************************************************/			
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <system/helper.h>
#include <gui/widget/msgbox.h>
#include <gui/movieinfo.h>
#include <zapit/client/zapittools.h> /* ZapitTools::Latin1_to_UTF8 */
#include <driver/encoding.h>
#define TRACE printf
#define VLC_URI "vlc://"

/************************************************************************

************************************************************************/
CMovieInfo::CMovieInfo()
{
	//TRACE("[mi] new\r\n");
}

CMovieInfo::~CMovieInfo()
{
	//TRACE("[mi] del\r\n");
	;
}

/************************************************************************

************************************************************************/
bool CMovieInfo::convertTs2XmlName(char* char_filename,int size)
{
   bool result = false;
 	std::string filename = char_filename;
	if( convertTs2XmlName(&filename) == true)
	{
		strncpy(char_filename,filename.c_str(),size);
		char_filename[size-1] = 0;
		result = true;
 	}
	return(result);
}

/************************************************************************

************************************************************************/
bool CMovieInfo::convertTs2XmlName(std::string* filename)
{
//TRACE("[mi]->convertTs2XmlName\r\n");
	int bytes = filename->find(".ts");
	bool result = false;

	if(bytes != -1)
	{
		if(bytes > 3)
		{
			if((*filename)[bytes-4] == '.') // FileName.001.ts
			{
				bytes = bytes-4;
			}
			else if((*filename)[bytes-3] == '_') // fix for udrec support: FileName_01.ts , but why the hell do they use another format ????
			{
				bytes = bytes-3;
			}// fix end
		}
		*filename = filename->substr(0, bytes) + ".xml";
		result = true;
	}
	else // not a TS file, return!!!!!
	{
		TRACE("not a TS file: %s\n ",filename->c_str());
	}

	return(result);
}

/************************************************************************

************************************************************************/
#define XML_ADD_TAG_STRING(_xml_text_,_tag_name_,_tag_content_){ \
	_xml_text_ += "\t\t<"_tag_name_">"; \
	_xml_text_ += ZapitTools::UTF8_to_UTF8XML(_tag_content_.c_str()); \
	_xml_text_ += "</"_tag_name_">\n";}

#define XML_ADD_TAG_UNSIGNED(_xml_text_,_tag_name_,_tag_content_){\
	_xml_text_ +=	"\t\t<"_tag_name_">";\
	char _tmp_[50];\
	sprintf(_tmp_, "%u", _tag_content_);\
	_xml_text_ +=	_tmp_;\
	_xml_text_ +=	"</"_tag_name_">\n";}

#define XML_ADD_TAG_LONG(_xml_text_,_tag_name_,_tag_content_){\
	_xml_text_ +=	"\t\t<"_tag_name_">";\
	char _tmp_[50];\
	sprintf(_tmp_, "%llu", _tag_content_);\
	_xml_text_ +=	_tmp_;\
	_xml_text_ +=	"</"_tag_name_">\n";}

#define	XML_GET_DATA_STRING(_node_,_tag_,_string_dest_){\
	if(!strcmp(_node_->GetType(), _tag_))\
	{\
		if(_node_->GetData() != NULL)\
		{\
			_string_dest_ = _node_->GetData();\
		}\
	}}


bool CMovieInfo::encodeMovieInfoXml(std::string* extMessage,MI_MOVIE_INFO &movie_info)
{
	//TRACE("[mi]->encodeMovieInfoXml\r\n");
	char tmp[40];

	*extMessage =	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n";
	*extMessage +=	"<"MI_XML_TAG_NEUTRINO" commandversion=\"1\">\n";
	*extMessage +=	"\t<"MI_XML_TAG_RECORD" command=\"";
	*extMessage +=	"record";
	*extMessage +=  "\">\n";
	XML_ADD_TAG_STRING	(*extMessage, MI_XML_TAG_CHANNELNAME,		movie_info.epgChannel);
	XML_ADD_TAG_STRING	(*extMessage, MI_XML_TAG_EPGTITLE,			movie_info.epgTitle);
	XML_ADD_TAG_UNSIGNED(*extMessage, MI_XML_TAG_ID,				movie_info.epgId);
	XML_ADD_TAG_STRING	(*extMessage, MI_XML_TAG_INFO1,				movie_info.epgInfo1);
	XML_ADD_TAG_STRING	(*extMessage, MI_XML_TAG_INFO2,				movie_info.epgInfo2);
	XML_ADD_TAG_LONG	(*extMessage, MI_XML_TAG_EPGID,				movie_info.epgEpgId); // %llu
	XML_ADD_TAG_UNSIGNED(*extMessage, MI_XML_TAG_MODE,				movie_info.epgMode);//%d
	XML_ADD_TAG_UNSIGNED(*extMessage, MI_XML_TAG_VIDEOPID,			movie_info.epgVideoPid);//%u
	if(movie_info.audioPids.size()>0)
	{
		*extMessage +=	"\t\t<"MI_XML_TAG_AUDIOPIDS" selected=\"";
		sprintf(tmp, "%u", movie_info.audioPids[0].epgAudioPid); //pids.APIDs[i].pid);
		*extMessage += tmp;
		*extMessage	+=	"\">\n";

		for(unsigned int i = 0; i < movie_info.audioPids.size(); i++) // pids.APIDs.size()
		{
			*extMessage += "\t\t\t<"MI_XML_TAG_AUDIO" "MI_XML_TAG_PID"=\"";
			sprintf(tmp, "%u", movie_info.audioPids[i].epgAudioPid); //pids.APIDs[i].pid);
			*extMessage += tmp;
			*extMessage += "\" "MI_XML_TAG_NAME"=\"";
			*extMessage += ZapitTools::UTF8_to_UTF8XML(movie_info.audioPids[i].epgAudioPidName.c_str());
			*extMessage += "\"/>\n";
		}
		*extMessage += "\t\t</"MI_XML_TAG_AUDIOPIDS">\n";
	}
	XML_ADD_TAG_UNSIGNED(*extMessage, MI_XML_TAG_VTXTPID,			movie_info.epgVTXPID);//%u
	/*****************************************************
	 *	new tags										*/
	XML_ADD_TAG_UNSIGNED(*extMessage, MI_XML_TAG_GENRE_MAJOR,		movie_info.genreMajor);
	XML_ADD_TAG_UNSIGNED(*extMessage, MI_XML_TAG_GENRE_MINOR,		movie_info.genreMinor);
	XML_ADD_TAG_STRING	(*extMessage, MI_XML_TAG_SERIE_NAME,		movie_info.serieName);
	XML_ADD_TAG_UNSIGNED(*extMessage, MI_XML_TAG_LENGTH,			movie_info.length);
	XML_ADD_TAG_UNSIGNED(*extMessage, MI_XML_TAG_RECLENGTH,			movie_info.rec_length);
	XML_ADD_TAG_STRING	(*extMessage, MI_XML_TAG_PRODUCT_COUNTRY,	movie_info.productionCountry);
	XML_ADD_TAG_UNSIGNED(*extMessage, MI_XML_TAG_PRODUCT_DATE,		movie_info.productionDate);
	XML_ADD_TAG_UNSIGNED(*extMessage, MI_XML_TAG_QUALITY,			movie_info.quality);
	XML_ADD_TAG_UNSIGNED(*extMessage, MI_XML_TAG_PARENTAL_LOCKAGE,	movie_info.parentalLockAge);
	XML_ADD_TAG_UNSIGNED(*extMessage, MI_XML_TAG_DATE_OF_LAST_PLAY,	(unsigned int)movie_info.dateOfLastPlay);
	*extMessage +=	"\t\t<"MI_XML_TAG_BOOKMARK">\n";
	*extMessage +=	"\t";XML_ADD_TAG_UNSIGNED(*extMessage, MI_XML_TAG_BOOKMARK_START,	movie_info.bookmarks.start);
	*extMessage +=	"\t";XML_ADD_TAG_UNSIGNED(*extMessage, MI_XML_TAG_BOOKMARK_END,		movie_info.bookmarks.end);
	*extMessage +=	"\t";XML_ADD_TAG_UNSIGNED(*extMessage, MI_XML_TAG_BOOKMARK_LAST,	movie_info.bookmarks.lastPlayStop);
	for(int i =0 ; i < MI_MOVIE_BOOK_USER_MAX ; i++ )
	{
		if( movie_info.bookmarks.user[i].pos != 0 || i == 0)
		{
			// encode any valid book, at least 1
			*extMessage += "\t\t\t<"MI_XML_TAG_BOOKMARK_USER" "MI_XML_TAG_BOOKMARK_USER_POS"=\"";
			sprintf(tmp, "%d", movie_info.bookmarks.user[i].pos); //pids.APIDs[i].pid);
			*extMessage += tmp;
			*extMessage += "\" "MI_XML_TAG_BOOKMARK_USER_TYPE"=\"";
			sprintf(tmp, "%d", movie_info.bookmarks.user[i].length); //pids.APIDs[i].pid);
			*extMessage += tmp;
			*extMessage += "\" "MI_XML_TAG_BOOKMARK_USER_NAME"=\"";
			*extMessage += ZapitTools::UTF8_to_UTF8XML(movie_info.bookmarks.user[i].name.c_str());
			*extMessage += "\"/>\n";
		}
	}

	*extMessage +=	"\t\t</"MI_XML_TAG_BOOKMARK">\n";
	 /*****************************************************/

	*extMessage +=	"\t</"MI_XML_TAG_RECORD">\n";
	*extMessage +=	"</"MI_XML_TAG_NEUTRINO">\n";
	return true;
}

/************************************************************************

************************************************************************/
bool CMovieInfo::saveMovieInfo(MI_MOVIE_INFO& movie_info, CFile* file)
{
	//TRACE("[mi]->saveXml \r\n");
	bool result = true;
	std::string text;
	CFile file_xml;
	
	if(file == NULL)
	{
		file_xml.Name = movie_info.file.Name;
		result = convertTs2XmlName(&file_xml.Name);
		// result is always false for .vdr files...
	}
	else
	{
		file_xml.Name = file->Name;
	}
	TRACE("[mi] saveXml: %s\r\n",file_xml.Name.c_str());
	
	if( result == true )
	{
		// ...so we don't need to worry about what happens here in the vdr case
		result = encodeMovieInfoXml(&text,movie_info);
		if(result == true)
		{
			result = saveFile(file_xml,text.c_str(), text.size());// save
			if(result == false)
			{
				TRACE("[mi] saveXml: save error\r\n");
			}
		}
		else
		{
			TRACE("[mi] saveXml: encoding error\r\n");
		}
	}
	else
	{
		TRACE("[mi] saveXml: error\r\n");
	}
	return(result);
}

/************************************************************************

************************************************************************/
bool CMovieInfo::loadMovieInfo( MI_MOVIE_INFO* movie_info, CFile* file )
{
	//TRACE("[mi]->loadMovieInfo \r\n");
	bool result = true;
	CFile file_xml;
	bool is_vdr = false;

	if(file == NULL)
	{
		// if there is no give file, we use the file name from movie info but we have to convert the ts name to xml name first
		file_xml.Name = movie_info->file.Name;
		int pos = file_xml.Name.rfind(".vdr");
		if (pos == (int)file_xml.Name.length() - 4)
		{
			is_vdr = true;
			file_xml.Name = movie_info->file.getPath() + "/info.vdr";
			result = !access(file_xml.Name.c_str(), R_OK);
		}
		else
		{
			result = convertTs2XmlName(&file_xml.Name);
		}
	}
	else
	{
		file_xml.Name = file->Name;
	}
	
	if(result == true)
	{
		// load xml file in buffer
		char text[6000];
		result = loadFile(file_xml,text, 6000);
		if(result == true)  
		{
			if (is_vdr)
			{
				result = parseInfoVDR(text, movie_info);
			}
			else
			{
#ifdef XMLTREE_LIB
				a result = parseXmlTree(text, movie_info);
#else /* XMLTREE_LIB */
				result = parseXmlQuickFix(text, movie_info);
#endif /* XMLTREE_LIB */
			}
		}
	}
    if(movie_info->productionDate >50 && movie_info->productionDate <200)// backwardcompaibility
        movie_info->productionDate += 1900;

	return (result);
}	

/************************************************************************

************************************************************************/
bool CMovieInfo::parseXmlTree (char* /*text*/, MI_MOVIE_INFO* /*movie_info*/)
{
#ifndef XMLTREE_LIB
	return (false); // no XML lib available return false
#else /* XMLTREE_LIB */

	int helpIDtoLoad = 80;

	XMLTreeParser *parser=new XMLTreeParser("ISO-8859-1");

	if (!parser->Parse(text, strlen(text), 1))
	{
		TRACE("parse error: %s at line %d \r\n", parser->ErrorString(parser->GetErrorCode()), parser->GetCurrentLineNumber());
		fclose(in);
		delete parser;
		return (false);
	}

	XMLTreeNode *root=parser->RootNode();
	if (!root)
	{
		TRACE(" root error \r\n");
		return (false);
	}
	
	if (strcmp(root->GetType(), MI_XML_TAG_NEUTRINO))
	{
		TRACE("not neutrino file. %s",root->GetType());
		return (false);
	}

	XMLTreeNode *node = parser->RootNode();
	
	for (node=node->GetChild(); node; node=node->GetNext())
	{
		if (!strcmp(node->GetType(), MI_XML_TAG_RECORD))
		{
			for (XMLTreeNode *xam1=node->GetChild(); xam1; xam1=xam1->GetNext())
			{
				XML_GET_DATA_STRING	(xam1, MI_XML_TAG_CHANNELNAME,	movie_info->epgChannel);
				XML_GET_DATA_STRING	(xam1, MI_XML_TAG_EPGTITLE,		movie_info->epgTitle);
				XML_GET_DATA_INT	(xam1, MI_XML_TAG_ID,			movie_info->epgId);
				XML_GET_DATA_STRING	(xam1, MI_XML_TAG_INFO1,		movie_info->epgInfo1);
				XML_GET_DATA_STRING	(xam1, MI_XML_TAG_INFO2,		movie_info->epgInfo2);
				XML_GET_DATA_LONGLONG	(xam1, MI_XML_TAG_EPGID,		movie_info->epgEpgId); // %llu
				XML_GET_DATA_INT	(xam1, MI_XML_TAG_MODE,			movie_info->epgMode);//%d
				XML_GET_DATA_INT	(xam1, MI_XML_TAG_VIDEOPID,		movie_info->epgVideoPid);//%u

				if (!strcmp(xam1->GetType(), MI_XML_TAG_AUDIOPIDS))
				{
					for (XMLTreeNode *xam2=xam1->GetChild(); xam2; xam2=xam2->GetNext())
					{
						if (!strcmp(xam2->GetType(), MI_XML_TAG_AUDIO))
						{
							EPG_AUDIO_PIDS pids;
							pids.epgAudioPid	=	atoi(xam2->GetAttributeValue(MI_XML_TAG_PID));
							pids.epgAudioPidName=	xam2->GetAttributeValue(MI_XML_TAG_NAME);
							movie_info->audioPids.push_back(pids);
						}
					}
				}
				XML_GET_DATA_INT	(xam1, MI_XML_TAG_VTXTPID,			movie_info->epgVTXPID);//%u
				/*****************************************************
				 *	new tags										*/
				XML_GET_DATA_INT	(xam1, MI_XML_TAG_GENRE_MAJOR,		movie_info->genreMajor);
				XML_GET_DATA_INT	(xam1, MI_XML_TAG_GENRE_MINOR,		movie_info->genreMinor);
				XML_GET_DATA_STRING	(xam1, MI_XML_TAG_SERIE_NAME,		movie_info->serieName);
				XML_GET_DATA_INT	(xam1, MI_XML_TAG_LENGTH,			movie_info->length);
				XML_GET_DATA_INT	(xam1, MI_XML_TAG_RECLENGTH,		movie_info->rec_length);
				XML_GET_DATA_STRING	(xam1, MI_XML_TAG_PRODUCT_COUNTRY,	movie_info->productionCountry);
				//if(!strcmp(xam1->GetType(), MI_XML_TAG_PRODUCT_COUNTRY)) if(xam1->GetData() != NULL)strncpy(movie_info->productionCountry, xam1->GetData(),4);	
				XML_GET_DATA_INT	(xam1, MI_XML_TAG_PRODUCT_DATE,		movie_info->productionDate);
				XML_GET_DATA_INT	(xam1, MI_XML_TAG_QUALITY,			movie_info->quality);
				XML_GET_DATA_INT	(xam1, MI_XML_TAG_PARENTAL_LOCKAGE,	movie_info->parentalLockAge);
				XML_GET_DATA_INT	(xam1, MI_XML_TAG_DATE_OF_LAST_PLAY,movie_info->dateOfLastPlay);
				
				if (!strcmp(xam1->GetType(), MI_XML_TAG_BOOKMARK))
				{
					for (XMLTreeNode *xam2=xam1->GetChild(); xam2; xam2=xam2->GetNext())
					{
						XML_GET_DATA_INT(xam2, MI_XML_TAG_BOOKMARK_START,	movie_info->bookmarks.start);
						XML_GET_DATA_INT(xam2, MI_XML_TAG_BOOKMARK_END,	movie_info->bookmarks.end);
						XML_GET_DATA_INT(xam2, MI_XML_TAG_BOOKMARK_LAST,	movie_info->bookmarks.lastPlayStop);
					}
				}
				/*****************************************************/
			}
		}
	}

	delete parser;
#endif /* XMLTREE_LIB */
	return(true);
}


#ifdef ENABLE_MOVIEPLAYER
/************************************************************************

************************************************************************/
void CMovieInfo::showMovieInfo(const char* filename)
{
	if(filename == NULL)
		return;
	
	MI_MOVIE_INFO movie_info;
	movie_info.file.Name = filename;
	if(loadMovieInfo(&movie_info ) == true)
		showMovieInfo(movie_info);
}

/************************************************************************

************************************************************************/
void CMovieInfo::showMovieInfo(MI_MOVIE_INFO& movie_info)
{
    std::string print_buffer; 
	tm* date_tm;
	char date_char[100];			
    // prepare print buffer  
    print_buffer = movie_info.epgInfo1; 
    print_buffer += "\n"; 
   	print_buffer += movie_info.epgInfo2; 

	 if( movie_info.productionCountry.size() != 0 || movie_info.productionDate != 0)
	 {
		print_buffer += "\n"; 
  		print_buffer += movie_info.productionCountry; 
		print_buffer += " "; 
		snprintf(date_char, 12,"%4d",movie_info.productionDate + 1900); 
		print_buffer += date_char; 
     }

    if(!movie_info.serieName.empty())
    {
    	print_buffer += "\n\n"; 
    	print_buffer += g_Locale->getText(LOCALE_MOVIEBROWSER_INFO_SERIE);
     	print_buffer += ": "; 
    	print_buffer += movie_info.serieName; 
    }
    if(!movie_info.epgChannel.empty())
    {
    	print_buffer += "\n\n"; 
    	print_buffer += g_Locale->getText(LOCALE_MOVIEBROWSER_INFO_CHANNEL);
      	print_buffer += ": "; 
    	print_buffer += movie_info.epgChannel; 
    }
    if(movie_info.quality != 0 )
    {
    	print_buffer += "\n"; 
    	print_buffer += g_Locale->getText(LOCALE_MOVIEBROWSER_INFO_QUALITY);
      	print_buffer += ": "; 
 		snprintf(date_char, 12,"%2d",movie_info.quality); 
    	print_buffer += date_char; 
    }
     if(movie_info.parentalLockAge != 0 )
    {
    	print_buffer += "\n"; 
   		print_buffer += g_Locale->getText(LOCALE_MOVIEBROWSER_INFO_PARENTAL_LOCKAGE);
      	print_buffer += ": "; 
 		snprintf(date_char, 12,"%2d",movie_info.parentalLockAge); 
    	print_buffer += date_char; 
 		print_buffer += " Jahre"; 
    }
     if(movie_info.length != 0 )
    {
    	print_buffer += "\n"; 
   		print_buffer += g_Locale->getText(LOCALE_MOVIEBROWSER_INFO_LENGTH);
      	print_buffer += ": "; 
 	 	snprintf(date_char, 12,"%3d",movie_info.length);
    	print_buffer += date_char; 
    }
     if(movie_info.rec_length != 0 )
    {
    	print_buffer += "\n"; 
   		print_buffer += g_Locale->getText(LOCALE_MOVIEBROWSER_INFO_LENGTH);
      	print_buffer += " rec: "; 
		snprintf(date_char, 12,"%3d:%02d",movie_info.rec_length / 60, movie_info.rec_length % 60);
    	print_buffer += date_char; 
    }
    if(movie_info.audioPids.size() != 0 )
     {
	    print_buffer += "\n"; 
   		print_buffer += g_Locale->getText(LOCALE_MOVIEBROWSER_INFO_AUDIO);
      	print_buffer += ": "; 
 	    for(unsigned int i=0; i < movie_info.audioPids.size() ; i++)
	    {
	     	print_buffer += movie_info.audioPids[i].epgAudioPidName; 
	     	print_buffer += ", ";
	    }
     }
     
    print_buffer += "\n\n"; 
   	print_buffer += g_Locale->getText(LOCALE_MOVIEBROWSER_INFO_PREVPLAYDATE);
      	print_buffer += ": "; 
  		 date_tm = localtime(&movie_info.dateOfLastPlay);
		snprintf(date_char, 12,"%02d.%02d.%04d",date_tm->tm_mday,date_tm->tm_mon+1,date_tm->tm_year +1900);
     	print_buffer += date_char; 
    print_buffer += "\n"; 
    print_buffer += g_Locale->getText(LOCALE_MOVIEBROWSER_INFO_RECORDDATE);
      	print_buffer += ": "; 
 		date_tm = localtime(&movie_info.file.Time);
		snprintf(date_char, 12,"%02d.%02d.%04d",date_tm->tm_mday,date_tm->tm_mon+1,date_tm->tm_year +1900);
    	print_buffer += date_char; 
      if(movie_info.file.Size != 0 )
    {
 		print_buffer += "\n"; 
   		print_buffer += g_Locale->getText(LOCALE_MOVIEBROWSER_INFO_SIZE);
     	print_buffer += ": "; 
 	 	snprintf(date_char, 12,"%4llu",movie_info.file.Size>>20);
		print_buffer += date_char; 
   }
    print_buffer += "\n" ;
        print_buffer += g_Locale->getText(LOCALE_MOVIEBROWSER_INFO_PATH);
          print_buffer += ": ";
        print_buffer += movie_info.file.Name;
 
     ShowMsg2UTF( 	movie_info.epgTitle.empty()? movie_info.file.getFileName().c_str() : movie_info.epgTitle.c_str() , 
					print_buffer.c_str(), 
	      			CMsgBox::mbrBack, 
					CMsgBox::mbBack); // UTF-8*/ 
  
} 
#endif
/************************************************************************

************************************************************************/
void CMovieInfo::printDebugMovieInfo(MI_MOVIE_INFO& movie_info)
{
	TRACE(" FileName: %s", movie_info.file.Name.c_str() );	
	//TRACE(" FilePath: %s", movie_info.file.GetFilePath );	
	//TRACE(" FileLength: %d", movie_info.file.GetLength );	
	//TRACE(" FileStatus: %d", movie_info.file.GetStatus );	

	TRACE(" ********** Movie Data ***********\r\n"); 		// (date, month, year)
	TRACE(" dateOfLastPlay: \t%ld\r\n", (long)movie_info.dateOfLastPlay ); 		// (date, month, year)
	TRACE(" dirItNr: \t\t%d\r\n", movie_info.dirItNr );  // 
    TRACE(" genreMajor: \t\t%d\r\n", movie_info.genreMajor );		//genreMajor;				
    TRACE(" genreMinor: \t\t%d\r\n", movie_info.genreMinor );		//genreMinor;				
	TRACE(" length: \t\t%d\r\n", movie_info.length );				// (minutes)
	TRACE(" length rec: \t\t%d\r\n", movie_info.rec_length );				// (minutes)
	TRACE(" quality: \t\t%d\r\n", movie_info.quality ); 				// (3 stars: classics, 2 stars: very good, 1 star: good, 0 stars: OK)
	TRACE(" productionCount:\t>%s<\r\n", movie_info.productionCountry.c_str() );
	TRACE(" productionDate: \t%d\r\n", movie_info.productionDate ); 		// (Year)  years since 1900
	TRACE(" parentalLockAge: \t\t\t%d\r\n", movie_info.parentalLockAge ); 			// MI_PARENTAL_LOCKAGE (0,6,12,16,18)
	TRACE(" format: \t\t%d\r\n", movie_info.format );				// MI_VIDEO_FORMAT(16:9, 4:3)
	TRACE(" audio: \t\t%d\r\n", movie_info.audio );// MI_AUDIO (AC3, Deutsch, Englisch)
	TRACE(" epgId: \t\t%d\r\n",  movie_info.epgId );
	TRACE(" epgEpgId: \t\t%llu\r\n",  movie_info.epgEpgId );
	TRACE(" epgMode: \t\t%d\r\n",  movie_info.epgMode );
	TRACE(" epgVideoPid: \t\t%d\r\n",  movie_info.epgVideoPid ); 
	TRACE(" epgVTXPID: \t\t%d\r\n",	 movie_info.epgVTXPID );
	TRACE(" Size: \t\t%ld\r\n",	 (long)movie_info.file.Size>>20 );
	TRACE(" Date: \t\t%ld\r\n",	 (long)movie_info.file.Time );

	for(unsigned int i = 0; i < movie_info.audioPids.size(); i++) 
	{
		TRACE(" audioPid (%d): \t\t%d\r\n", i, movie_info.audioPids[i].epgAudioPid); 
		TRACE(" audioName(%d): \t\t>%s<\r\n", i, movie_info.audioPids[i].epgAudioPidName.c_str() );
	}

	TRACE(" epgTitle: \t\t>%s<\r\n", movie_info.epgTitle.c_str() );	
	TRACE(" epgInfo1:\t\t>%s<\r\n", movie_info.epgInfo1.c_str() );		//epgInfo1		
//	TRACE(" epgInfo2:\t\t\t>%s<\r\n", movie_info.epgInfo2.c_str() );		//epgInfo2
	TRACE(" epgChannel:\t\t>%s<\r\n", movie_info.epgChannel.c_str() );
	TRACE(" serieName:\t\t>%s<\r\n", movie_info.serieName.c_str() );  	// (name e.g. 'StarWars)

	TRACE(" bookmarks start: \t%d\r\n", movie_info.bookmarks.start );
	TRACE(" bookmarks end: \t%d\r\n", movie_info.bookmarks.end );
	TRACE(" bookmarks lastPlayStop: %d\r\n", movie_info.bookmarks.lastPlayStop );

	for(int i =0 ; i < MI_MOVIE_BOOK_USER_MAX ; i++ )
	{
		if( movie_info.bookmarks.user[i].pos != 0 || i == 0) 
		{
			TRACE(" bookmarks user, pos:%d, type:%d, name: >%s<\r\n", movie_info.bookmarks.user[i].pos ,movie_info.bookmarks.user[i].length,movie_info.bookmarks.user[i].name.c_str());
		}
	}
}


/************************************************************************

************************************************************************/
int find_next_char(char to_find,char* text,int start_pos, int end_pos)
{
	while(start_pos < end_pos)
	{
		if(text[start_pos] == to_find)
		{
			return(start_pos);
		}
		start_pos++;
	}
	return(-1);
}

std::string decodeXmlSpecialChars(std::string s)
{
	StrSearchReplace(s,"&lt;","<");
	StrSearchReplace(s,"&gt;",">");
	StrSearchReplace(s,"&amp;","&");
	StrSearchReplace(s,"&quot;","\"");
	StrSearchReplace(s,"&apos;","\'");
	return s;
}

#define GET_XML_DATA_STRING(_text_,_pos_,_tag_,_dest_)\
	if(strncmp(&_text_[_pos_],_tag_,sizeof(_tag_)-1) == 0)\
	{\
		_pos_ += sizeof(_tag_) ;\
		int pos_prev = _pos_;\
		while(_pos_ < bytes && _text_[_pos_] != '<' ) _pos_++;\
		_dest_ = "";\
		_dest_.append(&_text_[pos_prev],_pos_ - pos_prev );\
		_dest_ = decodeXmlSpecialChars(_dest_);\
		_pos_ += sizeof(_tag_);\
		continue;\
	}

#define GET_XML_DATA_INT(_text_,_pos_,_tag_,_dest_)\
	if(strncmp(&_text_[pos],_tag_,sizeof(_tag_)-1) == 0)\
	{\
		_pos_ += sizeof(_tag_) ;\
		int pos_prev = _pos_;\
		while(_pos_ < bytes && _text_[_pos_] != '<' ) pos++;\
		_dest_ = atoi(&_text_[pos_prev]);\
		continue;\
	}
#define GET_XML_DATA_LONG(_text_,_pos_,_tag_,_dest_)\
	if(strncmp(&_text_[pos],_tag_,sizeof(_tag_)-1) == 0)\
	{\
		_pos_ += sizeof(_tag_) ;\
		int pos_prev = _pos_;\
		while(_pos_ < bytes && _text_[_pos_] != '<' ) pos++;\
		_dest_ = atoll(&_text_[pos_prev]);\
		continue;\
	}

 /************************************************************************

************************************************************************/
bool CMovieInfo::parseXmlQuickFix(char* text, MI_MOVIE_INFO* movie_info)
{
	int bookmark_nr = 0;
	movie_info->dateOfLastPlay = 0;//100*366*24*60*60; 		// (date, month, year)
	//	bool result = false;//unused variable
	
	int bytes = strlen(text);
	/** search ****/ 
	int pos = 0;

	EPG_AUDIO_PIDS audio_pids;

	while( (pos = find_next_char('<',text,pos,bytes)) != -1)
	{
		pos++;
		GET_XML_DATA_STRING(text,	pos,	MI_XML_TAG_CHANNELNAME,		movie_info->epgChannel)		
		GET_XML_DATA_STRING(text,	pos,	MI_XML_TAG_EPGTITLE,		movie_info->epgTitle)		
		GET_XML_DATA_INT   (text,	pos,	MI_XML_TAG_ID,				movie_info->epgId)		
		GET_XML_DATA_STRING(text,	pos,	MI_XML_TAG_INFO1,			movie_info->epgInfo1)		
		GET_XML_DATA_STRING(text,	pos,	MI_XML_TAG_INFO2,			movie_info->epgInfo2)		
		GET_XML_DATA_LONG  (text,	pos,	MI_XML_TAG_EPGID,			movie_info->epgEpgId)		
		GET_XML_DATA_INT   (text,	pos,	MI_XML_TAG_MODE,			movie_info->epgMode)		
		GET_XML_DATA_INT   (text,	pos,	MI_XML_TAG_VIDEOPID,		movie_info->epgVideoPid)		
		GET_XML_DATA_STRING(text,	pos,	MI_XML_TAG_NAME,			movie_info->epgChannel)		
		GET_XML_DATA_INT   (text,	pos,	MI_XML_TAG_VTXTPID,			movie_info->epgVTXPID)		
		GET_XML_DATA_INT   (text,	pos,	MI_XML_TAG_GENRE_MAJOR,		movie_info->genreMajor)		
		GET_XML_DATA_INT   (text,	pos,	MI_XML_TAG_GENRE_MINOR,		movie_info->genreMinor)		
		GET_XML_DATA_STRING(text,	pos,	MI_XML_TAG_SERIE_NAME,		movie_info->serieName)		
		GET_XML_DATA_INT   (text,	pos,	MI_XML_TAG_LENGTH,			movie_info->length)		
		GET_XML_DATA_INT   (text,	pos,	MI_XML_TAG_RECLENGTH,		movie_info->rec_length)		
		GET_XML_DATA_STRING(text,	pos,	MI_XML_TAG_PRODUCT_COUNTRY,	movie_info->productionCountry)		
		GET_XML_DATA_INT   (text,	pos,	MI_XML_TAG_PRODUCT_DATE,	movie_info->productionDate)		
		GET_XML_DATA_INT   (text,	pos,	MI_XML_TAG_PARENTAL_LOCKAGE,	movie_info->parentalLockAge)		
		GET_XML_DATA_INT   (text,	pos,	MI_XML_TAG_QUALITY,			movie_info->quality)		
		GET_XML_DATA_INT   (text,	pos,	MI_XML_TAG_DATE_OF_LAST_PLAY,movie_info->dateOfLastPlay)		

		if(strncmp(&text[pos],MI_XML_TAG_AUDIOPIDS,sizeof(MI_XML_TAG_AUDIOPIDS)-1) == 0) pos += sizeof(MI_XML_TAG_AUDIOPIDS);

		/* parse audio pids */
		if(strncmp(&text[pos],MI_XML_TAG_AUDIO,sizeof(MI_XML_TAG_AUDIO)-1) == 0)
		{
			pos += sizeof(MI_XML_TAG_AUDIO);

			int pos2;

			pos2 = strcspn(&text[pos],MI_XML_TAG_PID);
			if( pos2 >= 0 )
			{
				pos2 += sizeof(MI_XML_TAG_PID); 
				while(text[pos+pos2] != '\"' && text[pos+pos2] != 0 && text[pos+pos2] != '/')pos2++;
				if(text[pos+pos2] == '\"') audio_pids.epgAudioPid = atoi(&text[pos+pos2+1]);
			}
			else
				audio_pids.epgAudioPid = 0;

			audio_pids.epgAudioPidName = "";
			pos2 = strcspn(&text[pos],MI_XML_TAG_NAME);
			if( pos2 >= 0 )
			{
				pos2 += sizeof(MI_XML_TAG_PID); 
				while(text[pos+pos2] != '\"' && text[pos+pos2] != 0 && text[pos+pos2] != '/')pos2++;
				if(text[pos+pos2] == '\"') 
				{
					int pos3=pos2+1;
					while(text[pos+pos3] != '\"' && text[pos+pos3] != 0 && text[pos+pos3] != '/')pos3++;
					if(text[pos+pos3] == '\"')
					{
						audio_pids.epgAudioPidName.append(&text[pos+pos2+1],pos3-pos2-1);
						audio_pids.epgAudioPidName = decodeXmlSpecialChars(audio_pids.epgAudioPidName);
					}
				}
			}
			movie_info->audioPids.push_back(audio_pids); 
		}
		/* parse bookmarks */
		GET_XML_DATA_INT   (text,	pos,	MI_XML_TAG_BOOKMARK_START,	movie_info->bookmarks.start)		
		GET_XML_DATA_INT   (text,	pos,	MI_XML_TAG_BOOKMARK_END,	movie_info->bookmarks.end)		
		GET_XML_DATA_INT   (text,	pos,	MI_XML_TAG_BOOKMARK_LAST,	movie_info->bookmarks.lastPlayStop)		

		if(bookmark_nr < MI_MOVIE_BOOK_USER_MAX)
		{
			if(strncmp(&text[pos],MI_XML_TAG_BOOKMARK_USER,sizeof(MI_XML_TAG_BOOKMARK_USER)-1) == 0)
			{
				pos += sizeof(MI_XML_TAG_BOOKMARK_USER);
				//int pos2 = strcspn(&text[pos],MI_XML_TAG_BOOKMARK_USER_POS);
				if( strcspn(&text[pos],MI_XML_TAG_BOOKMARK_USER_POS) == 0 )
				{
					int pos2=0;
					pos2 += sizeof(MI_XML_TAG_BOOKMARK_USER_POS); 
					while(text[pos+pos2] != '\"' && text[pos+pos2] != 0 && text[pos+pos2] != '/')pos2++;
					if(text[pos+pos2] == '\"')
					{
						movie_info->bookmarks.user[bookmark_nr].pos = atoi(&text[pos+pos2+1]);
						
						//pos2 = strcspn(&text[pos],MI_XML_TAG_BOOKMARK_USER_TYPE);
						pos++;
						while(text[pos+pos2] == ' ') pos++;
						if( strcspn(&text[pos],MI_XML_TAG_BOOKMARK_USER_TYPE) == 0 )
						{
							pos2 += sizeof(MI_XML_TAG_BOOKMARK_USER_TYPE); 
							while(text[pos+pos2] != '\"' && text[pos+pos2] != 0 && text[pos+pos2] != '/')pos2++;
							if(text[pos+pos2] == '\"')
							{
								 movie_info->bookmarks.user[bookmark_nr].length = atoi(&text[pos+pos2+1]);
								
								movie_info->bookmarks.user[bookmark_nr].name = "";
								//pos2 = ;
								if( strcspn(&text[pos],MI_XML_TAG_BOOKMARK_USER_NAME) == 0 )
								{
									pos2 += sizeof(MI_XML_TAG_BOOKMARK_USER_NAME); 
									while(text[pos+pos2] != '\"' && text[pos+pos2] != 0 && text[pos+pos2] != '/')pos2++;
									if(text[pos+pos2] == '\"') 
									{
										int pos3=pos2+1;
										while(text[pos+pos3] != '\"' && text[pos+pos3] != 0 && text[pos+pos3] != '/')pos3++;
										if(text[pos+pos3] == '\"')
										{
											movie_info->bookmarks.user[bookmark_nr].name.append(&text[pos+pos2+1],pos3-pos2-1);
											movie_info->bookmarks.user[bookmark_nr].name = decodeXmlSpecialChars(movie_info->bookmarks.user[bookmark_nr].name);
										}
									}
								}
							}
						}
						else
							movie_info->bookmarks.user[bookmark_nr].length = 0;
					}
					bookmark_nr++;					
				}
				else
					movie_info->bookmarks.user[bookmark_nr].pos = 0;
			}
		}
	}
	return(true);
}

static inline std::string convertVDRline(char *text, int len)
{
	std::string s;
	s = ((std::string)text).substr(2, len - 2);
	if (!isUTF8(s))
		s = Latin1_to_UTF8(s);
	return s;
}

bool CMovieInfo::parseInfoVDR(char* text, MI_MOVIE_INFO* movie_info)
{
	movie_info->dateOfLastPlay = 0;//100*366*24*60*60; 		// (date, month, year)
	char *nl = NULL;
	int epgid, start, length, table, ver;
	std::string tmp;
	int apidcounter = 0;
	int ac3counter = 32;

	EPG_AUDIO_PIDS audio_pids;

	while((nl = strchr(text, '\n')))
	{
		switch (*text)
		{
		case 'C':	// Channel ID
		case 'V':	// VPS time?
			break;
		case 'T':	// Title
			movie_info->epgTitle = convertVDRline(text, nl - text);
			break;
		case 'S':	// Short EPG
			movie_info->epgInfo1 = convertVDRline(text, nl - text);
			break;
		case 'D':	// Long EPG
			movie_info->epgInfo2 = convertVDRline(text, nl - text);
			StrSearchReplace(movie_info->epgInfo2, "|", "\n");
			break;
		case 'E':	// EPG ID, start time, duration
			tmp = convertVDRline(text, nl - text);
			if (sscanf(tmp.c_str(), "%d %d %d %x %x", &epgid, &start, &length, &table, &ver) == 5)
			{
				movie_info->file.Time = start;	//ugly, but there is no other field for the recording date
				movie_info->epgId = epgid;
				movie_info->length = length / 60;
			}
			else
				printf("COULD NOT CONVERT VDR LINE: '%s'\n", tmp.c_str());
			break;
		case 'X':
			/* X 1 03 deu Breitwand */
			if (*(text + 2) == '1')
			{
				movie_info->format = *(text + 5) - '1'; // 0 - 4:3, 2 - 16:9, not used anyway ATM.
				break;
			}
			/*
			   X 2 03 deu Stereo
			   X 2 03 deu Stereo
			   X 2 05 deu Dolby Digital 5.1
			 */
			if (*(text + 2) == '2')
			{
				bool found = true;
				unsigned int id;
				id = strtol(text + 4, NULL, 16);
				audio_pids.epgAudioPidName = convertVDRline(text + 5, nl - (text + 5));
				switch (id)
				{
				case 0x01: // mono
				case 0x03: // stereo
				case 0x04: // "audio description" on ARD
					audio_pids.epgAudioPid = apidcounter;
					apidcounter++;
					break;
				case 0x05:
					audio_pids.epgAudioPid = ac3counter;
					ac3counter++;
					break;
				// case 0x40: // unknown
				default:
					found = false;
					tmp = convertVDRline(text, nl - text);
					fprintf(stderr, "%s: invalid VDR line: '%s'\n", __FUNCTION__, tmp.c_str());
				}
				if (found)
					movie_info->audioPids.push_back(audio_pids);
			}
			break;
		default:
			break;
		}
		text = nl + 1;
		if (*text == 0)	// end of string
			break;
	}

	return true;
}

/************************************************************************

************************************************************************/
bool CMovieInfo::addNewBookmark(MI_MOVIE_INFO* movie_info,MI_BOOKMARK &new_bookmark)
{
	TRACE("[mi] addNewBookmark\r\n");
	bool result = false;
	if(movie_info != NULL)
	{
		// search for free entry 
		bool loop = true;
		for(int i=0; i<MI_MOVIE_BOOK_USER_MAX && loop == true; i++)
		{
			if(movie_info->bookmarks.user[i].pos == 0)
			{
				// empty entry found
				result = true;
				loop = false;
				movie_info->bookmarks.user[i].pos = new_bookmark.pos;
				movie_info->bookmarks.user[i].length = new_bookmark.length;
				if(movie_info->bookmarks.user[i].name.empty())
				{
					if(new_bookmark.length == 0) movie_info->bookmarks.user[i].name = g_Locale->getText(LOCALE_MOVIEBROWSER_BOOK_NEW);
					if(new_bookmark.length < 0) movie_info->bookmarks.user[i].name = g_Locale->getText(LOCALE_MOVIEBROWSER_BOOK_TYPE_BACKWARD);
					if(new_bookmark.length > 0) movie_info->bookmarks.user[i].name = g_Locale->getText(LOCALE_MOVIEBROWSER_BOOK_TYPE_FORWARD);
				}
				else
				{
					movie_info->bookmarks.user[i].name = new_bookmark.name;
				}
			}
		}
	}
	return(result);
}


/************************************************************************

************************************************************************/
void CMovieInfo::clearMovieInfo(MI_MOVIE_INFO* movie_info)
{
	//TRACE("[mi]->clearMovieInfo \r\n");
	tm timePlay;
	timePlay.tm_hour = 0;
	timePlay.tm_min = 0;
	timePlay.tm_sec = 0;
	timePlay.tm_year = 100;
	timePlay.tm_mday = 0;
	timePlay.tm_mon = 1;
	
	movie_info->file.Name = "";
	movie_info->file.Size = 0;					// Megabytes
	movie_info->file.Time = mktime(&timePlay); 
	movie_info->dateOfLastPlay = mktime(&timePlay); 		// (date, month, year)
	movie_info->dirItNr = 0;  // 
    movie_info->genreMajor = 0;		//genreMajor;				
    movie_info->genreMinor = 0;		//genreMinor;				
	movie_info->length = 0;				// (minutes)
	movie_info->rec_length = 0;				// (seconds)
	movie_info->quality = 0; 				// (3 stars: classics, 2 stars: very good, 1 star: good, 0 stars: OK)
	movie_info->productionDate = 0; 		// (Year)  years since 1900
	movie_info->parentalLockAge = 0; 			// MI_PARENTAL_LOCKAGE (0,6,12,16,18)
	movie_info->format = 0;				// MI_VIDEO_FORMAT(16:9, 4:3)
	movie_info->audio = 0;// MI_AUDIO (AC3, Deutsch, Englisch)

	movie_info->epgId = 0;
	movie_info->epgEpgId = 0;
	movie_info->epgMode = 0;
	movie_info->epgVideoPid = 0; 
	movie_info->epgVTXPID = 0;

	movie_info->audioPids.clear();

	movie_info->productionCountry = "";
	movie_info->epgTitle = "";	
	movie_info->epgInfo1 = "";		//epgInfo1		
	movie_info->epgInfo2 = "";		//epgInfo2
	movie_info->epgChannel = "";
	movie_info->serieName = "";  	// (name e.g. 'StarWars)
	movie_info->bookmarks.end = 0;
	movie_info->bookmarks.start = 0;
	movie_info->bookmarks.lastPlayStop = 0;
	for(int i = 0; i < MI_MOVIE_BOOK_USER_MAX; i++)
	{
		movie_info->bookmarks.user[i].pos = 0;
		movie_info->bookmarks.user[i].length = 0;
		movie_info->bookmarks.user[i].name = "";
	}
}

/************************************************************************

************************************************************************/
bool CMovieInfo::loadFile(CFile& file,char* buffer, int buffer_size)
{
	bool result = false;
#ifdef ENABLE_MOVIEPLAYER_VLC
	if (strncmp(file.getFileName().c_str(), VLC_URI, strlen(VLC_URI)) == 0)
	{
		result = loadFile_vlc(file, buffer,buffer_size);
	}
	else
#endif
	{
		result = loadFile_std(file, buffer,buffer_size);
	}
	return(result);
}

bool CMovieInfo::loadFile_std(CFile& file,char* buffer, int buffer_size)
{
	bool result = true;
	
   int fd = open(file.Name.c_str(), O_RDONLY); 
    if (fd == -1) // cannot open file, return!!!!! 
    { 
   		//TRACE( "[mi] !open:%s\r\n" ,file.getFileName().c_str());
   		TRACE("!");
		return(false); 
    } 
 
    //TRACE( "show_ts_info: File found (%s)\r\n" ,filename->c_str());
    // read file content to buffer 
    int bytes = read(fd, buffer, buffer_size-1); 
    if(bytes <= 0) // cannot read file into buffer, return!!!! 
    { 
		close(fd);
   		//TRACE( "[mi] !read:%s\r\n" ,file.getFileName().c_str() );
   		TRACE("#");
 		return(false); 
    } 
	close(fd);
	buffer[bytes]=0; // terminate string
	return(result);
}

#ifdef ENABLE_MOVIEPLAYER_VLC
bool CMovieInfo::loadFile_vlc(CFile& /*file*/, char* /*buffer*/, int /*buffer_size*/)
{
	bool result = false;
	return(result);
}
#endif

/************************************************************************

************************************************************************/
bool CMovieInfo::saveFile(const CFile& file, const char* text, const int text_size)
{
	bool result = false;
	std::string fn = file.getFileName();
#ifdef ENABLE_MOVIEPLAYER_VLC
	if (strncmp(fn.c_str(), VLC_URI, strlen(VLC_URI)) == 0)
	{
		result = saveFile_vlc(file, text,text_size);
	}
	// paranoia check, but saveFile should not even be called for .vdr files...
	else
#endif
	if (fn.rfind(".vdr") == fn.length() - 4 && fn.length() > 3)
	{
		fprintf(stderr, "ERROR! CMovieInfo::saveFile called for VDR file %s\n", fn.c_str());
		result = saveFile_vdr(file, text, text_size);
	}
	else
	{
		fprintf(stderr, "%s:%d saving TS movieinfo: %s\n", __FUNCTION__, __LINE__, fn.c_str());
		result = saveFile_std(file, text,text_size);
	}
	return(result);
}

bool CMovieInfo::saveFile_std(const CFile& file, const char* text, const int text_size)
{
	bool result = false;
	int fd;
	if ((fd = open(file.Name.c_str(), O_SYNC | O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) >= 0)
	{
		int nr;
		nr = write(fd, text, text_size);
		//fdatasync(fd);
		close(fd);
		result = true;
		//TRACE("[mi] saved (%d)\r\n",nr);
	}
	else
	{
		//TRACE("[mi] ERROR: cannot open\r\n");
		TRACE("?");
	}
 	return(result);
}

#ifdef ENABLE_MOVIEPLAYER_VLC
bool CMovieInfo::saveFile_vlc(const CFile& /*file*/, const char* /*text*/, const int /*text_size*/)
{
	bool result = false;
	return(result);
}
#endif

bool CMovieInfo::saveFile_vdr(const CFile& , const char* , const int)
{
	return false;
}



/* 	char buf[2048];
	
	int done;
	do
	{
		unsigned int len=fread(buf, 1, sizeof(buf), in);
		done=len<sizeof(buf);
		if (!parser->Parse(buf, len, 1))
		{
			TRACE("parse error: %s at line %d \r\n", parser->ErrorString(parser->GetErrorCode()), parser->GetCurrentLineNumber());
			fclose(in);
			delete parser;
			return (false);
		}
	} while (!done);
	fclose(in);
 * 
 * */
