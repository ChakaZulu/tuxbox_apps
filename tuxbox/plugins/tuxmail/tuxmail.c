/******************************************************************************
 *                        <<< TuxMail - Mail Plugin >>>
 *                (c) Thomas "LazyT" Loewe 2003 (LazyT@gmx.net)
 *-----------------------------------------------------------------------------
 * $Log: tuxmail.c,v $
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
 * ControlDaemon (o=fail, 1=done)
 ******************************************************************************/

int ControlDaemon(int command)
{
	int fd_sock;
	struct sockaddr_un srvaddr;
	socklen_t addrlen;

	//setup connection

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

	//send command

		switch(command)
		{
			case GET_STATUS:	send(fd_sock, "G", 1, 0);
						recv(fd_sock, &online, 1, 0);
						break;

			case SET_STATUS:	send(fd_sock, "S", 1, 0);
						send(fd_sock, &online, 1, 0);
						break;

			case RELOAD_SPAMLIST:	send(fd_sock, "L", 1, 0);
		}

		close(fd_sock);

	return 1;
}

#if HAVE_DVB_API_VERSION == 3

/******************************************************************************
 * GetRCCode
 ******************************************************************************/

int GetRCCode()
{
	static __u16 rc_last_key = KEY_RESERVED;
	//get code

	if(read(rc, &ev, sizeof(ev)) == sizeof(ev))
	{
		if(ev.value)
		{
			if(ev.code != rc_last_key)
			{
				rc_last_key = ev.code;
				switch(ev.code)
				{
					case KEY_UP:		rccode = RC_UP;
								break;

					case KEY_DOWN:		rccode = RC_DOWN;
								break;

					case KEY_LEFT:		rccode = RC_LEFT;
								break;

					case KEY_RIGHT:		rccode = RC_RIGHT;
								break;

					case KEY_OK:		rccode = RC_OK;
								break;

					case KEY_0:		rccode = RC_0;
								break;

					case KEY_1:		rccode = RC_1;
								break;

					case KEY_2:		rccode = RC_2;
								break;

					case KEY_3:		rccode = RC_3;
								break;

					case KEY_4:		rccode = RC_4;
								break;

					case KEY_5:		rccode = RC_5;
								break;

					case KEY_6:		rccode = RC_6;
								break;

					case KEY_7:		rccode = RC_7;
								break;

					case KEY_8:		rccode = RC_8;
								break;

					case KEY_9:		rccode = RC_9;
								break;

					case KEY_RED:		rccode = RC_RED;
								break;

					case KEY_GREEN:		rccode = RC_GREEN;
								break;

					case KEY_YELLOW:	rccode = RC_YELLOW;
								break;

					case KEY_BLUE:		rccode = RC_BLUE;
								break;

					case KEY_VOLUMEUP:	rccode = RC_PLUS;
								break;

					case KEY_VOLUMEDOWN:	rccode = RC_MINUS;
								break;

					case KEY_MUTE:		rccode = RC_MUTE;
								break;

					case KEY_HELP:		rccode = RC_HELP;
								break;

					case KEY_SETUP:		rccode = RC_DBOX;
								break;

					case KEY_HOME:		rccode = RC_HOME;
								break;

					case KEY_POWER:		rccode = RC_STANDBY;
								break;

					default:		rccode = -1;
				}
			}
			else
				rccode = -1;
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

/******************************************************************************
 * GetRCCode
 ******************************************************************************/

int GetRCCode()
{
	static unsigned short LastKey = -1;

	//get code

		read(rc, &rccode, sizeof(rccode));

		if(rccode != LastKey)
		{
			LastKey = rccode;

			//translation required?

				if((rccode & 0xFF00) == 0x5C00)
				{
					switch(rccode)
					{
						case RC1_UP:		rccode = RC_UP;
									break;

						case RC1_DOWN:		rccode = RC_DOWN;
									break;

						case RC1_LEFT:		rccode = RC_LEFT;
									break;

						case RC1_RIGHT:		rccode = RC_RIGHT;
									break;

						case RC1_OK:		rccode = RC_OK;
									break;

						case RC1_0:		rccode = RC_0;
									break;

						case RC1_1:		rccode = RC_1;
									break;

						case RC1_2:		rccode = RC_2;
									break;

						case RC1_3:		rccode = RC_3;
									break;

						case RC1_4:		rccode = RC_4;
									break;

						case RC1_5:		rccode = RC_5;
									break;

						case RC1_6:		rccode = RC_6;
									break;

						case RC1_7:		rccode = RC_7;
									break;

						case RC1_8:		rccode = RC_8;
									break;

						case RC1_9:		rccode = RC_9;
									break;

						case RC1_RED:		rccode = RC_RED;
									break;

						case RC1_GREEN:		rccode = RC_GREEN;
									break;

						case RC1_YELLOW:	rccode = RC_YELLOW;
									break;

						case RC1_BLUE:		rccode = RC_BLUE;
									break;

						case RC1_PLUS:		rccode = RC_PLUS;
									break;

						case RC1_MINUS:		rccode = RC_MINUS;
									break;

						case RC1_MUTE:		rccode = RC_MUTE;
									break;

						case RC1_HELP:		rccode = RC_HELP;
									break;

						case RC1_DBOX:		rccode = RC_DBOX;
									break;

						case RC1_HOME:		rccode = RC_HOME;
									break;

						case RC1_STANDBY:	rccode = RC_STANDBY;
					}
				}
				else rccode &= 0x003F;
		}
		else rccode = -1;

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

	if(!result) printf("TuxMail <Font \"%s\" loaded>\n", (char*)face_id);
	else        printf("TuxMail <Font \"%s\" failed>\n", (char*)face_id);

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
			if(lcd_digits[digit*15*10 + x + y*10]) lcd_buffer[sx + x + ((sy + y)/8)*120] |= 1 << ((sy + y)%8);
			else lcd_buffer[sx + x + ((sy + y)/8)*120] &= ~(1 << ((sy + y)%8));
		}
	}
}

/******************************************************************************
 * UpdateLCD
 ******************************************************************************/

void UpdateLCD(int account)
{
	int x, y;

	//set online status

		for(y = 0; y < 19; y++)
		{
			for(x = 0; x < 17; x++)
			{
				if(lcd_status[online*17*19 + x + y*17]) lcd_buffer[4 + x + ((18 + y)/8)*120] |= 1 << ((18 + y)%8);
				else lcd_buffer[4 + x + ((18 + y)/8)*120] &= ~(1 << ((18 + y)%8));
			}
		}

	//set digits

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

	//copy to lcd

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
		else kerning.x = 0;

	//render char

		if(color != -1) /* don't render char, return charwidth only */
		{
			if(sx + sbit->xadvance >= ex) return -1; /* limit to maxwidth */

			for(row = 0; row < sbit->height; row++)
			{
				for(pitch = 0; pitch < sbit->pitch; pitch++)
				{
					for(bit = 7; bit >= 0; bit--)
					{
						if(pitch*8 + 7-bit >= sbit->width) break; /* render needed bits only */

						if((sbit->buffer[row * sbit->pitch + pitch]) & 1<<bit) *(lbb + startx + sx + sbit->left + kerning.x + x + var_screeninfo.xres*(starty + sy - sbit->top + y)) = color;

						x++;
					}
				}

				x = 0;
				y++;
			}
		}

	//return charwidth

		return sbit->xadvance + kerning.x;
}

/******************************************************************************
 * GetStringLen
 ******************************************************************************/

int GetStringLen(unsigned char *string)
{
	int stringlen = 0;

	//reset kerning

		prev_glyphindex = 0;

	//calc len

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

	//set size

		if(size == SMALL) desc.font.pix_width = desc.font.pix_height = 24;
		else desc.font.pix_width = desc.font.pix_height = 40;

	//set alignment

		if(layout != LEFT)
		{
			stringlen = GetStringLen(string);

			switch(layout)
			{
				case CENTER:	if(stringlen < maxwidth) sx += (maxwidth - stringlen)/2;
						break;

				case RIGHT:	if(stringlen < maxwidth) sx += maxwidth - stringlen;
			}
		}

	//reset kerning

		prev_glyphindex = 0;

	//render string

		ex = sx + maxwidth;

		while(*string != '\0')
		{
			if((charwidth = RenderChar(*string, sx, sy, ex, color)) == -1) return; /* string > maxwidth */

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

	if(mode == FILL) for(; sy <= ey; sy++) memset(lbb + startx + sx + var_screeninfo.xres*(starty + sy), color, ex-sx + 1);
	else
	{
		//hor lines

			for(loop = sx; loop <= ex; loop++)
			{
				*(lbb + startx+loop + var_screeninfo.xres*(sy+starty)) = color;
				*(lbb + startx+loop + var_screeninfo.xres*(sy+1+starty)) = color;

				*(lbb + startx+loop + var_screeninfo.xres*(ey-1+starty)) = color;
				*(lbb + startx+loop + var_screeninfo.xres*(ey+starty)) = color;
			}

		//ver lines

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

	//set color

		switch(type)
		{
			case 'N':	color = GREEN;
					break;

			case 'O':	color = YELLOW;
					break;

			case 'D':	color = RED;
					break;

			default:	return;
		}

	//render

		for(y = 0; y < 15; y++)
		{
			for(x = 0; x < 15; x++) if(circle[x + y*15]) memset(lbb + startx + sx + x + var_screeninfo.xres*(starty + sy + y), color, 1);
		}
}

/******************************************************************************
 * ShowMessage
 ******************************************************************************/

void ShowMessage(int message)
{
	//layout

		RenderBox(155, 178, 464, 327, FILL, BLUE1);
		RenderBox(155, 178, 464, 327, GRID, BLUE2);
		RenderBox(155, 220, 464, 327, GRID, BLUE2);

	//message

		RenderString("TuxMailD Statusinfo", 157, 213, 306, CENTER, BIG, ORANGE);

		switch(message)
		{
			case NODAEMON:	RenderString("Daemon ist nicht geladen!", 157, 265, 306, CENTER, BIG, WHITE);
					break;

			case STARTDONE:	RenderString("Abfrage wurde gestartet.", 157, 265, 306, CENTER, BIG, WHITE);
					break;

			case STARTFAIL:	RenderString("Start ist fehlgeschlagen!", 157, 265, 306, CENTER, BIG, WHITE);
					break;

			case STOPDONE:	RenderString("Abfrage wurde gestoppt.", 157, 265, 306, CENTER, BIG, WHITE);
					break;

			case STOPFAIL:	RenderString("Stop ist fehlgeschlagen!", 157, 265, 306, CENTER, BIG, WHITE);
					break;

			case BOOTON:	RenderString("Autostart aktiviert.", 157, 265, 306, CENTER, BIG, WHITE);
					break;

			case BOOTOFF:	RenderString("Autostart deaktiviert.", 157, 265, 306, CENTER, BIG, WHITE);
					break;

			case ADD2SPAM:	RenderString("Spamliste wurde erweitert.", 157, 265, 306, CENTER, BIG, WHITE);
					break;

			case SPAMFAIL:	RenderString("Update fehlgeschlagen!", 157, 265, 306, CENTER, BIG, WHITE);
		}

		RenderBox(285, 286, 334, 310, FILL, BLUE2);
		RenderString("OK", 287, 305, 46, CENTER, SMALL, WHITE);
		memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);

		while(GetRCCode() != RC_OK);
}

/******************************************************************************
 * ShowMailInfo
 ******************************************************************************/

void ShowMailInfo(int account, int mailindex)
{
	int scrollbar_len, scrollbar_ofs, scrollbar_cor, loop;
	int sy = 61;

	//lcd

		UpdateLCD(account);

	//layout

		RenderBox(0, 0, 619, 504, FILL, BLUE1);
		RenderBox(0, 0, 619, 504, GRID, BLUE2);
		RenderBox(0, 42, 593, 504, GRID, BLUE2);
		RenderBox(592, 42, 619, 69, GRID, BLUE2);
		RenderBox(592, 477, 619, 504, GRID, BLUE2);

	//selectbar

		if(maildb[account].mails) RenderBox(2, 44 + (mailindex%10)*46, 591, 44 + (mailindex%10)*46 + 44, FILL, BLUE0);

	//scrollbar

		mailindex = (mailindex/10)*10;

		for(loop = 0; loop < 14; loop++)
		{
			memcpy(lbb + startx + 599 + var_screeninfo.xres*(starty +  49 + loop), scroll_up + loop*14, 14);
			memcpy(lbb + startx + 599 + var_screeninfo.xres*(starty + 484 + loop), scroll_dn + loop*14, 14);
		}

		scrollbar_len = 403 / ((maildb[account].mails - 1)/10 + 1);
		scrollbar_ofs = scrollbar_len*mailindex / 10;
		scrollbar_cor = 403 - ((403/scrollbar_len)*scrollbar_len);
		RenderBox(596, 72 + scrollbar_ofs, 615, 72 + scrollbar_ofs + scrollbar_len + scrollbar_cor - 1, FILL, BLUE2);

	//status and mails

		RenderString(maildb[account].nr, 12, 34, 20, LEFT, BIG, ORANGE);
		RenderString(maildb[account].time, 32, 34, 75, RIGHT, BIG, ORANGE);
		RenderString(maildb[account].name, 122, 34, 371, CENTER, BIG, ORANGE);
		RenderString(maildb[account].status, 503, 34, 105, RIGHT, BIG, ORANGE);

		for(loop = 0; loop < 10; loop++)
		{
			RenderString((char*)&maildb[account].mailinfo[mailindex + loop].date, 2, sy, 50, RIGHT, SMALL, WHITE);
			RenderString((char*)&maildb[account].mailinfo[mailindex + loop].time, 2, sy + 22, 50, RIGHT, SMALL, WHITE);
			RenderString((char*)&maildb[account].mailinfo[mailindex + loop].from, 75, sy, 517, LEFT, SMALL, WHITE);
			RenderString((char*)&maildb[account].mailinfo[mailindex + loop].subj, 75, sy + 22, 517, LEFT, SMALL, WHITE);

			RenderCircle(sy - 4, maildb[account].mailinfo[mailindex + loop].type[0]);

			sy += 46;
		}

	//copy backbuffer to framebuffer

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

	//open status file

		msg_file[sizeof(msg_file) - 2] = account | '0';

		if(!(fd_msg = fopen(msg_file, "r")))
		{
			printf("TuxMail <no Status for Account %d>\n", account);

			maildb[account].nr[0] = account | '0';
			memcpy(maildb[account].time, "00:00", 5);
			memcpy(maildb[account].name, "keine Info verfügbar", 20);
			memcpy(maildb[account].status, "000/000", 7);
			maildb[account].mails = 0;
			maildb[account].inactive = 1;
			return;
		}

	//get account, timestamp, name and mailstatus

		fgets(linebuffer, sizeof(linebuffer), fd_msg);
		sscanf(linebuffer, "%s %s %s %s", maildb[account].nr, maildb[account].time, maildb[account].name, maildb[account].status);

	//get date, time, from and subject for every mail

		while(fgets(linebuffer, sizeof(linebuffer), fd_msg))
		{
			maildb[account].mails++;

			strncpy(maildb[account].mailinfo[line].type, (char*)strtok(linebuffer, "|"), sizeof(maildb[account].mailinfo[line].type));
			strncpy(maildb[account].mailinfo[line].uid, (char*)strtok(NULL, "|"), sizeof(maildb[account].mailinfo[line].uid));
			strncpy(maildb[account].mailinfo[line].date, (char*)strtok(NULL, "|"), sizeof(maildb[account].mailinfo[line].date));
			strncpy(maildb[account].mailinfo[line].time, (char*)strtok(NULL, "|"), sizeof(maildb[account].mailinfo[line].time));
			strncpy(maildb[account].mailinfo[line].from, (char*)strtok(NULL, "|"), sizeof(maildb[account].mailinfo[line].from));
			strncpy(maildb[account].mailinfo[line].subj, (char*)strtok(NULL, "|"), sizeof(maildb[account].mailinfo[line].subj));

			maildb[account].mailinfo[line].save[0] = maildb[account].mailinfo[line].type[0];

			line++;
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

	//set current file

		msg_file[sizeof(msg_file) - 2] = account | '0';

	//update

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
		else printf("TuxMail <could not update Status for Account %d>\n", account);
}

/******************************************************************************
 * Add2SpamList (0=fail, 1=done)
 ******************************************************************************/

int Add2SpamList(int account, int mailindex)
{
	FILE *fd_spam;
	char *ptr1, *ptr2;
	char mailaddress[256];

	//open or create spamlist

		if(!(fd_spam = fopen(CFGPATH SPMFILE, "a")))
		{
			printf("TuxMail <could not create Spamlist: %s>\n", strerror(errno));
			return 0;
		}

	//find address

		if((ptr1 = strchr(maildb[account].mailinfo[mailindex].from, '@')))
		{
			while(*(ptr1 - 1) != '\0' && *(ptr1 - 1) != '<') ptr1--;
			ptr2 = ptr1;
			while(*(ptr2) != '\0' && *(ptr2) != '>') ptr2++;

			strncpy(mailaddress, ptr1, ptr2 - ptr1);
			mailaddress[ptr2 - ptr1] = '\0';
		}
		else
		{
			printf("TuxMail <Mailaddress \"%s\" invalid, not added to Spamlist>\n", maildb[account].mailinfo[mailindex].from);

			fclose(fd_spam);

			return 0;
		}

	//add address to spamlist

		printf("TuxMail <Mailaddress \"%s\" added to Spamlist>\n", mailaddress);

		fprintf(fd_spam, "%s\n", mailaddress);

		fclose(fd_spam);

		return 1;
}

/******************************************************************************
 * plugin_exec
 ******************************************************************************/

void plugin_exec(PluginParam *par)
{
	char cvs_revision[] = "$Revision: 1.8 $", versioninfo[12];
	int loop, account, mailindex;
	FILE *fd_run;
	FT_Error error;
	
	//show versioninfo

		sscanf(cvs_revision, "%*s %s", versioninfo);
		printf("TuxMail %s\n", versioninfo);

	//get params

		fb = rc = lcd = sx = ex = sy = ey = -1;

		for(; par; par = par->next)
		{
			if	(!strcmp(par->id, P_ID_FBUFFER)) fb = atoi(par->val);
			else if	(!strcmp(par->id, P_ID_RCINPUT)) rc = atoi(par->val);
			else if	(!strcmp(par->id, P_ID_LCD))	lcd = atoi(par->val);
			else if	(!strcmp(par->id, P_ID_OFF_X))   sx = atoi(par->val);
			else if	(!strcmp(par->id, P_ID_END_X))   ex = atoi(par->val);
			else if	(!strcmp(par->id, P_ID_OFF_Y))   sy = atoi(par->val);
			else if	(!strcmp(par->id, P_ID_END_Y))   ey = atoi(par->val);
		}

		if(fb == -1 || rc == -1 || lcd == -1 || sx == -1 || ex == -1 || sy == -1 || ey == -1)
		{
			printf("TuxMail <missing Param(s)>\n");
			return;
		}

	//init framebuffer

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

		if(ioctl(fb, FBIOPUTCMAP, &colormap) == -1)
		{
			printf("TuxMail <FBIOPUTCMAP failed>\n");
			return;
		}

		if(!(lfb = (unsigned char*)mmap(0, fix_screeninfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0)))
		{
			printf("TuxMail <mapping of Framebuffer failed>\n");
			return;
		}

	//init fontlibrary

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
		desc.flags = FT_LOAD_MONOCHROME;

	//init backbuffer

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

	//get daemon status

		if(!ControlDaemon(GET_STATUS)) online = 2;

	//fill database

		memset(maildb, 0, sizeof(maildb));

		for(loop = 0; loop < 10; loop++) FillDB(loop);

	//remove last key & set rc to blocking mode

#if HAVE_DVB_API_VERSION == 3

		read(rc, &ev, sizeof(ev));

#else

		read(rc, &rccode, sizeof(rccode));

#endif

		fcntl(rc, F_SETFL, fcntl(rc, F_GETFL) &~ O_NONBLOCK);

	//show first account with new mail or account 0

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

	//main loop

		do{
			switch((rccode = GetRCCode()))
			{
				case RC_0:	account = 0;
						mailindex = 0;
						break;

				case RC_1:	account = 1;
						mailindex = 0;
						break;

				case RC_2:	account = 2;
						mailindex = 0;
						break;

				case RC_3:	account = 3;
						mailindex = 0;
						break;

				case RC_4:	account = 4;
						mailindex = 0;
						break;

				case RC_5:	account = 5;
						mailindex = 0;
						break;

				case RC_6:	account = 6;
						mailindex = 0;
						break;

				case RC_7:	account = 7;
						mailindex = 0;
						break;

				case RC_8:	account = 8;
						mailindex = 0;
						break;

				case RC_9:	account = 9;
						mailindex = 0;
						break;

				case RC_MINUS:	mailindex = 0;

						for(loop = 0; loop < 10; loop++)
						{
							if(account > 0) account--;
							else account = 9;

							if(!maildb[account].inactive) break;
						}
						break;

				case RC_PLUS:	mailindex = 0;

						for(loop = 0; loop < 10; loop++)
						{
							if(account < 9) account++;
							else account = 0;

							if(!maildb[account].inactive) break;
						}
						break;


				case RC_UP:	if(mailindex > 0) mailindex--;
						break;

				case RC_DOWN:	if(mailindex < maildb[account].mails-1) mailindex++;
						break;

				case RC_LEFT:	if(!(mailindex%10) && mailindex) mailindex--;
						else mailindex = (mailindex/10)*10;
						break;

				case RC_RIGHT:	if(mailindex%10 == 9) mailindex++;
						else mailindex = (mailindex/10)*10 + 9;

						if(mailindex >= maildb[account].mails)
						{
							if(maildb[account].mails) mailindex = maildb[account].mails - 1;
							else mailindex = 0;
						}
						break;

				case RC_OK:	if(maildb[account].mails)
						{
							if(maildb[account].mailinfo[mailindex].type[0] != 'D') maildb[account].mailinfo[mailindex].type[0] = 'D';
							else
							{
								maildb[account].mailinfo[mailindex].type[0] = maildb[account].mailinfo[mailindex].save[0];
								if(maildb[account].mailinfo[mailindex].type[0] == 'D') maildb[account].mailinfo[mailindex].type[0] = 'O';
							}
						}
						break;

				case RC_RED:	if(maildb[account].mark_green && maildb[account].mark_yellow) maildb[account].mark_red = 1;
						else if(!maildb[account].mark_green && !maildb[account].mark_yellow) maildb[account].mark_red = 0;
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
						break;

				case RC_GREEN:	maildb[account].mark_green++;
						maildb[account].mark_green &= 1;

						if(maildb[account].mark_green)
						{
							for(loop = 0;loop < maildb[account].mails; loop++)
							{
								if(maildb[account].mailinfo[loop].type[0] == 'N') maildb[account].mailinfo[loop].type[0] = 'D';
							}
						}
						else
						{
							for(loop = 0;loop < maildb[account].mails; loop++)
							{
								if(maildb[account].mailinfo[loop].save[0] == 'N') maildb[account].mailinfo[loop].type[0] = 'N';
							}
						}
						break;

				case RC_YELLOW:	maildb[account].mark_yellow++;
						maildb[account].mark_yellow &= 1;

						if(maildb[account].mark_yellow)
						{
							for(loop = 0;loop < maildb[account].mails; loop++)
							{
								if(maildb[account].mailinfo[loop].type[0] == 'O') maildb[account].mailinfo[loop].type[0] = 'D';
							}
						}
						else
						{
							for(loop = 0;loop < maildb[account].mails; loop++)
							{
								if(maildb[account].mailinfo[loop].save[0] == 'O') maildb[account].mailinfo[loop].type[0] = 'O';
							}
						}
						break;

				case RC_BLUE:	if(Add2SpamList(account, mailindex))
						{
							maildb[account].mailinfo[mailindex].type[0] = 'D';
							ControlDaemon(RELOAD_SPAMLIST);
							ShowMessage(ADD2SPAM);
						}
						else ShowMessage(SPAMFAIL);
						break;

				case RC_MUTE:	if(!ControlDaemon(GET_STATUS))
						{
							online = 2;
							ShowMessage(NODAEMON);
						}
						else
						{
							online++;
							online &= 1;

							if(!ControlDaemon(SET_STATUS))
							{
								if(online) ShowMessage(STARTFAIL);
								else ShowMessage(STOPFAIL);

								online++;
								online &= 1;
							}
							else
							{
								if(online) ShowMessage(STARTDONE);
								else ShowMessage(STOPDONE);
							}
						}
						break;

				case RC_STANDBY:if((fd_run = fopen(RUNFILE, "r")))
						{
							fclose(fd_run);
							remove(RUNFILE);
							ShowMessage(BOOTOFF);
						}
						else
						{
							fclose(fopen(RUNFILE, "w"));
							ShowMessage(BOOTON);
						}
						break;

				default:	continue;
			}

			ShowMailInfo(account, mailindex);

		}while(rccode != RC_HOME);

	//update database

		for(loop = 0; loop < 10; loop++) UpdateDB(loop);

	//cleanup

		FTC_Manager_Done(manager);
		FT_Done_FreeType(library);

		free(lbb);
		munmap(lfb, fix_screeninfo.smem_len);

		fcntl(rc, F_SETFL, O_NONBLOCK);

		return;
}
