/******************************************************************************
 *                        <<< TuxMail - Mail Plugin >>>
 *                (c) Thomas "LazyT" Loewe 2003 (LazyT@gmx.net)
 *-----------------------------------------------------------------------------
 * $Log: tuxmail.c,v $
 * Revision 1.21  2005/05/12 22:24:23  lazyt
 * - PIN-Fix
 * - add Messageboxes for send Mail done/fail
 *
 * Revision 1.20  2005/05/12 14:28:28  lazyt
 * - PIN-Protection for complete Account
 * - Preparation for sending Mails ;-)
 *
 * Revision 1.19  2005/05/11 19:00:21  robspr1
 * minor Mailreader changes / add to Spamlist undo
 *
 * Revision 1.18  2005/05/11 12:01:21  lazyt
 * Protect Mailreader with optional PIN-Code
 *
 * Revision 1.17  2005/05/10 12:55:15  lazyt
 * - LCD-Fix for DM500
 * - Autostart for DM7020 (use -DOE, put Init-Script to /etc/init.d/tuxmail)
 * - try again after 10s if first DNS-Lookup failed
 * - don't try to read Mails on empty Accounts
 *
 * Revision 1.16  2005/05/09 19:30:20  robspr1
 * support for mail reading
 *
 * Revision 1.15  2005/04/29 17:24:00  lazyt
 * use 8bit audiodata, fix skin and osd
 *
 * Revision 1.14  2005/03/28 14:14:14  lazyt
 * support for userdefined audio notify (put your 12/24/48KHz pcm wavefile to /var/tuxbox/config/tuxmail/tuxmail.wav)
 *
 * Revision 1.13  2005/03/24 13:15:20  lazyt
 * cosmetics, add SKIN=0/1 for different osd-colors
 *
 * Revision 1.12  2005/03/22 13:31:46  lazyt
 * support for english osd (OSD=G/E)
 *
 * Revision 1.11  2005/03/22 09:35:20  lazyt
 * lcd support for daemon (LCD=Y/N, GUI should support /tmp/lcd.locked)
 *
 * Revision 1.10  2005/02/26 10:23:48  lazyt
 * workaround for corrupt mail-db
 * add ADMIN=Y/N to conf (N to disable mail deletion via plugin)
 * show versioninfo via "?" button
 * limit display to last 100 mails (increase MAXMAIL if you need more)
 *
 * Revision 1.9  2004/07/10 11:38:14  lazyt
 * use -DOLDFT for older FreeType versions
 * replaced all remove() with unlink()
 *
 * Revision 1.8  2004/04/24 22:24:23  carjay
 * fix compiler warnings
 *
 * Revision 1.7  2004/04/14 17:46:00  metallica
 *  fix2: seg fault (better)
 *
 * Revision 1.6  2004/04/14 17:05:10  metallica
 *  fix: seg fault
 *
 * Revision 1.5  2003/06/24 07:19:35  alexw
 * bugfix & cleanup
 *
 * Revision 1.4  2003/06/23 15:18:49  alexw
 * hmpf, oldapi
 *
 * Revision 1.3  2003/06/23 15:16:45  alexw
 * rc fixed
 *
 * Revision 1.2  2003/05/16 15:07:23  lazyt
 * skip unused accounts via "plus/minus", add mailaddress to spamlist via "blue"
 *
 * Revision 1.1  2003/04/21 09:24:52  lazyt
 * add tuxmail, todo: sync (filelocking?) between daemon and plugin
 ******************************************************************************/

#include "tuxmail.h"

/******************************************************************************
 * ReadConf
 ******************************************************************************/

void ReadConf()
{
	FILE *fd_conf;
	char *ptr;
	char line_buffer[256];

	// open config

		if(!(fd_conf = fopen(CFGPATH CFGFILE, "r")))
		{
			printf("TuxMail <Config not found, using defaults>\n");

			return;
		}

	// read config

		while(fgets(line_buffer, sizeof(line_buffer), fd_conf))
		{
			if((ptr = strstr(line_buffer, "ADMIN=")))
			{
				sscanf(ptr + 6, "%c", &admin);
			}
			else if((ptr = strstr(line_buffer, "OSD=")))
			{
				sscanf(ptr + 4, "%c", &osd);
			}
			else if((ptr = strstr(line_buffer, "SKIN=")))
			{
				sscanf(ptr + 5, "%d", &skin);
			}
			else if((ptr = strstr(line_buffer, "CODE0=")))
			{
				sscanf(ptr + 6, "%4s", maildb[0].code);
			}
			else if((ptr = strstr(line_buffer, "CODE1=")))
			{
				sscanf(ptr + 6, "%4s", maildb[1].code);
			}
			else if((ptr = strstr(line_buffer, "CODE2=")))
			{
				sscanf(ptr + 6, "%4s", maildb[2].code);
			}
			else if((ptr = strstr(line_buffer, "CODE3=")))
			{
				sscanf(ptr + 6, "%4s", maildb[3].code);
			}
			else if((ptr = strstr(line_buffer, "CODE4=")))
			{
				sscanf(ptr + 6, "%4s", maildb[4].code);
			}
			else if((ptr = strstr(line_buffer, "CODE5=")))
			{
				sscanf(ptr + 6, "%4s", maildb[5].code);
			}
			else if((ptr = strstr(line_buffer, "CODE6=")))
			{
				sscanf(ptr + 6, "%4s", maildb[6].code);
			}
			else if((ptr = strstr(line_buffer, "CODE7=")))
			{
				sscanf(ptr + 6, "%4s", maildb[7].code);
			}
			else if((ptr = strstr(line_buffer, "CODE8=")))
			{
				sscanf(ptr + 6, "%4s", maildb[8].code);
			}
			else if((ptr = strstr(line_buffer, "CODE9=")))
			{
				sscanf(ptr + 6, "%4s", maildb[9].code);
			}
		}

		fclose(fd_conf);

	// check config

		if(admin != 'Y' && admin != 'N')
		{
			printf("TuxMail <ADMIN=%c invalid, set to \"Y\">\n", admin);

			admin = 'Y';
		}

		if(osd != 'G' && osd != 'E')
		{
			printf("TuxMail <OSD=%c invalid, set to \"G\">\n", osd);

			osd = 'G';
		}

		if(skin != 1 && skin != 2)
		{
			printf("TuxMail <SKIN=%d invalid, set to \"1\">\n", skin);

			skin = 1;
		}
}

/******************************************************************************
 * ControlDaemon (0=fail, 1=done)
 ******************************************************************************/

int ControlDaemon(int command, int account, int mailindex)
{
	int fd_sock;
	struct sockaddr_un srvaddr;
	socklen_t addrlen;
	char sendcmd[88];
	char mailsend;

	// setup connection

		srvaddr.sun_family = AF_UNIX;
		strcpy(srvaddr.sun_path, SCKFILE);
		addrlen = sizeof(srvaddr.sun_family) + strlen(srvaddr.sun_path);

		if((fd_sock = socket(PF_UNIX, SOCK_STREAM, 0)) == -1)
		{
			printf("TuxMail <Socketerror: socket failed>\n");

			return 0;
		}

		if(connect(fd_sock, (struct sockaddr*)&srvaddr, addrlen) == -1)
		{
			printf("TuxMail <Socketerror: connect failed>\n");

			close(fd_sock);

			return 0;
		}

	// send command

		switch(command)
		{
			case GET_STATUS:

				send(fd_sock, "G", 1, 0);
				recv(fd_sock, &online, 1, 0);

				break;

			case SET_STATUS:

				send(fd_sock, "S", 1, 0);
				send(fd_sock, &online, 1, 0);

				break;

			case RELOAD_SPAMLIST:

				send(fd_sock, "L", 1, 0);

				break;

			case GET_VERSION:

				send(fd_sock, "V", 1, 0);
				recv(fd_sock, &versioninfo_d, sizeof(versioninfo_d), 0);

				break;

			case GET_MAIL:

				ShowMessage(GETMAIL);

				send(fd_sock, "M", 1, 0);
				sprintf(sendcmd, "%d-%02d-%s", account, mailindex, maildb[account].mailinfo[mailindex].uid);
				send(fd_sock, sendcmd, 5 + sizeof(maildb[account].mailinfo[mailindex].uid), 0);
				recv(fd_sock, &mailfile, 1, 0);

				break;

			case SEND_MAIL:

				send(fd_sock, "W", 1, 0);
				recv(fd_sock, &mailsend, 1, 0);

				mailsend ? ShowMessage(SENDMAILDONE) : ShowMessage(SENDMAILFAIL);
		}

		close(fd_sock);

	return 1;
}

/******************************************************************************
 * GetRCCode
 ******************************************************************************/

#if HAVE_DVB_API_VERSION == 3

int GetRCCode()
{
	static __u16 rc_last_key = KEY_RESERVED;

	if(sim_key)
	{
		sim_key = 0;

		return rccode;
	}

	if(read(rc, &ev, sizeof(ev)) == sizeof(ev))
	{
		if(ev.value)
		{
			if(ev.code != rc_last_key)
			{
				rc_last_key = ev.code;

				switch(ev.code)
				{
					case KEY_UP:

						rccode = RC_UP;

						break;

					case KEY_DOWN:

						rccode = RC_DOWN;

						break;

					case KEY_LEFT:

						rccode = RC_LEFT;

						break;

					case KEY_RIGHT:

						rccode = RC_RIGHT;

						break;

					case KEY_OK:

						rccode = RC_OK;

						break;

					case KEY_0:

						rccode = RC_0;

						break;

					case KEY_1:

						rccode = RC_1;

						break;

					case KEY_2:

						rccode = RC_2;

						break;

					case KEY_3:

						rccode = RC_3;

						break;

					case KEY_4:

						rccode = RC_4;

						break;

					case KEY_5:

						rccode = RC_5;

						break;

					case KEY_6:

						rccode = RC_6;

						break;

					case KEY_7:

						rccode = RC_7;

						break;

					case KEY_8:

						rccode = RC_8;

						break;

					case KEY_9:

						rccode = RC_9;

						break;

					case KEY_RED:

						rccode = RC_RED;

						break;

					case KEY_GREEN:

						rccode = RC_GREEN;

						break;

					case KEY_YELLOW:

						rccode = RC_YELLOW;

						break;

					case KEY_BLUE:

						rccode = RC_BLUE;

						break;

					case KEY_VOLUMEUP:

						rccode = RC_PLUS;

						break;

					case KEY_VOLUMEDOWN:

						rccode = RC_MINUS;

						break;

					case KEY_MUTE:

						rccode = RC_MUTE;

						break;

					case KEY_HELP:

						rccode = RC_HELP;

						break;

					case KEY_SETUP:

						rccode = RC_DBOX;

						break;

					case KEY_HOME:

						rccode = RC_HOME;

						break;

					case KEY_POWER:

						rccode = RC_STANDBY;

						break;

					default:

						rccode = -1;
				}
			}
			else
			{
				rccode = -1;
			}
		}
		else
		{
			rccode = -1;
			rc_last_key = KEY_RESERVED;
		}
	}

	return rccode;
}

#else

int GetRCCode()
{
	static unsigned short LastKey = -1;

	if(sim_key)
	{
		sim_key = 0;

		return rccode;
	}

	read(rc, &rccode, sizeof(rccode));

	if(rccode != LastKey)
	{
		LastKey = rccode;

		// translation required?

			if((rccode & 0xFF00) == 0x5C00)
			{
				switch(rccode)
				{
					case RC1_UP:

						rccode = RC_UP;

						break;

					case RC1_DOWN:

						rccode = RC_DOWN;

						break;

					case RC1_LEFT:

						rccode = RC_LEFT;

						break;

					case RC1_RIGHT:

						rccode = RC_RIGHT;

						break;

					case RC1_OK:

						rccode = RC_OK;

						break;

					case RC1_0:

						rccode = RC_0;

						break;

					case RC1_1:

						rccode = RC_1;

						break;

					case RC1_2:

						rccode = RC_2;

						break;

					case RC1_3:

						rccode = RC_3;

						break;

					case RC1_4:

						rccode = RC_4;

						break;

					case RC1_5:

						rccode = RC_5;

						break;

					case RC1_6:

						rccode = RC_6;

						break;

					case RC1_7:

						rccode = RC_7;

						break;

					case RC1_8:

						rccode = RC_8;

						break;

					case RC1_9:

						rccode = RC_9;

						break;

					case RC1_RED:

						rccode = RC_RED;

						break;

					case RC1_GREEN:

						rccode = RC_GREEN;

						break;

					case RC1_YELLOW:

						rccode = RC_YELLOW;

						break;

					case RC1_BLUE:

						rccode = RC_BLUE;

						break;

					case RC1_PLUS:

						rccode = RC_PLUS;

						break;

					case RC1_MINUS:

						rccode = RC_MINUS;

						break;

					case RC1_MUTE:

						rccode = RC_MUTE;

						break;

					case RC1_HELP:

						rccode = RC_HELP;

						break;

					case RC1_DBOX:

						rccode = RC_DBOX;

						break;

					case RC1_HOME:

						rccode = RC_HOME;

						break;

					case RC1_STANDBY:

						rccode = RC_STANDBY;
				}
			}
			else
			{
				rccode &= 0x003F;
			}
	}
	else
	{
		rccode = -1;
	}

	return rccode;
}

#endif

/******************************************************************************
 * MyFaceRequester
 ******************************************************************************/

FT_Error MyFaceRequester(FTC_FaceID face_id, FT_Library library, FT_Pointer request_data, FT_Face *aface)
{
	FT_Error result;

	result = FT_New_Face(library, face_id, 0, aface);

	if(!result)
	{
		printf("TuxMail <Font \"%s\" loaded>\n", (char*)face_id);
	}
	else
	{
		printf("TuxMail <Font \"%s\" failed>\n", (char*)face_id);
	}

	return result;
}

/******************************************************************************
 * RenderLCDDigit
 ******************************************************************************/

void RenderLCDDigit(int digit, int sx, int sy)
{
	int x, y;

	for(y = 0; y < 15; y++)
	{
		for(x = 0; x < 10; x++)
		{
			if(lcd_digits[digit*15*10 + x + y*10])
			{
				lcd_buffer[sx + x + ((sy + y)/8)*120] |= 1 << ((sy + y)%8);
			}
			else
			{
				lcd_buffer[sx + x + ((sy + y)/8)*120] &= ~(1 << ((sy + y)%8));
			}
		}
	}
}

/******************************************************************************
 * UpdateLCD
 ******************************************************************************/

void UpdateLCD(int account)
{
	int x, y;

	// set online status

		for(y = 0; y < 19; y++)
		{
			for(x = 0; x < 17; x++)
			{
				if(lcd_status[online*17*19 + x + y*17])
				{
					lcd_buffer[4 + x + ((18 + y)/8)*120] |= 1 << ((18 + y)%8);
				}
				else
				{
					lcd_buffer[4 + x + ((18 + y)/8)*120] &= ~(1 << ((18 + y)%8));
				}
			}
		}

	// set digits

		RenderLCDDigit(maildb[account].nr[0] - '0', 41, 20);

		RenderLCDDigit(maildb[account].time[0] - '0', 58, 20);
		RenderLCDDigit(maildb[account].time[1] - '0', 71, 20);
		RenderLCDDigit(maildb[account].time[3] - '0', 93, 20);
		RenderLCDDigit(maildb[account].time[4] - '0', 106, 20);

		RenderLCDDigit(maildb[account].status[0] - '0', 28, 44);
		RenderLCDDigit(maildb[account].status[1] - '0', 41, 44);
		RenderLCDDigit(maildb[account].status[2] - '0', 54, 44);
		RenderLCDDigit(maildb[account].status[4] - '0', 80, 44);
		RenderLCDDigit(maildb[account].status[5] - '0', 93, 44);
		RenderLCDDigit(maildb[account].status[6] - '0', 106, 44);

	// copy to lcd

		write(lcd, &lcd_buffer, sizeof(lcd_buffer));
}

/******************************************************************************
 * RenderChar
 ******************************************************************************/

int RenderChar(FT_ULong currentchar, int sx, int sy, int ex, int color)
{
	int row, pitch, bit, x = 0, y = 0;
	FT_Error error;
	FT_UInt glyphindex;
	FT_Vector kerning;
	FTC_Node anode;

	//load char

		if(!(glyphindex = FT_Get_Char_Index(face, currentchar)))
		{
			printf("TuxMail <FT_Get_Char_Index for Char \"%c\" failed: \"undefined character code\">\n", (int)currentchar);

			return 0;
		}

		if((error = FTC_SBitCache_Lookup(cache, &desc, glyphindex, &sbit, &anode)))
		{
			printf("TuxMail <FTC_SBitCache_Lookup for Char \"%c\" failed with Errorcode 0x%.2X>\n", (int)currentchar, error);

			return 0;
		}

		if(use_kerning)
		{
			FT_Get_Kerning(face, prev_glyphindex, glyphindex, ft_kerning_default, &kerning);

			prev_glyphindex = glyphindex;
			kerning.x >>= 6;
		}
		else
		{
			kerning.x = 0;
		}

	// render char

		if(color != -1) /* don't render char, return charwidth only */
		{
			if(sx + sbit->xadvance >= ex)
			{
				return -1; /* limit to maxwidth */
			}

			for(row = 0; row < sbit->height; row++)
			{
				for(pitch = 0; pitch < sbit->pitch; pitch++)
				{
					for(bit = 7; bit >= 0; bit--)
					{
						if(pitch*8 + 7-bit >= sbit->width)
						{
							break; /* render needed bits only */
						}

						if((sbit->buffer[row * sbit->pitch + pitch]) & 1<<bit)
						{
							*(lbb + startx + sx + sbit->left + kerning.x + x + var_screeninfo.xres*(starty + sy - sbit->top + y)) = color;
						}

						x++;
					}
				}

				x = 0;
				y++;
			}
		}

	// return charwidth

		return sbit->xadvance + kerning.x;
}

/******************************************************************************
 * GetStringLen
 ******************************************************************************/

int GetStringLen(unsigned char *string)
{
	int stringlen = 0;

	// reset kerning

		prev_glyphindex = 0;

	// calc len

		while(*string != '\0')
		{
			stringlen += RenderChar(*string, -1, -1, -1, -1);
			string++;
		}

	return stringlen;
}

/******************************************************************************
 * RenderString
 ******************************************************************************/

void RenderString(unsigned char *string, int sx, int sy, int maxwidth, int layout, int size, int color)
{
	int stringlen, ex, charwidth;

	// set size

		if(size == SMALL)
		{
			desc.font.pix_width = desc.font.pix_height = 24;
		}
		else
		{
			desc.font.pix_width = desc.font.pix_height = 40;
		}

	// set alignment

		if(layout != LEFT)
		{
			stringlen = GetStringLen(string);

			switch(layout)
			{
				case CENTER:
					if(stringlen < maxwidth)
					{
						sx += (maxwidth - stringlen)/2;
					}

					break;

				case RIGHT:

					if(stringlen < maxwidth)
					{
						sx += maxwidth - stringlen;
					}
			}
		}

	// reset kerning

		prev_glyphindex = 0;

	// render string

		ex = sx + maxwidth;

		while(*string != '\0')
		{
			if((charwidth = RenderChar(*string, sx, sy, ex, color)) == -1)
			{
				return; /* string > maxwidth */
			}

			sx += charwidth;
			string++;
		}
}

/******************************************************************************
 * RenderBox
 ******************************************************************************/

void RenderBox(int sx, int sy, int ex, int ey, int mode, int color)
{
	int loop;

	if(mode == FILL)
	{
		for(; sy <= ey; sy++)
		{
			memset(lbb + startx + sx + var_screeninfo.xres*(starty + sy), color, ex-sx + 1);
		}
	}
	else
	{
		// hor lines

			for(loop = sx; loop <= ex; loop++)
			{
				*(lbb + startx+loop + var_screeninfo.xres*(sy+starty)) = color;
				*(lbb + startx+loop + var_screeninfo.xres*(sy+1+starty)) = color;

				*(lbb + startx+loop + var_screeninfo.xres*(ey-1+starty)) = color;
				*(lbb + startx+loop + var_screeninfo.xres*(ey+starty)) = color;
			}

		// ver lines

			for(loop = sy; loop <= ey; loop++)
			{
				*(lbb + startx+sx + var_screeninfo.xres*(loop+starty)) = color;
				*(lbb + startx+sx+1 + var_screeninfo.xres*(loop+starty)) = color;

				*(lbb + startx+ex-1 + var_screeninfo.xres*(loop+starty)) = color;
				*(lbb + startx+ex + var_screeninfo.xres*(loop+starty)) = color;
			}
	}
}

/******************************************************************************
 * RenderCircle
 ******************************************************************************/

void RenderCircle(int sy, char type)
{
	int sx = 56, x, y, color;

	// set color

		switch(type)
		{
			case 'N':

				color = GREEN;

				break;

			case 'O':

				color = YELLOW;

				break;

			case 'D':

				color = RED;

				break;

			default:

				return;
		}

	// render

		for(y = 0; y < 15; y++)
		{
			for(x = 0; x < 15; x++)
			{
				if(circle[x + y*15])
				{
					memset(lbb + startx + sx + x + var_screeninfo.xres*(starty + sy + y), color, 1);
				}
			}
		}
}


/******************************************************************************
 * PaintMailHeader                                                                   *
 ******************************************************************************/

void PaintMailHeader( void )
{
	RenderBox(0, 0, VIEWX, 3*FONTHEIGHT_SMALL+2*BORDERSIZE+2, FILL, SKIN0);
	RenderBox(0, 3*FONTHEIGHT_SMALL+2*BORDERSIZE, VIEWX,VIEWY, FILL, SKIN1);
	RenderBox(0, 0, VIEWX, 3*FONTHEIGHT_SMALL+2*BORDERSIZE+2, GRID, SKIN2);
	RenderBox(0, 3*FONTHEIGHT_SMALL+2*BORDERSIZE, VIEWX, VIEWY, GRID, SKIN2);
}

/******************************************************************************
 * ShowMailHeader                                                             *
 ******************************************************************************/

void ShowMailHeader(char* szAction)
{
	char *p;
	
	
	p=strchr(szAction, '\n');
	if(p)
	{
		*p = 0;
		RenderString((osd == 'G') ? "Absender:" : "From:", 2*BORDERSIZE, BORDERSIZE+FONTHEIGHT_SMALL-2  , VIEWX-4*BORDERSIZE, LEFT, SMALL, ORANGE);
		RenderString(szAction, 2*BORDERSIZE+100, BORDERSIZE+FONTHEIGHT_SMALL-2  , VIEWX-4*BORDERSIZE-100, LEFT, SMALL, WHITE);
		szAction = ++p;
	}
	if(p) 
	{
		p = strchr(szAction, '\n');
	}
	if(p)
	{
		*p = 0;
		RenderString((osd == 'G') ? "Betreff:" : "Subject:", 2*BORDERSIZE, BORDERSIZE+2*FONTHEIGHT_SMALL-2  , VIEWX-4*BORDERSIZE, LEFT, SMALL, ORANGE);
		RenderString(szAction, 2*BORDERSIZE+100, BORDERSIZE+2*FONTHEIGHT_SMALL-2  , VIEWX-4*BORDERSIZE-100, LEFT, SMALL, WHITE);
		szAction = ++p;
	}
	if(p)
	{
		p = strchr(szAction,'\n');
	}
	if(p)
	{
		*p = 0;
		RenderString((osd == 'G') ? "Datum:" : "Date:", 2*BORDERSIZE, BORDERSIZE+3*FONTHEIGHT_SMALL-2  , VIEWX-4*BORDERSIZE, LEFT, SMALL, ORANGE);
		RenderString(szAction, 2*BORDERSIZE+100, BORDERSIZE+3*FONTHEIGHT_SMALL-2 , VIEWX-4*BORDERSIZE-100, LEFT, SMALL, WHITE);
		szAction = ++p;
	}
	if(p)
	{
		p = strchr(szAction,'\n');
	}
	if(p)
	{
		*p = 0;
		RenderString((osd == 'G') ? "Seite:" : "Page:", 2*BORDERSIZE+450, BORDERSIZE+3*FONTHEIGHT_SMALL-2  , VIEWX-4*BORDERSIZE, LEFT, SMALL, ORANGE);
		RenderString(szAction, 2*BORDERSIZE+500, BORDERSIZE+3*FONTHEIGHT_SMALL-2 , VIEWX-4*BORDERSIZE-500, LEFT, SMALL, WHITE);
		szAction = ++p;
	}
	
}

/******************************************************************************
 * ShowMailFile                                                                   *
 ******************************************************************************/
void ShowMailFile(char* filename, char* szAction)
{
	// Code from splugin (with some modifications...)
	char *p;
	char line[256];
	int iPage;
	int iMaxPages = 0;
	long iPagePos[100];
	char szHeader[256];

	FILE* pipe;
	pipe = fopen(filename,"r");

	if  (pipe == NULL)
	{
		return;
	}


	// Render output window
	PaintMailHeader();
	int row = 0;
	
	// calculate number of pages
	while( fgets( line, 83, pipe ))
	{
		if ( ++row > (FRAMEROWS) )
		{
			row = 0;
			iMaxPages ++;
		}
	}
	iMaxPages ++;
	
	row = 0;
	iPage = 0;
	// position of 1. byte of all the pages
	fseek(pipe, 0, SEEK_SET);
	iPagePos[0] = ftell(pipe);
	while(1)
	{
		while( fgets( line, 83, pipe ) )
		{
			p = strchr(line,'\n');
			if( p )
			{
				*p = 0;
			}
			row++;
			RenderString(line, 2*BORDERSIZE+2, 2*BORDERSIZE+3*FONTHEIGHT_SMALL+2+row*FONTHEIGHT_SMALL -FONT_OFFSET, VIEWX-4*BORDERSIZE, LEFT, SMALL, WHITE);

			if(row > FRAMEROWS)
			{
				// Render output window
				sprintf(szHeader,"%s%u / %u\n", szAction, iPage+1, iMaxPages);
				ShowMailHeader(szHeader);
				memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);

				while(1)
				{
					GetRCCode();
					if( rccode == RC_HOME || rccode == RC_OK || rccode == RC_RIGHT || rccode == RC_DOWN )
					{
						if (iPage < 99) 
						{
							iPage++;
						}
						break;
					}
					if( rccode == RC_LEFT || rccode == RC_UP )
					{
						if (iPage) 
						{
							iPage--;
							fseek(pipe, iPagePos[iPage], SEEK_SET);
						}
						else
						{
							fseek(pipe, 0, SEEK_SET);
						}
						break;
					}
				}
				row = 0;
				iPagePos[iPage] = ftell(pipe);
				if(rccode == RC_HOME) 
				{
					break;
				}
				// Render output window
				PaintMailHeader();

			}
		}
		if(row > 0)
		{
			// Render output window
			iMaxPages = iPage+1;
			sprintf(szHeader,"%s%u / %u\n",szAction,iPage+1,iMaxPages);
			ShowMailHeader(szHeader);
			memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
			while(1)
			{
				GetRCCode();
				if( rccode == RC_HOME || rccode == RC_OK || rccode == RC_DOWN )
				{
					break;
				}
				if( rccode == RC_LEFT || rccode == RC_UP )
				{
					if( iPage ) 
					{
						iPage--;
						fseek(pipe, iPagePos[iPage], SEEK_SET);
					}
					else
					{
						fseek(pipe, 0, SEEK_SET);
					}
					row = 0;
					iPagePos[iPage] = ftell(pipe);
					if (rccode == RC_HOME)
					{
						break;
					}
					break;
				}
			}
		}
		PaintMailHeader();
		if(rccode == RC_HOME)
		{
			break;	
		}
	}
	rccode = 0;
	fclose(pipe);
}


/******************************************************************************
 * ShowMessage
 ******************************************************************************/

void ShowMessage(int message)
{
    char info[32];

	// layout

		RenderBox(155, 178, 464, 220, FILL, SKIN0);
		RenderBox(155, 220, 464, 327, FILL, SKIN1);
		RenderBox(155, 178, 464, 327, GRID, SKIN2);
		RenderBox(155, 220, 464, 327, GRID, SKIN2);

	// message

		if(message != INFO)
		{
			RenderString("TuxMail Statusinfo", 157, 213, 306, CENTER, BIG, ORANGE);
		}

		switch(message)
		{
			case NODAEMON:

				RenderString((osd == 'G') ? "Daemon ist nicht geladen!" : "Daemon not running!", 157, 265, 306, CENTER, BIG, WHITE);

				break;

			case STARTDONE:

				RenderString((osd == 'G') ? "Abfrage wurde gestartet." : "Polling started.", 157, 265, 306, CENTER, BIG, WHITE);

				break;

			case STARTFAIL:

				RenderString((osd == 'G') ? "Start ist fehlgeschlagen!" : "Start failed!", 157, 265, 306, CENTER, BIG, WHITE);

				break;

			case STOPDONE:

				RenderString((osd == 'G') ? "Abfrage wurde gestoppt." : "Polling stopped.", 157, 265, 306, CENTER, BIG, WHITE);

				break;

			case STOPFAIL:

				RenderString((osd == 'G') ? "Stop ist fehlgeschlagen!" : "Stop failed!", 157, 265, 306, CENTER, BIG, WHITE);

				break;

			case BOOTON:

				RenderString((osd == 'G') ? "Autostart aktiviert." : "Autostart enabled.", 157, 265, 306, CENTER, BIG, WHITE);

				break;

			case BOOTOFF:

				RenderString((osd == 'G') ? "Autostart deaktiviert." : "Autostart disabled.", 157, 265, 306, CENTER, BIG, WHITE);

				break;

			case ADD2SPAM:

				RenderString((osd == 'G') ? "Spamliste wurde erweitert." : "Added to Spamlist.", 157, 265, 306, CENTER, BIG, WHITE);

				break;

			case DELSPAM:

				RenderString((osd == 'G') ? "von Spamliste entfernt." : "Removed from Spamlist.", 157, 265, 306, CENTER, BIG, WHITE);

				break;

			case SPAMFAIL:

				RenderString((osd == 'G') ? "Update fehlgeschlagen!" : "Update failed!", 157, 265, 306, CENTER, BIG, WHITE);

				break;

			case GETMAIL:

				RenderString((osd == 'G') ? "Mail wird gelesen." : "Reading Mail.", 157, 265, 306, CENTER, BIG, WHITE);
				RenderString((osd == 'G') ? "Moment bitte..." : "Please wait...", 157, 305, 306, CENTER, BIG, WHITE);

				break;

			case GETMAILFAIL:

				RenderString((osd == 'G') ? "Mail lesen fehlgeschlagen!" : "Reading Mail failed!", 157, 265, 306, CENTER, BIG, WHITE);

				break;

			case SENDMAILDONE:

				RenderString((osd == 'G') ? "Mail wurde gesendet." : "Mailing successful.", 157, 265, 306, CENTER, BIG, WHITE);

				break;

			case SENDMAILFAIL:

				RenderString((osd == 'G') ? "Mail nicht gesendet!" : "Mailing failed!", 157, 265, 306, CENTER, BIG, WHITE);

				break;

			case INFO:

				ControlDaemon(GET_VERSION,0,0);

				sprintf(info, "TuxMail (P%s/D%s)", versioninfo_p, versioninfo_d);

				RenderString(info, 157, 213, 306, CENTER, BIG, ORANGE);
				RenderString("(C) 2003-2005 Thomas \"LazyT\" Loewe", 157, 265, 306, CENTER, SMALL, WHITE);
		}

		if(message != GETMAIL)
		{
		    RenderBox(285, 286, 334, 310, FILL, SKIN2);
		    RenderString("OK", 287, 305, 46, CENTER, SMALL, WHITE);
		}

		memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);

		if(message != GETMAIL)
		{
			while(GetRCCode() != RC_OK);
		}
}

/******************************************************************************
 * ShowMailInfo
 ******************************************************************************/

void ShowMailInfo(int account, int mailindex)
{
	int scrollbar_len, scrollbar_ofs, scrollbar_cor, loop;
	int sy = 61;
	int selectbar_mailindex = mailindex;

	// lcd

		if(lcd != -1)
		{
		    UpdateLCD(account);
		}
		else
		{
		    printf("TuxMail <no LCD found>\n");
		}

	// layout

		RenderBox(0, 0, 619, 504, FILL, SKIN0);
		RenderBox(0, 42, 593, 504, FILL, SKIN1);
		RenderBox(0, 0, 619, 504, GRID, SKIN2);
		RenderBox(0, 42, 593, 504, GRID, SKIN2);
		RenderBox(592, 42, 619, 69, GRID, SKIN2);
		RenderBox(592, 477, 619, 504, GRID, SKIN2);

	// status

		RenderString(maildb[account].nr, 12, 34, 20, LEFT, BIG, ORANGE);
		RenderString(maildb[account].time, 32, 34, 75, RIGHT, BIG, ORANGE);
		RenderString(maildb[account].name, 122, 34, 371, CENTER, BIG, ORANGE);
		RenderString(maildb[account].status, 503, 34, 105, RIGHT, BIG, ORANGE);

	// scrollbar

		mailindex = (mailindex/10)*10;

		for(loop = 0; loop < 14; loop++)
		{
			memcpy(lbb + startx + 599 + var_screeninfo.xres*(starty +  49 + loop), scroll_up + loop*14, 14);
			memcpy(lbb + startx + 599 + var_screeninfo.xres*(starty + 484 + loop), scroll_dn + loop*14, 14);
		}

		scrollbar_len = 403 / ((maildb[account].mails - 1)/10 + 1);
		scrollbar_ofs = scrollbar_len*mailindex / 10;
		scrollbar_cor = 403 - ((403/scrollbar_len)*scrollbar_len);
		RenderBox(596, 72 + scrollbar_ofs, 615, 72 + scrollbar_ofs + scrollbar_len + scrollbar_cor - 1, FILL, SKIN2);

	// check pin

		if(!CheckPIN(account))
		{
			do
			{
				GetRCCode();
			}
			while(rccode != RC_HOME && rccode != RC_0 && rccode != RC_1 && rccode != RC_2 && rccode != RC_3 && rccode != RC_4 && rccode != RC_5 && rccode != RC_6 && rccode != RC_7 && rccode != RC_8 && rccode != RC_9 && rccode != RC_PLUS && rccode != RC_MINUS);

			sim_key = 1;

			return;
		}
		else
		{
			RenderBox(155, 178, 464, 327, FILL, SKIN1);
		}

	// selectbar

		if(maildb[account].mails)
		{
			RenderBox(2, 44 + (selectbar_mailindex%10)*46, 591, 44 + (selectbar_mailindex%10)*46 + 44, FILL, SKIN2);
		}

	// mails

		for(loop = 0; loop < 10; loop++)
		{
			RenderString((char*)&maildb[account].mailinfo[mailindex + loop].date, 2, sy, 50, RIGHT, SMALL, WHITE);
			RenderString((char*)&maildb[account].mailinfo[mailindex + loop].time, 2, sy + 22, 50, RIGHT, SMALL, WHITE);
			RenderString((char*)&maildb[account].mailinfo[mailindex + loop].from, 75, sy, 517, LEFT, SMALL, WHITE);
			RenderString((char*)&maildb[account].mailinfo[mailindex + loop].subj, 75, sy + 22, 517, LEFT, SMALL, WHITE);

			RenderCircle(sy - 4, maildb[account].mailinfo[mailindex + loop].type[0]);

			sy += 46;
		}

	// copy backbuffer to framebuffer

		memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
}

/******************************************************************************
 * FillDB
 ******************************************************************************/

void FillDB(int account)
{
	char msg_file[] = "/tmp/tuxmail.?";
	char linebuffer[1024];
	FILE *fd_msg;
	int line = 0;

	// open status file

		msg_file[sizeof(msg_file) - 2] = account | '0';

		if(!(fd_msg = fopen(msg_file, "r")))
		{
			printf("TuxMail <no Status for Account %d>\n", account);

			maildb[account].nr[0] = account | '0';
			memcpy(maildb[account].time, "00:00", 5);
			memcpy(maildb[account].name, (osd == 'G') ? "keine Info verfügbar" : "Info isn't available", 20);
			memcpy(maildb[account].status, "000/000", 7);
			maildb[account].mails = 0;
			maildb[account].inactive = 1;

			return;
		}

	// get account, timestamp, name and mailstatus

		fgets(linebuffer, sizeof(linebuffer), fd_msg);
		sscanf(linebuffer, "%s %s %s %s", maildb[account].nr, maildb[account].time, maildb[account].name, maildb[account].status);

	// get date, time, from and subject for every mail

		while(fgets(linebuffer, sizeof(linebuffer), fd_msg))
		{
			char *entrystart;

			maildb[account].mails++;

			if((entrystart = strtok(linebuffer, "|")))
			{
				strncpy(maildb[account].mailinfo[line].type, entrystart, sizeof(maildb[account].mailinfo[line].type));
			}
			else
			{
				strcpy(maildb[account].mailinfo[line].type, "?");
			}

			if((entrystart = strtok(NULL, "|")))
			{
				strncpy(maildb[account].mailinfo[line].uid, entrystart, sizeof(maildb[account].mailinfo[line].uid));
			}
			else
			{
				strcpy(maildb[account].mailinfo[line].uid, "?");
			}

			if((entrystart = strtok(NULL, "|")))
			{
				strncpy(maildb[account].mailinfo[line].date, entrystart, sizeof(maildb[account].mailinfo[line].date));
			}
			else
			{
				strcpy(maildb[account].mailinfo[line].date, "??.???");
			}

			if((entrystart = strtok(NULL, "|")))
			{
				strncpy(maildb[account].mailinfo[line].time, entrystart, sizeof(maildb[account].mailinfo[line].time));
			}
			else
			{
				strcpy(maildb[account].mailinfo[line].time, "??:??");
			}

			if((entrystart = strtok(NULL, "|")))
			{
				strncpy(maildb[account].mailinfo[line].from, entrystart, sizeof(maildb[account].mailinfo[line].from));
			}
			else
			{
				strcpy(maildb[account].mailinfo[line].from, (osd == 'G') ? "TuxMail: DB-Eintrag defekt!" : "TuxMail: DB-Entry broken!");
			}

			if((entrystart = strtok(NULL, "|")))
			{
				strncpy(maildb[account].mailinfo[line].subj, entrystart, sizeof(maildb[account].mailinfo[line].subj));
			}
			else
			{
				strcpy(maildb[account].mailinfo[line].subj, (osd == 'G') ? "TuxMail: DB-Eintrag defekt!" : "TuxMail: DB-Entry broken!");
			}

			maildb[account].mailinfo[line].save[0] = maildb[account].mailinfo[line].type[0];

			if(++line >= MAXMAIL)
			{
				break;
			}
		}

	fclose(fd_msg);
}

/******************************************************************************
 * UpdateDB
 ******************************************************************************/

void UpdateDB(int account)
{
	char msg_file[] = "/tmp/tuxmail.?";
	char linebuffer[1024];
	FILE *fd_msg;
	int loop, pos1, pos2;

	// set current file

		msg_file[sizeof(msg_file) - 2] = account | '0';

	// update

		if((fd_msg = fopen(msg_file, "r+")))
		{
			fgets(linebuffer, sizeof(linebuffer), fd_msg);

			for(loop = 0; loop < maildb[account].mails; loop++)
			{
				while(!feof(fd_msg))
				{
					pos1 = ftell(fd_msg);
					fgets(linebuffer, sizeof(linebuffer), fd_msg);
					pos2 = ftell(fd_msg);

					if(strstr(linebuffer, maildb[account].mailinfo[loop].uid))
					{
						fseek(fd_msg, pos1, SEEK_SET);
						fprintf(fd_msg, "|%c|", maildb[account].mailinfo[loop].type[0]);
						fseek(fd_msg, pos2, SEEK_SET);

						break;
					}
				}
			}

			fclose(fd_msg);
		}
		else
		{
			printf("TuxMail <could not update Status for Account %d>\n", account);
		}
}

/******************************************************************************
 * Add2SpamList (0=fail, 1=added, 2=removed)
 ******************************************************************************/

int Add2SpamList(int account, int mailindex)
{
	FILE *fd_spam;
	char *ptr1, *ptr2;
	char mailaddress[256];
	char linebuffer[256];
	char filebuffer[4096];
	char bRemove = 0;
	
	// open or create spamlist

	// find address

		if((ptr1 = strchr(maildb[account].mailinfo[mailindex].from, '@')))
		{
			while(*(ptr1 - 1) != '\0' && *(ptr1 - 1) != '<')
			{
				ptr1--;
			}

			ptr2 = ptr1;

			while(*(ptr2) != '\0' && *(ptr2) != '>')
			{
				ptr2++;
			}

			strncpy(mailaddress, ptr1, ptr2 - ptr1);
			mailaddress[ptr2 - ptr1] = '\0';
		}
		else
		{
			printf("TuxMail <Mailaddress \"%s\" invalid, not added to Spamlist>\n", maildb[account].mailinfo[mailindex].from);

			return 0;
		}

		if((fd_spam = fopen(CFGPATH SPMFILE, "r")))
		{

	// now we check if this address is already in the spamlist, if so we remove it
			while ( fgets( linebuffer, sizeof(linebuffer), fd_spam ) )
			{
				if( !strstr(linebuffer, mailaddress) )
				{
					sprintf(filebuffer, "%s\n", linebuffer);
				}
				else
				{
					bRemove = 1;
				}
			}
			fclose(fd_spam);
		}
		
		if(!(fd_spam = fopen(CFGPATH SPMFILE, "w")))
		{
			printf("TuxMail <could not create Spamlist: %s>\n", strerror(errno));

			return 0;
		}

		if( !bRemove )
		{		
	// add address to spamlist
	
			printf("TuxMail <Mailaddress \"%s\" added to Spamlist>\n", mailaddress);

			fprintf(fd_spam, "%s\n", mailaddress);

			fclose(fd_spam);

			return 1;
		}

	// remove address from spamlist

			printf("TuxMail <Mailaddress \"%s\" removed from Spamlist>\n", mailaddress);

			fprintf(fd_spam, "%s", filebuffer);

			fclose(fd_spam);

			return 2;

}

/******************************************************************************
 * CheckPIN (0=wrong, 1=ok)
 ******************************************************************************/

int CheckPIN(int Account)
{
	int result = 0;
	int skip;
	int count = 0;
	char code[4];

	// pin active?

		if(!maildb[Account].code[0])
		{
			return 1;
		}

	// account locked?

		if(maildb[Account].pincount == 3)
		{
			RenderBox(155, 178, 464, 220, FILL, SKIN0);
			RenderBox(155, 220, 464, 327, FILL, SKIN1);
			RenderBox(155, 178, 464, 327, GRID, SKIN2);
			RenderBox(155, 220, 464, 327, GRID, SKIN2);

			RenderString((osd == 'G') ? "Sicherheitsabfrage" : "Security Check", 157, 213, 306, CENTER, BIG, ORANGE);
			RenderString((osd == 'G') ? "Konto gesperrt!" : "Account locked!", 157, 265, 306, CENTER, BIG, WHITE);
			RenderString((osd == 'G') ? "Bitte anderes Konto wählen..." : "Try another Account...", 157, 305, 306, CENTER, SMALL, WHITE);

			memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);

			return 0;
		}

	// layout

		RenderBox(155, 178, 464, 220, FILL, SKIN0);
		RenderBox(155, 220, 464, 327, FILL, SKIN1);
		RenderBox(155, 178, 464, 327, GRID, SKIN2);
		RenderBox(155, 220, 464, 327, GRID, SKIN2);

		RenderString((osd == 'G') ? "Sicherheitsabfrage" : "Security Check", 157, 213, 306, CENTER, BIG, ORANGE);
		RenderString("PIN-Code :", 219, 265, 120, RIGHT, BIG, WHITE);
		RenderString("????", 346, 265, 100, LEFT, BIG, WHITE);

		RenderBox(285, 286, 334, 310, FILL, SKIN2);
		RenderString("EXIT", 287, 305, 46, CENTER, SMALL, WHITE);

		memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);

    	// get pin

		do
		{
			skip = 0;

			switch((rccode = GetRCCode()))
			{
				case RC_0:
					code[count] = '0';
					break;

				case RC_1:
					code[count] = '1';
					break;

				case RC_2:
					code[count] = '2';
					break;

				case RC_3:
					code[count] = '3';
					break;

				case RC_4:
					code[count] = '4';
					break;

				case RC_5:
					code[count] = '5';
					break;

				case RC_6:
					code[count] = '6';
					break;

				case RC_7:
					code[count] = '7';
					break;

				case RC_8:
					code[count] = '8';
					break;

				case RC_9:
					code[count] = '9';
					break;

				default:
					skip = 1;
			}

			if(!skip)
			{
				RenderBox(341, 236, 393, 270, FILL, SKIN1);

				switch(++count)
				{
					case 1:
						RenderString("*???", 346, 265, 100, LEFT, BIG, WHITE);
						break;

					case 2:
						RenderString("**??", 346, 265, 100, LEFT, BIG, WHITE);
						break;

					case 3:
						RenderString("***?", 346, 265, 100, LEFT, BIG, WHITE);
						break;

					case 4:
						RenderString("****", 346, 265, 100, LEFT, BIG, WHITE);
				}

				memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
			}
		}
		while(rccode != RC_HOME && count < 4);

	// check pin

		if(count == 4)
		{
			if(strncmp(code, maildb[Account].code, 4))
			{
				maildb[Account].pincount++;

				RenderBox(157, 222, 462, 325, FILL, SKIN1);
				RenderString((osd == 'G') ? "Falsche PIN!" : "Wrong PIN!", 157, 265, 306, CENTER, BIG, WHITE);
				RenderString((osd == 'G') ? "Nächster Versuch in 5 Sekunden..." : "Try again in 5 Seconds...", 157, 305, 306, CENTER, SMALL, WHITE);
				memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);

				sleep(5);
			}
			else
			{
				maildb[Account].code[0] = 0;
				result = 1;
			}
		}

		if(!result)
		{
			RenderBox(157, 222, 462, 325, FILL, SKIN1);
			RenderString((osd == 'G') ? "Zugriff verweigert!" : "Access denied!", 157, 265, 306, CENTER, BIG, WHITE);
			RenderString((osd == 'G') ? "Bitte anderes Konto wählen..." : "Try another Account...", 157, 305, 306, CENTER, SMALL, WHITE);

			memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
		}

		return result;
}

/******************************************************************************
 * plugin_exec
 ******************************************************************************/

void plugin_exec(PluginParam *par)
{
	char cvs_revision[] = "$Revision: 1.21 $";
	int loop, account, mailindex;
	FILE *fd_run;
	FT_Error error;

	// show versioninfo

		sscanf(cvs_revision, "%*s %s", versioninfo_p);
		printf("TuxMail %s\n", versioninfo_p);

	// get params

		fb = rc = lcd = sx = ex = sy = ey = -1;

		for(; par; par = par->next)
		{
			if(!strcmp(par->id, P_ID_FBUFFER))
			{
				fb = atoi(par->val);
			}
			else if(!strcmp(par->id, P_ID_RCINPUT))
			{
				rc = atoi(par->val);
			}
			else if(!strcmp(par->id, P_ID_LCD))
			{
				lcd = atoi(par->val);
			}
			else if(!strcmp(par->id, P_ID_OFF_X))
			{
				sx = atoi(par->val);
			}
			else if(!strcmp(par->id, P_ID_END_X))
			{
				ex = atoi(par->val);
			}
			else if(!strcmp(par->id, P_ID_OFF_Y))
			{
				sy = atoi(par->val);
			}
			else if(!strcmp(par->id, P_ID_END_Y))
			{
				ey = atoi(par->val);
			}
		}

		if(fb == -1 || rc == -1 || sx == -1 || ex == -1 || sy == -1 || ey == -1)
		{
			printf("TuxMail <missing Param(s)>\n");

			return;
		}

	// fill database

		memset(maildb, 0, sizeof(maildb));

		for(loop = 0; loop < 10; loop++)
		{
			FillDB(loop);
		}

	// read config

		ReadConf();

	// init framebuffer

		if(ioctl(fb, FBIOGET_FSCREENINFO, &fix_screeninfo) == -1)
		{
			printf("TuxMail <FBIOGET_FSCREENINFO failed>\n");

			return;
		}

		if(ioctl(fb, FBIOGET_VSCREENINFO, &var_screeninfo) == -1)
		{
			printf("TuxMail <FBIOGET_VSCREENINFO failed>\n");

			return;
		}

		if(ioctl(fb, FBIOPUTCMAP, (skin == 1) ? &colormap1 : &colormap2) == -1)
		{
			printf("TuxMail <FBIOPUTCMAP failed>\n");

			return;
		}

		if(!(lfb = (unsigned char*)mmap(0, fix_screeninfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0)))
		{
			printf("TuxMail <mapping of Framebuffer failed>\n");

			return;
		}

	// init fontlibrary

		if((error = FT_Init_FreeType(&library)))
		{
			printf("TuxMail <FT_Init_FreeType failed with Errorcode 0x%.2X>", error);

			munmap(lfb, fix_screeninfo.smem_len);

			return;
		}

		if((error = FTC_Manager_New(library, 1, 2, 0, &MyFaceRequester, NULL, &manager)))
		{
			printf("TuxMail <FTC_Manager_New failed with Errorcode 0x%.2X>\n", error);

			FT_Done_FreeType(library);

			munmap(lfb, fix_screeninfo.smem_len);

			return;
		}

		if((error = FTC_SBitCache_New(manager, &cache)))
		{
			printf("TuxMail <FTC_SBitCache_New failed with Errorcode 0x%.2X>\n", error);

			FTC_Manager_Done(manager);

			FT_Done_FreeType(library);

			munmap(lfb, fix_screeninfo.smem_len);

			return;
		}

		if((error = FTC_Manager_Lookup_Face(manager, FONT, &face)))
		{
			printf("TuxMail <FTC_Manager_Lookup_Face failed with Errorcode 0x%.2X>\n", error);

			FTC_Manager_Done(manager);

			FT_Done_FreeType(library);

			munmap(lfb, fix_screeninfo.smem_len);

			return;
		}

		use_kerning = FT_HAS_KERNING(face);

		desc.font.face_id = FONT;
#ifdef OLDFT
		desc.type = ftc_image_mono;
#else
		desc.flags = FT_LOAD_MONOCHROME;
#endif
	// init backbuffer

		if(!(lbb = malloc(var_screeninfo.xres*var_screeninfo.yres)))
		{
			printf("TuxMail <allocating of Backbuffer failed>\n");

			FTC_Manager_Done(manager);

			FT_Done_FreeType(library);

			munmap(lfb, fix_screeninfo.smem_len);

			return;
		}

		memset(lbb, 0, var_screeninfo.xres*var_screeninfo.yres);

		startx = sx + (((ex-sx) - 620)/2);
		starty = sy + (((ey-sy) - 505)/2);

	// get daemon status

		if(!ControlDaemon(GET_STATUS,0,0))
		{
			online = 2;
		}

	// remove last key & set rc to blocking mode

#if HAVE_DVB_API_VERSION == 3

		read(rc, &ev, sizeof(ev));
#else
		read(rc, &rccode, sizeof(rccode));
#endif
		fcntl(rc, F_SETFL, fcntl(rc, F_GETFL) &~ O_NONBLOCK);

	// show first account with new mail or account 0

		account = mailindex = 0;

		for(loop = 0; loop < 10; loop++)
		{
			if(maildb[loop].status[2] > '0' || maildb[loop].status[1] > '0' || maildb[loop].status[0] > '0')
			{
				account = loop;

				break;
			}
		}

		ShowMailInfo(account, mailindex);

	// main loop

		do
		{
			switch((rccode = GetRCCode()))
			{
				case RC_0:

					account = 0;
					mailindex = 0;

					break;

				case RC_1:

					account = 1;
					mailindex = 0;

					break;

				case RC_2:

					account = 2;
					mailindex = 0;

					break;

				case RC_3:

					account = 3;
					mailindex = 0;

					break;

				case RC_4:

					account = 4;
					mailindex = 0;

					break;

				case RC_5:

					account = 5;
					mailindex = 0;

					break;

				case RC_6:

					account = 6;
					mailindex = 0;

					break;

				case RC_7:

					account = 7;
					mailindex = 0;

					break;

				case RC_8:

					account = 8;
					mailindex = 0;

					break;

				case RC_9:

					account = 9;
					mailindex = 0;

					break;

				case RC_MINUS:

					mailindex = 0;

					for(loop = 0; loop < 10; loop++)
					{
						if(account > 0)
						{
							account--;
						}
						else
						{
							account = 9;
						}

						if(!maildb[account].inactive)
						{
							break;
						}
					}

					break;

				case RC_PLUS:

					mailindex = 0;

					for(loop = 0; loop < 10; loop++)
					{
						if(account < 9)
						{
							account++;
						}
						else
						{
							account = 0;
						}

						if(!maildb[account].inactive)
						{
							break;
						}
					}

					break;


				case RC_UP:

					if(mailindex > 0)
					{
						mailindex--;
					}

					break;

				case RC_DOWN:

					if(mailindex < maildb[account].mails-1)
					{
						mailindex++;
					}

					break;

				case RC_LEFT:

					if(!(mailindex%10) && mailindex)
					{
						mailindex--;
					}
					else
					{
						mailindex = (mailindex/10)*10;
					}

					break;

				case RC_RIGHT:

					if(mailindex%10 == 9)
					{
						mailindex++;
					}
					else
					{
						mailindex = (mailindex/10)*10 + 9;
					}

					if(mailindex >= maildb[account].mails)
					{
						if(maildb[account].mails)
						{
							mailindex = maildb[account].mails - 1;
						}
						else
						{
							mailindex = 0;
						}
					}

					break;

				case RC_OK:

					if(admin == 'Y')
					{
						if(maildb[account].mails)
						{
							if(maildb[account].mailinfo[mailindex].type[0] != 'D')
							{
						    		maildb[account].mailinfo[mailindex].type[0] = 'D';
						    		}
							else
							{
								maildb[account].mailinfo[mailindex].type[0] = maildb[account].mailinfo[mailindex].save[0];
								
								if(maildb[account].mailinfo[mailindex].type[0] == 'D')
								{
									maildb[account].mailinfo[mailindex].type[0] = 'O';
								}
							}
						}
					}

					break;

				case RC_RED:

					if(admin == 'Y')
					{
						if(maildb[account].mark_green && maildb[account].mark_yellow)
						{
							maildb[account].mark_red = 1;
						}
						else if(!maildb[account].mark_green && !maildb[account].mark_yellow)
						{
							maildb[account].mark_red = 0;
						}

						maildb[account].mark_red++;
						maildb[account].mark_red &= 1;

						if(maildb[account].mark_red)
						{
							for(loop = 0;loop < maildb[account].mails; loop++)
							{
								maildb[account].mailinfo[loop].type[0] = 'D';
							}

							maildb[account].mark_green = 1;
							maildb[account].mark_yellow = 1;
						}
						else
						{
							for(loop = 0;loop < maildb[account].mails; loop++)
							{
								maildb[account].mailinfo[loop].type[0] = maildb[account].mailinfo[loop].save[0];
							}

							maildb[account].mark_green = 0;
							maildb[account].mark_yellow = 0;
						}
					}

					break;

				case RC_GREEN:

					maildb[account].mark_green++;
					maildb[account].mark_green &= 1;

					if(maildb[account].mark_green)
					{
						for(loop = 0;loop < maildb[account].mails; loop++)
						{
							if(maildb[account].mailinfo[loop].type[0] == 'N')
							{
								maildb[account].mailinfo[loop].type[0] = 'D';
							}
						}
					}
					else
					{
						for(loop = 0;loop < maildb[account].mails; loop++)
						{
							if(maildb[account].mailinfo[loop].save[0] == 'N')
							{
								maildb[account].mailinfo[loop].type[0] = 'N';
							}
						}
					}

					break;

				case RC_YELLOW:

					maildb[account].mark_yellow++;
					maildb[account].mark_yellow &= 1;

					if(maildb[account].mark_yellow)
					{
						for(loop = 0;loop < maildb[account].mails; loop++)
						{
							if(maildb[account].mailinfo[loop].type[0] == 'O')
							{
								maildb[account].mailinfo[loop].type[0] = 'D';
							}
						}
					}
					else
					{
						for(loop = 0;loop < maildb[account].mails; loop++)
						{
							if(maildb[account].mailinfo[loop].save[0] == 'O')
							{
								maildb[account].mailinfo[loop].type[0] = 'O';
							}
						}
					}

					break;

				case RC_BLUE:

					if(admin == 'Y')
					{
						int iRet = Add2SpamList(account, mailindex);
						if(iRet == 1)
						{
							maildb[account].mailinfo[mailindex].type[0] = 'D';
							ControlDaemon(RELOAD_SPAMLIST,0,0);
							ShowMessage(ADD2SPAM);
						}
						else if(iRet == 2)
						{
							maildb[account].mailinfo[mailindex].type[0] = 'O';
							ControlDaemon(RELOAD_SPAMLIST,0,0);
							ShowMessage(DELSPAM);
						}
						else
						{
							ShowMessage(SPAMFAIL);
						}
					}

					break;

				case RC_MUTE:

					if(!ControlDaemon(GET_STATUS,0,0))
					{
						online = 2;
						ShowMessage(NODAEMON);
					}
					else
					{
						online++;
						online &= 1;

						if(!ControlDaemon(SET_STATUS,0,0))
						{
							if(online)
							{
								ShowMessage(STARTFAIL);
							}
							else
							{
								ShowMessage(STOPFAIL);
							}

							online++;
							online &= 1;
						}
						else
						{
							if(online)
							{
								ShowMessage(STARTDONE);
							}
							else
							{
								ShowMessage(STOPDONE);
							}
						}
					}

					break;

				case RC_STANDBY:

					if((fd_run = fopen(RUNFILE, "r")))
					{
						fclose(fd_run);
						unlink(RUNFILE);
#ifdef OE
						unlink(OE_START);
						unlink(OE_KILL0);
						unlink(OE_KILL6);
#endif
						ShowMessage(BOOTOFF);
					}
					else
					{
						fclose(fopen(RUNFILE, "w"));
#ifdef OE
					        symlink("../init.d/tuxmail", OE_START);
						symlink("../init.d/tuxmail", OE_KILL0);
						symlink("../init.d/tuxmail", OE_KILL6);
#endif
						ShowMessage(BOOTON);
					}

					break;

				case RC_HELP:

					if(maildb[account].mails)
					{
						ControlDaemon(GET_MAIL, account, mailindex);

						if(!mailfile)
						{
							ShowMessage(GETMAILFAIL);
						}
						else
						{
							char szInfo[256];
							sprintf(szInfo,"%s\n%s\n%s %s\n",maildb[account].mailinfo[mailindex].from,maildb[account].mailinfo[mailindex].subj,maildb[account].mailinfo[mailindex].date,maildb[account].mailinfo[mailindex].time);
							ShowMailFile(POP3FILE, szInfo);					
						}
					}

					break;

				case RC_DBOX:

					ShowMessage(INFO);

					break;

				default:

					continue;
			}

			ShowMailInfo(account, mailindex);
		}
		while(rccode != RC_HOME);

	// reset lcd lock

	    unlink(LCKFILE);

	// update database

		for(loop = 0; loop < 10; loop++)
		{
			UpdateDB(loop);
		}

	// cleanup

		FTC_Manager_Done(manager);
		FT_Done_FreeType(library);

		free(lbb);
		munmap(lfb, fix_screeninfo.smem_len);

		fcntl(rc, F_SETFL, O_NONBLOCK);

		return;
}
