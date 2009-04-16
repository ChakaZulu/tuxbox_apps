
/***************************************************************************
	Neutrino-GUI  -   DBoxII-Project

	Homepage: http://dbox.cyberphoria.org/

	$Id: textbox.cpp,v 1.6 2009/04/16 18:40:11 rhabarber1848 Exp $

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

	Module Name: textbox.cpp

	Description: implementation of the CTextBox class
				 This class provides a plain textbox with selectable features:
				 	- Foot, Title
				 	- Scroll bar
				 	- Frame shadow
				 	- Auto line break
				 	- fixed position or auto width and auto height (later not tested yet)
				 	- Center Text

	Date:	Nov 2005

	Author: Günther@tuxbox.berlios.org
		based on code of Steffen Hehn 'McClean'

	$Log: textbox.cpp,v $
	Revision 1.6  2009/04/16 18:40:11  rhabarber1848
	fix movieinfo display, patch by Gaucho316: http://tuxbox-forum.dreambox-fan.de/forum/viewtopic.php?p=366884#p366884
	
	Revision 1.5  2009/03/29 16:23:11  seife
	widgets: fix shadow warnings
	
	Revision 1.4  2009/01/10 18:13:39  seife
	Make the scrollbar of the textbox consistent with the rest of neutrino
	(width 15 pixels, rounded corners)
	
	Revision 1.3  2009/01/10 01:43:34  seife
	Fix the textbox swallowing the last character, if it is not a newline.
	
	Revision 1.2  2006/02/20 01:10:36  guenther
	- temporary parental lock updated - remove 1s debug prints in movieplayer- Delete file without rescan of movies- Crash if try to scroll in list with 2 movies only- UTF8XML to UTF8 conversion in preview- Last file selection recovered- use of standard folders adjustable in config- reload and remount option in config
	

****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "textbox.h"
#include <gui/widget/icons.h>

#define	TEXT_BORDER_WIDTH			 8
#define	SCROLL_FRAME_WIDTH			15
#define	SCROLL_MARKER_BORDER		 2

#define MAX_WINDOW_WIDTH  (g_settings.screen_EndX - g_settings.screen_StartX - 40)
#define MAX_WINDOW_HEIGHT (g_settings.screen_EndY - g_settings.screen_StartY - 40)	

#define MIN_WINDOW_WIDTH  ((g_settings.screen_EndX - g_settings.screen_StartX)>>1)
#define MIN_WINDOW_HEIGHT 40	

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

bool UTF8XMLtoUTF8(std::string* text);


//////////////////////////////////////////////////////////////////////
// Function Name:	CTextBox	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
CTextBox::CTextBox(const char *text,
		   Font *font_text,
		   const int _mode,
		   const CBox *position,
		   CFBWindow::color_t textBackgroundColor)
{
	//TRACE("[CTextBox] new\r\n");
	initVar();

	m_pcWindow = NULL;

 	if(text != NULL)		m_cText = text;
	if(font_text != NULL)	m_pcFontText = font_text;
	if(position != NULL)
	{
		m_cFrame	= *position;
		m_nMaxHeight = m_cFrame.iHeight;
		m_nMaxWidth = m_cFrame.iWidth;
	}

	m_nMode = _mode;

	/* in case of auto line break, we do no support auto width  yet */
	if (!(_mode & NO_AUTO_LINEBREAK))
	{
		m_nMode = m_nMode & ~AUTO_WIDTH; /* delete any AUTO_WIDTH*/
	}

#if 0
	TRACE("  Mode: ");
	if(_mode & SCROLL) TRACE("SCROLL ");
	if(_mode & NO_AUTO_LINEBREAK) TRACE("NO_AUTO_LINEBREAK ");
	if(_mode & AUTO_WIDTH) TRACE("AUTO_WIDTH ");
	if(_mode & AUTO_HIGH) TRACE("AUTO_HIGH");
	TRACE("\r\n");

#endif
	//TRACE(" CTextBox::m_cText: %d, m_nMode %d\t\r\n",m_cText.size(),m_nMode);

	m_textBackgroundColor = textBackgroundColor;
	m_nFontTextHeight  = m_pcFontText->getHeight();
	//TRACE(" CTextBox::m_nFontTextHeight: %d\t\r\n",m_nFontTextHeight);

	/* Initialise the window frames first */
	initFramesRel();

	// than refresh text line array 
	refreshTextLineArray();
}


//////////////////////////////////////////////////////////////////////
// Function Name:	CTextBox	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
CTextBox::CTextBox(const char * text)
{
	//TRACE("[CTextBox] new\r\n");
	initVar();

	m_pcWindow = NULL;
	if(text != NULL)
	{
		m_cText = *text;
		UTF8XMLtoUTF8(&m_cText); // remove UTF8XML tags
	}

	/* Initialise the window frames first */
	initFramesRel();

	// than refresh text line array 
	refreshTextLineArray();
}

//////////////////////////////////////////////////////////////////////
// Function Name:	CTextBox	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
CTextBox::CTextBox()
{
	//TRACE("[CTextBox] new\r\n");
	initVar();
	initFramesRel();

	m_pcWindow = NULL;
}

//////////////////////////////////////////////////////////////////////
// Function Name:	~CTextBox	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
CTextBox::~CTextBox()
{
	//TRACE("[CTextBox] del\r\n");
	m_cLineArray.clear();
	hide();

}

//////////////////////////////////////////////////////////////////////
// Variable initialisation
//////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////
// Function Name:	InitVar	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CTextBox::initVar(void)
{
	//TRACE("[CTextBox]->InitVar\r\n");
	m_cText	= "";
	m_nMode = SCROLL;

	m_pcFontText = NULL;
	m_pcFontText  =  g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1];
	m_nFontTextHeight = m_pcFontText->getHeight();

	m_nNrOfPages = 1;
	m_nNrOfLines = 0;
	m_nLinesPerPage = 0;
	m_nCurrentLine = 0;
	m_nCurrentPage = 0;

	m_cFrame.iX		= g_settings.screen_StartX + ((g_settings.screen_EndX - g_settings.screen_StartX - MIN_WINDOW_WIDTH) >>1);
	m_cFrame.iWidth	= MIN_WINDOW_WIDTH;
	m_cFrame.iY		= g_settings.screen_StartY + ((g_settings.screen_EndY - g_settings.screen_StartY - MIN_WINDOW_HEIGHT) >>1);
	m_cFrame.iHeight	= MIN_WINDOW_HEIGHT;

	m_nMaxHeight = MAX_WINDOW_HEIGHT;
	m_nMaxWidth = MAX_WINDOW_WIDTH;
	
	m_textBackgroundColor = COL_MENUCONTENT_PLUS_0;

	m_cLineArray.clear();
}

//////////////////////////////////////////////////////////////////////
// Function Name:	ReSizeMainFrameWidth	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CTextBox::reSizeMainFrameWidth(int textWidth)
{
	//TRACE("[CTextBox]->ReSizeMainFrameWidth: %d, current: %d\r\n",textWidth,m_cFrameTextRel.iWidth);

	int iNewWindowWidth =	textWidth  
							+ m_cFrameScrollRel.iWidth   
							+ 2*TEXT_BORDER_WIDTH;

	if( iNewWindowWidth > m_nMaxWidth) iNewWindowWidth = m_nMaxWidth;
	if( iNewWindowWidth < MIN_WINDOW_WIDTH) iNewWindowWidth = MIN_WINDOW_WIDTH;

	m_cFrame.iWidth	= iNewWindowWidth;

	/* Re-Init the children frames due to new main window */
	initFramesRel();
}

//////////////////////////////////////////////////////////////////////
// Function Name:	ReSizeMainFrameHeight	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CTextBox::reSizeMainFrameHeight(int textHeight)
{
	TRACE("[CTextBox]->ReSizeMainFrameHeight: %d, current: %d\r\n",textHeight,m_cFrameTextRel.iHeight);

	int iNewWindowHeight =	textHeight 
							+ 2*TEXT_BORDER_WIDTH;

	if( iNewWindowHeight > m_nMaxHeight) iNewWindowHeight = m_nMaxHeight;
	if( iNewWindowHeight < MIN_WINDOW_HEIGHT) iNewWindowHeight = MIN_WINDOW_HEIGHT;

	m_cFrame.iHeight	= iNewWindowHeight;

	/* Re-Init the children frames due to new main window */
	initFramesRel();
}

//////////////////////////////////////////////////////////////////////
// Function Name:	InitFramesRel	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CTextBox::initFramesRel(void)
{
	//TRACE("[CTextBox]->InitFramesRel\r\n");

	m_cFrameTextRel.iX		= 0;
	m_cFrameTextRel.iY		= 0;
	m_cFrameTextRel.iHeight	= m_cFrame.iHeight ;
	
	if(m_nMode & SCROLL)
	{
		m_cFrameScrollRel.iX		= m_cFrame.iWidth - SCROLL_FRAME_WIDTH;
		m_cFrameScrollRel.iY		= m_cFrameTextRel.iY;
		m_cFrameScrollRel.iWidth	= SCROLL_FRAME_WIDTH;
		m_cFrameScrollRel.iHeight	= m_cFrameTextRel.iHeight;
	}
	else
	{
		m_cFrameScrollRel.iX		= 0;
		m_cFrameScrollRel.iY		= 0;
		m_cFrameScrollRel.iHeight	= 0;
		m_cFrameScrollRel.iWidth	= 0;
	}

	m_cFrameTextRel.iWidth	= m_cFrame.iWidth - m_cFrameScrollRel.iWidth;

	m_nLinesPerPage = (m_cFrameTextRel.iHeight - (2*TEXT_BORDER_WIDTH)) / m_nFontTextHeight;

#if 0
	TRACE_1("Frames\r\n\tScren:\t%3d,%3d,%3d,%3d\r\n\tMain:\t%3d,%3d,%3d,%3d\r\n\tText:\t%3d,%3d,%3d,%3d \r\n\tScroll:\t%3d,%3d,%3d,%3d \r\n",
		g_settings.screen_StartX,
		g_settings.screen_StartY,
		g_settings.screen_EndX,
		g_settings.screen_EndY,
		m_cFrame.iX,
		m_cFrame.iY,
		m_cFrame.iWidth,
		m_cFrame.iHeight,
		m_cFrameTextRel.iX,
		m_cFrameTextRel.iY,
		m_cFrameTextRel.iWidth,
		m_cFrameTextRel.iHeight,
		m_cFrameScrollRel.iX,
		m_cFrameScrollRel.iY,
		m_cFrameScrollRel.iWidth,
		m_cFrameScrollRel.iHeight
		);
#endif
}

//////////////////////////////////////////////////////////////////////
// Function Name:	RefreshTextLineArray	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CTextBox::refreshTextLineArray(void)
{      
	//TRACE("[CTextBox]->RefreshLineArray \r\n");
	int loop = true;
	int pos_prev = 0;
	int pos = 0;
	int	aktWidth = 0;
	int aktWordWidth = 0;
	int lineBreakWidth;
	int maxTextWidth = 0;

	m_nNrOfNewLine = 0;

	std::string	aktLine = "";
	std::string	aktWord = "";

	/* clear current line vector */
	m_cLineArray.clear();
	m_nNrOfLines = 0;

	if( m_nMode & AUTO_WIDTH)
	{
		/* In case of autowidth, we calculate the max allowed width of the textbox */
		lineBreakWidth =	MAX_WINDOW_WIDTH  
							- m_cFrameScrollRel.iWidth 
							- 2*TEXT_BORDER_WIDTH;
	}
	else
	{
		/* If not autowidth, we just take the actuall textframe width */
		lineBreakWidth = m_cFrameTextRel.iWidth - 2*TEXT_BORDER_WIDTH;
	}
	
	int TextChars = m_cText.size();
	// do not parse, if text is empty 
	if(TextChars > 0)
	{
		while(loop)
		{
			if(m_nMode & NO_AUTO_LINEBREAK)
			{
				pos = m_cText.find_first_of("\n",pos_prev);
			}
			else
			{
				pos = m_cText.find_first_of("\n-. ",pos_prev);
			}
	
			//TRACE_1("     pos: %d pos_prev: %d\r\n",pos,pos_prev);
	
			if(pos == -1)
			{
				pos = TextChars+1;
				loop = false; // note, this is not 100% correct. if the last characters does not fit in one line, the characters after are cut
				//TRACE_1(" Textend found\r\n");
			}
	
			aktWord = m_cText.substr(pos_prev, pos - pos_prev + 1);
			aktWordWidth = m_pcFontText->getRenderWidth(aktWord);
			pos_prev = pos + 1;
			//if(aktWord.find("&quot;") == )
			if(1)
			{
				//TRACE_1("     aktWord: >%s< pos:%d\r\n",aktWord.c_str(),pos);
		
				if( aktWidth + aktWordWidth > lineBreakWidth &&
					!(m_nMode & NO_AUTO_LINEBREAK))
				{
					/* we need a new line before we can continue */
					m_cLineArray.push_back(aktLine);
					//TRACE_1("  end line: %s\r\n", aktLine.c_str());
					m_nNrOfLines++;
					aktLine = "";
					aktWidth = 0;
					
					if(pos_prev >= TextChars) loop = false;
				}
		
				aktLine  += aktWord;
				aktWidth += aktWordWidth;
				if (aktWidth > maxTextWidth) maxTextWidth = aktWidth;
				//TRACE_1("     aktLine : %s\r\n",aktLine.c_str());
				//TRACE_1("     aktWidth: %d aktWordWidth:%d\r\n",aktWidth,aktWordWidth);
		
				if( m_cText[pos] == '\n' ||
					loop == false)
				{
					// current line ends with an carriage return, make new line
					if (m_cText[pos] == '\n')
						aktLine.erase(aktLine.size() - 1,1);
					m_cLineArray.push_back(aktLine);
					m_nNrOfLines++;
					aktLine = "";
					aktWidth = 0;
					m_nNrOfNewLine++;
					if(pos_prev >= TextChars) loop = false;
				}
			}
		}


		/* check if we have to recalculate the window frame size, due to auto width and auto height */
		if( m_nMode & AUTO_WIDTH)
		{
			reSizeMainFrameWidth(maxTextWidth);
		}
	
		if(m_nMode & AUTO_HIGH)
		{
			reSizeMainFrameHeight(m_nNrOfLines * m_nFontTextHeight);
		}
	
		m_nLinesPerPage = (m_cFrameTextRel.iHeight - (2*TEXT_BORDER_WIDTH)) / m_nFontTextHeight;
		m_nNrOfPages =	((m_nNrOfLines-1) / m_nLinesPerPage) + 1;
	
		if(m_nCurrentPage >= m_nNrOfPages)
		{
			m_nCurrentPage = m_nNrOfPages - 1;
			m_nCurrentLine = m_nCurrentPage * m_nLinesPerPage;
		}
	}
	else
	{
		m_nNrOfPages = 0;
		m_nNrOfLines = 0;
		m_nCurrentPage = 0;
		m_nCurrentLine = 0;
		m_nLinesPerPage = 1;
	}
#if 0	
	TRACE_1(" m_nNrOfPages:     %d\r\n",m_nNrOfPages);
	TRACE_1(" m_nNrOfLines:     %d\r\n",m_nNrOfLines);
	TRACE_1(" m_nNrOfNewLine:   %d\r\n",m_nNrOfNewLine);
	TRACE_1(" maxTextWidth:  %d\r\n",maxTextWidth);
	TRACE_1(" m_nLinesPerPage:  %d\r\n",m_nLinesPerPage);
	TRACE_1(" m_nFontTextHeight:%d\r\n",m_nFontTextHeight);
	TRACE_1(" m_nCurrentPage:   %d\r\n",m_nCurrentPage);
	TRACE_1(" m_nCurrentLine:   %d\r\n",m_nCurrentLine);
#endif
}

//////////////////////////////////////////////////////////////////////
// Function Name:	RefreshScroll	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CTextBox::refreshScroll(void)
{
	if(	!(m_nMode & SCROLL)) return;
	if( m_pcWindow == NULL) return;


	if (m_nNrOfPages > 1) 
	{
		m_pcWindow->paintBoxRel(	m_cFrameScrollRel.iX, 
								m_cFrameScrollRel.iY, 
								m_cFrameScrollRel.iWidth, 
								m_cFrameScrollRel.iHeight, 
								COL_MENUCONTENT_PLUS_1);
		unsigned int marker_size = m_cFrameScrollRel.iHeight / m_nNrOfPages;
		m_pcWindow->paintBoxRel(	m_cFrameScrollRel.iX + SCROLL_MARKER_BORDER, 
								m_cFrameScrollRel.iY + m_nCurrentPage * marker_size, 
								m_cFrameScrollRel.iWidth - 2*SCROLL_MARKER_BORDER, 
								marker_size, 
								COL_MENUCONTENT_PLUS_3,
								RADIUS_SMALL);
	}
	else
	{
		m_pcWindow->paintBoxRel(	m_cFrameScrollRel.iX, 
								m_cFrameScrollRel.iY, 
								m_cFrameScrollRel.iWidth, 
								m_cFrameScrollRel.iHeight, 
								m_textBackgroundColor);
	}
}

//////////////////////////////////////////////////////////////////////
// Function Name:	RefreshText	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CTextBox::refreshText(void)
{
	if( m_pcWindow == NULL) return;
	//TRACE("   CTextBox::refreshText: %d,%s\r\n",m_nCurrentLine,m_cLineArray[m_nCurrentLine].c_str());
	//Paint Text Background
	m_pcWindow->paintBoxRel(	m_cFrameTextRel.iX, 
							m_cFrameTextRel.iY, 
							m_cFrameTextRel.iWidth, 
							m_cFrameTextRel.iHeight,  
							m_textBackgroundColor);

	if( m_nNrOfLines <= 0) return;
	int y = m_cFrameTextRel.iY + TEXT_BORDER_WIDTH;
	int i;
	int x_center = 0;

	for(	i = m_nCurrentLine; 
			i < m_nNrOfLines && i < m_nCurrentLine + m_nLinesPerPage; 
			i++)
	{
			y += m_nFontTextHeight;
			
			if( m_nMode & CENTER )
			{
				x_center = (m_cFrameTextRel.iWidth - m_pcFontText->getRenderWidth(m_cLineArray[i]))>>1;
			}
			
			m_pcWindow->RenderString(	m_pcFontText,
											m_cFrameTextRel.iX + TEXT_BORDER_WIDTH + x_center, 
											y, 
											m_cFrameTextRel.iWidth, 
											m_cLineArray[i].c_str(), 
											COL_MENUCONTENT, 
											0, 
											true); // UTF-8
	}
}

//////////////////////////////////////////////////////////////////////
// global Functions
//////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////
// Function Name:	ScrollPageDown	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CTextBox::scrollPageDown(const int pages)
{
	if( !(m_nMode & SCROLL)) return;
	if( m_nNrOfLines <= 0) return;
	TRACE("[CTextBox]->ScrollPageDown \r\n");


	if(m_nCurrentPage + pages < m_nNrOfPages)
	{
		m_nCurrentPage += pages; 
	}
	else 
	{
		m_nCurrentPage = m_nNrOfPages - 1;
	}
	m_nCurrentLine = m_nCurrentPage * m_nLinesPerPage; 
	refresh();
}

//////////////////////////////////////////////////////////////////////
// Function Name:	ScrollPageUp	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CTextBox::scrollPageUp(const int pages)
{
	if( !(m_nMode & SCROLL)) return;
	if( m_nNrOfLines <= 0) return;
	TRACE("[CTextBox]->ScrollPageUp \r\n");


	if(m_nCurrentPage - pages > 0)
	{
		m_nCurrentPage -= pages; 
	}
	else 
	{
		m_nCurrentPage = 0;
	}
	m_nCurrentLine = m_nCurrentPage * m_nLinesPerPage; 
	refresh();
}

//////////////////////////////////////////////////////////////////////
// Function Name:	Refresh	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CTextBox::refresh(void)
{
	if( m_pcWindow == NULL) return;
	//TRACE("[CTextBox]->Refresh\r\n");

	//Paint text
	refreshScroll();
	refreshText();
}

//////////////////////////////////////////////////////////////////////
// Function Name:	SetText	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
bool CTextBox::setText(const std::string* newText)
{
	//TRACE("[CTextBox]->SetText \r\n");
	bool result = false;
	if (newText != NULL)
	{
		m_cText = *newText;
		UTF8XMLtoUTF8(&m_cText); // remove UTF8XML tags
		refreshTextLineArray();
		refresh();
		result = true;
	}
	
	return(result);
};

//////////////////////////////////////////////////////////////////////
// Function Name:	paint	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CTextBox::paint (void)
{
	if(m_pcWindow != NULL) return;
	//TRACE("[CTextBox]->paint \r\n");
	m_pcWindow = new CFBWindow( m_cFrame.iX,
								m_cFrame.iY,
								m_cFrame.iWidth,
								m_cFrame.iHeight);
	refresh();
}

//////////////////////////////////////////////////////////////////////
// Function Name:	hide	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CTextBox::hide (void)
{
	if(m_pcWindow == NULL) return;
	//TRACE("[CTextBox]->hide \r\n");
	delete m_pcWindow;
	m_pcWindow = NULL;
}

/************************************************************************

************************************************************************/
bool UTF8XMLtoUTF8(std::string* text)
{
	bool result = true;
#if 1 // use whatever is faster
	int pos=0;
	int end = text->size()-5;
	while(pos !=-1)
	{
		pos = text->find("&",pos);
		if(pos != -1)
		{
			if(pos < end)
			{
				if( (*text)[pos+1] == 'q' &&
					(*text)[pos+2] == 'u' &&
					(*text)[pos+3] == 'o' &&
					(*text)[pos+4] == 't' &&
					(*text)[pos+5] == ';' )
				{
					text->replace(pos,6,"\"");
					end = text->size()-5;
				}
				else if(	(*text)[pos+1] == 'a' &&
							(*text)[pos+2] == 'p' &&
							(*text)[pos+3] == 'o' &&
							(*text)[pos+4] == 's' &&
							(*text)[pos+5] == ';' )
				{
					text->replace(pos,6,"\'");
					end = text->size()-5;
				}
			}
			pos++;
		}
	}

#else
	int pos=0;
	int end = text->size()-5;
	for(pos = 0 ; pos < end; pos++)
	{
		if( movie_info.epgInfo2[pos] == '&' &&
			(*text)[pos+5] == ';' && // this line is added here to speed up the routine, since the ; is at the same pos for both &apos; and &quot; , other are not supported yet
			(*text)[pos+3] == 'o')   // this line is added here to speed up the routine, since the o is at the same pos for both &apos; and &quot; , other are not supported yet
		{
			if( (*text)[pos+1] == 'q' &&
				(*text)[pos+2] == 'u' &&
				/*(*text)[pos+3] == 'o' &&*/
				(*text)[pos+4] == 't' /*&&
				(*text)[pos+5] == ';' */)
			{
				(*text)[pos] = '\"';
				text->erase(pos+1,5);
				end = text->size()-5;
			}
			else if(	(*text)[pos+1] == 'a' &&
						(*text)[pos+2] == 'p' &&
						/*(*text)[pos+3] == 'o' &&*/
						(*text)[pos+4] == 's' /*&&
						(*text)[pos+5] == ';' */)
			{
				(*text)[pos] = '\'';
				text->erase(pos+1,5);
				end = text->size()-5;
			}
		}
	}
#endif
	return(result);
}
