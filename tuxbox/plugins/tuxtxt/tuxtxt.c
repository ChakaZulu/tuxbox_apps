/******************************************************************************
 *                    <<< TuxTxt - Videotext via d-box >>>                    *
 *                                                                            *
 *                        (c) Thomas "LazyT" Loewe '02                        *
 *----------------------------------------------------------------------------*
 * History                                                                    *
 *                                                                            *
 *    V1.9: enx fixed, subpage zapping fixed, tvmode reactivated              *
 *    V1.8: zap subpages, text&picture mode, subtitle fixed                   *
 *    V1.7: speedup?, neutrino look ;)                                        *
 *    V1.6: added transparency mode                                           *
 *    V1.5: added newsflash/subtitle support                                  *
 *    V1.4: skip not received pages on +/-10, some mods                       *
 *    V1.3: segfault fixed                                                    *
 *    V1.2: made it work under enigma                                         *
 *    V1.1: added tuxtxt to cvs                                               *
 ******************************************************************************/

#include "tuxtxt.h"

/******************************************************************************
 * plugin_exec                                                                *
 ******************************************************************************/

void plugin_exec(PluginParam *par)
{
	//show versioninfo

		printf("\nTuxTxt 1.9 - Coypright (c) Thomas \"LazyT\" Loewe and the TuxBox-Team\n\n");

	//get params

		vtxtpid = -1;
		fb = -1;
		rc = -1;
		sx = -1;
		ex = -1;
		sy = -1;
		ey = -1;

		for(; par; par = par->next)
		{
			if (!strcmp(par->id, P_ID_VTXTPID))
			{
				vtxtpid = atoi(par->val);
			}
			else if(!strcmp(par->id, P_ID_FBUFFER))
			{
				fb = atoi(par->val);
			}
			else if (!strcmp(par->id, P_ID_RCINPUT))
			{
				rc = atoi(par->val);
			}
			else if (!strcmp(par->id, P_ID_OFF_X))
			{
				sx = atoi(par->val);
			}
			else if (!strcmp(par->id, P_ID_END_X))
			{
				ex = atoi(par->val);
			}
			else if (!strcmp(par->id, P_ID_OFF_Y))
			{
				sy = atoi(par->val);
			}
			else if (!strcmp(par->id, P_ID_END_Y))
			{
				ey = atoi(par->val);
			}
		}

		if(vtxtpid == -1 || fb == -1 || rc == -1 || sx == -1 || ex == -1 || sy == -1 || ey == -1)
		{
			printf("TuxTxt <Invalid Param(s)>\n");
			return;
		}

	//initialisations

		if(Init() == 0) return;

	//main loop

		do
		{
			if(GetRCCode() == 1)
			{
				switch(RCCode)
				{
					case RC_UP:		GetNextPageOne();
									break;

					case RC_DOWN:	GetPrevPageOne();
									break;

					case RC_RIGHT:	GetNextPageTen();
									break;

					case RC_LEFT:	GetPrevPageTen();
									break;

					case RC_PLUS:	GetNextSubPage();
									break;

					case RC_MINUS:	GetPrevSubPage();
									break;

					case RC_0:		PageInput(0);
									break;

					case RC_1:		PageInput(1);
									break;

					case RC_2:		PageInput(2);
									break;

					case RC_3:		PageInput(3);
									break;

					case RC_4:		PageInput(4);
									break;

					case RC_5:		PageInput(5);
									break;

					case RC_6:		PageInput(6);
									break;

					case RC_7:		PageInput(7);
									break;

					case RC_8:		PageInput(8);
									break;

					case RC_9:		PageInput(9);
									break;

					case RC_OK:		SwitchTranspMode();
									break;

					case RC_DBOX:	SwitchScreenMode();
									break;

					case RC_MUTE:	SwitchTVMode();
				}
			}

			//update page or timestring

				RenderPage();

		}
		while(RCCode != RC_HOME);

	//exit

		CleanUp();
}

/******************************************************************************
 * Init                                                                       *
 ******************************************************************************/

int Init()
{
	struct dmxPesFilterParams dmx_flt;
	int error;

	//open demuxer

		if((dmx = open("/dev/ost/demux0", O_RDWR)) == -1)
		{
			perror("TuxTxt <open /dev/ost/demux0>");
			return 0;
		}

	//open pig

		pig = -1;

		if((pig = open("/dev/dbox/pig0", O_RDONLY)) == -1)
		{
			perror("TuxTxt <open /dev/dbox/pig0>");
		}

	//setup rc

		fcntl(rc, F_SETFL, O_NONBLOCK);
		ioctl(rc, RC_IOCTL_BCODES, 1);

	//init fontlibrary

		if((error = FT_Init_FreeType(&library)) != 0)
		{
			printf("TuxTxt <FT_Init_FreeType => 0x%.2X>", error);
			return 0;
		}

	//load font

		if((error = FT_New_Face(library,"/share/fonts/tuxtxt.fon", 0, &face)) != 0)
		{
			printf("TuxTxt <FT_New_Face => 0x%.2X>", error);
			return 0;
		}

	//set fontsize

		fontwidth  = 16;
		fontheight = 22;

		if((error = FT_Set_Pixel_Sizes(face, fontwidth, fontheight)) != 0)
		{
			printf("TuxTxt <FT_Set_Pixel_Sizes => 0x%.2X>", error);
			return 0;
		}

	//center screen

		StartX = sx + (((ex-sx) - 40*fontwidth) / 2);
		StartY = sy + (((ey-sy) - 24*fixfontheight) / 2);

	//get fixed screeninfo

		if (ioctl(fb, FBIOGET_FSCREENINFO, &fix_screeninfo) == -1)
		{
			perror("TuxTxt <FBIOGET_FSCREENINFO>");
			return 0;
		}

	//get variable screeninfo

		if (ioctl(fb, FBIOGET_VSCREENINFO, &var_screeninfo) == -1)
		{
			perror("TuxTxt <FBIOGET_VSCREENINFO>");
			return 0;
		}

	//set new colormap

		if (ioctl(fb, FBIOPUTCMAP, &colormap) == -1)
		{
			perror("TuxTxt <FBIOPUTCMAP>");
			return 0;
		}

	//map framebuffer into memory
    
		lfb = (unsigned char*)mmap(0, fix_screeninfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);

		if(!lfb)
		{
			perror("TuxTxt <mmap>");
			return 0;
		}

	//init data

		memset(&cachetable, 0, sizeof(cachetable));
		memset(&subpagetable, 0xFF, sizeof(subpagetable));
		memset(&backbuffer, black, sizeof(backbuffer));

		page_atrb[32] = transp<<4 | transp;

		inputcounter = 2;

		current_page	= -1;
		current_subpage	= -1;

		page	 = 0x100;
		lastpage = 0x100;
		subpage	 = 0;

		pageupdate = 0;

		zap_subpage_manual = 0;

	//set filter & start demuxer

		dmx_flt.pid		= vtxtpid;
		dmx_flt.input	= DMX_IN_FRONTEND;
		dmx_flt.output	= DMX_OUT_TAP;
		dmx_flt.pesType	= DMX_PES_OTHER;
		dmx_flt.flags	= DMX_IMMEDIATE_START;

		if(ioctl(dmx, DMX_SET_PES_FILTER, &dmx_flt) == -1)
		{
			perror("TuxTxt <DMX_SET_PES_FILTER>");
			return 0;
		}

	//show infobar

		RenderPageNotFound();

	//create decode-thread

		if(pthread_create(&thread_id, NULL, CacheThread, NULL) != 0)
		{
			perror("TuxTxt <pthread_create>");
			return 0;
		}

	//init successfull

		return 1;
}

/******************************************************************************
 * Cleanup                                                                    *
 ******************************************************************************/

void CleanUp()
{
	int clear_page, clear_subpage;

	//hide pig

		if(screenmode) avia_pig_hide(pig);

	//stop decode-thread

		if(pthread_cancel(thread_id) != 0)
		{
			perror("TuxTxt <pthread_cancel>");
			return;
		}

		if(pthread_join(thread_id, &thread_result) != 0)
		{
			perror("TuxTxt <pthread_join>");
			return;
		}

	//stop & close demuxer

		ioctl(dmx, DMX_STOP);
		close(dmx);

	//close freetype

		FT_Done_FreeType(library);

	//unmap framebuffer

		munmap(lfb, fix_screeninfo.smem_len);

	//free pagebuffers

		for(clear_page = 0; clear_page < 0x8FF; clear_page++)
		{
			for(clear_subpage = 0; clear_subpage < 0x79; clear_subpage++)
			{
				if(cachetable[clear_page][clear_subpage] != 0);
				{
					free(cachetable[clear_page][clear_subpage]);
				}
			}
		}
}

/******************************************************************************
 * PageInput                                                                  *
 ******************************************************************************/

void PageInput(int Number)
{
	static int temp_page;

	//clear temp_page

		if(inputcounter == 2) temp_page = 0;

	//check for 0 & 9 on first position

		if(Number == 0 && inputcounter == 2)
		{
			//show feedback

				PosX = StartX + 8*fontwidth;
				PosY = StartY;
				RenderCharFB('<', white);
				RenderCharFB('0', white);
				RenderCharFB('>', white);

			//set page

				temp_page = 0x100;

				inputcounter = -1;
		}
		else if(Number == 9 && inputcounter == 2)
		{
			//show feedback

				PosX = StartX + 8*fontwidth;
				PosY = StartY;
				RenderCharFB('<', white);
				RenderCharFB('9', white);
				RenderCharFB('>', white);

			//set page

				temp_page = lastpage;

				inputcounter = -1;
		}

	//show pageinput

		PosY = StartY;

		switch(inputcounter)
		{
			case 2:	PosX = StartX + 8*fontwidth;
					RenderCharFB(Number | '0', white);
					RenderCharFB('-', white);
					RenderCharFB('-', white);
					break;

			case 1:	PosX = StartX + 9*fontwidth;
					RenderCharFB(Number | '0', white);
					break;

			case 0:	PosX = StartX + 10*fontwidth;
					RenderCharFB(Number | '0', white);
					break;
		}

	//generate pagenumber

		temp_page |= Number << inputcounter*4;

		inputcounter--;

		if(inputcounter < 0)
		{
			//disable subpage zapping

				zap_subpage_manual = 0;

			//reset input

				inputcounter = 2;

			//set new page

				lastpage = page;

				page = temp_page;

			//check cache

				if(subpagetable[page] != 0xFF)
				{
					subpage = subpagetable[page];
					pageupdate = 1;
					printf("TuxTxt <DirectInput => %.3X-%.2X>\n", page, subpage);
				}
				else
				{
					RenderPageNotFound();
					printf("TuxTxt <DirectInput => %.3X not found>\n", page);
				}
		}
}

/******************************************************************************
 * GetNextPageOne                                                             *
 ******************************************************************************/

void GetNextPageOne()
{
	//disable subpage zapping

		zap_subpage_manual = 0;

	//abort pageinput

		inputcounter = 2;

	//show feedback

		PosX = StartX + 8*fontwidth;
		PosY = StartY;
		RenderCharFB('-', white);
		RenderCharFB('-', white);
		RenderCharFB('>', white);

	//find next cached page

		lastpage = page;

		do
		{
			page++;

			//skip hex pages

				if((page & 0x00F) > 0x009) page += 0x006;
				if((page & 0x0F0) > 0x090) page += 0x060;

			//wrap around

				if(page > 0x899) page = 0x100;

		}while(subpagetable[page] == 0xFF);

	//update page

		subpage = subpagetable[page];
		pageupdate = 1;
		printf("TuxTxt <NextPageOne => %.3X-%.2X>\n", page, subpage);
}

/******************************************************************************
 * GetPrevPageOne                                                             *
 ******************************************************************************/

void GetPrevPageOne()
{
	//disable subpage zapping

		zap_subpage_manual = 0;

	//abort pageinput

		inputcounter = 2;

	//show feedback

		PosX = StartX + 8*fontwidth;
		PosY = StartY;
		RenderCharFB('<', white);
		RenderCharFB('-', white);
		RenderCharFB('-', white);

	//find previous cached page

		lastpage = page;

		do
		{
			page--;

			//skip hex pages

				if((page & 0x00F) > 0x009) page -= 0x006;
				if((page & 0x0F0) > 0x090) page -= 0x060;

			//wrap around

				if(page < 0x100) page = 0x899;

		}while(subpagetable[page] == 0xFF);

	//update page

		subpage = subpagetable[page];
		pageupdate = 1;
		printf("TuxTxt <PrevPageOne => %.3X-%.2X>\n", page, subpage);
}

/******************************************************************************
 * GetNextPageTen                                                             *
 ******************************************************************************/

void GetNextPageTen()
{
	//disable subpage zapping

		zap_subpage_manual = 0;

	//abort pageinput

		inputcounter = 2;

	//show feedback

		PosX = StartX + 8*fontwidth;
		PosY = StartY;
		RenderCharFB('-', white);
		RenderCharFB('>', white);
		RenderCharFB('>', white);

	//find next cached page

		lastpage = page;

		page &= 0xFF0;

		do
		{
			//find next ten

				if((page & 0x0F0) == 0x090) page += 0x070;
				else						page += 0x010;

			//wrap around

				if(page >= 0x900) page = 0x100;

		}while(subpagetable[page] == 0xFF);

	//update page

		subpage = subpagetable[page];
		pageupdate = 1;
		printf("TuxTxt <NextPageTen => %.3X-%.2X>\n", page, subpage);
}

/******************************************************************************
 * GetPrevPageTen                                                             *
 ******************************************************************************/

void GetPrevPageTen()
{
	//disable subpage zapping

		zap_subpage_manual = 0;

	//abort pageinput

		inputcounter = 2;

	//show feedback

		PosX = StartX + 8*fontwidth;
		PosY = StartY;
		RenderCharFB('<', white);
		RenderCharFB('<', white);
		RenderCharFB('-', white);

	//find previous cached page

		lastpage = page;

		if(page & 0x00F) page += 0x010;

		page &= 0xFF0;

		do
		{
			//find previous ten

				if((page & 0x0F0) == 0x000) page -= 0x070;
				else 						page -= 0x010;

			//wrap around

				if(page <= 0x090) page = 0x890;

		}while(subpagetable[page] == 0xFF);

	//update page

		subpage = subpagetable[page];
		pageupdate = 1;
		printf("TuxTxt <PrevPageTen => %.3X-%.2X>\n", page, subpage);
}

/******************************************************************************
 * GetNextSubPage                                                             *
 ******************************************************************************/

void GetNextSubPage()
{
	int loop;

	//abort pageinput

		inputcounter = 2;

	//search subpage

		if(subpage != 0)
		{
			//search next subpage

				for(loop = subpage + 1; loop <= 0x79; loop++)
				{
					if(cachetable[page][loop] != 0)
					{
						//show feedback

							PosX = StartX + 8*fontwidth;
							PosY = StartY;
							RenderCharFB('S', white);
							RenderCharFB('+', white);
							RenderCharFB('1', white);

						//enable manual subpage zapping

							zap_subpage_manual = 1;

						//update page

							subpage = loop;
							pageupdate = 1;
							printf("TuxTxt <NextSubPage => %.3X-%.2X>\n", page, subpage);
							return;
					}
				}

				for(loop = 1; loop < subpage; loop++)
				{
					if(cachetable[page][loop] != 0)
					{
						//show feedback

							PosX = StartX + 8*fontwidth;
							PosY = StartY;
							RenderCharFB('S', white);
							RenderCharFB('+', white);
							RenderCharFB('1', white);

						//enable manual subpage zapping

							zap_subpage_manual = 1;

						//update page

							subpage = loop;
							pageupdate = 1;
							printf("TuxTxt <NextSubPage => %.3X-%.2X>\n", page, subpage);
							return;
					}
				}			

				printf("TuxTxt <NextSubPage => no other SubPage>\n");
		}
		else
		{
			printf("TuxTxt <NextSubPage => no SubPages>\n");
		}
}

/******************************************************************************
 * GetPrevSubPage                                                             *
 ******************************************************************************/

void GetPrevSubPage()
{
	int loop;

	//abort pageinput

		inputcounter = 2;

	//search subpage

		if(subpage != 0)
		{
			//search previous subpage

				for(loop = subpage - 1; loop > 0x00; loop--)
				{
					if(cachetable[page][loop] != 0)
					{
						//show feedback

							PosX = StartX + 8*fontwidth;
							PosY = StartY;
							RenderCharFB('S', white);
							RenderCharFB('-', white);
							RenderCharFB('1', white);

						//enable manual subpage zapping

							zap_subpage_manual = 1;

						//update page

							subpage = loop;
							pageupdate = 1;
							printf("TuxTxt <PrevSubPage => %.3X-%.2X>\n", page, subpage);
							return;
					}
				}

				for(loop = 0x79; loop > subpage; loop--)
				{
					if(cachetable[page][loop] != 0)
					{
						//show feedback

							PosX = StartX + 8*fontwidth;
							PosY = StartY;
							RenderCharFB('S', white);
							RenderCharFB('-', white);
							RenderCharFB('1', white);

						//enable manual subpage zapping

							zap_subpage_manual = 1;

						//update page

							subpage = loop;
							pageupdate = 1;
							printf("TuxTxt <PrevSubPage => %.3X-%.2X>\n", page, subpage);
							return;
					}
				}			

				printf("TuxTxt <PrevSubPage => no other SubPage>\n");
		}
		else
		{
			printf("TuxTxt <PrevSubPage => no SubPages>\n");
		}
}

/******************************************************************************
 * SwitchTranspMode                                                           *
 ******************************************************************************/

void SwitchTranspMode()
{
	if(!screenmode)
	{
		//toggle mode

			transpmode++;
			transpmode &= 1;

			printf("TuxTxt <SwitchTranspMode => %d>\n", transpmode);

		//update page

			pageupdate = 1;
	}
}

/******************************************************************************
 * SwitchScreenMode                                                           *
 ******************************************************************************/

void SwitchScreenMode()
{
	int error;

	if(pig != -1)
	{
		//reset transparency mode

			if(transpmode) SwitchTranspMode();

		//toggle mode

			screenmode++;
			screenmode &= 1;

			printf("TuxTxt <SwitchScreenMode => %d>\n", screenmode);

		//update page

			pageupdate = 1;

		//clear backbuffer

			memset(&backbuffer, black, sizeof(backbuffer));

		//set mode

			if(screenmode)
			{
				fontwidth  =  8;
				fontheight = 21;

				avia_pig_set_pos(pig, (StartX+269), (StartY+20));
				avia_pig_set_size(pig, 320, 504);
				avia_pig_show(pig);
			}
			else
			{
				fontwidth  = 16;
				fontheight = 22;

				avia_pig_hide(pig);
			}

			if((error = FT_Set_Pixel_Sizes(face, fontwidth, fontheight)) != 0)
			{
				printf("TuxTxt <FT_Set_Pixel_Sizes => 0x%.2X>", error);
			}
	}
}

/******************************************************************************
 * SwitchTVMode                                                               *
 ******************************************************************************/

void SwitchTVMode()
{
	if(!screenmode)
	{
		//toggle mode

			tvmode++;
			tvmode &= 1;

			printf("TuxTxt <SwitchTVMode => %d>\n", tvmode);

		//set mode

			if(tvmode)
			{
				memset(lfb, transp, var_screeninfo.xres * var_screeninfo.yres);
			}
			else
			{
				pageupdate = 1;
			}
	}
}

/******************************************************************************
 * RenderCharFB                                                               *
 ******************************************************************************/

void RenderCharFB(int Char, int Attribute)
{
	int Row, Pitch, Bit, x = 0, y = 0;
	int error;

	//load char

		if((error = FT_Load_Char(face, Char + ((Attribute>>8 & 1) * (128-32)), FT_LOAD_RENDER | FT_LOAD_MONOCHROME)) != 0)
		{
			printf("TuxTxt <FT_Load_Char => 0x%.2X>\n", error);
			PosX += fontwidth;
			return;
		}

	//render char

		for(Row = 0; Row < fixfontheight; Row++)
		{
			for(Pitch = 0; Pitch < face->glyph->bitmap.pitch; Pitch++)
			{
				for(Bit = 7; Bit >= 0; Bit--)
				{
					if((face->glyph->bitmap.buffer[Row * face->glyph->bitmap.pitch + Pitch]) & 1<<Bit)
					{
						*(lfb + (x+PosX) + ((y+PosY)*var_screeninfo.xres)) = Attribute & 15;
					}
					else
					{
						if(transpmode)
						{
							Attribute &= 0xFF0F;
							Attribute |= transp<<4;
						}

						*(lfb + (x+PosX) + ((y+PosY)*var_screeninfo.xres)) = Attribute>>4 & 15;
					}

					x++;
				}
			}

			x = 0;
			y++;
		}

	PosX += fontwidth;
}

/******************************************************************************
 * RenderCharBB                                                               *
 ******************************************************************************/

void RenderCharBB(int Char, int Attribute)
{
	int Row, Pitch, Bit, x = 0, y = 0;
	int error;

	//skip doubleheight chars in lower line

		if(Char == 0xFF)
		{
			PosX += fontwidth;
			return;
		}

	//load char

		if((error = FT_Load_Char(face, Char + ((Attribute>>8 & 1) * (128-32)), FT_LOAD_RENDER | FT_LOAD_MONOCHROME)) != 0)
		{
			printf("TuxTxt <FT_Load_Char %.3d => 0x%.2X>\n", Char, error);
			PosX += fontwidth;
			return;
		}

	//render char

		for(Row = 0; Row < fixfontheight; Row++)
		{
			for(Pitch = 0; Pitch < face->glyph->bitmap.pitch; Pitch++)
			{
				for(Bit = 7; Bit >= 0; Bit--)
				{
					if((face->glyph->bitmap.buffer[Row * face->glyph->bitmap.pitch + Pitch]) & 1<<Bit)
					{
						backbuffer[(x+PosX) + ((y+PosY)*var_screeninfo.xres)] = Attribute & 15;

						if(Attribute & 1<<9) backbuffer[(x+PosX) + ((y+PosY+1)*var_screeninfo.xres)] = Attribute & 15;
					}
					else
					{
						if(transpmode)
						{
							Attribute &= 0xFF0F;
							Attribute |= transp<<4;
						}

						backbuffer[(x+PosX) + ((y+PosY)*var_screeninfo.xres)] = Attribute>>4 & 15;

						if(Attribute & 1<<9) backbuffer[(x+PosX) + ((y+PosY+1)*var_screeninfo.xres)] = Attribute>>4 & 15;
					}

					x++;
				}
			}

			x = 0;
			y++;

			if(Attribute & 1<<9) y++;
		}

	PosX += fontwidth;
}

/******************************************************************************
 * RenderPageNotFound                                                         *
 ******************************************************************************/

void RenderPageNotFound()
{
	int byte;
	int fbcolor, timecolor;
	char message_1[] = "אבבבבבבבבבבבבבבבבבבבבבבבבבבבבבבבבבבבבגט";
	char message_2[] = "ד Seite ??? nicht vorhanden: warte...הי";
	char message_3[] = "וזזזזזזזזזזזזזזזזזזזזזזזזזזזזזזזזזזזזחי";
	char message_4[] = "כלללללללללללללללללללללללללללללללללללללך";

	//set colors

		if(screenmode == 1)
		{
			fbcolor   = black;
			timecolor = black;
		}
		else
		{

			fbcolor   = transp;
			timecolor = transp<<4 | transp;
		}

	//clear framebuffer

		memset(lfb, fbcolor, var_screeninfo.xres * var_screeninfo.yres);

	//hide timestring

		page_atrb[32] = timecolor;

	//set pagenumber

		message_2[ 8] = (page >> 8) | '0';
		message_2[ 9] = (page & 0x0F0)>>4 | '0';
		message_2[10] = (page & 0x00F) | '0';

	//render infobar

		PosX = StartX + fontwidth-3;
		PosY = StartY + fixfontheight*18;
		for(byte = 0; byte < 38; byte++)
		{
			RenderCharFB(message_1[byte], menu1<<4 | menu2);
		}
		RenderCharFB(message_1[38], fbcolor<<4 | menu2);

		PosX = StartX + fontwidth-3;
		PosY = StartY + fixfontheight*19;
		RenderCharFB(message_2[0], menu1<<4 | menu2);
		for(byte = 1; byte < 37; byte++)
		{
			RenderCharFB(message_2[byte], menu1<<4 | white);
		}
		RenderCharFB(message_2[37], menu1<<4 | menu2);
		RenderCharFB(message_2[38], fbcolor<<4 | menu2);

		PosX = StartX + fontwidth-3;
		PosY = StartY + fixfontheight*20;
		for(byte = 0; byte < 38; byte++)
		{
			RenderCharFB(message_3[byte], menu1<<4 | menu2);
		}
		RenderCharFB(message_3[38], fbcolor<<4 | menu2);

		PosX = StartX + fontwidth-3;
		PosY = StartY + fixfontheight*21;
		for(byte = 0; byte < 39; byte++)
		{
			RenderCharFB(message_4[byte], fbcolor<<4 | menu2);
		}
}

/******************************************************************************
 * RenderPage                                                                 *
 ******************************************************************************/

void RenderPage()
{
	int row, col, byte;

	if(!tvmode && pageupdate && current_page != page && inputcounter == 2)
	{
		//reset update flag

			pageupdate = 0;

		//decode page

			if(subpagetable[page] != 0xFF) DecodePage();
			else
			{
				RenderPageNotFound();
				return;
			}

		//render page

			PosY = StartY;

			for(row = 0; row < 24; row++)
			{
				PosX = StartX;

				for(col = 0; col < 40; col++)
				{
					RenderCharBB(page_char[row*40 + col], page_atrb[row*40 + col]);
				}

				PosY += fixfontheight;
			}

		//update framebuffer

			memcpy(lfb, &backbuffer, sizeof(backbuffer));
	}
	else if(!tvmode)
	{
		//update timestring

			PosX = StartX + 32*fontwidth;
			PosY = StartY;

			for(byte = 0; byte < 8; byte++)
			{
				RenderCharFB(timestring[byte], page_atrb[32]);
			}
	}
}

/******************************************************************************
 * DecodePage                                                                 *
 ******************************************************************************/

void DecodePage()
{
	int row, col;
	int boxed = 0, clear, loop;
	int foreground, background, doubleheight, charset;

	//copy page to decode buffer

		if(zap_subpage_manual) memcpy(&page_char, cachetable[page][subpage], PAGESIZE);
		else				   memcpy(&page_char, cachetable[page][subpagetable[page]], PAGESIZE);

	//update timestring

		memcpy(&page_char[32], &timestring, 8);

	//check for newsflash & subtitle

		if(dehamming[page_char[11-6]] & 12 && !screenmode) boxed = 1;

	//modify header

		if(boxed) memset(&page_char, ' ', 40);
		else     memcpy(&page_char, " TuxTxt ", 8);

	//decode

		for(row = 0; row < 24; row++)
		{
			//start-of-row default conditions

				foreground = white;
				background = black;
				doubleheight = 0;
				charset = 0;

				if(boxed == 1 && memchr(&page_char[row*40], start_box, 40) == 0)
				{
					foreground = transp;
					background = transp;
				}

			for(col = 0; col < 40; col++)
			{
				if(page_char[row*40 + col] < ' ')
				{
					switch(page_char[row*40 + col])
					{
						case alpha_black:		page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												foreground = black;
												charset = 0;
												break;

						case alpha_red:			page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												foreground = red;
												charset = 0;
												break;

						case alpha_green:		page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												foreground = green;
												charset = 0;
												break;

						case alpha_yellow:		page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												foreground = yellow;
												charset = 0;
												break;

						case alpha_blue:		page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												foreground = blue;
												charset = 0;
												break;

						case alpha_magenta:		page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												foreground = magenta;
												charset = 0;
												break;

						case alpha_cyan:		page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												foreground = cyan;
												charset = 0;
												break;

						case alpha_white:		page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												foreground = white;
												charset = 0;
												break;

						case flash:				page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												//todo
												break;

						case steady:			//todo
												page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												break;

						case end_box:			page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												if(boxed)
												{
													foreground = transp;
													background = transp;
												}
												break;

						case start_box:			page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												if(boxed) for(clear = 0; clear < col; clear++) page_atrb[row*40 + clear] = transp<<4 | transp;
												break;

						case normal_size:		doubleheight = 0;
												page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												break;

						case double_height:		page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												doubleheight = 1;
												break;

						case double_width:		page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												//todo
												break;

						case double_size:		page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												//todo
												break;

						case mosaic_black:		page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												foreground = black;
												charset = 1;
												break;

						case mosaic_red:		page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												foreground = red;
												charset = 1;
												break;

						case mosaic_green:		page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												foreground = green;
												charset = 1;
												break;

						case mosaic_yellow:		page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												foreground = yellow;
												charset = 1;
												break;

						case mosaic_blue:		page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												foreground = blue;
												charset = 1;
												break;

						case mosaic_magenta:	page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												foreground = magenta;
												charset = 1;
												break;

						case mosaic_cyan:		page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												foreground = cyan;
												charset = 1;
												break;

						case mosaic_white:		page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												foreground = white;
												charset = 1;
												break;

						case conceal:			//todo
												page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												break;

						case contiguous_mosaic:	//todo
												page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												break;

						case separated_mosaic:	//todo
												page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												break;

						case esc:				page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												//todo
												break;

						case black_background:	background = black;
												page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												break;

						case new_background:	background = foreground;
												page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												break;

						case hold_mosaic:		//todo
												page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												break;

						case release_mosaic:	page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												//todo
												break;
					}

					//show spacing attribute as space

						page_char[row*40 + col] = ' ';
				}
				else
				{
					page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;

					//skip doubleheight chars in lower line

						if(doubleheight) page_char[(row+1)*40 + col] = 0xFF;
				}
			}

			//copy attribute to lower line if doubleheight

				if(memchr(&page_char[(row+1)*40], 0xFF, 40) != 0)
				{
					for(loop = 0; loop < 40; loop++)
					{
						page_atrb[(row+1)*40 + loop] = (page_atrb[row*40 + loop] & 0xF0) | (page_atrb[row*40 + loop] & 0xF0)>>4;
					}

					row++;
				}
		}
}

/******************************************************************************
 * GetRCCode                                                                  *
 ******************************************************************************/

int GetRCCode()
{
	static unsigned short LastKey = -1;

	//get code

		if(read(rc, &RCCode, 2) == 2)
		{
			if(RCCode != LastKey)
			{
				LastKey = RCCode;

				//translation required?

					if((RCCode & 0xFF00) == 0x5C00)
					{
						switch(RCCode)
						{
							case RC1_UP:	RCCode = RC_UP;
											break;

							case RC1_DOWN:	RCCode = RC_DOWN;
											break;

							case RC1_RIGHT:	RCCode = RC_RIGHT;
											break;

							case RC1_LEFT:	RCCode = RC_LEFT;
											break;

							case RC1_PLUS:	RCCode = RC_PLUS;
											break;

							case RC1_MINUS:	RCCode = RC_MINUS;
											break;

							case RC1_0:		RCCode = RC_0;
											break;

							case RC1_1:		RCCode = RC_1;
											break;

							case RC1_2:		RCCode = RC_2;
											break;

							case RC1_3:		RCCode = RC_3;
											break;

							case RC1_4:		RCCode = RC_4;
											break;

							case RC1_5:		RCCode = RC_5;
											break;

							case RC1_6:		RCCode = RC_6;
											break;

							case RC1_7:		RCCode = RC_7;
											break;

							case RC1_8:		RCCode = RC_8;
											break;

							case RC1_9:		RCCode = RC_9;
											break;

							case RC1_OK:	RCCode = RC_OK;
											break;

							case RC1_DBOX:	RCCode = RC_DBOX;
											break;

							case RC1_HOME:	RCCode = RC_HOME;
											break;

							case RC1_MUTE:	RCCode = RC_MUTE;
						}
					}
					else
					{
						RCCode &= 0x003F;
					}
			}
			else
			{
				RCCode = -1;
			}

			//command received

				return 1;
		}
		else
		{
			//no command received

				usleep(1000000/100);
				return 0;
		}
}

/******************************************************************************
 * CacheThread                                                                *
 ******************************************************************************/

void *CacheThread(void *arg)
{
	unsigned char pes_packet[184];
	unsigned char vtxt_row[1+45];
	int line, byte, bit;
	int b1, b2, b3, b4;
	int packet_number;

	while(1)
	{
		//check stopsignal

			pthread_testcancel();

		//read packet

			read(dmx, &pes_packet, sizeof(pes_packet));

		//analyze it

			for(line = 0; line < 4; line++)
			{
				if((pes_packet[line*0x2E] == 0x02 || pes_packet[line*0x2E] == 0x03) && (pes_packet[line*0x2E + 1] == 0x2C))
				{
					//clear rowbuffer

						memset(&vtxt_row, 0, sizeof(vtxt_row));

					//convert row from lsb to msb (begin with magazin number)

						for(byte = 4; byte <= 45; byte++)
						{
							for(bit = 0; bit <= 7; bit++)
							{
								if(pes_packet[line*0x2E + byte] & 1<<bit)
								{
									vtxt_row[byte] |= 128>>bit;
								}
							}
						}

					//get packet number

						b1 = dehamming[vtxt_row[4]] & 8;
						b2 = dehamming[vtxt_row[5]];

						if(b1 == 0xFF || b2 == 0xFF)
						{
							printf("TuxTxt <Biterror in Packet>\n");
							goto SkipPacket;
						}

						packet_number = b1>>3 | b2<<1;

					//analyze row

						if(packet_number == 0)
						{
							//remove parity bit from data bytes (dirty, i know...)

								for(byte = 14; byte <= 45; byte++)
								{
									vtxt_row[byte] &= 127;
								}

							//get pagenumber

								b1 = dehamming[vtxt_row[4]] & 7;
								b2 = dehamming[vtxt_row[7]];
								b3 = dehamming[vtxt_row[6]];

								if(b1 == 0xFF || b2 == 0xFF || b3 == 0xFF)
								{
									current_page = -1;
									printf("TuxTxt <Biterror in Page>\n");
									goto SkipPacket;
								}

								if(b2 > 9 || b3 > 9)
								{
									current_page = -1;
									goto SkipPacket;
								}
								else
								{
									current_page = b1<<8 | b2<<4 | b3;
								}

								if(current_page < 0x100)
								{
									current_page += 0x800;
								}

							//get subpagenumber

								b1 = dehamming[vtxt_row[11]] & 3;
								b2 = dehamming[vtxt_row[10]];
								b3 = dehamming[vtxt_row[9]] & 7;
								b4 = dehamming[vtxt_row[8]];

								if(b1 == 0xFF || b2 == 0xFF || b3 == 0xFF || b4 == 0xFF)
								{
									current_subpage = -1;
									printf("TuxTxt <Biterror in SubPage>\n");
									goto SkipPacket;
								}

								if(b1 != 0 || b2 != 0 || b3 > 7 || b4 > 9)
								{
									current_subpage = -1;
									goto SkipPacket;
								}
								else
								{
									current_subpage = b1<<12 | b2<<8 | b3<<4 | b4;
								}

							//check cachetable and allocate memory if needed

								if(cachetable[current_page][current_subpage] == 0)
								{
//									printf("TuxTxt <Adding Page %.3X-%.2X>\n", current_page, current_subpage);
									cachetable[current_page][current_subpage] = malloc(PAGESIZE);
									memset(cachetable[current_page][current_subpage], ' ', PAGESIZE);
								}

							//store current subpage for this page

								subpagetable[current_page] = current_subpage;

							//copy timestring

								memcpy(&timestring, &vtxt_row[38], 8);

							//set update flag

								if(current_page == page)
								{
									pageupdate = 1;

									if(!zap_subpage_manual) subpage = current_subpage;
								}

							//check controlbits

								if(dehamming[vtxt_row[9]] & 8)	//C4 -> erase page
								{
//									printf("TuxTxt <Erase Page => %.3X-%.2X>\n", current_page, current_subpage);
									memset(cachetable[current_page][current_subpage], ' ', PAGESIZE);
								}

//								if(dehamming[vtxt_row[12]] & 2)	//C7 -> suppress header
//								{
//									printf("TuxTxt <Suppress Header => %.3X-%.2X>\n", current_page, current_subpage);
//									goto SkipPacket;
//								}

//								if(dehamming[vtxt_row[12]] & 8)	//C10 -> inhibit display
//								{
//									printf("TuxTxt <Inhibit Display => %.3X-%.2X>\n", current_page, current_subpage);
//									current_page = -1;
//								}
						}
						else if(packet_number < 24)
						{
							//remove parity bit from data bytes (dirty, i know...)

								for(byte = 6; byte <= 45; byte++)
								{
									vtxt_row[byte] &= 127;
								}
						}

					//copy row to pagebuffer

						if(current_page != -1 && current_subpage != -1 && packet_number < 24)
						{
							memcpy(cachetable[current_page][current_subpage] + packet_number*40, &vtxt_row[6], 40);
						}
SkipPacket:;
				}
			}
	}
}
