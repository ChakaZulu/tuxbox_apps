/******************************************************************************
 *                 <<< TuxTxt - Videotext SoftwareDecoder >>>                 *
 *                                                                            *
 *                (c) Thomas "LazyT" Loewe '02 (LazyT@gmx.net)                *
 *----------------------------------------------------------------------------*
 * History                                                                    *
 *                                                                            *
 *    V1.40: add color setup, use fontcaching                                 *
 *    V1.39: ported to dvb api v3 by obi                                      *
 *    V1.38: some mods & fixes                                                *
 *    V1.37: fixing includes by woglinde                                      *
 *    V1.36: fix lcd-support                                                  *
 *    V1.35: add lcd-support                                                  *
 *    V1.34: add infoline for pagecatching                                    *
 *    V1.33: fix service-switch by wjoost                                     *
 *    V1.32: fix 16:9/4:3 (wss override)                                      *
 *    V1.31: damned infobar                                                   *
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

		printf("\nTuxTxt 1.40 - Copyright (c) Thomas \"LazyT\" Loewe and the TuxBox-Team\n\n");

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
				if(transpmode == 2 && RCCode != RC_MUTE) continue;
				if(subpagetable[page] == 0xFF && RCCode == RC_OK) continue;

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

					case RC_DBOX:	ConfigMenu(0);
									break;

					case RC_STANDBY:;
				}
			}

			//update page or timestring and lcd

				RenderPage();

		}
		while(RCCode != RC_HOME);

	//exit

		CleanUp();
}

/******************************************************************************
 * MyFaceRequester
 ******************************************************************************/

FT_Error MyFaceRequester(FTC_FaceID face_id, FT_Library library, FT_Pointer request_data, FT_Face *aface)
{
	return FT_New_Face(library, face_id, 0, aface);
}

/******************************************************************************
 * Init                                                                       *
 ******************************************************************************/

int Init()
{
	struct dmx_pes_filter_params dmx_flt;
	int error;

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

	//init lcd

		UpdateLCD();

	//load config

		if((conf = fopen(CONFIGDIR "/tuxtxt/tuxtxt.conf", "rb+")) == 0)
		{
			perror("TuxTxt <fopen tuxtxt.conf>");
			return 0;
		}

		fread(&screen_mode1, 1, sizeof(screen_mode1), conf);
		fread(&screen_mode2, 1, sizeof(screen_mode2), conf);
		fread(&color_mode, 1, sizeof(color_mode), conf);

		screen_old1 = screen_mode1;
		screen_old2 = screen_mode2;
		color_old   = color_mode;

	//init fontlibrary

		if((error = FT_Init_FreeType(&library)))
		{
			printf("TuxTxt <FT_Init_FreeType: 0x%.2X>", error);
			return 0;
		}

		if((error = FTC_Manager_New(library, 0, 0, 0, &MyFaceRequester, NULL, &manager)))
		{
			printf("TuxTxt <FTC_Manager_New: 0x%.2X>\n", error);
			return 0;
		}

		if((error = FTC_SBit_Cache_New(manager, &cache)))
		{
			printf("TuxTxt <FTC_SBit_Cache_New: 0x%.2X>\n", error);
			return 0;
		}

		desc.font.face_id	 = FONTDIR "/tuxtxt.fon";
		desc.image_type		 = ftc_image_mono;
		desc.font.pix_width  = 16;
		desc.font.pix_height = 22;

	//center screen

		StartX = sx + (((ex-sx) - 40*desc.font.pix_width) / 2);
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

		if(color_mode)
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

	//map framebuffer into memory

		lfb = (unsigned char*)mmap(0, fix_screeninfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);

		if(!lfb)
		{
			perror("TuxTxt <mmap>");
			return 0;
		}

	//open demuxer

		if((dmx = open("/dev/dvb/adapter0/demux0", O_RDWR)) == -1)
		{
			perror("TuxTxt <open /dev/dvb/adapter0/demux0>");
			return 0;
		}

	//get all vtxt-pids

		if(GetVideotextPIDs() == 0)
		{
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			close(dmx);
			return 0;
		}
		else
		{
			RenderCharLCD(pids_found/10,  7, 44);
			RenderCharLCD(pids_found%10, 19, 44);
		}

	//open avs

		if((avs = open("/dev/dbox/avs0", O_RDWR)) == -1)
		{
			perror("TuxTxt <open /dev/dbox/avs0>");
			return 0;
		}

		ioctl(avs, AVSIOGSCARTPIN8, &fnc_old);
		ioctl(avs, AVSIOSSCARTPIN8, &fncmodes[screen_mode1]);

	//open saa

		if((saa = open("/dev/dbox/saa0", O_RDWR)) == -1)
		{
			perror("TuxTxt <open /dev/dbox/saa0>");
			return 0;
		}

		ioctl(saa, SAAIOGWSS, &saa_old);
		ioctl(saa, SAAIOSWSS, &saamodes[screen_mode1]);

	//open pig

		if((pig = open("/dev/v4l2/capture0", O_RDWR)) == -1)
		{
			perror("TuxTxt <open /dev/v4l2/capture0>");
			return 0;
		}

	//setup rc

		fcntl(rc, F_SETFL, O_NONBLOCK);
		ioctl(rc, RC_IOCTL_BCODES, 1);


	//set filter & start demuxer

		if(vtxtpid == 0)
		{
			if(pids_found > 1) ConfigMenu(1);
			else
			{
				vtxtpid = pid_table[0].vtxt_pid;

				current_service = 0;
				RenderMessage(ShowServiceName);
			}
		}

		dmx_flt.pid		= vtxtpid;
		dmx_flt.input	= DMX_IN_FRONTEND;
		dmx_flt.output	= DMX_OUT_TAP;
		dmx_flt.pes_type= DMX_PES_OTHER;
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
	int preview;

	//hide pig

		if(screenmode) {

			preview = 1;
			
			ioctl(pig, VIDIOC_PREVIEW ,&preview);
			
		}


	//restore videoformat

		ioctl(avs, AVSIOSSCARTPIN8, &fnc_old);
		ioctl(saa, SAAIOSWSS, &saa_old);

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

	//close saa

		close(saa);

	//close freetype

		FTC_Manager_Done(manager);
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

		if(screen_mode1 != screen_old1 || screen_mode2 != screen_old2 || color_mode != color_old)
		{
			rewind(conf);

			fwrite(&screen_mode1, 1, sizeof(screen_mode1), conf);
			fwrite(&screen_mode2, 1, sizeof(screen_mode2), conf);
			fwrite(&color_mode, 1, sizeof(color_mode), conf);

			printf("TuxTxt <saving config>\n");
		}

		fclose(conf);
}

/******************************************************************************
 * GetVideotextPIDs                                                           *
 ******************************************************************************/

int GetVideotextPIDs()
{
	struct dmx_sct_filter_params dmx_flt;
	int pat_scan, pmt_scan, sdt_scan, desc_scan, pid_test, byte, diff;

	unsigned char PAT[1024];
	unsigned char SDT[1024];
	unsigned char PMT[1024];

	//show infobar

		RenderMessage(ShowInfoBar);

	//read PAT to get all PMT's

		memset(&dmx_flt.filter, 0x00, sizeof(struct dmx_filter));

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

		SDT_ready = 0;

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

			RenderMessage(ShowServiceName);
		}

	return 1;
}

/******************************************************************************
 * ConfigMenu                                                                 *
 ******************************************************************************/

void ConfigMenu(int Init)
{
	struct dmx_pes_filter_params dmx_flt;
	int val, byte, line, menuitem = 1;
	int current_pid = 0;
	int preview;

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
					"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
					"ã       Farbintensit{t       äéËÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇË›"
					"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
					"ãText-Farben reduzieren = ausäéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
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

	//set 16:9 modi & color correction

		if(screen_mode1) memcpy(&menu[10*62 + 26], "ein", 3);
		if(!screen_mode2) memcpy(&menu[12*62 + 26], "aus", 3);
		if(color_mode) memcpy(&menu[16*62 + 26], "ein", 3);

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

			preview = 1;
			
			ioctl(pig, VIDIOC_PREVIEW ,&preview);

			ioctl(avs, AVSIOSSCARTPIN8, &fncmodes[screen_mode1]);
			ioctl(saa, SAAIOSWSS, &saamodes[screen_mode1]);

			desc.font.pix_width  = 16;
			desc.font.pix_height = 22;
		}

	//render menu

		PosY = StartY + fixfontheight*5;

		for(line = 0; line < 19; line++)
		{
			PosX = StartX + desc.font.pix_width*4 + desc.font.pix_width/2;

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

								switch(menuitem)
								{
									case 1:	PosX = StartX + desc.font.pix_width*4 + desc.font.pix_width/2;
											PosY = StartY + fixfontheight*11;
											for(byte = 0; byte < 31; byte++)
											{
												RenderCharFB(menu[62*6 + byte], menu[62*6 + byte+31]);
											}

											PosX = StartX + desc.font.pix_width*4 + desc.font.pix_width/2;
											PosY = StartY + fixfontheight*15;
											for(byte = 0; byte < 31; byte++)
											{
												RenderCharFB(menu[62*10 + byte], menu[62*10 + byte+31]);
											}
											break;

									case 2:	PosX = StartX + desc.font.pix_width*4 + desc.font.pix_width/2;
											PosY = StartY + fixfontheight*15;
											for(byte = 0; byte < 31; byte++)
											{
												RenderCharFB(menu[62*10 + byte], menu[62*6 + byte+31]);
											}

											PosX = StartX + desc.font.pix_width*4 + desc.font.pix_width/2;
											PosY = StartY + fixfontheight*17;
											for(byte = 0; byte < 31; byte++)
											{
												RenderCharFB(menu[62*12 + byte], menu[62*12 + byte+31]);
											}
											break;

									case 3:	PosX = StartX + desc.font.pix_width*4 + desc.font.pix_width/2;
											PosY = StartY + fixfontheight*17;
											for(byte = 0; byte < 31; byte++)
											{
												RenderCharFB(menu[62*12 + byte], menu[62*6 + byte+31]);
											}

											PosX = StartX + desc.font.pix_width*4 + desc.font.pix_width/2;
											PosY = StartY + fixfontheight*21;
											for(byte = 0; byte < 31; byte++)
											{
												RenderCharFB(menu[62*16 + byte], menu[62*16 + byte+31]);
											}
								}
								break;

				case RC_DOWN:	if(menuitem < 4) menuitem++;

								switch(menuitem)
								{
									case 2:	PosX = StartX + desc.font.pix_width*4 + desc.font.pix_width/2;
											PosY = StartY + fixfontheight*11;
											for(byte = 0; byte < 31; byte++)
											{
												RenderCharFB(menu[62*6 + byte], menu[62*10 + byte+31]);
											}

											PosX = StartX + desc.font.pix_width*4 + desc.font.pix_width/2;
											PosY = StartY + fixfontheight*15;
											for(byte = 0; byte < 31; byte++)
											{
												RenderCharFB(menu[62*10 + byte], menu[62*6 + byte+31]);
											}
											break;

									case 3:	PosX = StartX + desc.font.pix_width*4 + desc.font.pix_width/2;
											PosY = StartY + fixfontheight*15;
											for(byte = 0; byte < 31; byte++)
											{
												RenderCharFB(menu[62*10 + byte], menu[62*10 + byte+31]);
											}

											PosX = StartX + desc.font.pix_width*4 + desc.font.pix_width/2;
											PosY = StartY + fixfontheight*17;
											for(byte = 0; byte < 31; byte++)
											{
												RenderCharFB(menu[62*12 + byte], menu[62*6 + byte+31]);
											}
											break;

									case 4:	PosX = StartX + desc.font.pix_width*4 + desc.font.pix_width/2;
											PosY = StartY + fixfontheight*17;
											for(byte = 0; byte < 31; byte++)
											{
												RenderCharFB(menu[62*12 + byte], menu[62*12 + byte+31]);
											}

											PosX = StartX + desc.font.pix_width*4 + desc.font.pix_width/2;
											PosY = StartY + fixfontheight*21;
											for(byte = 0; byte < 31; byte++)
											{
												RenderCharFB(menu[62*16 + byte], menu[62*6 + byte+31]);
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

									PosX = StartX + desc.font.pix_width*4 + desc.font.pix_width/2;
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

									PosX = StartX + desc.font.pix_width*4 + desc.font.pix_width/2;
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
												if(Init)
												{
													vtxtpid = pid_table[current_pid].vtxt_pid;
												}
												else
												{
													//stop old decode-thread

														if(pthread_cancel(thread_id) != 0)
														{
															perror("TuxTxt <pthread_cancel>");
														}

														if(pthread_join(thread_id, &thread_result) != 0)
														{
															perror("TuxTxt <pthread_join>");
														}

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

														hintmode = 0;

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
														dmx_flt.pes_type= DMX_PES_OTHER;
														dmx_flt.flags	= DMX_IMMEDIATE_START;

														if(ioctl(dmx, DMX_SET_PES_FILTER, &dmx_flt) == -1)
														{
															perror("TuxTxt <DMX_SET_PES_FILTER>");
														}

													//start new decode-thread

														if(pthread_create(&thread_id, NULL, CacheThread, NULL) != 0)
														{
															perror("TuxTxt <pthread_create>");
														}

														pageupdate = 1;
												}

												//show new videotext

													current_service = current_pid;
													RenderMessage(ShowServiceName);

													fcntl(rc, F_SETFL, O_NONBLOCK);
													RCCode = 0;
													return;
											}
											break;

									case 2:	screen_mode1++;
											screen_mode1 &= 1;

											if(screen_mode1) memcpy(&menu[62*10 + 26], "ein", 3);
											else			 memcpy(&menu[62*10 + 26], "aus", 3);

											PosX = StartX + desc.font.pix_width*4 + desc.font.pix_width/2;
											PosY = StartY + fixfontheight*15;
											for(byte = 0; byte < 31; byte++)
											{
												RenderCharFB(menu[62*10 + byte], menu[62*6 + byte+31]);
											}

											ioctl(avs, AVSIOSSCARTPIN8, &fncmodes[screen_mode1]);
											ioctl(saa, SAAIOSWSS, &saamodes[screen_mode1]);

											break;

									case 3:	screen_mode2++;
											screen_mode2 &= 1;

											if(screen_mode2) memcpy(&menu[62*12 + 26], "ein", 3);
											else			 memcpy(&menu[62*12 + 26], "aus", 3);

											PosX = StartX + desc.font.pix_width*4 + desc.font.pix_width/2;
											PosY = StartY + fixfontheight*17;
											for(byte = 0; byte < 31; byte++)
											{
												RenderCharFB(menu[62*12 + byte], menu[62*6 + byte+31]);
											}
											break;

									case 4:	color_mode++;
											color_mode &= 1;

											if(color_mode) memcpy(&menu[62*16 + 26], "ein", 3);
											else		   memcpy(&menu[62*16 + 26], "aus", 3);

											PosX = StartX + desc.font.pix_width*4 + desc.font.pix_width/2;
											PosY = StartY + fixfontheight*21;
											for(byte = 0; byte < 31; byte++)
											{
												RenderCharFB(menu[62*16 + byte], menu[62*6 + byte+31]);
											}

											if(color_mode)
											{
												if (ioctl(fb, FBIOPUTCMAP, &colormap_2) == -1)
												{
													perror("TuxTxt <FBIOPUTCMAP>");
												}
											}
											else
											{
												if (ioctl(fb, FBIOPUTCMAP, &colormap_1) == -1)
												{
													perror("TuxTxt <FBIOPUTCMAP>");
												}
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
			case 2:	PosX = StartX + 8*desc.font.pix_width;
					RenderCharFB(Number | '0', black<<4 | white);
					RenderCharFB('-', black<<4 | white);
					RenderCharFB('-', black<<4 | white);
					break;

			case 1:	PosX = StartX + 9*desc.font.pix_width;
					RenderCharFB(Number | '0', black<<4 | white);
					break;

			case 0:	PosX = StartX + 10*desc.font.pix_width;
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
					printf("TuxTxt <DirectInput: %.3X-%.2X>\n", page, subpage);
				}
				else
				{
					subpage = 0;
					RenderMessage(PageNotFound);
					printf("TuxTxt <DirectInput: %.3X not found>\n", page);
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
		printf("TuxTxt <NextPageOne: %.3X-%.2X>\n", page, subpage);
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
		printf("TuxTxt <PrevPageOne: %.3X-%.2X>\n", page, subpage);
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
							printf("TuxTxt <NextSubPage: %.3X-%.2X>\n", page, subpage);
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
							printf("TuxTxt <NextSubPage: %.3X-%.2X>\n", page, subpage);
							return;
					}
				}

				printf("TuxTxt <NextSubPage: no other SubPage>\n");
		}
		else
		{
			printf("TuxTxt <NextSubPage: no SubPages>\n");
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
							printf("TuxTxt <PrevSubPage: %.3X-%.2X>\n", page, subpage);
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
							printf("TuxTxt <PrevSubPage: %.3X-%.2X>\n", page, subpage);
							return;
					}
				}

				printf("TuxTxt <PrevSubPage: no other SubPage>\n");
		}
		else
		{
			printf("TuxTxt <PrevSubPage: no SubPages>\n");
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
	inputcounter = 2;
	pageupdate = 1;

	printf("TuxTxt <Prev100: %.3X>\n", page);
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
	inputcounter = 2;
	pageupdate = 1;

	printf("TuxTxt <Prev10: %.3X>\n", page);
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
	inputcounter = 2;
	pageupdate = 1;

	printf("TuxTxt <Next10: %.3X>\n", page);
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
	inputcounter = 2;
	pageupdate = 1;

	printf("TuxTxt <Next100: %.3X>\n", page);
}

/******************************************************************************
 * PageCatching                                                               *
 ******************************************************************************/

void PageCatching()
{
	int val;

	//abort pageinput

		inputcounter = 2;

	//show info line

		pagecatching = 1;
		CopyBB2FB();

	//check for pagenumber(s)

		CatchNextPage(1);

		if(!catched_page)
		{
			pagecatching = 0;
			CopyBB2FB();
			return;
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
				case RC_UP:		CatchPrevPage();
								break;

				case RC_DOWN:	CatchNextPage(0);
								break;

				case RC_HOME:	fcntl(rc, F_SETFL, O_NONBLOCK);
								pageupdate = 1;
								pagecatching = 0;
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
		pagecatching = 0;

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

						printf("TuxTxt <PageCatching: %.3X\n", catched_page);

						return;
					}
				}
			}

			catch_col = 0;
		}

		if(Init)
		{
			printf("TuxTxt <PageCatching: no PageNumber>\n");
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

						printf("TuxTxt <PageCatching: %.3X\n", catched_page);

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

		PosX = StartX + old_col*desc.font.pix_width;
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

		PosX = StartX + catch_col*desc.font.pix_width;
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

			printf("TuxTxt <SwitchZoomMode: %d>\n", zoommode);

		//update page

			CopyBB2FB();
	}
}

/******************************************************************************
 * SwitchScreenMode                                                           *
 ******************************************************************************/

void SwitchScreenMode()
{
	int preview;
	struct v4l2_window window;

	//reset transparency mode

		if(transpmode) transpmode = 0;

	//toggle mode

		screenmode++;
		screenmode &= 1;

		printf("TuxTxt <SwitchScreenMode: %d>\n", screenmode);

	//update page

		pageupdate = 1;

	//clear backbuffer

		memset(&backbuffer, black, sizeof(backbuffer));

	//set mode

		if(screenmode)
		{
			desc.font.pix_width  = 8;
			desc.font.pix_height = 21;

			window.x = StartX+322;
			window.y = StartY;
			window.width = 320;
			window.height = 526;
			window.chromakey = 0;
			window.clips = NULL;
			window.clipcount = 0;
			
			ioctl(pig, VIDIOC_S_WIN, &window);
			
			preview = 1;
			
			ioctl(pig, VIDIOC_PREVIEW ,&preview);

			ioctl(avs, AVSIOSSCARTPIN8, &fncmodes[screen_mode2]);
			ioctl(saa, SAAIOSWSS, &saamodes[screen_mode2]);
		}
		else
		{
			desc.font.pix_width  = 16;
			desc.font.pix_height = 22;

			preview = 0;
			
			ioctl(pig, VIDIOC_PREVIEW ,&preview);

			ioctl(avs, AVSIOSSCARTPIN8, &fncmodes[screen_mode1]);
			ioctl(saa, SAAIOSWSS, &saamodes[screen_mode1]);
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

			printf("TuxTxt <SwitchTranspMode: %d>\n", transpmode);

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

		printf("TuxTxt <SwitchHintMode: %d>\n", hintmode);

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

		if((error = FTC_SBit_Cache_Lookup(cache, &desc, Char + ((Attribute>>8 & 1) * (128-32)) + 1, &sbit)))
		{
			printf("TuxTxt <FTC_SBit_Cache_Lookup: 0x%.2X>\n", error);
			PosX += desc.font.pix_width;
			return;
		}

	//render char

		for(Row = 0; Row < fixfontheight; Row++)
		{
			for(Pitch = 0; Pitch < sbit->pitch; Pitch++)
			{
				for(Bit = 7; Bit >= 0; Bit--)
				{
					if((sbit->buffer[Row * sbit->pitch + Pitch]) & 1<<Bit)
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

	PosX += desc.font.pix_width;
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
			PosX += desc.font.pix_width;
			return;
		}

	//load char

		if((error = FTC_SBit_Cache_Lookup(cache, &desc, Char + ((Attribute>>8 & 1) * (128-32)) + 1, &sbit)))
		{
			printf("TuxTxt <FT_SBit_Cache_Lookup %.3d: 0x%.2X>\n", Char, error);
			PosX += desc.font.pix_width;
			return;
		}

	//render char

		for(Row = 0; Row < fixfontheight; Row++)
		{
			for(Pitch = 0; Pitch < sbit->pitch; Pitch++)
			{
				for(Bit = 7; Bit >= 0; Bit--)
				{
					if((sbit->buffer[Row * sbit->pitch + Pitch]) & 1<<Bit)
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

	PosX += desc.font.pix_width;
}

/******************************************************************************
 * RenderCharLCD                                                             *
 ******************************************************************************/

void RenderCharLCD(int Digit, int XPos, int YPos)
{
	int x, y;

	//render digit to lcd backbuffer

		for(y = 0; y < 15; y++)
		{
			for(x = 0; x < 10; x++)
			{
				if(lcd_digits[Digit*15*10 + x + y*10]) lcd_backbuffer[XPos + x + ((YPos+y)/8)*120] |= 1 << ((YPos+y)%8);
				else								   lcd_backbuffer[XPos + x + ((YPos+y)/8)*120] &= ~(1 << ((YPos+y)%8));
			}
		}
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

		PosX = StartX + desc.font.pix_width+5;
		PosY = StartY + fixfontheight*16;
		for(byte = 0; byte < 37; byte++)
		{
			RenderCharFB(message_1[byte], menucolor<<4 | menu2);
		}
		RenderCharFB(message_1[37], fbcolor<<4 | menu2);

		PosX = StartX + desc.font.pix_width+5;
		PosY = StartY + fixfontheight*17;
		RenderCharFB(message_2[0], menucolor<<4 | menu2);
		for(byte = 1; byte < 36; byte++)
		{
			RenderCharFB(message_2[byte], menucolor<<4 | white);
		}
		RenderCharFB(message_2[36], menucolor<<4 | menu2);
		RenderCharFB(message_2[37], fbcolor<<4 | menu2);

		PosX = StartX + desc.font.pix_width+5;
		PosY = StartY + fixfontheight*18;
		RenderCharFB(message_3[0], menucolor<<4 | menu2);
		for(byte = 1; byte < 36; byte++)
		{
			RenderCharFB(message_3[byte], menucolor<<4 | white);
		}
		RenderCharFB(message_3[36], menucolor<<4 | menu2);
		RenderCharFB(message_3[37], fbcolor<<4 | menu2);

		PosX = StartX + desc.font.pix_width+5;
		PosY = StartY + fixfontheight*19;
		RenderCharFB(message_4[0], menucolor<<4 | menu2);
		for(byte = 1; byte < 36; byte++)
		{
			RenderCharFB(message_4[byte], menucolor<<4 | white);
		}
		RenderCharFB(message_4[36], menucolor<<4 | menu2);
		RenderCharFB(message_4[37], fbcolor<<4 | menu2);

		PosX = StartX + desc.font.pix_width+5;
		PosY = StartY + fixfontheight*20;
		for(byte = 0; byte < 37; byte++)
		{
			RenderCharFB(message_5[byte], menucolor<<4 | menu2);
		}
		RenderCharFB(message_5[37], fbcolor<<4 | menu2);

		PosX = StartX + desc.font.pix_width+5;
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

	//update lcd

		UpdateLCD();

	//update page or timestring

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

				PosX = StartX + 32*desc.font.pix_width;
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
	char line25_1[] = "   ?00<      ??0<      >??0      >?00   ((((((((((1111111111AAAAAAAAAAXXXXXXXXXX";
	char line25_2[] = " ïð w{hlen   ñò anzeigen   óô abbrechen ¤¨¨¤¤¤¤¤¤¤¤¤¤¨¨¤¤¤¤¤¤¤¤¤¤¤¤¨¨¤¤¤¤¤¤¤¤¤¤¤";

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

		line25_1[3] = (prev_100 >> 8) | '0';

	//get next 100th

		next_100 = page & 0xF00;

		do
		{
			next_100 += 0x100;

			if(next_100 == 0x900) next_100 = 0x100;
		}
		while(subpagetable[next_100] == 0xFF);

		line25_1[34] = (next_100 >> 8) | '0';

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

		line25_1[13] = (prev_10 >> 8) | '0';
		line25_1[14] = ((prev_10 & 0x0F0)>>4) | '0';

	//get next 10th

		next_10 = page & 0xFF0;

		do
		{
			if((next_10 & 0x0F0) == 0x090) next_10 += 0x70;
			else						   next_10 += 0x10;

			if(next_10 >= 0x900)		   next_10 = 0x100;
		}
		while(subpagetable[next_10] == 0xFF);

		line25_1[24] = (next_10 >> 8) | '0';
		line25_1[25] = ((next_10 & 0x0F0)>>4) | '0';

	//render line 25

		PosX = StartX;
		PosY = StartY + 24*fixfontheight;

		for(byte = 0; byte < 40; byte++)
		{
			if(boxed)			  RenderCharBB(' ', transp<<4 | transp);
			else if(pagecatching) RenderCharBB(line25_2[byte], line25_2[byte + 40]);
			else				  RenderCharBB(line25_1[byte], line25_1[byte + 40]);
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
 * UpdateLCD                                                                  *
 ******************************************************************************/

void UpdateLCD()
{
	static int init_lcd = 1, old_cached_pages = -1, old_page = -1, old_subpage = -1, old_subpage_max = -1, old_hintmode = -1;
	int  x, y, subpage_max = 0, update_lcd = 0;

	//init or update lcd

		if(init_lcd)
		{
			init_lcd = 0;

			for(y = 0; y < 64; y++)
			{
				for(x = 0; x < 120; x++)
				{
					if(lcd_layout[x + y*120]) lcd_backbuffer[x + (y/8)*120] |= 1 << (y%8);
					else					  lcd_backbuffer[x + (y/8)*120] &= ~(1 << (y%8));
				}
			}

			write(lcd, &lcd_backbuffer, sizeof(lcd_backbuffer));

			for(y = 15; y <= 58; y++)
			{
				for(x = 1; x < 118; x++)
				{
					lcd_backbuffer[x + (y/8)*120] &= ~(1 << (y%8));
				}
			}

			for(x = 3; x <= 116; x++)
			{
				lcd_backbuffer[x + (39/8)*120] |= 1 << (39%8);
			}

			for(y = 42; y <= 60; y++)
			{
				lcd_backbuffer[35 + (y/8)*120] |= 1 << (y%8);
			}

			for(y = 42; y <= 60; y++)
			{
				lcd_backbuffer[60 + (y/8)*120] |= 1 << (y%8);
			}

			RenderCharLCD(10, 43, 20);
			RenderCharLCD(11, 79, 20);

			return;
		}
		else
		{
			//page

				if(old_page != page)
				{
					RenderCharLCD(page>>8,  7, 20);
					RenderCharLCD((page&0x0F0)>>4, 19, 20);
					RenderCharLCD(page&0x00F, 31, 20);

					old_page = page;
					update_lcd = 1;
				}

			//current subpage

				if(old_subpage != subpage)
				{
					RenderCharLCD(subpage>>4,  55, 20);
					RenderCharLCD(subpage&0x0F,  67, 20);

					old_subpage = subpage;
					update_lcd = 1;
				}

			//max subpage

				for(x = 0; x <= 0x79; x++)
				{
					if(cachetable[page][x] != 0) subpage_max = x;
				}

				if(old_subpage_max != subpage_max)
				{
					RenderCharLCD(subpage_max>>4,  91, 20);
					RenderCharLCD(subpage_max&0x0F, 103, 20);

					old_subpage_max = subpage_max;
					update_lcd = 1;
				}

			//cachestatus

				if(old_cached_pages != cached_pages)
				{
					RenderCharLCD(cached_pages/1000, 67, 44);
					RenderCharLCD(cached_pages%1000/100, 79, 44);
					RenderCharLCD(cached_pages%100/10, 91, 44);
					RenderCharLCD(cached_pages%10, 103, 44);

					old_cached_pages = cached_pages;
					update_lcd = 1;
				}

			//mode

				if(old_hintmode != hintmode)
				{
					if(hintmode) RenderCharLCD(12, 43, 44);
					else		 RenderCharLCD(13, 43, 44);

					old_hintmode = hintmode;
					update_lcd = 1;
				}
		}

		if(update_lcd)
		{
			//printf("TuxTxt <update LCD: %.3x-%.2x/%.2x %.2d %.1d %.4d>\n", page, subpage, subpage_max, pids_found, hintmode, cached_pages);

			write(lcd, &lcd_backbuffer, sizeof(lcd_backbuffer));
		}
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
							continue;
						}

						packet_number = b1>>3 | b2<<1;

					//analyze row

						if(packet_number == 0)
						{
							//check parity

								for(byte = 14; byte <= 45; byte++)
								{
									if((vtxt_row[byte]&1) ^ ((vtxt_row[byte]>>1)&1) ^ ((vtxt_row[byte]>>2)&1) ^ ((vtxt_row[byte]>>3)&1) ^ ((vtxt_row[byte]>>4)&1) ^ ((vtxt_row[byte]>>5)&1) ^ ((vtxt_row[byte]>>6)&1) ^ (vtxt_row[byte]>>7)) vtxt_row[byte] &= 127;
									else vtxt_row[byte] = ' ';
								}

							//get pagenumber

								b1 = dehamming[vtxt_row[4]] & 7;
								b2 = dehamming[vtxt_row[7]];
								b3 = dehamming[vtxt_row[6]];

								if(b1 == 0xFF || b2 == 0xFF || b3 == 0xFF)
								{
									current_page = -1;
									printf("TuxTxt <Biterror in Page>\n");
									continue;
								}

								if(b2 > 9 || b3 > 9)
								{
									current_page = -1;
									continue;
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
									continue;
								}

								if(b1 != 0 || b2 != 0 || b3 > 7 || b4 > 9)
								{
									current_subpage = -1;
									continue;
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
							//check parity

								for(byte = 6; byte <= 45; byte++)
								{
									if((vtxt_row[byte]&1) ^ ((vtxt_row[byte]>>1)&1) ^ ((vtxt_row[byte]>>2)&1) ^ ((vtxt_row[byte]>>3)&1) ^ ((vtxt_row[byte]>>4)&1) ^ ((vtxt_row[byte]>>5)&1) ^ ((vtxt_row[byte]>>6)&1) ^ (vtxt_row[byte]>>7)) vtxt_row[byte] &= 127;
									else vtxt_row[byte] = ' ';
								}
						}

					//copy row to pagebuffer

						if(current_page != -1 && current_subpage != -1 && packet_number < 24)
						{
							memcpy(cachetable[current_page][current_subpage] + packet_number*40, &vtxt_row[6], 40);
						}
				}
			}
	}

	return 0;
}
