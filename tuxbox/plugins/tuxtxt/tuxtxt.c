/******************************************************************************
 *                      <<< TuxTxt - Teletext Plugin >>>                      *
 *                                                                            *
 *             (c) Thomas "LazyT" Loewe 2002-2003 (LazyT@gmx.net)             *
 *                                                                            *
 *    TOP-Text Support 2004 by Roland Meier <RolandMeier@Siemens.com>         *
 *    Info entnommen aus videotext-0.6.19991029,                              *
 *    Copyright (c) 1994-96 Martin Buck  <martin-2.buck@student.uni-ulm.de>   *
 *                                                                            *
 ******************************************************************************/

#include "tuxtxt.h"

/******************************************************************************
 * plugin_exec                                                                *
 ******************************************************************************/

#if HAVE_DVB_API_VERSION < 3
 #define dmx_pes_filter_params dmxPesFilterParams
 #define pes_type pesType
 #define dmx_sct_filter_params dmxSctFilterParams
#endif


void next_dec(int *i) /* skip to next decimal */
{
	(*i)++;

	if ((*i & 0x0F) > 0x09)
		*i += 0x06;

	if ((*i & 0xF0) > 0x90)
		*i += 0x60;

	if (*i > 0x899)
		*i = 0x100;
}

void prev_dec(int *i)           /* counting down */
{
	(*i)--;

	if ((*i & 0x0F) > 0x09)
		*i -= 0x06;

	if ((*i & 0xF0) > 0x90)
		*i -= 0x60;

	if (*i < 0x100)
		*i = 0x899;
}

int is_dec(int i)
{
	return ((i & 0x00F) <= 9) && ((i & 0x0F0) <= 0x90);
}

int next_hex(int i) /* return next existing non-decimal page number */
{
	int startpage = i;
	
	do
	{
		i++;
		if (i > 0x8FF)
			i = 0x10F;
	} while (((subpagetable[i] == 0xFF) || is_dec(i)) && (startpage != i));
	return i;
}

void FillRect(int x, int y, int w, int h, int color)
{
	unsigned char *p = lfb + x + y * var_screeninfo.xres;

	if (w > 0)
		for ( ; h > 0 ; h--)
		{
			memset(p, color, w);
			p += var_screeninfo.xres;
		}
}

int getIndexOfPageInHotlist()
{
	int i;
	for (i = 0; i <= maxhotlist; i++)
	{
		if (page == hotlist[i])
			return i;
	}
	return -1;
}

void gethotlist()
{
	FILE *hl;
	char line[100];

	hotlistchanged = 0;
	maxhotlist = -1;
	sprintf(line, CONFIGDIR "/tuxtxt/hotlist%d.conf", vtxtpid);
#if DEBUG
	printf("TuxTxt <gethotlist %s", line);
#endif
	if ((hl = fopen(line, "rb")) != 0)
	{
		do {
			if (!fgets(line, sizeof(line), hl))
				break;

			if (1 == sscanf(line, "%x", &hotlist[maxhotlist+1]))
			{
				if (hotlist[maxhotlist+1] >= 0x100 && hotlist[maxhotlist+1] <= 0x899)
				{
#if DEBUG
					printf(" %03x", hotlist[maxhotlist+1]);
#endif
					maxhotlist++;
					continue;
				}
			}
#if DEBUG
			else
				printf(" ?%s?", line);
#endif
		} while (maxhotlist < (sizeof(hotlist)/sizeof(hotlist[0])-1));
		fclose(hl);
	}
#if DEBUG
	printf(">\n");
#endif
	if (maxhotlist < 0) /* hotlist incorrect or not found */
	{
		hotlist[0] = 0x100; /* create one */
		hotlist[1] = 0x303;
		maxhotlist = 1;
	}
}

void savehotlist()
{
	FILE *hl;
	char line[100];
	int i;

	hotlistchanged = 0;
	sprintf(line, CONFIGDIR "/tuxtxt/hotlist%d.conf", vtxtpid);
#if DEBUG
	printf("TuxTxt <savehotlist %s", line);
#endif
	if (maxhotlist != 1 || hotlist[0] != 0x100 || hotlist[1] != 0x303)
	{
		if ((hl = fopen(line, "wb")) != 0)
		{
			for (i = 0; i <= maxhotlist; i++)
			{
				fprintf(hl, "%03x\n", hotlist[i]);
#if DEBUG
				printf(" %03x", hotlist[i]);
#endif
			}
			fclose(hl);
		}
	}
	else
	{
		unlink(line); /* remove current hotlist file */
#if DEBUG
		printf(" (default - just deleted)");
#endif
	}
#if DEBUG
	printf(">\n");
#endif
}

void hex2str(char *s, unsigned int n)
{
	/* print hex-number into string, s points to last digit, caller has to provide enough space, no termination */
	do {
		char c = (n & 0xF);

		if (c > 9)
			c += 'A'-10;
		else
			c += '0';

		*s-- = c;
		n >>= 4;
	} while (n);
}

void decode_btt()
{
	/* basic top table */
	int i, current, b1, b2, b3, b4;

	current = 0x100;
	for (i = 0; i < 799; i++)
	{
		b1 = cachetable[0x1f0][0][40+i];
		if (b1 == ' ')
			b1 = 0;
		else
		{
			b1 = dehamming[b1];
			if (b1 == 0xFF) /* hamming error in btt */
			{
				cachetable[0x1f0][0][40+799] = 0; /* mark btt as not received */
				return;
			}
		}
		basictop[current] = b1;
		next_dec(&current);
	}
	/* page linking table */
	maxadippg = -1; /* rebuild table of adip pages */
	for (i = 0; i < 10; i++)
	{
		b1 = dehamming[cachetable[0x1f0][0][840 + 8*i +0]];

		if (b1 == 0xE)
			continue; /* unused */
		else if (b1 == 0xF)
			break; /* end */

		b4 = dehamming[cachetable[0x1f0][0][840 + 8*i +7]];

		if (b4 != 2) /* only adip, ignore multipage (1) */
			continue;

		b2 = dehamming[cachetable[0x1f0][0][840 + 8*i +1]];
		b3 = dehamming[cachetable[0x1f0][0][840 + 8*i +2]];

		if (b1 == 0xFF || b2 == 0xFF || b3 == 0xFF)
		{
			printf("TuxTxt <Biterror in btt/plt index %d>\n", i);
			cachetable[0x1f0][0][40+799] = 0; /* mark btt as not received */
			return;
		}

		b1 = b1<<8 | b2<<4 | b3; /* page number */
		adippg[++maxadippg] = b1;
	}
#if DEBUG
	printf("TuxTxt <BTT decoded>\n");
#endif
	bttok = 1;
}

void decode_adip() /* additional information table */
{
	int i, p, j, b1, b2, b3, charfound;

	for (i = 0; i <= maxadippg; i++)
	{
		p = adippg[i];
		if (cachetable[p][0]) /* cached (avoid segfault) */
		{
			if (cachetable[p][0][40+20*43+0] != 0x01) /* completely received, 1 is invalid as hamming */
			{
				for (j = 0; j < 44; j++)
				{
					b1 = dehamming[cachetable[p][0][40+20*j+0]];
					if (b1 == 0xE)
						continue; /* unused */

					if (b1 == 0xF)
						break; /* end */

					b2 = dehamming[cachetable[p][0][40+20*j+1]];
					b3 = dehamming[cachetable[p][0][40+20*j+2]];

					if (b1 == 0xFF || b2 == 0xFF || b3 == 0xFF)
					{
						printf("TuxTxt <Biterror in ait %03x %d %02x %02x %02x %02x %02x %02x>\n", p, j,
								 cachetable[p][0][40+20*j+0],
								 cachetable[p][0][40+20*j+1],
								 cachetable[p][0][40+20*j+2],
								 b1, b2, b3
								 );
						cachetable[p][40+20*43+0] = 0; /* mark as not received */
						return;
					}

					if (b1>8 || b2>9 || b3>9)
					{
						continue;
					}

					b1 = b1<<8 | b2<<4 | b3; /* page number */
					charfound = 0; /* flag: no printable char found */

					for (b2 = 11; b2 >= 0; b2--)
					{
						b3 = cachetable[p][0][40+20*j + 8 + b2];

						if ((b3&1) ^ ((b3>>1)&1) ^ ((b3>>2)&1) ^ ((b3>>3)&1) ^
						    ((b3>>4)&1) ^ ((b3>>5)&1) ^ ((b3>>6)&1) ^ (b3>>7))
							b3 &= 0x7F;
						else
							b3 = ' ';

						if (b3 < ' ')
							b3 = ' ';

						if (b3 == ' ' && !charfound)
							adip[b1][b2] = '\0';
						else
						{
							adip[b1][b2] = b3;
							charfound = 1;
						}
					}
				}
				adippg[i] = 0; /* completely decoded: clear entry */
			}
		}
#if DEBUG
		printf("TuxTxt <ADIP %03x decoded>\n", p);
#endif
	}

	while (!adippg[maxadippg] && (maxadippg >= 0)) /* and shrink table */
		maxadippg--;
}


int toptext_getnext(int startpage, int up, int findgroup)
{
	int current, nextgrp, nextblk;

	nextgrp = nextblk = 0;
	current = startpage;

	do {
		if (up)
			next_dec(&current);
		else
			prev_dec(&current);

		if (!bttok || basictop[current]) /* only if existent */
		{
			if (findgroup)
			{
				if (basictop[current] >= 6 && basictop[current] <= 7)
					return current;

				if (!nextgrp && (current&0x00F) == 0)
				{
					if (!bttok)
						return current;

					nextgrp = current;
				}
			}
			if (basictop[current] >= 2 && basictop[current] <= 5) /* always find block */
				return current;

			if (!nextblk && (current&0x0FF) == 0)
			{
				if (!bttok)
					return current;

				nextblk = current;
			}
		}
	} while (current != startpage);

	if (nextgrp)
		return nextgrp;
	else if (nextblk)
		return nextblk;
	else
		return startpage;
}

void RenderClearMenuLineBB(char *p, int attrcol, int attr)
{
	int col;

	PosX = TOPMENUSTARTX;
	RenderCharBB(' ', attrcol);			 /* indicator for navigation keys */
#if 0
	RenderCharBB(' ', attr);				 /* separator */
#endif
	for(col = 0; col < TOPMENUCHARS; col++)
	{
		RenderCharBB(*p++, attr);
	}
	PosY += fontheight;
	memset(p-TOPMENUCHARS, ' ', TOPMENUCHARS); /* init with spaces */
}

void ClearBB(int color)
{
	if (var_screeninfo.yoffset)
		memset(lfb, color, var_screeninfo.xres*var_screeninfo.yres);
	else
		memset(lfb + var_screeninfo.xres*var_screeninfo.yres, color, var_screeninfo.xres*var_screeninfo.yres);
}

void ClearFB(int color)
{
	memset(lfb + var_screeninfo.xres*var_screeninfo.yoffset, color, var_screeninfo.xres*var_screeninfo.yres);
}

void ClearB(int color)
{
	memset(lfb, color, 2*var_screeninfo.xres*var_screeninfo.yres);
}
void SetPosX(int column)
{
#if CFGTTF
		int abx = ((displaywidth)%(40-nofirst) == 0 ? displaywidth+1 : (displaywidth)/(((displaywidth)%(40-nofirst))));// distance between 'inserted' pixels
//		int abx = (displaywidth)/(((displaywidth)%(40-nofirst))+1);// distance between 'inserted' pixels
		PosX = StartX;
		int i;
		for (i = 0; i < column-nofirst; i++)
			PosX += fontwidth+(((PosX-sx) / abx) < ((PosX+fontwidth+1-sx) /abx) ? 1 : 0);
//		PosX = StartX + (column-nofirst)*fontwidth + ((column-nofirst)*fontwidth/abx);
#else
		PosX = StartX + (column-nofirst)*fontwidth;
#endif
}

void plugin_exec(PluginParam *par)
{
	char cvs_revision[] = "$Revision: 1.80 $";

	/* show versioninfo */
	sscanf(cvs_revision, "%*s %s", versioninfo);
	printf("TuxTxt %s\n", versioninfo);

	/* get params */
	vtxtpid = fb = lcd = rc = sx = ex = sy = ey = -1;

	for (; par; par = par->next)
	{
		if (!strcmp(par->id, P_ID_VTXTPID))
			vtxtpid = atoi(par->val);
		else if (!strcmp(par->id, P_ID_FBUFFER))
			fb = atoi(par->val);
		else if (!strcmp(par->id, P_ID_LCD))
			lcd = atoi(par->val);
		else if (!strcmp(par->id, P_ID_RCINPUT))
			rc = atoi(par->val);
		else if (!strcmp(par->id, P_ID_OFF_X))
			sx = atoi(par->val);
		else if (!strcmp(par->id, P_ID_END_X))
			ex = atoi(par->val);
		else if (!strcmp(par->id, P_ID_OFF_Y))
			sy = atoi(par->val);
		else if (!strcmp(par->id, P_ID_END_Y))
			ey = atoi(par->val);
	}

	if (vtxtpid == -1 || fb == -1 || rc == -1 || sx == -1 || ex == -1 || sy == -1 || ey == -1)
	{
		printf("TuxTxt <Invalid Param(s)>\n");
		return;
	}

	/* initialisations */
	if (Init() == 0)
		return;

	/* main loop */
	do {
		if (GetRCCode() == 1)
		{
			if (transpmode == 2) /* TV mode */
			{
				switch (RCCode)
				{
				case RC_UP:
				case RC_DOWN:
				case RC_0:
				case RC_1:
				case RC_2:
				case RC_3:
				case RC_4:
				case RC_5:
				case RC_6:
				case RC_7:
				case RC_8:
				case RC_9:
				case RC_RED:
				case RC_GREEN:
				case RC_YELLOW:
				case RC_BLUE:
				case RC_PLUS:
				case RC_MINUS:
				case RC_DBOX:
				case RC_STANDBY:
					transpmode = 1; /* switch to normal mode */
					SwitchTranspMode();
					break;		/* and evaluate key */

				case RC_MUTE:		/* regular toggle to transparent */
					break;

#ifndef DREAMBOX
				case RC_HELP: /* switch to scart input and back */
				{
					int i, n;
					int vendor = tuxbox_get_vendor();

					if (--vendor < 3)	/* scart-parameters only known for 3 dboxes, FIXME: order must be like in info.h */
					{
						for (i = 0; i < 6; i++) /* FIXME: FBLK seems to cause troubles */
						{
							n = avstable_scart[vendor][i];
							if ((ioctl(avs, avstable_ioctl[i], &n)) < 0)
								perror("TuxTxt <ioctl(avs)>");
						}
						
						while (GetRCCode() != 1) /* wait for any key */
							UpdateLCD();

						if (RCCode == RC_HELP)
							restoreaudio = 1;
						else
							restoreaudio = 0;
						
						for (i = 0; i < 6; i += (restoreaudio ? 2 : 1)) /* exit with ?: just restore video, leave audio */
						{
							n = avstable_dvb[vendor][i];
							if ((ioctl(avs, avstable_ioctl[i], &n)) < 0)
								perror("TuxTxt <ioctl(avs)>");
						}
					}
					continue; /* otherwise ignore exit key */
				}
#endif
				default:
					continue; /* ignore all other keys */
				}
			}

			switch (RCCode)
			{
			case RC_UP:
				if (swapupdown)
					GetPrevPageOne();
				else
					GetNextPageOne();
				break;
			case RC_DOWN:
				if (swapupdown)
					GetNextPageOne();
				else
					GetPrevPageOne();
				break;
			case RC_RIGHT:	GetNextSubPage();	break;
			case RC_LEFT:	GetPrevSubPage();	break;
			case RC_OK:
				if (subpagetable[page] == 0xFF)
					continue;
				PageCatching();
				break;

			case RC_0:	PageInput(0);		break;
			case RC_1:	PageInput(1);		break;
			case RC_2:	PageInput(2);		break;
			case RC_3:	PageInput(3);		break;
			case RC_4:	PageInput(4);		break;
			case RC_5:	PageInput(5);		break;
			case RC_6:	PageInput(6);		break;
			case RC_7:	PageInput(7);		break;
			case RC_8:	PageInput(8);		break;
			case RC_9:	PageInput(9);		break;
			case RC_RED:	Prev100();		break;
			case RC_GREEN:	Prev10();		break;
			case RC_YELLOW:	Next10();		break;
			case RC_BLUE:	Next100();		break;
			case RC_PLUS:	SwitchZoomMode();	break;
			case RC_MINUS:	SwitchScreenMode(-1);	break;
			case RC_MUTE:	SwitchTranspMode();	break;
			case RC_HELP:	SwitchHintMode();	break;
			case RC_DBOX:	ConfigMenu(0);		break;
			}
		}

		/* update page or timestring and lcd */
		RenderPage();
	} while ((RCCode != RC_HOME) && (RCCode != RC_STANDBY));

	/* exit */
	CleanUp();
}

/******************************************************************************
 * MyFaceRequester
 ******************************************************************************/

FT_Error MyFaceRequester(FTC_FaceID face_id, FT_Library library, FT_Pointer request_data, FT_Face *aface)
{
	FT_Error result;

	result = FT_New_Face(library, face_id, 0, aface);

#if DEBUG
	if (!result)
		printf("TuxTxt <font %s loaded>\n", (char*)face_id);
	else
		printf("TuxTxt <open font %s failed>\n", (char*)face_id);
#endif

	return result;
}

/******************************************************************************
 * Init                                                                       *
 ******************************************************************************/

int Init()
{
	struct dmx_pes_filter_params dmx_flt;
	int error;
	unsigned char magazine;

	/* init data */
	memset(&cachetable, 0, sizeof(cachetable));
	memset(&subpagetable, 0xFF, sizeof(subpagetable));
	memset(&countrycontrolbitstable, 0xFF, sizeof(countrycontrolbitstable));

	memset(&basictop, 0, sizeof(basictop));
	memset(&adip, 0, sizeof(adip));
	maxadippg  = -1;
	bttok      = 0;
	maxhotlist = -1;

	page_atrb[32] = transp<<4 | transp;
	inputcounter  = 2;
	cached_pages  = 0;

	for (magazine = 1; magazine < 9; magazine++)
	{
		current_page  [magazine] = -1;
		current_subpage [magazine] = -1;
	}
	page_receiving = -1;

	page       = 0x100;
	lastpage   = 0x100;
	prev_100   = 0x100;
	prev_10    = 0x100;
	next_100   = 0x100;
	next_10    = 0x100;
	subpage    = 0;
	pageupdate = 0;

	zap_subpage_manual = 0;

	/* init lcd */
	UpdateLCD();

	/* load config */
	screenmode = 0;
	screen_mode1 = 0;
	screen_mode2 = 0;
	color_mode   = 1;
	menulanguage = 0;	/* german */
	national_subset = 4;	/* german */
	auto_national   = 1;
	swapupdown      = 0;
	showhex         = 0;
	showflof        = 1;
	show39          = 1;

	if ((conf = fopen(TUXTXTCONF, "rt")) == 0)
	{
		perror("TuxTxt <fopen tuxtxt.conf>");
	}
	else
	{
		while(1)
		{
			char line[100];
			int ival;
			
			if (!fgets(line, sizeof(line), conf))
				break;
			
			if (1 == sscanf(line, "ScreenMode16x9Normal %d", &ival))
				screen_mode1 = ival & 1;
			else if (1 == sscanf(line, "ScreenMode16x9Divided %d", &ival))
				screen_mode2 = ival & 1;
			else if (1 == sscanf(line, "ColorDimmed %d", &ival))
				color_mode = ival & 1;
			else if (1 == sscanf(line, "AutoNational %d", &ival))
				auto_national = ival & 1;
			else if (1 == sscanf(line, "NationalSubset %d", &ival))
			{
				if (ival >= 0 && ival <= 12)
					national_subset = ival;
			}
			else if (1 == sscanf(line, "MenuLanguage %d", &ival))
			{
				if (ival >= 0 && ival <= MAXMENULANGUAGE)
					menulanguage = ival;
			}
			else if (1 == sscanf(line, "SwapUpDown %d", &ival))
				swapupdown = ival & 1;
			else if (1 == sscanf(line, "ShowHexPages %d", &ival))
				showhex = ival & 1;
			else if (1 == sscanf(line, "OverlayTransparency %x", &ival))
				tr1[transp2-1] = tr2[transp2-1] = ival & 0xFFFF;
			else if (1 == sscanf(line, "TTFWidthFactor %d", &ival))
	            TTFWIDTHFACTOR = ival;
			else if (1 == sscanf(line, "Screenmode %d", &ival))
	            screenmode = ival;
			else if (1 == sscanf(line, "ShowFLOF %d", &ival))
	            showflof = ival;
			else if (1 == sscanf(line, "Show39 %d", &ival))
	            show39 = ival;
		}
		fclose(conf);
	}
	saveconfig = 0;
	savedscreenmode = screenmode;
	
	/* init fontlibrary */
	if ((error = FT_Init_FreeType(&library)))
	{
		printf("TuxTxt <FT_Init_FreeType: 0x%.2X>", error);
		return 0;
	}

	if ((error = FTC_Manager_New(library, 7, 2, 0, &MyFaceRequester, NULL, &manager)))
	{
		FT_Done_FreeType(library);
		printf("TuxTxt <FTC_Manager_New: 0x%.2X>\n", error);
		return 0;
	}

	if ((error = FTC_SBit_Cache_New(manager, &cache)))
	{
		FTC_Manager_Done(manager);
		FT_Done_FreeType(library);
		printf("TuxTxt <FTC_SBit_Cache_New: 0x%.2X>\n", error);
		return 0;
	}

#if CFGTTF 

	/* calculate font dimensions */
	displaywidth = (ex-sx);
	fontheight = 21;//(ey-sy) / 25;
	fontwidth = fontwidth_normal = (ex-sx) / 40;
	fontwidth_topmenumain = (TV43STARTX-sx) / 40;
	fontwidth_topmenusmall = (ex- TV43STARTX) / TOPMENUCHARS;
	fontwidth_small = (TV169FULLSTARTX-sx)  / 40;
	ymosaic[0] = 0; /* y-offsets for 2*3 mosaic */
	ymosaic[1] = (fontheight + 1) / 3;
	ymosaic[2] = (fontheight * 2 + 1) / 3;
	ymosaic[3] = fontheight;

	/* center screen */
	StartX = sx; //+ (((ex-sx) - 40*fontwidth) / 2);
	StartY = sy + (((ey-sy) - 25*fontheight) / 2);

	typettf.font.face_id = (FTC_FaceID) TUXTXTTTF;
	typettf.font.pix_width  = (FT_UShort) fontwidth * TTFWIDTHFACTOR;
	typettf.font.pix_height = (FT_UShort) fontheight;
#if HAVE_DVB_API_VERSION >= 3
	typettf.flags = FT_LOAD_MONOCHROME;
#else 
	typettf.image_type = ftc_image_mono; 
#endif
	if ((error = FTC_Manager_Lookup_Face(manager, TUXTXTTTF, &face)))
	{
		printf("TuxTxt <FTC_Manager_Lookup_Face failed with Errorcode 0x%.2X>\n", error);
		FTC_Manager_Done(manager);
		FT_Done_FreeType(library);
		return 0;
	}
	ascender = fontheight * face->ascender / face->units_per_EM;
#if DEBUG
	printf("TuxTxt <fh%d fw%d fs%d tm%d ts%d ym%d %d %d sx%d sy%d a%d>\n",
			 fontheight, fontwidth, fontwidth_small, fontwidth_topmenumain, fontwidth_topmenusmall,
			 ymosaic[0], ymosaic[1], ymosaic[2], StartX, StartY, ascender);
#endif
	
#else	 /* !TTF: fixed fonts */

	type0.font.face_id = (FTC_FaceID) TUXTXT0;
	type1.font.face_id = (FTC_FaceID) TUXTXT1;
	type2.font.face_id = (FTC_FaceID) TUXTXT2;
#if HAVE_DVB_API_VERSION >= 3
	type0.flags = type1.flags = type2.flags = FT_LOAD_MONOCHROME;
#endif
	type0.font.pix_width  = type1.font.pix_width  = type2.font.pix_width  = (FT_UShort) fontwidth_normal;
	type0.font.pix_height = type1.font.pix_height = type2.font.pix_height = (FT_UShort) fontheight+2;

	type0r = type0;
	type1r = type1;
	type2r = type2;
	type0r.font.pix_width  = type1r.font.pix_width  = type2r.font.pix_width  = (FT_UShort) fontwidth_topmenumain;
	type0r.font.face_id = (FTC_FaceID) TUXTXT0R;
	type1r.font.face_id = (FTC_FaceID) TUXTXT1R;
	type2r.font.face_id = (FTC_FaceID) TUXTXT2R;

	/* center screen */
	StartX = sx + (((ex-sx) - 40*fontwidth) / 2);
	StartY = sy + (((ey-sy) - 25*fontheight) / 2);

#endif /* !TTF */

	/* get fixed screeninfo */
	if (ioctl(fb, FBIOGET_FSCREENINFO, &fix_screeninfo) == -1)
	{
		perror("TuxTxt <FBIOGET_FSCREENINFO>");
		return 0;
	}

	/* get variable screeninfo */
	if (ioctl(fb, FBIOGET_VSCREENINFO, &var_screeninfo) == -1)
	{
		perror("TuxTxt <FBIOGET_VSCREENINFO>");
		return 0;
	}

	/* set variable screeninfo for double buffering */
	var_screeninfo.yres_virtual = 2*var_screeninfo.yres;
	var_screeninfo.xres_virtual = var_screeninfo.xres;
	var_screeninfo.yoffset      = 0;

	if (ioctl(fb, FBIOPUT_VSCREENINFO, &var_screeninfo) == -1)
	{
		perror("TuxTxt <FBIOPUT_VSCREENINFO>");
		return 0;
	}
#if DEBUG
	if (ioctl(fb, FBIOGET_VSCREENINFO, &var_screeninfo) == -1)
	{
		perror("TuxTxt <FBIOGET_VSCREENINFO>");
		return 0;
	}

	printf("TuxTxt <screen real %d*%d, virtual %d*%d, offset %d>\n",
	       var_screeninfo.xres, var_screeninfo.yres,
	       var_screeninfo.xres_virtual, var_screeninfo.yres_virtual,
	       var_screeninfo.yoffset);
#endif

#ifndef DREAMBOX
	/* "correct" semi-transparent for Nokia (GTX only allows 2(?) levels of transparency) */
	if (tuxbox_get_vendor() == TUXBOX_VENDOR_NOKIA)
	{
		tr1[transp2-1] = 0xFFFF;
		tr2[transp2-1] = 0xFFFF;
	}
#endif

	/* set new colormap */
	if (color_mode)
	{
		if (ioctl(fb, FBIOPUTCMAP, &colormap_2) == -1)
		{
			perror("TuxTxt <FBIOPUTCMAP>");
			return 0;
		}
	}
	else
	{
		if (ioctl(fb, FBIOPUTCMAP, &colormap_1) == -1)
		{
			perror("TuxTxt <FBIOPUTCMAP>");
			return 0;
		}
	}

	/* map framebuffer into memory */
	lfb = (unsigned char*)mmap(0, fix_screeninfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);

	if (!lfb)
	{
		perror("TuxTxt <mmap>");
		return 0;
	}
	ClearBB(black); /* initialize backbuffer */

	/* open demuxer */
	if ((dmx = open(DMX, O_RDWR)) == -1)
	{
		perror("TuxTxt <open DMX>");
		return 0;
	}

	/*  if no vtxtpid for current service, search PIDs */
	if (vtxtpid == 0)
	{
		/* get all vtxt-pids */
		getpidsdone = -1;						 /* don't kill thread */
		if (GetTeletextPIDs() == 0)
		{
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			close(dmx);
			return 0;
		}

		if (pids_found > 1)
			ConfigMenu(1);
		else
		{
			vtxtpid = pid_table[0].vtxt_pid;
			current_national_subset = pid_table[0].national_subset;
			current_service = 0;
			RenderMessage(ShowServiceName);
		}
	}
	else
	{
		SDT_ready = 0;
		getpidsdone = 0;
		current_national_subset = (auto_national ? -1 : national_subset); /* default from config file if not set to automatic */
		pageupdate = 1; /* force display of message page not found (but not twice) */
	}

	/* open avs */
	if ((avs = open(AVS, O_RDWR)) == -1)
	{
		perror("TuxTxt <open AVS>");
		return 0;
	}

	ioctl(avs, AVSIOGSCARTPIN8, &fnc_old);
	ioctl(avs, AVSIOSSCARTPIN8, &fncmodes[screen_mode1]);

	/* open saa */
	if ((saa = open(SAA, O_RDWR)) == -1)
	{
		perror("TuxTxt <open SAA>");
		return 0;
	}

	ioctl(saa, SAAIOGWSS, &saa_old);
	ioctl(saa, SAAIOSWSS, &saamodes[screen_mode1]);

	/* open pig */
	if ((pig = open(PIG, O_RDWR)) == -1)
	{
		perror("TuxTxt <open PIG>");
		return 0;
	}

	/* setup rc */
	fcntl(rc, F_SETFL, O_NONBLOCK);
	ioctl(rc, RC_IOCTL_BCODES, 1);

	if (ioctl(dmx, DMX_SET_BUFFER_SIZE, 64*1024) < 0)
	{
		perror("Tuxtxt <DMX_SET_BUFFERSIZE>");
		return 0;
	}

	/* set filter & start demuxer */
	dmx_flt.pid      = vtxtpid;
	dmx_flt.input    = DMX_IN_FRONTEND;
	dmx_flt.output   = DMX_OUT_TAP;
	dmx_flt.pes_type = DMX_PES_OTHER;
	dmx_flt.flags    = DMX_IMMEDIATE_START;

	if (ioctl(dmx, DMX_SET_PES_FILTER, &dmx_flt) == -1)
	{
		perror("TuxTxt <DMX_SET_PES_FILTER>");
		return 0;
	}

	/* create decode-thread */
	if (pthread_create(&thread_id, NULL, CacheThread, NULL) != 0)
	{
		perror("TuxTxt <pthread_create>");
		return 0;
	}

	gethotlist();
	SwitchScreenMode(screenmode);

	/* init successfull */
	return 1;
}

/******************************************************************************
 * Cleanup                                                                    *
 ******************************************************************************/

void CleanUp()
{
	int curscreenmode = screenmode;
	
	/* hide pig */
	if (screenmode)
		SwitchScreenMode(0); /* turn off divided screen */

	/* restore videoformat */
	ioctl(avs, AVSIOSSCARTPIN8, &fnc_old);
	ioctl(saa, SAAIOSWSS, &saa_old);

#ifndef DREAMBOX
	if (restoreaudio)
	{
		int vendor = tuxbox_get_vendor() - 1;
		if (vendor < 3) /* scart-parameters only known for 3 dboxes, FIXME: order must be like in info.h */
		{
			for (i = 1; i < 6; i += 2) /* restore dvb audio */
			{
				n = avstable_dvb[vendor][i];
				if ((ioctl(avs, avstable_ioctl[i], &n)) < 0)
					perror("TuxTxt <ioctl(avs)>");
			}
		}
	}
#endif

	/* stop decode-thread */
	if (pthread_cancel(thread_id) != 0)
	{
		perror("TuxTxt <pthread_cancel>");
		return;
	}

	if (pthread_join(thread_id, &thread_result) != 0)
	{
		perror("TuxTxt <pthread_join>");
		return;
	}

	if (var_screeninfo.yoffset)
	{
		var_screeninfo.yoffset = 0;

		if (ioctl(fb, FBIOPAN_DISPLAY, &var_screeninfo) == -1)
			perror("TuxTxt <FBIOPAN_DISPLAY>");
	}

	/* stop & close demuxer */
	ioctl(dmx, DMX_STOP);
	close(dmx);

	 /* close avs */
	close(avs);

	/* close saa */
	close(saa);

	/* close freetype */
	FTC_Manager_Done(manager);
	FT_Done_FreeType(library);

	/* unmap framebuffer */
	munmap(lfb, fix_screeninfo.smem_len);

	/* free pagebuffers */
	for (clear_page = 0; clear_page < 0x900; clear_page++)
		for (clear_subpage = 0; clear_subpage < 0x80; clear_subpage++)
			if (cachetable[clear_page][clear_subpage] != 0)
				free(cachetable[clear_page][clear_subpage]);

	if (hotlistchanged)
		savehotlist();
				
	/* save config */
	if (saveconfig || curscreenmode != savedscreenmode)
	{
		if ((conf = fopen(TUXTXTCONF, "wt")) == 0)
		{
			perror("TuxTxt <fopen tuxtxt.conf>");
		}
		else
		{
			printf("TuxTxt <saving config>\n");
			fprintf(conf, "ScreenMode16x9Normal %d\n", screen_mode1);
			fprintf(conf, "ScreenMode16x9Divided %d\n", screen_mode2);
			fprintf(conf, "ColorDimmed %d\n", color_mode);
			fprintf(conf, "MenuLanguage %d\n", menulanguage);
			fprintf(conf, "AutoNational %d\n", auto_national);
			fprintf(conf, "NationalSubset %d\n", national_subset);
			fprintf(conf, "SwapUpDown %d\n", swapupdown);
			fprintf(conf, "ShowHexPages %d\n", showhex);
			fprintf(conf, "OverlayTransparency %X\n", tr1[transp2-1]);
			fprintf(conf, "TTFWidthFactor %d\n", TTFWIDTHFACTOR);
			fprintf(conf, "Screenmode %d\n", curscreenmode);
			fprintf(conf, "ShowFLOF %d\n", showflof);
			fprintf(conf, "Show39 %d\n", show39);
			fclose(conf);
		}
	}
}

/******************************************************************************
 * GetTeletextPIDs                                                           *
 ******************************************************************************/

int GetTeletextPIDs()
{
	struct dmx_sct_filter_params dmx_flt;
	struct dmx_pes_filter_params dmx_pes_flt;
	int pat_scan, pmt_scan, sdt_scan, desc_scan, pid_test, byte, diff, first_sdt_sec;

	unsigned char PAT[1024];
	unsigned char SDT[1024];
	unsigned char PMT[1024];

	if (!getpidsdone)							 /* call not from Init */
	{
		/* stop old decode-thread */
		if (pthread_cancel(thread_id) != 0)
		{
			perror("TuxTxt <pthread_cancel>");
		}

		if (pthread_join(thread_id, &thread_result) != 0)
		{
			perror("TuxTxt <pthread_join>");
		}

		/* stop demuxer */
		ioctl(dmx, DMX_STOP);
	}

	/* show infobar */
	RenderMessage(ShowInfoBar);

	/* read PAT to get all PMT's */
#if HAVE_DVB_API_VERSION < 3
	memset(dmx_flt.filter.filter, 0, DMX_FILTER_SIZE);
	memset(dmx_flt.filter.mask, 0, DMX_FILTER_SIZE);
#else
	memset(&dmx_flt.filter, 0x00, sizeof(struct dmx_filter));
#endif

	dmx_flt.pid              = 0x0000;
	dmx_flt.flags            = DMX_ONESHOT | DMX_CHECK_CRC | DMX_IMMEDIATE_START;
	dmx_flt.filter.filter[0] = 0x00;
	dmx_flt.filter.mask[0]   = 0xFF;
	dmx_flt.timeout          = 5000;

	if (ioctl(dmx, DMX_SET_FILTER, &dmx_flt) == -1)
	{
		perror("TuxTxt <DMX_SET_FILTER PAT>");
		return 0;
	}

	if (read(dmx, PAT, sizeof(PAT)) == -1)
	{
		perror("TuxTxt <read PAT>");
		return 0;
	}

	/* scan each PMT for vtxt-pid */
	pids_found = 0;

	for (pat_scan = 0x0A; pat_scan < 0x0A + (((PAT[0x01]<<8 | PAT[0x02]) & 0x0FFF) - 9); pat_scan += 4)
	{
		if (((PAT[pat_scan - 2]<<8) | (PAT[pat_scan - 1])) == 0)
			continue;

		dmx_flt.pid               = (PAT[pat_scan]<<8 | PAT[pat_scan+1]) & 0x1FFF;
		dmx_flt.flags             = DMX_ONESHOT | DMX_CHECK_CRC | DMX_IMMEDIATE_START;
		dmx_flt.filter.filter[0]  = 0x02;
		dmx_flt.filter.mask[0]    = 0xFF;
		dmx_flt.timeout           = 5000;

		if (ioctl(dmx, DMX_SET_FILTER, &dmx_flt) == -1)
		{
			perror("TuxTxt <DMX_SET_FILTER PMT>");
			continue;
		}

		if (read(dmx, PMT, sizeof(PMT)) == -1)
		{
			perror("TuxTxt <read PMT>");
			continue;
		}
		for (pmt_scan = 0x0C + ((PMT[0x0A]<<8 | PMT[0x0B]) & 0x0FFF); pmt_scan < (((PMT[0x01]<<8 | PMT[0x02]) & 0x0FFF) - 7); pmt_scan += 5 + PMT[pmt_scan + 4])
		{
			if (PMT[pmt_scan] == 6)
			{
				for (desc_scan = pmt_scan + 5; desc_scan < pmt_scan + ((PMT[pmt_scan + 3]<<8 | PMT[pmt_scan + 4]) & 0x0FFF) + 5; desc_scan += 2 + PMT[desc_scan + 1])
				{
					if (PMT[desc_scan] == 0x56)
					{
						char country_code[4];
							
						for (pid_test = 0; pid_test < pids_found; pid_test++)
							if (pid_table[pid_test].vtxt_pid == ((PMT[pmt_scan + 1]<<8 | PMT[pmt_scan + 2]) & 0x1FFF))
								goto skip_pid;

						pid_table[pids_found].vtxt_pid     = (PMT[pmt_scan + 1]<<8 | PMT[pmt_scan + 2]) & 0x1FFF;
						pid_table[pids_found].service_id = PMT[0x03]<<8 | PMT[0x04];
						if (PMT[desc_scan + 1] == 5)
						{
							country_code[0] = PMT[desc_scan + 2] | 0x20;
							country_code[1] = PMT[desc_scan + 3] | 0x20;
							country_code[2] = PMT[desc_scan + 4] | 0x20;
							country_code[3] = 0;
							pid_table[pids_found].national_subset = GetNationalSubset(country_code);
						}
						else
						{
							country_code[0] = 0;
							pid_table[pids_found].national_subset = -1;
						}

						if (pid_table[pids_found].vtxt_pid == vtxtpid)
							printf("TuxTxt <Country code \"%s\" national subset %d>\n",
									 country_code,
									 pid_table[pids_found].national_subset);

						pids_found++;
skip_pid:
					;
					}
				}
			}
		}
	}

	/* check for teletext */
	if (pids_found == 0)
	{
		printf("TuxTxt <no Teletext on TS found>\n");

		RenderMessage(NoServicesFound);
		sleep(3);
		return 0;
	}

	/* read SDT to get servicenames */
	SDT_ready = 0;

	dmx_flt.pid              = 0x0011;
	dmx_flt.flags            = DMX_CHECK_CRC | DMX_IMMEDIATE_START;
	dmx_flt.filter.filter[0] = 0x42;
	dmx_flt.filter.mask[0]   = 0xFF;
	dmx_flt.timeout          = 5000;

	if (ioctl(dmx, DMX_SET_FILTER, &dmx_flt) == -1)
	{
		perror("TuxTxt <DMX_SET_FILTER SDT>");

		RenderMessage(ShowServiceName);

		return 1;
	}

	first_sdt_sec = -1;
	while (1)
	{
		if (read(dmx, SDT, 3) == -1)
		{
			perror("TuxTxt <read SDT>");

			ioctl(dmx, DMX_STOP);
			RenderMessage(ShowServiceName);
			return 1;
		}

		if (read(dmx, SDT+3, ((SDT[1] & 0x0f) << 8) | SDT[2]) == -1)
		{
			perror("TuxTxt <read SDT>");

			ioctl(dmx, DMX_STOP);
			RenderMessage(ShowServiceName);
			return 1;
		}

		if (first_sdt_sec == SDT[6])
			break;

		if (first_sdt_sec == -1)
			first_sdt_sec = SDT[6];

		/* scan SDT to get servicenames */
		for (sdt_scan = 0x0B; sdt_scan < ((SDT[1]<<8 | SDT[2]) & 0x0FFF) - 7; sdt_scan += 5 + ((SDT[sdt_scan + 3]<<8 | SDT[sdt_scan + 4]) & 0x0FFF))
		{
			for (pid_test = 0; pid_test < pids_found; pid_test++)
			{
				if ((SDT[sdt_scan]<<8 | SDT[sdt_scan + 1]) == pid_table[pid_test].service_id && SDT[sdt_scan + 5] == 0x48)
				{
					diff = 0;
					pid_table[pid_test].service_name_len = SDT[sdt_scan+9 + SDT[sdt_scan+8]];

					for (byte = 0; byte < pid_table[pid_test].service_name_len; byte++)
					{
						if (SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] == (unsigned char)'Ä')
							SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] = 0x5B;
						if (SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] == (unsigned char)'ä')
							SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] = 0x7B;
						if (SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] == (unsigned char)'Ö')
							SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] = 0x5C;
						if (SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] == (unsigned char)'ö')
							SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] = 0x7C;
						if (SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] == (unsigned char)'Ü')
							SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] = 0x5D;
						if (SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] == (unsigned char)'ü')
							SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] = 0x7D;
						if (SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] == (unsigned char)'ß')
							SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] = 0x7E;
						if (SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] >= 0x80 && SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] <= 0x9F)
							diff--;
						else
							pid_table[pid_test].service_name[byte + diff] = SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte];
					}

					pid_table[pid_test].service_name_len += diff;
				}
			}
		}
	}
	ioctl(dmx, DMX_STOP);
	SDT_ready = 1;

	/* show current servicename */
	current_service = 0;

	if (vtxtpid != 0)
	{
		while (pid_table[current_service].vtxt_pid != vtxtpid && current_service < pids_found)
			current_service++;

		current_national_subset = pid_table[current_service].national_subset;
		RenderMessage(ShowServiceName);
	}

	if (!getpidsdone)							 /* call not from Init */
	{
		/* restore filter & start demuxer */
		dmx_pes_flt.pid      = vtxtpid;
		dmx_pes_flt.input    = DMX_IN_FRONTEND;
		dmx_pes_flt.output   = DMX_OUT_TAP;
		dmx_pes_flt.pes_type = DMX_PES_OTHER;
		dmx_pes_flt.flags    = DMX_IMMEDIATE_START;

		if (ioctl(dmx, DMX_SET_PES_FILTER, &dmx_pes_flt) == -1)
		{
			perror("TuxTxt <DMX_SET_PES_FILTER>");
			return 0;
		}

		/* start new decode-thread */
		if (pthread_create(&thread_id, NULL, CacheThread, NULL) != 0)
			perror("TuxTxt <pthread_create>");
	}
	getpidsdone = 1;

	RenderCharLCD(pids_found/10,  7, 44);
	RenderCharLCD(pids_found%10, 19, 44);

	return 1;
}

/******************************************************************************
 * GetNationalSubset                                                          *
 ******************************************************************************/

int GetNationalSubset(char *cc)
{
	if (memcmp(cc, "cze", 3) == 0 || memcmp(cc, "ces", 3) == 0 ||
	    memcmp(cc, "slo", 3) == 0 || memcmp(cc, "slk", 3) == 0)
		return 0;
	if (memcmp(cc, "eng", 3) == 0)
		return 1;
	if (memcmp(cc, "est", 3) == 0)
		return 2;
	if (memcmp(cc, "fre", 3) == 0 || memcmp(cc, "fra", 3) == 0)
		return 3;
	if (memcmp(cc, "ger", 3) == 0 || memcmp(cc, "deu", 3) == 0)
		return 4;
	if (memcmp(cc, "ita", 3) == 0)
		return 5;
	if (memcmp(cc, "lav", 3) == 0 || memcmp(cc, "lit", 3) == 0)
		return 6;
	if (memcmp(cc, "pol", 3) == 0)
		return 7;
	if (memcmp(cc, "spa", 3) == 0 || memcmp(cc, "por", 3) == 0)
		return 8;
	if (memcmp(cc, "rum", 3) == 0 || memcmp(cc, "ron", 3) == 0)
		return 9;
	if (memcmp(cc, "scc", 3) == 0 || memcmp(cc, "srp", 3) == 0 ||
	    memcmp(cc, "scr", 3) == 0 || memcmp(cc, "hrv", 3) == 0 ||
	    memcmp(cc, "slv", 3) == 0)
		return 10;
	if (memcmp(cc, "swe", 3) == 0 ||
	    memcmp(cc, "dan", 3) == 0 ||
	    memcmp(cc, "nor", 3) == 0 ||
	    memcmp(cc, "fin", 3) == 0 ||
	    memcmp(cc, "hun", 3) == 0)
		return 11;
	if (memcmp(cc, "tur", 3) == 0)
		return 12;

	return -1;
}

/******************************************************************************
 * ConfigMenu                                                                 *
 ******************************************************************************/

void Menu_HighlightLine(char *menu, int line, int high)
{
	char hilitline[] = "ZXXXXXXXXXXXXXXXXXXXXXXXXXXXXZ›";
	int itext = 2*Menu_Width*line; /* index start menuline */
	int byte;

	PosX = Menu_StartX;
	PosY = Menu_StartY + line*fontheight;

	for (byte = 0; byte < Menu_Width; byte++)
		RenderCharFB(menu[itext + byte], (high ? hilitline[byte] : menu[itext + byte+Menu_Width]));
}

void Menu_UpdateHotlist(char *menu, int hotindex, int menuitem)
{
	int i, j, k, attr;

	PosX = Menu_StartX + 6*fontwidth;
	PosY = Menu_StartY + (MenuLine[M_HOT]+1)*fontheight;
	j = 2*Menu_Width*(MenuLine[M_HOT]+1) + 6; /* start index in menu */

	for (i = 0; i <= maxhotlist+1; i++)
	{
		if (i == maxhotlist+1) /* clear last+1 entry in case it was deleted */
		{
			attr = 'È';
			memset(&menu[j], ' ', 3);
		}
		else
		{
			if (i == hotindex)
			{
				attr = '¤';
			}
			else
			{
				attr = 'È';
			}
			hex2str(&menu[j+2], hotlist[i]);
		}

		for (k = 0; k < 3; k++)
			RenderCharFB(menu[j+k], attr);

		if (i == 4)
		{
			PosX = Menu_StartX + 6*fontwidth;
			PosY += fontheight;
			j += 2*Menu_Width - 4*4;
		}
		else
		{
			j += 4; /* one space distance */
			PosX += fontwidth;
		}
	}

	hex2str(&menu[2*Menu_Width*MenuLine[M_HOT] + hotlistpagecolumn[menulanguage]], (hotindex >= 0) ? hotlist[hotindex] : page);
	memcpy(&menu[2*Menu_Width*MenuLine[M_HOT] + hotlisttextcolumn[menulanguage]], &hotlisttext[menulanguage][(hotindex >= 0) ? 5 : 0], 5);
	PosX = Menu_StartX + 20*fontwidth;
	PosY = Menu_StartY + MenuLine[M_HOT]*fontheight;

	Menu_HighlightLine(menu, MenuLine[M_HOT], (menuitem == M_HOT) ? 1 : 0);
}

void Menu_Init(char *menu, int current_pid, int menuitem, int hotindex)
{
	int byte, line;

	memcpy(menu, configmenu[menulanguage], 2*Menu_Height*Menu_Width);
	
	if (SDT_ready)
		memcpy(&menu[MenuLine[M_PID]*2*Menu_Width+3+(24-pid_table[current_pid].service_name_len)/2], &pid_table[current_pid].service_name, pid_table[current_pid].service_name_len);
	else
		hex2str(&menu[MenuLine[M_PID]*2*Menu_Width + 13 + 3], vtxtpid);

	if (current_pid == 0 || pids_found == 1)
		menu[MenuLine[M_PID]*2*Menu_Width +  1] = ' ';

	if (current_pid == pids_found - 1 || pids_found == 1)
		menu[MenuLine[M_PID]*2*Menu_Width + 28] = ' ';

	/* set 16:9 modi, colors & national subset */
	memcpy(&menu[2*Menu_Width*MenuLine[M_SC1] + Menu_Width - 5], &configonoff[menulanguage][screen_mode1  ? 3 : 0], 3);
	memcpy(&menu[2*Menu_Width*MenuLine[M_SC2] + Menu_Width - 5], &configonoff[menulanguage][screen_mode2  ? 3 : 0], 3);
	memcpy(&menu[2*Menu_Width*MenuLine[M_COL] + Menu_Width - 5], &configonoff[menulanguage][color_mode    ? 3 : 0], 3);
	memcpy(&menu[2*Menu_Width*MenuLine[M_AUN] + Menu_Width - 5], &configonoff[menulanguage][auto_national ? 3 : 0], 3);
	if (national_subset != 4)
		memcpy(&menu[2*Menu_Width*MenuLine[M_NAT] + 2], &countrystring[national_subset*26], 26);
	if (national_subset == 0  || auto_national)
		menu[MenuLine[M_NAT]*2*Menu_Width +  1] = ' ';
	if (national_subset == 12 || auto_national)
		menu[MenuLine[M_NAT]*2*Menu_Width + 28] = ' ';
	if (showhex)
		menu[MenuLine[M_PID]*2*Menu_Width + 27] = '?';

	/* render menu */
	PosY = Menu_StartY;
	for (line = 0; line < Menu_Height; line++)
	{
		PosX = Menu_StartX;

		for (byte = 0; byte < Menu_Width; byte++)
			RenderCharFB(menu[line*2*Menu_Width + byte], menu[line*2*Menu_Width + byte+Menu_Width]);

		PosY += fontheight;
	}
	Menu_HighlightLine(menu, MenuLine[menuitem], 1);
	Menu_UpdateHotlist(menu, hotindex, menuitem);
}

void ConfigMenu(int Init)
{
	struct dmx_pes_filter_params dmx_flt;
	int val, menuitem = M_Start;
	int current_pid = 0;
	int hotindex;
	int oldscreenmode;
	char menu[2*Menu_Height*Menu_Width];

	if (!getpidsdone)
		GetTeletextPIDs();

	/* set current vtxt */
	if (vtxtpid == 0)
		vtxtpid = pid_table[0].vtxt_pid;
	else
		while(pid_table[current_pid].vtxt_pid != vtxtpid && current_pid < pids_found)
			current_pid++;

	/* reset to normal mode */
	if (zoommode)
		zoommode = 0;

	if (transpmode)
	{
		transpmode = 0;
		ClearBB(black);
	}

	oldscreenmode = screenmode;
	if (screenmode)
		SwitchScreenMode(0); /* turn off divided screen */

	hotindex = getIndexOfPageInHotlist();

	/* clear framebuffer */
	ClearFB(transp);
	clearbbcolor = black;
	Menu_Init(menu, current_pid, menuitem, hotindex);
	
	/* set blocking mode */
	val = fcntl(rc, F_GETFL);
	fcntl(rc, F_SETFL, val &~ O_NONBLOCK);

	/* loop */
	do {
		if (GetRCCode() == 1)
		{

			if (
#if (RC_1 > 0)
				RCCode >= RC_1 && /* generates a warning... */
#endif
				RCCode <= RC_1+M_MaxDirect) /* direct access */
			{
				Menu_HighlightLine(menu, MenuLine[menuitem], 0);
				menuitem = RCCode-RC_1;
				Menu_HighlightLine(menu, MenuLine[menuitem], 1);

				if (menuitem != M_PID) /* just select */
					RCCode = RC_OK;
			}

			switch (RCCode)
			{
			case RC_UP:
				Menu_HighlightLine(menu, MenuLine[menuitem], 0);
				if (--menuitem < 0)
					menuitem = M_Number-1;
				if (auto_national && (menuitem == M_NAT))
					menuitem--;
				Menu_HighlightLine(menu, MenuLine[menuitem], 1);
				break;

			case RC_DOWN:
				Menu_HighlightLine(menu, MenuLine[menuitem], 0);
				if (++menuitem > M_Number-1)
					menuitem = 0;
				if (auto_national && (menuitem == M_NAT))
					menuitem++;
				Menu_HighlightLine(menu, MenuLine[menuitem], 1);
				break;

			case RC_LEFT:
				switch (menuitem)
				{
				case M_PID:
				{
					if (current_pid > 0)
					{
						current_pid--;

						memset(&menu[MenuLine[M_PID]*2*Menu_Width + 3], ' ', 24);

						if (SDT_ready)
						{
							memcpy(&menu[MenuLine[M_PID]*2*Menu_Width+3+(24-pid_table[current_pid].service_name_len)/2],
							       &pid_table[current_pid].service_name,
							       pid_table[current_pid].service_name_len);
						}
						else
							hex2str(&menu[MenuLine[M_PID]*2*Menu_Width + 13 + 3], vtxtpid);

						if (pids_found > 1)
						{
							if (current_pid == 0)
							{
								menu[MenuLine[M_PID]*2*Menu_Width +  1] = ' ';
								menu[MenuLine[M_PID]*2*Menu_Width + 28] = 'î';
							}
							else
							{
								menu[MenuLine[M_PID]*2*Menu_Width +  1] = 'í';
								menu[MenuLine[M_PID]*2*Menu_Width + 28] = 'î';
							}
						}

						Menu_HighlightLine(menu, MenuLine[menuitem], 1);

						if (auto_national)
						{
							national_subset = pid_table[current_pid].national_subset;

							memcpy(&menu[2*Menu_Width*MenuLine[M_NAT] + 2], &countrystring[national_subset*26], 26);
							Menu_HighlightLine(menu, MenuLine[M_NAT], 0);
						}
					}
					break;
				}

				case M_NAT:
					saveconfig = 1;
					if (national_subset > 0)
					{
						national_subset--;

						if (national_subset == 0)
						{
							menu[MenuLine[M_NAT]*2*Menu_Width +  1] = ' ';
							menu[MenuLine[M_NAT]*2*Menu_Width + 28] = 'î';
						}
						else
						{
							menu[MenuLine[M_NAT]*2*Menu_Width +  1] = 'í';
							menu[MenuLine[M_NAT]*2*Menu_Width + 28] = 'î';
						}

						Menu_Init(menu, current_pid, menuitem, hotindex);
					}
					break;

				case M_HOT: /* move towards top of hotlist */
					if (hotindex <= 0) /* if not found, start at end */
						hotindex = maxhotlist;
					else
						hotindex--;
					Menu_UpdateHotlist(menu, hotindex, menuitem);
					break;

				case M_LNG:
					saveconfig = 1;
					if (--menulanguage < 0)
						menulanguage = MAXMENULANGUAGE;
					Menu_Init(menu, current_pid, menuitem, hotindex);
					break;
				} /* switch menuitem */
				break; /* RC_LEFT */

			case RC_RIGHT:
				switch (menuitem)
				{
				case M_PID:
					if (current_pid < pids_found - 1)
					{
						current_pid++;

						memset(&menu[MenuLine[M_PID]*2*Menu_Width + 3], ' ', 24);

						if (SDT_ready)
							memcpy(&menu[MenuLine[M_PID]*2*Menu_Width + 3 +
											 (24-pid_table[current_pid].service_name_len)/2],
									 &pid_table[current_pid].service_name,
									 pid_table[current_pid].service_name_len);
						else
							hex2str(&menu[MenuLine[M_PID]*2*Menu_Width + 13 + 3], pid_table[current_pid].vtxt_pid);

						if (pids_found > 1)
						{
							if (current_pid == pids_found - 1)
							{
								menu[MenuLine[M_PID]*2*Menu_Width +  1] = 'í';
								menu[MenuLine[M_PID]*2*Menu_Width + 28] = ' ';
							}
							else
							{
								menu[MenuLine[M_PID]*2*Menu_Width +  1] = 'í';
								menu[MenuLine[M_PID]*2*Menu_Width + 28] = 'î';
							}
						}

						Menu_HighlightLine(menu, MenuLine[menuitem], 1);

						if (auto_national)
						{
							national_subset = pid_table[current_pid].national_subset;
							memcpy(&menu[2*Menu_Width*MenuLine[M_NAT] + 2], &countrystring[national_subset*26], 26);
							Menu_HighlightLine(menu, MenuLine[M_NAT], 0);
						}
					}
					break;

				case M_NAT:
					saveconfig = 1;
					if (national_subset < 12)
					{
						national_subset++;

						if (national_subset == 12)
						{
							menu[MenuLine[M_NAT]*2*Menu_Width +  1] = 'í';
							menu[MenuLine[M_NAT]*2*Menu_Width + 28] = ' ';
						}
						else
						{
							menu[MenuLine[M_NAT]*2*Menu_Width +  1] = 'í';
							menu[MenuLine[M_NAT]*2*Menu_Width + 28] = 'î';
						}

						Menu_Init(menu, current_pid, menuitem, hotindex);
					}
					break;

				case M_HOT: /* select hotindex */
					if ((unsigned int)hotindex >= maxhotlist) /* if not found, start at 0 */
						hotindex = 0;
					else
						hotindex++;
					Menu_UpdateHotlist(menu, hotindex, menuitem);
					break;

				case M_LNG:
					saveconfig = 1;
					if (++menulanguage > MAXMENULANGUAGE)
						menulanguage = 0;
					Menu_Init(menu, current_pid, menuitem, hotindex);
					break;
				}
				break; /* RC_RIGHT */

			case RC_PLUS:
				switch (menuitem)
				{
				case M_HOT: /* move towards end of hotlist */
				{
					if (hotindex<0) /* not found: add page at end */
					{
						if (maxhotlist < (sizeof(hotlist)/sizeof(hotlist[0])-1)) /* only if still room left */
						{
							hotindex = ++maxhotlist;
							hotlist[hotindex] = page;
							hotlistchanged = 1;
							Menu_UpdateHotlist(menu, hotindex, menuitem);
						}
					}
					else /* found */
					{
						if (hotindex < maxhotlist) /* not already at end */
						{
							int temp = hotlist[hotindex];
							hotlist[hotindex] = hotlist[hotindex+1];
							hotlist[hotindex+1] = temp;
							hotindex++;
							hotlistchanged = 1;
							Menu_UpdateHotlist(menu, hotindex, menuitem);
						}
					}
				}
				break;
				}
				break; /* RC_PLUS */

			case RC_MINUS:
				switch (menuitem)
				{
				case M_HOT: /* move towards top of hotlist */
				{
					if (hotindex<0) /* not found: add page at top */
					{
						if (maxhotlist < (sizeof(hotlist)/sizeof(hotlist[0])-1)) /* only if still room left */
						{
							for (hotindex = maxhotlist; hotindex >= 0; hotindex--) /* move rest of list */
							{
								hotlist[hotindex+1] = hotlist[hotindex];
							}
							maxhotlist++;
							hotindex = 0;
							hotlist[hotindex] = page;
							hotlistchanged = 1;
							Menu_UpdateHotlist(menu, hotindex, menuitem);
						}
					}
					else /* found */
					{
						if (hotindex > 0) /* not already at front */
						{
							int temp = hotlist[hotindex];
							hotlist[hotindex] = hotlist[hotindex-1];
							hotlist[hotindex-1] = temp;
							hotindex--;
							hotlistchanged = 1;
							Menu_UpdateHotlist(menu, hotindex, menuitem);
						}
					}
				}
				break;
				}
				break; /* RC_MINUS */

			case RC_HELP:
				switch (menuitem)
				{
				case M_HOT: /* current page is added to / removed from hotlist */
				{
					if (hotindex<0) /* not found: add page */
					{
						if (maxhotlist < (sizeof(hotlist)/sizeof(hotlist[0])-1)) /* only if still room left */
						{
							hotlist[++maxhotlist] = page;
							hotindex = maxhotlist;
							hotlistchanged = 1;
							Menu_UpdateHotlist(menu, hotindex, menuitem);
						}
					}
					else /* found: remove */
					{
						if (maxhotlist > 0) /* don't empty completely */
						{
							int i;

							for (i=hotindex; i<maxhotlist; i++) /* move rest of list */
							{
								hotlist[i] = hotlist[i+1];
							}
							maxhotlist--;
							if (hotindex > maxhotlist)
								hotindex = maxhotlist;
							hotlistchanged = 1;
							Menu_UpdateHotlist(menu, hotindex, menuitem);
						}
					}
				}
				break;
				case M_PID:
					showhex ^= 1;
					menu[MenuLine[M_PID]*2*Menu_Width + 27] = (showhex ? '?' : ' ');
					Menu_HighlightLine(menu, MenuLine[menuitem], 1);
				break;
				}
				break; /* RC_MUTE */

			case RC_OK:
				switch (menuitem)
				{
				case M_PID:
					if (pids_found > 1)
					{
						if (Init)
							vtxtpid = pid_table[current_pid].vtxt_pid;
						else
						{
							unsigned char magazine;

							if (hotlistchanged)
								savehotlist();

							/* stop old decode-thread */
							if (pthread_cancel(thread_id) != 0)
								perror("TuxTxt <pthread_cancel>");

							if (pthread_join(thread_id, &thread_result) != 0)
								perror("TuxTxt <pthread_join>");

							/* stop demuxer */
							ioctl(dmx, DMX_STOP);

							/* reset data */
							memset(&subpagetable, 0xFF, sizeof(subpagetable));
							memset(&countrycontrolbitstable, 0xFF, sizeof(countrycontrolbitstable));

							memset(&basictop, 0, sizeof(basictop));
							memset(&adip, 0, sizeof(adip));
							maxadippg = -1;
							bttok = 0;

							page_atrb[32] = transp<<4 | transp;
							inputcounter = 2;
							cached_pages = 0;

							for (magazine = 1; magazine < 9; magazine++)
							{
								current_page   [magazine] = -1;
								current_subpage [magazine] = -1;
							}
							page_receiving = -1;

							page     = 0x100;
							lastpage = 0x100;
							prev_100 = 0x100;
							prev_10  = 0x100;
							next_100 = 0x100;
							next_10  = 0x100;
							subpage  = 0;

							pageupdate = 0;
							zap_subpage_manual = 0;
							hintmode = 0;

							/* free pagebuffers */
							for (clear_page = 0; clear_page < 0x8FF; clear_page++)
							{
								for (clear_subpage = 0; clear_subpage < 0x79; clear_subpage++)
								{
									if (cachetable[clear_page][clear_subpage] != 0);
									{
										free(cachetable[clear_page][clear_subpage]);
										cachetable[clear_page][clear_subpage] = 0;
									}
								}
							}

							/* start demuxer with new vtxtpid */
							vtxtpid = pid_table[current_pid].vtxt_pid;
							current_national_subset = pid_table[current_pid].national_subset;

							dmx_flt.pid      = vtxtpid;
							dmx_flt.input    = DMX_IN_FRONTEND;
							dmx_flt.output   = DMX_OUT_TAP;
							dmx_flt.pes_type = DMX_PES_OTHER;
							dmx_flt.flags    = DMX_IMMEDIATE_START;

							if (ioctl(dmx, DMX_SET_PES_FILTER, &dmx_flt) == -1)
								perror("TuxTxt <DMX_SET_PES_FILTER>");

							/* start new decode-thread */
							if (pthread_create(&thread_id, NULL, CacheThread, NULL) != 0)
								perror("TuxTxt <pthread_create>");

							ClearBB(black);
							pageupdate = 1;
							gethotlist();
						}

						/* show new teletext */
						current_service = current_pid;
						RenderMessage(ShowServiceName);

						fcntl(rc, F_SETFL, O_NONBLOCK);
						RCCode = -1;
						return;
					}
					break;

				case M_SC1:
					saveconfig = 1;
					screen_mode1++;
					screen_mode1 &= 1;

					memcpy(&menu[2*Menu_Width*MenuLine[M_SC1] + Menu_Width - 5], &configonoff[menulanguage][screen_mode1  ? 3 : 0], 3);
					Menu_HighlightLine(menu, MenuLine[menuitem], 1);

					ioctl(avs, AVSIOSSCARTPIN8, &fncmodes[screen_mode1]);
					ioctl(saa, SAAIOSWSS, &saamodes[screen_mode1]);

					break;

				case M_SC2:
					saveconfig = 1;
					screen_mode2++;
					screen_mode2 &= 1;

					memcpy(&menu[2*Menu_Width*MenuLine[M_SC2] + Menu_Width - 5], &configonoff[menulanguage][screen_mode2  ? 3 : 0], 3);
					Menu_HighlightLine(menu, MenuLine[menuitem], 1);
					break;

				case M_COL:
					saveconfig = 1;
					color_mode++;
					color_mode &= 1;

					memcpy(&menu[2*Menu_Width*MenuLine[M_COL] + Menu_Width - 5], &configonoff[menulanguage][color_mode    ? 3 : 0], 3);
					Menu_HighlightLine(menu, MenuLine[menuitem], 1);

					if (color_mode)
					{
						if (ioctl(fb, FBIOPUTCMAP, &colormap_2) == -1)
							perror("TuxTxt <FBIOPUTCMAP>");
					}
					else
					{
						if (ioctl(fb, FBIOPUTCMAP, &colormap_1) == -1)
							perror("TuxTxt <FBIOPUTCMAP>");
					}
					break;

				case M_AUN:
					saveconfig = 1;
					auto_national++;
					auto_national &= 1;
					if (auto_national)
						national_subset = pid_table[current_pid].national_subset;
					Menu_Init(menu, current_pid, menuitem, hotindex);
					break;
				case M_HOT: /* show selected page */
				{
					if (hotindex >= 0) /* not found: ignore */
					{
						lastpage = page;
						page = hotlist[hotindex];
						subpage = subpagetable[page];
						inputcounter = 2;
						pageupdate = 1;
						RCCode = RC_HOME;		 /* leave menu */
					}
				}
				break;
				} /* RC_OK */
				break;
			}
		}
		UpdateLCD(); /* update number of cached pages */
	} while ((RCCode != RC_HOME) && (RCCode != RC_DBOX) && (RCCode != RC_MUTE));

	/* reset to nonblocking mode */
	fcntl(rc, F_SETFL, O_NONBLOCK);
	pageupdate = 1;
	RCCode = -1;
	if (oldscreenmode)
		SwitchScreenMode(oldscreenmode); /* restore divided screen */
}

/******************************************************************************
 * PageInput                                                                  *
 ******************************************************************************/

void PageInput(int Number)
{
	int zoom = 0;

	/* clear temp_page */
	if (inputcounter == 2)
		temp_page = 0;

	/* check for 0 & 9 on first position */
	if (Number == 0 && inputcounter == 2)
	{
		/* set page */
		temp_page = lastpage; /* 0 toggles to last page as in program switching */
		inputcounter = -1;
	}
	else if (Number == 9 && inputcounter == 2)
	{
		/* set page */
		temp_page = getIndexOfPageInHotlist(); /* 9 toggles through hotlist */

		if (temp_page<0 || temp_page==maxhotlist) /* from any (other) page go to first page in hotlist */
			temp_page = (maxhotlist >= 0) ? hotlist[0] : 0x100;
		else
			temp_page = hotlist[temp_page+1];

		inputcounter = -1;
	}

	/* show pageinput */
	if (zoommode == 2)
	{
		zoommode = 1;
		CopyBB2FB();
	}

	if (zoommode == 1)
		zoom = 1<<10;

	PosY = StartY;

	switch (inputcounter)
	{
	case 2:
		SetPosX(1);
		RenderCharFB(Number | '0', black<<4 | white);
		RenderCharFB('-', black<<4 | white);
		RenderCharFB('-', black<<4 | white);
		break;

	case 1:
		SetPosX(2);
		RenderCharFB(Number | '0', black<<4 | white);
		break;

	case 0:
		SetPosX(3);
		RenderCharFB(Number | '0', black<<4 | white);
		break;
	}

	/* generate pagenumber */
	temp_page |= Number << inputcounter*4;

	inputcounter--;

	if (inputcounter < 0)
	{
		/* disable subpage zapping */
		zap_subpage_manual = 0;

		/* reset input */
		inputcounter = 2;

		/* set new page */
		lastpage = page;

		page = temp_page;

		/* check cache */
		if (subpagetable[page] != 0xFF)
		{
			subpage = subpagetable[page];
			pageupdate = 1;
#if DEBUG
			printf("TuxTxt <DirectInput: %.3X-%.2X>\n", page, subpage);
#endif
		}
		else
		{
			subpage = 0;
			RenderMessage(PageNotFound);
#if DEBUG
			printf("TuxTxt <DirectInput: %.3X not found>\n", page);
#endif
		}
	}
}

/******************************************************************************
 * GetNextPageOne                                                             *
 ******************************************************************************/

void GetNextPageOne()
{
	/* disable subpage zapping */
	zap_subpage_manual = 0;

	/* abort pageinput */
	inputcounter = 2;

	/* find next cached page */
	lastpage = page;

	do {
		next_dec(&page);
	} while (subpagetable[page] == 0xFF && page != lastpage);

	/* update page */
	if (page != lastpage)
	{
		if (zoommode == 2)
			zoommode = 1;

		subpage = subpagetable[page];
		pageupdate = 1;
#if DEBUG
		printf("TuxTxt <NextPageOne: %.3X-%.2X>\n", page, subpage);
#endif
	}
}

/******************************************************************************
 * GetPrevPageOne                                                             *
 ******************************************************************************/

void GetPrevPageOne()
{
	/* disable subpage zapping */
	zap_subpage_manual = 0;

	 /* abort pageinput */
	inputcounter = 2;

	/* find previous cached page */
	lastpage = page;

	do {
		prev_dec(&page);
	} while (subpagetable[page] == 0xFF && page != lastpage);

	/* update page */
	if (page != lastpage)
	{
		if (zoommode == 2)
			zoommode = 1;

		subpage = subpagetable[page];
		pageupdate = 1;
#if DEBUG
		printf("TuxTxt <PrevPageOne: %.3X-%.2X>\n", page, subpage);
#endif
	}
}

/******************************************************************************
 * GetNextSubPage                                                             *
 ******************************************************************************/
void GetNextSubPage()
{
	int loop;

	/* abort pageinput */
	inputcounter = 2;

	/* search subpage */
	if (subpage != 0)
	{
		/* search next subpage */
		for (loop = subpage + 1; loop <= 0x79; loop++)
		{
			if (cachetable[page][loop] != 0)
			{
				/* enable manual subpage zapping */
				zap_subpage_manual = 1;

				/* update page */
				if (zoommode == 2)
					zoommode = 1;

				subpage = loop;
				pageupdate = 1;
#if DEBUG
				printf("TuxTxt <NextSubPage: %.3X-%.2X>\n", page, subpage);
#endif
				return;
			}
		}

		for (loop = 1; loop < subpage; loop++)
		{
			if (cachetable[page][loop] != 0)
			{
				/* enable manual subpage zapping */
				zap_subpage_manual = 1;

				/* update page */
				if (zoommode == 2)
					zoommode = 1;

				subpage = loop;
				pageupdate = 1;
#if DEBUG
				printf("TuxTxt <NextSubPage: %.3X-%.2X>\n", page, subpage);
#endif
				return;
			}
		}

#if DEBUG
		printf("TuxTxt <NextSubPage: no other SubPage>\n");
#endif
	}
	else
	{
#if DEBUG
		printf("TuxTxt <NextSubPage: no SubPages>\n");
#endif
	}
}

/******************************************************************************
 * GetPrevSubPage                                                             *
 ******************************************************************************/

void GetPrevSubPage()
{
	int loop;

	/* abort pageinput */
	inputcounter = 2;

	/* search subpage */
	if (subpage != 0)
	{
		/* search previous subpage */
		for (loop = subpage - 1; loop > 0x00; loop--)
		{
			if (cachetable[page][loop] != 0)
			{
				/* enable manual subpage zapping */
				zap_subpage_manual = 1;

				/* update page */
				if (zoommode == 2)
					zoommode = 1;

				subpage = loop;
				pageupdate = 1;
#if DEBUG
				printf("TuxTxt <PrevSubPage: %.3X-%.2X>\n", page, subpage);
#endif
				return;
			}
		}

		for (loop = 0x79; loop > subpage; loop--)
		{
			if (cachetable[page][loop] != 0)
			{
				/* enable manual subpage zapping */
				zap_subpage_manual = 1;

				/* update page */
				if (zoommode == 2)
					zoommode = 1;

				subpage = loop;
				pageupdate = 1;
#if DEBUG
				printf("TuxTxt <PrevSubPage: %.3X-%.2X>\n", page, subpage);
#endif
				return;
			}
		}

#if DEBUG
		printf("TuxTxt <PrevSubPage: no other SubPage>\n");
#endif
	}
	else
	{
#if DEBUG
		printf("TuxTxt <PrevSubPage: no SubPages>\n");
#endif
	}
}

/******************************************************************************
 * Prev100                                                                    *
 ******************************************************************************/

void Prev100()
{
	if (prev_100 == 0) return;
	if (zoommode == 2)
		zoommode = 1;

	lastpage     = page;
	page         = prev_100;
	subpage      = subpagetable[page];
	inputcounter = 2;
	pageupdate   = 1;

#if DEBUG
	printf("TuxTxt <Prev100: %.3X>\n", page);
#endif
}

/******************************************************************************
 * Prev10                                                                     *
 ******************************************************************************/

void Prev10()
{
	if (prev_10 == 0) return;
	if (zoommode == 2)
		zoommode = 1;

	lastpage     = page;
	page         = prev_10;
	subpage      = subpagetable[page];
	inputcounter = 2;
	pageupdate   = 1;

#if DEBUG
	printf("TuxTxt <Prev10: %.3X>\n", page);
#endif
}

/******************************************************************************
 * Next10                                                                    *
 ******************************************************************************/

void Next10()
{
	if (next_10 == 0) return;
	if (zoommode == 2)
		zoommode = 1;

	lastpage     = page;
	page         = next_10;
	subpage      = subpagetable[page];
	inputcounter = 2;
	pageupdate   = 1;

#if DEBUG
	printf("TuxTxt <Next10: %.3X>\n", page);
#endif
}

/******************************************************************************
 * Next100                                                                    *
 ******************************************************************************/

void Next100()
{
	if (next_100 == 0) return;
	if (zoommode == 2)
		zoommode = 1;

	lastpage     = page;
	page         = next_100;
	subpage      = subpagetable[page];
	inputcounter = 2;
	pageupdate   = 1;

#if DEBUG
	printf("TuxTxt <Next100: %.3X>\n", page);
#endif
}

/******************************************************************************
 * PageCatching                                                               *
 ******************************************************************************/

void PageCatching()
{
	int val, byte;
	int oldzoommode = zoommode;

	pagecatching = 1;

	/* abort pageinput */
	inputcounter = 2;

	/* show info line */
	zoommode = 0;
	PosX = StartX;
	PosY = StartY + 24*fontheight;
	for (byte = 0; byte < 40-nofirst; byte++)
		RenderCharFB(catchmenutext[menulanguage][byte], catchmenutext[menulanguage][byte+40]);
	zoommode = oldzoommode;

	/* check for pagenumber(s) */
	catch_row    = 1;
	catch_col    = 0;
	catched_page = 0;
	pc_old_row = pc_old_col = 0; /* no inverted page number to restore yet */
	CatchNextPage(0, 1);

	if (!catched_page)
	{
		pagecatching = 0;
		pageupdate = 1;
		return;
	}

	/* set blocking mode */
	val = fcntl(rc, F_GETFL);
	fcntl(rc, F_SETFL, val &~ O_NONBLOCK);

	/* loop */
	do {
		GetRCCode();

		switch (RCCode)
		{
		case RC_LEFT:
			CatchNextPage(0, -1);
			break;
		case RC_RIGHT:
			CatchNextPage(0, 1);
			break;
		case RC_UP:
			CatchNextPage(-1, -1);
			break;
		case RC_DOWN:
			CatchNextPage(1, 1);
			break;
		case RC_0:
		case RC_1:
		case RC_2:
		case RC_3:
		case RC_4:
		case RC_5:
		case RC_6:
		case RC_7:
		case RC_8:
		case RC_9:
		case RC_RED:
		case RC_GREEN:
		case RC_YELLOW:
		case RC_BLUE:
		case RC_PLUS:
		case RC_MINUS:
		case RC_DBOX:
		case RC_HOME:
		case RC_HELP:
		case RC_MUTE:
			fcntl(rc, F_SETFL, O_NONBLOCK);
			pageupdate = 1;
			pagecatching = 0;
			RCCode = -1;
			return;
		}
		UpdateLCD();
	} while (RCCode != RC_OK);

	/* set new page */
	if (zoommode == 2)
		zoommode = 1;

	lastpage     = page;
	page         = catched_page;
	pageupdate   = 1;
	pagecatching = 0;

	if (subpagetable[page] != 0xFF)
		subpage = subpagetable[page];
	else
		subpage = 0;

	/* reset to nonblocking mode */
	fcntl(rc, F_SETFL, O_NONBLOCK);
}

/******************************************************************************
 * CatchNextPage                                                              *
 ******************************************************************************/

void CatchNextPage(int firstlineinc, int inc)
{
	int tmp_page, allowwrap = 1; /* allow first wrap around */

	/* catch next page */
	for(;;)
	{
		unsigned char *p = &(page_char[catch_row*40 + catch_col]);
		int a = page_atrb[catch_row*40 + catch_col];
		
		if (!(a & 1<<8) && /* no mosaic */
			 (((a & 0xF0) >> 4) != (a & 0x0F)) && /* not hidden */
			 (*p >= '1' && *p <= '8' && /* valid page number */
			  *(p+1) >= '0' && *(p+1) <= '9' &&
			  *(p+2) >= '0' && *(p+2) <= '9') &&
			 (catch_row == 0 || (*(p-1) < '0' || *(p-1) > '9')) && /* non-numeric char before and behind */
			 (catch_row == 37 || (*(p+3) < '0' || *(p+3) > '9')))
		{
			tmp_page = ((*p - '0')<<8) | ((*(p+1) - '0')<<4) | (*(p+2) - '0');

#if 0
			if (tmp_page != catched_page)	/* confusing to skip identical page numbers - I want to reach what I aim to */
#endif
			{
				catched_page = tmp_page;
				RenderCatchedPage();
				catch_col += inc;	/* FIXME: limit */
				printf("TuxTxt <PageCatching: %.3X\n", catched_page);
				return;
			}
		}

		if (firstlineinc > 0)
		{
			catch_row++;
			catch_col = 0;
			firstlineinc = 0;
		}
		else if (firstlineinc < 0)
		{
			catch_row--;
			catch_col = 37;
			firstlineinc = 0;
		}
		else
			catch_col += inc;
			
		if (catch_col > 37)
		{
			catch_row++;
			catch_col = 0;
		}
		else if (catch_col < 0)
		{
			catch_row--;
			catch_col = 37;
		}

		if (catch_row > 23)
		{
			if (allowwrap)
			{
				allowwrap = 0;
				catch_row = 1;
				catch_col = 0;
			}
			else
			{
				printf("TuxTxt <PageCatching: no PageNumber>\n");
				return;
			}
		}
		else if (catch_row < 1)
		{
			if (allowwrap)
			{
				allowwrap = 0;
				catch_row = 23;
				catch_col =37;
			}
			else
			{
				printf("TuxTxt <PageCatching: no PageNumber>\n");
				return;
			}
		}
	}
}


/******************************************************************************
 * RenderCatchedPage                                                          *
 ******************************************************************************/

void RenderCatchedPage()
{
	int zoom = 0;

	/* handle zoom */
	if (zoommode)
		zoom = 1<<10;

	if (pc_old_row || pc_old_col) /* not at first call */
	{
		/* restore pagenumber */
		SetPosX(pc_old_col);

		if (zoommode == 2)
			PosY = StartY + (pc_old_row-12)*fontheight*((zoom>>10)+1);
		else
			PosY = StartY + pc_old_row*fontheight*((zoom>>10)+1);

		RenderCharFB(page_char[pc_old_row*40 + pc_old_col    ], page_atrb[pc_old_row*40 + pc_old_col    ]);
		RenderCharFB(page_char[pc_old_row*40 + pc_old_col + 1], page_atrb[pc_old_row*40 + pc_old_col + 1]);
		RenderCharFB(page_char[pc_old_row*40 + pc_old_col + 2], page_atrb[pc_old_row*40 + pc_old_col + 2]);
	}

	pc_old_row = catch_row;
	pc_old_col = catch_col;

	/* mark pagenumber */
	if (zoommode == 1 && catch_row > 11)
	{
		zoommode = 2;
		CopyBB2FB();
	}
	else if (zoommode == 2 && catch_row < 12)
	{
		zoommode = 1;
		CopyBB2FB();
	}
	SetPosX(catch_col);


	if (zoommode == 2)
		PosY = StartY + (catch_row-12)*fontheight*((zoom>>10)+1);
	else
		PosY = StartY + catch_row*fontheight*((zoom>>10)+1);

	RenderCharFB(page_char[catch_row*40 + catch_col    ],
					 (page_atrb[catch_row*40 + catch_col    ] & 1<<10) |
					 ((page_atrb[catch_row*40 + catch_col    ] & 0x0F)<<4) |
					 ((page_atrb[catch_row*40 + catch_col    ] & 0xF0)>>4));
	RenderCharFB(page_char[catch_row*40 + catch_col + 1],
					 (page_atrb[catch_row*40 + catch_col + 1] & 1<<10) |
					 ((page_atrb[catch_row*40 + catch_col + 1] & 0x0F)<<4) |
					 ((page_atrb[catch_row*40 + catch_col + 1] & 0xF0)>>4));
	RenderCharFB(page_char[catch_row*40 + catch_col + 2],
					 (page_atrb[catch_row*40 + catch_col + 2] & 1<<10) |
					 ((page_atrb[catch_row*40 + catch_col + 2] & 0x0F)<<4) |
					 ((page_atrb[catch_row*40 + catch_col + 2] & 0xF0)>>4));
}

/******************************************************************************
 * SwitchZoomMode                                                             *
 ******************************************************************************/

void SwitchZoomMode()
{
	if (subpagetable[page] != 0xFF)
	{
		/* toggle mode */
		zoommode++;

		if (zoommode == 3)
			zoommode = 0;

		printf("TuxTxt <SwitchZoomMode: %d>\n", zoommode);

		/* update page */
		pageupdate = 1; /* FIXME */
	}
}

/******************************************************************************
 * SwitchScreenMode                                                           *
 ******************************************************************************/

void SwitchScreenMode(int newscreenmode)
{
#if HAVE_DVB_API_VERSION >= 3
	struct v4l2_format format;
#endif
	/* reset transparency mode */
	if (transpmode)
		transpmode = 0;

	if (newscreenmode < 0) /* toggle mode */
		screenmode++;
	else /* set directly */
		screenmode = newscreenmode;
	if ((screenmode > (screen_mode2 ? 2 : 1)) || (screenmode < 0))
		screenmode = 0;

	printf("TuxTxt <SwitchScreenMode: %d>\n", screenmode);

	/* update page */
	pageupdate = 1;

	/* clear back buffer */
#ifndef DREAMBOX
	clearbbcolor = black;
#else
	clearbbcolor = screenmode?transp:black;
#endif
	ClearBB(clearbbcolor);


	/* set mode */
	if (screenmode)								 /* split */
	{
#ifdef DREAMBOX
		if ( screenmode == 2 && zoommode )
			ClearFB(clearbbcolor);
#endif

		int fw, fh, tx, ty, tw, th;


		if (screenmode==1) /* split with topmenu */
		{
			fw = fontwidth_topmenumain;
			fh = fontheight;
			tw = TV43WIDTH;
#if CFGTTF 
			displaywidth= (TV43STARTX     -sx);
		StartX = sx; //+ (((ex-sx) - (40*fw+2+tw)) / 2); /* center screen */
#endif
			tx = TV43STARTX;
			ty = TV43STARTY;
			th = TV43HEIGHT;
		}
		else /* 2: split with full height tv picture */
		{
			fw = fontwidth_small;
			fh = fontheight;
			tx = TV169FULLSTARTX;
			ty = TV169FULLSTARTY;
			tw = TV169FULLWIDTH;
			th = TV169FULLHEIGHT;
#if CFGTTF
			displaywidth= (TV169FULLSTARTX-sx);
#endif
		}
		
#if CFGTTF 
		fontwidth = fw;
		typettf.font.pix_width  = (FT_UShort) fontwidth * TTFWIDTHFACTOR;
#else	 /* !TTF */
		type0.font.pix_width = type1.font.pix_width = type2.font.pix_width = fw;
		type0.font.pix_height = type1.font.pix_height = type2.font.pix_height = fh+1;
#endif	 /* !TTF */

#if HAVE_DVB_API_VERSION < 3
		avia_pig_hide(pig);
		avia_pig_set_pos(pig, tx, ty);
		avia_pig_set_size(pig, tw, th);
		avia_pig_set_stack(pig, 2);
		avia_pig_show(pig);
#else
		ioctl(pig, VIDIOC_OVERLAY, &sm);
		sm = 1;
		ioctl(pig, VIDIOC_G_FMT, &format);
		format.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
		format.fmt.win.w.left   = tx;
		format.fmt.win.w.top    = ty;
		format.fmt.win.w.width  = tw;
		format.fmt.win.w.height = th;
		ioctl(pig, VIDIOC_S_FMT, &format);
		ioctl(pig, VIDIOC_OVERLAY, &sm);
#endif
		ioctl(avs, AVSIOSSCARTPIN8, &fncmodes[screen_mode2]);
		ioctl(saa, SAAIOSWSS, &saamodes[screen_mode2]);
	}
	else /* not split */
	{
#if HAVE_DVB_API_VERSION < 3
		avia_pig_hide(pig);
#else
		ioctl(pig, VIDIOC_OVERLAY, &screenmode);
#endif

#if CFGTTF 
		displaywidth= (ex             -sx);
		fontwidth = fontwidth_normal;
		typettf.font.pix_width  = (FT_UShort) fontwidth * TTFWIDTHFACTOR;
		StartX = sx; //+ (ex-sx - 40*fontwidth) / 2; /* center screen */
#else	 /* !TTF */
		type0.font.pix_width = type1.font.pix_width = type2.font.pix_width = fontwidth_normal;
		type0.font.pix_height = type1.font.pix_height = type2.font.pix_height = fontheight+2;
#endif	 /* !TTF */

		ioctl(avs, AVSIOSSCARTPIN8, &fncmodes[screen_mode1]);
		ioctl(saa, SAAIOSWSS, &saamodes[screen_mode1]);
	}
}

/******************************************************************************
 * SwitchTranspMode                                                           *
 ******************************************************************************/

void SwitchTranspMode()
{
	if (screenmode)
		SwitchScreenMode(0); /* turn off divided screen */


	/* toggle mode */
	if (!transpmode)
		transpmode = 2;
	else
		transpmode--; /* backward to immediately switch to TV-screen */

	printf("TuxTxt <SwitchTranspMode: %d>\n", transpmode);

	/* set mode */
	if (!transpmode)
	{
		ClearBB(black);
		pageupdate = 1;
	}
	else if (transpmode == 1)
	{
		ClearBB(transp);
		pageupdate = 1;
	}
	else
	{
		ClearFB(transp);
		clearbbcolor = black;
	}
}

/******************************************************************************
 * SwitchHintMode                                                             *
 ******************************************************************************/

void SwitchHintMode()
{
	/* toggle mode */
	hintmode ^= 1;

	printf("TuxTxt <SwitchHintMode: %d>\n", hintmode);

	/* update page */
	pageupdate = 1;
}

/******************************************************************************
 * RenderChar (TTF)                                                           *
 ******************************************************************************/

#if CFGTTF

void DrawVLine(int x, int y, int l, int color)
{
	unsigned char *p = lfb + x + y * var_screeninfo.xres;

	for ( ; l > 0 ; l--)
	{
		*p = color;
		p += var_screeninfo.xres;
	}
}

void DrawHLine(int x, int y, int l, int color)
{
	if (l > 0)
		memset(lfb + x + y * var_screeninfo.xres, color, l);
}

void FillRectMosaicSeparated(int x, int y, int w, int h, int fgcolor, int bgcolor, int set)
{
	FillRect(x, y, w, h, bgcolor);
	if (set)
	{
		FillRect(x+1, y+1, w-2, h-2, fgcolor);
	}
}

void RenderChar(int Char, int Attribute, int zoom, int yoffset)
{
	int Row, Pitch, Bit;
	int error, glyph;
	int bgcolor, fgcolor;
	int factor;
	unsigned char *sbitbuffer;

	int abx = ((displaywidth)%(40-nofirst) == 0 ? displaywidth+1 : (displaywidth)/(((displaywidth)%(40-nofirst))));// distance between 'inserted' pixels
	int curfontwidth = fontwidth+(((PosX-sx) / abx) < ((PosX+fontwidth+1-sx) /abx) ? 1 : 0);

	if (Char == 0xFF)	/* skip doubleheight chars in lower line */
	{
		PosX += curfontwidth;
		return;
	}

	/* get colors */
	fgcolor = Attribute & 0x0F;
	if (transpmode == 1 && PosY < StartY + 24*fontheight)
	{
		if (fgcolor == transp) /* outside boxed elements (subtitles, news) completely transparent */
			bgcolor = transp;
		else
			bgcolor = transp2;
	}
	else
		bgcolor = (Attribute>>4) & 0x0F;

	/* handle mosaic */
	if ((Attribute & 0x300) &&
		 ((Char&0xA0) == 0x20))
	{
		int w1 = curfontwidth / 2;
		int w2 = curfontwidth - w1;
		int y;
		
		Char = (Char & 0x1f) | ((Char & 0x40) >> 1);
		if (Attribute & 0x200) /* separated mosaic */
			for (y = 0; y < 3; y++)
			{
				FillRectMosaicSeparated(PosX,      PosY + yoffset + ymosaic[y], w1, ymosaic[y+1] - ymosaic[y], fgcolor, bgcolor, Char & 0x01);
				FillRectMosaicSeparated(PosX + w1, PosY + yoffset + ymosaic[y], w2, ymosaic[y+1] - ymosaic[y], fgcolor, bgcolor, Char & 0x02);
				Char >>= 2;
			}
		else
			for (y = 0; y < 3; y++)
			{
				FillRect(PosX,      PosY + yoffset + ymosaic[y], w1, ymosaic[y+1] - ymosaic[y], (Char & 0x01) ? fgcolor : bgcolor);
				FillRect(PosX + w1, PosY + yoffset + ymosaic[y], w2, ymosaic[y+1] - ymosaic[y], (Char & 0x02) ? fgcolor : bgcolor);
				Char >>= 2;
			}
		
		PosX += curfontwidth;
		return;
	}
	
	if (zoom && (Attribute & 1<<10))
		factor = 4;
	else if (zoom || (Attribute & 1<<10))
		factor = 2;
	else
		factor = 1;
		
	/* load char */
	switch (Char)
	{
	case 0x00:
	case 0x20:
		FillRect(PosX, PosY + yoffset, curfontwidth, factor*fontheight, bgcolor);
		PosX += curfontwidth;
		return;
	case 0x23:
	case 0x24:
		Char = nationaltable23[national_subset][Char-0x23];
		break;
	case 0x40:
		Char = nationaltable40[national_subset];
		break;
	case 0x5B:
	case 0x5C:
	case 0x5D:
	case 0x5E:
	case 0x5F:
	case 0x60:
		Char = nationaltable5b[national_subset][Char-0x5B];
		break;
	case 0x7B:
	case 0x7C:
	case 0x7D:
	case 0x7E:
		Char = nationaltable7b[national_subset][Char-0x7B];
		break;
	case 0x7F:
		FillRect(PosX, PosY + yoffset, curfontwidth, factor*ascender, fgcolor);
		FillRect(PosX, PosY + yoffset + factor*ascender, curfontwidth, factor*(fontheight-ascender), bgcolor);
		PosX += curfontwidth;
		return;
	case 0xE0: /* |- */
		DrawHLine(PosX, PosY + yoffset, curfontwidth, fgcolor);
		DrawVLine(PosX, PosY + yoffset +1, fontheight -1, fgcolor);
		FillRect(PosX +1, PosY + yoffset +1, curfontwidth-1, fontheight-1, bgcolor);
		PosX += curfontwidth;
		return;
	case 0xE1: /* - */
		DrawHLine(PosX, PosY + yoffset, curfontwidth, fgcolor);
		FillRect(PosX, PosY + yoffset +1, curfontwidth, fontheight-1, bgcolor);
		PosX += curfontwidth;
		return;
	case 0xE2: /* -| */
		DrawHLine(PosX, PosY + yoffset, curfontwidth, fgcolor);
		DrawVLine(PosX + curfontwidth -1, PosY + yoffset +1, fontheight -1, fgcolor);
		FillRect(PosX, PosY + yoffset +1, curfontwidth-1, fontheight-1, bgcolor);
		PosX += curfontwidth;
		return;
	case 0xE3: /* |  */
		DrawVLine(PosX, PosY + yoffset, fontheight, fgcolor);
		FillRect(PosX +1, PosY + yoffset, curfontwidth -1, fontheight, bgcolor);
		PosX += curfontwidth;
		return;
	case 0xE4: /*  | */
		DrawVLine(PosX + curfontwidth -1, PosY + yoffset, fontheight, fgcolor);
		FillRect(PosX, PosY + yoffset, curfontwidth -1, fontheight, bgcolor);
		PosX += curfontwidth;
		return;
	case 0xE5: /* |_ */
		DrawHLine(PosX, PosY + yoffset + fontheight -1, curfontwidth, fgcolor);
		DrawVLine(PosX, PosY + yoffset, fontheight -1, fgcolor);
		FillRect(PosX +1, PosY + yoffset, curfontwidth-1, fontheight-1, bgcolor);
		PosX += curfontwidth;
		return;
	case 0xE6: /* _ */
		DrawHLine(PosX, PosY + yoffset + fontheight -1, curfontwidth, fgcolor);
		FillRect(PosX, PosY + yoffset, curfontwidth, fontheight-1, bgcolor);
		PosX += curfontwidth;
		return;
	case 0xE7: /* _| */
		DrawHLine(PosX, PosY + yoffset + fontheight -1, curfontwidth, fgcolor);
		DrawVLine(PosX + curfontwidth -1, PosY + yoffset, fontheight -1, fgcolor);
		FillRect(PosX, PosY + yoffset, curfontwidth-1, fontheight-1, bgcolor);
		PosX += curfontwidth;
		return;
	case 0xE8: /* Ii */
		FillRect(PosX +1, PosY + yoffset, curfontwidth -1, fontheight, bgcolor);
		for (Row=0; Row < curfontwidth/2; Row++)
			DrawVLine(PosX + Row, PosY + yoffset + Row, fontheight - Row, fgcolor);
		PosX += curfontwidth;
		return;
	case 0xE9: /* II */
		FillRect(PosX, PosY + yoffset, curfontwidth/2, fontheight, fgcolor);
		FillRect(PosX + curfontwidth/2, PosY + yoffset, (curfontwidth+1)/2, fontheight, bgcolor);
		PosX += curfontwidth;
		return;
	case 0xEA: /* °  */
		FillRect(PosX, PosY + yoffset, curfontwidth, fontheight, bgcolor);
		FillRect(PosX, PosY + yoffset, curfontwidth/2, curfontwidth/2, fgcolor);
		PosX += curfontwidth;
		return;
	case 0xEB: /* ¬ */
		FillRect(PosX, PosY + yoffset +1, curfontwidth, fontheight -1, bgcolor);
		for (Row=0; Row < curfontwidth/2; Row++)
			DrawHLine(PosX + Row, PosY + yoffset + Row, curfontwidth - Row, fgcolor);
		PosX += curfontwidth;
		return;
	case 0xEC: /* -- */
		FillRect(PosX, PosY + yoffset, curfontwidth, curfontwidth/2, fgcolor);
		FillRect(PosX, PosY + yoffset + curfontwidth/2, curfontwidth, fontheight - curfontwidth/2, bgcolor);
		PosX += curfontwidth;
		return;
	case 0xED:
	case 0xEE:
	case 0xEF:
	case 0xF0:
	case 0xF1:
	case 0xF2:
	case 0xF3:
	case 0xF4:
	case 0xF5:
	case 0xF6:
		Char = arrowtable[Char - 0xED];
		break;
	default:
		break;
	}

	if (!(glyph = FT_Get_Char_Index(face, Char)))
	{
#if DEBUG
		printf("TuxTxt <FT_Get_Char_Index for Char %x \"%c\" failed\n", Char, Char);
#endif
		FillRect(PosX, PosY + yoffset, curfontwidth, fontheight, bgcolor);
		PosX += curfontwidth;
		return;
	}

//	typettf.font.pix_width  = (FT_UShort) curfontwidth * TTFWIDTHFACTOR - 2;
#ifndef DREAMBOX
	if ((error = FTC_SBitCache_Lookup(cache, &typettf, glyph, &sbit, NULL)) != 0)
#else
	if ((error = FTC_SBit_Cache_Lookup(cache, &typettf, glyph, &sbit)) != 0)
#endif
	{
#if DEBUG
		printf("TuxTxt <FTC_SBitCache_Lookup: 0x%x> c%x a%x g%x w%d h%d x%d y%d\n",
				 error, Char, Attribute, glyph, curfontwidth, fontheight, PosX, PosY);
#endif
		FillRect(PosX, PosY + yoffset, curfontwidth, fontheight, bgcolor);
		PosX += curfontwidth;
		return;
	}

	/* render char */
	sbitbuffer = sbit->buffer;
  	if (yoffset >= 0)	/* framebuffer */
	{
		unsigned char *p;
		int f; /* runningh counter for zoom factor */
		
		Row = factor * (ascender - sbit->top);
		FillRect(PosX, PosY + yoffset, curfontwidth, Row, bgcolor); /* fill upper margin */

		p = lfb + PosX + (yoffset + PosY + Row) * var_screeninfo.xres; /* running pointer into framebuffer */
		for (Row = sbit->height; Row; Row--) /* row counts up, but down may be a little faster :) */
		{
			int pixtodo = sbit->width;
			char *pstart = p;

#if 1
			for (Bit = sbit->left; Bit > 0; Bit--) /* fill left margin */
			{
				for (f = factor-1; f >= 0; f--)
					*(p + f*var_screeninfo.xres) = bgcolor;
				p++;
			}
#endif

			for (Pitch = sbit->pitch; Pitch; Pitch--)
			{
				for (Bit = 0x80; Bit; Bit >>= 1)
				{
					int color;
				
					if (--pixtodo < 0)
						break;

					if (*sbitbuffer & Bit) /* bit set -> foreground */
						color = fgcolor;
					else /* bit not set -> background */
						color = bgcolor;
				
					for (f = factor-1; f >= 0; f--)
						*(p + f*var_screeninfo.xres) = color;

					p++;
				}
				sbitbuffer++;
			}

#if 1
			for (Bit = curfontwidth - sbit->width - sbit->left; Bit > 0; Bit--) /* fill rest of char width */
#else
			for (Bit = curfontwidth - sbit->width; Bit > 0; Bit--) /* fill rest of char width */
#endif
			{
				for (f = factor-1; f >= 0; f--)
					*(p + f*var_screeninfo.xres) = bgcolor;
				p++;
			}

			p = pstart + factor*var_screeninfo.xres;
		}

		Row = ascender - sbit->top + sbit->height;
		FillRect(PosX, PosY + yoffset + Row*factor, curfontwidth, (fontheight - Row) * factor, bgcolor); /* fill lower margin */

		PosX += curfontwidth;

	}
	else /* yoffset<0: LCD */
	{
		int x = ((-yoffset) & 0xFF) + sbit->left;
		int y = ((-yoffset)>>8 & 0xFF) + ascender - sbit->top;
		int xstart = x;

		for (Row = fontheight; Row; Row--) /* row counts up, but down may be a little faster :) */
		{
			int pixtodo = sbit->width;
			int lcdbase = (y/8)*120;
			int lcdmask = 1 << (y%8);

			for (Pitch = sbit->pitch; Pitch; Pitch--)
			{
				for (Bit = 0x80; Bit; Bit >>= 1)
				{
					if (--pixtodo < 0)
						break;

					if (*sbitbuffer & Bit)
						lcd_backbuffer[x + lcdbase] |= lcdmask;
					else
						lcd_backbuffer[x + lcdbase] &= ~lcdmask;
					x++;
				}
				sbitbuffer++;
			}
			x = xstart;
			y++;
		}
	}
}

/******************************************************************************
 * RenderChar (!TTF)                                                          *
 ******************************************************************************/

#else	 /* !TTF */

void RenderChar(int Char, int Attribute, int zoom, int yoffset)
{
	int Row, Pitch, Bit;
	int error;
	int bgcolor, fgcolor;
	unsigned char *sbitbuffer;
	FONTTYPE *pt;

	/* skip doubleheight chars in lower line */
	if (Char == 0xFF)
	{
		PosX += fontwidth;
		return;
	}

#if (fontwidth_topmenumain==fontwidth_small)
#define pt0 &type0
#define pt1 &type1
#define pt2 &type2
#else
	FONTTYPE *pt0, *pt1, *pt2;
	if (fontwidth == fontwidth_topmenumain)
	{
		/* pointer to current main font type (8/16pt or 12pt for split screen with topmenu) */
		pt0 = &type0r;
		pt1 = &type1r;
		pt2 = &type2r;
	}
	else
	{
		pt0 = &type0;
		pt1 = &type1;
		pt2 = &type2;
	}
#endif

	/* load char */
	if ((Attribute>>8) & 3)
	{
		switch (Char)
		{
		case 0x40:
			Char = 0x02;
			pt = pt2;
			break;
		case 0x5B:	
		case 0x5C:
		case 0x5D:
		case 0x5E:
		case 0x5F:
			Char += 0x03 - 0x5B;
			pt = pt2;
			break;
		default:
			Char += ((((Attribute>>8) & 3) - 1) * 96) + 1;
			pt = pt1;
			break;
		}
	}
	else
	{
		switch (Char)
		{
		case 0x23:
		case 0x24:
			Char += 0x00 - 0x23;
			pt = pt2;
			break;
		case 0x40:
			Char = 0x02;
			pt = pt2;
			break;
		case 0x5B:
		case 0x5C:
		case 0x5D:
		case 0x5E:
		case 0x5F:
		case 0x60:
			Char += 0x03 - 0x5B;
			pt = pt2;
			break;
		case 0x7B:
		case 0x7C:
		case 0x7D:
		case 0x7E:
			Char += 0x09 - 0x7B;
			pt = pt2;
			break;
		default:
			Char += 1;
			pt = pt0;
			break;
		}
	}

	if (pt == pt2)
		Char += national_subset*13 + 1;

#ifndef DREAMBOX
	if ((error = FTC_SBitCache_Lookup(cache, pt, Char, &sbit, NULL)) != 0)
#else
	if ((error = FTC_SBit_Cache_Lookup(cache, pt, Char, &sbit)) != 0)
#endif
	{
#if DEBUG
		printf("TuxTxt <FTC_SBitCache_Lookup: 0x%x> c%x a%x w%d h%d x%d y%d\n",
				 error, Char, Attribute, (*pt).font.pix_width, (*pt).font.pix_height, PosX, PosY);
#endif
		PosX += (*pt).font.pix_width;
		return;
	}

	/* render char */
	sbitbuffer = sbit->buffer;
  	if (yoffset >= 0)	/* framebuffer */
	{
		unsigned char *p = lfb + PosX + (yoffset+PosY)*var_screeninfo.xres; /* running pointer into framebuffer */
		fgcolor = Attribute & 0x0F;
		if (transpmode == 1 && PosY < StartY + 24*fontheight)
		{
			if (fgcolor == transp) /* outside boxed elements (subtitles, news) completely transparent */
				bgcolor = transp;
			else
				bgcolor = transp2;
		}
		else
			bgcolor = (Attribute>>4) & 0x0F;
		
		for (Row = sbit->height; Row; Row--) /* row counts up, but down may be a little faster :) */
		{
			int pixtodo = sbit->width;
			char *pstart = p;

			for (Pitch = sbit->pitch; Pitch; Pitch--)
			{
				for (Bit = 0x80; Bit; Bit >>= 1)
				{
					int color;
				
					if (--pixtodo < 0)
						break;

					if (*sbitbuffer & Bit) /* bit set -> foreground */
						color = fgcolor;
					else /* bit not set -> background */
						color = bgcolor;
				
					*p = color;

					if (zoom || (Attribute & 1<<10))
					{
						*(p + 1*var_screeninfo.xres) = color;
						if (zoom && (Attribute & 1<<10))
						{
							*(p + 2*var_screeninfo.xres) = color;
							*(p + 3*var_screeninfo.xres) = color;
						}
					}

					p++;
				}
				sbitbuffer++;
			}

			if (zoom && (Attribute & 1<<10))
				p = pstart + 4*var_screeninfo.xres;
			else if (zoom || (Attribute & 1<<10))
				p = pstart + 2*var_screeninfo.xres;
			else
				p = pstart + 1*var_screeninfo.xres;
		}

#if 0		
		PosX += sbit->xadvance;
#else
		PosX += fontwidth;
#endif
	}
	else /* yoffset<0: LCD */
	{
		int x = (-yoffset) & 0xFF;
		int y = (-yoffset)>>8 & 0xFF;
		int xstart = x;

		for (Row = fontheight; Row; Row--) /* row counts up, but down may be a little faster :) */
		{
			int pixtodo = sbit->width;
			int lcdbase = (y/8)*120;
			int lcdmask = 1 << (y%8);

			for (Pitch = sbit->pitch; Pitch; Pitch--)
			{
				for (Bit = 0x80; Bit; Bit >>= 1)
				{
					if (--pixtodo < 0)
						break;

					if (*sbitbuffer & Bit)
						lcd_backbuffer[x + lcdbase] |= lcdmask;
					else
						lcd_backbuffer[x + lcdbase] &= ~lcdmask;
					x++;
				}
				sbitbuffer++;
			}
			x = xstart;
			y++;
		}
	}
}

#endif /* !TTF */

/******************************************************************************
 * RenderCharFB                                                               *
 ******************************************************************************/

void RenderCharFB(int Char, int Attribute)
{
	RenderChar(Char, Attribute, zoommode, var_screeninfo.yoffset);
}

/******************************************************************************
 * RenderCharBB                                                               *
 ******************************************************************************/

void RenderCharBB(int Char, int Attribute)
{
	if (var_screeninfo.yoffset)
		RenderChar(Char, Attribute, 0, 0);
	else
		RenderChar(Char, Attribute, 0, var_screeninfo.yres);
}

/******************************************************************************
 * RenderCharLCD                                                             *
 ******************************************************************************/

void RenderCharLCD(int Digit, int XPos, int YPos)
{
	int x, y;

	/* render digit to lcd backbuffer */
	for (y = 0; y < 15; y++)
	{
		for (x = 0; x < 10; x++)
		{
			if (lcd_digits[Digit*15*10 + x + y*10])
				lcd_backbuffer[XPos + x + ((YPos+y)/8)*120] |= 1 << ((YPos+y)%8);
			else
				lcd_backbuffer[XPos + x + ((YPos+y)/8)*120] &= ~(1 << ((YPos+y)%8));
		}
	}
}

#if 0
void RenderCharLCDsmall(int Char, int XPos, int YPos)
{
	int old_width = fontwidth;
	int old_height = fontheight;
	typettf.font.pix_width = fontwidth = fontwidth_small_lcd;
	typettf.font.pix_height = fontheight = fontwidth_small_lcd;
	RenderChar(Char, 0, 0, -(YPos<<8 | XPos));
	fontwidth = old_width;
	typettf.font.pix_width  = (FT_UShort) fontwidth * TTFWIDTHFACTOR;
	typettf.font.pix_height = fontheight = old_height;
}
#endif

/******************************************************************************
 * RenderMessage                                                              *
 ******************************************************************************/

void RenderMessage(int Message)
{
	int byte;
	int fbcolor, timecolor, menucolor;
	int pagecolumn;
	const char *msg;


/*                     00000000001111111111222222222233333333334 */
/*                     01234567890123456789012345678901234567890 */
	char message_1[] = "àááááááá www.tuxtxt.com x.xx áááááááâè";
	char message_2[] = "ã                                   äé";
/* 	char message_3[] = "ã   suche nach Teletext-Anbietern   äé"; */
	char message_4[] = "ã                                   äé";
	char message_5[] = "åæææææææææææææææææææææææææææææææææææçé";
	char message_6[] = "ëììììììììììììììììììììììììììììììììììììê";

/* 	char message_7[] = "ã kein Teletext auf dem Transponder äé"; */
/* 	char message_8[] = "ã  warte auf Empfang von Seite 100  äé"; */
/* 	char message_9[] = "ã     Seite 100 existiert nicht!    äé"; */

	memcpy(&message_1[24], versioninfo, 4);
	/* reset zoom */
	zoommode = 0;

	/* set colors */
#ifndef DREAMBOX
	if (screenmode)
	{
		fbcolor   = black;
		timecolor = black<<4 | black;
		menucolor = menu1;
	}
	else
#endif
	{
		fbcolor   = transp;
		timecolor = transp<<4 | transp;
		menucolor = menu3;
	}

	/* clear framebuffer */
	ClearFB(fbcolor);

	/* hide timestring */
	page_atrb[32] = timecolor;

	/* set pagenumber */
	if (Message == PageNotFound || Message == ShowServiceName)
	{
		if (bttok && !basictop[page]) /* page non-existent according to TOP (continue search anyway) */
		{
			pagecolumn = MESSAGE9PAGECOLUMN;
			msg = message_9[menulanguage];
		}
		else
		{
			pagecolumn = message8pagecolumn[menulanguage];
			msg = message_8[menulanguage];
		}
		memcpy(&message_4, msg, sizeof(message_4));
		hex2str(message_4+pagecolumn, page);

		if (SDT_ready)
			memcpy(&message_2[2 + (35 - pid_table[current_service].service_name_len)/2],
					 &pid_table[current_service].service_name, pid_table[current_service].service_name_len);
		else if (Message == ShowServiceName)
			hex2str(&message_2[17+3], vtxtpid);

		msg = &message_3_blank[0];
	}
	else if (Message == NoServicesFound)
		msg = &message_7[menulanguage][0];
	else
		msg = &message_3[menulanguage][0];

	/* render infobar */
	PosX = StartX + fontwidth+5;
	PosY = StartY + fontheight*16;
	for (byte = 0; byte < 37; byte++)
		RenderCharFB(message_1[byte], menucolor<<4 | ((byte >= 9 && byte <= 27) ? yellow : menu2));
	RenderCharFB(message_1[37], fbcolor<<4 | menu2);

	PosX = StartX + fontwidth+5;
	PosY = StartY + fontheight*17;
	RenderCharFB(message_2[0], menucolor<<4 | menu2);
	for (byte = 1; byte < 36; byte++)
		RenderCharFB(message_2[byte], menucolor<<4 | white);
	RenderCharFB(message_2[36], menucolor<<4 | menu2);
	RenderCharFB(message_2[37], fbcolor<<4 | menu2);

	PosX = StartX + fontwidth+5;
	PosY = StartY + fontheight*18;
	RenderCharFB(msg[0], menucolor<<4 | menu2);
	for (byte = 1; byte < 36; byte++)
		RenderCharFB(msg[byte], menucolor<<4 | white);
	RenderCharFB(msg[36], menucolor<<4 | menu2);
	RenderCharFB(msg[37], fbcolor<<4 | menu2);

	PosX = StartX + fontwidth+5;
	PosY = StartY + fontheight*19;
	RenderCharFB(message_4[0], menucolor<<4 | menu2);
	for (byte = 1; byte < 36; byte++)
		RenderCharFB(message_4[byte], menucolor<<4 | white);
	RenderCharFB(message_4[36], menucolor<<4 | menu2);
	RenderCharFB(message_4[37], fbcolor<<4 | menu2);

	PosX = StartX + fontwidth+5;
	PosY = StartY + fontheight*20;
	for (byte = 0; byte < 37; byte++)
		RenderCharFB(message_5[byte], menucolor<<4 | menu2);
	RenderCharFB(message_5[37], fbcolor<<4 | menu2);

	PosX = StartX + fontwidth+5;
	PosY = StartY + fontheight*21;
	for (byte = 0; byte < 38; byte++)
		RenderCharFB(message_6[byte], fbcolor<<4 | menu2);
}

/******************************************************************************
 * RenderPage                                                                 *
 ******************************************************************************/

void RenderPage()
{
	int row, col, byte;

	/* update lcd */
	UpdateLCD();


	/* update page or timestring */
	if (transpmode != 2 && pageupdate && page_receiving != page && inputcounter == 2)
	{

		/* get national subset */
		if (auto_national)
		{
			if (current_national_subset >= 0)
				national_subset = current_national_subset;
			else
				national_subset = countryconversiontable[countrycontrolbitstable[page][subpage]];
		}
		
		/* reset update flag */
		pageupdate = 0;

		/* decode page */
		if (subpagetable[page] != 0xFF)
			DecodePage();
		else
		{
			RenderMessage(PageNotFound);
			return;
		}

		/* display first column?  */
		nofirst = show39;
		for (row = 1; row < 24; row++)
		{
			if (page_char[row*40] != ' ' && page_char[row*40] != 0x00 && page_char[row*40] != 0xFF && page_atrb[row*40] != (black <<4 | black)) {nofirst = 0; break;}
		}
#if CFGTTF
		fontwidth = fontwidth_normal = (ex-sx) / (40-nofirst);
		fontwidth_topmenumain = (TV43STARTX-sx) / (40-nofirst);
		fontwidth_topmenusmall = (ex- TV43STARTX - TOPMENUINDENTDEF) / TOPMENUCHARS;
		fontwidth_small = (TV169FULLSTARTX-sx)  / (40-nofirst);
		switch(screenmode)
		{
			case 0:	fontwidth = fontwidth_normal     ; displaywidth= (ex             -sx);break;
			case 1: fontwidth = fontwidth_topmenumain; displaywidth= (TV43STARTX     -sx);break;
			case 2: fontwidth = fontwidth_small      ; displaywidth= (TV169FULLSTARTX-sx);break;
		}
#endif
		if (transpmode || (boxed && !screenmode))
		{
			ClearBB(transp);
			clearbbcolor = black;
		}

		/* render page */
		PosY = StartY;

		for (row = 0; row < 24; row++)
		{
			PosX = StartX;

			for (col = nofirst; col < 40; col++)
				RenderCharBB(page_char[row*40 + col], page_atrb[row*40 + col]);

			PosY += fontheight;
		}

		/* update framebuffer */
		CopyBB2FB();
	}
	else if (transpmode != 2 && zoommode != 2)
	{
		/* update timestring */
		SetPosX(32);
		PosY = StartY;

		for (byte = 0; byte < 8; byte++)
			RenderCharFB(timestring[byte], page_atrb[32]);
	}
}

/******************************************************************************
 * CreateLine25                                                               *
 ******************************************************************************/

void showlink(int column, int linkpage, int Attrib)
{
	unsigned char *p, line[] = "   >???   ";
	int oldfontwidth = fontwidth;
	int yoffset;

	if (var_screeninfo.yoffset)
		yoffset = 0;
	else
		yoffset = var_screeninfo.yres;
	
#if CFGTTF
	int abx = (displaywidth)/(((displaywidth)%(40-nofirst))+1);// distance between 'inserted' pixels
	int width = displaywidth /4;
#else
	int width = ((40-nofirst)*oldfontwidth)/4;
#endif
	
	PosY = StartY + 24*fontheight;

	if (boxed)
	{
		PosX = StartX + column*width;
#if CFGTTF
		FillRect(PosX, PosY+yoffset, displaywidth, fontheight, transp);
#else
		FillRect(PosX, PosY+yoffset, 40*oldfontwidth, fontheight, transp);
#endif
		return;
	}
	
	if (adip[linkpage][0])
	{
		PosX = StartX + column*width;
#if CFGTTF
		int l = strlen(adip[linkpage]);
		
		if (l > 9) /* smaller font, if no space for one half space at front and end */
		{
			fontwidth = oldfontwidth * 10 / (l+1);
			typettf.font.pix_width  = (FT_UShort) fontwidth * TTFWIDTHFACTOR;
		}
		FillRect(PosX, PosY+yoffset, width+(displaywidth%4), fontheight, Attrib >> 4);
		PosX += ((width) - (l*fontwidth+l*fontwidth/abx))/2; /* center */
		for (p = adip[linkpage]; *p; p++)
			RenderCharBB(*p, Attrib);
		fontwidth = oldfontwidth;
		typettf.font.pix_width = (FT_UShort) fontwidth * TTFWIDTHFACTOR;
#else	 /* !TTF */
		for (p = adip[linkpage]; p < adip[linkpage]+10; p++) /* only first 10 chars */
			RenderCharBB(*p, Attrib);
#endif /* !TTF */
	}
	else
	{

		if (showflof && flofpages[page][0] != 0) // FLOF-Navigation present
		{
			if (column == 0)
			{
				PosX = StartX;
#if CFGTTF
				FillRect(PosX, PosY+yoffset, displaywidth, fontheight, black);
#else
				FillRect(PosX, PosY+yoffset, (40-nofirst)*oldfontwidth, fontheight, black);
#endif
			}
			p = page_char+24*40;
			char* l;
			char p1[41];
			memset(p1,0,41);
			strncpy(p1,p,40);
			char* p2 = strchr(p1,(char)(column+1));
			if (!p2 && column == 3) p2 = strchr(p1,(char)(6));
			if (p2)
			{
				char* p3 = strchr(p2,(char)(column+2));
			    if (!p3) p3 = strchr(p2,(char)(6));

				if (p3) *p3= 0x00;
				if (nofirst == 0 || column > 0)
				{
					FillRect(PosX, PosY+yoffset, (fontwidth/2), fontheight, Attrib>>4);
					PosX += (fontwidth/2);
				}
				for (l = p2+1; *l; l++)
				{
					RenderCharBB(*l , Attrib);
				}
#if CFGTTF
				FillRect(PosX, PosY+yoffset, (StartX+displaywidth)-PosX, fontheight, Attrib>>4);
#else
				FillRect(PosX, PosY+yoffset, (StartX+(40-nofirst)*oldfontwidth)-PosX, fontheight, Attrib>>4);
#endif
				PosX += (fontwidth/2);
			}
		}
		else
		{
			PosX = StartX + column*width;
		if (linkpage < page)
		{
			line[6] = '<';
			hex2str(line + 5, linkpage);
		}
		else
			hex2str(line + 6, linkpage);
		for (p = line; p < line+10; p++)
			RenderCharBB(*p, Attrib);
	}
}
}

void CreateLine25()
{
	if (!bttok && cachetable[0x1f0][0] && cachetable[0x1f0][0][40+799]) /* btt received and not yet decoded */
		decode_btt();
	if (maxadippg >= 0)
		decode_adip();

	if (!showhex && showflof && flofpages[page][0] != 0) // FLOF-Navigation present
	{
		prev_100 = flofpages[page][0];
		prev_10  = flofpages[page][1];
		next_10  = flofpages[page][2];
		next_100 = flofpages[page][3];
	}
	else
	{
/*  1: blk-1, grp-1, grp+1, blk+1 */
/*  2: blk-1, grp+1, grp+2, blk+1 */
#if (LINE25MODE == 1)
	prev_10  = toptext_getnext(page, 0, 1); /* arguments: startpage, up, findgroup */
	if (showhex)
		prev_100 = next_hex(page);
	else
		prev_100 = toptext_getnext(prev_10, 0, 0);
	next_10  = toptext_getnext(page, 1, 1);
#else
	if (showhex)
		prev_100 = next_hex(page);
	else
		prev_100 = toptext_getnext(page, 0, 0);
	prev_10  = toptext_getnext(page, 1, 1);
	next_10  = toptext_getnext(prev_10, 1, 1);
#endif
	next_100 = toptext_getnext(next_10, 1, 0);
	}
	showlink(0, prev_100, red<<4 | white);
	showlink(1, prev_10, green<<4 | black);
	showlink(2, next_10, yellow<<4 | black);
	showlink(3, next_100, blue<<4 | white);

	
	if (bttok && screenmode == 1) /* TOP-Info present, divided screen -> create TOP overview */
	{
		char line[TOPMENUCHARS];
		int current;
		int prev10done, next10done, next100done, indent;
		int attrcol; /* color attribute for navigation keys */
		int attr;

#if CFGTTF 
		int olddisplaywidth = displaywidth;
		displaywidth = fontwidth_topmenusmall*(40-nofirst);
		fontwidth = fontwidth_topmenusmall;
		typettf.font.pix_width  = (FT_UShort) fontwidth * TTFWIDTHFACTOR;
#else	 /* !TTF */
		type0.font.pix_width = type1.font.pix_width = type2.font.pix_width = fontwidth_topmenusmall;
#endif /* !TTF */

		PosY = TOPMENUSTARTY;
		memset(line, ' ', TOPMENUCHARS); /* init with spaces */

		memcpy(line+TOPMENUINDENTBLK, adip[prev_100], 12);
		hex2str(&line[TOPMENUINDENTDEF+12+TOPMENUSPC+2], prev_100);
		RenderClearMenuLineBB(line, '(', black<<4 | yellow);

/*  1: blk-1, grp-1, grp+1, blk+1 */
/*  2: blk-1, grp+1, grp+2, blk+1 */
#if (LINE25MODE == 1)
		current = prev_10 - 1;
#else
		current = page - 1;
#endif

		prev10done = next10done = next100done = 0;
		while (PosY <= (TOPMENUENDY-fontheight))
		{
			attr = 0;
			attrcol = black<<4 | white;
			if (!next100done && (PosY > (TOPMENUENDY - 2*fontheight))) /* last line */
			{
				attrcol = 'X';
				current = next_100;
			}
			else if (!next10done && (PosY > (TOPMENUENDY - 3*fontheight))) /* line before */
			{
				attrcol = 'A';
				current = next_10;
			}
			else if (!prev10done && (PosY > (TOPMENUENDY - 4*fontheight))) /* line before */
			{
				attrcol = '1';
				current = prev_10;
			}
			else do
			{
				next_dec(&current);
				if (current == prev_10)
				{
					attrcol = '1';
					prev10done = 1;
					break;
				}
				else if (current == next_10)
				{
					attrcol = 'A';
					next10done = 1;
					break;
				}
				else if (current == next_100)
				{
					attrcol = 'X';
					next100done = 1;
					break;
				}
				else if (current == page)
				{
					attr = black<<4 | magenta;
					break;
				}
			} while (adip[current][0] == 0 && (basictop[current] < 2 || basictop[current] > 7));

			if (basictop[current] >= 2 && basictop[current] <= 5) /* block */
			{
				indent = TOPMENUINDENTBLK;
				if (!attr)
					attr = black<<4 | (basictop[current] <=3 ? green : yellow);	/* green for program block */
			}
			else if (basictop[current] >= 6 && basictop[current] <= 7) /* group */
			{
				indent = TOPMENUINDENTGRP;
				if (!attr)
					attr = black<<4 | cyan;
			}
			else
			{
				indent = TOPMENUINDENTDEF;
				if (!attr)
					attr = black<<4 | white;
			}
			memcpy(line+indent, adip[current], 12);
			hex2str(&line[TOPMENUINDENTDEF+12+TOPMENUSPC+2], current);
			RenderClearMenuLineBB(line, attrcol, attr);
		}
#if CFGTTF 
		displaywidth = olddisplaywidth;
		fontwidth = fontwidth_topmenumain;
		typettf.font.pix_width  = (FT_UShort) fontwidth * TTFWIDTHFACTOR;
#else	 /* !TTF */
		type0.font.pix_width = type1.font.pix_width = type2.font.pix_width = fontwidth_topmenumain;
#endif /* !TTF */
	}
}

/******************************************************************************
 * CopyBB2FB                                                                  *
 ******************************************************************************/

void CopyBB2FB()
{
	unsigned char *src, *dst, *topsrc;
	int fillcolor, i, screenwidth;

	/* line 25 */
	if (!pagecatching)
		CreateLine25();

	/* copy backbuffer to framebuffer */
	if (!zoommode)
	{
		if (var_screeninfo.yoffset)
			var_screeninfo.yoffset = 0;
		else
			var_screeninfo.yoffset = var_screeninfo.yres;

		if (ioctl(fb, FBIOPAN_DISPLAY, &var_screeninfo) == -1)
			perror("TuxTxt <FBIOPAN_DISPLAY>");

		if (StartX > 0 && *lfb != *(lfb + var_screeninfo.xres * var_screeninfo.yres)) /* adapt background of backbuffer if changed */
			 ClearBB(*(lfb + var_screeninfo.xres * var_screeninfo.yoffset));
		if (clearbbcolor >= 0)
		{
			ClearBB(clearbbcolor);
			clearbbcolor = -1;
		}
		return;
	}

	src = dst = topsrc = lfb + StartY*var_screeninfo.xres;

	if (zoommode == 2)
		src += 12*fontheight*var_screeninfo.xres;

	if (var_screeninfo.yoffset)
		dst += var_screeninfo.xres * var_screeninfo.yres;
	else
	{
		src += var_screeninfo.xres * var_screeninfo.yres;
		topsrc += var_screeninfo.xres * var_screeninfo.yres;
	}

	if (transpmode)
		fillcolor = transp;
	else
		fillcolor = black;


	if (screenmode == 1) /* copy topmenu in normal height (since PIG also keeps dimensions) */
	{
		unsigned char *topdst = dst;
		
		screenwidth = TV43STARTX;

		topsrc += screenwidth;
		topdst += screenwidth;
		for (i = 0; i < 25*fontheight; i++)
		{
			memcpy(topdst, topsrc,screenwidth);
			topdst += var_screeninfo.xres;
			topsrc += var_screeninfo.xres;
		}
	}
	else if (screenmode == 2)
		screenwidth = TV169FULLSTARTX+sx;
	else
		screenwidth = var_screeninfo.xres;
		
	for (i = StartY; i>0;i--)
	{
		memset(dst - i*var_screeninfo.xres, fillcolor, screenwidth);
	}

	for (i = 12*fontheight; i; i--)
	{
		memcpy(dst, src, screenwidth);
		dst += var_screeninfo.xres;
		memcpy(dst, src, screenwidth);
		dst += var_screeninfo.xres;
		src += var_screeninfo.xres;
	}

	if (!pagecatching)
		memcpy(dst, lfb + (StartY+24*fontheight)*var_screeninfo.xres, var_screeninfo.xres*fontheight); /* copy line25 in normal height */
	for (i = 0; i<var_screeninfo.yres - StartY - 25*fontheight;i++)
	{
		memset(dst + var_screeninfo.xres*(fontheight+i), fillcolor, screenwidth);
	}
}

/******************************************************************************
 * UpdateLCD                                                                  *
 ******************************************************************************/

void UpdateLCD()
{
	static int init_lcd = 1, old_cached_pages = -1, old_page = -1, old_subpage = -1, old_subpage_max = -1, old_hintmode = -1;
	int  x, y, subpage_max = 0, update_lcd = 0;

	if (lcd == -1) return; // for Dreamboxes without LCD-Display (5xxx)
	/* init or update lcd */
	if (init_lcd)
	{
		init_lcd = 0;

		for (y = 0; y < 64; y++)
		{
			int lcdbase = (y/8)*120;
			int lcdmask = 1 << (y%8);
			
			for (x = 0; x < 120; )
			{
				int rommask;
				int rombyte = lcd_layout[x/8 + y*120/8];
				
				for (rommask = 0x80; rommask; rommask >>= 1)
				{
					if (rombyte & rommask)
						lcd_backbuffer[x + lcdbase] |= lcdmask;
					else
						lcd_backbuffer[x + lcdbase] &= ~lcdmask;
					x++;
				}
			}
		}

		write(lcd, &lcd_backbuffer, sizeof(lcd_backbuffer));

		for (y = 16; y < 56; y += 8)	/* clear rectangle in backbuffer */
			for (x = 1; x < 118; x++)
				lcd_backbuffer[x + (y/8)*120] = 0;

		for (x = 3; x <= 116; x++)
			lcd_backbuffer[x + (39/8)*120] |= 1 << (39%8);

		for (y = 42; y <= 60; y++)
			lcd_backbuffer[35 + (y/8)*120] |= 1 << (y%8);

		for (y = 42; y <= 60; y++)
			lcd_backbuffer[60 + (y/8)*120] |= 1 << (y%8);

		RenderCharLCD(10, 43, 20);
		RenderCharLCD(11, 79, 20);

		return;
	}
	else
	{
		int p;

		if (inputcounter == 2)
			p = page;
		else
			p = temp_page + (0xDD >> 4*(1-inputcounter)); /* partial pageinput (filled with spaces) */

		/* page */
		if (old_page != p)
		{
			RenderCharLCD(p>>8,  7, 20);
			RenderCharLCD((p&0x0F0)>>4, 19, 20);
			RenderCharLCD(p&0x00F, 31, 20);

			old_page = p;
			update_lcd = 1;
		}

		/* current subpage */
		if (old_subpage != subpage)
		{
			if (!subpage)
			{
				RenderCharLCD(0, 55, 20);
				RenderCharLCD(1, 67, 20);
			}
			else
			{
				if (subpage >= 0xFF)
					subpage = 1;
				else if (subpage > 99)
					subpage = 0;

				RenderCharLCD(subpage>>4, 55, 20);
				RenderCharLCD(subpage&0x0F, 67, 20);
			}

			old_subpage = subpage;
			update_lcd = 1;
		}

		/* max subpage */
		for (x = 0; x <= 0x79; x++)
		{
			if (cachetable[page][x] != 0)
				subpage_max = x;
		}

		if (old_subpage_max != subpage_max)
		{
			if (!subpage_max)
			{
				RenderCharLCD(0,  91, 20);
				RenderCharLCD(1, 103, 20);
			}
			else
			{
				RenderCharLCD(subpage_max>>4,  91, 20);
				RenderCharLCD(subpage_max&0x0F, 103, 20);
			}

			old_subpage_max = subpage_max;
			update_lcd = 1;
		}

		/* cachestatus */
		if (old_cached_pages != cached_pages)
		{
			#if 0
			int s;
			int p = cached_pages;
			for (s=107; s >= 107-4*fontwidth_small_lcd; s -= fontwidth_small_lcd)
			{
				int c = p % 10;
				if (p)
					RenderCharLCDsmall('0'+c, s, 44);
				else
					RenderCharLCDsmall(' ', s, 44);
				p /= 10;
			}
			#else
			RenderCharLCD(cached_pages/1000, 67, 44);
			RenderCharLCD(cached_pages%1000/100, 79, 44);
			RenderCharLCD(cached_pages%100/10, 91, 44);
			RenderCharLCD(cached_pages%10, 103, 44);
			#endif

			old_cached_pages = cached_pages;
			update_lcd = 1;
		}

		/* mode */
		if (old_hintmode != hintmode)
		{
			if (hintmode)
				RenderCharLCD(12, 43, 44);
			else
				RenderCharLCD(13, 43, 44);

			old_hintmode = hintmode;
			update_lcd = 1;
		}
	}

	if (update_lcd)
		write(lcd, &lcd_backbuffer, sizeof(lcd_backbuffer));
}

/******************************************************************************
 * DecodePage                                                                 *
 ******************************************************************************/

void DecodePage()
{
	int row, col;
	int hold, clear, loop;
	int foreground, background, doubleheight, charset, mosaictype;
	unsigned char held_mosaic;

	/* copy page to decode buffer */
	if (zap_subpage_manual)
		memcpy(&page_char, cachetable[page][subpage], PAGESIZE);
	else
		memcpy(&page_char, cachetable[page][subpagetable[page]], PAGESIZE);

	/* update timestring */
	memcpy(&page_char[32], &timestring, 8);

	/* check for newsflash & subtitle */
	if (dehamming[page_char[11-6]] & 12 && !screenmode && is_dec(page))
		boxed = 1;
	else
		boxed = 0;

	/* modify header */
	if (boxed)
		memset(&page_char, ' ', 40);
	else
	{
//		memcpy(&page_char, " TuxTxt ", 8);
		memcpy(&page_char, "        ", 8);
		hex2str(page_char+3, page);
		if (subpage != 0)
		{
			hex2str(page_char+6, subpage);
			*(page_char+4) ='/';
		}

	}

	if (!is_dec(page))
	{
		int i;
		unsigned char *p, c, n, h, parityerror = 0;

		/* show (usually nonexistent) page number for hex pages */
//		hex2str(page_char + 8 + 2, page);

		for (i = 0; i < 8 + 3; i++)
			page_atrb[i] = black<<4 | white;

		/* decode parity/hamming */
		for (; i < sizeof(page_char); i++)
		{
			page_atrb[i] = black<<4 | white;
			p = page_char + i;
			h = dehamming[*p];
			if (parityerror && h != 0xFF)	/* if no regular page (after any parity error) */
				hex2str(p, h);	/* first try dehamming */
			else 
			{
				n = 0;
				for (c = *p; c; c &= (c-1)) /* calc parity */
					n ^= 1;
				if (n)
					*p &= 127;
				else
				{
					parityerror = 1;
					if (h != 0xFF)	/* first parity error: try dehamming */
						hex2str(p, h);
					else
						*p = ' ';
				}
			}
		}
		if (parityerror)
		{
			boxed = 0;
			return; /* don't interpret irregular pages */
		}
	}

	/* decode */
	for (row = 0; row < 24; row++)
	{
		/* start-of-row default conditions */
		foreground   = white;
		background   = black;
		doubleheight = 0;
		charset      = 0;
		mosaictype   = 0;
		hold         = 0;
		held_mosaic  = ' ';

		if (boxed && memchr(&page_char[row*40], start_box, 40) == 0)
		{
			foreground = transp;
			background = transp;
		}

		for (col = 0; col < 40; col++)
		{
			int index = row*40 + col;
			
			page_atrb[index] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
			
			if (page_char[index] < ' ')
			{
				switch (page_char[index])
				{
				case alpha_black:
					foreground = black;
					charset = 0;
					break;

				case alpha_red:
					foreground = red;
					charset = 0;
					break;

				case alpha_green:
					foreground = green;
					charset = 0;
					break;

				case alpha_yellow:
					foreground = yellow;
					charset = 0;
					break;

				case alpha_blue:
					foreground = blue;
					charset = 0;
					break;

				case alpha_magenta:
					foreground = magenta;
					charset = 0;
					break;

				case alpha_cyan:
					foreground = cyan;
					charset = 0;
					break;

				case alpha_white:
					foreground = white;
					charset = 0;
					break;

				case flash:
					/* todo */
					break;

				case steady:
					/* todo */
					break;

				case end_box:
					if (boxed)
					{
						foreground = transp;
						background = transp;
					}
					break;

				case start_box:
					if (boxed)
					{
						if (col > 0)
							memset(&page_char[row*40], ' ', col);
						for (clear = 0; clear < col; clear++)
							page_atrb[row*40 + clear] = doubleheight<<10 | transp<<4 | transp;
					}
					break;

				case normal_size:
					doubleheight = 0;
					page_atrb[index] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					break;

				case double_height:
					if (row < 23)
						doubleheight = 1;
					break;

				case double_width:
					/* todo */
					break;

				case double_size:
					/* todo */
					break;

				case mosaic_black:
					foreground = black;
					charset = 1 + mosaictype;
					break;

				case mosaic_red:
					foreground = red;
					charset = 1 + mosaictype;
					break;

				case mosaic_green:
					foreground = green;
					charset = 1 + mosaictype;
					break;

				case mosaic_yellow:
					foreground = yellow;
					charset = 1 + mosaictype;
					break;

				case mosaic_blue:
					foreground = blue;
					charset = 1 + mosaictype;
					break;

				case mosaic_magenta:
					foreground = magenta;
					charset = 1 + mosaictype;
					break;

				case mosaic_cyan:
					foreground = cyan;
					charset = 1 + mosaictype;
					break;

				case mosaic_white:
					foreground = white;
					charset = 1 + mosaictype;
					break;

				case conceal:
					if (!hintmode) 
					{
						foreground = background;
						page_atrb[index] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					}
					break;

				case contiguous_mosaic:
					mosaictype = 0;
					if (charset)
					{
						charset = 1;
						page_atrb[index] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					}
					break;

				case separated_mosaic:
					mosaictype = 1;
					if (charset)
					{
						charset = 2;
						page_atrb[index] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					}
					break;

				case esc:
					/* todo */
					break;

				case black_background:
					background = black;
					page_atrb[index] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					break;

				case new_background:
					background = foreground;
					page_atrb[index] = doubleheight<<10 | charset<<8 | background<<4 | foreground;
					break;

				case hold_mosaic:
					hold = 1;
					break;

				case release_mosaic:
					hold = 2;
					break;
				}

				/* handle spacing attributes */
				if (hold && charset)
					page_char[index] = held_mosaic;
				else
					page_char[index] = ' ';

				if (hold == 2)
					hold = 0;
			}
			else /* char >= ' ' */
			{
				/* set new held-mosaic char */
				if (charset)
					held_mosaic = page_char[index];

				/* skip doubleheight chars in lower line */
				if (doubleheight)
					page_char[index + 40] = 0xFF;
			}
		}

		/* copy attribute to lower line if doubleheight */
		if (memchr(&page_char[(row+1)*40], 0xFF, 40) != 0)
		{
			for (loop = 0; loop < 40; loop++)
				page_atrb[(row+1)*40 + loop] = (page_atrb[row*40 + loop] & 0xF0) | (page_atrb[row*40 + loop] & 0xF0)>>4;

			row++;
		}
	}
}

/******************************************************************************
 * GetRCCode                                                                  *
 ******************************************************************************/

int GetRCCode()
{
#if HAVE_DVB_API_VERSION < 3
	static unsigned short LastKey = -1;
#else
	struct input_event ev;
	static __u16 rc_last_key = KEY_RESERVED;
#endif
	/* get code */
#if HAVE_DVB_API_VERSION < 3
	if (read(rc, &RCCode, 2) == 2)
	{
		if (RCCode != LastKey)
		{
			LastKey = RCCode;

			if ((RCCode & 0xFF00) == 0x5C00)
			{
				switch (RCCode)
#else
	if (read(rc, &ev, sizeof(ev)) == sizeof(ev))
	{
		if (ev.value)
		{
			if (ev.code != rc_last_key)
			{
				rc_last_key = ev.code;
				switch (ev.code)
#endif
				{
				case KEY_UP:		RCCode = RC_UP;		break;
				case KEY_DOWN:		RCCode = RC_DOWN;	break;
				case KEY_LEFT:		RCCode = RC_LEFT;	break;
				case KEY_RIGHT:		RCCode = RC_RIGHT;	break;
				case KEY_OK:		RCCode = RC_OK;		break;
				case KEY_0:		RCCode = RC_0;		break;
				case KEY_1:		RCCode = RC_1;		break;
				case KEY_2:		RCCode = RC_2;		break;
				case KEY_3:		RCCode = RC_3;		break;
				case KEY_4:		RCCode = RC_4;		break;
				case KEY_5:		RCCode = RC_5;		break;
				case KEY_6:		RCCode = RC_6;		break;
				case KEY_7:		RCCode = RC_7;		break;
				case KEY_8:		RCCode = RC_8;		break;
				case KEY_9:		RCCode = RC_9;		break;
				case KEY_RED:		RCCode = RC_RED;	break;
				case KEY_GREEN:		RCCode = RC_GREEN;	break;
				case KEY_YELLOW:	RCCode = RC_YELLOW;	break;
				case KEY_BLUE:		RCCode = RC_BLUE;	break;
				case KEY_VOLUMEUP:	RCCode = RC_PLUS;	break;
				case KEY_VOLUMEDOWN:	RCCode = RC_MINUS;	break;
				case KEY_MUTE:		RCCode = RC_MUTE;	break;
				case KEY_HELP:		RCCode = RC_HELP;	break;
				case KEY_SETUP:		RCCode = RC_DBOX;	break;
				case KEY_HOME:		RCCode = RC_HOME;	break;
				case KEY_POWER:		RCCode = RC_STANDBY;	break;
				}
				return 1;
			}
#if HAVE_DVB_API_VERSION < 3
			else
				RCCode &= 0x003F;
#endif
		}
#if HAVE_DVB_API_VERSION < 3
		else
			RCCode = -1;

		return 1;
#else
		else
		{
			RCCode = -1;
			rc_last_key = KEY_RESERVED;
		}
#endif
	}

	RCCode = -1;
	usleep(1000000/100);

	return 0;
}

/******************************************************************************
 * CacheThread                                                                *
 ******************************************************************************/

void allocate_cache(int magazine)
{
	/* check cachetable and allocate memory if needed */
	if (cachetable[current_page[magazine]][current_subpage[magazine]] == 0)
	{
		cachetable[current_page[magazine]][current_subpage[magazine]] = malloc(PAGESIZE);
		memset(cachetable[current_page[magazine]][current_subpage[magazine]], ' ', PAGESIZE);
		memset(flofpages[current_page[magazine]], 0 , FLOFSIZE);
		cached_pages++;
	}
}

void *CacheThread(void *arg)
{
	const unsigned char rev_lut[32] = {
		0x00,0x08,0x04,0x0c, /*  upper nibble */
		0x02,0x0a,0x06,0x0e,
		0x01,0x09,0x05,0x0d,
		0x03,0x0b,0x07,0x0f,
		0x00,0x80,0x40,0xc0, /*  lower nibble */
		0x20,0xa0,0x60,0xe0,
		0x10,0x90,0x50,0xd0,
		0x30,0xb0,0x70,0xf0 };
	unsigned char pes_packet[184];
	unsigned char vtxt_row[42];
	int line, byte/*, bit*/;
	int b1, b2, b3, b4;
	int packet_number;
	unsigned char magazine;

	while (1)
	{
		/* check stopsignal */
		pthread_testcancel();

		/* read packet */
		ssize_t readcnt;
		readcnt = read(dmx, &pes_packet, sizeof(pes_packet));

		if (readcnt != sizeof(pes_packet))
		{
#if DEBUG
			printf ("TuxTxt: readerror\n");
#endif
			continue;
		}

		/* analyze it */
		for (line = 0; line < 4; line++)
		{
			unsigned char *vtx_rowbyte = &pes_packet[line*0x2e];
			if ((vtx_rowbyte[0] == 0x02 || vtx_rowbyte[0] == 0x03) && (vtx_rowbyte[1] == 0x2C))
			{
				/* clear rowbuffer */
				/* convert row from lsb to msb (begin with magazin number) */
				for (byte = 4; byte < 46; byte++)
				{
					unsigned char upper,lower;
					upper = (vtx_rowbyte[byte] >> 4) & 0xf;
					lower = vtx_rowbyte[byte] & 0xf;
					vtxt_row[byte-4] = (rev_lut[upper]) | (rev_lut[lower+16]);
				}

				/* get packet number */
				b1 = dehamming[vtxt_row[0]];
				b2 = dehamming[vtxt_row[1]];

				if (b1 == 0xFF || b2 == 0xFF)
				{
#if DEBUG
					printf("TuxTxt <Biterror in Packet>\n");
#endif
					continue;
				}

				b1 &= 8;

				packet_number = b1>>3 | b2<<1;

				/* get magazine number */
				magazine = dehamming[vtxt_row[0]] & 7;
				if (!magazine) magazine = 8;

				/* analyze row */
				if (packet_number == 0)
				{
					/* get pagenumber */
					b1 = dehamming[vtxt_row[0]];
					b2 = dehamming[vtxt_row[3]];
					b3 = dehamming[vtxt_row[2]];

					if (b1 == 0xFF || b2 == 0xFF || b3 == 0xFF)
					{
						current_page[magazine] = page_receiving = -1;
#if DEBUG
						printf("TuxTxt <Biterror in Page>\n");
#endif
						continue;
					}

					b1 &= 7;
					if (b1 == 0)
						b1 = 8;
					current_page[magazine] = page_receiving = b1<<8 | b2<<4 | b3;

					if (b2 > 9 || b3 > 9) /* hex page number: just copy page */
					{
						current_subpage[magazine] = 0;
						subpagetable[current_page[magazine]] = 0;
						allocate_cache(magazine); /* FIXME: only until TOP-Info decoded? */
						continue;
					}

					/* get subpagenumber */
					b1 = dehamming[vtxt_row[7]];
					b2 = dehamming[vtxt_row[6]];
					b3 = dehamming[vtxt_row[5]];
					b4 = dehamming[vtxt_row[4]];

					if (b1 == 0xFF || b2 == 0xFF || b3 == 0xFF || b4 == 0xFF)
					{
						current_subpage[magazine] = -1;
#if DEBUG
						printf("TuxTxt <Biterror in SubPage>\n");
#endif
						continue;
					}

					b1 &= 3;
					b3 &= 7;

					if (b1 != 0 || b2 != 0 || b4 > 9)
					{
						current_subpage[magazine] = -1;
						continue;
					}
					else
						current_subpage[magazine] = b3<<4 | b4;

					/* get country control bits */
					b1 = dehamming[vtxt_row[9]];

					if (b1 == 0xFF)
					{
						countrycontrolbitstable[current_page[magazine]][current_subpage[magazine]] = 0xff;
#if DEBUG
						printf("TuxTxt <Biterror in CountryFlags>\n");
#endif
					}
					else
						countrycontrolbitstable[current_page[magazine]][current_subpage[magazine]] =
							((b1 >> 3) & 0x01) | (((b1 >> 2) & 0x01) << 1) | (((b1 >> 1) & 0x01) << 2);

					allocate_cache(magazine);

					/* store current subpage for this page */
					subpagetable[current_page[magazine]] = current_subpage[magazine];

					/* check parity */
					for (byte = 10; byte < 42; byte++)
					{
						if ((vtxt_row[byte]&1) ^ ((vtxt_row[byte]>>1)&1) ^
						    ((vtxt_row[byte]>>2)&1) ^ ((vtxt_row[byte]>>3)&1) ^
						    ((vtxt_row[byte]>>4)&1) ^ ((vtxt_row[byte]>>5)&1) ^
						    ((vtxt_row[byte]>>6)&1) ^ (vtxt_row[byte]>>7))
							vtxt_row[byte] &= 127;
						else
							vtxt_row[byte] = ' ';
					}

					/* copy timestring */
					memcpy(&timestring, &vtxt_row[34], 8);

					/* set update flag */
					if (current_page[magazine] == page)
					{
						pageupdate = 1;
						if (!zap_subpage_manual)
							subpage = current_subpage[magazine];
					}

					/* check controlbits */
					if (dehamming[vtxt_row[5]] & 8)   /* C4 -> erase page */
						memset(cachetable[current_page[magazine]][current_subpage[magazine]], ' ', PAGESIZE);
				}
				else if (packet_number <= 24)
				{
					if ((current_page[magazine] & 0x0F0) <= 0x090 &&
					    (current_page[magazine] & 0x00F) <= 0x009)
					{    /* no parity check for TOP pages, just copy */

						/* check parity */
						for (byte = 2; byte < 42; byte++)
						{
							if ((vtxt_row[byte]&1) ^ ((vtxt_row[byte]>>1)&1) ^
							    ((vtxt_row[byte]>>2)&1) ^ ((vtxt_row[byte]>>3)&1) ^
							    ((vtxt_row[byte]>>4)&1) ^ ((vtxt_row[byte]>>5)&1) ^
							    ((vtxt_row[byte]>>6)&1) ^ (vtxt_row[byte]>>7))
								vtxt_row[byte] &= 127;
							else
								vtxt_row[byte] = ' ';
						}
					}
				}
				else if ((packet_number == 27) && (dehamming[vtxt_row[2]] == 0)) // reading FLOF-Pagelinks
				{
					if (current_page[magazine] != -1)
					{
						b1 = dehamming[vtxt_row[0]];
						if (b1 != 0xff)
						{
							b1 &= 7;

							for (byte = 0; byte < FLOFSIZE; byte++)
							{
								b2 = dehamming[vtxt_row[4+byte*6]];
								b3 = dehamming[vtxt_row[3+byte*6]];

								if (b2 != 0xff && b3 != 0xff)
								{
									b4 =  ((b1 &4) ^((dehamming[vtxt_row[8+byte*6]]>>1) & 4)) |
										  ((b1 &2) ^((dehamming[vtxt_row[8+byte*6]]>>1) & 2)) |
										  ((b1 &1) ^((dehamming[vtxt_row[6+byte*6]]>>3) & 1));
									if (b4 == 0)
										b4 = 8;
									if (b2 <= 9 && b3 <= 9)
										flofpages[current_page[magazine] ][byte] = b4<<8 | b2<<4 | b3;
								}
							}
						}

					}
				}

				/* copy row to pagebuffer */
				if (current_page[magazine] != -1 && current_subpage[magazine] != -1 &&
				    packet_number <= 24 && cachetable[current_page[magazine]][current_subpage[magazine]]) /* avoid segfault */
				{
					memcpy(cachetable[current_page[magazine]][current_subpage[magazine]] + packet_number*40, &vtxt_row[2], 40);

					/* set update flag */
					if (current_page[magazine] == page)
					{
						pageupdate = 1;
						if (!zap_subpage_manual)
							subpage = current_subpage[magazine];
					}
				}
			}
		}
	}
	return 0;
}

/* Local Variables: */
/* indent-tabs-mode:t */
/* tab-width:3 */
/* c-basic-offset:3 */
/* comment-column:0 */
/* fill-column:120 */
/* End: */
