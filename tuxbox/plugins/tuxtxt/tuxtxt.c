/******************************************************************************
 *                    <<< TuxTxt - Videotext via d-box >>>                    *
 *                                                                            *
 *                        (c) Thomas "LazyT" Loewe '02                        *
 *----------------------------------------------------------------------------*
 * History                                                                    *
 *                                                                            *
 *    V1.6 (LazyT): added transparency mode                                   *
 *    V1.5 (LazyT): added newsflash/subtitle support                          *
 *    V1.4 (LazyT): skip not received pages on +/-10, some mods               *
 *    V1.3 (LazyT): segfault fixed                                            *
 *    V1.2 (trh)  : made it work under enigma                                 *
 *    V1.1 (LazyT): added tuxtxt to cvs                                       *
 ******************************************************************************/

#include "tuxtxt.h"

/******************************************************************************
 * entry function                                                             *
 ******************************************************************************/

void plugin_exec(PluginParam *par)
{
	//show versioninfo

		printf("\nTuxTxt 1.6 - Coypright (c) Thomas \"LazyT\" Loewe and the TuxBox Team\n\n");

	//get params

		for(; par; par = par->next)
		{
			if(!strcmp(par->id, P_ID_FBUFFER))
			{
				fb = atoi(par->val);
			}
			else if (!strcmp(par->id, P_ID_RCINPUT))
			{
				rc = atoi(par->val);
			}
			else if (!strcmp(par->id, P_ID_VTXTPID))
			{
				vtxtpid = atoi(par->val);
			}
			else if (!strcmp(par->id, P_ID_OFF_X))
			{
				sx = atoi(par->val);
			}
			else if (!strcmp(par->id, P_ID_OFF_Y))
			{
				sy = atoi(par->val);
			}
			else if (!strcmp(par->id, P_ID_END_X))
			{
				ex = atoi(par->val);
			}
			else if (!strcmp(par->id, P_ID_END_Y))
			{
				ey = atoi(par->val);
			}
		}

		if(fb == -1 || rc == -1 || vtxtpid == -1 || sx == -1 || sy == -1 || ex == -1 || ey == -1)
		{
			printf("invalid params: fb = %d, rc = %d, vtxtpid = %X, sx = %d, ex = %d, sy = %d, ey = %d\nTuxtxt aborted!\n\n", fb, rc, vtxtpid, sx, ex, sy, ey);
			return;
		}

	//init

		if(Init() == 0)
		{
			printf("TuxTxt aborted!\n\n");
			return;
		}

	//main loop

		do
		{
			if(GetRCCode() == 1)
			{
				switch(RCCode)
				{
					case RC_0:	//direct page number or page 100

									if(PageInputCount == 2)
									{
										PageInput = 0;
										PageInputCount = 2;
										Page = 0x100;
										update = 1;
									}
									else
									{
										RenderPageNumber('0');
										PageInputCount--;
									}
									break;

					case RC_1:	//direct page number

									RenderPageNumber('1');
									PageInput |= 1 << PageInputCount*4;
									PageInputCount--;
									break;

					case RC_2:	//direct page number

									RenderPageNumber('2');
									PageInput |= 2 << PageInputCount*4;
									PageInputCount--;
									break;

					case RC_3:	//direct page number

									RenderPageNumber('3');
									PageInput |= 3 << PageInputCount*4;
									PageInputCount--;
									break;

					case RC_4:	//direct page number

									RenderPageNumber('4');
									PageInput |= 4 << PageInputCount*4;
									PageInputCount--;
									break;

					case RC_5:	//direct page number

									RenderPageNumber('5');
									PageInput |= 5 << PageInputCount*4;
									PageInputCount--;
									break;

					case RC_6:	//direct page number

									RenderPageNumber('6');
									PageInput |= 6 << PageInputCount*4;
									PageInputCount--;
									break;

					case RC_7:	//direct page number

									RenderPageNumber('7');
									PageInput |= 7 << PageInputCount*4;
									PageInputCount--;
									break;

					case RC_8:	//direct page number

									RenderPageNumber('8');
									PageInput |= 8 << PageInputCount*4;
									PageInputCount--;
									break;

					case RC_9:	//direct page number

									if(PageInputCount == 2)
									{
									  break;
									}
									else
									{
										RenderPageNumber('9');
										PageInput |= 9 << PageInputCount*4;
									}

									PageInputCount--;
									break;

					case RC_UP:	//page +1

									PageInput = 0;
									PageInputCount = 2;

									if(Page < 0x899)
									{
										Page++;

										if((Page & 0x00F) > 0x09)
										{
											Page += 0x06;
										}

										if((Page & 0x0F0) > 0x90)
										{
											Page += 0x60;
										}
									}
									else
									{
										Page = 0x100;
									}

								//skip not received pages

									if(pagetable[Page] == 0)
									{
										for( ; Page < 0x900; Page++)
										{
											if(Page > 0x899)
											{
												Page = 0x100;
											}

											if(pagetable[Page] == 1)
											{
												break;
											}
										}
									}

								//feedback

									PosX = StartX + 8*fontwidth;
									PosY = StartY;
									RenderCharFB('-', white);
									RenderCharFB('-', white);
									RenderCharFB('>', white);

									update = 1;
									break;

					case RC_DOWN://page -1

									PageInput = 0;
									PageInputCount = 2;

									if(Page > 0x100)
									{
										Page--;

										if((Page & 0x00F) > 0x09)
										{
											Page -= 0x06;
										}

										if((Page & 0x0F0) > 0x90)
										{
											Page -= 0x60;
										}
									}
									else
									{
										Page = 0x899;
									}

								//skip not received pages

									if(pagetable[Page] == 0)
									{
										for( ; Page > 0x100; Page--)
										{
											if(pagetable[Page] == 1)
											{
												break;
											}
										}
									}

								//feedback

									PosX = StartX + 8*fontwidth;
									PosY = StartY;
									RenderCharFB('<', white);
									RenderCharFB('-', white);
									RenderCharFB('-',white);

									update = 1;
									break;

					case RC_RIGHT://page +10

									PageInput = 0;
									PageInputCount = 2;
									Page &= 0xFF0;

									if((Page & 0x0F0) == 0x90)
									{
										Page += 0x70;
									}
									else
									{
										Page += 0x10;
									}

									if(Page == 0x900)
									{
										Page = 0x100;
									}

								//skip not received pages

									if(pagetable[Page] == 0)
									{
										for( ; Page < 0x900; Page+=0x10)
										{
											if(Page > 0x899)
											{
												Page = 0x100;
											}

											if(pagetable[Page] == 1)
											{
												break;
											}
										}
									}

								//feedback

									PosX = StartX + 8*fontwidth;
									PosY = StartY;
									RenderCharFB('-', white);
									RenderCharFB('>', white);
									RenderCharFB('>', white);

									update = 1;
									break;

					case RC_LEFT://page -10

									PageInput = 0;
									PageInputCount = 2;

									if((Page & 0x00F) == 0)
									{
										Page -= 0x10;
									}
									else
									{
										Page &= 0xFF0;
									}

									if((Page & 0x0F0) == 0xF0)
									{
										Page &= 0xFF0;
										Page -= 0x60;
									}

									if(Page == 0x090)
									{
										Page = 0x890;
									}

								//skip not received pages

									if(pagetable[Page] == 0)
									{
										for( ; Page > 0x100; Page-=0x10)
										{
											if(pagetable[Page] == 1)
											{
												break;
											}
										}
									}

								//feedback

									PosX = StartX + 8*fontwidth;
									PosY = StartY;
									RenderCharFB('<', white);
									RenderCharFB('<', white);
									RenderCharFB('-', white);

									update = 1;
									break;

					case RC_OK:	//set display mode

								visible++;

								switch(visible)
								{
									case 3:	//same as hidden but reset modecounter

											 visible = 0;

									case 0:	//hidden

											ClearFramebuffer(transp);

											PosX = StartX + fontwidth*3;
											PosY = StartY + fontheight*2;
											RenderCharFB(249, transp<<4 | yellow);
											RenderCharFB(250, transp<<4 | yellow);
											RenderCharFB(251, transp<<4 | yellow);
											PosX = StartX + fontwidth*3;
											PosY = StartY + fontheight*3;
											RenderCharFB(252, transp<<4 | yellow);
											RenderCharFB(253, transp<<4 | yellow);
											RenderCharFB(254, transp<<4 | yellow);
											break;

									case 1:	//normal
									case 2:	//transp
											update = 1;
											break;
								}

								show_string = 1;
								break;
				}

				if(PageInputCount < 0)
				{
					Page = PageInput;
					PageInput = 0;
					PageInputCount = 2;
					update = 1;
					show_string = 1;
				}
			}

			RenderPage();
		}
		while(RCCode != RC_HOME);

	//cleanup

		printf("stopping decode-thread...");
		if(pthread_cancel(thread_id1) != 0)
		{
			printf("cancel failed!\n\n");
			return;
		}
		if(pthread_join(thread_id1, &thread_result1) != 0)
		{
			printf("join failed!\n\n");
			return;
		}
		printf("done\n\n");

		ioctl(dmx, DMX_STOP);

		close(dmx);

		FT_Done_FreeType(library);

		munmap(lfb, fix_screeninfo.smem_len);

		free(pagebuffer);
}

/******************************************************************************
 * init                                                                       *
 ******************************************************************************/

int Init()
{
	struct dmxPesFilterParams dmx_flt;

	//calculate screen position

		if((ex-sx) - 40*fontwidth >= 0)
		{
			StartX = sx + (((ex-sx) - 40*fontwidth) / 2);
		}
		else
		{
			StartX = ex - 40*fontwidth;
		}

		if((ey-sy) - 24*fontheight >= 0)
		{
			StartY = sy + (((ey-sy) - 24*fontheight) / 2);
		}
		else
		{
			StartY = sy + fontheight/2;
		}

	//open demuxer

		if((dmx = open("/dev/ost/demux0", O_RDWR)) == -1)
		{
			perror("open demuxer failed");
			return 0;
		}

	//setup rc

		fcntl(rc, F_SETFL, O_NONBLOCK);
		ioctl(rc, RC_IOCTL_BCODES, 1);

	//allocate pagebuffer & init all buffers

		if((pagebuffer = malloc(0x899 * 40*24)) == 0)
		{
			perror("allocate pagebuffers failed");
			return 0;
		}

		memset(pagebuffer, ' ', 0x899 * 40*24);
		memset(pagetable, 0, 0x899);
		memset(page_char, ' ', sizeof(page_char));
		memset(page_atrb, transp<<4 | transp, sizeof(page_atrb));

	//init fontlibrary

		if(FT_Init_FreeType(&library) != 0)
		{
			printf("init fontlibrary failed!\n");
			return 0;
		}

	//load font

		if(FT_New_Face(library, FONTDIR "/tuxtxt.fon", 0, &face) != 0)
		{
			printf("loading font \"tuxtxt.fon\" failed!\n");
			return 0;
		}

	//set fontsize

		if(FT_Set_Pixel_Sizes(face, fontwidth, fontheight) != 0)
		{
			printf("setting fontsize %dx%d failed!\n", fontwidth, fontheight);
			return 0;
		}

	//get fixed screeninfo

		if (ioctl(fb, FBIOGET_FSCREENINFO, &fix_screeninfo) == -1)
		{
			perror("getting fixed screeninfo failed");
			return 0;
		}

	//get variable screeninfo

		if (ioctl(fb, FBIOGET_VSCREENINFO, &var_screeninfo) == -1)
		{
			perror("getting variable screeninfo failed");
			return 0;
		}

	//set new colormap

		if (ioctl(fb, FBIOPUTCMAP, &colormap) == -1)
		{
			perror("setting palette failed");
			return 0;
		}

	//map framebuffer into memory
    
		lfb = (unsigned char*)mmap(0, fix_screeninfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);

		if(!lfb)
		{
			perror("mapping framebuffer failed");
			return 0;
		}

	//set filter

		dmx_flt.pid		= vtxtpid;
		dmx_flt.input	= DMX_IN_FRONTEND;
		dmx_flt.output	= DMX_OUT_TAP;
		dmx_flt.pesType	= DMX_PES_OTHER;
		dmx_flt.flags	= DMX_IMMEDIATE_START;

		if(ioctl(dmx, DMX_SET_PES_FILTER, &dmx_flt) == -1)
		{
			perror("set filter failed");
			return 0;
		}

	//create decode-thread

		printf("starting decode-thread...");
		if(pthread_create(&thread_id1, NULL, DecodePacket, NULL) != 0)
		{
			perror("could not create decode-thread");
			return 0;
		}
		printf("done\n");

	//init successfully

		return 1;
}

/******************************************************************************
 * clear framebuffer                                                          *
 ******************************************************************************/

void ClearFramebuffer(int color)
{
	memset(lfb, color, var_screeninfo.xres * var_screeninfo.yres);
}

/******************************************************************************
 * render char to backbuffer                                                  *
 ******************************************************************************/

void RenderChar(int Char, int Attribute)
{
	int Row, Pitch, Bit, x = 0, y = 0;

	//skip doubleheight char

		if(Char == 0xFF)
		{
			PosX += fontwidth;
			return;
		}

	//load char

		if(FT_Load_Char(face, Char + (((Attribute>>8) & 3)*(128-32)), FT_LOAD_RENDER | FT_LOAD_MONOCHROME) != 0)
		{
			printf("load char %.2X failed!\n", Char);
			return;
		}

	//render char

		for(Row = 0; Row < face->glyph->bitmap.rows; Row++)
		{
			for(Pitch = 0; Pitch < face->glyph->bitmap.pitch; Pitch++)
			{
				for(Bit = 7; Bit >= 0; Bit--)
				{
					if((face->glyph->bitmap.buffer[Row * face->glyph->bitmap.pitch + Pitch]) & 1<<Bit)
					{
						backbuffer[(x+PosX) + ((y+PosY)*var_screeninfo.xres)] = Attribute & 15;

						if((Attribute>>10) & 1)
						{
							backbuffer[(x+PosX) + ((y+PosY+1)*var_screeninfo.xres)] = Attribute & 15;
						}
					}
					else
					{
						if(visible == 2)
						{
							Attribute &= 0xFF0F;
							Attribute |= transp<<4;
						}

						backbuffer[(x+PosX) + ((y+PosY)*var_screeninfo.xres)] = (Attribute>>4) & 15;

						if((Attribute>>10) & 1)
						{
							backbuffer[(x+PosX) + ((y+PosY+1)*var_screeninfo.xres)] = (Attribute>>4) & 15;
						}
					}

					x++;
				}
			}

			x = 0;
			y++;

			if((Attribute>>10) & 1)
			{
				y++;
			}
		}

	PosX += fontwidth;
}

/******************************************************************************
 * render char to framebuffer                                                 *
 ******************************************************************************/

void RenderCharFB(int Char, int Attribute)
{
	int Row, Pitch, Bit, x = 0, y = 0;

	//skip doubleheight char

		if(Char == 0xFF)
		{
			PosX += fontwidth;
			return;
		}

	//load char

		if(FT_Load_Char(face, Char + (((Attribute>>8) & 3)*(128-32)), FT_LOAD_RENDER | FT_LOAD_MONOCHROME) != 0)
		{
			printf("load char %.2X failed!\n", Char);
			return;
		}

	//render char

		for(Row = 0; Row < face->glyph->bitmap.rows; Row++)
		{
			for(Pitch = 0; Pitch < face->glyph->bitmap.pitch; Pitch++)
			{
				for(Bit = 7; Bit >= 0; Bit--)
				{
					if((face->glyph->bitmap.buffer[Row * face->glyph->bitmap.pitch + Pitch]) & 1<<Bit)
					{
						*(lfb + (x+PosX) + ((y+PosY)*var_screeninfo.xres)) = Attribute & 15;

						if((Attribute>>10) & 1)
						{
							*(lfb + (x+PosX) + ((y+PosY+1)*var_screeninfo.xres)) = Attribute & 15;
						}
					}
					else
					{
						if(visible == 2)
						{
							Attribute &= 0xFF0F;
							Attribute |= transp<<4;
						}

						*(lfb + (x+PosX) + ((y+PosY)*var_screeninfo.xres)) = (Attribute>>4) & 15;

						if((Attribute>>15) & 1)
						{
							*(lfb + (x+PosX) + ((y+PosY+1)*var_screeninfo.xres)) = (Attribute>>4) & 15;
						}
					}

					x++;
				}
			}

			x = 0;
			y++;

			if((Attribute>>10) & 1)
			{
				y++;
			}
		}

	PosX += fontwidth;
}

/******************************************************************************
 * render pagenumber to framebuffer                                           *
 ******************************************************************************/

void RenderPageNumber(int Char)
{
	PosX = StartX;
	PosY = StartY;

	//render page number to backbuffer

		switch(PageInputCount)
		{
			case 2:
					PosX += 16*8;
					RenderCharFB(Char, white);
					RenderCharFB('-',  white);
					RenderCharFB('-',  white);
					break;

			case 1:
					PosX += 16*9;
					RenderCharFB(Char, white);
					break;

			case 0:
					PosX += 16*10;
					RenderCharFB(Char, white);
					break;
		}
}

/******************************************************************************
 * render string to framebuffer                                               *
 ******************************************************************************/

void RenderString()
{
	char message_1[] = "7#####################################k";
	char message_2[] = "5 Seite ??? nicht vorhanden: warte... j";
	char message_3[] = "upppppppppppppppppppppppppppppppppppppz";

	int x;

	show_string = 0;

	//clear framebuffer

		ClearFramebuffer(transp);

	//set pagenumber

		message_2[ 8] = (Page >> 8) | '0';
		message_2[ 9] = (Page & 0x0F0)>>4 | '0';
		message_2[10] = (Page & 0x00F) | '0';

	//render strings to framebuffer

		PosX = StartX + fontwidth/2;
		PosY = StartY + fontheight*11;
		for(x = 0; x < 39; x++)
		{
			RenderCharFB(message_1[x], 1<<8 | red<<4 | white);
		}

		PosX = StartX + fontwidth/2;
		PosY = StartY + fontheight*12;
		RenderCharFB(message_2[0], 1<<8 | red<<4 | white);
		for(x = 1; x < 38; x++)
		{
			RenderCharFB(message_2[x], red<<4 | white);
		}
		RenderCharFB(message_2[38], 1<<8 | red<<4 | white);

		PosX = StartX + fontwidth/2;
		PosY = StartY + fontheight*13;
		for(x = 0; x < 39; x++)
		{
			RenderCharFB(message_3[x], 1<<8 | red<<4 | white);
		}
}

/******************************************************************************
 * render page to framebuffer                                                 *
 ******************************************************************************/

void RenderPage()
{
	int row, col;

	if(visible)
	{	
		if(update && Page != current_page)
		{
			if(pagetable[Page] == 1)
			{
				update = 0;
				show_string = 1;
				PosY = StartY;

				//build page

					DecodePage();

				//render to backbuffer

					for(row = 0; row < 24; row++)
					{
						PosX = StartX;

						for(col = 0; col < 40; col++)
						{
							RenderChar(page_char[row*40 + col], page_atrb[row*40 + col]);
						}

						PosY += fontheight;
					}

				//update framebuffer

					memcpy(lfb, &backbuffer, sizeof(backbuffer));
			}
			else
			{
				if(show_string)
				{
					RenderString();
				}
			}
		}
		else
		{
			PosX = StartX + 32*fontwidth;
			PosY = StartY;

			//copy time

				memcpy(&page_char[32], &timestring[0], 8);

			//render time

				for(col = 32; col < 40; col++)
				{
					RenderCharFB(page_char[col], page_atrb[col]);
				}
		}
	}
	else
	{
		if(update == 1)
		{
			PosX = StartX + fontwidth*3;
			PosY = StartY + fontheight*2;
			RenderCharFB(243, transp<<4 | yellow);
			RenderCharFB(244, transp<<4 | yellow);
			RenderCharFB(245, transp<<4 | yellow);
			PosX = StartX + fontwidth*3;
			PosY = StartY + fontheight*3;
			RenderCharFB(246, transp<<4 | yellow);
			RenderCharFB(247, transp<<4 | yellow);
			RenderCharFB(248, transp<<4 | yellow);
		}
	}

	//pause

		usleep(1000000/100);
}

/******************************************************************************
 * decode a teletext page                                                     *
 ******************************************************************************/

void DecodePage()
{
	int col, row, loop;
	int foreground, background, charset, doubleheight, boxed, page_is_boxed = 0;

	//copy page to decode buffer, modify header & update time

		memcpy(&page_char[0], &pagebuffer[Page*40*24], sizeof(page_char));
		memcpy(&page_char[0], " TuxTxt ", 8);
		memcpy(&page_char[32], &timestring[0], 8);

	//decode page

	for(row = 0; row < 24; row++)
	{
		//start-of-row default conditions

			foreground = white;
			background = black;
			doubleheight = 0;
			charset = 0;
			boxed = 0;
			//steady ?
			//release mosaic ?

		for(col = 0; col < 40; col++)
		{
			if(page_char[row*40 + col] < ' ')
			{
				switch(page_char[row*40 + col])
				{
					//set-at codes

						case steady:			//todo
												page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												break;

						case normal_size:		doubleheight = 0;
												page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												break;

						case conceal:			//todo
												page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												break;

						case contiguous_mosaic:	//charset = 1;
												page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												break;

						case separated_mosaic:	//charset = 2;
												page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												break;

						case black_background:	background = black;
												page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												break;

						case new_background:	background = foreground;
												page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												break;

						case hold_mosaic:
												page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												break;

					//set-after codes

						case alpha_black:		page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												foreground = black;
												charset = 0;
												break;

						case alpha_red:			page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												foreground = red;
												charset = 0;
												break;

						case alpha_green:		page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												foreground = green;
												charset = 0;
												break;

						case alpha_yellow:		page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												foreground = yellow;
												charset = 0;
												break;

						case alpha_blue:		page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												foreground = blue;
												charset = 0;
												break;

						case alpha_magenta:		page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												foreground = magenta;
												charset = 0;
												break;

						case alpha_cyan:		page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												foreground = cyan;
												charset = 0;
												break;

						case alpha_white:		page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												foreground = white;
												charset = 0;
												break;

						case flash:				page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												//todo
												break;

						case end_box:			page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												boxed = 0;
												break;

						case start_box:			page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;

												if(page_char[row*40 + col +1] == start_box)
												{
													boxed = 1;
													page_is_boxed = 1;
												}

												break;

						case double_height:		page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												doubleheight = 1;
												break;

						case double_width:		page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												//todo
												break;

						case double_size:		page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												//todo
												break;

						case mosaic_black:		page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												foreground = black;
												charset = 1;
												break;

						case mosaic_red:		page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												foreground = red;
												charset = 1;
												break;

						case mosaic_green:		page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												foreground = green;
												charset = 1;
												break;

						case mosaic_yellow:		page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												foreground = yellow;
												charset = 1;
												break;

						case mosaic_blue:		page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												foreground = blue;
												charset = 1;
												break;

						case mosaic_magenta:	page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												foreground = magenta;
												charset = 1;
												break;

						case mosaic_cyan:		page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												foreground = cyan;
												charset = 1;
												break;

						case mosaic_white:		page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												foreground = white;
												charset = 1;
												break;

						case esc:				page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												//todo
												break;

						case release_mosaic:	page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;
												//todo
												break;
				}

				//show spacing attribute as space

					page_char[row*40 + col] = ' ';
			}
			else
			{
				page_atrb[row*40 + col] = boxed<<11 | doubleheight<<10 | charset<<8 | background<<4 | foreground;

				//skip doubleheight in lower line

					if(doubleheight)
					{
						page_char[(row+1)*40 + col] = 255;
					}
			}
		}

		//search line for doubleheight chars

			if(memchr(&page_char[(row+1)*40], 255, 40) != 0)
			{
				for(loop = 0; loop < 40; loop++)
				{
					page_atrb[(row+1)*40 + loop] = (page_atrb[row*40 + loop] & 0xF0) | (page_atrb[row*40 + loop] & 0xF0)>>4;
				}

				row++;
			}
	}

	//search page for boxed chars

		if(page_is_boxed)
		{
			for(loop = 0; loop < 40*24; loop++)
			{
				if(((page_atrb[loop]>>11) & 1) != 1)
				{
					page_atrb[loop] = transp<<4 | transp;
				}
			}
		}
}

/******************************************************************************
 * decode a teletext pes-packet                                               *
 ******************************************************************************/

void *DecodePacket(void *arg)
{
	unsigned char pes_packet[184];
	unsigned char vtxt_line[45];

	int line, byte, bit;
	int packet_number;
	int deham1, deham2, deham3;

	while(1)
	{
		//check for exit

			pthread_testcancel();

		//read pes packet

			read(dmx, &pes_packet, sizeof(pes_packet));

		//analyze the vtxt-packet

			for(line = 0; line < 4; line++)
			{
				if((pes_packet[line*0x2E] == 0x02 || pes_packet[line*0x2E] == 0x03) && (pes_packet[line*0x2E + 1] == 0x2C))
				{
					//clear vtxtline-buffer

						memset(&vtxt_line, 0, sizeof(vtxt_line));

					//convert the vtxtline from lsb to msb (skip everything before magazin & packet)

						for(byte = 3; byte <= sizeof(vtxt_line); byte++)
						{
							for(bit = 0; bit <= 7; bit++)
							{
								if(pes_packet[line*0x2E + 1 + byte] & 1<<bit)
								{
									vtxt_line[byte] |= 128>>bit;
								}
							}
						}

					//decode packet number

						deham1 = (dehamming[vtxt_line[3]] & 8) >> 3;
						deham2 = dehamming[vtxt_line[4]] << 1;

						if(deham1 == 0xFF || deham2 == 0xFF)
						{
							printf("biterror in packet detected - skipping...\n");
							goto SkipPacket;
						}

						packet_number = deham1 | deham2;

					//analyze packet

						if(packet_number == 0)								//page header
						{
							//remove parity bit from data bytes (dirty, i know...)

								for(byte = 13; byte < 45; byte++)
								{
									vtxt_line[byte] &= 127;
								}

							//decode pagenumber

								deham1 = (dehamming[vtxt_line[3]] & 7) << 8;
								deham2 = dehamming[vtxt_line[6]] << 4;
								deham3 = dehamming[vtxt_line[5]];

								if(deham1 == 0xFF || deham2 == 0xFF || deham3 == 0xFF)
								{
									printf("biterror in pageheader detected - skipping...\n");
									current_page = -1;
									goto SkipPacket;
								}

								current_page = deham1 | deham2 | deham3;

							//copy time info

								memcpy(&timestring[0], &vtxt_line[37], 8);

							//mod pagenumber, skip hex-pages and set cachetable

								if(current_page < 0x100)
								{
									current_page += 0x800;
								}

								if(((current_page & 0x0F0) > 0x090) || ((current_page & 0x00F) > 0x009))
								{
									current_page = -1;
									goto SkipPacket;
								}

								pagetable[current_page] = 1;

							//get subpage

								//todo

							//get controlbits

								//todo

							//set update flag

								if(current_page == Page && PageInputCount == 2)
								{
									update = 1;
								}
						}
						else if(packet_number < 24 && current_page != -1)	//displayable packet for valid page
						{
							//remove parity bit from data bytes (dirty, i know...)

								for(byte = 5; byte < 45; byte++)
								{
									vtxt_line[byte] &= 127;
								}
						}
						else												//non displayable packet
						{
							goto SkipPacket;
						}

					//copy packet to pagebuffer

						memcpy(&pagebuffer[current_page*40*24 + packet_number*40], &vtxt_line[5], 40);
SkipPacket:;
				}
			}
	}
}

/******************************************************************************
 * check remotecontrol                                                        *
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
							case RC1_0:
											RCCode = RC_0;
											break;

							case RC1_1:
											RCCode = RC_1;
											break;

							case RC1_2:
											RCCode = RC_2;
											break;

							case RC1_3:
											RCCode = RC_3;
											break;

							case RC1_4:
											RCCode = RC_4;
											break;

							case RC1_5:
											RCCode = RC_5;
											break;

							case RC1_6:
											RCCode = RC_6;
											break;

							case RC1_7:
											RCCode = RC_7;
											break;

							case RC1_8:
											RCCode = RC_8;
											break;

							case RC1_9:
											RCCode = RC_9;
											break;

							case RC1_RIGHT:
											RCCode = RC_RIGHT;
											break;

							case RC1_LEFT:
											RCCode = RC_LEFT;
											break;

							case RC1_UP:
											RCCode = RC_UP;
											break;

							case RC1_DOWN:
											RCCode = RC_DOWN;
											break;

							case RC1_HOME:
											RCCode = RC_HOME;
											break;

							case RC1_OK:
											RCCode = RC_OK;
											break;
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

				return 0;
		}
}
