/******************************************************************************
 *                  <<< TuxCom - TuxBox-Commander Plugin >>>                  *
 *                                                                            *
 *             (c) dbluelle 2004 (dbluelle@blau-weissoedingen.de)             *
 ******************************************************************************/

#include "tuxcom.h"

/******************************************************************************
 * GetRCCode  (Code from TuxTxt)
 ******************************************************************************/

int GetRCCode(int scrollmode)
{

#if TUXCOM_DBOX_VERSION < 3
	static unsigned short LastKey = -1;
#else
	struct input_event ev;
	static __u16 rc_last_key = KEY_RESERVED;
#endif
	//get code
#if TUXCOM_DBOX_VERSION < 3
	if(read(rc, &rccode, 2) == 2)
	{
		if (rccode != LastKey)
		{
			LastKey = rccode;

			if ((rccode & 0xFF00) == 0x5C00)
			{
				switch(rccode)
#else
	if(read(rc, &ev, sizeof(ev)) == sizeof(ev))
	{
		if(ev.value)
		{
			if(ev.code != rc_last_key)
			{
				rc_last_key = ev.code;
				switch(ev.code)
#endif
				{

					case KEY_UP:
// modification to allow keeping button pressed
#if TUXCOM_DBOX_VERSION < 3
										if (LastKey == rccode) LastKey = -1;
#else
										rc_last_key = KEY_RESERVED;
#endif
										rccode = RC_UP;
										break;

					case KEY_DOWN:
// modification to allow keeping button pressed
#if TUXCOM_DBOX_VERSION < 3
										if (LastKey == rccode) LastKey = -1;
#else
										rc_last_key = KEY_RESERVED;
#endif
										rccode = RC_DOWN;
										break;

					case KEY_LEFT:		if (scrollmode != NOSCROLL_LEFTRIGHT)
					                    {
// modification to allow keeping button pressed
#if TUXCOM_DBOX_VERSION < 3
											if (LastKey == rccode) LastKey = -1;
#else
											rc_last_key = KEY_RESERVED;
#endif
										}
										rccode = RC_LEFT;
										break;

					case KEY_RIGHT:		if (scrollmode != NOSCROLL_LEFTRIGHT)
					                    {
// modification to allow keeping button pressed
#if TUXCOM_DBOX_VERSION < 3
											if (LastKey == rccode) LastKey = -1;
#else
											rc_last_key = KEY_RESERVED;
#endif
										}
										rccode = RC_RIGHT;
										break;

					case KEY_OK:		rccode = RC_OK;
										break;

					case KEY_0:			rccode = RC_0;
										break;

					case KEY_1:			rccode = RC_1;
										break;

					case KEY_2:			rccode = RC_2;
										break;

					case KEY_3:			rccode = RC_3;
										break;

					case KEY_4:			rccode = RC_4;
										break;

					case KEY_5:			rccode = RC_5;
										break;

					case KEY_6:			rccode = RC_6;
										break;

					case KEY_7:			rccode = RC_7;
										break;

					case KEY_8:			rccode = RC_8;
										break;

					case KEY_9:			rccode = RC_9;
										break;

					case KEY_RED:		rccode = RC_RED;
										break;

					case KEY_GREEN:		if (scrollmode == SCROLL_GREEN)
					                    {
// modification to allow keeping button pressed
#if TUXCOM_DBOX_VERSION < 3
											if (LastKey == rccode) LastKey = -1;
#else
											rc_last_key = KEY_RESERVED;
#endif
										}
										rccode = RC_GREEN;
										break;

					case KEY_YELLOW:	rccode = RC_YELLOW;
										break;

					case KEY_BLUE:		rccode = RC_BLUE;
										break;

					case KEY_VOLUMEUP:
// modification to allow keeping button pressed
#if TUXCOM_DBOX_VERSION < 3
										if (LastKey == rccode) LastKey = -1;
#else
										rc_last_key = KEY_RESERVED;
#endif
										rccode = RC_PLUS;
										break;

					case KEY_VOLUMEDOWN:
// modification to allow keeping button pressed
#if TUXCOM_DBOX_VERSION < 3
										if (LastKey == rccode) LastKey = -1;
#else
										rc_last_key = KEY_RESERVED;
#endif
										rccode = RC_MINUS;
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
				}
				return 1;
			}
#if TUXCOM_DBOX_VERSION < 3
			else
			{
				rccode &= 0x003F;
			}
#endif
		}
#if TUXCOM_DBOX_VERSION < 3
		else
		{
			rccode = -1;
		}
		return 1;
#else
		else
		{
			rccode = 0;
			rc_last_key = KEY_RESERVED;
		}
#endif
	}

		rccode = 0;
		usleep(1000000/100);
		return 0;
}

/******************************************************************************
 * MyFaceRequester
 ******************************************************************************/

FT_Error MyFaceRequester(FTC_FaceID face_id, FT_Library library, FT_Pointer request_data, FT_Face *aface)
{
	FT_Error result;

	result = FT_New_Face(library, face_id, 0, aface);

	if(!result) printf("TuxCom <Font \"%s\" loaded>\n", (char*)face_id);
	else        printf("TuxCom <Font \"%s\" failed>\n", (char*)face_id);

	return result;
}

/******************************************************************************
 * RenderChar
 ******************************************************************************/

int RenderChar(FT_ULong currentchar, int sx, int sy, int ex, int color)
{
	int row, pitch, bit, x = 0, y = 0;
	FT_UInt glyphindex;
	FT_Vector kerning;
	FT_Error error;

	//load char

		if(!(glyphindex = FT_Get_Char_Index(face, currentchar)))
		{
			printf("TuxCom <FT_Get_Char_Index for Char \"%c\" failed: \"undefined character code\">\n", (int)currentchar);
			return 0;
		}


#if TUXCOM_DBOX_VERSION < 2
		if((error = FTC_SBit_Cache_Lookup(cache, &desc, glyphindex, &sbit)))
#else
		FTC_Node anode;
		if((error = FTC_SBitCache_Lookup(cache, &desc, glyphindex, &sbit, &anode)))
#endif
		{
			printf("TuxCom <FTC_SBitCache_Lookup for Char \"%c\" failed with Errorcode 0x%.2X>\n", (int)currentchar, error);
			return 0;
		}

// no kerning used
/*
		if(use_kerning)
		{
			FT_Get_Kerning(face, prev_glyphindex, glyphindex, ft_kerning_default, &kerning);

			prev_glyphindex = glyphindex;
			kerning.x >>= 6;
		}
		else
*/
			kerning.x = 0;

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

						if((sbit->buffer[row * sbit->pitch + pitch]) & 1<<bit) *(lbb + StartX + sx + sbit->left + kerning.x + x + var_screeninfo.xres*(StartY + sy - sbit->top + y)) = color;

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

int GetStringLen(unsigned char *string, int size)
{
	int stringlen = 0;

	//set size

		switch (size)
		{
			case VERY_SMALL: desc.font.pix_width = desc.font.pix_height = FONTHEIGHT_VERY_SMALL; break;
			case SMALL     : desc.font.pix_width = desc.font.pix_height = FONTHEIGHT_SMALL     ; break;
		    case BIG       : desc.font.pix_width = desc.font.pix_height = FONTHEIGHT_BIG       ; break;
	 	}

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

		switch (size)
		{
			case VERY_SMALL: desc.font.pix_width = desc.font.pix_height = FONTHEIGHT_VERY_SMALL; break;
			case SMALL     : desc.font.pix_width = desc.font.pix_height = FONTHEIGHT_SMALL     ; break;
		    case BIG       : desc.font.pix_width = desc.font.pix_height = FONTHEIGHT_BIG       ; break;
	 	}

	//set alignment

		if(layout != LEFT)
		{
			stringlen = GetStringLen(string, size);

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

		while(*string != '\0' && *string != '\n')
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
	if(mode == FILL)
	{
		for(; sy <= ey; sy++)
		{
			memset(lbb + StartX + sx + var_screeninfo.xres*(StartY + sy), color, ex-sx + 1);
		}
	}
	else
	{
		//hor lines

			for(loop = sx; loop <= ex; loop++)
			{
				*(lbb + StartX+loop + var_screeninfo.xres*(sy+StartY)) = color;
				*(lbb + StartX+loop + var_screeninfo.xres*(sy+1+StartY)) = color;

				*(lbb + StartX+loop + var_screeninfo.xres*(ey-1+StartY)) = color;
				*(lbb + StartX+loop + var_screeninfo.xres*(ey+StartY)) = color;
			}

		//ver lines

			for(loop = sy; loop <= ey; loop++)
			{
				*(lbb + StartX+sx + var_screeninfo.xres*(loop+StartY)) = color;
				*(lbb + StartX+sx+1 + var_screeninfo.xres*(loop+StartY)) = color;

				*(lbb + StartX+ex-1 + var_screeninfo.xres*(loop+StartY)) = color;
				*(lbb + StartX+ex + var_screeninfo.xres*(loop+StartY)) = color;
			}
	}
}


/******************************************************************************
 * plugin_exec                                                                *
 ******************************************************************************/

void plugin_exec(PluginParam *par)
{

	FT_Error error;
	//show versioninfo
	
	printf(MSG_VERSION);
	char szMessage[400];

	//get params


	fb = rc = sx = ex = sy = ey = -1;

	for(; par; par = par->next)
	{
		if	(!strcmp(par->id, P_ID_FBUFFER)) fb = atoi(par->val);
		else if	(!strcmp(par->id, P_ID_RCINPUT)) rc = atoi(par->val);
		else if	(!strcmp(par->id, P_ID_OFF_X))   sx = atoi(par->val);
		else if	(!strcmp(par->id, P_ID_END_X))   ex = atoi(par->val);
		else if	(!strcmp(par->id, P_ID_OFF_Y))   sy = atoi(par->val);
		else if	(!strcmp(par->id, P_ID_END_Y))   ey = atoi(par->val);
	}

	if(fb == -1 || rc == -1 || sx == -1 || ex == -1 || sy == -1 || ey == -1)
	{
		printf("TuxCom <missing Param(s)>\n");
		return;
	}

	//init framebuffer

	if(ioctl(fb, FBIOGET_FSCREENINFO, &fix_screeninfo) == -1)
	{
		printf("TuxCom <FBIOGET_FSCREENINFO failed>\n");
		return;
	}

	if(ioctl(fb, FBIOGET_VSCREENINFO, &var_screeninfo) == -1)
	{
		printf("TuxCom <FBIOGET_VSCREENINFO failed>\n");
		return;
	}

	if(ioctl(fb, FBIOPUTCMAP, &colormap) == -1)
	{
		printf("TuxCom <FBIOPUTCMAP failed>\n");
		return;
	}

	if(!(lfb = (unsigned char*)mmap(0, fix_screeninfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0)))
	{
		printf("TuxCom <mapping of Framebuffer failed>\n");
		return;
	}

	//init fontlibrary

	if((error = FT_Init_FreeType(&library)))
	{
		printf("TuxCom <FT_Init_FreeType failed with Errorcode 0x%.2X>", error);
		munmap(lfb, fix_screeninfo.smem_len);
		return;
	}

	if((error = FTC_Manager_New(library, 1, 2, 0, &MyFaceRequester, NULL, &manager)))
	{
		printf("TuxCom <FTC_Manager_New failed with Errorcode 0x%.2X>\n", error);
		FT_Done_FreeType(library);
		munmap(lfb, fix_screeninfo.smem_len);
		return;
	}

	if((error = FTC_SBitCache_New(manager, &cache)))
	{
		printf("TuxCom <FTC_SBitCache_New failed with Errorcode 0x%.2X>\n", error);
		FTC_Manager_Done(manager);
		FT_Done_FreeType(library);
		munmap(lfb, fix_screeninfo.smem_len);
		return;
	}

	if((error = FTC_Manager_Lookup_Face(manager, FONT, &face)))
	{
		if((error = FTC_Manager_Lookup_Face(manager, FONT2, &face)))
		{
			printf("TuxCom <FTC_Manager_Lookup_Face failed with Errorcode 0x%.2X>\n", error);
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return;
		}
		else
			desc.font.face_id = FONT2;
	}
	else
		desc.font.face_id = FONT;


	use_kerning = FT_HAS_KERNING(face);

#if TUXCOM_DBOX_VERSION > 1
	desc.flags = FT_LOAD_MONOCHROME;
#else
	desc.image_type = ftc_image_mono;
//	desc.type = ftc_image_mono;
#endif


	//init backbuffer

	if(!(lbb = malloc(var_screeninfo.xres*var_screeninfo.yres)))
	{
		printf("TuxCom <allocating of Backbuffer failed>\n");
		FTC_Manager_Done(manager);
		FT_Done_FreeType(library);
		munmap(lfb, fix_screeninfo.smem_len);
		return;
	}

	memset(lbb, 0, var_screeninfo.xres*var_screeninfo.yres);
	RenderBox(0,0,var_screeninfo.xres,var_screeninfo.yres,FILL,BLACK);


	//init data
	curframe = 0;
	cursort = SORT_UP;
	curvisibility = 0;
	commandsize =sysconf(_SC_ARG_MAX )-100;
	szCommand = (char*)malloc(commandsize);
	szCommand [0]= 0x00;
	szZipCommand = (char*)malloc(commandsize);
	szZipCommand [0]= 0x00;
	memset(tool, ACTION_NOACTION, sizeof(tool));
	colortool[0] = ACTION_EXEC   ;
	colortool[1] = ACTION_MARKER ;
	colortool[2] = ACTION_SORT   ;
	colortool[3] = ACTION_REFRESH;

	memset(&finfo[0], 0, sizeof(finfo[0]));
	memset(&finfo[1], 0, sizeof(finfo[0]));
	ReadSettings();

	printf("Settings read\n");

	// center output on screen
	StartX = sx;
	StartY = sy;
	viewx = ex - sx;
	viewy = ey - sy;
    menuitemwidth  = viewx / MENUITEMS;
    menuitemnumber = viewx / (MENUITEMS*6);

	framerows = (viewy-MENUSIZE - 3*BORDERSIZE - FONTHEIGHT_SMALL) / FONTHEIGHT_SMALL;

	FrameWidth = viewx/2;
	NameWidth = (FrameWidth / 3 ) * 2;
	SizeWidth = (FrameWidth / 3 ) - 3* BORDERSIZE;

	if (strncmp(setlocale( LC_ALL, NULL ),"de",2) == 0) language=LANG_DE; else language=LANG_INT;
	// setup screen
	FillDir(1-curframe,SELECT_NOCHANGE);
	FillDir(  curframe,SELECT_NOCHANGE);

	RenderFrame(LEFTFRAME);
	RenderFrame(RIGHTFRAME);
	memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
	printf("TuxCom init successful\n");

	fcntl(rc, F_SETFL, fcntl(rc, F_GETFL) &~ O_NONBLOCK);

	struct fileentry *pfe;
	char action[256];
	char szSel [256];
	int pos, check;
	do{
		overwriteall = 0;
		skipall = 0;
		GetRCCode(SCROLL_GREEN);
		switch(rccode)
		{
				case RC_HELP:
					MessageBox(MSG_VERSION,MSG_COPYRIGHT,OK);
					break;
				case RC_OK:
					pfe = GetSelected(curframe);
					if (pfe && S_ISDIR(pfe->fentry.st_mode))
					{
						if (strcmp(pfe->name,"..") == 0)
						{
							ClearMarker(curframe);
							FillDir(curframe,SELECT_UPDIR);
						}
						else
						{
							if (finfo[curframe].zipfile[0] != 0x00)
							{
								strncat(finfo[curframe].zippath,pfe->name,256);
								strncat(finfo[curframe].zippath,"/",1);
							}
							else
							{
								strncat(finfo[curframe].path,pfe->name,256);
								strncat(finfo[curframe].path,"/",1);
							}
							finfo[curframe].selected =0;
							finfo[curframe].first    =0;
							ClearMarker(curframe);
							FillDir(curframe,SELECT_NOCHANGE);
						}
					}
					else if (pfe && S_ISLNK(pfe->fentry.st_mode))
					{
						struct stat fs;
						char fullfile[FILENAME_MAX];
						sprintf(fullfile,"%s%s",finfo[curframe].path,pfe->name);
						stat(fullfile,&fs);
						if (S_ISDIR(fs.st_mode))
						{
							strncat(finfo[curframe].path,pfe->name,256);
							strncat(finfo[curframe].path,"/",1);
							finfo[curframe].selected =0;
							finfo[curframe].first    =0;
							ClearMarker(curframe);
							FillDir(curframe,SELECT_NOCHANGE);
						}
						else
						{
							if (pfe && ((pfe->fentry.st_mode & S_IXUSR) == S_IXUSR))
							{
								sprintf(szMessage,msg[MSG_EXEC*NUM_LANG+language], pfe->name);
								switch (MessageBox(szMessage,info[INFO_EXEC*NUM_LANG+language],OKHIDDENCANCEL))
								{
									case YES:
										sprintf(action,"\"%s%s\"",finfo[curframe].path, pfe->name);
										DoExecute(action, SHOW_OUTPUT);
										FillDir(1-curframe,SELECT_NOCHANGE);
										FillDir(  curframe,SELECT_NOCHANGE);
										break;
									case HIDDEN:
										sprintf(action,"\"%s%s\" &",finfo[curframe].path, pfe->name);
										DoExecute(action, SHOW_NO_OUTPUT);
										break;
									default:
										rccode = 0;
								}
							}
						}

					}
					else if (pfe && ((pfe->fentry.st_mode & S_IRUSR) == S_IRUSR) && ((check = CheckZip(pfe->name))>= GZIP))
					{
						ReadZip(check);
						FillDir(curframe,SELECT_NOCHANGE);
					}
					else if (pfe && ((pfe->fentry.st_mode & S_IXUSR) == S_IXUSR) && finfo[curframe].zipfile[0] == 0x00)
					{
						sprintf(szMessage,msg[MSG_EXEC*NUM_LANG+language], pfe->name);
						switch (MessageBox(szMessage,info[INFO_EXEC*NUM_LANG+language],OKHIDDENCANCEL))
						{
							case YES:
								sprintf(action,"\"%s%s\"",finfo[curframe].path, pfe->name);
								DoExecute(action, SHOW_OUTPUT);
								FillDir(1-curframe,SELECT_NOCHANGE);
								FillDir(  curframe,SELECT_NOCHANGE);
								break;
							case HIDDEN:
								sprintf(action,"\"%s%s\" &",finfo[curframe].path, pfe->name);
								DoExecute(action, SHOW_NO_OUTPUT);
								break;
							default:
								rccode = 0;
						}
					}
					break;
				case RC_LEFT:
					curframe = 0;
					break;
				case RC_RIGHT:
					curframe = 1;
					break;
				case RC_UP:
					finfo[curframe].selected--;
					if (finfo[curframe].selected  < 0)
						finfo[curframe].selected  = finfo[curframe].count -1;
					break;
				case RC_DOWN:
					finfo[curframe].selected++;
					if (finfo[curframe].selected >= finfo[curframe].count)
						finfo[curframe].selected  = 0;
					break;
				case RC_PLUS:
					finfo[curframe].selected-= framerows;
					break;
				case RC_MINUS:
					finfo[curframe].selected+= framerows;
					break;
				case RC_1:
					if (tool[ACTION_PROPS-1] == ACTION_PROPS)
					{
						RenderMenuLine(ACTION_PROPS-1, YES);
						if (ShowProperties() == YES)
						{
							FillDir(1-curframe,SELECT_NOCHANGE);
							FillDir(  curframe,SELECT_NOCHANGE);
						}
					}
					break;
				case RC_2:
					if (tool[ACTION_RENAME-1] == ACTION_RENAME)
					{
						RenderMenuLine(ACTION_RENAME-1, YES);
						pfe = GetSelected(curframe);
						char szBuf[256];
						char szMsg[356];
						strcpy(szBuf,pfe->name);
						sprintf(szMsg,msg[MSG_RENAME*NUM_LANG+language], pfe->name);
						int nok = 0;
						while (nok == 0)
						{
							switch (GetInputString(400,255,szBuf,szMsg))
							{
								case RC_OK:
								{
									if (*szBuf == 0x00)
									{
										nok = 1;
										break;
									}
									if (FindFile(curframe,szBuf) != NULL)
									{
										char szMsg2[356];
										sprintf(szMsg2,msg[MSG_FILE_EXISTS*NUM_LANG+language], szBuf);
										MessageBox(szMsg2,"",OK);
										break;
									}
									else
									{
										char szOld[FILENAME_MAX],szNew[FILENAME_MAX];
										sprintf(szOld,"%s%s",finfo[curframe].path, pfe->name);
										sprintf(szNew,"%s%s",finfo[curframe].path, szBuf    );
										rename(szOld,szNew);
										RenameMarker(curframe,pfe->name,szBuf);
										if (strcmp(finfo[curframe].path,finfo[1-curframe].path) == 0)
											RenameMarker(1-curframe,pfe->name,szBuf);
										FillDir(1-curframe,SELECT_NOCHANGE);
										FillDir(  curframe,SELECT_NOCHANGE);
										nok = 1;
									}
								}
								default:
									rccode = 0;
									nok = 1;
							}
						}
					}
					break;
				case RC_3:
					if (tool[ACTION_VIEW-1] == ACTION_VIEW)
					{
						RenderMenuLine(ACTION_VIEW-1, YES);
						pfe = GetSelected(curframe);
						DoViewFile(pfe->name);
					}
					break;
				case RC_4:
					if (tool[ACTION_EDIT-1] == ACTION_EDIT)
					{
						colortool[0] = ACTION_DELLINE ;
						colortool[1] = ACTION_INSLINE ;
						colortool[2] = ACTION_NOACTION;
						colortool[3] = ACTION_NOACTION;
						RenderMenuLine(ACTION_EDIT-1, YES);
						pfe = GetSelected(curframe);
						sprintf(action,"%s%s",finfo[curframe].path, pfe->name);
						DoEditFile(action);
					}
					break;
				case RC_5:
					if (tool[ACTION_COPY-1] == ACTION_COPY)
					{
						szZipCommand[0] = 0x00;
						tmpzipdir[0] = 0x00;
						RenderMenuLine(ACTION_COPY-1, YES);
						pfe = GetSelected(curframe);
						if ((finfo[curframe].zipfile[0] == 0x00) && (strcmp(finfo[curframe].path, finfo[1-curframe].path) == 0))
						{
							MessageBox(msg[MSG_COPY_NOT_POSSIBLE*NUM_LANG+language],"",OK);
						}
						else
						{
							if (finfo[curframe].markcount > 0)
							{
								sprintf(szMessage,msg[MSG_COPY_MULTI*NUM_LANG+language], finfo[curframe].markcount, finfo[1-curframe].path);
								switch (MessageBox(szMessage,(finfo[curframe].zipfile[0] == 0x00 ? info[INFO_COPY*NUM_LANG+language] :""),(finfo[curframe].zipfile[0] == 0x00 ? OKHIDDENCANCEL: OKCANCEL )))
								{
									case YES:
									    for (pos = 0; pos < finfo[curframe].count; pos++)
									    {
											if (IsMarked(curframe,pos))
											{
												pfe = getfileentry(curframe, pos);
												check = CheckOverwrite(pfe->name, OVERWRITESKIPCANCEL);
												if (check < 0) break;
												if (check == SKIP) continue;
												DoCopy(pfe,YES);
											}
										}
										DoZipCopyEnd();
										FillDir(1-curframe,SELECT_NOCHANGE);
										FillDir(  curframe,SELECT_NOCHANGE);
										break;
									case HIDDEN:
									    for (pos = 0; pos < finfo[curframe].count; pos++)
									    {
											if (IsMarked(curframe,pos))
											{
												pfe = getfileentry(curframe, pos);
												check = CheckOverwrite(pfe->name, OVERWRITESKIPCANCEL);
												if (check < 0) break;
												if (check == SKIP) continue;
												DoCopy(pfe,HIDDEN);
											}
										}
										break;
									default:
										rccode = 0;
								}
							}
							else
							{
								sprintf(szMessage,msg[MSG_COPY*NUM_LANG+language], pfe->name, finfo[1-curframe].path);
								switch (MessageBox(szMessage,(finfo[curframe].zipfile[0] == 0x00 ? info[INFO_COPY*NUM_LANG+language]:""),(finfo[curframe].zipfile[0] == 0x00 ? OKHIDDENCANCEL : OKCANCEL )))
								{
									case YES:
										if (CheckOverwrite(pfe->name, OVERWRITECANCEL) < 0) break;
										DoCopy(pfe,YES);
										DoZipCopyEnd();
										FillDir(1-curframe,SELECT_NOCHANGE);
										FillDir(  curframe,SELECT_NOCHANGE);
										break;
									case HIDDEN:
										if (CheckOverwrite(pfe->name, OVERWRITECANCEL) < 0) break;
										DoCopy(pfe,HIDDEN);
										break;
									default:
										rccode = 0;
								}
							}
						}
					}
					break;
				case RC_6:
					if (tool[ACTION_MOVE-1] == ACTION_MOVE)
					{
						RenderMenuLine(ACTION_MOVE-1, YES);
						pfe = GetSelected(curframe);
						if (finfo[curframe].markcount > 0)
						{
							sprintf(szMessage,msg[MSG_MOVE_MULTI*NUM_LANG+language], finfo[curframe].markcount, finfo[1-curframe].path);
							switch (MessageBox(szMessage,info[INFO_MOVE*NUM_LANG+language],OKHIDDENCANCEL))
							{
								case YES:
									for (pos = 0; pos < finfo[curframe].count; pos++)
									{
										if (IsMarked(curframe,pos))
										{
											pfe = getfileentry(curframe, pos);
											check = CheckOverwrite(pfe->name, OVERWRITESKIPCANCEL);
											if (check < 0) break;
											if (check == SKIP) continue;
											sprintf(szMessage,msg[MSG_MOVE_PROGRESS*NUM_LANG+language], pfe->name, finfo[1-curframe].path);
											MessageBox(szMessage,"",NOBUTTON);
											DoMove(pfe->name, YES);
										}
									}
									ClearMarker(curframe);
									FillDir(1-curframe,SELECT_NOCHANGE);
									FillDir(  curframe,SELECT_NOCHANGE);
									break;
								case HIDDEN:
									for (pos = 0; pos < finfo[curframe].count; pos++)
									{
										if (IsMarked(curframe,pos))
										{
											pfe = getfileentry(curframe, pos);
											check = CheckOverwrite(pfe->name, OVERWRITESKIPCANCEL);
											if (check < 0) break;
											if (check == SKIP) continue;
											DoMove(pfe->name, HIDDEN);
										}
									}
									ClearMarker(curframe);
									break;
								default:
									rccode = 0;
							}
						}
						else
						{
							sprintf(szMessage,msg[MSG_MOVE*NUM_LANG+language], pfe->name, finfo[1-curframe].path);
							switch (MessageBox(szMessage,info[INFO_MOVE*NUM_LANG+language],OKHIDDENCANCEL))
							{
								case YES:
									if (CheckOverwrite(pfe->name, OVERWRITECANCEL) < 0) break;
									sprintf(action,"mv -f \"%s%s\" \"%s\" ",finfo[curframe].path,pfe->name, finfo[1-curframe].path);
									sprintf(szMessage,msg[MSG_MOVE_PROGRESS*NUM_LANG+language], pfe->name, finfo[1-curframe].path);
									MessageBox(szMessage,"",NOBUTTON);
									DoExecute(action, SHOW_NO_OUTPUT);
									FillDir(1-curframe,SELECT_NOCHANGE);
									FillDir(  curframe,SELECT_NOCHANGE);
									break;
								case HIDDEN:
									if (CheckOverwrite(pfe->name, OVERWRITECANCEL) < 0) break;
									sprintf(action,"mv -f \"%s%s\" \"%s\" &",finfo[curframe].path,pfe->name, finfo[1-curframe].path);
									DoExecute(action, SHOW_NO_OUTPUT);
									break;
								default:
									rccode = 0;
							}
						}
					}
					break;
				case RC_7:
					if (tool[ACTION_MKDIR-1] == ACTION_MKDIR)
					{
						RenderMenuLine(ACTION_MKDIR-1, YES);
						char szDir[FILENAME_MAX];
						szDir[0] = 0x00;
						char szMsg[356];
						sprintf(szMsg,msg[MSG_MKDIR*NUM_LANG+language]);
						switch (GetInputString(400,255,szDir,szMsg))
						{
							case RC_OK:
							{
								if (*szDir != 0x00)
								{
									sprintf(action,"mkdir -p \"%s%s\"",finfo[curframe].path, szDir);
									DoExecute(action, SHOW_NO_OUTPUT);
									FillDir(1-curframe,SELECT_NOCHANGE);
									FillDir(  curframe,SELECT_NOCHANGE);
								}
							}
							default:
								rccode = 0;
						}
					}
					break;
				case RC_8:
					if (tool[ACTION_DELETE-1] == ACTION_DELETE)
					{
						RenderMenuLine(ACTION_DELETE-1, YES);
						pfe = GetSelected(curframe);
						if (finfo[curframe].markcount > 0)
						{
							sprintf(szMessage,msg[MSG_DELETE_MULTI*NUM_LANG+language], finfo[curframe].markcount);
							if (MessageBox(szMessage,"",OKCANCEL) == YES)
							{
								for (pos = 0; pos < finfo[curframe].count; pos++)
								{
									if (IsMarked(curframe,pos))
									{
										pfe = getfileentry(curframe, pos);
										sprintf(szMessage,msg[MSG_DELETE_PROGRESS*NUM_LANG+language], pfe->name);
										MessageBox(szMessage,"",NOBUTTON);
										sprintf(action,"rm -f -r \"%s%s\"",finfo[curframe].path,pfe->name);
										DoExecute(action, SHOW_NO_OUTPUT);
									}
								}
								ClearMarker(curframe);
								FillDir(1-curframe,SELECT_NOCHANGE);
								FillDir(  curframe,SELECT_NOCHANGE);
							}
						}
						else
						{
							sprintf(szMessage,msg[MSG_DELETE*NUM_LANG+language], pfe->name);
							if (MessageBox(szMessage,"",OKCANCEL) == YES)
							{
								sprintf(szMessage,msg[MSG_DELETE_PROGRESS*NUM_LANG+language], pfe->name);
								MessageBox(szMessage,"",NOBUTTON);
								sprintf(action,"rm -f -r \"%s%s\"",finfo[curframe].path,pfe->name);
								DoExecute(action, SHOW_NO_OUTPUT);
								FillDir(1-curframe,SELECT_NOCHANGE);
								FillDir(  curframe,SELECT_NOCHANGE);
							}
						}
						rccode = 0;
					}
					break;
				case RC_9:
					if (tool[ACTION_MKFILE-1] == ACTION_MKFILE)
					{
						RenderMenuLine(ACTION_MKFILE-1, YES);
						char szDir[FILENAME_MAX];
						szDir[0] = 0x00;
						char szMsg[356];
						sprintf(szMsg,msg[MSG_MKFILE*NUM_LANG+language], finfo[curframe].path);
						switch (GetInputString(400,255,szDir,szMsg))
						{
							case RC_OK:
							{
								if (*szDir != 0x00)
								{
									sprintf(action,"touch \"%s%s\"",finfo[curframe].path, szDir);
									DoExecute(action, SHOW_NO_OUTPUT);
									FillDir(1-curframe,SELECT_NOCHANGE);
									FillDir(  curframe,SELECT_NOCHANGE);
								}
							}
							default:
								rccode = 0;
						}
					}
					break;
				case RC_0:
					if (tool[ACTION_MKLINK-1] == ACTION_MKLINK)
					{
						RenderMenuLine(ACTION_MKLINK-1, YES);
						char szDir[FILENAME_MAX];
						szDir[0] = 0x00;
						pfe = GetSelected(curframe);
						char szMsg[356];
						sprintf(szMsg,msg[MSG_MKLINK*NUM_LANG+language], finfo[curframe].path, pfe->name, finfo[1-curframe].path);
						switch (GetInputString(400,255,szDir,szMsg))
						{
							case RC_OK:
							{
								if (*szDir != 0x00)
								{
									sprintf(action,"ln -s \"%s%s\" \"%s%s\"",finfo[curframe].path, pfe->name,finfo[1-curframe].path, szDir);
									DoExecute(action, SHOW_NO_OUTPUT);
									FillDir(1-curframe,SELECT_NOCHANGE);
									FillDir(  curframe,SELECT_NOCHANGE);
								}
							}
							default:
								rccode = 0;
						}
					}
					break;
				case RC_RED:
					{
						char szMsg[356];
						sprintf(szMsg,msg[MSG_COMMAND*NUM_LANG+language]);
						switch (GetInputString(400,commandsize,szCommand,szMsg))
						{
							case RC_OK:
								DoExecute(szCommand, SHOW_OUTPUT);
								FillDir(1-curframe,SELECT_NOCHANGE);
								FillDir(  curframe,SELECT_NOCHANGE);
								break;
							default:
								rccode = 0;
						}
					}
					break;
				case RC_GREEN: // toggle marker
				    ToggleMarker(curframe);
					finfo[curframe].selected++;
				    break;
				case RC_YELLOW:
					cursort = finfo[curframe].sort = finfo[curframe].sort * -1;
					strcpy(szSel,GetSelected(curframe)->name);
					sortframe(curframe, szSel);
					break;
				case RC_BLUE: // Refresh
					FillDir(1-curframe,SELECT_NOCHANGE);
					FillDir(  curframe,SELECT_NOCHANGE);
					break;
				case RC_MUTE: // toggle visibility
					curvisibility++;
					if (curvisibility > 2) curvisibility = 0;
					break;

				default:
					continue;
		}
		if (finfo[curframe].selected  < 0)
			finfo[curframe].selected  = 0;
		if (finfo[curframe].selected >= finfo[curframe].count)
			finfo[curframe].selected  = finfo[curframe].count -1;
		if (finfo[curframe].first     > finfo[curframe].selected)
			finfo[curframe].first     = finfo[curframe].selected;
		if (finfo[curframe].selected >= finfo[curframe].first + framerows)
			finfo[curframe].first     = finfo[curframe].selected - framerows+1;
		RenderFrame(LEFTFRAME);
		RenderFrame(RIGHTFRAME);
		memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);

	}while(rccode != RC_HOME);


	rccode = -1;
	FTC_Manager_Done(manager);
	FT_Done_FreeType(library);

	free(lbb);
	munmap(lfb, fix_screeninfo.smem_len);

	fcntl(rc, F_SETFL, O_NONBLOCK);

	ClearEntries   (LEFTFRAME );
	ClearEntries   (RIGHTFRAME);
	ClearMarker    (LEFTFRAME );
	ClearMarker    (RIGHTFRAME);
	ClearZipEntries(LEFTFRAME );
	ClearZipEntries(RIGHTFRAME);
	WriteSettings();
	free(szCommand);
	free(szZipCommand);
	return;
}

/******************************************************************************
 * RenderMenuLine                                                              *
 ******************************************************************************/

void RenderMenuLine(int highlight, int refresh)
{
	char szEntry[20];
	int i;
	RenderBox(menuitemwidth * MENUITEMS                 ,viewy-MENUSIZE, viewx, viewy-MENUSIZE / 2 , FILL, (highlight == MENUITEMS-1 ? GREEN : BLUE2) );
	for (i = 0; i < MENUITEMS; i++)
	{
		RenderBox(menuitemwidth * i                 ,viewy-MENUSIZE, menuitemwidth *(i+1)                 , viewy-MENUSIZE / 2 , FILL, (i == highlight ? GREEN : BLUE2) );
		RenderBox(menuitemwidth * i                 ,viewy-MENUSIZE, menuitemwidth * i   + menuitemnumber , viewy-MENUSIZE / 2 , FILL, BLUE1);

		sprintf(szEntry,"%d",(i+1)%MENUITEMS);
		RenderString(szEntry          , menuitemwidth * i +1              , viewy-(MENUSIZE/2 + FONT_OFFSET_BIG) , menuitemnumber              , CENTER, SMALL, WHITE);
		RenderString(menuline[tool[i]*NUM_LANG+language], menuitemwidth * i + menuitemnumber, viewy-(MENUSIZE/2 + FONT_OFFSET_BIG) , menuitemwidth-menuitemnumber, CENTER, SMALL, WHITE);


	}
	RenderBox( viewx-COLORBUTTONS ,viewy-MENUSIZE/2, viewx , viewy , FILL,  BLUE1);
	RenderBox( 0,viewy- MENUSIZE , viewx , viewy-MENUSIZE / 2 , GRID, WHITE);

	for (i = 0; i < COLORBUTTONS; i++)
	{

		RenderBox( (viewx/COLORBUTTONS) *i ,viewy-MENUSIZE/2, (viewx/COLORBUTTONS) *(i+1) , viewy , FILL,  (i == 0 ? RED    :
		                                                            					                   (i == 1 ? GREEN  :
		                                                            					                   (i == 2 ? YELLOW : BLUE1))));
		RenderBox( (viewx/COLORBUTTONS) *i ,viewy-MENUSIZE/2, (i < COLORBUTTONS-1 ? (viewx/COLORBUTTONS) *(i+1) : viewx) , viewy , GRID,  WHITE );
		RenderString(colorline[colortool[i]*NUM_LANG+language], (viewx/COLORBUTTONS) *i , viewy- FONT_OFFSET_BIG , viewx/COLORBUTTONS, CENTER, SMALL  , WHITE);
	}
	if (refresh == YES)
		memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);

}


/******************************************************************************
 * RenderFrame                                                                *
 ******************************************************************************/

void RenderFrame(int frame)
{

	int row = 0;
	int bcolor, fcolor;
	char sizeString[100];
	short bselected;
	struct fileentry* pfe;


	int nBackColor;

	colortool[0] = ACTION_EXEC   ;
	colortool[1] = ACTION_MARKER ;
	colortool[2] = ACTION_SORT   ;
	colortool[3] = ACTION_REFRESH;

	if (curvisibility == 0)
		nBackColor = (finfo[frame].writable ? BLUE1: BLACK );
	else
		nBackColor = trans_map[curvisibility];

	if (curframe == frame)
	{
		memset(tool, ACTION_NOACTION, sizeof(tool));
	}

	if (curframe == frame)
	{
		if (finfo[frame].writable)
		{
			tool[ACTION_MKDIR-1 ] = ACTION_MKDIR; // mkdir allowed
			tool[ACTION_MKFILE-1] = ACTION_MKFILE; // mkfile allowed
			if (finfo[1-frame].writable)
				tool[ACTION_MKLINK-1] = ACTION_MKLINK; // mklink allowed
		}
	}
	while (row < framerows && (finfo[frame].first + row < finfo[frame].count))
	{
		bselected =  ((finfo[frame].first + row == finfo[frame].selected) && (curframe == frame));
		bcolor = (bselected ? (IsMarked(frame,finfo[frame].first + row) ? BLUE3 : BLUE2)
		                    : (IsMarked(frame,finfo[frame].first + row) ? trans_map_mark[curvisibility] : nBackColor));
		pfe = getfileentry(frame, finfo[frame].first + row);
		if (bselected && strcmp(pfe->name,"..") != 0)
			tool[ACTION_PROPS-1] = ACTION_PROPS; // view properties allowed if entry is not ..
		*sizeString = 0x00;
		fcolor = WHITE;
		if ((pfe->fentry.st_mode & S_IRUSR) == S_IRUSR )
		{
			fcolor = GREEN2 ;
			if (bselected)
			{
				tool[ACTION_COPY-1] = (finfo[1-frame].writable  ? ACTION_COPY : ACTION_NOACTION); // copy allowed, if other frame writable;
				tool[ACTION_VIEW-1] = ACTION_VIEW; // view allowed
			}
		}
		if ((pfe->fentry.st_mode & S_IWUSR) == S_IWUSR )
		{
			fcolor = GRAY   ;
			if (bselected)
			{
				tool[ACTION_MOVE-1] = (finfo[1-frame].writable && finfo[frame].writable ? ACTION_MOVE   : ACTION_NOACTION); // move   allowed, if both frames writable;
				tool[ACTION_DELETE-1] = (finfo[  frame].writable ? ACTION_DELETE : ACTION_NOACTION); // delete allowed, if current frame writable
				tool[ACTION_RENAME-1] = (finfo[  frame].writable ? ACTION_RENAME : ACTION_NOACTION); // rename allowed, if current frame writable
				tool[ACTION_EDIT-1] = ((pfe->fentry.st_size < FILEBUFFER_SIZE) && finfo[frame].writable ?  ACTION_EDIT : ACTION_NOACTION); // edit allowed, if size of current file < FILEBUFFER_SIZE;
			}
		}
		if      ((pfe->fentry.st_mode & S_IXUSR) == S_IXUSR )
		{
			fcolor = YELLOW ;
		}
		if     (S_ISDIR(pfe->fentry.st_mode))
		{
			fcolor = WHITE  ;
			sprintf(sizeString,"<DIR>");
			if (bselected)
			{
				tool[ACTION_VIEW-1] = ACTION_NOACTION; // view not allowed
				tool[ACTION_EDIT-1] = ACTION_NOACTION; // edit not allowed
			}
		}
		else if (S_ISLNK(pfe->fentry.st_mode))
		{
			fcolor = ORANGE ;
			sprintf(sizeString,"<LINK>");
			if (bselected)
				tool[ACTION_VIEW-1] = ACTION_NOACTION; // view not allowed
		}
		else
		{
			GetSizeString(sizeString,pfe->fentry.st_size);
		}
		if (bselected)
		{
			if (finfo[frame].markcount > 0) // files marked in current frame
			{
				tool[ACTION_COPY  -1] = (finfo[1-frame].writable  ? ACTION_COPY : ACTION_NOACTION); // copy allowed, if other frame writable;
				tool[ACTION_MOVE  -1] = (finfo[1-frame].writable && finfo[frame].writable ? ACTION_MOVE   : ACTION_NOACTION); // move   allowed, if both frames writable;
				tool[ACTION_DELETE-1] = (finfo[  frame].writable ? ACTION_DELETE : ACTION_NOACTION); // delete allowed, if current frame writable
			}
			RenderMenuLine(-1, NO);
		}

		PosY = (row+1) * FONTHEIGHT_SMALL + BORDERSIZE ;
		PosX = frame * FrameWidth + BORDERSIZE;

		RenderBox(PosX, PosY-FONTHEIGHT_SMALL,((1+frame)*FrameWidth), PosY, FILL, bcolor);
		RenderString(pfe->name, PosX+2, PosY-FONT_OFFSET, NameWidth-2, LEFT, SMALL,fcolor);
		RenderString(sizeString, (1+frame)*FrameWidth -2*BORDERSIZE - 2*SizeWidth, PosY-FONT_OFFSET, 2*SizeWidth, RIGHT, SMALL,fcolor);
		row++;
	}
	// fill empty rows
	RenderBox(PosX, PosY,((1+frame)*FrameWidth), PosY+ FONTHEIGHT_SMALL*(framerows-row+1) , FILL, nBackColor);

	// draw Rectangle
	RenderBox(   frame *FrameWidth              , 0                            ,    frame *FrameWidth          +BORDERSIZE, viewy-MENUSIZE                              , FILL, (curframe == frame ? WHITE : BLUE2));
	RenderBox((1+frame)*FrameWidth -BORDERSIZE  , 0                            , (1+frame)*FrameWidth                     , viewy-MENUSIZE                              , FILL, (curframe == frame ? WHITE : BLUE2));
	RenderBox(   frame *FrameWidth              , 0                            , (1+frame)*FrameWidth                     , BORDERSIZE                                  , FILL, (curframe == frame ? WHITE : BLUE2));
	RenderBox(   frame *FrameWidth              , viewy-2*BORDERSIZE-FONTHEIGHT_SMALL - MENUSIZE, (1+frame)*FrameWidth    , viewy-BORDERSIZE-FONTHEIGHT_SMALL - MENUSIZE, FILL, (curframe == frame ? WHITE : BLUE2));
	RenderBox(   frame *FrameWidth +NameWidth   , 0                            ,    frame *FrameWidth+NameWidth+BORDERSIZE, viewy-BORDERSIZE-FONTHEIGHT_SMALL - MENUSIZE, FILL, (curframe == frame ? WHITE : BLUE2));
	RenderBox(   frame *FrameWidth              , viewy-BORDERSIZE - MENUSIZE  , (1+frame)*FrameWidth                     , viewy-MENUSIZE                              , FILL, (curframe == frame ? WHITE : BLUE2));

	// Info line
	RenderBox(PosX   , viewy-BORDERSIZE-FONTHEIGHT_SMALL-MENUSIZE, PosX+FrameWidth-2*BORDERSIZE, viewy-BORDERSIZE-MENUSIZE, FILL, BLACK);
	if (finfo[frame].markcount > 0)
	{
		sprintf(sizeString,info[INFO_MARKER*NUM_LANG+language],finfo[frame].markcount);
		RenderString(sizeString, PosX+2, viewy-BORDERSIZE-FONT_OFFSET-MENUSIZE , NameWidth-2, LEFT, SMALL,(curframe == frame ? WHITE : BLUE2));
		GetSizeString(sizeString,finfo[frame].marksize);
	}
	else
	{
		RenderString(finfo[frame].zipfile[0] != 0x00 ? finfo[frame].zipfile : finfo[frame].path, PosX+2, viewy-BORDERSIZE-FONT_OFFSET-MENUSIZE , NameWidth-2, LEFT, SMALL,(curframe == frame ? WHITE : BLUE2));
		GetSizeString(sizeString,finfo[frame].size);
	}
	RenderString(sizeString, (1+frame)*FrameWidth -BORDERSIZE - 2*SizeWidth, viewy-BORDERSIZE-FONT_OFFSET-MENUSIZE , 2*SizeWidth, RIGHT, SMALL,(curframe == frame ? WHITE : BLUE2));
}

/******************************************************************************
 * MessageBox                                                                 *
 ******************************************************************************/

int MessageBox(char* msg1, char* msg2, int mode)
{

	int sel = 0, le1=0, le2=0 , wi, he, maxsel=0;
	int ps[5];

	switch (mode)
	{
		case OKCANCEL:
			ps[0] = YES;
			ps[1] = CANCEL;
			ps[2] = 0;
			ps[3] = 0;
			ps[4] = 0;
			sel = 1;
			maxsel = 1;
			break;
		case OKHIDDENCANCEL:
			ps[0] = YES;
			ps[1] = CANCEL;
			ps[2] = HIDDEN;
			ps[3] = 0;
			ps[4] = 0;
			sel = 1;
			maxsel = 2;
			break;
		case YESNOCANCEL:
			ps[0] = YES;
			ps[1] = CANCEL;
			ps[2] = NO;
			ps[3] = 0;
			ps[4] = 0;
			sel = 1;
			maxsel = 2;
			break;
		case OVERWRITECANCEL:
			ps[0] = OVERWRITE;
			ps[1] = CANCEL;
			ps[2] = 0;
			ps[3] = 0;
			ps[4] = 0;
			sel = 1;
			maxsel = 1;
			break;
		case OVERWRITESKIPCANCEL:
			ps[0] = OVERWRITE;
			ps[1] = CANCEL;
			ps[2] = SKIP;
			ps[3] = OVERWRITEALL;
			ps[4] = SKIPALL;
			sel = 1;
			maxsel = 4;
			break;
	}

	le1 = GetStringLen(msg1, BIG);
	le2 = GetStringLen(msg2, BIG);
	wi = MINBOX;
	if (le1 > wi ) wi = le1;
	if (le2 > wi ) wi = le2;
	if (wi > viewx - 6* BORDERSIZE) wi = viewx - 6* BORDERSIZE;

	he = 4* BORDERSIZE+ BUTTONHEIGHT + (*msg2 == 0x00 ?  1 : 2) * FONTHEIGHT_BIG + (maxsel > 2 ? BORDERSIZE+BUTTONHEIGHT : 0);


	RenderBox((viewx-wi)/2 - 2*BORDERSIZE, (viewy-he) /2, viewx-(viewx-wi)/2+ 2*BORDERSIZE, viewy-(viewy-he)/2, FILL, trans_map[curvisibility]);
	RenderBox((viewx-wi)/2 - 2*BORDERSIZE, (viewy-he) /2, viewx-(viewx-wi)/2+ 2*BORDERSIZE, viewy-(viewy-he)/2, GRID, WHITE);
	RenderString(msg1,(viewx-wi)/2-BORDERSIZE , (viewy-he)/2 + BORDERSIZE + FONTHEIGHT_BIG-FONT_OFFSET , wi+2*BORDERSIZE, CENTER, BIG, WHITE);
	if (le2 > 0)
		RenderString(msg2,(viewx-wi)/2-BORDERSIZE , (viewy-he)/2 + BORDERSIZE + 2*FONTHEIGHT_BIG-FONT_OFFSET , wi+2*BORDERSIZE, CENTER, BIG, WHITE);


	RenderButtons(he, mode);
	if (mode == NOBUTTON) return 0;
	int drawsel = 0;
	do{
		GetRCCode(NOSCROLL_LEFTRIGHT);
		switch(rccode)
		{
				case RC_OK:
					rccode = -1;
					return ps[sel];
				case RC_LEFT:
					sel--;
					if (sel < 0 ) sel = 0;
					drawsel = 1;
					break;
				case RC_RIGHT:
					sel++;
					if (sel > maxsel) sel = maxsel;
					drawsel = 1;
					break;
				case RC_UP:
				    if (sel > 2) sel = 1;
					drawsel = 1;
					break;
				case RC_DOWN:
				    if (sel < 3) sel = 3;
					drawsel = 1;
					break;
				case RC_RED:
					rccode = -1;
					return ps[0];
				case RC_GREEN:
				case RC_HOME:
					rccode = -1;
					return ps[1];
				case RC_YELLOW:
					rccode = -1;
					if (maxsel > 1)
						return ps[2];
				default:
					continue;
		}
		if (drawsel)
		{
			switch(maxsel)
			{
				case 1:
					RenderBox(viewx/2 - 2* BORDERSIZE -BUTTONWIDTH  , viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT  , viewx/2 - 2* BORDERSIZE                ,viewy-(viewy-he)/2- 2* BORDERSIZE  , GRID, (sel == 0 ? WHITE : RED  ));
					RenderBox(viewx/2 - 2* BORDERSIZE -BUTTONWIDTH+1, viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT+1, viewx/2 - 2* BORDERSIZE              -1,viewy-(viewy-he)/2- 2* BORDERSIZE-1, GRID, (sel == 0 ? WHITE : RED  ));

					RenderBox(viewx/2 + 2* BORDERSIZE               , viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT  , viewx/2 + 2* BORDERSIZE +BUTTONWIDTH   ,viewy-(viewy-he)/2- 2* BORDERSIZE  , GRID, (sel == 1 ? WHITE : GREEN));
					RenderBox(viewx/2 + 2* BORDERSIZE             +1, viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT+1, viewx/2 + 2* BORDERSIZE +BUTTONWIDTH -1,viewy-(viewy-he)/2- 2* BORDERSIZE-1, GRID, (sel == 1 ? WHITE : GREEN));
					memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
					break;
				case 2:
					RenderBox(viewx/2 - 4* BORDERSIZE - BUTTONWIDTH - BUTTONWIDTH/2  , viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT  , viewx/2 - 4* BORDERSIZE  - BUTTONWIDTH/2              ,viewy-(viewy-he)/2- 2* BORDERSIZE  , GRID, (sel == 0 ? WHITE : RED  ));
					RenderBox(viewx/2 - 4* BORDERSIZE - BUTTONWIDTH - BUTTONWIDTH/2+1, viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT+1, viewx/2 - 4* BORDERSIZE  - BUTTONWIDTH/2            -1,viewy-(viewy-he)/2- 2* BORDERSIZE-1, GRID, (sel == 0 ? WHITE : RED  ));

					RenderBox(viewx/2 - BUTTONWIDTH/2                                , viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT  , viewx/2 + BUTTONWIDTH/2                               ,viewy-(viewy-he)/2- 2* BORDERSIZE  , GRID, (sel == 1 ? WHITE : GREEN ));
					RenderBox(viewx/2 - BUTTONWIDTH/2                              +1, viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT+1, viewx/2 + BUTTONWIDTH/2                             -1,viewy-(viewy-he)/2- 2* BORDERSIZE-1, GRID, (sel == 1 ? WHITE : GREEN ));

					RenderBox(viewx/2 + 4* BORDERSIZE + BUTTONWIDTH/2                , viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT  , viewx/2 + 4* BORDERSIZE +BUTTONWIDTH + BUTTONWIDTH/2  ,viewy-(viewy-he)/2- 2* BORDERSIZE  , GRID, (sel == 2 ? WHITE : YELLOW ));
					RenderBox(viewx/2 + 4* BORDERSIZE + BUTTONWIDTH/2              +1, viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT+1, viewx/2 + 4* BORDERSIZE +BUTTONWIDTH + BUTTONWIDTH/2-1,viewy-(viewy-he)/2- 2* BORDERSIZE-1, GRID, (sel == 2 ? WHITE : YELLOW ));
					memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
					break;
				case 4:
					RenderBox(viewx/2 - 4* BORDERSIZE - BUTTONWIDTH - BUTTONWIDTH/2  , viewy-(viewy-he)/2 - 4*BORDERSIZE - 2*BUTTONHEIGHT  , viewx/2 - 4* BORDERSIZE  - BUTTONWIDTH/2              ,viewy-(viewy-he)/2- 4* BORDERSIZE - BUTTONHEIGHT  , GRID, (sel == 0 ? WHITE : RED    ));
					RenderBox(viewx/2 - 4* BORDERSIZE - BUTTONWIDTH - BUTTONWIDTH/2+1, viewy-(viewy-he)/2 - 4*BORDERSIZE - 2*BUTTONHEIGHT+1, viewx/2 - 4* BORDERSIZE  - BUTTONWIDTH/2            -1,viewy-(viewy-he)/2- 4* BORDERSIZE - BUTTONHEIGHT-1, GRID, (sel == 0 ? WHITE : RED    ));

					RenderBox(viewx/2 - BUTTONWIDTH/2                                , viewy-(viewy-he)/2 - 4*BORDERSIZE - 2*BUTTONHEIGHT  , viewx/2 + BUTTONWIDTH/2                               ,viewy-(viewy-he)/2- 4* BORDERSIZE - BUTTONHEIGHT  , GRID, (sel == 1 ? WHITE : GREEN  ));
					RenderBox(viewx/2 - BUTTONWIDTH/2                              +1, viewy-(viewy-he)/2 - 4*BORDERSIZE - 2*BUTTONHEIGHT+1, viewx/2 + BUTTONWIDTH/2                             -1,viewy-(viewy-he)/2- 4* BORDERSIZE - BUTTONHEIGHT-1, GRID, (sel == 1 ? WHITE : GREEN  ));

 					RenderBox(viewx/2 + 4* BORDERSIZE + BUTTONWIDTH/2                , viewy-(viewy-he)/2 - 4*BORDERSIZE - 2*BUTTONHEIGHT  , viewx/2 + 4* BORDERSIZE +BUTTONWIDTH + BUTTONWIDTH/2  ,viewy-(viewy-he)/2- 4* BORDERSIZE - BUTTONHEIGHT  , GRID, (sel == 2 ? WHITE : YELLOW ));
 					RenderBox(viewx/2 + 4* BORDERSIZE + BUTTONWIDTH/2              +1, viewy-(viewy-he)/2 - 4*BORDERSIZE - 2*BUTTONHEIGHT+1, viewx/2 + 4* BORDERSIZE +BUTTONWIDTH + BUTTONWIDTH/2-1,viewy-(viewy-he)/2- 4* BORDERSIZE - BUTTONHEIGHT-1, GRID, (sel == 2 ? WHITE : YELLOW ));

					RenderBox(viewx/2 - 4* BORDERSIZE - BUTTONWIDTH - BUTTONWIDTH/2  , viewy-(viewy-he)/2 - 2*BORDERSIZE -   BUTTONHEIGHT  , viewx/2 - 2* BORDERSIZE                               ,viewy-(viewy-he)/2- 2* BORDERSIZE                 , GRID, (sel == 3 ? WHITE : BLUE2  ));
					RenderBox(viewx/2 - 4* BORDERSIZE - BUTTONWIDTH - BUTTONWIDTH/2+1, viewy-(viewy-he)/2 - 2*BORDERSIZE -   BUTTONHEIGHT+1, viewx/2 - 2* BORDERSIZE                             -1,viewy-(viewy-he)/2- 2* BORDERSIZE               -1, GRID, (sel == 3 ? WHITE : BLUE2  ));

					RenderBox(viewx/2 + 2* BORDERSIZE                                , viewy-(viewy-he)/2 - 2*BORDERSIZE -   BUTTONHEIGHT  , viewx/2 + 4* BORDERSIZE +BUTTONWIDTH + BUTTONWIDTH/2  ,viewy-(viewy-he)/2- 2* BORDERSIZE                 , GRID, (sel == 4 ? WHITE : BLUE2  ));
					RenderBox(viewx/2 + 2* BORDERSIZE                              +1, viewy-(viewy-he)/2 - 2*BORDERSIZE -   BUTTONHEIGHT+1, viewx/2 + 4* BORDERSIZE +BUTTONWIDTH + BUTTONWIDTH/2-1,viewy-(viewy-he)/2- 2* BORDERSIZE               -1, GRID, (sel == 4 ? WHITE : BLUE2  ));
					memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
					break;
			}
			drawsel = 0;
		}

	}while(1);
	rccode = -1;
	return sel;

}
/******************************************************************************
 * RenderButtons                                                              *
 ******************************************************************************/

void RenderButtons(int he, int mode)
{
	switch(mode)
	{
		case OKCANCEL:
			RenderBox(viewx/2 - 2* BORDERSIZE -BUTTONWIDTH , viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT, viewx/2 - 2* BORDERSIZE              ,viewy-(viewy-he)/2- 2* BORDERSIZE, FILL, RED  );
			RenderBox(viewx/2 + 2* BORDERSIZE              , viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT, viewx/2 + 2* BORDERSIZE +BUTTONWIDTH ,viewy-(viewy-he)/2- 2* BORDERSIZE, FILL, GREEN);
			RenderString(mbox[BTN_OK    *NUM_LANG+language],viewx/2 - 2* BORDERSIZE -BUTTONWIDTH , viewy-(viewy-he)/2 - 2*BORDERSIZE-FONT_OFFSET , BUTTONWIDTH, CENTER, BIG, WHITE);
			RenderString(mbox[BTN_CANCEL*NUM_LANG+language],viewx/2 + 2* BORDERSIZE              , viewy-(viewy-he)/2 - 2*BORDERSIZE-FONT_OFFSET , BUTTONWIDTH, CENTER, BIG, WHITE);
			RenderBox(viewx/2 + 2* BORDERSIZE                , viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT  , viewx/2 + 2* BORDERSIZE +BUTTONWIDTH   ,viewy-(viewy-he)/2- 2* BORDERSIZE  , GRID, WHITE);
			RenderBox(viewx/2 + 2* BORDERSIZE              +1, viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT+1, viewx/2 + 2* BORDERSIZE +BUTTONWIDTH -1,viewy-(viewy-he)/2- 2* BORDERSIZE-1, GRID, WHITE);
			break;
		case OKHIDDENCANCEL:
			RenderBox(viewx/2 - 4* BORDERSIZE - BUTTONWIDTH - BUTTONWIDTH/2, viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT, viewx/2 - 4* BORDERSIZE - BUTTONWIDTH/2             ,viewy-(viewy-he)/2- 2* BORDERSIZE, FILL, RED   );
			RenderBox(viewx/2 - BUTTONWIDTH/2                              , viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT, viewx/2 + BUTTONWIDTH/2                             ,viewy-(viewy-he)/2- 2* BORDERSIZE, FILL, GREEN );
			RenderBox(viewx/2 + 4* BORDERSIZE + BUTTONWIDTH/2              , viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT, viewx/2 + 4* BORDERSIZE +BUTTONWIDTH + BUTTONWIDTH/2,viewy-(viewy-he)/2- 2* BORDERSIZE, FILL, YELLOW);
			RenderString(mbox[BTN_OK    *NUM_LANG+language],viewx/2 - 4* BORDERSIZE -BUTTONWIDTH - BUTTONWIDTH/2 , viewy-(viewy-he)/2 - 2*BORDERSIZE-FONT_OFFSET , BUTTONWIDTH, CENTER, BIG, WHITE);
			RenderString(mbox[BTN_CANCEL*NUM_LANG+language],(viewx-BUTTONWIDTH)/2  , viewy-(viewy-he)/2 - 2*BORDERSIZE-FONT_OFFSET , BUTTONWIDTH, CENTER, BIG, WHITE);
			RenderString(mbox[BTN_HIDDEN*NUM_LANG+language],viewx/2 + 4* BORDERSIZE +BUTTONWIDTH/2            , viewy-(viewy-he)/2 - 2*BORDERSIZE-FONT_OFFSET , BUTTONWIDTH, CENTER, BIG, WHITE);
			RenderBox(viewx/2 -BUTTONWIDTH/2  , viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT  , viewx/2 + BUTTONWIDTH/2   ,viewy-(viewy-he)/2- 2* BORDERSIZE  , GRID, WHITE);
			RenderBox(viewx/2 -BUTTONWIDTH/2+1, viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT+1, viewx/2 + BUTTONWIDTH/2 -1,viewy-(viewy-he)/2- 2* BORDERSIZE-1, GRID, WHITE);
			break;
		case YESNOCANCEL:
			RenderBox(viewx/2 - 4* BORDERSIZE - BUTTONWIDTH - BUTTONWIDTH/2, viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT, viewx/2 - 4* BORDERSIZE - BUTTONWIDTH/2             ,viewy-(viewy-he)/2- 2* BORDERSIZE, FILL, RED   );
			RenderBox(viewx/2 - BUTTONWIDTH/2                              , viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT, viewx/2 + BUTTONWIDTH/2                             ,viewy-(viewy-he)/2- 2* BORDERSIZE, FILL, GREEN );
			RenderBox(viewx/2 + 4* BORDERSIZE + BUTTONWIDTH/2              , viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT, viewx/2 + 4* BORDERSIZE +BUTTONWIDTH + BUTTONWIDTH/2,viewy-(viewy-he)/2- 2* BORDERSIZE, FILL, YELLOW);
			RenderString(mbox[BTN_YES   *NUM_LANG+language],viewx/2 - 4* BORDERSIZE -BUTTONWIDTH - BUTTONWIDTH/2 , viewy-(viewy-he)/2 - 2*BORDERSIZE-FONT_OFFSET , BUTTONWIDTH, CENTER, BIG, WHITE);
			RenderString(mbox[BTN_CANCEL*NUM_LANG+language],(viewx-BUTTONWIDTH)/2  , viewy-(viewy-he)/2 - 2*BORDERSIZE-FONT_OFFSET , BUTTONWIDTH, CENTER, BIG, WHITE);
			RenderString(mbox[BTN_NO    *NUM_LANG+language],viewx/2 + 4* BORDERSIZE +BUTTONWIDTH/2            , viewy-(viewy-he)/2 - 2*BORDERSIZE-FONT_OFFSET , BUTTONWIDTH, CENTER, BIG, WHITE);
			RenderBox(viewx/2 -BUTTONWIDTH/2  , viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT  , viewx/2 + BUTTONWIDTH/2   ,viewy-(viewy-he)/2- 2* BORDERSIZE  , GRID, WHITE);
			RenderBox(viewx/2 -BUTTONWIDTH/2+1, viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT+1, viewx/2 + BUTTONWIDTH/2 -1,viewy-(viewy-he)/2- 2* BORDERSIZE-1, GRID, WHITE);
			break;
		case OVERWRITECANCEL:
			RenderBox(viewx/2 - 2* BORDERSIZE -BUTTONWIDTH , viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT, viewx/2 - 2* BORDERSIZE              ,viewy-(viewy-he)/2- 2* BORDERSIZE, FILL, RED  );
			RenderBox(viewx/2 + 2* BORDERSIZE              , viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT, viewx/2 + 2* BORDERSIZE +BUTTONWIDTH ,viewy-(viewy-he)/2- 2* BORDERSIZE, FILL, GREEN);
			RenderString(mbox[BTN_OVERWRITE*NUM_LANG+language],viewx/2 - 2* BORDERSIZE -BUTTONWIDTH , viewy-(viewy-he)/2 - 2*BORDERSIZE-FONT_OFFSET , BUTTONWIDTH, CENTER, BIG, WHITE);
			RenderString(mbox[BTN_CANCEL   *NUM_LANG+language],viewx/2 + 2* BORDERSIZE              , viewy-(viewy-he)/2 - 2*BORDERSIZE-FONT_OFFSET , BUTTONWIDTH, CENTER, BIG, WHITE);
			RenderBox(viewx/2 + 2* BORDERSIZE                , viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT  , viewx/2 + 2* BORDERSIZE +BUTTONWIDTH   ,viewy-(viewy-he)/2- 2* BORDERSIZE  , GRID, WHITE);
			RenderBox(viewx/2 + 2* BORDERSIZE              +1, viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT+1, viewx/2 + 2* BORDERSIZE +BUTTONWIDTH -1,viewy-(viewy-he)/2- 2* BORDERSIZE-1, GRID, WHITE);
			break;
		case OVERWRITESKIPCANCEL:
			RenderBox(viewx/2 - 4* BORDERSIZE - BUTTONWIDTH - BUTTONWIDTH/2, viewy-(viewy-he)/2 - 4*BORDERSIZE - 2*BUTTONHEIGHT, viewx/2 - 4* BORDERSIZE  - BUTTONWIDTH/2            ,viewy-(viewy-he)/2- 4* BORDERSIZE - BUTTONHEIGHT, FILL, RED    );
			RenderBox(viewx/2 - BUTTONWIDTH/2                              , viewy-(viewy-he)/2 - 4*BORDERSIZE - 2*BUTTONHEIGHT, viewx/2 + BUTTONWIDTH/2                             ,viewy-(viewy-he)/2- 4* BORDERSIZE - BUTTONHEIGHT, FILL, GREEN  );
			RenderBox(viewx/2 + 4* BORDERSIZE + BUTTONWIDTH/2              , viewy-(viewy-he)/2 - 4*BORDERSIZE - 2*BUTTONHEIGHT, viewx/2 + 4* BORDERSIZE +BUTTONWIDTH + BUTTONWIDTH/2,viewy-(viewy-he)/2- 4* BORDERSIZE - BUTTONHEIGHT, FILL, YELLOW );
			RenderBox(viewx/2 - 4* BORDERSIZE - BUTTONWIDTH - BUTTONWIDTH/2, viewy-(viewy-he)/2 - 2*BORDERSIZE -   BUTTONHEIGHT, viewx/2 - 2* BORDERSIZE                             ,viewy-(viewy-he)/2- 2* BORDERSIZE               , FILL, BLUE2  );
			RenderBox(viewx/2 + 2* BORDERSIZE                              , viewy-(viewy-he)/2 - 2*BORDERSIZE -   BUTTONHEIGHT, viewx/2 + 4* BORDERSIZE +BUTTONWIDTH + BUTTONWIDTH/2,viewy-(viewy-he)/2- 2* BORDERSIZE               , FILL, BLUE2  );
			RenderString(mbox[BTN_OVERWRITE   *NUM_LANG+language],viewx/2 - 4* BORDERSIZE - BUTTONWIDTH - BUTTONWIDTH/2, viewy-(viewy-he)/2 - 4*BORDERSIZE-FONT_OFFSET-BUTTONHEIGHT , BUTTONWIDTH, CENTER, BIG, WHITE);
			RenderString(mbox[BTN_CANCEL      *NUM_LANG+language],(viewx-BUTTONWIDTH)/2                                , viewy-(viewy-he)/2 - 4*BORDERSIZE-FONT_OFFSET-BUTTONHEIGHT , BUTTONWIDTH, CENTER, BIG, WHITE);
			RenderString(mbox[BTN_SKIP        *NUM_LANG+language],viewx/2 + 4* BORDERSIZE +BUTTONWIDTH/2               , viewy-(viewy-he)/2 - 4*BORDERSIZE-FONT_OFFSET-BUTTONHEIGHT , BUTTONWIDTH, CENTER, BIG, WHITE);
			RenderString(mbox[BTN_OVERWRITEALL*NUM_LANG+language],viewx/2 - 4* BORDERSIZE - BUTTONWIDTH - BUTTONWIDTH/2, viewy-(viewy-he)/2 - 2*BORDERSIZE-FONT_OFFSET              , BUTTONWIDTH + BUTTONWIDTH/2 + 2* BORDERSIZE, CENTER, BIG, WHITE);
			RenderString(mbox[BTN_SKIPALL     *NUM_LANG+language],viewx/2 + 2* BORDERSIZE                              , viewy-(viewy-he)/2 - 2*BORDERSIZE-FONT_OFFSET              , BUTTONWIDTH + BUTTONWIDTH/2 + 2* BORDERSIZE, CENTER, BIG, WHITE);
			RenderBox(viewx/2 -BUTTONWIDTH/2  , viewy-(viewy-he)/2 - 4*BORDERSIZE - 2*BUTTONHEIGHT  , viewx/2 + BUTTONWIDTH/2  ,viewy-(viewy-he)/2- 4* BORDERSIZE - BUTTONHEIGHT  , GRID, WHITE);
			RenderBox(viewx/2 -BUTTONWIDTH/2+1, viewy-(viewy-he)/2 - 4*BORDERSIZE - 2*BUTTONHEIGHT+1, viewx/2 + BUTTONWIDTH/2-1,viewy-(viewy-he)/2- 4* BORDERSIZE - BUTTONHEIGHT-1, GRID, WHITE);
			break;
		case NOBUTTON:
		    break;
		default:
			RenderBox((viewx-BUTTONWIDTH)/2 , viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT, viewx - (viewx-BUTTONWIDTH)/2,viewy-(viewy-he)/2 - 2*BORDERSIZE , FILL, RED  );
			RenderString(mbox[BTN_OK*NUM_LANG+language],(viewx-BUTTONWIDTH)/2  , viewy-(viewy-he)/2 - 2*BORDERSIZE-FONT_OFFSET , BUTTONWIDTH, CENTER, BIG, WHITE);
			RenderBox((viewx-BUTTONWIDTH)/2 , viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT, viewx - (viewx-BUTTONWIDTH)/2,viewy-(viewy-he)/2 - 2*BORDERSIZE , GRID, WHITE);
			break;
	}
	memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
}

/******************************************************************************
 * ShowProperties                                                             *
 ******************************************************************************/

int ShowProperties()
{
	struct fileentry *pfe = GetSelected(curframe);

	int sel = NO, pos = -1, mode, i, le1, wi , he = 6 * BORDERSIZE + BUTTONHEIGHT + 4 * FONTHEIGHT_BIG;
	int ri[3];
	char action[FILENAME_MAX];

	ri[0] =  ((pfe->fentry.st_mode & S_IRUSR) == S_IRUSR ? 1 : 0);
	ri[1] =  ((pfe->fentry.st_mode & S_IWUSR) == S_IWUSR ? 1 : 0);
	ri[2] =  ((pfe->fentry.st_mode & S_IXUSR) == S_IXUSR ? 1 : 0);

	le1 = GetStringLen(pfe->name, BIG);
	wi = 300;
	if (le1 + 4*BORDERSIZE > wi  ) wi = le1 + 4*BORDERSIZE;
	if (wi > viewx - 4* BORDERSIZE) wi = viewx - 4* BORDERSIZE;

	mode  =  (finfo[curframe].writable ? OKCANCEL : OK);
	RenderBox((viewx-wi)/2 , (viewy-he) /2, viewx-(viewx-wi)/2, viewy-(viewy-he)/2, FILL, trans_map[curvisibility]);
	RenderBox((viewx-wi)/2 , (viewy-he) /2, viewx-(viewx-wi)/2, viewy-(viewy-he)/2, GRID, WHITE);
	RenderString(pfe->name,(viewx-wi)/2+  2* BORDERSIZE , (viewy-he)/2 + 2*BORDERSIZE + FONTHEIGHT_BIG-FONT_OFFSET , wi, CENTER, BIG, WHITE);

	for (i = 0; i < 3 ; i++)
	{
		RenderString(props[i*NUM_LANG+language],(viewx-wi)/2+ 3* BORDERSIZE , (viewy-he)/2 + 3*BORDERSIZE + (i+2)*FONTHEIGHT_BIG-FONT_OFFSET , wi, LEFT, BIG, WHITE);
		RenderBox(viewx-(viewx-wi)/2 - 2* BORDERSIZE - FONTHEIGHT_BIG, (viewy-he)/2 + 3*BORDERSIZE + (i+1)*FONTHEIGHT_BIG, viewx-(viewx-wi)/2 - 2*BORDERSIZE, (viewy-he)/2 + 3*BORDERSIZE + (i+2)*FONTHEIGHT_BIG, FILL, (ri[i] == 0 ? RED : GREEN));
		RenderBox(      (viewx-wi)/2 + 2* BORDERSIZE                 , (viewy-he)/2 + 3*BORDERSIZE + (i+1)*FONTHEIGHT_BIG, viewx-(viewx-wi)/2 - 2*BORDERSIZE, (viewy-he)/2 + 3*BORDERSIZE + (i+2)*FONTHEIGHT_BIG, GRID, (pos == i ? WHITE :trans_map[curvisibility]));
	}
	RenderButtons(he,mode);
	int drawsel = 0;
	do{
		GetRCCode(SCROLL_NORMAL);
		switch(rccode)
		{
				case RC_OK:
					if (sel == NO) return NO;
					if (sel == YES)
					{
						if(!finfo[curframe].writable) return NO;
						int m = (ri[0] << 2) | (ri[1] << 1) | ri[2];
						sprintf(action,"chmod -R %d%d%d \"%s%s\"",m,m,m, finfo[curframe].path, pfe->name);
						DoExecute(action, SHOW_NO_OUTPUT);
						rccode = -1;
						return YES;
					}
					if (pos != -1)
					{
						ri[pos] = 1- ri[pos];
						drawsel = 1;
					}
					break;
				case RC_LEFT:
					sel = YES;
					drawsel = 1;
					break;
				case RC_RIGHT:
					sel = NO;
					drawsel = 1;
					break;
				case RC_UP:
					if (mode == OKCANCEL)
					{
						if (sel != -1)
						{
							pos = 2;
						}
						else
						{
							if (pos > 0) pos--;
						}
						sel = -1;
						drawsel = 1;
					}
					break;
				case RC_DOWN:
					if (mode == OKCANCEL)
					{
						pos++;
						if (pos == 3)
							sel = YES;
						if (pos >= 4)
						{
							pos = 4;
							sel = NO;
						}
						drawsel = 1;

					}
					break;
				case RC_RED:
					rccode = -1;
					return YES;
				case RC_GREEN:
				case RC_HOME:
					rccode = -1;
					return NO;
				default:
					continue;
		}
		if (drawsel)
		{
			for (i = 0; i < 3 ; i++)
			{
				RenderBox(viewx-(viewx-wi)/2 - 2* BORDERSIZE - FONTHEIGHT_BIG, (viewy-he)/2 + 3*BORDERSIZE + (i+1)*FONTHEIGHT_BIG, viewx-(viewx-wi)/2 - 2*BORDERSIZE, (viewy-he)/2 + 3*BORDERSIZE + (i+2)*FONTHEIGHT_BIG, FILL, (ri[i] == 0 ? RED : GREEN));
				RenderBox(      (viewx-wi)/2 + 2* BORDERSIZE                 , (viewy-he)/2 + 3*BORDERSIZE + (i+1)*FONTHEIGHT_BIG, viewx-(viewx-wi)/2 - 2*BORDERSIZE, (viewy-he)/2 + 3*BORDERSIZE + (i+2)*FONTHEIGHT_BIG, GRID, (pos == i ? WHITE :trans_map[curvisibility]));
			}
			RenderBox(viewx/2 - 2* BORDERSIZE -BUTTONWIDTH , viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT, viewx/2 - 2* BORDERSIZE              ,viewy-(viewy-he)/2- 2* BORDERSIZE, GRID, (sel == YES ? WHITE : RED  ));
			RenderBox(viewx/2 + 2* BORDERSIZE              , viewy-(viewy-he)/2 - 2*BORDERSIZE - BUTTONHEIGHT, viewx/2 + 2* BORDERSIZE +BUTTONWIDTH ,viewy-(viewy-he)/2- 2* BORDERSIZE, GRID, (sel == NO ? WHITE : GREEN));
			memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
			drawsel = 0;
		}

	}while(1);
	rccode = -1;
	return sel;

}
/******************************************************************************
 * GetInputString                                                             *
 ******************************************************************************/

int GetInputString(int width,int maxchars, char* str, char * msg)
{


	int le1, wi, he, x , y;



	le1 = GetStringLen(msg, BIG);
	wi = MINBOX;
	if (width > viewx - 8* BORDERSIZE) width = viewx - 8* BORDERSIZE;
	if (le1   > wi ) wi = le1 + 6*BORDERSIZE;
	if (width > wi ) wi = width + 6*BORDERSIZE;
	if (wi > viewx - 6* BORDERSIZE) wi = viewx - 6* BORDERSIZE;

	he = 6* BORDERSIZE+ 2*FONTHEIGHT_BIG;


	RenderBox((viewx-wi)/2 , (viewy-he) /2, viewx-(viewx-wi)/2, viewy-(viewy-he)/2, FILL, trans_map[curvisibility]);
	RenderBox((viewx-wi)/2 , (viewy-he) /2, viewx-(viewx-wi)/2, viewy-(viewy-he)/2, GRID, WHITE);
	RenderString(msg,(viewx-wi)/2-BORDERSIZE , (viewy-he)/2 + BORDERSIZE + FONTHEIGHT_BIG-FONT_OFFSET , wi+2*BORDERSIZE, CENTER, BIG, WHITE);

	x = (viewx-width)/2 - BORDERSIZE;
	y = (viewy-he)/2+ 2*BORDERSIZE + FONTHEIGHT_BIG;

	RenderBox(x,y, x+width +2*BORDERSIZE, y+FONTHEIGHT_BIG+2*BORDERSIZE, FILL, trans_map[curvisibility]);
	RenderBox(x,y, x+width              , y+FONTHEIGHT_BIG+2*BORDERSIZE, GRID, WHITE);

	return DoEditString(x+BORDERSIZE,y+BORDERSIZE,width-2*BORDERSIZE,maxchars,str, BIG,BLUE1);
}
/******************************************************************************
 * doEditString                                                               *
 ******************************************************************************/


int DoEditString(int x, int y, int width, int maxchars, char* str, int vsize, int back)
{

	int pos = 0, start = 0, slen, he = (vsize==BIG ? FONTHEIGHT_BIG : FONTHEIGHT_SMALL);
	char szbuf[maxchars+1];
	char szdst[maxchars+1];
	char * pch;

	memset(szdst, 0,maxchars+1);
	strcpy(szdst,str);
	szdst[strlen(szdst)] = ' ';
	szdst[strlen(szdst)+1] = 0x00;
	strcpy(szbuf,str);
	szbuf[1] = 0x00;

	RenderBox(x-1,y, x+width+1, y+he, FILL, back);
	RenderBox(x-1,y, x+GetStringLen(szbuf, vsize)+1, y+he, FILL, RED);
	RenderString(szdst,x, y+he-FONT_OFFSET, width, LEFT, vsize, WHITE);

	memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);

	do{
		GetRCCode(SCROLL_NORMAL);
		switch(rccode)
		{
				case RC_OK:
					strncpy(str,szdst,maxchars);
					while (str[strlen(str)-1] == ' ')
					{
						str[strlen(str)-1] = 0x00;
					}
					rccode = -1;
					return RC_OK;
				case RC_LEFT:
					pos--;
					break;
				case RC_RIGHT:
					pos++;
					break;
				case RC_PLUS:
					if (szdst[pos] != 0x00)
					{
						strcpy(szbuf,(char*)(szdst+pos));
						szdst[pos] = ' ';
						strcpy((char*)(szdst+pos+1),szbuf);
					}
					break;
				case RC_MINUS:
					if (szdst[pos] != 0x00)
					{
						strcpy(szbuf,(char*)(szdst+pos+1));
						strcpy((char*)(szdst+pos),szbuf);
					}
					break;
				case RC_DOWN:
					pch = strchr(charset,szdst[pos]);
					if (pch == NULL) szdst[pos] = ' ';
					else
					{
						if (pch == charset) szdst[pos] = charset[strlen(charset)-1];
						else szdst[pos] = *((char*)pch-1);
					}
					break;
				case RC_UP:
					pch = strchr(charset,szdst[pos]);
					if (pch == NULL) szdst[pos] = ' ';
					else
					{
						if (pch == &(charset[strlen(charset)-1])) szdst[pos] = charset[0];
						else szdst[pos] = *((char*)pch+1);
					}
					break;
				case RC_RED:
					szdst[0] = 0x00;
					break;
				case RC_HOME:
					rccode = -1;
					return RC_HOME;
				default:
					continue;
		}
		if (pos <  0            ) pos = 0;
		if (pos >= strlen(szdst))
		{
			if (pos > maxchars) pos = maxchars;
			else
				strcat(szdst," ");
		}
		if (start > pos) start = pos;
		while (1)
		{
			strcpy(szbuf,(char*)(szdst+start));
			szbuf[pos+1-start] = 0x00;
			slen = GetStringLen(szbuf, vsize);
		 	if (slen >= width - 2*BORDERSIZE)
				start++;
			else
				break;
		}
		strcpy(szbuf,(char*)(szdst+start));
		szbuf[pos+1-start] = 0x00;
		slen = GetStringLen(szbuf, vsize);
		szbuf[pos-start] = 0x00;
		int slen2=GetStringLen(szbuf, vsize);

		RenderBox(x-1,y, x+width+1, y+he, FILL, back);
		RenderBox(x+slen2-1,y, x+slen+1, y+he, FILL, RED);
		RenderString((char*)(szdst+start),x, y+he-FONT_OFFSET, width, LEFT, vsize, WHITE);
		memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
	}while(1);

	rccode = -1;
	return rccode;
}

/******************************************************************************
 * flistcmp                                                                   *
 ******************************************************************************/

int flistcmp(struct fileentry * p1, struct fileentry * p2)
{

	if (S_ISDIR(p1->fentry.st_mode) )
	{
		if (strcmp(p1->name,"..") == 0)
			return -1;
		if (S_ISDIR(p2->fentry.st_mode) )
			return strcmp(p1->name,p2->name) * cursort;
		else
			return -1;
	}
	if (S_ISDIR(p2->fentry.st_mode) )
	{
		if (strcmp(p1->name,"..") == 0)
			return -1;
		return 1;
	}
	else
		return strcmp(p1->name,p2->name) * cursort;
}

/******************************************************************************
 * sortframe                                                                  *
 ******************************************************************************/

void sortframe(int frame, char* szSel)
{
	int i;

	qsort(finfo[frame].flist,finfo[frame].count, sizeof(struct fileentry),(__compar_fn_t)flistcmp);
	for (i =0; i < finfo[frame].count; i++)
	{
		if (strcmp(getfileentry(frame, i)->name,szSel) == 0)
		{
			finfo[frame].selected = i;
			break;
		}
	}
}
/******************************************************************************
 * getfileentry                                                               *
 ******************************************************************************/

struct fileentry* getfileentry(int frame, int pos)
{
	return &finfo[frame].flist[pos];
}

/******************************************************************************
 * GetSelected                                                                *
 ******************************************************************************/

struct fileentry* GetSelected(int frame)
{
	return &finfo[frame].flist[finfo[frame].selected];
}
/******************************************************************************
 * FindFile                                                                   *
 ******************************************************************************/

struct fileentry* FindFile(int frame, const char* szFile)
{
	int i;
	for (i = 0; i < finfo[frame].count; i++)
	{
		if (strcmp(finfo[frame].flist[i].name,szFile) == 0) return &finfo[frame].flist[i];
	}
	return NULL;
}

/******************************************************************************
 * ClearEntries                                                               *
 ******************************************************************************/

void ClearEntries(int frame)
{

	if (finfo[frame].flist != NULL)
	{
		free(finfo[frame].flist);
		finfo[frame].flist = NULL;
	}
}
/******************************************************************************
 * ClearZipEntries                                                            *
 ******************************************************************************/

void ClearZipEntries(int frame)
{
	struct zipfileentry * pzfe, *pzfe1 = finfo[frame].allziplist;
	while (pzfe1 != NULL)
	{
		pzfe  = pzfe1;
		pzfe1 = pzfe1->next;
		free(pzfe);
	}
	finfo[frame].allziplist = NULL;
	finfo[frame].zipfile[0] = 0x00;

}
/******************************************************************************
 * ClearMarker                                                                *
 ******************************************************************************/

void ClearMarker(int frame)
{
	struct marker * pmk, *pmk1 = finfo[frame].mlist;
	while (pmk1 != NULL)
	{
		pmk = pmk1;
		pmk1 = pmk1->next;
		free(pmk);
	}
	finfo[frame].mlist = NULL;
	finfo[frame].markcount = 0;
	finfo[frame].marksize  = 0;
}
/******************************************************************************
 * ToggleMarker                                                               *
 ******************************************************************************/

void ToggleMarker(int frame)
{
	struct fileentry *pfe = GetSelected(frame);
	struct marker * pmk = NULL, *pmk1 = finfo[frame].mlist;

	if (strcmp(pfe->name,"..") == 0) return;
	while (pmk1 != NULL)
	{
		if (strcmp(pmk1->name,pfe->name) == 0)
		{
		  if (pmk == NULL)
			  finfo[frame].mlist = pmk1->next;
		  else
			  pmk->next = pmk1->next;
		  free(pmk1);
		  finfo[frame].markcount--;
		  if (!S_ISDIR(pfe->fentry.st_mode))
		  	finfo[frame].marksize -= pfe->fentry.st_size;
		  return;
		}
		pmk = pmk1;
		pmk1 = pmk1->next;
	}
	pmk = malloc(sizeof(struct marker));
	strcpy(pmk->name,pfe->name);
	pmk->next = finfo[frame].mlist;
	finfo[frame].mlist = pmk;
	finfo[frame].markcount++;
	if (!S_ISDIR(pfe->fentry.st_mode))
		finfo[frame].marksize += pfe->fentry.st_size;
}
/******************************************************************************
 * RenameMarker                                                               *
 ******************************************************************************/

void RenameMarker(int frame, const char* szOld, const char* szNew )
{
	struct marker * pmk = finfo[frame].mlist;

	while (pmk != NULL)
	{
		if (strcmp(pmk->name,szOld) == 0)
		{
		  strcpy(pmk->name,szNew);
		  return;
		}
		pmk = pmk->next;
	}
}
/******************************************************************************
 * IsMarked                                                                   *
 ******************************************************************************/

int IsMarked(int frame, int pos)
{
	struct fileentry *pfe = getfileentry(frame, pos);
	struct marker * pmk = finfo[frame].mlist;
	while (pmk != NULL)
	{
		if (strcmp(pmk->name,pfe->name) == 0)
		{
		  return 1;
		}
		pmk = pmk->next;
	}
	return 0;
}

/******************************************************************************
 * CheckOverwrite                                                             *
 ******************************************************************************/
int CheckOverwrite(const char* szFile, int mode)
{
	char szMessage[356];

	if (overwriteall != 0) return overwriteall;

	if (FindFile(1-curframe,szFile) != NULL)
	{
		if (skipall != 0)      return skipall;
		sprintf(szMessage,msg[MSG_FILE_EXISTS*NUM_LANG+language], szFile);
		switch (MessageBox(szMessage,"",mode))
		{
			case OVERWRITE:
				return OVERWRITE;
				break;
			case OVERWRITEALL:
				overwriteall = OVERWRITE;
				return OVERWRITE;
			case SKIP:
				return SKIP;
			case SKIPALL:
				skipall = SKIP;
				return SKIP;
			case CANCEL:
				overwriteall = -1;
				skipall = -1;
				return -1;

		}
	}
	return OVERWRITE;
}
/******************************************************************************
 * FillDir                                                                    *
 ******************************************************************************/

void FillDir(int frame, int selmode)
{

	char* pch;
	char selentry[256];
	*selentry = 0x00;
	char szSel[256];
	*szSel = 0x00;
	int npos = 0;
	struct fileentry* pfe = NULL;
	if (finfo[frame].selected > 0)
	{
	  strcpy(szSel,GetSelected(frame)->name);
	}
	if (finfo[frame].zipfile[0] != 0x00)
	{
		if ((selmode == SELECT_UPDIR) &&(frame == curframe))
		{
			if (strcmp(finfo[frame].zippath,"/") == 0)
			{
				strncat(finfo[curframe].path,finfo[frame].zipfile,256);
				strncat(finfo[curframe].path,"/",1);
				ClearZipEntries(frame);
			}
		}
	}
	ClearEntries(frame);
	if (finfo[frame].zipfile[0] != 0x00) // filling zipfile structure
	{
		if ((selmode == SELECT_UPDIR) &&(frame == curframe))
		{
			finfo[curframe].zippath[strlen(finfo[curframe].zippath)-1]=0x00;
			pch = strrchr(finfo[frame].zippath,'/');
			if (pch)
			{
				strcpy(selentry,(pch+1));
				*(pch+1) = 0x00;
			}
		}
		finfo[frame].count = 1;
		finfo[frame].size  = 0;
		finfo[frame].writable = 0;
		struct zipfileentry * pzfe;
		int zlen = strlen(finfo[frame].zippath);

		pzfe = finfo[frame].allziplist;
		while (pzfe != NULL)
		{
			if ((strncmp(finfo[frame].zippath,pzfe->name,zlen) == 0) && (strrchr(pzfe->name,'/') == (char*)(pzfe->name+zlen-1)))
			{
				finfo[frame].count++;
				finfo[frame].size+= pzfe->fentry.st_size;
			}
			pzfe = pzfe->next;
		}
		finfo[frame].flist = (struct fileentry*)calloc(finfo[frame].count, sizeof(struct fileentry));

		pfe = getfileentry(frame, npos);
		// create ".." entry
		strcpy(pfe->name,"..");
		memset(&pfe->fentry, 0, sizeof(struct stat));
		pfe->fentry.st_mode = S_IFDIR;
		npos++;
		pzfe = finfo[frame].allziplist;
		while (pzfe != NULL)
		{
			if ((strncmp(finfo[frame].zippath,pzfe->name,zlen) == 0)  && (strrchr(pzfe->name,'/') == (char*)(pzfe->name+zlen-1)))
			{
				pfe = getfileentry(frame, npos);
				int ppos = strcspn((char*)(pzfe->name+zlen),"/");
				if (ppos > 0)
					strncpy(pfe->name,(char*)(pzfe->name+zlen),ppos);
				else
					strcpy(pfe->name,(char*)(pzfe->name+zlen));
				memcpy(&pfe->fentry,&pzfe->fentry,sizeof(struct stat));
				if ((selmode == SELECT_UPDIR) && (strcmp(pfe->name,selentry) == 0) && (frame==curframe))
				{
					finfo[frame].selected = npos;
					strcpy(szSel,GetSelected(frame)->name);
				}
				npos++;
			}
			pzfe = pzfe->next;
		}
	}
	else // filling normal directory
	{
		if (finfo[frame].path[0] != '/') strcpy(finfo[frame].path, DEFAULT_PATH);
		if ((selmode == SELECT_UPDIR) &&(frame == curframe))
		{
			finfo[curframe].path[strlen(finfo[curframe].path)-1]=0x00;
			pch = strrchr(finfo[frame].path,'/');
			if (pch)
			{
				strcpy(selentry,(pch+1));
				*(pch+1) = 0x00;
			}

		}
		printf("filling directory structure:<%s>\n", finfo[frame].path);
		DIR* dp = opendir(finfo[frame].path);

		struct dirent *dentry;
		char fullfile[FILENAME_MAX];
		finfo[frame].count = (strcmp(finfo[frame].path,"/") == 0 ? 0 : 1);
		finfo[frame].size  = 0;
		finfo[frame].writable = 0;
		if (!dp)
		{
			printf("cannot read %s\n", finfo[frame].path);
			strcpy(finfo[frame].path, DEFAULT_PATH);
			dp = opendir(finfo[frame].path);
		}
		while((dentry = readdir(dp)) != NULL)
		{
			if (strcmp(dentry->d_name,".") == 0)
			{
				struct stat   st;
				sprintf(fullfile,"%s.",finfo[frame].path);
				lstat(fullfile,&st);
				if ((st.st_mode & S_IWUSR) == S_IWUSR )
					finfo[frame].writable = 1;
				continue;
			}
			if (strcmp(dentry->d_name,"..") == 0)
				continue;
			finfo[frame].count++;
		}

		finfo[frame].flist = (struct fileentry*)calloc(finfo[frame].count, sizeof(struct fileentry));


		// rewinddir not defined ?????????????????
		closedir(dp);
		dp = opendir(finfo[frame].path);

		if (strcmp(finfo[frame].path,"/") != 0)
		{

			pfe = getfileentry(frame, npos);
			// create ".." entry when not in root dir
			strcpy(pfe->name,"..");
			memset(&pfe->fentry, 0, sizeof(struct stat));
			pfe->fentry.st_mode = S_IFDIR;
			npos++;
		}
		while((dentry = readdir(dp)) != NULL && npos < finfo[frame].count)
		{
			if (strcmp(dentry->d_name,".") == 0)
				continue;
			if (strcmp(dentry->d_name,"..") == 0)
				continue;
			pfe = getfileentry(frame, npos);
			strcpy(pfe->name,dentry->d_name);
			sprintf(fullfile,"%s%s",finfo[frame].path,pfe->name);
			lstat(fullfile,&(pfe->fentry));
			if     (!((S_ISDIR(pfe->fentry.st_mode)) || (S_ISLNK(pfe->fentry.st_mode))))
			{
				finfo[frame].size+= pfe->fentry.st_size;
			}
			if ((selmode == SELECT_UPDIR) && (strcmp(pfe->name,selentry) == 0) && (frame==curframe))
			{
				finfo[frame].selected = npos;
				strcpy(szSel,GetSelected(frame)->name);
			}
			npos++;
		}
		closedir(dp);
	}
	cursort = finfo[frame].sort;
	sortframe(frame, szSel);



}

/******************************************************************************
 * DoCopy                                                                     *
 ******************************************************************************/

void DoCopy(struct fileentry* pfe, int typ)
{
	int i = 1;
	char action[512], szFullFile[1000], tp;
	if (finfo[curframe].zipfile[0] != 0x00)
	{
		if (tmpzipdir[0] == 0x00)
		while(1)
		{
			char szMessage[400];
			sprintf(szMessage,msg[MSG_EXTRACT*NUM_LANG+language], finfo[curframe].zipfile);
			MessageBox(szMessage,"",NOBUTTON);

			sprintf(tmpzipdir,"ziptmp%d",i);
			if (FindFile(1-curframe,tmpzipdir) == NULL)
			{
				sprintf(action,"mkdir -p \"%s%s\"",finfo[1-curframe].path, tmpzipdir);
				DoExecute(action, SHOW_NO_OUTPUT);
				break;
			}
			i++;
		}
		int zlen = strlen(szZipCommand);
		if (zlen == 0)
		{
			switch(finfo[curframe].ziptype)
			{
				case GZIP     : tp = 'z'; break;
				case BZIP2    : tp = 'j'; break;
				case COMPRESS : tp = 'Z'; break;
				case TAR      : tp = ' '; break;
				default: return;
			}
			sprintf(szZipCommand,"tar  -x%c -f \"%s%s\" -C \"%s%s\""
						,tp
						,finfo[curframe].path
						,finfo[curframe].zipfile
						,finfo[1-curframe].path
						,tmpzipdir);
			zlen = strlen(szZipCommand);

		}
		if (S_ISDIR(pfe->fentry.st_mode))
		{
			struct zipfileentry *pzfe = finfo[curframe].allziplist;
			while (pzfe != NULL)
			{
				if (zlen == 0)
				{
					switch(finfo[curframe].ziptype)
					{
						case GZIP     : tp = 'z'; break;
						case BZIP2    : tp = 'j'; break;
						case COMPRESS : tp = 'Z'; break;
						case TAR      : tp = ' '; break;
						default: return;
					}
					sprintf(szZipCommand,"tar  -x%c -f \"%s%s\" -C \"%s%s\""
								,tp
								,finfo[curframe].path
								,finfo[curframe].zipfile
								,finfo[1-curframe].path
								,tmpzipdir);
					zlen = strlen(szZipCommand);

				}

				sprintf(szFullFile,"%s%s/",finfo[curframe].zippath,pfe->name);
				if (strncmp(pzfe->name,szFullFile,strlen(szFullFile))== 0)
				{
					int dlen = strlen(pzfe->name)+2;
					if (dlen + zlen >= commandsize)
					{
						DoExecute(szZipCommand, SHOW_NO_OUTPUT);
						szZipCommand[0]=0x00;
						zlen = strlen(szZipCommand);
					}
					else
					{
						sprintf(szFullFile," \"%s\"",(char*)(pzfe->name+1));
						strcat(szZipCommand,szFullFile);
					}
				}
				pzfe = pzfe->next;
			}

		}
		int elen = strlen(pfe->name)+strlen(finfo[curframe].zippath)+2;
		if (elen + zlen >= commandsize)
		{
			DoExecute(szZipCommand, SHOW_NO_OUTPUT);
			szZipCommand[0]=0x00;
		}
		else
		{
			sprintf(szFullFile," \"%s%s\"",(char*)(finfo[curframe].zippath+1),pfe->name);
			strcat(szZipCommand,szFullFile);
		}
	}
	else
	{
		if (typ != HIDDEN)
		{
			char szMessage[400];
			sprintf(szMessage,msg[MSG_COPY_PROGRESS*NUM_LANG+language], pfe->name, finfo[1-curframe].path);
			MessageBox(szMessage,"",NOBUTTON);
		}
		sprintf(action,"cp -dpR \"%s%s\" \"%s\"%s",finfo[curframe].path,pfe->name, finfo[1-curframe].path,typ == HIDDEN ? " &" : "");
		DoExecute(action, SHOW_NO_OUTPUT);
	}
}

/******************************************************************************
 * DoZipCopyEnd                                                               *
 ******************************************************************************/

void DoZipCopyEnd()
{
	if (finfo[curframe].zipfile[0] != 0x00)
	{
		int zlen = strlen(szZipCommand);
		if (zlen > 0) DoExecute(szZipCommand, SHOW_NO_OUTPUT);
		sprintf(szZipCommand,"mv -f \"%s%s%s\"* \"%s\"",finfo[1-curframe].path,tmpzipdir,finfo[curframe].zippath, finfo[1-curframe].path);
		DoExecute(szZipCommand, SHOW_NO_OUTPUT);
		sprintf(szZipCommand,"rm -f -r \"%s%s\"",finfo[1-curframe].path,tmpzipdir);
		DoExecute(szZipCommand, SHOW_NO_OUTPUT);
	}
}
/******************************************************************************
 * DoMove                                                                     *
 ******************************************************************************/

void DoMove(char* szFile, int typ)
{
	char action[1000];
	sprintf(action,"mv -f \"%s%s\" \"%s\"%s",finfo[curframe].path,szFile, finfo[1-curframe].path,typ == HIDDEN ? " &":"");
	DoExecute(action, SHOW_NO_OUTPUT);
}
/******************************************************************************
 * DoViewFile                                                                 *
 ******************************************************************************/

void DoViewFile(char* szFile)
{
	char action[400];
	FILE* pFile;
	if (finfo[curframe].zipfile[0] != 0x00)
	{
		sprintf(action,"tar  -x%cO -f \"%s%s\"  \"%s%s\"",finfo[curframe].ziptype == GZIP ? 'z' :'j',finfo[curframe].path,finfo[curframe].zipfile,(char*)(finfo[curframe].zippath+1),szFile);
		pFile = OpenPipe(action);
	}
	else
	{
		sprintf(action,"%s%s",finfo[curframe].path, szFile);
		pFile = fopen(action,"r");
	}
	if  (pFile != NULL)
	{
		ShowFile(pFile, szFile);
		fclose(pFile);
}
}
/******************************************************************************
 * DoEditFile                                                                 *
 ******************************************************************************/

void DoEditFile(char* szFile)
{
	FILE* pFile = fopen(szFile,"r");
	char *p = szFileBuffer, *p1, *pcur = szFileBuffer;
	char szInputBuffer[1001];
	char szLineNumber[20];
	int count = 1;
	int changed = 0;

	*szFileBuffer = 0x00;
	while( fgets( p, FILEBUFFER_SIZE, pFile ) )
	{
	  p = (char*)(p+strlen(p));
	  count++;
	}
	fclose(pFile);

	int i,row = 0, sel = 0, strsize;



	while( 1 )
	{
		// Render output window
		RenderBox(               0, 0                          , viewx     , viewy-MENUSIZE             , FILL, trans_map[curvisibility]);
		RenderBox(               0, 0                          , BORDERSIZE, viewy-MENUSIZE             , FILL, WHITE);
		RenderBox(viewx-BORDERSIZE, 0                          , viewx     , viewy-MENUSIZE             , FILL, WHITE);
		RenderBox(               0, 0                          , viewx     , BORDERSIZE                 , FILL, WHITE);
		RenderBox(               0, BORDERSIZE+FONTHEIGHT_BIG  , viewx     , 2*BORDERSIZE+FONTHEIGHT_BIG, FILL, WHITE);
		RenderBox(               0, viewy-BORDERSIZE- MENUSIZE , viewx     , viewy-MENUSIZE             , FILL, WHITE);

		p = szFileBuffer;
		if (sel < 0 ) sel = 0;
		if (sel >= count) sel = count-1;
		if (sel < row) row = sel;
		if (sel > row+(framerows-2)) row = sel-(framerows-2);
		for (i =0; i < row; i++)
		{
          p1=strchr(p,'\n');
          if (p1 == NULL) break;
          p= p1+1;
		}
		sprintf(szLineNumber,msg[MSG_LINE*NUM_LANG+language],sel+1, count);
		strsize = GetStringLen(szLineNumber, BIG);
		RenderString(szFile      ,2*BORDERSIZE               , BORDERSIZE+FONTHEIGHT_BIG-FONT_OFFSET_BIG  , viewx-5*BORDERSIZE-strsize, LEFT, BIG, WHITE);
		RenderString(szLineNumber,viewx-2*BORDERSIZE-strsize , BORDERSIZE+FONTHEIGHT_BIG-FONT_OFFSET_BIG  , strsize+BORDERSIZE        , RIGHT, BIG, WHITE);

		if ( p )
		{
			for (i =0; i < framerows; i++)
			{
				if (sel == row + i)
				{
					pcur = p;
					RenderBox(BORDERSIZE, 2*BORDERSIZE+FONTHEIGHT_BIG+i*FONTHEIGHT_SMALL , viewx- BORDERSIZE , 2*BORDERSIZE+FONTHEIGHT_BIG+(i+1)*FONTHEIGHT_SMALL, FILL, BLUE2);
				}
          		RenderString(p,2*BORDERSIZE, 2*BORDERSIZE+FONTHEIGHT_BIG+(i+1)*FONTHEIGHT_SMALL -FONT_OFFSET, viewx-4*BORDERSIZE, LEFT, SMALL, WHITE);
          		p1=strchr(p,'\n');
	            if (p1 == NULL)
	            {
					i++;
					if (sel == row + i)
					{
						pcur+=strlen(pcur);
						RenderBox(BORDERSIZE, 2*BORDERSIZE+FONTHEIGHT_BIG+i*FONTHEIGHT_SMALL , viewx- BORDERSIZE , 2*BORDERSIZE+FONTHEIGHT_BIG+(i+1)*FONTHEIGHT_SMALL, FILL, BLUE2);
					}
					break;
				}
	            p = p1+1;
			}
			memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
			GetRCCode(SCROLL_NORMAL);
			switch (rccode)
			{
				case RC_HOME :
					break;
				case RC_OK :
				{
					p1 = strchr(pcur,'\n');
					int plen = (p1 ? p1-pcur: strlen(pcur));
					strncpy(szInputBuffer,pcur,plen);
					szInputBuffer[plen]=0x00;
					RenderBox(0, 2*BORDERSIZE+FONTHEIGHT_BIG+(sel-row)*FONTHEIGHT_SMALL-1 , viewx, 2*BORDERSIZE+FONTHEIGHT_BIG+(sel-row+1)*FONTHEIGHT_SMALL+1, GRID, WHITE);
					switch (DoEditString(BORDERSIZE,2*BORDERSIZE+FONTHEIGHT_BIG+(sel-row)*FONTHEIGHT_SMALL, viewx- 2*BORDERSIZE ,1000,szInputBuffer,BIG/*SMALL*/,BLUE2))
					{
						case RC_OK:
						{
							if (*pcur==0x00) {*pcur = '\n'; pcur++;}
							if (p1 && (plen != strlen(szInputBuffer)))
							  memmove(pcur+strlen(szInputBuffer),p1,FILEBUFFER_SIZE-((pcur+strlen(szInputBuffer))-szFileBuffer));
							memcpy(pcur,szInputBuffer,strlen(szInputBuffer));
							changed = 1;
							break;
						}
						default:
							rccode = 0;
							break;
					}
					break;
				}
				case RC_UP:
					sel--;
					break;
				case RC_DOWN:
					sel++;
					break;
				case RC_LEFT:
					sel-= framerows-1;
					if (sel >= 0) row = sel;
					break;
				case RC_RIGHT:
					sel+= framerows-1;
					if (sel < count) row = sel;
					break;
				case RC_PLUS:
					sel = 0;
					break;
				case RC_MINUS:
					sel = count;
					break;
				case RC_RED:
					p1 = strchr(pcur,'\n');
					if (p1)
					{
						memmove(pcur,p1+1,FILEBUFFER_SIZE-(pcur-szFileBuffer));
						changed = 1;
						if (count > 0 ) count--;
					}
					break;
				case RC_GREEN:
					memmove(pcur+1,pcur,FILEBUFFER_SIZE-(pcur-szFileBuffer+1));
					*pcur = '\n';
					count++;
					changed = 1;
					break;

			}
			if (rccode == RC_HOME)
			{
				if (changed)
				{
					char szMessage[400];
					sprintf(szMessage,msg[MSG_SAVE*NUM_LANG+language], szFile);
					switch (MessageBox(szMessage,"",YESNOCANCEL))
					{
						case YES:
							pFile = fopen(szFile,"w");
							if (pFile)
							{
								fputs(szFileBuffer,pFile);
								fclose(pFile);
							}

							rccode = -1;
							return;
						case NO:
							rccode = -1;
							return;
					}
				}
				else
				{
					rccode = -1;
					return;
				}

			}
		}
	}
	rccode = -1;
}
/******************************************************************************
 * DoExecute                                                                  *
 ******************************************************************************/

void DoExecute(char* szAction, int showoutput)
{
	printf("executing: %s\n", szAction);

	if (showoutput == SHOW_NO_OUTPUT)
	{

		char line1[128];
		char line2[128];

		*line1 = 0x00;
		*line2 = 0x00;

		int x = system(szAction);
		printf("result %d \n",x);

		if (x != 0)
			MessageBox("Error",strerror(x),OK);
	}
	else
	{

		FILE* pipe;
		pipe = OpenPipe(szAction);
		if (pipe== NULL)
		{
			printf("tuxcom: could not open pipe\n");
			char message[1000];
			sprintf(message,msg[MSG_EXEC_NOT_POSSIBLE*NUM_LANG+language],szAction);
			MessageBox(message,"",OK);

			return;
		}
		ShowFile(pipe, szAction);


		fclose(pipe);

	}
	rccode = -1;
}
/******************************************************************************
 * CheckZip                                                                   *
 ******************************************************************************/

int CheckZip(char* szName)
{
	int len = strlen(szName);
	if (len < 4) return -1;
	if (strcmp((char*)(szName+len-4),".tar") == 0) return TAR;
	if (len < 6) return -1;
	if (strcmp((char*)(szName+len-6),".tar.Z") == 0) return COMPRESS;
	if (len < 7) return -1;
	if (strcmp((char*)(szName+len-7),".tar.gz") == 0) return GZIP;
	if (len < 8) return -1;
	if (strcmp((char*)(szName+len-8),".tar.bz2") == 0) return BZIP2;
	return -1;
}
/******************************************************************************
 * ReadZip                                                                    *
 ******************************************************************************/

void ReadZip(int typ)
{
	MessageBox(msg[MSG_READ_ZIP_DIR*NUM_LANG+language],"",NOBUTTON);
	FILE* pipe;
	char szAction[400], szLine[400], name[FILENAME_MAX];
	char* p;
	char d,r,w,x;
	struct zipfileentry* pzfe1 = NULL, *pzfe2 = NULL;
	long size=0;

	ClearZipEntries(curframe);
	if      (typ == GZIP)
		sprintf(szAction,"tar -tzv -f \"%s%s\"",finfo[curframe].path,GetSelected(curframe)->name);
	else if (typ == BZIP2)
		sprintf(szAction,"tar -tjv -f \"%s%s\"",finfo[curframe].path,GetSelected(curframe)->name);
	else if (typ == COMPRESS)
		sprintf(szAction,"tar -tZv -f \"%s%s\"",finfo[curframe].path,GetSelected(curframe)->name);
	else if (typ == TAR)
		sprintf(szAction,"tar -tv -f \"%s%s\"",finfo[curframe].path,GetSelected(curframe)->name);
	else
	    return;
	strcpy(finfo[curframe].zipfile,GetSelected(curframe)->name);
	strcpy(finfo[curframe].zippath,"/");
	finfo[curframe].ziptype = typ;
	pipe = OpenPipe(szAction);
	if (pipe== NULL)
	{
		printf("tuxcom: could not open pipe\n");
		char message[1000];
		sprintf(message,msg[MSG_EXEC_NOT_POSSIBLE*NUM_LANG+language],szAction);
		MessageBox(message,"",OK);

		return;
	}
	while( fgets( szLine, 400, pipe ) )
	{
		p=strchr(szLine,'\n');
		if ( p )
			*p=0;
		sscanf(szLine,"%c%c%c%c%*s%*s%lu%*s%*s%s",&d,&r,&w,&x,&size,name);


		if (name[0] != 0x00)
		{
			pzfe1 = malloc(sizeof(struct zipfileentry));

			if (name[strlen(name)-1] == '/') name[strlen(name)-1]=0x00;
			if (name[0] != '/')
				sprintf(pzfe1->name,"/%s",name);
			else
				strcpy(pzfe1->name,name);
			pzfe1->next = NULL;
			memset(&pzfe1->fentry, 0, sizeof(struct stat));
			pzfe1->fentry.st_size = size;
			if (d == 'd') { pzfe1->fentry.st_mode |= S_IFDIR;pzfe1->fentry.st_size = 0;}
			if (r == 'r')   pzfe1->fentry.st_mode |= S_IRUSR;
			if (w == 'w')   pzfe1->fentry.st_mode |= S_IWUSR;
			if (w == 'x')   pzfe1->fentry.st_mode |= S_IXUSR;

			if (pzfe2 == NULL)
			  finfo[curframe].allziplist = pzfe1;
			else
			  pzfe2->next = pzfe1;
			pzfe2 = pzfe1;
		}
	}


	fclose(pipe);

}

/******************************************************************************
 * ShowFile                                                                   *
 ******************************************************************************/

void ShowFile(FILE* pipe, char* szAction)
{
	// Code from splugin (with little modifications...)
	char *p;
	char line[256];



	// Render output window
	RenderBox(               0, 0                          , viewx     , viewy-MENUSIZE             , FILL, trans_map[curvisibility]);
	RenderBox(               0, 0                          , BORDERSIZE, viewy-MENUSIZE             , FILL, WHITE);
	RenderBox(viewx-BORDERSIZE, 0                          , viewx     , viewy-MENUSIZE             , FILL, WHITE);
	RenderBox(               0, 0                          , viewx     , BORDERSIZE                 , FILL, WHITE);
	RenderBox(               0, BORDERSIZE+FONTHEIGHT_BIG  , viewx     , 2*BORDERSIZE+FONTHEIGHT_BIG, FILL, WHITE);
	RenderBox(               0, viewy-BORDERSIZE- MENUSIZE , viewx     , viewy-MENUSIZE             , FILL, WHITE);
	RenderString(szAction,2*BORDERSIZE, BORDERSIZE+FONTHEIGHT_BIG-FONT_OFFSET_BIG  , viewx-4*BORDERSIZE, CENTER, BIG, WHITE);

	int row = 0;
	while( fgets( line, 128, pipe ) )
	{
		p=strchr(line,'\n');
		if ( p )
			*p=0;
		row++;
		RenderString(line,2*BORDERSIZE, 2*BORDERSIZE+FONTHEIGHT_BIG+row*FONTHEIGHT_SMALL -FONT_OFFSET, viewx-4*BORDERSIZE, LEFT, SMALL, WHITE);

		if (row > framerows - 2)
		{
			memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
			while (1)
			{
				GetRCCode(SCROLL_NORMAL);
				if (rccode == RC_HOME || rccode == RC_OK)
					break;
			}
			row = 0;
			if (rccode == RC_HOME) break;
			// Render output window
			RenderBox(               0, 0                          , viewx     , viewy-MENUSIZE             , FILL, trans_map[curvisibility]);
			RenderBox(               0, 0                          , BORDERSIZE, viewy-MENUSIZE             , FILL, WHITE);
			RenderBox(viewx-BORDERSIZE, 0                          , viewx     , viewy-MENUSIZE             , FILL, WHITE);
			RenderBox(               0, 0                          , viewx     , BORDERSIZE                 , FILL, WHITE);
			RenderBox(               0, BORDERSIZE+FONTHEIGHT_BIG  , viewx     , 2*BORDERSIZE+FONTHEIGHT_BIG, FILL, WHITE);
			RenderBox(               0, viewy-BORDERSIZE- MENUSIZE , viewx     , viewy-MENUSIZE             , FILL, WHITE);
			RenderString(szAction,2*BORDERSIZE, BORDERSIZE+FONTHEIGHT_BIG-FONT_OFFSET_BIG  , viewx-4*BORDERSIZE, CENTER, BIG, WHITE);
		}
	}
	if (row>0)
	{
		memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
		while (1)
		{
			GetRCCode(SCROLL_NORMAL);
			if (rccode == RC_HOME || rccode == RC_OK)
				break;
		}
	}
	rccode = -1;
}
/******************************************************************************
 * OpenPipe                                                                   *
 ******************************************************************************/

FILE* OpenPipe(char* szAction)
{
	FILE *pipe;
#if TUXCOM_DBOX_VERSION == 3
	char szCommand[4000];
	sprintf(szCommand,"%s > /tmp/tuxcom.out",szAction);
	system(szCommand);
	pipe = fopen("/tmp/tuxcom.out","r");
#else
	pipe = popen(szAction,"r");
#endif
	return pipe;
}


/******************************************************************************
 * GetSizeString                                                              *
 ******************************************************************************/

void GetSizeString(char* sizeString, unsigned long long size)
{
	unsigned long long tmp = size;
	char sztmp[100];
	*sztmp = 0x00;


	while (tmp > 1000)
	{
		sprintf(sizeString,".%03lu%s",(unsigned long)(tmp % (unsigned long long)1000), sztmp);
		strcpy(sztmp,sizeString);
		tmp /= (unsigned long long)1000;
	}
	sprintf(sizeString,"%lu%s",(unsigned long)tmp,sztmp);

}

/******************************************************************************
 * ReadSettings                                                               *
 ******************************************************************************/

void ReadSettings()
{
	FILE *fp;
	char *p;
	char line[256];

	printf("tuxcom: reading settings \n");

	finfo[LEFTFRAME].sort = SORT_UP;
	finfo[RIGHTFRAME].sort = SORT_UP;

	fp = fopen( CONFIGDIR "/tuxcom.conf", "r" );
	if ( !fp )
	{
		printf("tuxcom: could not open " CONFIGDIR "/tuxcom.conf !!!\n");
	}
	else
	{
		while( fgets( line, 128, fp ) )
		{
			if ( *line == '#' )
				continue;
			if ( *line == ';' )
				continue;
			p=strchr(line,'\n');
			if ( p )
				*p=0;
			p=strchr(line,'=');
			if ( !p )
				continue;
			*p=0;
			p++;
			if ( !strcmp(line,"version") )
			{
				continue;
			}
			else if ( !strcmp(line,"curframe") )
			{
				curframe = atoi(p);
			}
			else if ( !strcmp(line,"curvisibility") )
			{
				curvisibility = atoi(p);
			}
			else if ( !strcmp(line,"ldir") )
			{
				strcpy(finfo[LEFTFRAME].path, p);
			}
			else if ( !strcmp(line,"rdir") )
			{
				strcpy(finfo[RIGHTFRAME].path, p);
			}
			else if ( !strcmp(line,"lsort") )
			{
				finfo[LEFTFRAME].sort = atoi(p);
				if (finfo[LEFTFRAME].sort == 0) finfo[LEFTFRAME].sort = SORT_UP;

			}
			else if ( !strcmp(line,"rsort") )
			{
				finfo[RIGHTFRAME].sort = atoi(p);
				if (finfo[RIGHTFRAME].sort == 0) finfo[RIGHTFRAME].sort = SORT_UP;
			}
		}
		fclose(fp);
	}
}
/******************************************************************************
 * WriteSettings                                                              *
 ******************************************************************************/

void WriteSettings()
{

	FILE *fp;


	fp = fopen( CONFIGDIR "/tuxcom.conf", "w" );
	if ( !fp )
	{
		printf("tuxcom: could not open " CONFIGDIR "/tuxcom.conf !!!\n");
	}
	else
	{
		fprintf(fp,"version=%d\n", INI_VERSION);
		fprintf(fp,"curframe=%d\n", curframe);
		fprintf(fp,"curvisibility=%d\n", curvisibility);
		fprintf(fp,"ldir=%s\n",finfo[LEFTFRAME ].path);
		fprintf(fp,"rdir=%s\n",finfo[RIGHTFRAME].path);
		fprintf(fp,"lsort=%d\n",finfo[LEFTFRAME ].sort);
		fprintf(fp,"rsort=%d\n",finfo[RIGHTFRAME].sort);
		fclose(fp);
	}
}
