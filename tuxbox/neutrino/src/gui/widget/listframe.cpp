
/***************************************************************************
	Neutrino-GUI  -   DBoxII-Project

 	Homepage: http://dbox.cyberphoria.org/

	$Id: listframe.cpp,v 1.5 2009/11/22 15:36:58 rhabarber1848 Exp $

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

	Module Name: listframe.cpp .

	Description: Implementation of the CListFrame class
				 This class provides a plain Listbox with numerous rows and lines. 

	Date:	 Nov 2005

	Author: Günther@tuxbox.berlios.org
		based on code of Steffen Hehn 'McClean'

****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "stdlib.h"
#include "listframe.h"
#include <gui/widget/icons.h>

#define	TEXT_BORDER_WIDTH			 8
#define ROW_BORDER_WIDTH             4
#define	SCROLL_FRAME_WIDTH			10
#define	SCROLL_MARKER_BORDER		 2

#define MAX_WINDOW_WIDTH  (g_settings.screen_EndX - g_settings.screen_StartX - 40)
#define MAX_WINDOW_HEIGHT (g_settings.screen_EndY - g_settings.screen_StartY - 40)	

#define MIN_WINDOW_WIDTH  ((g_settings.screen_EndX - g_settings.screen_StartX)>>1)
#define MIN_WINDOW_HEIGHT 40	

#define TITLE_BACKGROUND_COLOR ((CFBWindow::color_t)COL_MENUHEAD_PLUS_0)
#define HEADER_LIST_BACKGROUND_COLOR ((CFBWindow::color_t)COL_MENUCONTENT_PLUS_0)
#define LIST_BACKGROUND_COLOR ((CFBWindow::color_t)COL_MENUCONTENT_PLUS_0)
#define LIST_BACKGROUND_COLOR_SELECTED ((CFBWindow::color_t)COL_MENUCONTENTSELECTED_PLUS_0)

#define TITLE_FONT_COLOR ((CFBWindow::color_t)COL_MENUHEAD)
#define HEADER_LIST_FONT_COLOR COL_MENUCONTENT
#define LIST_FONT_COLOR COL_MENUCONTENT
#define LIST_FONT_COLOR_SELECTED COL_MENUCONTENTSELECTED

#define FONT_LIST g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]
#define FONT_HEADER_LIST g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]
#define FONT_TITLE g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE];

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////
// Function Name:	CListFrame	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
CListFrame::CListFrame(LF_LINES* lines,
		       Font* font_text,
		       const int _mode,
		       const CBox* position,
		       const char* textTitle,
		       Font* font_title)
{
	//TRACE("[CListFrame] new\r\n");
	initVar();

	m_pcWindow = NULL;

 	if(lines != NULL)
 	{
		m_pLines = lines;
		m_nNrOfRows = lines->rows;
		if(m_nNrOfRows > LF_MAX_ROWS)
			m_nNrOfRows = LF_MAX_ROWS;
	}
	if(font_text != NULL)	m_pcFontList = font_text;
	if(font_text != NULL)	m_pcFontHeaderList = font_text;
	if(position != NULL)
	{
		m_cFrame	= *position;
		m_nMaxHeight = m_cFrame.iHeight;
		m_nMaxWidth = m_cFrame.iWidth;
	}

	m_nMode = _mode;
	
#if 0
	TRACE("  Mode: ");
	if(mode & SCROLL) TRACE("SCROLL ");
	if(mode & AUTO_WIDTH) TRACE("AUTO_WIDTH ");
	if(mode & AUTO_HIGH) TRACE("AUTO_HIGH");
	TRACE("\r\n");
#endif


	if(font_title != NULL) 
		m_pcFontTitle = font_title;
	
	if( textTitle != NULL) 
		m_textTitle = textTitle;
		
	m_nFontListHeight  = m_pcFontList->getHeight();
	m_nFontHeaderListHeight = m_pcFontHeaderList->getHeight();
	m_nFontTitleHeight = m_pcFontTitle->getHeight();
	
	//TRACE(" CListFrame::m_nFontTextHeight: %d\t\r\n",m_nFontListHeight);

	/* Initialise the window frames first */
	initFramesRel();

	// than refresh text line array 
	onNewLineArray();
}


//////////////////////////////////////////////////////////////////////
// Function Name:	CListFrame	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		  
// Notes:		
//////////////////////////////////////////////////////////////////////
CListFrame::CListFrame(	LF_LINES* lines)
{
	//TRACE("[CListFrame] new\r\n");
	initVar();

	m_pcWindow = NULL;
 	if(lines != NULL)
 	{
		m_pLines = lines;
		m_nNrOfRows = lines->rows;
		if(m_nNrOfRows > LF_MAX_ROWS)
			m_nNrOfRows = LF_MAX_ROWS;
	}

	/* Initialise the window frames first */
	initFramesRel();

	// than refresh text line array 
	onNewLineArray();
}

//////////////////////////////////////////////////////////////////////
// Function Name:	CListFrame	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
CListFrame::CListFrame()
{
	//TRACE("[CListFrame] new\r\n");
	initVar();
	initFramesRel();
	
	m_pLines = NULL;
	m_pcWindow = NULL;
}

//////////////////////////////////////////////////////////////////////
// Function Name:	~CListFrame	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
CListFrame::~CListFrame()
{
	//TRACE("[CListFrame] del\r\n");
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
void CListFrame::initVar(void)
{
	//TRACE("[CListFrame]->InitVar\r\n");
	m_nMode = SCROLL;
	
	m_showSelection = true;

	m_pcFontList  =  FONT_LIST ;
	m_nFontListHeight = m_pcFontList->getHeight();
	
	m_pcFontHeaderList  =  FONT_HEADER_LIST ;
	m_nFontHeaderListHeight = m_pcFontHeaderList->getHeight();
	
	m_pcFontTitle = FONT_TITLE;
	m_textTitle = "MovieBrowser";
	m_nFontTitleHeight = m_pcFontTitle->getHeight();
	
	m_nNrOfPages = 1;
	m_nNrOfLines = 0;
	m_nNrOfRows = 1;
	m_nLinesPerPage = 0;
	m_nCurrentLine = 0;
	m_nCurrentPage = 0;
	m_nSelectedLine = 0;

	m_cFrame.iX		= g_settings.screen_StartX + ((g_settings.screen_EndX - g_settings.screen_StartX - MIN_WINDOW_WIDTH) >>1);
	m_cFrame.iWidth	= MIN_WINDOW_WIDTH;
	m_cFrame.iY		= g_settings.screen_StartY + ((g_settings.screen_EndY - g_settings.screen_StartY - MIN_WINDOW_HEIGHT) >>1);
	m_cFrame.iHeight	= MIN_WINDOW_HEIGHT;

	m_nMaxHeight = MAX_WINDOW_HEIGHT;
	m_nMaxWidth = MAX_WINDOW_WIDTH;
}

//////////////////////////////////////////////////////////////////////
// Function Name:	ReSizeMainFrameWidth	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CListFrame::reSizeMainFrameWidth(int textWidth)
{
	//TRACE("[CListFrame]->ReSizeMainFrameWidth: %d, current: %d\r\n",textWidth,m_cFrameListRel.iWidth);

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
void CListFrame::reSizeMainFrameHeight(int textHeight)
{
	//TRACE("[CListFrame]->ReSizeMainFrameHeight: %d, current: %d\r\n",textHeight,m_cFrameListRel.iHeight);

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
void CListFrame::initFramesRel(void)
{
	//TRACE("-[CListFrame]->InitFramesRel\r\n");

	if(m_nMode & TITLE)
	{
		m_cFrameTitleRel.iX			= 0;
		m_cFrameTitleRel.iY			= 0;
		m_cFrameTitleRel.iHeight	= m_nFontTitleHeight +2 ;
		m_cFrameTitleRel.iWidth		= m_cFrame.iWidth ;
	}
	else
	{
		m_cFrameTitleRel.iX		= 0;
		m_cFrameTitleRel.iY		= 0;
		m_cFrameTitleRel.iHeight= 0;
		m_cFrameTitleRel.iWidth	= 0;
	}

	if(m_nMode & HEADER_LINE)
	{
		m_cFrameHeaderListRel.iX		= 0;
		m_cFrameHeaderListRel.iY		= 0 + m_cFrameTitleRel.iHeight;
		m_cFrameHeaderListRel.iHeight	= m_nFontHeaderListHeight ;
	}
	else
	{
		m_cFrameHeaderListRel.iX		= 0;
		m_cFrameHeaderListRel.iY		= 0;
		m_cFrameHeaderListRel.iHeight	= 0;
		m_cFrameHeaderListRel.iWidth	= 0;
	}

	m_cFrameListRel.iX		= 0;
	m_cFrameListRel.iY		= m_cFrameHeaderListRel.iHeight + m_cFrameTitleRel.iHeight;
	m_cFrameListRel.iHeight	= m_cFrame.iHeight - m_cFrameHeaderListRel.iHeight - m_cFrameTitleRel.iHeight;

	if(m_nMode & SCROLL)
	{
		m_cFrameScrollRel.iX		= m_cFrame.iWidth - SCROLL_FRAME_WIDTH;
		m_cFrameScrollRel.iY		= m_cFrameTitleRel.iHeight;
		m_cFrameScrollRel.iWidth	= SCROLL_FRAME_WIDTH;
		m_cFrameScrollRel.iHeight	= m_cFrameListRel.iHeight + m_cFrameHeaderListRel.iHeight;
	}
	else
	{
		m_cFrameScrollRel.iX		= 0;
		m_cFrameScrollRel.iY		= 0;
		m_cFrameScrollRel.iHeight	= 0;
		m_cFrameScrollRel.iWidth	= 0;
	}

	m_cFrameListRel.iWidth	= m_cFrame.iWidth - m_cFrameScrollRel.iWidth;
	
	if(m_nMode & HEADER_LINE)
	{
		m_cFrameHeaderListRel.iWidth	= m_cFrame.iWidth - m_cFrameScrollRel.iWidth;
	}

	m_nLinesPerPage = (m_cFrameListRel.iHeight - (2*TEXT_BORDER_WIDTH)) / m_nFontListHeight;
#if 0
	TRACE_1("Frames\r\n\tScren:\t%3d,%3d,%3d,%3d\r\n\tMain:\t%3d,%3d,%3d,%3d\r\n\tTitle:\t%3d,%3d,%3d,%3d \r\n\tHeader:\t%3d,%3d,%3d,%3d \r\n\tList:\t%3d,%3d,%3d,%3d \r\n\tScroll:\t%3d,%3d,%3d,%3d \r\n",
		g_settings.screen_StartX,
		g_settings.screen_StartY,
		g_settings.screen_EndX,
		g_settings.screen_EndY,
		m_cFrame.iX,
		m_cFrame.iY,
		m_cFrame.iWidth,
		m_cFrame.iHeight,
		m_cFrameTitleRel.iX,
		m_cFrameTitleRel.iY,
		m_cFrameTitleRel.iWidth,
		m_cFrameTitleRel.iHeight,
		m_cFrameHeaderListRel.iX,
		m_cFrameHeaderListRel.iY,
		m_cFrameHeaderListRel.iWidth,
		m_cFrameHeaderListRel.iHeight,
		m_cFrameListRel.iX,
		m_cFrameListRel.iY,
		m_cFrameListRel.iWidth,
		m_cFrameListRel.iHeight,
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
void CListFrame::onNewLineArray(void)
{      
	//TRACE("[CListFrame]->onNewLineArray \r\n");
	int maxTextWidth = 0;

	maxTextWidth = 300; // TODO
	m_nNrOfLines = m_pLines->lineArray[0].size();
	
	if(m_nNrOfLines > 0 )
	{
		/* check if we have to recalculate the window frame size, due to auto width and auto height */
		if( m_nMode & AUTO_WIDTH)
		{
			reSizeMainFrameWidth(maxTextWidth);
		}
	
		if(m_nMode & AUTO_HIGH)
		{
			reSizeMainFrameHeight(m_nNrOfLines * m_nFontListHeight);
		}
	
		m_nLinesPerPage = (m_cFrameListRel.iHeight - (2*TEXT_BORDER_WIDTH)) / m_nFontListHeight;
		m_nNrOfPages =	((m_nNrOfLines-1) / m_nLinesPerPage) + 1;
	
		if(m_nCurrentLine >= m_nNrOfLines)
		{
			m_nCurrentPage = m_nNrOfPages - 1;
			m_nCurrentLine = m_nCurrentPage * m_nLinesPerPage;
		}
		if(m_nSelectedLine >= m_nNrOfLines)
		{
			m_nSelectedLine = m_nCurrentLine;
		}
	}
	else
	{
		m_nNrOfLines = 0;
		m_nCurrentLine = 0;
		m_nSelectedLine = 0;
		m_nLinesPerPage = 1;
		m_nNrOfPages = 0;
	}

//	TRACE_1(" m_nNrOfPages:     %d\r\n",m_nNrOfPages);
//	TRACE_1(" m_nNrOfLines:     %d\r\n",m_nNrOfLines);
//	TRACE_1(" maxTextWidth:  %d\r\n",maxTextWidth);
//	TRACE_1(" m_nLinesPerPage:  %d\r\n",m_nLinesPerPage);
//	TRACE_1(" m_nFontTextHeight:%d\r\n",m_nFontListHeight);
	//TRACE_1(" m_nCurrentPage:   %d\r\n",m_nCurrentPage);
	//TRACE_1(" m_nCurrentLine:   %d\r\n",m_nCurrentLine);
}

//////////////////////////////////////////////////////////////////////
// Function Name:	RefreshTitle	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CListFrame::refreshTitle(void)
{
	//TRACE("[CListFrame]->refreshHeaderList \r\n");
	if( m_pcWindow == NULL) return;
	//Paint Text Background
	m_pcWindow->paintBoxRel(	m_cFrameTitleRel.iX, 
								m_cFrameTitleRel.iY, 
								m_cFrameTitleRel.iWidth, 
								m_cFrameTitleRel.iHeight, 
								TITLE_BACKGROUND_COLOR);
									
	m_pcWindow->RenderString(	m_pcFontTitle,
								m_cFrameTitleRel.iX + TEXT_BORDER_WIDTH, 
								m_cFrameTitleRel.iY + m_cFrameTitleRel.iHeight, 
								m_cFrameTitleRel.iWidth - TEXT_BORDER_WIDTH<<1, 
								m_textTitle.c_str(), 
								TITLE_FONT_COLOR, 
								0, 
								true); // UTF-8
}


//////////////////////////////////////////////////////////////////////
// Function Name:	RefreshScroll	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CListFrame::refreshScroll(void)
{
	//TRACE("[CListFrame]->refreshScroll\r\n");
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
								COL_MENUCONTENT_PLUS_3);
	}
	else
	{
		m_pcWindow->paintBoxRel(	m_cFrameScrollRel.iX, 
								m_cFrameScrollRel.iY, 
								m_cFrameScrollRel.iWidth, 
								m_cFrameScrollRel.iHeight, 
								(CFBWindow::color_t)COL_MENUCONTENT_PLUS_0);
	}
}

//////////////////////////////////////////////////////////////////////
// Function Name:	refreshList	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CListFrame::refreshList(void)
{
	//TRACE("[CListFrame]->refreshList: %d\r\n",m_nCurrentLine);
	
	if( m_pcWindow == NULL) return;
	//Paint List Background
	m_pcWindow->paintBoxRel(m_cFrameListRel.iX, 
							m_cFrameListRel.iY, 
							m_cFrameListRel.iWidth, 
							m_cFrameListRel.iHeight,  
							LIST_BACKGROUND_COLOR);

	if(  m_nNrOfLines <= 0) return;
	
	int y = m_cFrameListRel.iY + TEXT_BORDER_WIDTH ;
	for(	int line = m_nCurrentLine; 
			line < m_nNrOfLines && line < m_nCurrentLine + m_nLinesPerPage; 
			line++)
	{
		int color = LIST_FONT_COLOR;
		// draw line
		if(line == m_nSelectedLine && m_showSelection == true)
		{
			color = LIST_FONT_COLOR_SELECTED;
			m_pcWindow->paintBoxRel(m_cFrameListRel.iX, 
									y, 
									m_cFrameListRel.iWidth, 
									m_nFontListHeight,  
									LIST_BACKGROUND_COLOR_SELECTED);
		}
		int width;
		int x = m_cFrameListRel.iX + TEXT_BORDER_WIDTH;
		y += m_nFontListHeight;
		for(int row = 0; row < m_pLines->rows; row++)
		{
			width = m_pLines->rowWidth[row] ;
			if(width > m_cFrameListRel.iWidth - x + m_cFrameListRel.iX - TEXT_BORDER_WIDTH)
			{
				width = m_cFrameListRel.iWidth - x + m_cFrameListRel.iX - TEXT_BORDER_WIDTH;
				//TRACE("   normalize width to %d , x:%d \r\n",width,x);
			}
			m_pcWindow->RenderString(	m_pcFontList,
										x, 
										y, 
										width, 
										m_pLines->lineArray[row][line].c_str(), 
										color, 
										0, 
										true); // UTF-8
			x += m_pLines->rowWidth[row] + ROW_BORDER_WIDTH;								
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Function Name:	refreshLine	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CListFrame::refreshLine(int line)
{
	if( m_pcWindow == NULL) return;
	if(  m_nNrOfLines <= 0) return;
	
	if((line < m_nCurrentLine) && (line > m_nCurrentLine + m_nLinesPerPage))return;
	
	uint8_t color;
	int rel_line = line - m_nCurrentLine;
	int y = m_cFrameListRel.iY + TEXT_BORDER_WIDTH + (rel_line*m_nFontListHeight);

	if(line == m_nSelectedLine && m_showSelection == true)
	{
		color = LIST_FONT_COLOR_SELECTED;
		m_pcWindow->paintBoxRel(	m_cFrameListRel.iX, 
										y, 
										m_cFrameListRel.iWidth, 
										m_nFontListHeight,  
										LIST_BACKGROUND_COLOR_SELECTED);
	}
	else
	{
		color = LIST_FONT_COLOR;
		m_pcWindow->paintBoxRel(	m_cFrameListRel.iX, 
										y, 
										m_cFrameListRel.iWidth, 
										m_nFontListHeight,  
										LIST_BACKGROUND_COLOR);
	}
	int width;
	int x = m_cFrameListRel.iX + TEXT_BORDER_WIDTH;
	y += m_nFontListHeight;
	for(int row = 0; row < m_pLines->rows; row++)
	{
		width = m_pLines->rowWidth[row] ;
		if(width > m_cFrameListRel.iWidth - x + m_cFrameListRel.iX - 2*TEXT_BORDER_WIDTH)
		{
			width = m_cFrameListRel.iWidth - x + m_cFrameListRel.iX - 2*TEXT_BORDER_WIDTH;
			//TRACE("   normalize to %d,x:%d\r\n",width,x);
		}
		m_pcWindow->RenderString(	m_pcFontList,
										x, 
										y, 
										width, 
										m_pLines->lineArray[row][line].c_str(), 
										color, 
										0, 
										true); // UTF-8
		x += m_pLines->rowWidth[row] + ROW_BORDER_WIDTH;								
	}
}

//////////////////////////////////////////////////////////////////////
// Function Name:	refreshHeaderList	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CListFrame::refreshHeaderList(void)
{
	//TRACE("[CListFrame]->refreshHeaderList \r\n");
	if(!(m_nMode & HEADER_LINE))return;
	if( m_pcWindow == NULL) return;
	//Paint Text Background
	m_pcWindow->paintBoxRel(m_cFrameHeaderListRel.iX, 
							m_cFrameHeaderListRel.iY, 
							m_cFrameHeaderListRel.iWidth, 
							m_cFrameHeaderListRel.iHeight, 
							HEADER_LIST_BACKGROUND_COLOR);
									
	int width;
	int x = m_cFrameHeaderListRel.iX + TEXT_BORDER_WIDTH;
	int y = m_cFrameHeaderListRel.iY + m_nFontHeaderListHeight + 2;
	bool loop = true;
	for(int row = 0; row < m_pLines->rows && loop == true; row++)
	{
		width = m_pLines->rowWidth[row] ;
		if(width > m_cFrameHeaderListRel.iWidth - x + m_cFrameHeaderListRel.iX - 2*TEXT_BORDER_WIDTH)
		{
			width = m_cFrameHeaderListRel.iWidth - x + m_cFrameHeaderListRel.iX - 2*TEXT_BORDER_WIDTH;
			//TRACE("   normalize width to %d , x:%d \r\n",width,x);
			loop = false;
		}
		m_pcWindow->RenderString(	m_pcFontHeaderList,
										x, 
										y, 
										width, 
										m_pLines->lineHeader[row].c_str(), 
										HEADER_LIST_FONT_COLOR, 
										0, 
										true); // UTF-8
		x += m_pLines->rowWidth[row] + ROW_BORDER_WIDTH;								
	}
}

//////////////////////////////////////////////////////////////////////
// global Functions
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Function Name:		
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CListFrame::scrollLineDown(const int lines)
{
	//TRACE("[CListFrame]->scrollLineDown \r\n");

	if( !(m_nMode & SCROLL)) return;
	if( m_nNrOfLines <= 1) return;
	
	if(m_nSelectedLine < m_nNrOfLines - 1 && m_nNrOfLines != 0) m_nSelectedLine++;
	
	// check if the cursor moves out of the window
	if(m_nSelectedLine - m_nCurrentLine > m_nLinesPerPage-1)
	{
		// yes, scroll to next page
		//TRACE("[CListFrame]  m_nSelectedLine: %d, \r\n",m_nSelectedLine);
		scrollPageDown(1);
	}
	else
	{
		refreshLine(m_nSelectedLine-lines);
		refreshLine(m_nSelectedLine);
	}
}
//////////////////////////////////////////////////////////////////////
// Function Name:		
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CListFrame::scrollLineUp(const int lines)
{
	//TRACE("[CListFrame]->scrollLineUp \r\n");
	if( !(m_nMode & SCROLL)) return;
	if( m_nNrOfLines <= 1) return;

	if(m_nSelectedLine > 0) m_nSelectedLine--;
	// check if the cursor moves out of the window
	if(m_nSelectedLine < m_nCurrentLine )
	{
		// yes, scroll to next page
		//TRACE("[CListFrame]  m_nSelectedLine: %d, \r\n",m_nSelectedLine);
		scrollPageUp(1);
	}
	else
	{
		refreshLine(m_nSelectedLine+lines);
		refreshLine(m_nSelectedLine);
	}
}

//////////////////////////////////////////////////////////////////////
// Function Name:	ScrollPageDown	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CListFrame::scrollPageDown(const int pages)
{
	//TRACE("[CListFrame]->ScrollPageDown \r\n");

	if( !(m_nMode & SCROLL)) return;
	if( m_nNrOfLines <= 1) return;
	
	if(m_nCurrentPage + pages < m_nNrOfPages)
	{
		m_nCurrentPage += pages; 
	}
	else 
	{
		m_nCurrentPage = m_nNrOfPages - 1;
	}
	m_nCurrentLine = m_nCurrentPage * m_nLinesPerPage; 
	if(m_nSelectedLine < m_nCurrentLine || m_nSelectedLine -m_nCurrentLine >= m_nLinesPerPage )
	{
		m_nSelectedLine = m_nCurrentLine;
	}
	//TRACE("[CListFrame]  m_nCurrentLine: %d, m_nCurrentPage: %d \r\n",m_nCurrentLine,m_nCurrentPage);
	refresh();
};

//////////////////////////////////////////////////////////////////////
// Function Name:	ScrollPageUp	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CListFrame::scrollPageUp(const int pages)
{
	//TRACE("[CListFrame]->ScrollPageUp \r\n");

	if( !(m_nMode & SCROLL)) return;
	if( m_nNrOfLines <= 1) return;

	if(m_nCurrentPage - pages > 0)
	{
		m_nCurrentPage -= pages; 
	}
	else 
	{
		m_nCurrentPage = 0;
	}
	m_nCurrentLine = m_nCurrentPage * m_nLinesPerPage; 
	if(m_nSelectedLine < m_nCurrentLine || m_nSelectedLine -m_nCurrentLine >= m_nLinesPerPage )
	{
		m_nSelectedLine = m_nCurrentLine;
	}
	//TRACE("[CListFrame]  m_nCurrentLine: %d, m_nCurrentPage: %d \r\n",m_nCurrentLine,m_nCurrentPage);
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
void CListFrame::refresh(void)
{
	if( m_pcWindow == NULL) return;
	//TRACE("[CListFrame]->Refresh\r\n");

	//Paint List
	refreshTitle();
	refreshScroll();
	refreshHeaderList();
	refreshList();
}

//////////////////////////////////////////////////////////////////////
// Function Name:	SetText	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
bool CListFrame::setLines(LF_LINES* lines)
{
	if(lines == NULL) return(false);
	//TRACE("[CListFrame]->setLines \r\n");

	m_pLines = lines;
	m_nNrOfRows = lines->rows;
	if(m_nNrOfRows > LF_MAX_ROWS)
		m_nNrOfRows = LF_MAX_ROWS;
	onNewLineArray();
	refresh();
	
	return(true);
}

//////////////////////////////////////////////////////////////////////
// Function Name:	setTitle	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
bool CListFrame::setTitle(char* title)
{
	//TRACE("[CListFrame]->setTitle \r\n");
	if(!(m_nMode & TITLE)) return(false);
	if(title == NULL) return(false);

	m_textTitle = title;
	refreshTitle();
	
	return(true);
}

//////////////////////////////////////////////////////////////////////
// Function Name:	setSelectedLine	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
bool CListFrame::setSelectedLine(int selection)
{
	//TRACE("[CListFrame]->setSelectedLine %d \r\n",selection);
	bool result = false;
	if(selection >= 0 && selection < m_nNrOfLines)
	{ 
		m_nSelectedLine = selection;
		m_nCurrentPage =  selection / m_nLinesPerPage;
		m_nCurrentLine = m_nCurrentPage * m_nLinesPerPage;
		refreshList();
		refreshScroll();  //NEW
		result = true;
		//TRACE(" selected line: %d,%d,%d \r\n",m_nSelectedLine,m_nCurrentPage,m_nCurrentLine);
	}
	
	return (result);
}

//////////////////////////////////////////////////////////////////////
// Function Name:	hide	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CListFrame::hide(void)
{
	if(m_pcWindow == NULL) return;
	//TRACE("[CListFrame]->hide \r\n");
	delete m_pcWindow;
	m_pcWindow = NULL;
}

//////////////////////////////////////////////////////////////////////
// Function Name:	paint	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CListFrame::paint(void)
{
	if(m_pcWindow != NULL) return;
	//TRACE("[CListFrame]->paint \r\n");
	m_pcWindow = new CFBWindow( m_cFrame.iX,
								m_cFrame.iY,
								m_cFrame.iWidth,
								m_cFrame.iHeight);
	refresh();
}
