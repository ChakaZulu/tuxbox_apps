/******************************************************************************
 *                 <<< TuxTxt - Videotext SoftwareDecoder >>>                 *
 *                                                                            *
 *                (c) Thomas "LazyT" Loewe '02 (LazyT@gmx.net)                *
 *----------------------------------------------------------------------------*
 * History                                                                    *
 *                                                                            *
 *    V1,31: damned infobar                                                   *
 *    V1.30: change infobar, fix servicescan (segfault on RTL Shop)           *
 *    V1.29: infobar improvements by AlexW                                    *
 *    V1.28: use devfs device names by obi                                    *
 *    V1.27: show cache-status on lcd                                         *
 *    V1.26: ups, forgot this one                                             *
 *    V1.25: fixed colors (color 0 transparent)                               *
 *    V1.2x: some mods by Homar                                               *
 *    V1.22: small zoom-fix                                                   *
 *    V1.21: cleanup                                                          *
 *    V1.20: show servicename instead of pid                                  *
 *    V1.19: added configmenu                                                 *
 *    V1.18: hide navbar in newsflash/subtitle mode, workaround for gtx-pig   *
 *    V1.17: some mods by AlexW                                               *
 *    V1.16: colorkeys fixed                                                  *
 *    V1.15: added colorkeys                                                  *
 *    V1.14: use videoformat-settings                                         *
 *    V1.13: some fixes                                                       *
 *    V1.12: added zoom, removed +/-10                                        *
 *    V1.11: added pagecatching, use 16:9 for text&picture mode               *
 *    V1.10: added conceal/hold mosaics/release mosaics                       *
 *    V1.09: enx fixed, subpage zapping fixed, tvmode reactivated             *
 *    V1.08: zap subpages, text&picture mode, subtitle fixed                  *
 *    V1.07: speedup?, neutrino look ;)                                       *
 *    V1.06: added transparency mode                                          *
 *    V1.05: added newsflash/subtitle support                                 *
 *    V1.04: skip not received pages on +/-10, some mods                      *
 *    V1.03: segfault fixed                                                   *
 *    V1.02: made it work under enigma by trh                                 *
 *    V1.01: added tuxtxt to cvs                                              *
 ******************************************************************************/

#include "tuxtxt.h"

/******************************************************************************
 * plugin_exec                                                                *
 ******************************************************************************/

void plugin_exec(PluginParam *par)
{
	//show versioninfo

		printf("\nTuxTxt 1.31 - Copyright (c) Thomas \"LazyT\" Loewe and the TuxBox-Team\n\n");

	//get params

		vtxtpid = fb = lcd = rc = sx = ex = sy = ey = -1;

		for(; par; par = par->next)
		{
			if(!strcmp(par->id, P_ID_VTXTPID))
			{
				vtxtpid = atoi(par->val);
			}
			else if(!strcmp(par->id, P_ID_FBUFFER))
			{
				fb = atoi(par->val);
			}
			else if(!strcmp(par->id, P_ID_LCD))
			{
				lcd = atoi(par->val);
			}
			else if(!strcmp(par->id, P_ID_RCINPUT))
			{
				rc = atoi(par->val);
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

		if(vtxtpid == -1 || fb == -1 || lcd == -1 || rc == -1 || sx == -1 || ex == -1 || sy == -1 || ey == -1)
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

					case RC_RIGHT:	GetNextSubPage();
									break;

					case RC_LEFT:	GetPrevSubPage();
									break;

					case RC_OK:		PageCatching();
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

					case RC_RED:	Prev100();
									break;

					case RC_GREEN:	Prev10();
									break;

					case RC_YELLOW:	Next10();
									break;

					case RC_BLUE:	Next100();
									break;

					case RC_PLUS:	SwitchZoomMode();
									break;

					case RC_MINUS:	SwitchScreenMode();
									break;

					case RC_MUTE:	SwitchTranspMode();
									break;

					case RC_HELP:	SwitchHintMode();
									break;

					case RC_DBOX:	ConfigMenu();
									break;

					case RC_STANDBY:ShowCacheStatus();
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

		if((dmx = open("/dev/dvb/card0/demux0", O_RDWR)) == -1)
		{
			perror("TuxTxt <open /dev/dvb/card0/demux0>");
			return 0;
		}

	//open pig

		if((pig = open("/dev/dbox/pig0", O_RDWR)) == -1)
		{
			perror("TuxTxt <open /dev/dbox/pig0>");
			return 0;
		}

	//load config

		if((conf = fopen(CONFIGDIR "/tuxtxt/tuxtxt.conf", "rb+")) == 0)
		{
			perror("TuxTxt <fopen tuxtxt.conf>");
			return 0;
		}

		fread(&fnc_mode1, 1, sizeof(fnc_mode1), conf);
		fread(&fnc_mode2, 1, sizeof(fnc_mode2), conf);

	//open avs

		if((avs = open("/dev/dbox/avs0", O_RDWR)) == -1)
		{
			perror("TuxTxt <open /dev/dbox/avs0>");
			return 0;
		}

		ioctl(avs, AVSIOGSCARTPIN8, &fnc_old);

		ioctl(avs, AVSIOSSCARTPIN8, &fnc_mode1);

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

		if((error = FT_New_Face(library, FONTDIR "/tuxtxt.fon", 0, &face)) != 0)
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
		StartY = sy + (((ey-sy) - 25*fixfontheight) / 2);

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

		cached_pages = 0;

		current_page	= -1;
		current_subpage	= -1;

		page	 = 0x100;
		lastpage = 0x100;
		prev_100 = 0x100;
		prev_10  = 0x100;
		next_100 = 0x100;
		next_10  = 0x100;
		subpage	 = 0;
		pageupdate = 0;

		zap_subpage_manual = 0;

		SDT_ready = 0;

	//get all vtxt-pids

		if(GetVideotextPIDs() == 0) return 0;

	//set filter & start demuxer

		if(vtxtpid == 0)
		{
			vtxtpid = pid_table[0].vtxt_pid;
		}

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
	//hide pig

		if(screenmode) avia_pig_hide(pig);

	//restore videoformat

		ioctl(avs, AVSIOSSCARTPIN8, &fnc_old);

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

	//close avs

		close(avs);

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

	//save config

		if(fnc_mode1 != fnc_old1 || fnc_mode2 != fnc_old2)
		{
			rewind(conf);

			fwrite(&fnc_mode1, 1, sizeof(fnc_mode1), conf);
			fwrite(&fnc_mode2, 1, sizeof(fnc_mode2), conf);
		}

		fclose(conf);
}

/******************************************************************************
 * GetVideotextPIDs                                                           *
 ******************************************************************************/

int GetVideotextPIDs()
{
	struct dmxSctFilterParams dmx_flt;
	int pat_scan, pmt_scan, sdt_scan, desc_scan, pid_test, byte, diff;

	unsigned char PAT[1024];
	unsigned char SDT[1024];
	unsigned char PMT[1024];

	//show infobar

		RenderMessage(ShowInfoBar);

	//read PAT to get all PMT's

		memset(&dmx_flt.filter.filter, 0x00, DMX_FILTER_SIZE);
		memset(&dmx_flt.filter.mask,   0x00, DMX_FILTER_SIZE);

		dmx_flt.pid				= 0x0000;
		dmx_flt.flags			= DMX_ONESHOT | DMX_CHECK_CRC | DMX_IMMEDIATE_START;
		dmx_flt.filter.filter[0]= 0x00;
		dmx_flt.filter.mask[0]	= 0xFF;
		dmx_flt.timeout			= 5000;

		if(ioctl(dmx, DMX_SET_FILTER, &dmx_flt) == -1)
		{
			perror("TuxTxt <DMX_SET_FILTER PAT>");
			return 0;
		}

		if(read(dmx, PAT, sizeof(PAT)) == -1)
		{
			perror("TuxTxt <read PAT>");
			return 0;
		}

	//scan each PMT for vtxt-pid

		pids_found = 0;

		for(pat_scan = 0x0A; pat_scan < 0x0A + (((PAT[0x01]<<8 | PAT[0x02]) & 0x0FFF) - 9); pat_scan += 4)
		{
			if(((PAT[pat_scan - 2]<<8) | (PAT[pat_scan - 1])) == 0) continue;

			dmx_flt.pid				= (PAT[pat_scan]<<8 | PAT[pat_scan+1]) & 0x1FFF;
			dmx_flt.flags			= DMX_ONESHOT | DMX_CHECK_CRC | DMX_IMMEDIATE_START;
			dmx_flt.filter.filter[0]= 0x02;
			dmx_flt.filter.mask[0]	= 0xFF;
			dmx_flt.timeout			= 5000;

			if(ioctl(dmx, DMX_SET_FILTER, &dmx_flt) == -1)
			{
				perror("TuxTxt <DMX_SET_FILTER PMT>");
				continue;
			}

			if(read(dmx, PMT, sizeof(PMT)) == -1)
			{
				perror("TuxTxt <read PMT>");
				continue;
			}
			for(pmt_scan = 0x0C + ((PMT[0x0A]<<8 | PMT[0x0B]) & 0x0FFF); pmt_scan < (((PMT[0x01]<<8 | PMT[0x02]) & 0x0FFF) - 7); pmt_scan += 5 + PMT[pmt_scan + 4])
			{
				if(PMT[pmt_scan] == 6)
				{
					for(desc_scan = pmt_scan + 5; desc_scan < pmt_scan + ((PMT[pmt_scan + 3]<<8 | PMT[pmt_scan + 4]) & 0x0FFF) + 5; desc_scan += 2 + PMT[desc_scan + 1])
					{
						if(PMT[desc_scan] == 0x56)
						{
							for(pid_test = 0; pid_test < pids_found; pid_test++)
							{
								if(pid_table[pid_test].vtxt_pid == ((PMT[pmt_scan + 1]<<8 | PMT[pmt_scan + 2]) & 0x1FFF)) goto skip_pid;
							}

							pid_table[pids_found].vtxt_pid	 = (PMT[pmt_scan + 1]<<8 | PMT[pmt_scan + 2]) & 0x1FFF;
							pid_table[pids_found].service_id = PMT[0x03]<<8 | PMT[0x04];
							pids_found++;
skip_pid:;
						}
					}
				}
			}
		}

	//check for videotext

		if(pids_found == 0)
		{
			printf("TuxTxt <no Videotext on TS found>\n");

			RenderMessage(NoServicesFound);

			sleep(3);

			return 0;
		}

	//read SDT to get servicenames

		dmx_flt.pid				= 0x0011;
		dmx_flt.flags			= DMX_ONESHOT | DMX_CHECK_CRC | DMX_IMMEDIATE_START;
		dmx_flt.filter.filter[0]= 0x42;
		dmx_flt.filter.mask[0]	= 0xFF;
		dmx_flt.timeout			= 5000;

		if(ioctl(dmx, DMX_SET_FILTER, &dmx_flt) == -1)
		{
			perror("TuxTxt <DMX_SET_FILTER SDT>");

			RenderMessage(ShowServiceName);

			return 1;
		}

		if(read(dmx, SDT, sizeof(SDT)) == -1)
		{
			perror("TuxTxt <read SDT>");

			RenderMessage(ShowServiceName);

			return 1;
		}

		SDT_ready = 1;

	//scan SDT to get servicenames

		for(sdt_scan = 0x0B; sdt_scan < ((SDT[1]<<8 | SDT[2]) & 0x0FFF) - 7; sdt_scan += 5 + ((SDT[sdt_scan + 3]<<8 | SDT[sdt_scan + 4]) & 0x0FFF))
		{
			for(pid_test = 0; pid_test < pids_found; pid_test++)
			{
				if((SDT[sdt_scan]<<8 | SDT[sdt_scan + 1]) == pid_table[pid_test].service_id && SDT[sdt_scan + 5] == 0x48)
				{
					diff = 0;
					pid_table[pid_test].service_name_len = SDT[sdt_scan+9 + SDT[sdt_scan+8]];
					for(byte = 0; byte < pid_table[pid_test].service_name_len; byte++)
					{
						if(SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] == 'Ä') SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] = 0x5B;
						if(SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] == 'ä') SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] = 0x7B;
						if(SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] == 'Ö') SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] = 0x5C;
						if(SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] == 'ö') SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] = 0x7C;
						if(SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] == 'Ü') SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] = 0x5D;
						if(SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] == 'ü') SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] = 0x7D;
						if(SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] == 'ß') SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] = 0x7E;

						if(SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] >= 0x80 && SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte] <= 0x9F) diff--;
						else pid_table[pid_test].service_name[byte + diff] = SDT[sdt_scan+10 + SDT[sdt_scan + 8] + byte];
					}

					pid_table[pid_test].service_name_len += diff;
				}
			}
		}

	//show current servicename

		current_service = 0;

		if(vtxtpid != 0) 
		{
			while(pid_table[current_service].vtxt_pid != vtxtpid && current_service < pids_found)
			{
				current_service++;
			}
		}

		RenderMessage(ShowServiceName);

	return 1;
}

/******************************************************************************
 * ConfigMenu                                                                 *
 ******************************************************************************/

void ConfigMenu()
{
	struct dmxPesFilterParams dmx_flt;
	int val, byte, line, menuitem = 1;
	int current_pid = 0;

	char menu[] =	"àááááááááááááááááááááááááááááâè««««««««««««««««««««««««««««««›"
					"ã    TuxTxt-Konfiguration    äé«¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤«›"
					"åææææææææææææææææææææææææææææçé««««««««««««««««««««««««««««««›"
					"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
					"ã      Videotextauswahl      äéËÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇË›"
					"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
					"ãí                          îäéZXXXXXXXXXXXXXXXXXXXXXXXXXXXXZ›"
					"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
					"ã      Bildschirmformat      äéËÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇË›"
					"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
					"ã16:9 im Standard-Modus = ausäéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
					"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
					"ã16:9 im TextBild-Modus = einäéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
					"åææææææææææææææææææææææææææææçéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
					"ëìììììììììììììììììììììììììììììê›››››››››››››››››››››››››››››››";

	//set current vtxt

		if(vtxtpid == 0) vtxtpid = pid_table[0].vtxt_pid;
		else
		{
			while(pid_table[current_pid].vtxt_pid != vtxtpid && current_pid < pids_found)
			{
				current_pid++;
			}
		}

		if (SDT_ready)
			memcpy(&menu[6*62 + 3 + (24-pid_table[current_pid].service_name_len)/2], &pid_table[current_pid].service_name, pid_table[current_pid].service_name_len);
		else
			sprintf(&menu[6*62 + 13], "%.4X", vtxtpid);

		if(current_pid == 0 || pids_found == 1)				 menu[6*62 +  1] = ' ';
		if(current_pid == pids_found - 1 || pids_found == 1) menu[6*62 + 28] = ' ';

	//set 16:9 modi

		if(fnc_mode1 == AVS_FNCOUT_EXT169) memcpy(&menu[10*62 + 26], "ein", 3);
		if(fnc_mode2 == AVS_FNCOUT_EXT43)  memcpy(&menu[12*62 + 26], "aus", 3);

	//clear framebuffer

		memset(lfb, transp, var_screeninfo.xres * var_screeninfo.yres);

	//reset to normal mode

		if(zoommode) zoommode = 0;

		if(transpmode)
		{
			transpmode = 0;

			memset(&backbuffer, black, sizeof(backbuffer));
		}

		if(screenmode)
		{
			screenmode = 0;

			avia_pig_hide(pig);

			ioctl(avs, AVSIOSSCARTPIN8, &fnc_mode1);

			fontwidth  = 16;
			fontheight = 22;
			FT_Set_Pixel_Sizes(face, fontwidth, fontheight);
		}

	//render menu

		PosY = StartY + fixfontheight*5;

		for(line = 0; line < 15; line++)
		{
			PosX = StartX + fontwidth*4 + fontwidth/2;

			for(byte = 0; byte < 31; byte++)
			{
				RenderCharFB(menu[line*62 + byte], menu[line*62 + byte+31]);
			}

			PosY += fixfontheight;
		}

	//set blocking mode

		val = fcntl(rc, F_GETFL);
		fcntl(rc, F_SETFL, val &~ O_NONBLOCK);

	//loop

		do
		{
			GetRCCode();

			switch(RCCode)
			{
				case RC_UP:		if(menuitem > 1) menuitem--;

								if(menuitem == 1)
								{
									PosX = StartX + fontwidth*4 + fontwidth/2;
									PosY = StartY + fixfontheight*11;
									for(byte = 0; byte < 31; byte++)
									{
										RenderCharFB(menu[62*6 + byte], menu[62*6 + byte+31]);
									}

									PosX = StartX + fontwidth*4 + fontwidth/2;
									PosY = StartY + fixfontheight*15;
									for(byte = 0; byte < 31; byte++)
									{
										RenderCharFB(menu[62*10 + byte], menu[62*10 + byte+31]);
									}
								}
								else
								{
									PosX = StartX + fontwidth*4 + fontwidth/2;
									PosY = StartY + fixfontheight*15;
									for(byte = 0; byte < 31; byte++)
									{
										RenderCharFB(menu[62*10 + byte], menu[62*6 + byte+31]);
									}

									PosX = StartX + fontwidth*4 + fontwidth/2;
									PosY = StartY + fixfontheight*17;
									for(byte = 0; byte < 31; byte++)
									{
										RenderCharFB(menu[62*12 + byte], menu[62*10 + byte+31]);
									}
								}
								break;

				case RC_DOWN:	if(menuitem < 3) menuitem++;

								if(menuitem == 2)
								{
									PosX = StartX + fontwidth*4 + fontwidth/2;
									PosY = StartY + fixfontheight*11;
									for(byte = 0; byte < 31; byte++)
									{
										RenderCharFB(menu[62*6 + byte], menu[62*10 + byte+31]);
									}

									PosX = StartX + fontwidth*4 + fontwidth/2;
									PosY = StartY + fixfontheight*15;
									for(byte = 0; byte < 31; byte++)
									{
										RenderCharFB(menu[62*10 + byte], menu[62*6 + byte+31]);
									}
								}
								else
								{
									PosX = StartX + fontwidth*4 + fontwidth/2;
									PosY = StartY + fixfontheight*15;
									for(byte = 0; byte < 31; byte++)
									{
										RenderCharFB(menu[62*10 + byte], menu[62*10 + byte+31]);
									}

									PosX = StartX + fontwidth*4 + fontwidth/2;
									PosY = StartY + fixfontheight*17;
									for(byte = 0; byte < 31; byte++)
									{
										RenderCharFB(menu[62*12 + byte], menu[62*6 + byte+31]);
									}
								}
								break;

				case RC_LEFT:	if(menuitem == 1 && current_pid > 0)
								{
									current_pid--;

									memset(&menu[6*62 + 3], ' ', 24);

									if(SDT_ready) memcpy(&menu[6*62 + 3 + (24-pid_table[current_pid].service_name_len)/2], &pid_table[current_pid].service_name, pid_table[current_pid].service_name_len);
									else		  sprintf(&menu[6*62 + 13], "%.4X", pid_table[current_pid].vtxt_pid);

										if(pids_found > 1)
										{
											if(current_pid == 0)
											{
												menu[6*62 +  1] = ' ';
												menu[6*62 + 28] = 'î';
											}
											else
											{
												menu[6*62 +  1] = 'í';
												menu[6*62 + 28] = 'î';
											}
										}

									PosX = StartX + fontwidth*4 + fontwidth/2;
									PosY = StartY + fixfontheight*11;
									for(byte = 0; byte < 31; byte++)
									{
										RenderCharFB(menu[62*6 + byte], menu[62*6 + byte+31]);
									}
								}
								break;

				case RC_RIGHT:	if(menuitem == 1 && current_pid < pids_found - 1)
								{
									current_pid++;

									memset(&menu[6*62 + 3], ' ', 24);

									if(SDT_ready) memcpy(&menu[6*62 + 3 + (24-pid_table[current_pid].service_name_len)/2], &pid_table[current_pid].service_name, pid_table[current_pid].service_name_len);
									else		  sprintf(&menu[6*62 + 13], "%.4X", pid_table[current_pid].vtxt_pid);

									if(pids_found > 1)
									{
										if(current_pid == pids_found - 1)
										{
											menu[6*62 +  1] = 'í';
											menu[6*62 + 28] = ' ';
										}
										else
										{
											menu[6*62 +  1] = 'í';
											menu[6*62 + 28] = 'î';
										}
									}

									PosX = StartX + fontwidth*4 + fontwidth/2;
									PosY = StartY + fixfontheight*11;
									for(byte = 0; byte < 31; byte++)
									{
										RenderCharFB(menu[62*6 + byte], menu[62*6 + byte+31]);
									}
								}
								break;

				case RC_OK:		switch(menuitem)
								{
									case 1:	if(pids_found > 1)
											{
												//stop demuxer

													ioctl(dmx, DMX_STOP);

												//reset data

													memset(&cachetable, 0, sizeof(cachetable));
													memset(&subpagetable, 0xFF, sizeof(subpagetable));
													memset(&backbuffer, black, sizeof(backbuffer));

													page_atrb[32] = transp<<4 | transp;

													inputcounter = 2;

													cached_pages = 0;

													current_page	= -1;
													current_subpage	= -1;

													page	 = 0x100;
													lastpage = 0x100;
													prev_100 = 0x100;
													prev_10  = 0x100;
													next_100 = 0x100;
													next_10  = 0x100;
													subpage	 = 0;

													pageupdate = 0;

													zap_subpage_manual = 0;

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

												//start demuxer with new vtxtpid

													vtxtpid = pid_table[current_pid].vtxt_pid;

													dmx_flt.pid		= vtxtpid;
													dmx_flt.input	= DMX_IN_FRONTEND;
													dmx_flt.output	= DMX_OUT_TAP;
													dmx_flt.pesType	= DMX_PES_OTHER;
													dmx_flt.flags	= DMX_IMMEDIATE_START;

													if(ioctl(dmx, DMX_SET_PES_FILTER, &dmx_flt) == -1)
													{
														perror("TuxTxt <DMX_SET_PES_FILTER>");
													}

												//show new videotext

													current_service = current_pid;
													RenderMessage(ShowServiceName);

													fcntl(rc, F_SETFL, O_NONBLOCK);
													pageupdate = 1;
													RCCode = 0;
													return;
											}
											break;

									case 2:	fnc_mode1++;
											if(fnc_mode1 > 2) fnc_mode1 = 1;

											if(fnc_mode1 == 1) memcpy(&menu[62*10 + 26], "ein", 3);
											else			   memcpy(&menu[62*10 + 26], "aus", 3);

											PosX = StartX + fontwidth*4 + fontwidth/2;
											PosY = StartY + fixfontheight*15;
											for(byte = 0; byte < 31; byte++)
											{
												RenderCharFB(menu[62*10 + byte], menu[62*6 + byte+31]);
											}

											ioctl(avs, AVSIOSSCARTPIN8, &fnc_mode1);

											break;

									case 3:	fnc_mode2++;
											if(fnc_mode2 > 2) fnc_mode2 = 1;

											if(fnc_mode2 == 1) memcpy(&menu[62*12 + 26], "ein", 3);
											else			   memcpy(&menu[62*12 + 26], "aus", 3);

											PosX = StartX + fontwidth*4 + fontwidth/2;
											PosY = StartY + fixfontheight*17;
											for(byte = 0; byte < 31; byte++)
											{
												RenderCharFB(menu[62*12 + byte], menu[62*6 + byte+31]);
											}
								}
			}
		}
		while(RCCode != RC_HOME);

	//reset to nonblocking mode

		fcntl(rc, F_SETFL, O_NONBLOCK);
		pageupdate = 1;
		RCCode = 0;
}

/******************************************************************************
 * PageInput                                                                  *
 ******************************************************************************/

void PageInput(int Number)
{
	static int temp_page;
	int zoom = 0;

	//clear temp_page

		if(inputcounter == 2) temp_page = 0;

	//check for 0 & 9 on first position

		if(Number == 0 && inputcounter == 2)
		{
			//set page

				temp_page = 0x100;

				inputcounter = -1;
		}
		else if(Number == 9 && inputcounter == 2)
		{
			//set page

				temp_page = lastpage;

				inputcounter = -1;
		}

	//show pageinput

		if(zoommode == 2)
		{
			zoommode = 1;
			CopyBB2FB();
		}

		if(zoommode == 1) zoom = 1<<9;

		PosY = StartY;

		switch(inputcounter)
		{
			case 2:	PosX = StartX + 8*fontwidth;
					RenderCharFB(Number | '0', black<<4 | white);
					RenderCharFB('-', black<<4 | white);
					RenderCharFB('-', black<<4 | white);
					break;

			case 1:	PosX = StartX + 9*fontwidth;
					RenderCharFB(Number | '0', black<<4 | white);
					break;

			case 0:	PosX = StartX + 10*fontwidth;
					RenderCharFB(Number | '0', black<<4 | white);
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
					RenderMessage(PageNotFound);
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

		if(zoommode == 2) zoommode = 1;

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

		if(zoommode == 2) zoommode = 1;

		subpage = subpagetable[page];
		pageupdate = 1;
		printf("TuxTxt <PrevPageOne => %.3X-%.2X>\n", page, subpage);
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
						//enable manual subpage zapping

							zap_subpage_manual = 1;

						//update page

							if(zoommode == 2) zoommode = 1;

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
						//enable manual subpage zapping

							zap_subpage_manual = 1;

						//update page

							if(zoommode == 2) zoommode = 1;

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
						//enable manual subpage zapping

							zap_subpage_manual = 1;

						//update page

							if(zoommode == 2) zoommode = 1;

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
						//enable manual subpage zapping

							zap_subpage_manual = 1;

						//update page

							if(zoommode == 2) zoommode = 1;

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
 * Prev100                                                                    *
 ******************************************************************************/

void Prev100()
{
	if(zoommode == 2) zoommode = 1;

	lastpage = page;
	page = prev_100;
	subpage = subpagetable[page];
	pageupdate = 1;

	printf("TuxTxt <Prev100 => %.3X>\n", page);
}

/******************************************************************************
 * Prev10                                                                     *
 ******************************************************************************/

void Prev10()
{
	if(zoommode == 2) zoommode = 1;

	lastpage = page;
	page = prev_10;
	subpage = subpagetable[page];
	pageupdate = 1;

	printf("TuxTxt <Prev10 => %.3X>\n", page);
}

/******************************************************************************
 * Next10                                                                    *
 ******************************************************************************/

void Next10()
{
	if(zoommode == 2) zoommode = 1;

	lastpage = page;
	page = next_10;
	subpage = subpagetable[page];
	pageupdate = 1;

	printf("TuxTxt <Next10 => %.3X>\n", page);
}

/******************************************************************************
 * Next100                                                                    *
 ******************************************************************************/

void Next100()
{
	if(zoommode == 2) zoommode = 1;

	lastpage = page;
	page = next_100;
	subpage = subpagetable[page];
	pageupdate = 1;

	printf("TuxTxt <Next100 => %.3X>\n", page);
}

/******************************************************************************
 * PageCatching                                                               *
 ******************************************************************************/

void PageCatching()
{
	int val;

	//check for pagenumber(s)

		CatchNextPage(1);

		if(!catched_page) return;

	//set blocking mode

		val = fcntl(rc, F_GETFL);
		fcntl(rc, F_SETFL, val &~ O_NONBLOCK);

	//loop

		do
		{
			GetRCCode();

			switch(RCCode)
			{
				case RC_UP:		CatchPrevPage();
								break;

				case RC_DOWN:	CatchNextPage(0);
								break;

				case RC_HOME:	fcntl(rc, F_SETFL, O_NONBLOCK);
								pageupdate = 1;
								RCCode = 0;
								return;
			}
		}
		while(RCCode != RC_OK);

	//set new page

		if(zoommode == 2) zoommode = 1;

		lastpage = page;
		page = catched_page;
		subpage = subpagetable[page];
		pageupdate = 1;

	//reset to nonblocking mode

		fcntl(rc, F_SETFL, O_NONBLOCK);
}

/******************************************************************************
 * CatchNextPage                                                              *
 ******************************************************************************/

void CatchNextPage(int Init)
{
	int tmp_page, pages_found=0;

	//init

		if(Init)
		{
			catch_row = 1;
			catch_col = 0;
			catched_page = 0;
		}

	//catch next page

		for( ; catch_row < 24; catch_row++)
		{
			for( ; catch_col < 40; catch_col++)
			{
				if(!(page_atrb[catch_row*40 + catch_col] & 1<<8) && (page_char[catch_row*40 + catch_col] >= '0' && page_char[catch_row*40 + catch_col] <= '9' && page_char[catch_row*40 + catch_col + 1] >= '0' && page_char[catch_row*40 + catch_col + 1] <= '9' && page_char[catch_row*40 + catch_col + 2] >= '0' && page_char[catch_row*40 + catch_col + 2] <= '9') && (page_char[catch_row*40 + catch_col - 1] < '0' || page_char[catch_row*40 + catch_col - 1] > '9') && (page_char[catch_row*40 + catch_col + 3] < '0' || page_char[catch_row*40 + catch_col + 3] > '9'))
				{
					tmp_page = ((page_char[catch_row*40 + catch_col] - '0')<<8) | ((page_char[catch_row*40 + catch_col + 1] - '0')<<4) | (page_char[catch_row*40 + catch_col + 2] - '0');

					if(tmp_page != catched_page && tmp_page >= 0x100 && tmp_page <= 0x899)
					{
						catched_page = tmp_page;
						pages_found++;

						RenderCatchedPage();

						catch_col += 3;

						printf("TuxTxt <PageCatching => %.3X\n", catched_page);

						return;
					}
				}
			}

			catch_col = 0;
		}

		if(Init)
		{
			printf("TuxTxt <PageCatching => no PageNumber>\n");
			return;
		}

	//wrap around

		catch_row = 1;
		catch_col = 0;

		if(!pages_found) return;

		pages_found = 0;

		CatchNextPage(0);
}

/******************************************************************************
 * CatchPrevPage                                                              *
 ******************************************************************************/

void CatchPrevPage()
{
	int tmp_page, pages_found=0;

	//catch prev page

		for( ; catch_row > 0; catch_row--)
		{
			for( ; catch_col > 0; catch_col--)
			{
				if(!(page_atrb[catch_row*40 + catch_col] & 1<<8) && (page_char[catch_row*40 + catch_col] >= '0' && page_char[catch_row*40 + catch_col] <= '9' && page_char[catch_row*40 + catch_col + 1] >= '0' && page_char[catch_row*40 + catch_col + 1] <= '9' && page_char[catch_row*40 + catch_col + 2] >= '0' && page_char[catch_row*40 + catch_col + 2] <= '9') && (page_char[catch_row*40 + catch_col - 1] < '0' || page_char[catch_row*40 + catch_col - 1] > '9') && (page_char[catch_row*40 + catch_col + 3] < '0' || page_char[catch_row*40 + catch_col + 3] > '9'))
				{
					tmp_page = ((page_char[catch_row*40 + catch_col] - '0')<<8) | ((page_char[catch_row*40 + catch_col + 1] - '0')<<4) | (page_char[catch_row*40 + catch_col + 2] - '0');

					if(tmp_page != catched_page && tmp_page >= 0x100 && tmp_page <= 0x899)
					{
						catched_page = tmp_page;
						pages_found++;

						RenderCatchedPage();

						catch_col -= 3;

						printf("TuxTxt <PageCatching => %.3X\n", catched_page);

						return;
					}
				}
			}

			catch_col = 39;
		}

	//wrap around

		catch_row = 23;
		catch_col = 39;

		if(!pages_found) return;

		pages_found = 0;

		CatchPrevPage();
}

/******************************************************************************
 * RenderCatchedPage                                                          *
 ******************************************************************************/

void RenderCatchedPage()
{
	static int old_row, old_col;
	int zoom = 0;

	//handle zoom

		if(zoommode) zoom = 1<<9;

	//restore pagenumber

		PosX = StartX + old_col*fontwidth;
		if(zoommode == 2) PosY = StartY + (old_row-12)*fixfontheight*((zoom>>9)+1);
		else			  PosY = StartY + old_row*fixfontheight*((zoom>>9)+1);

		RenderCharFB(page_char[old_row*40 + old_col    ], page_atrb[old_row*40 + old_col    ]);
		RenderCharFB(page_char[old_row*40 + old_col + 1], page_atrb[old_row*40 + old_col + 1]);
		RenderCharFB(page_char[old_row*40 + old_col + 2], page_atrb[old_row*40 + old_col + 2]);

		old_row = catch_row;
		old_col = catch_col;

	//mark pagenumber

		if(zoommode == 1 && catch_row > 11)
		{
			zoommode = 2;
			CopyBB2FB();
		}
		else if(zoommode == 2 && catch_row < 12)
		{
			zoommode = 1;
			CopyBB2FB();
		}

		PosX = StartX + catch_col*fontwidth;
		if(zoommode == 2) PosY = StartY + (catch_row-12)*fixfontheight*((zoom>>9)+1);
		else			  PosY = StartY + catch_row*fixfontheight*((zoom>>9)+1);

		RenderCharFB(page_char[catch_row*40 + catch_col    ], (page_atrb[catch_row*40 + catch_col    ] & 1<<9) | ((page_atrb[catch_row*40 + catch_col    ] & 0x0F)<<4) | ((page_atrb[catch_row*40 + catch_col    ] & 0xF0)>>4));
		RenderCharFB(page_char[catch_row*40 + catch_col + 1], (page_atrb[catch_row*40 + catch_col + 1] & 1<<9) | ((page_atrb[catch_row*40 + catch_col + 1] & 0x0F)<<4) | ((page_atrb[catch_row*40 + catch_col + 1] & 0xF0)>>4));
		RenderCharFB(page_char[catch_row*40 + catch_col + 2], (page_atrb[catch_row*40 + catch_col + 2] & 1<<9) | ((page_atrb[catch_row*40 + catch_col + 2] & 0x0F)<<4) | ((page_atrb[catch_row*40 + catch_col + 2] & 0xF0)>>4));
}

/******************************************************************************
 * SwitchZoomMode                                                             *
 ******************************************************************************/

void SwitchZoomMode()
{
	if(subpagetable[page] != 0xFF)
	{
		//toggle mode

			zoommode++;
			if(zoommode == 3) zoommode = 0;

			printf("TuxTxt <SwitchZoomMode => %d>\n", zoommode);

		//update page

			CopyBB2FB();
	}
}

/******************************************************************************
 * SwitchScreenMode                                                           *
 ******************************************************************************/

void SwitchScreenMode()
{
	int error;

	//reset transparency mode

		if(transpmode) transpmode = 0;

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

			avia_pig_set_pos(pig, (StartX+322), StartY);
			avia_pig_set_size(pig, 320, 526);
			avia_pig_set_stack(pig, 2);
			avia_pig_show(pig);

			ioctl(avs, AVSIOSSCARTPIN8, &fnc_mode2);
		}
		else
		{
			fontwidth  = 16;
			fontheight = 22;

			avia_pig_hide(pig);

			ioctl(avs, AVSIOSSCARTPIN8, &fnc_mode1);
		}

		if((error = FT_Set_Pixel_Sizes(face, fontwidth, fontheight)) != 0)
		{
			printf("TuxTxt <FT_Set_Pixel_Sizes => 0x%.2X>", error);
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
			if(transpmode == 3) transpmode = 0;

			printf("TuxTxt <SwitchTranspMode => %d>\n", transpmode);

		//set mode

			if(!transpmode)
			{
				memset(&backbuffer, black, sizeof(backbuffer));
				pageupdate = 1;
			}
			else if(transpmode == 1)
			{
				memset(&backbuffer, transp, sizeof(backbuffer));
				pageupdate = 1;
			}
			else
			{
				memset(lfb, transp, var_screeninfo.xres * var_screeninfo.yres);
			}
	}
}

/******************************************************************************
 * SwitchHintMode                                                             *
 ******************************************************************************/

void SwitchHintMode()
{
	//toggle mode

		hintmode++;
		hintmode &= 1;

		printf("TuxTxt <SwitchHintMode => %d>\n", hintmode);

	//update page

		pageupdate = 1;
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

						if(zoommode && (Attribute & 1<<9))
						{
							*(lfb + (x+PosX) + ((y+PosY+1)*var_screeninfo.xres)) = Attribute & 15;
							*(lfb + (x+PosX) + ((y+PosY+2)*var_screeninfo.xres)) = Attribute & 15;
							*(lfb + (x+PosX) + ((y+PosY+3)*var_screeninfo.xres)) = Attribute & 15;
						}
						else if(zoommode || (Attribute & 1<<9)) *(lfb + (x+PosX) + ((y+PosY+1)*var_screeninfo.xres)) = Attribute & 15;
					}
					else
					{
						if(transpmode == 1)
						{
							Attribute &= 0xFF0F;
							Attribute |= transp<<4;
						}

						*(lfb + (x+PosX) + ((y+PosY)*var_screeninfo.xres)) = Attribute>>4 & 15;

						if(zoommode && (Attribute & 1<<9))
						{
							*(lfb + (x+PosX) + ((y+PosY+1)*var_screeninfo.xres)) = Attribute>>4 & 15;
							*(lfb + (x+PosX) + ((y+PosY+2)*var_screeninfo.xres)) = Attribute>>4 & 15;
							*(lfb + (x+PosX) + ((y+PosY+3)*var_screeninfo.xres)) = Attribute>>4 & 15;
						}
						else if(zoommode || (Attribute & 1<<9)) *(lfb + (x+PosX) + ((y+PosY+1)*var_screeninfo.xres)) = Attribute>>4 & 15;
					}

					x++;
				}
			}

			x = 0;
			y++;

			if(zoommode && (Attribute & 1<<9)) y += 3;
			else if(zoommode || (Attribute & 1<<9)) y++;
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
						if(transpmode == 1 && PosY < StartY + 24*fixfontheight)
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
 * RenderMessage                                                              *
 ******************************************************************************/

void RenderMessage(int Message)
{
	int byte;
	int fbcolor, timecolor, menucolor;
	char message_1[] = "àáááááááááááááááááááááááááááááááááááâè";
	char message_2[] = "ã                                   äé";
	char message_3[] = "ã   suche nach Videotext-Diensten   äé";
	char message_4[] = "ã                                   äé";
	char message_5[] = "åæææææææææææææææææææææææææææææææææææçé";
	char message_6[] = "ëììììììììììììììììììììììììììììììììììììê";

	char message_7[] = "ã keine Videotext-Dienste verf}gbar äé";
	char message_8[] = "ã  warte auf Empfang von Seite 100  äé";

	//reset zoom

		zoommode = 0;

	//set colors

		if(screenmode == 1)
		{
			fbcolor   = black;
			timecolor = black<<4 | black;
			menucolor = menu1;
		}
		else
		{

			fbcolor   = transp;
			timecolor = transp<<4 | transp;
			menucolor = menu3;
		}

	//clear framebuffer

		memset(lfb, fbcolor, var_screeninfo.xres * var_screeninfo.yres);

	//hide timestring

		page_atrb[32] = timecolor;

	//set pagenumber

		if(Message == PageNotFound || Message == ShowServiceName)
		{
			memset(&message_3[1], ' ', 35);

			message_8[31] = (page >> 8) | '0';
			message_8[32] = (page & 0x0F0)>>4 | '0';
			message_8[33] = (page & 0x00F) | '0';
			memcpy(&message_4, &message_8, sizeof(message_8));

			if(SDT_ready) memcpy(&message_2[2 + (35 - pid_table[current_service].service_name_len)/2], &pid_table[current_service].service_name, pid_table[current_service].service_name_len);
			else		  sprintf(&message_2[17], "%.4X", pid_table[current_service].vtxt_pid);
		}
		else if(Message == NoServicesFound) memcpy(&message_3, &message_7, sizeof(message_7));

	//render infobar

		PosX = StartX + fontwidth+5;
		PosY = StartY + fixfontheight*16;
		for(byte = 0; byte < 37; byte++)
		{
			RenderCharFB(message_1[byte], menucolor<<4 | menu2);
		}
		RenderCharFB(message_1[37], fbcolor<<4 | menu2);

		PosX = StartX + fontwidth+5;
		PosY = StartY + fixfontheight*17;
		RenderCharFB(message_2[0], menucolor<<4 | menu2);
		for(byte = 1; byte < 36; byte++)
		{
			RenderCharFB(message_2[byte], menucolor<<4 | white);
		}
		RenderCharFB(message_2[36], menucolor<<4 | menu2);
		RenderCharFB(message_2[37], fbcolor<<4 | menu2);

		PosX = StartX + fontwidth+5;
		PosY = StartY + fixfontheight*18;
		RenderCharFB(message_3[0], menucolor<<4 | menu2);
		for(byte = 1; byte < 36; byte++)
		{
			RenderCharFB(message_3[byte], menucolor<<4 | white);
		}
		RenderCharFB(message_3[36], menucolor<<4 | menu2);
		RenderCharFB(message_3[37], fbcolor<<4 | menu2);

		PosX = StartX + fontwidth+5;
		PosY = StartY + fixfontheight*19;
		RenderCharFB(message_4[0], menucolor<<4 | menu2);
		for(byte = 1; byte < 36; byte++)
		{
			RenderCharFB(message_4[byte], menucolor<<4 | white);
		}
		RenderCharFB(message_4[36], menucolor<<4 | menu2);
		RenderCharFB(message_4[37], fbcolor<<4 | menu2);

		PosX = StartX + fontwidth+5;
		PosY = StartY + fixfontheight*20;
		for(byte = 0; byte < 37; byte++)
		{
			RenderCharFB(message_5[byte], menucolor<<4 | menu2);
		}
		RenderCharFB(message_5[37], fbcolor<<4 | menu2);

		PosX = StartX + fontwidth+5;
		PosY = StartY + fixfontheight*21;
		for(byte = 0; byte < 38; byte++)
		{
			RenderCharFB(message_6[byte], fbcolor<<4 | menu2);
		}
}

/******************************************************************************
 * RenderPage                                                                 *
 ******************************************************************************/

void RenderPage()
{
	int row, col, byte;

	if(transpmode != 2 && pageupdate && current_page != page && inputcounter == 2)
	{
		//reset update flag

			pageupdate = 0;

		//decode page

			if(subpagetable[page] != 0xFF) DecodePage();
			else
			{
				RenderMessage(PageNotFound);
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

			CopyBB2FB();
	}
	else if(transpmode != 2 && zoommode != 2)
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
 * CreateLine25                                                               *
 ******************************************************************************/

void CreateLine25()
{
	int byte;
	char line25[] = "   ?00<      ??0<      >??0      >?00   ((((((((((1111111111AAAAAAAAAAXXXXXXXXXX";

	//get prev 100th

		prev_100 = page & 0xF00;

		if(!(page & 0x0FF) || subpagetable[prev_100] == 0xFF)
		{
			do
			{
				prev_100 -= 0x100;

				if(prev_100 == 0x000) prev_100 = 0x800;
			}
			while(subpagetable[prev_100] == 0xFF);
		}

		line25[3] = (prev_100 >> 8) | '0';

	//get next 100th

		next_100 = page & 0xF00;

		do
		{
			next_100 += 0x100;

			if(next_100 == 0x900) next_100 = 0x100;
		}
		while(subpagetable[next_100] == 0xFF);

		line25[34] = (next_100 >> 8) | '0';

	//get prev 10th

		prev_10 = page & 0xFF0;

		if(!(page & 0x00F) || subpagetable[prev_10] == 0xFF)
		{
			do
			{
				if((prev_10 & 0x0F0) == 0x000) prev_10 -= 0x70;
				else						   prev_10 -= 0x10;

				if(prev_10 <= 0x090)		   prev_10 = 0x890;
			}
			while(subpagetable[prev_10] == 0xFF);
		}

		line25[13] = (prev_10 >> 8) | '0';
		line25[14] = ((prev_10 & 0x0F0)>>4) | '0';

	//get next 10th

		next_10 = page & 0xFF0;

		do
		{
			if((next_10 & 0x0F0) == 0x090) next_10 += 0x70;
			else						   next_10 += 0x10;

			if(next_10 >= 0x900)		   next_10 = 0x100;
		}
		while(subpagetable[next_10] == 0xFF);

		line25[24] = (next_10 >> 8) | '0';
		line25[25] = ((next_10 & 0x0F0)>>4) | '0';

	//render line 25

		PosX = StartX;
		PosY = StartY + 24*fixfontheight;

		for(byte = 0; byte < 40; byte++)
		{
			if(boxed) RenderCharBB(' ', transp<<4 | transp);
			else	  RenderCharBB(line25[byte], line25[byte + 40]);
		}
}

/******************************************************************************
 * CopyBB2FB                                                                  *
 ******************************************************************************/

void CopyBB2FB()
{
	int src, dst = 0;
	int fillcolor = black;

	//line 25

		CreateLine25();

	//copy backbuffer to framebuffer

		if(!zoommode)
		{
			memcpy(lfb, &backbuffer, sizeof(backbuffer));
			return;
		}
		else if(zoommode == 1)
		{
			src = StartY*var_screeninfo.xres;
		}
		else
		{
			src = StartY*var_screeninfo.xres + 12*fixfontheight*var_screeninfo.xres;
		}

		if(transpmode) fillcolor = transp;

		memset(lfb, fillcolor, StartY*var_screeninfo.xres);

		do
		{
			memcpy(lfb + StartY*var_screeninfo.xres + dst, backbuffer + src, var_screeninfo.xres);
			dst += var_screeninfo.xres;
			memcpy(lfb + StartY*var_screeninfo.xres + dst, backbuffer + src, var_screeninfo.xres);
			dst += var_screeninfo.xres;
			src += var_screeninfo.xres;
		}
		while(dst < var_screeninfo.xres * 24*fixfontheight);

		memcpy(lfb + StartY*var_screeninfo.xres + dst, backbuffer + StartY*var_screeninfo.xres + 24*fixfontheight*var_screeninfo.xres, var_screeninfo.xres*fixfontheight);

		memset(lfb + (StartY + 25*fixfontheight)*var_screeninfo.xres, fillcolor, var_screeninfo.xres*var_screeninfo.yres - (StartY + 25*fixfontheight)*var_screeninfo.xres);
}

/******************************************************************************
 * ShowCacheStatus                                                            *
 ******************************************************************************/

void ShowCacheStatus()
{
	int  x, y, show_pages;
	char lcd_backup[120*64 / 8], lcd_backbuffer[120*64 / 8];

	//backup lcd

		read(lcd, &lcd_backup, sizeof(lcd_backup));

	//show status

		show_pages = cached_pages;

		printf("TuxTxt <ShowCacheStatus => %d pages>\n", show_pages);

		memset(&lcd_backbuffer, 0, sizeof(lcd_backbuffer));

		for(y = 0; y < 64; y++)
		{
			for(x = 0; x < 120; x++)
			{
				lcd_backbuffer[x + (y/8)*120] |= lcd_layout[x + y*120] << (y%8);
			}
		}

		for(y = 0; y < 35; y++)
		{
			for(x = 0; x < 21; x++)
			{
				lcd_backbuffer[x+9  + ((y+20)/8)*120] |= digits[x + y*21 + show_pages/1000*21*35] << ((y+20)%8);
				lcd_backbuffer[x+36 + ((y+20)/8)*120] |= digits[x + y*21 + show_pages%1000/100*21*35] << ((y+20)%8);
				lcd_backbuffer[x+63 + ((y+20)/8)*120] |= digits[x + y*21 + show_pages%100/10*21*35] << ((y+20)%8);
				lcd_backbuffer[x+90 + ((y+20)/8)*120] |= digits[x + y*21 + show_pages%10*21*35] << ((y+20)%8);
			}
		}

		write(lcd, &lcd_backbuffer, sizeof(lcd_backbuffer));

		sleep(3);

	//restore lcd

		write(lcd, &lcd_backup, sizeof(lcd_backup));
}

/******************************************************************************
 * DecodePage                                                                 *
 ******************************************************************************/

void DecodePage()
{
	int row, col;
	int hold, clear, loop;
	int foreground, background, doubleheight, charset;
	unsigned char held_mosaic;

	//copy page to decode buffer

		if(zap_subpage_manual) memcpy(&page_char, cachetable[page][subpage], PAGESIZE);
		else				   memcpy(&page_char, cachetable[page][subpagetable[page]], PAGESIZE);

	//update timestring

		memcpy(&page_char[32], &timestring, 8);

	//check for newsflash & subtitle

		if(dehamming[page_char[11-6]] & 12 && !screenmode) boxed = 1;
		else											   boxed = 0;

	//modify header

		if(boxed) memset(&page_char, ' ', 40);
		else      memcpy(&page_char, " TuxTxt ", 8);

	//decode

		for(row = 0; row < 24; row++)
		{
			//start-of-row default conditions

				foreground = white;
				background = black;
				doubleheight = 0;
				charset = 0;
				hold = 0;
				held_mosaic = ' ';

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

						case conceal:			if(!hintmode) foreground = background;
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

						case hold_mosaic:		hold = 1;
												page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												break;

						case release_mosaic:	page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;
												hold = 0;
					}

					//handle spacing attributes

						if(hold && charset) page_char[row*40 + col] = held_mosaic;
						else				page_char[row*40 + col] = ' ';
				}
				else
				{
					page_atrb[row*40 + col] = doubleheight<<9 | charset<<8 | background<<4 | foreground;

					//set new held-mosaic char

						if(charset) held_mosaic = page_char[row*40 + col];

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
							case RC1_UP:		RCCode = RC_UP;
												break;

							case RC1_DOWN:		RCCode = RC_DOWN;
												break;

							case RC1_RIGHT:		RCCode = RC_RIGHT;
												break;

							case RC1_LEFT:		RCCode = RC_LEFT;
												break;

							case RC1_OK:		RCCode = RC_OK;
												break;

							case RC1_0:			RCCode = RC_0;
												break;

							case RC1_1:			RCCode = RC_1;
												break;

							case RC1_2:			RCCode = RC_2;
												break;

							case RC1_3:			RCCode = RC_3;
												break;

							case RC1_4:			RCCode = RC_4;
												break;

							case RC1_5:			RCCode = RC_5;
												break;

							case RC1_6:			RCCode = RC_6;
												break;

							case RC1_7:			RCCode = RC_7;
												break;

							case RC1_8:			RCCode = RC_8;
												break;

							case RC1_9:			RCCode = RC_9;
												break;

							case RC1_RED:		RCCode = RC_RED;
												break;

							case RC1_GREEN:		RCCode = RC_GREEN;
												break;

							case RC1_YELLOW:	RCCode = RC_YELLOW;
												break;

							case RC1_BLUE:		RCCode = RC_BLUE;
												break;

							case RC1_PLUS:		RCCode = RC_PLUS;
												break;

							case RC1_MINUS:		RCCode = RC_MINUS;
												break;

							case RC1_MUTE:		RCCode = RC_MUTE;
												break;

							case RC1_HELP:		RCCode = RC_HELP;
												break;

							case RC1_DBOX:		RCCode = RC_DBOX;
												break;

							case RC1_HOME:		RCCode = RC_HOME;
												break;

							case RC1_STANDBY:	RCCode = RC_STANDBY;
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
									cachetable[current_page][current_subpage] = malloc(PAGESIZE);
									memset(cachetable[current_page][current_subpage], ' ', PAGESIZE);
									cached_pages++;
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
									memset(cachetable[current_page][current_subpage], ' ', PAGESIZE);
								}
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
