/*
  Neutrino-GUI  -   DBoxII-Project

  Movieplayer (c) 2003, 2004 by gagga
  Based on code by Dirch, obi and the Metzler Bros. Thanks.

  $Id: bookmarkmanager.cpp,v 1.1 2004/02/05 01:05:30 gagga Exp $

  Homepage: http://www.giggo.de/dbox2/movieplayer.html

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
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/bookmarkmanager.h>

#include <global.h>
#include <neutrino.h>

#include <system/settings.h>

#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/stringinput.h>


#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


CBookmark::CBookmark(std::string inName, std::string inUrl, std::string inTime)
{
    name = inName;
    url = inUrl;
    time = inTime;
}

/*CBookmark::CBookmark()
{
    name = "";
    url = "";
    time = "";
}*/

CBookmark::~CBookmark() {
}


std::string CBookmark:: getName() {
    return name;
}

std::string CBookmark:: getUrl() {
    return url;
}

std::string CBookmark:: getTime() {
    return time;
}

void CBookmark:: setName(std::string inName) {
    name = inName;
}
void CBookmark:: setUrl(std::string inUrl) {
    url = inUrl;
}

void CBookmark:: setTime(std::string inTime) {
    time = inTime;
}

//------------------------------

int CBookmarkManager::addBookmark (CBookmark inBookmark) {
    if (bookmarks.size() < MAXBOOKMARKS) {
        bookmarks.push_back(inBookmark);
        printf("CBookmarkManager: addBookmark: %s\n",inBookmark.getName().c_str());
        bookmarksmodified = true;
        return 0;
    }
    // TODO:show dialog to delete old bookmark
    return -1;    
}

//------------------------------------------------------------------------

int CBookmarkManager::createBookmark (CBookmark inBookmark) {
    addBookmark(inBookmark);
    return 0;
}

int CBookmarkManager::createBookmark (std::string name, std::string url, std::string time) {
    CBookmark *bookmark = new CBookmark (name, url, time);
    return addBookmark(*bookmark);    
}

int CBookmarkManager::createBookmark (std::string url, std::string time) {
    char *bookmarkname="";
    CStringInputSMS * bookmarkname_input = new CStringInputSMS("movieplayer.bookmarkname", bookmarkname, 25, "movieplayer.bookmarkname_hint1", "movieplayer.bookmarkname_hint1", "abcdefghijklmnopqrstuvwxyz0123456789-_");
    bookmarkname_input->exec(NULL, "");
    std::string *namestring = new std::string(bookmarkname);
    // TODO: return -1 if no name was entered
    CBookmark *bookmark = new CBookmark (*namestring, url, time);
    return addBookmark(*bookmark);
    
}

//------------------------------------------------------------------------
void CBookmarkManager::readBookmarkFile() {
	if(bookmarkfile.loadConfig(BOOKMARKFILE)) {
    	bookmarksmodified = false;
    	bookmarks.clear();
        int bookmarkcount = bookmarkfile.getInt32( "bookmarkcount", 0 );
        printf("CBookmarkManager: read bookmarkcount:%d\n",bookmarkcount);
        if (bookmarkcount > MAXBOOKMARKS) bookmarkcount = MAXBOOKMARKS;
        //TODO: change to iterator
        for (int i=0;i<bookmarkcount;i++) {
            char counterstring[4];
            sprintf(counterstring, "%d",(i+1));
            std::string bookmarkstring = "bookmark";
            bookmarkstring += counterstring;
            std::string bookmarkname = bookmarkfile.getString(bookmarkstring + ".name","");
            std::string bookmarkurl = bookmarkfile.getString(bookmarkstring + ".url","");
            std::string bookmarktime =bookmarkfile.getString(bookmarkstring + ".time","");
            
            CBookmark *bookmark = new CBookmark(bookmarkname, bookmarkurl, bookmarktime);
            bookmarks.push_back(*bookmark);
            printf("CBookmarkManager: read bookmarkname: %s\n",bookmarks[i].getName().c_str());
            printf("CBookmarkManager: read bookmarkurl: %s\n",bookmarks[i].getUrl().c_str());
            printf("CBookmarkManager: read bookmarktime: %s\n",bookmarks[i].getTime().c_str());
        }
    }
}

//------------------------------------------------------------------------
void CBookmarkManager::writeBookmarkFile() {
    printf("CBookmarkManager: Writing bookmark file\n");
    //TODO: change to iterator
    for (unsigned int i=0;i<bookmarks.size();i++) {
        char counterstring[4];
        sprintf(counterstring, "%d",(i+1));
        std::string bookmarkstring = "bookmark";
        bookmarkstring.append(counterstring);
        std::string bookmarknamestring = bookmarkstring + ".name";
        std::string bookmarkurlstring = bookmarkstring + ".url";
        std::string bookmarktimestring = bookmarkstring + ".time";
        bookmarkfile.setString(bookmarknamestring,bookmarks[i].getName());
        bookmarkfile.setString(bookmarkurlstring,bookmarks[i].getUrl());
        bookmarkfile.setString(bookmarktimestring,bookmarks[i].getTime());
    }
    bookmarkfile.setInt32("bookmarkcount", bookmarks.size());
    bookmarkfile.saveConfig(BOOKMARKFILE);
}

//------------------------------------------------------------------------

/*bookmark_struct CBookmarkManager::getBookmark()
{
    //TODO: show userinterface
    return null;    
}
*/

//------------------------------------------------------------------------

CBookmarkManager::CBookmarkManager() : bookmarkfile ('\t')
{
    bookmarks.clear();
    readBookmarkFile();
}

//------------------------------------------------------------------------

CBookmarkManager::~CBookmarkManager () {
    flush();   
}

//------------------------------------------------------------------------

int CBookmarkManager::getBookmarkCount() {
    return bookmarks.size();
}

//------------------------------------------------------------------------

int CBookmarkManager::getMaxBookmarkCount() {
    return MAXBOOKMARKS;    
}

//------------------------------------------------------------------------

void CBookmarkManager::flush() {
    if (bookmarksmodified) {
        writeBookmarkFile();   
    }   
}

