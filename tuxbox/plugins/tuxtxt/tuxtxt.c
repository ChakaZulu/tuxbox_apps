/******************************************************************************
 *                      <<< TuxTxt - Teletext Plugin >>>                      *
 *                                                                            *
 *             (c) Thomas "LazyT" Loewe 2002-2003 (LazyT@gmx.net)             *
 *                                                                            *
 *    continued 2004-2005 by Roland Meier <RolandMeier@Siemens.com>           *
 *                       and DBLuelle <dbluelle@blau-weissoedingen.de>        *
 *                                                                            *
 ******************************************************************************/



#include "tuxtxt.h"

void FillBorder(int color)
{
	int ys =  (var_screeninfo.yres-var_screeninfo.yoffset);
	tuxtxt_FillRect(lfb,var_screeninfo.xres,0     , ys                     ,StartX      ,var_screeninfo.yres                       ,color);
	tuxtxt_FillRect(lfb,var_screeninfo.xres,StartX, ys                     ,displaywidth,StartY                                    ,color);
	tuxtxt_FillRect(lfb,var_screeninfo.xres,StartX, ys+StartY+25*fontheight,displaywidth,var_screeninfo.yres-(StartY+25*fontheight),color);

	if (screenmode == 0 )
		tuxtxt_FillRect(lfb,var_screeninfo.xres,StartX+displaywidth, ys,var_screeninfo.xres-(StartX+displaywidth),var_screeninfo.yres   ,color);
}

int getIndexOfPageInHotlist()
{
	int i;
	for (i = 0; i <= maxhotlist; i++)
	{
		if (tuxtxt_cache.page == hotlist[i])
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
	sprintf(line, CONFIGDIR "/tuxtxt/hotlist%d.conf", tuxtxt_cache.vtxtpid);
#if TUXTXT_DEBUG
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
#if TUXTXT_DEBUG
					printf(" %03x", hotlist[maxhotlist+1]);
#endif
					maxhotlist++;
					continue;
				}
			}
#if TUXTXT_DEBUG
			else
				printf(" ?%s?", line);
#endif
		} while (maxhotlist < (sizeof(hotlist)/sizeof(hotlist[0])-1));
		fclose(hl);
	}
#if TUXTXT_DEBUG
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
	sprintf(line, CONFIGDIR "/tuxtxt/hotlist%d.conf", tuxtxt_cache.vtxtpid);
#if TUXTXT_DEBUG
	printf("TuxTxt <savehotlist %s", line);
#endif
	if (maxhotlist != 1 || hotlist[0] != 0x100 || hotlist[1] != 0x303)
	{
		if ((hl = fopen(line, "wb")) != 0)
		{
			for (i = 0; i <= maxhotlist; i++)
			{
				fprintf(hl, "%03x\n", hotlist[i]);
#if TUXTXT_DEBUG
				printf(" %03x", hotlist[i]);
#endif
			}
			fclose(hl);
		}
	}
	else
	{
		unlink(line); /* remove current hotlist file */
#if TUXTXT_DEBUG
		printf(" (default - just deleted)");
#endif
	}
#if TUXTXT_DEBUG
	printf(">\n");
#endif
}



int toptext_getnext(int startpage, int up, int findgroup)
{
	int current, nextgrp, nextblk;

	int stoppage =  (tuxtxt_is_dec(startpage) ? startpage : startpage & 0xF00); // avoid endless loop in hexmode
	nextgrp = nextblk = 0;
	current = startpage;

	do {
		if (up)
			tuxtxt_next_dec(&current);
		else
			tuxtxt_prev_dec(&current);

		if (!tuxtxt_cache.bttok || tuxtxt_cache.basictop[current]) /* only if existent */
		{
			if (findgroup)
			{
				if (tuxtxt_cache.basictop[current] >= 6 && tuxtxt_cache.basictop[current] <= 7)
					return current;
				if (!nextgrp && (current&0x00F) == 0)
					nextgrp = current;
			}
			if (tuxtxt_cache.basictop[current] >= 2 && tuxtxt_cache.basictop[current] <= 5) /* always find block */
				return current;

			if (!nextblk && (current&0x0FF) == 0)
				nextblk = current;
		}
	} while (current != stoppage);

	if (nextgrp)
		return nextgrp;
	else if (nextblk)
		return nextblk;
	else
		return current;
}

void RenderClearMenuLineBB(char *p, tstPageAttr *attrcol, tstPageAttr *attr)
{
	int col;

	PosX = TOPMENUSTARTX;
	RenderCharBB(' ', attrcol); /* indicator for navigation keys */
#if 0
	RenderCharBB(' ', attr); /* separator */
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
	memset(lfb + (var_screeninfo.yres-var_screeninfo.yoffset)*var_screeninfo.xres, color, var_screeninfo.xres*var_screeninfo.yres);
}

void ClearFB(int color)
{
	memset(lfb + var_screeninfo.xres*var_screeninfo.yoffset, color, var_screeninfo.xres*var_screeninfo.yres);
}

void ClearB(int color)
{
	memset(lfb, color, 2*var_screeninfo.xres*var_screeninfo.yres);
}


int  GetCurFontWidth()
{
	int mx = (displaywidth)%(40-nofirst); // # of unused pixels
	int abx = (mx == 0 ? displaywidth+1 : (displaywidth)/(mx+1));// distance between 'inserted' pixels
	int nx= abx+1-((PosX-sx) % (abx+1)); // # of pixels to next insert
	return fontwidth+(((PosX+fontwidth+1-sx) <= displaywidth && nx <= fontwidth+1) ? 1 : 0);
}

void SetPosX(int column)
{
		PosX = StartX;
		int i;
		for (i = 0; i < column-nofirst; i++)
			PosX += GetCurFontWidth();
}

void setfontwidth(int newwidth)
{
	if (fontwidth != newwidth)
	{
		int i;
		fontwidth = newwidth;
		if (usettf)
			typettf.font.pix_width  = (FT_UShort) fontwidth * TTFWidthFactor16 / 16;
		else
		{
			if (newwidth < 11)
				newwidth = 21;
			else if (newwidth < 14)
				newwidth = 22;
			else
				newwidth = 23;
			typettf.font.pix_width  = typettf.font.pix_height = (FT_UShort) newwidth;
		}
		for (i = 0; i <= 12; i++)
			axdrcs[i] = (fontwidth * i + 6) / 12;
	}
}


void setcolors(unsigned short *pcolormap, int offset, int number)
{
	int i, changed=0;
	int j = offset; /* index in global color table */

	unsigned short t = tr0[transp2];
	tr0[transp2] = (trans_mode+7)<<11 | 0x7FF;
#ifndef HAVE_DREAMBOX_HARDWARE
	/* "correct" semi-transparent for Nokia (GTX only allows 2(?) levels of transparency) */
	if (tuxbox_get_vendor() == TUXBOX_VENDOR_NOKIA)
		tr0[transp2] = 0xFFFF;
#endif
	if (t != tr0[transp2]) changed = 1;
	for (i = 0; i < number; i++)
	{
		int r = (pcolormap[i] << 12) & 0xf000;
		int g = (pcolormap[i] <<  8) & 0xf000;
		int b = (pcolormap[i] <<  4) & 0xf000;


		r = (r * (0x3f+(color_mode<<3))) >> 8;
		g = (g * (0x3f+(color_mode<<3))) >> 8;
		b = (b * (0x3f+(color_mode<<3))) >> 8;
		if (rd0[j] != r)
		{
			rd0[j] = r;
			changed = 1;
		}
		if (gn0[j] != g)
		{
			gn0[j] = g;
			changed = 1;
		}
		if (bl0[j] != b)
		{
			bl0[j] = b;
			changed = 1;
		}
		j++;
	}
	if (changed)
		if (ioctl(fb, FBIOPUTCMAP, &colormap_0) == -1)
			perror("TuxTxt <FBIOPUTCMAP>");
}


/* hexdump of page contents to stdout for debugging */
void dump_page()
{
	int r, c;
	char *p;
	unsigned char pagedata[23*40];

	if (!tuxtxt_cache.astCachetable[tuxtxt_cache.page][tuxtxt_cache.subpage])
		return;
	tuxtxt_decompress_page(tuxtxt_cache.page,tuxtxt_cache.subpage,pagedata);
	for (r=1; r < 24; r++)
	{
		p = pagedata+40*(r-1);
		for (c=0; c < 40; c++)
			printf(" %02x", *p++);
		printf("\n");
		p = page_char + 40*r;
		for (c=0; c < 40; c++)
			printf("  %c", *p++);
		printf("\n");
	}
}



/******************************************************************************
 * plugin_exec                                                                *
 ******************************************************************************/

void plugin_exec(PluginParam *par)
{
	char cvs_revision[] = "$Revision: 1.99 $";

#if !TUXTXT_CFG_STANDALONE
	int initialized = tuxtxt_init();
	if ( initialized )
		tuxtxt_cache.page = 0x100;
#endif

	/* show versioninfo */
	sscanf(cvs_revision, "%*s %s", versioninfo);
	printf("TuxTxt %s\n", versioninfo);

	/* get params */
	tuxtxt_cache.vtxtpid = fb = lcd = rc = sx = ex = sy = ey = -1;

	for (; par; par = par->next)
	{
		if (!strcmp(par->id, P_ID_VTXTPID))
			tuxtxt_cache.vtxtpid = atoi(par->val);
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

	if (tuxtxt_cache.vtxtpid == -1 || fb == -1 || rc == -1 || sx == -1 || ex == -1 || sy == -1 || ey == -1)
	{
		printf("TuxTxt <Invalid Param(s)>\n");
		return;
	}

	/* initialisations */
	if (Init() == 0){
#if !TUXTXT_CFG_STANDALONE
		if ( initialized ){
			tuxtxt_close();
		}
#endif
		return;
	}
	
	/* main loop */
	do {
		if (GetRCCode() == 1)
		{
			if (transpmode == 2) /* TV mode */
			{
				switch (RCCode)
				{
//#if TUXTXT_DEBUG /* FIXME */
				case RC_OK:
					if (showhex)
					{
						dump_page(); /* hexdump of page contents to stdout for debugging */
					}
					continue; /* otherwise ignore key */
//#endif /* TUXTXT_DEBUG */
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

				case RC_HELP: /* switch to scart input and back */
				{
					int i, n;
#ifndef HAVE_DREAMBOX_HARDWARE
					int vendor = tuxbox_get_vendor() - 1;
#else
					int vendor = 3; /* values unknown, rely on requested values */
#endif

					if (vendor < 3) /* scart-parameters only known for 3 dboxes, FIXME: order must be like in info.h */
					{
						for (i = 0; i < 6; i++) /* FIXME: FBLK seems to cause troubles */
						{
							if (!restoreaudio || !(i & 1)) /* not for audio if scart-audio active */
							{
								if ((ioctl(avs, avstable_ioctl_get[i], &n)) < 0) /* get current values for restoration */
									perror("TuxTxt <ioctl(avs)>");
								else
									avstable_dvb[vendor][i] = n;
							}

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
				default:
					continue; /* ignore all other keys */
				}
			}

			switch (RCCode)
			{
			case RC_UP:
				GetNextPageOne(!swapupdown);
				break;
			case RC_DOWN:
				GetNextPageOne(swapupdown);
				break;
			case RC_RIGHT:	
				if (boxed)
				{
				    subtitledelay++;				    
		    		    // display subtitledelay
				    PosY = StartY;
				    char ns[10];
				    SetPosX(1);
				    sprintf(ns,"+%d    ",subtitledelay);
				    RenderCharFB(ns[0],&atrtable[ATR_WB]);
				    RenderCharFB(ns[1],&atrtable[ATR_WB]);
				    RenderCharFB(ns[2],&atrtable[ATR_WB]);
				    RenderCharFB(ns[4],&atrtable[ATR_WB]);					    
				}
				else
    				    GetNextSubPage(1);	
				break;
			case RC_LEFT:
				if (boxed)
				{
				    subtitledelay--;
				    if (subtitledelay < 0) subtitledelay = 0;
		    		    // display subtitledelay
				    PosY = StartY;
				    char ns[10];
				    SetPosX(1);
				    sprintf(ns,"+%d    ",subtitledelay);
				    RenderCharFB(ns[0],&atrtable[ATR_WB]);
				    RenderCharFB(ns[1],&atrtable[ATR_WB]);
				    RenderCharFB(ns[2],&atrtable[ATR_WB]);
				    RenderCharFB(ns[4],&atrtable[ATR_WB]);					    
				}
				else
				    GetNextSubPage(-1);	
				break;
			case RC_OK:
				if (tuxtxt_cache.subpagetable[tuxtxt_cache.page] == 0xFF)
					continue;
				PageCatching();
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
				PageInput(RCCode - RC_0);
				break;
			case RC_RED:	 ColorKey(prev_100);		break;
			case RC_GREEN:	 ColorKey(prev_10);		break;
			case RC_YELLOW: ColorKey(next_10);		break;
			case RC_BLUE:	 ColorKey(next_100);		break;
			case RC_PLUS:	 SwitchZoomMode();		break;
			case RC_MINUS:	 SwitchScreenMode(-1);prevscreenmode = screenmode; break;
			case RC_MUTE:	 SwitchTranspMode();	break;
			case RC_HELP:	 SwitchHintMode();		break;
			case RC_DBOX:	 ConfigMenu(0);			break;
			}
		}

		/* update page or timestring and lcd */
		RenderPage();
	} while ((RCCode != RC_HOME) && (RCCode != RC_STANDBY));

	/* exit */
	CleanUp();

#if !TUXTXT_CFG_STANDALONE
	if ( initialized )
		tuxtxt_close();
#endif

 	printf("Tuxtxt: plugin ended\n");

}

/******************************************************************************
 * MyFaceRequester
 ******************************************************************************/

FT_Error MyFaceRequester(FTC_FaceID face_id, FT_Library library, FT_Pointer request_data, FT_Face *aface)
{
	FT_Error result;

	result = FT_New_Face(library, face_id, 0, aface);

#if TUXTXT_DEBUG
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
	int error, i;
	unsigned char magazine;

	/* init data */


 	//page_atrb[32] = transp<<4 | transp;
	inputcounter  = 2;

	for (magazine = 1; magazine < 9; magazine++)
	{
		tuxtxt_cache.current_page  [magazine] = -1;
		tuxtxt_cache.current_subpage [magazine] = -1;
	}
#if TUXTXT_CFG_STANDALONE
/* init data */
	memset(&tuxtxt_cache.astCachetable, 0, sizeof(tuxtxt_cache.astCachetable));
	memset(&tuxtxt_cache.subpagetable, 0xFF, sizeof(tuxtxt_cache.subpagetable));
	memset(&tuxtxt_cache.astP29, 0, sizeof(tuxtxt_cache.astP29));

	memset(&tuxtxt_cache.basictop, 0, sizeof(tuxtxt_cache.basictop));
	memset(&tuxtxt_cache.adip, 0, sizeof(tuxtxt_cache.adip));
	memset(&tuxtxt_cache.flofpages, 0 , sizeof(tuxtxt_cache.flofpages));
	memset(subtitlecache,0,sizeof(subtitlecache));
	tuxtxt_cache.maxadippg  = -1;
	tuxtxt_cache.bttok      = 0;
	maxhotlist = -1;

	//page_atrb[32] = transp<<4 | transp;
	inputcounter  = 2;
	tuxtxt_cache.cached_pages  = 0;

	tuxtxt_cache.page_receiving = -1;
	tuxtxt_cache.page       = 0x100;
#endif
	lastpage   = tuxtxt_cache.page;
	prev_100   = 0x100;
	prev_10    = 0x100;
	next_100   = 0x100;
	next_10    = 0x100;
	tuxtxt_cache.subpage    = tuxtxt_cache.subpagetable[tuxtxt_cache.page];
	if (tuxtxt_cache.subpage == 0xff)
	tuxtxt_cache.subpage    = 0;
	
	tuxtxt_cache.pageupdate = 0;

	tuxtxt_cache.zap_subpage_manual = 0;
	pageinfo = NULL;
	boxed = 0;


	subtitledelay = 0;
	delaystarted = 0;

	/* init lcd */
	UpdateLCD();

	/* config defaults */
	screenmode = 0;
	screen_mode1 = 0;
	screen_mode2 = 0;
	color_mode   = 10;
	trans_mode   = 10;
	menulanguage = 0;	/* german */
	tuxtxt_cache.national_subset = 0;/* default */
	auto_national   = 1;
	swapupdown      = 0;
	showhex         = 0;
	showflof        = 1;
	show39          = 1;
	showl25         = 1;
	dumpl25         = 0;
	usettf          = 0;
	TTFWidthFactor16  = 28;
	TTFHeightFactor16 = 16;
	TTFShiftX         = 0;
	TTFShiftY         = 0;

	/* load config */
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

			if (1 == sscanf(line, "ScreenMode16x9Normal %i", &ival))
				screen_mode1 = ival & 1;
			else if (1 == sscanf(line, "ScreenMode16x9Divided %i", &ival))
				screen_mode2 = ival & 1;
			else if (1 == sscanf(line, "Brightness %i", &ival))
				color_mode = ival;
			else if (1 == sscanf(line, "AutoNational %i", &ival))
				auto_national = ival & 1;
			else if (1 == sscanf(line, "NationalSubset %i", &ival))
			{
				if (ival >= 0 && ival <= MAX_NATIONAL_SUBSET)
					tuxtxt_cache.national_subset = ival;
			}
			else if (1 == sscanf(line, "MenuLanguage %i", &ival))
			{
				if (ival >= 0 && ival <= MAXMENULANGUAGE)
					menulanguage = ival;
			}
			else if (1 == sscanf(line, "SwapUpDown %i", &ival))
				swapupdown = ival & 1;
			else if (1 == sscanf(line, "ShowHexPages %i", &ival))
				showhex = ival & 1;
			else if (1 == sscanf(line, "Transparency %i", &ival))
				trans_mode = ival;
			else if (1 == sscanf(line, "TTFWidthFactor16 %i", &ival))
	            TTFWidthFactor16 = ival;
			else if (1 == sscanf(line, "TTFHeightFactor16 %i", &ival))
	            TTFHeightFactor16 = ival;
			else if (1 == sscanf(line, "TTFShiftX %i", &ival))
	            TTFShiftX = ival;
			else if (1 == sscanf(line, "TTFShiftY %i", &ival))
	            TTFShiftY = ival;
			else if (1 == sscanf(line, "Screenmode %i", &ival))
	            screenmode = ival;
			else if (1 == sscanf(line, "ShowFLOF %i", &ival))
	            showflof = ival & 1;
			else if (1 == sscanf(line, "Show39 %i", &ival))
	            show39 = ival & 1;
			else if (1 == sscanf(line, "ShowLevel2p5 %i", &ival))
	            showl25 = ival & 1;
			else if (1 == sscanf(line, "DumpLevel2p5 %i", &ival))
	            dumpl25 = ival & 1;
			else if (1 == sscanf(line, "UseTTF %i", &ival))
	            usettf = ival & 1;
		}
		fclose(conf);
	}
	saveconfig = 0;
	savedscreenmode = screenmode;
	tuxtxt_cache.national_subset_secondary = NAT_DEFAULT;


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

	fontwidth = 0;	/* initialize at first setting */


	/* calculate font dimensions */
	displaywidth = (ex-sx);
	fontheight = (ey-sy) / 25; //21;
	fontwidth_normal = (ex-sx) / 40;
	setfontwidth(fontwidth_normal);
	fontwidth_topmenumain = (TV43STARTX-sx) / 40;
	fontwidth_topmenusmall = (ex- TOPMENUSTARTX) / TOPMENUCHARS;
	fontwidth_small = (TV169FULLSTARTX-sx)  / 40;
	{
		int i;
		for (i = 0; i <= 10; i++)
			aydrcs[i] = (fontheight * i + 5) / 10;
	}

	/* center screen */
	StartX = sx; //+ (((ex-sx) - 40*fontwidth) / 2);
	StartY = sy + (((ey-sy) - 25*fontheight) / 2);

	if (usettf)
	{
		typettf.font.face_id = (FTC_FaceID) TUXTXTTTFVAR;
		typettf.font.pix_height = (FT_UShort) fontheight * TTFHeightFactor16 / 16;
	}
	else
	{
		typettf.font.face_id = (FTC_FaceID) TUXTXTOTBVAR;
		typettf.font.pix_width  = (FT_UShort) 23;
		typettf.font.pix_height = (FT_UShort) 23;
	}

#if HAVE_DVB_API_VERSION >= 3
	typettf.flags = FT_LOAD_MONOCHROME;
#else
	typettf.image_type = ftc_image_mono;
#endif
	if ((error = FTC_Manager_Lookup_Face(manager, typettf.font.face_id, &face)))
	{
		typettf.font.face_id = (usettf ? (FTC_FaceID) TUXTXTTTF : TUXTXTOTB);
		if ((error = FTC_Manager_Lookup_Face(manager, typettf.font.face_id, &face)))
		{
			printf("TuxTxt <FTC_Manager_Lookup_Face failed with Errorcode 0x%.2X>\n", error);
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			return 0;
		}
	}
	ascender = (usettf ? fontheight * face->ascender / face->units_per_EM : 16);
#if TUXTXT_DEBUG
	printf("TuxTxt <fh%d fw%d fs%d tm%d ts%d ym%d %d %d sx%d sy%d a%d>\n",
			 fontheight, fontwidth, fontwidth_small, fontwidth_topmenumain, fontwidth_topmenusmall,
			 ymosaic[0], ymosaic[1], ymosaic[2], StartX, StartY, ascender);
#endif

	/* get fixed screeninfo */
	if (ioctl(fb, FBIOGET_FSCREENINFO, &fix_screeninfo) == -1)
	{
		perror("TuxTxt <FBIOGET_FSCREENINFO>");
		FTC_Manager_Done(manager);
		FT_Done_FreeType(library);
		return 0;
	}

	/* get variable screeninfo */
	if (ioctl(fb, FBIOGET_VSCREENINFO, &var_screeninfo) == -1)
	{
		perror("TuxTxt <FBIOGET_VSCREENINFO>");
		FTC_Manager_Done(manager);
		FT_Done_FreeType(library);
		return 0;
	}

	/* set variable screeninfo for double buffering */
	var_screeninfo.yres_virtual = 2*var_screeninfo.yres;
	var_screeninfo.xres_virtual = var_screeninfo.xres;
	var_screeninfo.yoffset      = 0;

	if (ioctl(fb, FBIOPUT_VSCREENINFO, &var_screeninfo) == -1)
	{
		perror("TuxTxt <FBIOPUT_VSCREENINFO>");
		FTC_Manager_Done(manager);
		FT_Done_FreeType(library);
		return 0;
	}

#if TUXTXT_DEBUG
	if (ioctl(fb, FBIOGET_VSCREENINFO, &var_screeninfo) == -1)
	{
		perror("TuxTxt <FBIOGET_VSCREENINFO>");
		FTC_Manager_Done(manager);
		FT_Done_FreeType(library);
		return 0;
	}

	printf("TuxTxt <screen real %d*%d, virtual %d*%d, offset %d>\n",
	       var_screeninfo.xres, var_screeninfo.yres,
	       var_screeninfo.xres_virtual, var_screeninfo.yres_virtual,
	       var_screeninfo.yoffset);
#endif


	/* set new colormap */
	setcolors((unsigned short *)tuxtxt_defaultcolors, 0, SIZECOLTABLE);

	/* map framebuffer into memory */
	lfb = (unsigned char*)mmap(0, fix_screeninfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);

	if (!lfb)
	{
		perror("TuxTxt <mmap>");
		FTC_Manager_Done(manager);
		FT_Done_FreeType(library);
		return 0;
	}
	ClearBB(transp); /* initialize backbuffer */
	for (i = 0; i < 40 * 25; i++)
	{
		page_char[i] = ' ';
		page_atrb[i].fg = transp;
		page_atrb[i].bg = transp;
		page_atrb[i].charset = C_G0P;
		page_atrb[i].doubleh = 0;
		page_atrb[i].doublew = 0;
		page_atrb[i].IgnoreAtBlackBgSubst = 0;
	}
	/*  if no vtxtpid for current service, search PIDs */
	if (tuxtxt_cache.vtxtpid == 0)
	{
		/* get all vtxt-pids */
		getpidsdone = -1;						 /* don't kill thread */
		if (GetTeletextPIDs() == 0)
		{
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return 0;
		}

		if (auto_national)
			tuxtxt_cache.national_subset = pid_table[0].national_subset;
		if (pids_found > 1)
			ConfigMenu(1);
		else
		{
			tuxtxt_cache.vtxtpid = pid_table[0].vtxt_pid;
			current_service = 0;
			RenderMessage(ShowServiceName);
		}
	}
	else
	{
		SDT_ready = 0;
		getpidsdone = 0;
//		tuxtxt_cache.pageupdate = 1; /* force display of message page not found (but not twice) */

	}
#if TUXTXT_CFG_STANDALONE
	tuxtxt_init_demuxer();
	tuxtxt_start_thread();
#else
	tuxtxt_start(tuxtxt_cache.vtxtpid);
#endif

	/* open avs */
	if ((avs = open(AVS, O_RDWR)) == -1)
	{
		perror("TuxTxt <open AVS>");
		FTC_Manager_Done(manager);
		FT_Done_FreeType(library);
		return 0;
	}

	ioctl(avs, AVSIOGSCARTPIN8, &fnc_old);
	ioctl(avs, AVSIOSSCARTPIN8, &fncmodes[screen_mode1]);

	/* open saa */
	if ((saa = open(SAA, O_RDWR)) == -1)
	{
		perror("TuxTxt <open SAA>");
		FTC_Manager_Done(manager);
		FT_Done_FreeType(library);
		return 0;
	}

	ioctl(saa, SAAIOGWSS, &saa_old);
	ioctl(saa, SAAIOSWSS, &saamodes[screen_mode1]);

	/* open pig */
	if ((pig = open(PIG, O_RDWR)) == -1)
	{
		perror("TuxTxt <open PIG>");
		FTC_Manager_Done(manager);
		FT_Done_FreeType(library);
		return 0;
	}

	/* setup rc */
	fcntl(rc, F_SETFL, O_NONBLOCK);
	ioctl(rc, RC_IOCTL_BCODES, 1);




	gethotlist();
	SwitchScreenMode(screenmode);
	prevscreenmode = screenmode;
	
	printf("TuxTxt: init ok\n");

	/* init successfull */
	return 1;
}

/******************************************************************************
 * Cleanup                                                                    *
 ******************************************************************************/

void CleanUp()
{
	int i, n, curscreenmode = screenmode;

	/* hide and close pig */
	if (screenmode)
		SwitchScreenMode(0); /* turn off divided screen */
	close(pig);

#if TUXTXT_CFG_STANDALONE
	tuxtxt_stop_thread();
	tuxtxt_clear_cache();
	if (tuxtxt_cache.dmx != -1)
    	    close(tuxtxt_cache.dmx);
	tuxtxt_cache.dmx = -1;
#else
	tuxtxt_stop();
#endif
	/* restore videoformat */
	ioctl(avs, AVSIOSSCARTPIN8, &fnc_old);
	ioctl(saa, SAAIOSWSS, &saa_old);

	if (restoreaudio)
	{
#ifndef HAVE_DREAMBOX_HARDWARE
		int vendor = tuxbox_get_vendor() - 1;
#else
		int vendor = 3; /* values unknown, rely on requested values */
#endif
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
	/* clear subtitlecache */
	for (i = 0; i < SUBTITLE_CACHESIZE; i++)
	{
		if (subtitlecache[i])
			free(subtitlecache[i]);
	}

	if (var_screeninfo.yoffset)
	{
		var_screeninfo.yoffset = 0;

		if (ioctl(fb, FBIOPAN_DISPLAY, &var_screeninfo) == -1)
			perror("TuxTxt <FBIOPAN_DISPLAY>");
	}
	 /* close avs */
	close(avs);

	/* close saa */
	close(saa);


	/* close freetype */
	FTC_Manager_Done(manager);
	FT_Done_FreeType(library);

	/* unmap framebuffer */
	munmap(lfb, fix_screeninfo.smem_len);


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
			fprintf(conf, "Brightness %d\n", color_mode);
			fprintf(conf, "MenuLanguage %d\n", menulanguage);
			fprintf(conf, "AutoNational %d\n", auto_national);
			fprintf(conf, "NationalSubset %d\n", tuxtxt_cache.national_subset);
			fprintf(conf, "SwapUpDown %d\n", swapupdown);
			fprintf(conf, "ShowHexPages %d\n", showhex);
			fprintf(conf, "Transparency 0x%X\n", trans_mode);
			fprintf(conf, "TTFWidthFactor16 %d\n", TTFWidthFactor16);
			fprintf(conf, "TTFHeightFactor16 %d\n", TTFHeightFactor16);
			fprintf(conf, "TTFShiftX %d\n", TTFShiftX);
			fprintf(conf, "TTFShiftY %d\n", TTFShiftY);
			fprintf(conf, "Screenmode %d\n", curscreenmode);
			fprintf(conf, "ShowFLOF %d\n", showflof);
			fprintf(conf, "Show39 %d\n", show39);
			fprintf(conf, "ShowLevel2p5 %d\n", showl25);
			fprintf(conf, "DumpLevel2p5 %d\n", dumpl25);
			fprintf(conf, "UseTTF %d\n", usettf);
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
	int pat_scan, pmt_scan, sdt_scan, desc_scan, pid_test, byte, diff, first_sdt_sec;

	unsigned char PAT[1024];
	unsigned char SDT[1024];
	unsigned char PMT[1024];
	int dmx;


	/* open demuxer */
	if ((dmx = open(DMX, O_RDWR)) == -1)
	{
		perror("TuxTxt <open DMX>");
		return 0;
	}
	if (ioctl(dmx, DMX_SET_BUFFER_SIZE, 64*1024) < 0)
	{
		perror("Tuxtxt <DMX_SET_BUFFERSIZE>");
		close(dmx);
		return 0;
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
		ioctl(dmx, DMX_STOP);
		close(dmx);
		return 0;
	}

	if (read(dmx, PAT, sizeof(PAT)) == -1)
	{
		perror("TuxTxt <read PAT>");
		ioctl(dmx, DMX_STOP);
		close(dmx);
		return 0;
	}

	/* scan each PMT for vtxt-pid */
	pids_found = 0;

	for (pat_scan = 0x0A; pat_scan < 0x0A + (((PAT[0x01]<<8 | PAT[0x02]) & 0x0FFF) - 9); pat_scan += 4)
	{
#if TUXTXT_DEBUG
		printf("PAT liefert:%04x, %04x \n",((PAT[pat_scan - 2]<<8) | (PAT[pat_scan - 1])),(PAT[pat_scan]<<8 | PAT[pat_scan+1]) & 0x1FFF);
#endif
		if (((PAT[pat_scan - 2]<<8) | (PAT[pat_scan - 1])) == 0)
			continue;
// workaround for Dreambox PMT "Connection timed out"-problem (not very nice, but it works...)
#ifdef HAVE_DREAMBOX_HARDWARE
		ioctl(dmx, DMX_STOP);
		close(dmx);
		if ((dmx = open(DMX, O_RDWR)) == -1)
		{
			perror("TuxTxt <open DMX>");
			return 0;
		}
#endif
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
		for (pmt_scan = 0x0C + ((PMT[0x0A]<<8 | PMT[0x0B]) & 0x0FFF);
			  pmt_scan < (((PMT[0x01]<<8 | PMT[0x02]) & 0x0FFF) - 7);
			  pmt_scan += 5 + PMT[pmt_scan + 4])
		{
			if (PMT[pmt_scan] == 6)
			{
				for (desc_scan = pmt_scan + 5;
					  desc_scan < pmt_scan + ((PMT[pmt_scan + 3]<<8 | PMT[pmt_scan + 4]) & 0x0FFF) + 5;
					  desc_scan += 2 + PMT[desc_scan + 1])
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
							pid_table[pids_found].national_subset = NAT_DEFAULT; /* use default charset */
						}

#if TUXTXT_DEBUG
						printf("TuxTxt <Service %04x Country code \"%3s\" national subset %2d%s>\n",
								 pid_table[pids_found].service_id,
								 country_code,
								 pid_table[pids_found].national_subset,
								 (pid_table[pids_found].vtxt_pid == tuxtxt_cache.vtxtpid) ? " * " : ""
								 );
#endif

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
		ioctl(dmx, DMX_STOP);
		close(dmx);
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
		ioctl(dmx, DMX_STOP);
		close(dmx);

		return 1;
	}

	first_sdt_sec = -1;
	while (1)
	{
		if (read(dmx, SDT, 3) == -1)
		{
			perror("TuxTxt <read SDT>");

			ioctl(dmx, DMX_STOP);
			close(dmx);
			RenderMessage(ShowServiceName);
			return 1;
		}

		if (read(dmx, SDT+3, ((SDT[1] & 0x0f) << 8) | SDT[2]) == -1)
		{
			perror("TuxTxt <read SDT>");

			ioctl(dmx, DMX_STOP);
			close(dmx);
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

	if (tuxtxt_cache.vtxtpid != 0)
	{
		while (pid_table[current_service].vtxt_pid != tuxtxt_cache.vtxtpid && current_service < pids_found)
			current_service++;

		if (auto_national && current_service < pids_found)
			tuxtxt_cache.national_subset = pid_table[current_service].national_subset;
		RenderMessage(ShowServiceName);
	}

	getpidsdone = 1;

	RenderCharLCD(pids_found/10,  7, 44);
	RenderCharLCD(pids_found%10, 19, 44);
	ioctl(dmx, DMX_STOP);
	close(dmx);

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
	if (memcmp(cc, "rus", 3) == 0 ||
	    memcmp(cc, "bul", 3) == 0 ||
	    memcmp(cc, "ser", 3) == 0 ||
	    memcmp(cc, "cro", 3) == 0 ||
	    memcmp(cc, "ukr", 3) == 0)
		return NAT_RU;
	if (memcmp(cc, "gre", 3) == 0)
		return NAT_GR;

	return NAT_DEFAULT;	/* use default charset */
}

/******************************************************************************
 * ConfigMenu                                                                 *
 ******************************************************************************/
#if TUXTXT_DEBUG
void charpage()
{
	PosY = StartY;
	PosX = StartX;
	char cachefill[100];
	int fullsize =0,hexcount = 0, col, p,sp;
	int escpage = 0;
	tstCachedPage* pg;
	ClearFB(black);

	int zipsize = 0;
	for (p = 0; p < 0x900; p++)
	{
		for (sp = 0; sp < 0x80; sp++)
		{
			pg = tuxtxt_cache.astCachetable[p][sp];
			if (pg)
			{

				fullsize+=23*40;
				zipsize += tuxtxt_get_zipsize(p,sp);
			}
		}
	}


	memset(cachefill,' ',40);
	sprintf(cachefill,"f:%d z:%d h:%d c:%d %03x",fullsize, zipsize, hexcount, tuxtxt_cache.cached_pages, escpage);

	for (col = 0; col < 40; col++)
	{
		RenderCharFB(cachefill[col], &atrtable[ATR_WB]);
	}
	tstPageAttr atr;
	memcpy(&atr,&atrtable[ATR_WB],sizeof(tstPageAttr));
	int row;
	atr.charset = C_G0P;
	PosY = StartY+fontheight;
	for (row = 0; row < 16; row++)
	{
		PosY+= fontheight;
		SetPosX(1);
		for (col=0; col < 6; col++)
		{
			RenderCharFB(col*16+row+0x20, &atr);
		}
	}
	atr.setX26 = 1;
	PosY = StartY+fontheight;
	for (row = 0; row < 16; row++)
	{
		PosY+= fontheight;
		SetPosX(10);
		for (col=0; col < 6; col++)
		{
			RenderCharFB(col*16+row+0x20, &atr);
		}
	}
	PosY = StartY+fontheight;
	atr.charset = C_G2;
	atr.setX26 = 0;
	for (row = 0; row < 16; row++)
	{
		PosY+= fontheight;
		SetPosX(20);
		for (col=0; col < 6; col++)
		{
			RenderCharFB(col*16+row+0x20, &atr);
		}
	}
	atr.charset = C_G3;
	PosY = StartY+fontheight;
	for (row = 0; row < 16; row++)
	{
		PosY+= fontheight;
		SetPosX(30);
		for (col=0; col < 6; col++)
		{
			RenderCharFB(col*16+row+0x20, &atr);
		}
	}
	do
	{
		GetRCCode();
	}
	while (RCCode != RC_OK && RCCode != RC_HOME);
}
#endif
void Menu_HighlightLine(char *menu, int line, int high)
{
	char hilitline[] = "0111111111111111111111111111102";
	int itext = Menu_Width*line; /* index start menuline */
	int byte;
	int national_subset_bak = tuxtxt_cache.national_subset;

	PosX = Menu_StartX;
	PosY = Menu_StartY + line*fontheight;
	if (line == MenuLine[M_NAT])
		tuxtxt_cache.national_subset = national_subset_bak;
	else
		tuxtxt_cache.national_subset = menusubset[menulanguage];

	for (byte = 0; byte < Menu_Width; byte++)
		RenderCharFB(menu[itext + byte],
						 high ?
						 &atrtable[hilitline[byte] - '0' + ATR_MENUHIL0] :
						 &atrtable[menuatr[itext + byte] - '0' + ATR_MENU0]);
	tuxtxt_cache.national_subset = national_subset_bak;
}

void Menu_UpdateHotlist(char *menu, int hotindex, int menuitem)
{
	int i, j, k;
	tstPageAttr *attr;

	PosX = Menu_StartX + 6*fontwidth;
	PosY = Menu_StartY + (MenuLine[M_HOT]+1)*fontheight;
	j = Menu_Width*(MenuLine[M_HOT]+1) + 6; /* start index in menu */

	for (i = 0; i <= maxhotlist+1; i++)
	{
		if (i == maxhotlist+1) /* clear last+1 entry in case it was deleted */
		{
			attr = &atrtable[ATR_MENU5];
			memset(&menu[j], ' ', 3);
		}
		else
		{
			if (i == hotindex)
				attr = &atrtable[ATR_MENU1];
			else
			attr = &atrtable[ATR_MENU5];
			tuxtxt_hex2str(&menu[j+2], hotlist[i]);
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

	tuxtxt_hex2str(&menu[Menu_Width*MenuLine[M_HOT] + hotlistpagecolumn[menulanguage]], (hotindex >= 0) ? hotlist[hotindex] : tuxtxt_cache.page);
	memcpy(&menu[Menu_Width*MenuLine[M_HOT] + hotlisttextcolumn[menulanguage]], &hotlisttext[menulanguage][(hotindex >= 0) ? 5 : 0], 5);
	PosX = Menu_StartX + 20*fontwidth;
	PosY = Menu_StartY + MenuLine[M_HOT]*fontheight;

	Menu_HighlightLine(menu, MenuLine[M_HOT], (menuitem == M_HOT) ? 1 : 0);
}

void Menu_Init(char *menu, int current_pid, int menuitem, int hotindex)
{
	int byte, line;
	int national_subset_bak = tuxtxt_cache.national_subset;

	memcpy(menu, configmenu[menulanguage], Menu_Height*Menu_Width);

	if (getpidsdone)
	{
		memset(&menu[MenuLine[M_PID]*Menu_Width+3], 0x20,24);
		if (SDT_ready)
			memcpy(&menu[MenuLine[M_PID]*Menu_Width+3+(24-pid_table[current_pid].service_name_len)/2], &pid_table[current_pid].service_name, pid_table[current_pid].service_name_len);
		else
			tuxtxt_hex2str(&menu[MenuLine[M_PID]*Menu_Width + 13 + 3], tuxtxt_cache.vtxtpid);
	}
	if (!getpidsdone || current_pid == 0 || pids_found == 1)
		menu[MenuLine[M_PID]*Menu_Width +  1] = ' ';

	if (!getpidsdone || current_pid == pids_found - 1 || pids_found == 1)
		menu[MenuLine[M_PID]*Menu_Width + 28] = ' ';


	/* set 16:9 modi, colors & national subset */
	memcpy(&menu[Menu_Width*MenuLine[M_SC1] + Menu_Width - 5], &configonoff[menulanguage][screen_mode1  ? 3 : 0], 3);
	memcpy(&menu[Menu_Width*MenuLine[M_SC2] + Menu_Width - 5], &configonoff[menulanguage][screen_mode2  ? 3 : 0], 3);

	menu[MenuLine[M_COL]*Menu_Width +  1] = (color_mode == 1  ? ' ' : 'í');
	menu[MenuLine[M_COL]*Menu_Width + 28] = (color_mode == 24 ? ' ' : 'î');
	memset(&menu[Menu_Width*MenuLine[M_COL] + 3             ], 0x7f,color_mode);
	memset(&menu[Menu_Width*MenuLine[M_COL] + 3+color_mode  ], 0x20,24-color_mode);
//	memcpy(&menu[Menu_Width*MenuLine[M_COL] + Menu_Width - 5], &configonoff[menulanguage][color_mode    ? 3 : 0], 3);
	menu[MenuLine[M_TRA]*Menu_Width +  1] = (trans_mode == 1  ? ' ' : 'í');
	menu[MenuLine[M_TRA]*Menu_Width + 28] = (trans_mode == 24 ? ' ' : 'î');
	memset(&menu[Menu_Width*MenuLine[M_TRA] + 3             ], 0x7f,trans_mode);
	memset(&menu[Menu_Width*MenuLine[M_TRA] + 3+trans_mode  ], 0x20,24-trans_mode);

	memcpy(&menu[Menu_Width*MenuLine[M_AUN] + Menu_Width - 5], &configonoff[menulanguage][auto_national ? 3 : 0], 3);
	if (tuxtxt_cache.national_subset != NAT_DE)
		memcpy(&menu[Menu_Width*MenuLine[M_NAT] + 2], &countrystring[tuxtxt_cache.national_subset*COUNTRYSTRING_WIDTH], COUNTRYSTRING_WIDTH);
	if (tuxtxt_cache.national_subset == 0  || auto_national)
		menu[MenuLine[M_NAT]*Menu_Width +  1] = ' ';
	if (tuxtxt_cache.national_subset == MAX_NATIONAL_SUBSET || auto_national)
		menu[MenuLine[M_NAT]*Menu_Width + 28] = ' ';
	if (showhex)
		menu[MenuLine[M_PID]*Menu_Width + 27] = '?';
	/* render menu */
	PosY = Menu_StartY;
	for (line = 0; line < Menu_Height; line++)
	{
		PosX = Menu_StartX;
		if (line == MenuLine[M_NAT])
			tuxtxt_cache.national_subset = national_subset_bak;
		else
			tuxtxt_cache.national_subset = menusubset[menulanguage];

		if (line == Menu_Height-2)
			memcpy(&menu[line*Menu_Width + 21], versioninfo, 4);

		for (byte = 0; byte < Menu_Width; byte++)
			RenderCharFB(menu[line*Menu_Width + byte], &atrtable[menuatr[line*Menu_Width + byte] - '0' + ATR_MENU0]);

		PosY += fontheight;
	}
	tuxtxt_cache.national_subset = national_subset_bak;
	Menu_HighlightLine(menu, MenuLine[menuitem], 1);
	Menu_UpdateHotlist(menu, hotindex, menuitem);
}

void ConfigMenu(int Init)
{
	int val, menuitem = M_Start;
	int current_pid = 0;
	int hotindex;
	int oldscreenmode;
	int i;
	int national_subset_bak = tuxtxt_cache.national_subset;
	char menu[Menu_Height*Menu_Width];

	if (auto_national && tuxtxt_cache.astCachetable[tuxtxt_cache.page][tuxtxt_cache.subpage] &&
		tuxtxt_cache.astCachetable[tuxtxt_cache.page][tuxtxt_cache.subpage]->pageinfo.nationalvalid)
		tuxtxt_cache.national_subset = countryconversiontable[tuxtxt_cache.astCachetable[tuxtxt_cache.page][tuxtxt_cache.subpage]->pageinfo.national];

	if (getpidsdone)
	{
		/* set current vtxt */
		if (tuxtxt_cache.vtxtpid == 0)
			tuxtxt_cache.vtxtpid = pid_table[0].vtxt_pid;
		else
			while(pid_table[current_pid].vtxt_pid != tuxtxt_cache.vtxtpid && current_pid < pids_found)
				current_pid++;
	}

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
				case M_COL:
					saveconfig = 1;
					color_mode--;
					if (color_mode < 1) color_mode = 1;
					menu[MenuLine[M_COL]*Menu_Width +  1] = (color_mode == 1  ? ' ' : 'í');
					menu[MenuLine[M_COL]*Menu_Width + 28] = (color_mode == 24 ? ' ' : 'î');
					memset(&menu[Menu_Width*MenuLine[M_COL] + 3             ], 0x7f,color_mode);
					memset(&menu[Menu_Width*MenuLine[M_COL] + 3+color_mode  ], 0x20,24-color_mode);
					Menu_HighlightLine(menu, MenuLine[menuitem], 1);
					setcolors((unsigned short *)tuxtxt_defaultcolors, 0, SIZECOLTABLE);
					break;
				case M_TRA:
					saveconfig = 1;
					trans_mode--;
					if (trans_mode < 1) trans_mode = 1;
					menu[MenuLine[M_TRA]*Menu_Width +  1] = (trans_mode == 1  ? ' ' : 'í');
					menu[MenuLine[M_TRA]*Menu_Width + 28] = (trans_mode == 24 ? ' ' : 'î');
					memset(&menu[Menu_Width*MenuLine[M_TRA] + 3             ], 0x7f,trans_mode);
					memset(&menu[Menu_Width*MenuLine[M_TRA] + 3+trans_mode  ], 0x20,24-trans_mode);
					Menu_HighlightLine(menu, MenuLine[menuitem], 1);
					setcolors((unsigned short *)tuxtxt_defaultcolors, 0, SIZECOLTABLE);
					break;
				case M_PID:
				{
					if (!getpidsdone)
					{
						GetTeletextPIDs();
						ClearFB(transp);
						/* set current vtxt */
						if (tuxtxt_cache.vtxtpid == 0)
							tuxtxt_cache.vtxtpid = pid_table[0].vtxt_pid;
						else
							while(pid_table[current_pid].vtxt_pid != tuxtxt_cache.vtxtpid && current_pid < pids_found)
								current_pid++;
						Menu_Init(menu, current_pid, menuitem, hotindex);
					}
					if (current_pid > 0)
					{
						current_pid--;

						memset(&menu[MenuLine[M_PID]*Menu_Width + 3], ' ', 24);

						if (SDT_ready)
						{
							memcpy(&menu[MenuLine[M_PID]*Menu_Width+3+(24-pid_table[current_pid].service_name_len)/2],
							       &pid_table[current_pid].service_name,
							       pid_table[current_pid].service_name_len);
						}
						else
							tuxtxt_hex2str(&menu[MenuLine[M_PID]*Menu_Width + 13 + 3], tuxtxt_cache.vtxtpid);

						if (pids_found > 1)
						{
							if (current_pid == 0)
							{
								menu[MenuLine[M_PID]*Menu_Width +  1] = ' ';
								menu[MenuLine[M_PID]*Menu_Width + 28] = 'î';
							}
							else
							{
								menu[MenuLine[M_PID]*Menu_Width +  1] = 'í';
								menu[MenuLine[M_PID]*Menu_Width + 28] = 'î';
							}
						}

						Menu_HighlightLine(menu, MenuLine[menuitem], 1);

						if (auto_national)
						{
							tuxtxt_cache.national_subset = pid_table[current_pid].national_subset;

							memcpy(&menu[Menu_Width*MenuLine[M_NAT] + 2], &countrystring[tuxtxt_cache.national_subset*COUNTRYSTRING_WIDTH], COUNTRYSTRING_WIDTH);
							Menu_HighlightLine(menu, MenuLine[M_NAT], 0);
						}
					}
					break;
				}

				case M_NAT:
					saveconfig = 1;
					if (tuxtxt_cache.national_subset > 0)
					{
						tuxtxt_cache.national_subset--;

						if (tuxtxt_cache.national_subset == 0)
						{
							menu[MenuLine[M_NAT]*Menu_Width +  1] = ' ';
							menu[MenuLine[M_NAT]*Menu_Width + 28] = 'î';
						}
						else
						{
							menu[MenuLine[M_NAT]*Menu_Width +  1] = 'í';
							menu[MenuLine[M_NAT]*Menu_Width + 28] = 'î';
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
				case M_COL:
					saveconfig = 1;
					color_mode++;
					if (color_mode > 24) color_mode = 24;
					menu[MenuLine[M_COL]*Menu_Width +  1] = (color_mode == 1  ? ' ' : 'í');
					menu[MenuLine[M_COL]*Menu_Width + 28] = (color_mode == 24 ? ' ' : 'î');
					memset(&menu[Menu_Width*MenuLine[M_COL] + 3             ], 0x7f,color_mode);
					memset(&menu[Menu_Width*MenuLine[M_COL] + 3+color_mode  ], 0x20,24-color_mode);
					Menu_HighlightLine(menu, MenuLine[menuitem], 1);
					setcolors((unsigned short *)tuxtxt_defaultcolors, 0, SIZECOLTABLE);
					break;
				case M_TRA:
					saveconfig = 1;
					trans_mode++;
					if (trans_mode > 24) trans_mode = 24;
					menu[MenuLine[M_TRA]*Menu_Width +  1] = (trans_mode == 1  ? ' ' : 'í');
					menu[MenuLine[M_TRA]*Menu_Width + 28] = (trans_mode == 24 ? ' ' : 'î');
					memset(&menu[Menu_Width*MenuLine[M_TRA] + 3             ], 0x7f,trans_mode);
					memset(&menu[Menu_Width*MenuLine[M_TRA] + 3+trans_mode  ], 0x20,24-trans_mode);
					Menu_HighlightLine(menu, MenuLine[menuitem], 1);
					setcolors((unsigned short *)tuxtxt_defaultcolors, 0, SIZECOLTABLE);
					break;
				case M_PID:
					if (!getpidsdone)
					{
						GetTeletextPIDs();
						ClearFB(transp);
						/* set current vtxt */
						if (tuxtxt_cache.vtxtpid == 0)
							tuxtxt_cache.vtxtpid = pid_table[0].vtxt_pid;
						else
							while(pid_table[current_pid].vtxt_pid != tuxtxt_cache.vtxtpid && current_pid < pids_found)
								current_pid++;
						Menu_Init(menu, current_pid, menuitem, hotindex);
					}
					if (current_pid < pids_found - 1)
					{
						current_pid++;

						memset(&menu[MenuLine[M_PID]*Menu_Width + 3], ' ', 24);

						if (SDT_ready)
							memcpy(&menu[MenuLine[M_PID]*Menu_Width + 3 +
											 (24-pid_table[current_pid].service_name_len)/2],
									 &pid_table[current_pid].service_name,
									 pid_table[current_pid].service_name_len);
						else
							tuxtxt_hex2str(&menu[MenuLine[M_PID]*Menu_Width + 13 + 3], pid_table[current_pid].vtxt_pid);

						if (pids_found > 1)
						{
							if (current_pid == pids_found - 1)
							{
								menu[MenuLine[M_PID]*Menu_Width +  1] = 'í';
								menu[MenuLine[M_PID]*Menu_Width + 28] = ' ';
							}
							else
							{
								menu[MenuLine[M_PID]*Menu_Width +  1] = 'í';
								menu[MenuLine[M_PID]*Menu_Width + 28] = 'î';
							}
						}

						Menu_HighlightLine(menu, MenuLine[menuitem], 1);

						if (auto_national)
						{
							if (getpidsdone)
								tuxtxt_cache.national_subset = pid_table[current_pid].national_subset;
							memcpy(&menu[Menu_Width*MenuLine[M_NAT] + 2], &countrystring[tuxtxt_cache.national_subset*COUNTRYSTRING_WIDTH], COUNTRYSTRING_WIDTH);
							Menu_HighlightLine(menu, MenuLine[M_NAT], 0);
						}
					}
					break;

				case M_NAT:
					saveconfig = 1;
					if (tuxtxt_cache.national_subset < MAX_NATIONAL_SUBSET)
					{
						tuxtxt_cache.national_subset++;

						if (tuxtxt_cache.national_subset == MAX_NATIONAL_SUBSET)
						{
							menu[MenuLine[M_NAT]*Menu_Width +  1] = 'í';
							menu[MenuLine[M_NAT]*Menu_Width + 28] = ' ';
						}
						else
						{
							menu[MenuLine[M_NAT]*Menu_Width +  1] = 'í';
							menu[MenuLine[M_NAT]*Menu_Width + 28] = 'î';
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
							hotlist[hotindex] = tuxtxt_cache.page;
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
							hotlist[hotindex] = tuxtxt_cache.page;
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
							hotlist[++maxhotlist] = tuxtxt_cache.page;
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
					menu[MenuLine[M_PID]*Menu_Width + 27] = (showhex ? '?' : ' ');
					Menu_HighlightLine(menu, MenuLine[menuitem], 1);
				break;
#if TUXTXT_DEBUG
				case M_LNG:
					charpage();
					ClearFB(transp);
					Menu_Init(menu, current_pid, menuitem, hotindex);
				break;
#endif
				}
				break; /* RC_MUTE */

			case RC_OK:
				switch (menuitem)
				{
				case M_PID:
					if (!getpidsdone)
					{
						GetTeletextPIDs();
						ClearFB(transp);
						/* set current vtxt */
						if (tuxtxt_cache.vtxtpid == 0)
							tuxtxt_cache.vtxtpid = pid_table[0].vtxt_pid;
						else
							while(pid_table[current_pid].vtxt_pid != tuxtxt_cache.vtxtpid && current_pid < pids_found)
								current_pid++;
						Menu_Init(menu, current_pid, menuitem, hotindex);
					}
					else if (pids_found > 1)
					{
							if (hotlistchanged)
								savehotlist();

						if (Init || tuxtxt_cache.vtxtpid != pid_table[current_pid].vtxt_pid)
							{
#if TUXTXT_CFG_STANDALONE
								tuxtxt_stop_thread();
								tuxtxt_clear_cache();
#else
								tuxtxt_stop();
							if (Init)
								tuxtxt_cache.vtxtpid = 0; // force clear cache
#endif
								/* reset data */


								//page_atrb[32] = transp<<4 | transp;
								inputcounter = 2;


								tuxtxt_cache.page     = 0x100;
								lastpage = 0x100;
								prev_100 = 0x100;
								prev_10  = 0x100;
								next_100 = 0x100;
								next_10  = 0x100;
								tuxtxt_cache.subpage  = 0;

								tuxtxt_cache.pageupdate = 0;
								tuxtxt_cache.zap_subpage_manual = 0;
								hintmode = 0;
								memset(page_char,' ',40 * 25);

								for (i = 0; i < 40*25; i++)
								{
									page_atrb[i].fg = transp;
									page_atrb[i].bg = transp;
								}
								ClearFB(transp);


								/* start demuxer with new vtxtpid */
								if (auto_national)
									tuxtxt_cache.national_subset = pid_table[current_pid].national_subset;

#if TUXTXT_CFG_STANDALONE
								tuxtxt_cache.vtxtpid = pid_table[current_pid].vtxt_pid;
								tuxtxt_start_thread();
#else
								tuxtxt_start(pid_table[current_pid].vtxt_pid);
#endif
							}
//							tuxtxt_cache.pageupdate = 1;

							ClearBB(black);
							gethotlist();

						/* show new teletext */
						current_service = current_pid;
//						RenderMessage(ShowServiceName);

						fcntl(rc, F_SETFL, O_NONBLOCK);
						RCCode = -1;
						if (oldscreenmode)
							SwitchScreenMode(oldscreenmode); /* restore divided screen */
						return;
					}
					break;

				case M_SC1:
					saveconfig = 1;
					screen_mode1++;
					screen_mode1 &= 1;

					memcpy(&menu[Menu_Width*MenuLine[M_SC1] + Menu_Width - 5], &configonoff[menulanguage][screen_mode1  ? 3 : 0], 3);
					Menu_HighlightLine(menu, MenuLine[menuitem], 1);

					ioctl(avs, AVSIOSSCARTPIN8, &fncmodes[screen_mode1]);
					ioctl(saa, SAAIOSWSS, &saamodes[screen_mode1]);

					break;

				case M_SC2:
					saveconfig = 1;
					screen_mode2++;
					screen_mode2 &= 1;

					memcpy(&menu[Menu_Width*MenuLine[M_SC2] + Menu_Width - 5], &configonoff[menulanguage][screen_mode2  ? 3 : 0], 3);
					Menu_HighlightLine(menu, MenuLine[menuitem], 1);
					break;


				case M_AUN:
					saveconfig = 1;
					auto_national++;
					auto_national &= 1;
					if (auto_national)
					{
					 	if (getpidsdone)
							tuxtxt_cache.national_subset = pid_table[current_pid].national_subset;
						else
						{
							if (tuxtxt_cache.astCachetable[tuxtxt_cache.page][tuxtxt_cache.subpage] &&
								tuxtxt_cache.astCachetable[tuxtxt_cache.page][tuxtxt_cache.subpage]->pageinfo.nationalvalid)
								tuxtxt_cache.national_subset = countryconversiontable[tuxtxt_cache.astCachetable[tuxtxt_cache.page][tuxtxt_cache.subpage]->pageinfo.national];
							else
								tuxtxt_cache.national_subset = national_subset_bak;
						}

					}
					Menu_Init(menu, current_pid, menuitem, hotindex);
					break;
				case M_HOT: /* show selected page */
				{
					if (hotindex >= 0) /* not found: ignore */
					{
						lastpage = tuxtxt_cache.page;
						tuxtxt_cache.page = hotlist[hotindex];
						tuxtxt_cache.subpage = tuxtxt_cache.subpagetable[tuxtxt_cache.page];
						inputcounter = 2;
						tuxtxt_cache.pageupdate = 1;
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
	tuxtxt_cache.pageupdate = 1;
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
		RenderCharFB(Number | '0', &atrtable[ATR_WB]);
		RenderCharFB('-', &atrtable[ATR_WB]);
		RenderCharFB('-', &atrtable[ATR_WB]);
		break;

	case 1:
		SetPosX(2);
		RenderCharFB(Number | '0', &atrtable[ATR_WB]);
		break;

	case 0:
		SetPosX(3);
		RenderCharFB(Number | '0', &atrtable[ATR_WB]);
		break;
	}

	/* generate pagenumber */
	temp_page |= Number << inputcounter*4;

	inputcounter--;

	if (inputcounter < 0)
	{
		/* disable subpage zapping */
		tuxtxt_cache.zap_subpage_manual = 0;

		/* reset input */
		inputcounter = 2;

		/* set new page */
		lastpage = tuxtxt_cache.page;

		tuxtxt_cache.page = temp_page;
		hintmode = 0;

		/* check cache */
		int subp = tuxtxt_cache.subpagetable[tuxtxt_cache.page];
		if (subp != 0xFF)
		{
			tuxtxt_cache.subpage = subp;
			tuxtxt_cache.pageupdate = 1;
#if TUXTXT_DEBUG
			printf("TuxTxt <DirectInput: %.3X-%.2X>\n", tuxtxt_cache.page, tuxtxt_cache.subpage);
#endif
		}
		else
		{
			tuxtxt_cache.subpage = 0;
//			RenderMessage(PageNotFound);
#if TUXTXT_DEBUG
			printf("TuxTxt <DirectInput: %.3X not found>\n", tuxtxt_cache.page);
#endif
		}
	}
}

/******************************************************************************
 * GetNextPageOne                                                             *
 ******************************************************************************/

void GetNextPageOne(int up)
{
	/* disable subpage zapping */
	tuxtxt_cache.zap_subpage_manual = 0;

	/* abort pageinput */
	inputcounter = 2;

	/* find next cached page */
	lastpage = tuxtxt_cache.page;

	int subp;
	do {
		if (up)
			tuxtxt_next_dec(&tuxtxt_cache.page);
		else
			tuxtxt_prev_dec(&tuxtxt_cache.page);
		subp = tuxtxt_cache.subpagetable[tuxtxt_cache.page];
	} while (subp == 0xFF && tuxtxt_cache.page != lastpage);

	/* update page */
	if (tuxtxt_cache.page != lastpage)
	{
		if (zoommode == 2)
			zoommode = 1;

		tuxtxt_cache.subpage = subp;
		hintmode = 0;
		tuxtxt_cache.pageupdate = 1;
#if TUXTXT_DEBUG
		printf("TuxTxt <NextPageOne: %.3X-%.2X>\n", tuxtxt_cache.page, tuxtxt_cache.subpage);
#endif
	}
}

/******************************************************************************
 * GetNextSubPage                                                             *
 ******************************************************************************/
void GetNextSubPage(int offset)
{
	int loop;

	/* abort pageinput */
	inputcounter = 2;

	for (loop = tuxtxt_cache.subpage + offset; loop != tuxtxt_cache.subpage; loop += offset)
	{
		if (loop < 0)
			loop = 0x79;
		else if (loop > 0x79)
			loop = 0;
		if (loop == tuxtxt_cache.subpage)
			break;

		if (tuxtxt_cache.astCachetable[tuxtxt_cache.page][loop])
		{
			/* enable manual subpage zapping */
			tuxtxt_cache.zap_subpage_manual = 1;

			/* update page */
			if (zoommode == 2) /* if zoomed to lower half */
				zoommode = 1; /* activate upper half */

			tuxtxt_cache.subpage = loop;
			hintmode = 0;
			tuxtxt_cache.pageupdate = 1;
#if TUXTXT_DEBUG
			printf("TuxTxt <NextSubPage: %.3X-%.2X>\n", tuxtxt_cache.page, tuxtxt_cache.subpage);
#endif
			return;
		}
	}

#if TUXTXT_DEBUG
	printf("TuxTxt <NextSubPage: no other SubPage>\n");
#endif
}
/******************************************************************************
 * ColorKey                                                                   *
 ******************************************************************************/

void ColorKey(int target)
{
	if (!target)
		return;
	if (zoommode == 2)
		zoommode = 1;
	lastpage     = tuxtxt_cache.page;
	tuxtxt_cache.page         = target;
	tuxtxt_cache.subpage      = tuxtxt_cache.subpagetable[tuxtxt_cache.page];
	inputcounter = 2;
	hintmode     = 0;
	tuxtxt_cache.pageupdate   = 1;
#if TUXTXT_DEBUG
	printf("TuxTxt <ColorKey: %.3X>\n", tuxtxt_cache.page);
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
		RenderCharFB(catchmenutext[menulanguage][byte], &atrtable[catchmenutext[menulanguage][byte+40] - '0' + ATR_CATCHMENU0]);
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
		tuxtxt_cache.pageupdate = 1;
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
			tuxtxt_cache.pageupdate = 1;
			pagecatching = 0;
			RCCode = -1;
			return;
		}
		UpdateLCD();
	} while (RCCode != RC_OK);

	/* set new page */
	if (zoommode == 2)
		zoommode = 1;

	lastpage     = tuxtxt_cache.page;
	tuxtxt_cache.page         = catched_page;
	hintmode = 0;
	tuxtxt_cache.pageupdate = 1;
	pagecatching = 0;

	int subp = tuxtxt_cache.subpagetable[tuxtxt_cache.page];
	if (subp != 0xFF)
		tuxtxt_cache.subpage = subp;
	else
		tuxtxt_cache.subpage = 0;

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
		tstPageAttr a = page_atrb[catch_row*40 + catch_col];

		if (!(a.charset == C_G1C || a.charset == C_G1S) && /* no mosaic */
			 (a.fg != a.bg) && /* not hidden */
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
#if TUXTXT_DEBUG
				printf("TuxTxt <PageCatching: %.3X\n", catched_page);
#endif
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
#if TUXTXT_DEBUG
				printf("TuxTxt <PageCatching: no PageNumber>\n");
#endif
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
#if TUXTXT_DEBUG
				printf("TuxTxt <PageCatching: no PageNumber>\n");
#endif
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

		RenderCharFB(page_char[pc_old_row*40 + pc_old_col    ], &page_atrb[pc_old_row*40 + pc_old_col    ]);
		RenderCharFB(page_char[pc_old_row*40 + pc_old_col + 1], &page_atrb[pc_old_row*40 + pc_old_col + 1]);
		RenderCharFB(page_char[pc_old_row*40 + pc_old_col + 2], &page_atrb[pc_old_row*40 + pc_old_col + 2]);
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

	tstPageAttr a0 = page_atrb[catch_row*40 + catch_col    ];
	tstPageAttr a1 = page_atrb[catch_row*40 + catch_col + 1];
	tstPageAttr a2 = page_atrb[catch_row*40 + catch_col + 2];
	int t;

	/* exchange colors */
	t = a0.fg; a0.fg = a0.bg; a0.bg = t;
	t = a1.fg; a1.fg = a1.bg; a1.bg = t;
	t = a2.fg; a2.fg = a2.bg; a2.bg = t;

	RenderCharFB(page_char[catch_row*40 + catch_col    ], &a0);
	RenderCharFB(page_char[catch_row*40 + catch_col + 1], &a1);
	RenderCharFB(page_char[catch_row*40 + catch_col + 2], &a2);
}

/******************************************************************************
 * SwitchZoomMode                                                             *
 ******************************************************************************/

void SwitchZoomMode()
{
	if (tuxtxt_cache.subpagetable[tuxtxt_cache.page] != 0xFF)
	{
		/* toggle mode */
		zoommode++;

		if (zoommode == 3)
			zoommode = 0;

#if TUXTXT_DEBUG
		printf("TuxTxt <SwitchZoomMode: %d>\n", zoommode);
#endif
		/* update page */
		tuxtxt_cache.pageupdate = 1; /* FIXME */
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
//	if ((screenmode > (screen_mode2 ? 2 : 1)) || (screenmode < 0))
	if ((screenmode > 2) || (screenmode < 0))
		screenmode = 0;

#if TUXTXT_DEBUG
	printf("TuxTxt <SwitchScreenMode: %d>\n", screenmode);
#endif

	/* update page */
	tuxtxt_cache.pageupdate = 1;

	/* clear back buffer */
#ifndef HAVE_DREAMBOX_HARDWARE
	clearbbcolor = black;
#else
	clearbbcolor = screenmode?transp:tuxtxt_cache.FullScrColor;
#endif
	ClearBB(clearbbcolor);


	/* set mode */
	if (screenmode)								 /* split */
	{
		ClearFB(clearbbcolor);

		int fw, fh, tx, ty, tw, th;

		if (screenmode==1) /* split with topmenu */
		{
			fw = fontwidth_topmenumain;
			fh = fontheight;
			tw = TV43WIDTH;
			displaywidth= (TV43STARTX     -sx);
			StartX = sx; //+ (((ex-sx) - (40*fw+2+tw)) / 2); /* center screen */
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
			displaywidth= (TV169FULLSTARTX-sx);
		}

		setfontwidth(fw);

#if HAVE_DVB_API_VERSION < 3
		avia_pig_hide(pig);
		avia_pig_set_pos(pig, tx, ty);
		avia_pig_set_size(pig, tw, th);
		avia_pig_set_stack(pig, 2);
		avia_pig_show(pig);
#else
		int sm = 0;
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

		setfontwidth(fontwidth_normal);
		displaywidth= (ex-sx);
		StartX = sx; //+ (ex-sx - 40*fontwidth) / 2; /* center screen */

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
	{
		prevscreenmode = screenmode;
		SwitchScreenMode(0); /* turn off divided screen */
	}
	/* toggle mode */
	if (!transpmode)
		transpmode = 2;
	else
		transpmode--; /* backward to immediately switch to TV-screen */

#if TUXTXT_DEBUG
	printf("TuxTxt <SwitchTranspMode: %d>\n", transpmode);
#endif

	/* set mode */
	if (!transpmode) /* normal text-only */
	{
		ClearBB(tuxtxt_cache.FullScrColor);
		tuxtxt_cache.pageupdate = 1;
	}
	else if (transpmode == 1) /* semi-transparent BG with FG text */
	{
		/* restore videoformat */
		ioctl(avs, AVSIOSSCARTPIN8, &fnc_old);
		ioctl(saa, SAAIOSWSS, &saa_old);

		ClearBB(transp);
		tuxtxt_cache.pageupdate = 1;
	}
	else /* TV mode */
	{
		/* restore videoformat */
		ioctl(avs, AVSIOSSCARTPIN8, &fnc_old);
		ioctl(saa, SAAIOSWSS, &saa_old);

		ClearFB(transp);
		clearbbcolor = tuxtxt_cache.FullScrColor;
	}
}

/******************************************************************************
 * SwitchHintMode                                                             *
 ******************************************************************************/

void SwitchHintMode()
{
	/* toggle mode */
	hintmode ^= 1;
#if TUXTXT_DEBUG
	printf("TuxTxt <SwitchHintMode: %d>\n", hintmode);
#endif

	if (!hintmode)	/* toggle evaluation of level 2.5 information by explicitly switching off hintmode */
	{
		showl25 ^= 1;
#if TUXTXT_DEBUG
		printf("TuxTxt <ShowLevel2p5: %d>\n", showl25);
#endif
	}
	/* update page */
	tuxtxt_cache.pageupdate = 1;
}



/******************************************************************************
 * RenderChar                                                                 *
 ******************************************************************************/

void RenderChar(int Char, tstPageAttr *Attribute, int zoom, int yoffset)
{
	int Row, Pitch, Bit;
	int error, glyph;
	int bgcolor, fgcolor;
	int factor, xfactor;
	int national_subset_local = tuxtxt_cache.national_subset;
	unsigned char *sbitbuffer;

	int curfontwidth = GetCurFontWidth();
	int t = curfontwidth;
	PosX += t;
	int curfontwidth2 = GetCurFontWidth();
	PosX -= t;
	int alphachar = tuxtxt_RenderChar(lfb+(yoffset+StartY)*var_screeninfo.xres,  var_screeninfo.xres,Char, &PosX, PosY-StartY, Attribute, zoom, curfontwidth, curfontwidth2, fontheight, transpmode,axdrcs, ascender);
	if (alphachar <= 0) return;

	if (zoom && Attribute->doubleh)
		factor = 4;
	else if (zoom || Attribute->doubleh)
		factor = 2;
	else
		factor = 1;

	fgcolor = Attribute->fg;
	if (transpmode == 1 && PosY < StartY + 24*fontheight)
	{
		if (fgcolor == transp) /* outside boxed elements (subtitles, news) completely transparent */
			bgcolor = transp;
		else
			bgcolor = transp2;
	}
	else
		bgcolor = Attribute->bg;
	if (Attribute->doublew)
	{
		curfontwidth += curfontwidth2;
		xfactor = 2;
	}
	else
		xfactor = 1;

	if (!(glyph = FT_Get_Char_Index(face, alphachar)))
	{
#if TUXTXT_DEBUG
		printf("TuxTxt <FT_Get_Char_Index for Char %x \"%c\" failed\n", alphachar, alphachar);
#endif
		tuxtxt_FillRect(lfb,var_screeninfo.xres,PosX, PosY + yoffset, curfontwidth, factor*fontheight, bgcolor);
		PosX += curfontwidth;
		return;
	}

#if HAVE_DVB_API_VERSION >= 3
	if ((error = FTC_SBitCache_Lookup(cache, &typettf, glyph, &sbit, NULL)) != 0)
#else
	if ((error = FTC_SBit_Cache_Lookup(cache, &typettf, glyph, &sbit)) != 0)
#endif
	{
#if TUXTXT_DEBUG
		printf("TuxTxt <FTC_SBitCache_Lookup: 0x%x> c%x a%x g%x w%d h%d x%d y%d\n",
				 error, alphachar, Attribute, glyph, curfontwidth, fontheight, PosX, PosY);
#endif
		tuxtxt_FillRect(lfb,var_screeninfo.xres,PosX, PosY + yoffset, curfontwidth, fontheight, bgcolor);
		PosX += curfontwidth;
		return;
	}

	/* render char */
	sbitbuffer = sbit->buffer;
	char localbuffer[1000]; // should be enough to store one character-bitmap...
	// add diacritical marks
	if (Attribute->diacrit)
	{
		FTC_SBit        sbit_diacrit;

		if (national_subset_local == NAT_GR)
			Char = G2table[2][0x20+ Attribute->diacrit];
		else if (national_subset_local == NAT_RU)
			Char = G2table[1][0x20+ Attribute->diacrit];
		else
			Char = G2table[0][0x20+ Attribute->diacrit];
		if ((glyph = FT_Get_Char_Index(face, Char)))
		{
#if HAVE_DVB_API_VERSION >= 3
			if ((error = FTC_SBitCache_Lookup(cache, &typettf, glyph, &sbit_diacrit, NULL)) == 0)
#else
			if ((error = FTC_SBit_Cache_Lookup(cache, &typettf, glyph, &sbit_diacrit)) == 0)
#endif
			{
					sbitbuffer = localbuffer;
					memcpy(sbitbuffer,sbit->buffer,sbit->pitch*sbit->height);

					for (Row = 0; Row < sbit->height; Row++)
					{
						for (Pitch = 0; Pitch < sbit->pitch; Pitch++)
						{
							if (sbit_diacrit->pitch > Pitch && sbit_diacrit->height > Row)
								sbitbuffer[Row*sbit->pitch+Pitch] |= sbit_diacrit->buffer[Row*sbit->pitch+Pitch];
						}
					}
				}
			}
		}

		unsigned char *p;
		int f; /* running counter for zoom factor */
		int he = sbit->height; // sbit->height should not be altered, I guess
		Row = factor * (ascender - sbit->top + TTFShiftY);
		if (Row < 0)
		{
		    sbitbuffer  -= sbit->pitch*Row;
		    he += Row;
		    Row = 0;
		}
		else		
		    tuxtxt_FillRect(lfb,var_screeninfo.xres,PosX, PosY + yoffset, curfontwidth, Row, bgcolor); /* fill upper margin */

		if (ascender - sbit->top + TTFShiftY + he > fontheight)
			he = fontheight - ascender + sbit->top - TTFShiftY; /* limit char height to defined/calculated fontheight */


		p = lfb + PosX + (yoffset + PosY + Row) * var_screeninfo.xres; /* running pointer into framebuffer */
		for (Row = he; Row; Row--) /* row counts up, but down may be a little faster :) */
		{
			int pixtodo = (usettf ? sbit->width : curfontwidth);
			char *pstart = p;

			for (Bit = xfactor * (sbit->left + TTFShiftX); Bit > 0; Bit--) /* fill left margin */
			{
				for (f = factor-1; f >= 0; f--)
					*(p + f*var_screeninfo.xres) = bgcolor;
				p++;
				if (!usettf)
					pixtodo--;
			}

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

					if (xfactor > 1) /* double width */
					{
						for (f = factor-1; f >= 0; f--)
							*(p + f*var_screeninfo.xres) = color;
						p++;
						if (!usettf)
							pixtodo--;
					}
				}
				sbitbuffer++;
			}
			for (Bit = (usettf ? (curfontwidth - xfactor*(sbit->width + sbit->left + TTFShiftX)) : pixtodo);
				  Bit > 0; Bit--) /* fill rest of char width */
			{
				for (f = factor-1; f >= 0; f--)
					*(p + f*var_screeninfo.xres) = bgcolor;
				p++;
			}

			p = pstart + factor*var_screeninfo.xres;
		}

		Row = ascender - sbit->top + he + TTFShiftY;
		tuxtxt_FillRect(lfb,var_screeninfo.xres,PosX, PosY + yoffset + Row*factor, curfontwidth, (fontheight - Row) * factor, bgcolor); /* fill lower margin */
		if (Attribute->underline)
			tuxtxt_FillRect(lfb,var_screeninfo.xres,PosX, PosY + yoffset + (fontheight-2)* factor, curfontwidth,2*factor, fgcolor); /* underline char */

		PosX += curfontwidth;
}

/******************************************************************************
 * RenderCharFB                                                               *
 ******************************************************************************/

void RenderCharFB(int Char, tstPageAttr *Attribute)
{
	RenderChar(Char, Attribute, zoommode, var_screeninfo.yoffset);
}

/******************************************************************************
 * RenderCharBB                                                               *
 ******************************************************************************/

void RenderCharBB(int Char, tstPageAttr *Attribute)
{
	RenderChar(Char, Attribute, 0, var_screeninfo.yres-var_screeninfo.yoffset);
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
	setfontwidth(fontwidth_small_lcd);
	typettf.font.pix_height = fontheight = fontwidth_small_lcd;
	RenderChar(Char, 0, 0, -(YPos<<8 | XPos));
	setfontwidth(old_width);
	typettf.font.pix_height = fontheight = old_height;
}
#endif

/******************************************************************************
 * RenderMessage                                                              *
 ******************************************************************************/

void RenderMessage(int Message)
{
	int byte;
	int fbcolor, timecolor, menuatr;
	int pagecolumn;
	const char *msg;


/*                     00000000001111111111222222222233333333334 */
/*                     01234567890123456789012345678901234567890 */
	char message_1[] = "àááááááá www.tuxtxt.net x.xx áááááááâè";
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
#ifndef HAVE_DREAMBOX_HARDWARE
	if (screenmode)
	{
		fbcolor   = black;
		timecolor = black<<4 | black;
		menuatr = ATR_MSGDRM0;
	}
	else
#endif
	{
		fbcolor   = transp;
		timecolor = transp<<4 | transp;
		menuatr = ATR_MSG0;
	}

	/* clear framebuffer */
	ClearFB(fbcolor);

	/* hide header */
	page_atrb[32].fg = transp;
	page_atrb[32].bg = transp;


	/* set pagenumber */
	if (Message == ShowServiceName)
	{
		pagecolumn = message8pagecolumn[menulanguage];
		msg = message_8[menulanguage];
		memcpy(&message_4, msg, sizeof(message_4));
		tuxtxt_hex2str(message_4+pagecolumn, tuxtxt_cache.page);

		if (SDT_ready)
			memcpy(&message_2[2 + (35 - pid_table[current_service].service_name_len)/2],
					 &pid_table[current_service].service_name, pid_table[current_service].service_name_len);
		else if (Message == ShowServiceName)
			tuxtxt_hex2str(&message_2[17+3], tuxtxt_cache.vtxtpid);

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
		RenderCharFB(message_1[byte], &atrtable[menuatr + ((byte >= 9 && byte <= 27) ? 1 : 0)]);
	RenderCharFB(message_1[37], &atrtable[menuatr + 2]);

	PosX = StartX + fontwidth+5;
	PosY = StartY + fontheight*17;
	RenderCharFB(message_2[0], &atrtable[menuatr + 0]);
	for (byte = 1; byte < 36; byte++)
		RenderCharFB(message_2[byte], &atrtable[menuatr + 3]);
	RenderCharFB(message_2[36], &atrtable[menuatr + 0]);
	RenderCharFB(message_2[37], &atrtable[menuatr + 2]);

	PosX = StartX + fontwidth+5;
	PosY = StartY + fontheight*18;
	RenderCharFB(msg[0], &atrtable[menuatr + 0]);
	for (byte = 1; byte < 36; byte++)
		RenderCharFB(msg[byte], &atrtable[menuatr + 3]);
	RenderCharFB(msg[36], &atrtable[menuatr + 0]);
	RenderCharFB(msg[37], &atrtable[menuatr + 2]);

	PosX = StartX + fontwidth+5;
	PosY = StartY + fontheight*19;
	RenderCharFB(message_4[0], &atrtable[menuatr + 0]);
	for (byte = 1; byte < 36; byte++)
		RenderCharFB(message_4[byte], &atrtable[menuatr + 3]);
	RenderCharFB(message_4[36], &atrtable[menuatr + 0]);
	RenderCharFB(message_4[37], &atrtable[menuatr + 2]);

	PosX = StartX + fontwidth+5;
	PosY = StartY + fontheight*20;
	for (byte = 0; byte < 37; byte++)
		RenderCharFB(message_5[byte], &atrtable[menuatr + 0]);
	RenderCharFB(message_5[37], &atrtable[menuatr + 2]);

	PosX = StartX + fontwidth+5;
	PosY = StartY + fontheight*21;
	for (byte = 0; byte < 38; byte++)
		RenderCharFB(message_6[byte], &atrtable[menuatr + 2]);
}

/******************************************************************************
 * RenderPage                                                                 *
 ******************************************************************************/

void DoFlashing(int startrow)
{
	int row, col;
	/* get national subset */
	if (auto_national &&
		 tuxtxt_cache.national_subset <= NAT_MAX_FROM_HEADER && /* not for GR/RU as long as line28 is not evaluated */
		 pageinfo && pageinfo->nationalvalid) /* individual subset according to page header */
	{
		tuxtxt_cache.national_subset = countryconversiontable[pageinfo->national];
	}
	/* Flashing */
	tstPageAttr flashattr;
	char flashchar;
	struct timeval tv;
	gettimeofday(&tv,NULL);
	long flashphase = (tv.tv_usec / 1000) % 1000;
	int srow = startrow;
	int erow = 24;
	int factor=1;
	switch (zoommode)
	{
		case 1: erow = 12; factor=2;break;
		case 2: srow = 12; factor=2;break;
	}
	PosY = StartY + startrow*fontheight*factor;
	for (row = srow; row < erow; row++)
	{
		int index = row * 40;
		int dhset = 0;
		int incflash = 3;
		int decflash = 2;
		PosX = StartX;
		for (col = nofirst; col < 40; col++)
		{
			if (page_atrb[index + col].flashing && page_char[index + col] > 0x20 && page_char[index + col]!= 0xff )
			{
				SetPosX(col);
				flashchar = page_char[index + col];
				int doflash = 0;
				memcpy(&flashattr,&page_atrb[index + col],sizeof(tstPageAttr));
				switch (flashattr.flashing &0x1c) // Flash Rate
				{
					case 0x00 :	// 1 Hz
						if (flashphase>500) doflash = 1;
						break;
					case 0x04 :	// 2 Hz  Phase 1
						if (flashphase<250) doflash = 1;
						break;
					case 0x08 :	// 2 Hz  Phase 2
						if (flashphase>=250 && flashphase<500) doflash = 1;
						break;
					case 0x0c :	// 2 Hz  Phase 3
						if (flashphase>=500 && flashphase<750) doflash = 1;
						break;
					case 0x10 :	// incremental flash
						incflash++;
						if (incflash>3) incflash = 1;
						switch (incflash)
						{
							case 1: if (flashphase<250) doflash = 1; break;
							case 2: if (flashphase>=250 && flashphase<500) doflash = 1;break;
							case 3: if (flashphase>=500 && flashphase<750) doflash = 1;
						}
						break;
					case 0x11 :	// decremental flash
						decflash--;
						if (decflash<1) decflash = 3;
						switch (decflash)
						{
							case 1: if (flashphase<250) doflash = 1; break;
							case 2: if (flashphase>=250 && flashphase<500) doflash = 1;break;
							case 3: if (flashphase>=500 && flashphase<750) doflash = 1;
						}
						break;

				}

				switch (flashattr.flashing &0x03) // Flash Mode
				{
					case 0x01 :	// normal Flashing
						if (doflash) flashattr.fg = flashattr.bg;
						break;
					case 0x02 :	// inverted Flashing
						doflash = 1-doflash;
						if (doflash) flashattr.fg = flashattr.bg;
						break;
					case 0x03 :	// color Flashing
						if (doflash) flashattr.fg = flashattr.fg + (flashattr.fg > 7 ? (-8) : 8);
						break;

				}
				RenderCharFB(flashchar,&flashattr);
				if (flashattr.doublew) col++;
				if (flashattr.doubleh) dhset = 1;
			}
		}
		if (dhset)
		{
			row++;
			PosY += fontheight*factor;
		}
		PosY += fontheight*factor;
	}

}
void DoRender(int startrow, int national_subset_bak)
{
	int row, col, byte;
		if (boxed)
		{ 
			if (screenmode != 0) 
				SwitchScreenMode(0); /* turn off divided screen */
		}
		else 
		{ 
			if (screenmode != prevscreenmode && !transpmode) 
				SwitchScreenMode(prevscreenmode);
		}

		/* display first column?  */
		nofirst = show39;
		for (row = 1; row < 24; row++)
		{
			byte = page_char[row*40];
			if (byte != ' '  && byte != 0x00 && byte != 0xFF &&
				page_atrb[row*40].fg != page_atrb[row*40].bg)
			{
				nofirst = 0;
				break;
			}
		}
		fontwidth_normal = (ex-sx) / (40-nofirst);
		setfontwidth(fontwidth_normal);
		fontwidth_topmenumain = (TV43STARTX-sx) / (40-nofirst);
		fontwidth_topmenusmall = (ex- TOPMENUSTARTX) / TOPMENUCHARS;
		fontwidth_small = (TV169FULLSTARTX-sx)  / (40-nofirst);
		switch(screenmode)
		{
			case 0:	setfontwidth(fontwidth_normal)     ; displaywidth= (ex             -sx);break;
			case 1:  setfontwidth(fontwidth_topmenumain); displaywidth= (TV43STARTX     -sx);break;
			case 2:  setfontwidth(fontwidth_small)      ; displaywidth= (TV169FULLSTARTX-sx);break;
		}
		if (transpmode || (boxed && !screenmode))
		{
			FillBorder(transp);//ClearBB(transp);
#ifndef HAVE_DREAMBOX_HARDWARE
			clearbbcolor = black;
#else
			clearbbcolor = transp;
#endif
		}

		/* get national subset */
		if (auto_national &&
			tuxtxt_cache.national_subset <= NAT_MAX_FROM_HEADER && /* not for GR/RU as long as line28 is not evaluated */
			pageinfo && pageinfo->nationalvalid) /* individual subset according to page header */
		{
			tuxtxt_cache.national_subset = countryconversiontable[pageinfo->national];
#if TUXTXT_DEBUG
			printf("p%03x b%d n%d v%d i%d\n", tuxtxt_cache.page,national_subset_bak, tuxtxt_cache.national_subset, pageinfo->nationalvalid, pageinfo->national);
#endif
		}
		/* render page */
		if (pageinfo && (pageinfo->function == FUNC_GDRCS || pageinfo->function == FUNC_DRCS)) /* character definitions */
		{
			#define DRCSROWS 8
			#define DRCSCOLS (48/DRCSROWS)
			#define DRCSZOOMX 3
			#define DRCSZOOMY 5
			#define DRCSXSPC (12*DRCSZOOMX + 2)
			#define DRCSYSPC (10*DRCSZOOMY + 2)

			unsigned char ax[] = { /* array[0..12] of x-offsets, array[0..10] of y-offsets for each pixel */
				DRCSZOOMX * 0,
				DRCSZOOMX * 1,
				DRCSZOOMX * 2,
				DRCSZOOMX * 3,
				DRCSZOOMX * 4,
				DRCSZOOMX * 5,
				DRCSZOOMX * 6,
				DRCSZOOMX * 7,
				DRCSZOOMX * 8,
				DRCSZOOMX * 9,
				DRCSZOOMX * 10,
				DRCSZOOMX * 11,
				DRCSZOOMX * 12,
				DRCSZOOMY * 0,
				DRCSZOOMY * 1,
				DRCSZOOMY * 2,
				DRCSZOOMY * 3,
				DRCSZOOMY * 4,
				DRCSZOOMY * 5,
				DRCSZOOMY * 6,
				DRCSZOOMY * 7,
				DRCSZOOMY * 8,
				DRCSZOOMY * 9,
				DRCSZOOMY * 10
			};
#if TUXTXT_DEBUG
			printf("TuxTxt <decoding *DRCS %03x/%02x %d>\n", tuxtxt_cache.page, tuxtxt_cache.subpage, pageinfo->function);
#endif
			ClearBB(black);
			for (col = 0; col < 24*40; col++)
				page_atrb[col] = atrtable[ATR_WB];

			for (row = 0; row < DRCSROWS; row++)
				for (col = 0; col < DRCSCOLS; col++)
					tuxtxt_RenderDRCS(var_screeninfo.xres,
						page_char + 20 * (DRCSCOLS * row + col + 2),
						lfb
						+ (StartY + fontheight + DRCSYSPC * row + var_screeninfo.yres - var_screeninfo.yoffset) * var_screeninfo.xres
						+ StartX + DRCSXSPC * col,
						ax, white, black);

			memset(page_char + 40, 0xff, 24*40); /* don't render any char below row 0 */
		}
		PosY = StartY + startrow*fontheight;
		for (row = startrow; row < 24; row++)
		{
			int index = row * 40;

			PosX = StartX;
			for (col = nofirst; col < 40; col++)
			{
				RenderCharBB(page_char[index + col], &page_atrb[index + col]);

				if (page_atrb[index + col].doubleh && page_char[index + col] != 0xff)	/* disable lower char in case of doubleh setting in l25 objects */
					page_char[index + col + 40] = 0xff;
				if (page_atrb[index + col].doublew)	/* skip next column if double width */
				{
					col++;
					if (page_atrb[index + col-1].doubleh && page_char[index + col] != 0xff)	/* disable lower char in case of doubleh setting in l25 objects */
						page_char[index + col + 40] = 0xff;
				}
			}
			PosY += fontheight;
		}
		DoFlashing(startrow);

		/* update framebuffer */
		CopyBB2FB();
		tuxtxt_cache.national_subset = national_subset_bak;
}
void RenderPage()
{
	int i, col, byte, startrow = 0;;
	int national_subset_bak = tuxtxt_cache.national_subset;


	/* update lcd */
	UpdateLCD();

	/* update page or timestring */
	if (transpmode != 2 && tuxtxt_cache.pageupdate && tuxtxt_cache.page_receiving != tuxtxt_cache.page && inputcounter == 2)
	{
		/* reset update flag */
		tuxtxt_cache.pageupdate = 0;
		if (boxed && subtitledelay) 
		{
			subtitle_cache* c = NULL;
			int j = -1;
			for (i = 0; i < SUBTITLE_CACHESIZE; i++)
			{
				if (j == -1 && !subtitlecache[i])
					j = i;
				if (subtitlecache[i] && !subtitlecache[i]->valid)
				{
					c = subtitlecache[i];
					break;
				}
			}
			if (c == NULL)
			{
				if (j == -1) // no more space in subtitlecache
					return;
				c= malloc(sizeof(subtitle_cache));
				if (c == NULL)
					return;
				memset(c, 0x00, sizeof(subtitle_cache));
				subtitlecache[j] = c;
			}
			c->valid = 0x01;
			gettimeofday(&c->tv_timestamp,NULL);
			if (tuxtxt_cache.subpagetable[tuxtxt_cache.page] != 0xFF)
			{
				tstPageinfo * p = tuxtxt_DecodePage(showl25,c->page_char,c->page_atrb,hintmode, showflof);
				if (p) 
				{
					boxed = p->boxed;
				}
			}
			delaystarted = 1;
			return;
		}
		delaystarted=0;
		/* decode page */
		if (tuxtxt_cache.subpagetable[tuxtxt_cache.page] != 0xFF)
		{
			tstPageinfo * p = tuxtxt_DecodePage(showl25,page_char,page_atrb,hintmode, showflof);
			if (p) 
			{
				pageinfo = p;
				boxed = p->boxed;
			}
			if (boxed || transpmode)
//				tuxtxt_cache.FullScrColor = transp;
				FillBorder(transp);
			else
				FillBorder(tuxtxt_cache.FullScrColor);
			if (tuxtxt_cache.colortable) /* as late as possible to shorten the time the old page is displayed with the new colors */
				setcolors(tuxtxt_cache.colortable, 16, 16); /* set colors for CLUTs 2+3 */
		}
		else
			startrow = 1;
		DoRender(startrow,national_subset_bak);
	}
	else if (transpmode != 2)
	{
		if (delaystarted)
		{
			struct timeval tv;
			gettimeofday(&tv,NULL);
			for (i = 0; i < SUBTITLE_CACHESIZE ; i++)
			{
				if (subtitlecache[i] && subtitlecache[i]->valid && tv.tv_sec - subtitlecache[i]->tv_timestamp.tv_sec >= subtitledelay)
				{
					memcpy(page_char, subtitlecache[i]->page_char,40 * 25);
					memcpy(page_atrb, subtitlecache[i]->page_atrb,40 * 25 * sizeof(tstPageAttr));
					DoRender(startrow,national_subset_bak);
					subtitlecache[i]->valid = 0;
					//memset(subtitlecache[i], 0x00, sizeof(subtitle_cache));
					return;
				}
			}
		}	
		if (zoommode != 2)
		{
			PosY = StartY;
			if (tuxtxt_cache.subpagetable[tuxtxt_cache.page] == 0xff)
			{
				page_atrb[32].fg = yellow;
				page_atrb[32].bg = menu1;
				int showpage = tuxtxt_cache.page_receiving;
				int showsubpage = tuxtxt_cache.subpagetable[showpage];
				if (showsubpage!=0xff)
				{

					tstCachedPage *pCachedPage;
					pCachedPage = tuxtxt_cache.astCachetable[showpage][showsubpage];
					if (pCachedPage && tuxtxt_is_dec(showpage))
					{
						PosX = StartX;
						if (inputcounter == 2)
						{
							if (tuxtxt_cache.bttok && !tuxtxt_cache.basictop[tuxtxt_cache.page]) /* page non-existent according to TOP (continue search anyway) */
							{
								page_atrb[0].fg = white;
								page_atrb[0].bg = red;
							}
							else
							{
								page_atrb[0].fg = yellow;
								page_atrb[0].bg = menu1;
							}
							tuxtxt_hex2str(page_char+3, tuxtxt_cache.page);
							for (col = nofirst; col < 7; col++) // selected page
							{
								RenderCharFB(page_char[col], &page_atrb[0]);
							}
							RenderCharFB(page_char[col], &page_atrb[32]);
						}
						else
							SetPosX(8);

						memcpy(&page_char[8], pCachedPage->p0, 24); /* header line without timestring */
						for (col = 0; col < 24; col++)
						{
							RenderCharFB(pCachedPage->p0[col], &page_atrb[32]);
						}
					}
				}
			}
			/* update timestring */
			SetPosX(32);
			for (byte = 0; byte < 8; byte++)
			{
				if (!page_atrb[32+byte].flashing)
					RenderCharFB(tuxtxt_cache.timestring[byte], &page_atrb[32]);
				else
				{
					SetPosX(33+byte);
					page_char[32+byte] = page_char[32+byte];
				}


			}
		}
		DoFlashing(startrow);
		tuxtxt_cache.national_subset = national_subset_bak;
	}
	else if (transpmode == 2 && tuxtxt_cache.pageupdate == 2)
	{
#if TUXTXT_DEBUG
		printf("received Update flag for page %03x\n",tuxtxt_cache.page);
#endif
		// display pagenr. when page has been updated while in transparency mode
		PosY = StartY;

		char ns[3];
		SetPosX(1);
		tuxtxt_hex2str(ns+2,tuxtxt_cache.page);

		RenderCharFB(ns[0],&atrtable[ATR_WB]);
		RenderCharFB(ns[1],&atrtable[ATR_WB]);
		RenderCharFB(ns[2],&atrtable[ATR_WB]);

		tuxtxt_cache.pageupdate=0;
	}
}

/******************************************************************************
 * CreateLine25                                                               *
 ******************************************************************************/

void showlink(int column, int linkpage)
{
	unsigned char *p, line[] = "   >???   ";
	int oldfontwidth = fontwidth;
	int yoffset;

	if (var_screeninfo.yoffset)
		yoffset = 0;
	else
		yoffset = var_screeninfo.yres;

	int abx = ((displaywidth)%(40-nofirst) == 0 ? displaywidth+1 : (displaywidth)/(((displaywidth)%(40-nofirst)))+1);// distance between 'inserted' pixels
	int width = displaywidth /4;

	PosY = StartY + 24*fontheight;

	if (boxed)
	{
		PosX = StartX + column*width;
		tuxtxt_FillRect(lfb,var_screeninfo.xres,PosX, PosY+yoffset, displaywidth, fontheight, transp);
		return;
	}

	if (tuxtxt_cache.adip[linkpage][0])
	{
		PosX = StartX + column*width;
		int l = strlen(tuxtxt_cache.adip[linkpage]);

		if (l > 9) /* smaller font, if no space for one half space at front and end */
			setfontwidth(oldfontwidth * 10 / (l+1));
		tuxtxt_FillRect(lfb,var_screeninfo.xres,PosX, PosY+yoffset, width+(displaywidth%4), fontheight, atrtable[ATR_L250 + column].bg);
		PosX += ((width) - (l*fontwidth+l*fontwidth/abx))/2; /* center */
		for (p = tuxtxt_cache.adip[linkpage]; *p; p++)
			RenderCharBB(*p, &atrtable[ATR_L250 + column]);
		setfontwidth(oldfontwidth);
	}
	else /* display number */
	{
		PosX = StartX + column*width;
		tuxtxt_FillRect(lfb,var_screeninfo.xres,PosX, PosY+yoffset, displaywidth+sx-PosX, fontheight, atrtable[ATR_L250 + column].bg);
		if (linkpage < tuxtxt_cache.page)
		{
			line[6] = '<';
			tuxtxt_hex2str(line + 5, linkpage);
		}
		else
			tuxtxt_hex2str(line + 6, linkpage);
		for (p = line; p < line+9; p++)
			RenderCharBB(*p, &atrtable[ATR_L250 + column]);
	}
}

void CreateLine25()
{

	if (!tuxtxt_cache.bttok)
		/* btt completely received and not yet decoded */
		tuxtxt_decode_btt();
	if (tuxtxt_cache.maxadippg >= 0)
		tuxtxt_decode_adip();

	if (!showhex && showflof &&
		 (tuxtxt_cache.flofpages[tuxtxt_cache.page][0] || tuxtxt_cache.flofpages[tuxtxt_cache.page][1] || tuxtxt_cache.flofpages[tuxtxt_cache.page][2] || tuxtxt_cache.flofpages[tuxtxt_cache.page][3])) // FLOF-Navigation present
	{
		int i;

		prev_100 = tuxtxt_cache.flofpages[tuxtxt_cache.page][0];
		prev_10  = tuxtxt_cache.flofpages[tuxtxt_cache.page][1];
		next_10  = tuxtxt_cache.flofpages[tuxtxt_cache.page][2];
		next_100 = tuxtxt_cache.flofpages[tuxtxt_cache.page][3];

		PosY = StartY + 24*fontheight;
		PosX = StartX;
		for (i=nofirst; i<40; i++)
			RenderCharBB(page_char[24*40 + i], &page_atrb[24*40 + i]);
	}
	else
	{
		/*  normal: blk-1, grp+1, grp+2, blk+1 */
		/*  hex:    hex+1, blk-1, grp+1, blk+1 */
		if (showhex)
		{
			/* arguments: startpage, up, findgroup */
			prev_100 = tuxtxt_next_hex(tuxtxt_cache.page);
			prev_10  = toptext_getnext(tuxtxt_cache.page, 0, 0);
			next_10  = toptext_getnext(tuxtxt_cache.page, 1, 1);
		}
		else
		{
			prev_100 = toptext_getnext(tuxtxt_cache.page, 0, 0);
			prev_10  = toptext_getnext(tuxtxt_cache.page, 1, 1);
			next_10  = toptext_getnext(prev_10, 1, 1);
		}
		next_100 = toptext_getnext(next_10, 1, 0);
		showlink(0, prev_100);
		showlink(1, prev_10);
		showlink(2, next_10);
		showlink(3, next_100);
	}

	if (//tuxtxt_cache.bttok &&
		 screenmode == 1) /* TOP-Info present, divided screen -> create TOP overview */
	{
		char line[TOPMENUCHARS];
		int current;
		int prev10done, next10done, next100done, indent;
		tstPageAttr *attrcol, *attr; /* color attribute for navigation keys and text */

		int olddisplaywidth = displaywidth;
		displaywidth = 1000*(40-nofirst); // disable pixelinsert;
		setfontwidth(fontwidth_topmenusmall);

		PosY = TOPMENUSTARTY;
		memset(line, ' ', TOPMENUCHARS); /* init with spaces */

		memcpy(line+TOPMENUINDENTBLK, tuxtxt_cache.adip[prev_100], 12);
		tuxtxt_hex2str(&line[TOPMENUINDENTDEF+12+TOPMENUSPC+2], prev_100);
		RenderClearMenuLineBB(line, &atrtable[ATR_L250], &atrtable[ATR_TOPMENU2]);

/*  1: blk-1, grp-1, grp+1, blk+1 */
/*  2: blk-1, grp+1, grp+2, blk+1 */
#if (LINE25MODE == 1)
		current = prev_10 - 1;
#else
		current = tuxtxt_cache.page - 1;
#endif

		prev10done = next10done = next100done = 0;
		while (PosY <= (TOPMENUENDY-fontheight))
		{
			attr = 0;
			attrcol = &atrtable[ATR_WB];
			if (!next100done && (PosY > (TOPMENUENDY - 2*fontheight))) /* last line */
			{
				attrcol = &atrtable[ATR_L253];
				current = next_100;
			}
			else if (!next10done && (PosY > (TOPMENUENDY - 3*fontheight))) /* line before */
			{
				attrcol = &atrtable[ATR_L252];
				current = next_10;
			}
			else if (!prev10done && (PosY > (TOPMENUENDY - 4*fontheight))) /* line before */
			{
				attrcol = &atrtable[ATR_L251];
				current = prev_10;
			}
			else do
			{
				tuxtxt_next_dec(&current);
				if (current == prev_10)
				{
					attrcol = &atrtable[ATR_L251];
					prev10done = 1;
					break;
				}
				else if (current == next_10)
				{
					attrcol = &atrtable[ATR_L252];
					next10done = 1;
					break;
				}
				else if (current == next_100)
				{
					attrcol = &atrtable[ATR_L253];
					next100done = 1;
					break;
				}
				else if (current == tuxtxt_cache.page)
				{
					attr = &atrtable[ATR_TOPMENU0];
					break;
				}
			} while (tuxtxt_cache.adip[current][0] == 0 && (tuxtxt_cache.basictop[current] < 2 || tuxtxt_cache.basictop[current] > 7));

			if (!tuxtxt_cache.bttok || (tuxtxt_cache.basictop[current] >= 2 && tuxtxt_cache.basictop[current] <= 5)) /* block (also for FLOF) */
			{
				indent = TOPMENUINDENTBLK;
				if (!attr)
					attr = &atrtable[tuxtxt_cache.basictop[current] <=3 ? ATR_TOPMENU1 : ATR_TOPMENU2]; /* green for program block */
			}
			else if (tuxtxt_cache.basictop[current] >= 6 && tuxtxt_cache.basictop[current] <= 7) /* group */
			{
				indent = TOPMENUINDENTGRP;
				if (!attr)
					attr = &atrtable[ATR_TOPMENU3];
			}
			else
			{
				indent = TOPMENUINDENTDEF;
				if (!attr)
					attr = &atrtable[ATR_WB];
			}
			memcpy(line+indent, tuxtxt_cache.adip[current], 12);
			tuxtxt_hex2str(&line[TOPMENUINDENTDEF+12+TOPMENUSPC+2], current);
			RenderClearMenuLineBB(line, attrcol, attr);
		}
		displaywidth = olddisplaywidth;
		setfontwidth(fontwidth_topmenumain);
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
			FillBorder(*(lfb + var_screeninfo.xres * var_screeninfo.yoffset));
//			 ClearBB(*(lfb + var_screeninfo.xres * var_screeninfo.yoffset));

		if (clearbbcolor >= 0)
		{
//			ClearBB(clearbbcolor);
			clearbbcolor = -1;
		}
		return;
	}

	src = dst = topsrc = lfb + StartY*var_screeninfo.xres;


	if (var_screeninfo.yoffset)
		dst += var_screeninfo.xres * var_screeninfo.yres;
	else
	{
		src += var_screeninfo.xres * var_screeninfo.yres;
		topsrc += var_screeninfo.xres * var_screeninfo.yres;
	}
	if (!pagecatching )
		memcpy(dst+(24*fontheight)*var_screeninfo.xres, src + (24*fontheight)*var_screeninfo.xres, var_screeninfo.xres*fontheight); /* copy line25 in normal height */

	if (transpmode)
		fillcolor = transp;
	else
		fillcolor = tuxtxt_cache.FullScrColor;

	if (zoommode == 2)
		src += 12*fontheight*var_screeninfo.xres;

	if (screenmode == 1) /* copy topmenu in normal height (since PIG also keeps dimensions) */
	{
		unsigned char *topdst = dst;

		screenwidth = TV43STARTX;

		topsrc += screenwidth;
		topdst += screenwidth;
		for (i=0; i < 24*fontheight; i++)
		{
			memcpy(topdst, topsrc,ex-screenwidth);
			topdst += var_screeninfo.xres;
			topsrc += var_screeninfo.xres;
		}
	}
	else if (screenmode == 2)
		screenwidth = TV169FULLSTARTX;
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

//	if (!pagecatching )
//		memcpy(dst, lfb + (StartY+24*fontheight)*var_screeninfo.xres, var_screeninfo.xres*fontheight); /* copy line25 in normal height */
	for (i = var_screeninfo.yres - StartY - 25*fontheight; i >= 0;i--)
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
			p = tuxtxt_cache.page;
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
		if (old_subpage != tuxtxt_cache.subpage)
		{
			if (!tuxtxt_cache.subpage)
			{
				RenderCharLCD(0, 55, 20);
				RenderCharLCD(1, 67, 20);
			}
			else
			{
				if (tuxtxt_cache.subpage >= 0xFF)
					tuxtxt_cache.subpage = 1;
				else if (tuxtxt_cache.subpage > 99)
					tuxtxt_cache.subpage = 0;

				RenderCharLCD(tuxtxt_cache.subpage>>4, 55, 20);
				RenderCharLCD(tuxtxt_cache.subpage&0x0F, 67, 20);
			}

			old_subpage = tuxtxt_cache.subpage;
			update_lcd = 1;
		}

		/* max subpage */
		for (x = 0; x <= 0x79; x++)
		{
			if (tuxtxt_cache.astCachetable[tuxtxt_cache.page][x])
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
		if (old_cached_pages != tuxtxt_cache.cached_pages)
		{
			#if 0
			int s;
			int p = tuxtxt_cache.cached_pages;
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
			RenderCharLCD(tuxtxt_cache.cached_pages/1000, 67, 44);
			RenderCharLCD(tuxtxt_cache.cached_pages%1000/100, 79, 44);
			RenderCharLCD(tuxtxt_cache.cached_pages%100/10, 91, 44);
			RenderCharLCD(tuxtxt_cache.cached_pages%10, 103, 44);
			#endif

			old_cached_pages = tuxtxt_cache.cached_pages;
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
/* Local Variables: */
/* indent-tabs-mode:t */
/* tab-width:3 */
/* c-basic-offset:3 */
/* comment-column:0 */
/* fill-column:120 */
/* End: */
