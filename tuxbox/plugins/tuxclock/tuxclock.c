/**
  tuxclock
  ========
  Version: 0.01 alpha by Blazej Bartyzel "blesb", at 2009-05-28
  Print page format: landscape
  program construct is based on TuxMail vom Thomas "LazyT" 2005

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  If this software will be used in commertial products and source code are
  not included or not offer for customer, please contact FSF or me over
  forum http://tuxbox-forum.dreambox-fan.de/forum/.

  Some assumption:
  ----------------
   Value 255 of FB colortab is always r=0x0000 g=0x0000 b=0x0000 t=0xFFFF
   I use FB control from TuxCal daemon and hope it havent errors ;-)
   all return value of function are set unix-like: zero all ok, non zero some error
   all config parameter are number (speed and simplicity of code)

  Program detail:
  ---------------

 (Future) functions:
  (none)

 ToDo & Ideas
  - ? autostart row in menu => set and delete /var/etc/.tuxclock
  - editline for number
  - screensaver

 Open problems:
  - use search function for find and user total transparency color (?255)
*/

#include "tuxclock.h"


/******************************************************************************
 errorlog
  write error log messages
 \param  (int) message number
 \return : none
 ******************************************************************************/
void errorlog(int num)
{
   slog ? syslog(LOG_USER | LOG_INFO,"error %d: %s", num, errormsg[num])
        : printf("TuxClock error %d: %s\n", num, errormsg[num]);
}


/******************************************************************************
 ReadConf
  read configuration-file. quite same as into tuxclockd
 \param  : none
 \return :
   0 = config file is readed and all parameter are correct
   1 = config file is not found or any parameter are missing or wrong (write config file)
 ******************************************************************************/
int ReadConf()
{
   /* standard config file /var/tuxbox/config/tuxclock.conf
    SHOWCLOCK=1
    STARTDELAY=45
    POS_X=600
    POS_Y=40
    INTERVALL=1
    SIZE=28
    COLOR=16
    BGROUND=0
    FORMAT=1
    DATE=0
   */

   FILE *fd_conf;
   char *ptr;                // string positions pointer
   int  retval = 0;          // predef of return value - all are ok.
   char line_buffer[256];

   // open config-file
   if (!(fd_conf = fopen(CFGPATH CFGFILE, "r"))) {
      errorlog(9);
      // set standard values,see tuxclockd.h
      show_clock   = SHOWCLOCKNORM;
      startdelay   = STARTDELAYNORM;
      start_x      = STARTXNORM;
      start_y      = STARTYNORM;
      intervall    = INTERVALLNORM;
      char_size    = CHARSIZENORM;
      disp_format  = FORMATNORM;
      disp_date    = SHOWDATENORM;
      return 1;
   }

   // set all parameter variables to undefined (-1)
   show_clock = startdelay = start_x = start_y = intervall =
   char_size = char_color = disp_format = disp_date = -1;
   // bgcolor have undefined as -2 (-1: dont display/use)
   char_bgcolor = -2;

   // read config file line-by-line
   while(fgets(line_buffer, sizeof(line_buffer), fd_conf)) {
      if((ptr = strstr(line_buffer, "SHOWCLOCK="))) {            // 1: begin clock display with deamon start
         sscanf(ptr + 10, "%d", &show_clock);
      }
      else if ((ptr = strstr(line_buffer, "STARTDELAY="))) {     // (sec) deamon can delay start of clock display
         sscanf(ptr + 11, "%d", &startdelay);
      }
      else if((ptr = strstr(line_buffer, "POS_X="))) {           // X-axis start position
         sscanf(ptr + 6, "%d", &start_x);
      }
      else if((ptr = strstr(line_buffer, "POS_Y="))) {           // Y-axis start position
         sscanf(ptr + 6, "%d", &start_y);
      }
      else if((ptr = strstr(line_buffer, "INTERVALL="))) {       // update interval in seconds
         sscanf(ptr + 10, "%d", &intervall);
      }
      else if((ptr = strstr(line_buffer, "SIZE="))) {            // FONTSIZE (small 24)
         sscanf(ptr + 5, "%d", &char_size);
      }
      else if((ptr = strstr(line_buffer, "COLOR="))) {           // clock number color
         sscanf(ptr + 6, "%d", &char_color);
      }
      else if((ptr = strstr(line_buffer, "BGROUND="))) {         // background color
         sscanf(ptr + 8, "%d", &char_bgcolor);
      }
      else if((ptr = strstr(line_buffer, "FORMAT="))) {          // 0: "HH:MM", 1: "HH:MM:SS"
         sscanf(ptr + 7, "%d", &disp_format);
      }
      else if((ptr = strstr(line_buffer, "DATE="))) {            // future function for screensaver
         sscanf(ptr + 5, "%d", &disp_date);
      }
   }
   fclose(fd_conf); // close config-file

   // check config, see clockscrd.h for -MIN,-MAX and -NORM values
   // Attn.: char_color and charbgcolor hav'nt -NORM values - colortab
   //        must be serched for white and black values
   if ((show_clock != SHOWCLOCKOFF) && (show_clock != SHOWCLOCKON)) {
      show_clock = SHOWCLOCKNORM;
      retval = 1;
   }
   if ((startdelay < STARTDELAYMIN) || (startdelay > STARTDELAYMAX)) {
      startdelay = STARTDELAYNORM;
      retval = 1;
   }
   if ((start_x < STARTXMIN) || (start_x > STARTXMAX)) {
      start_x = STARTXNORM;
      retval = 1;
   }
   if ((start_y < STARTYMIN) || (start_y > STARTYMAX)) {
      start_y = STARTYNORM;
      retval = 1;
   }
   if ((intervall < INTERVALLMIN) || (intervall > INTERVALLMAX)) {
      intervall = INTERVALLNORM;
      retval = 1;
   }
   if ((char_size < CHARSIZEMIN) || (char_size > CHARSIZEMAX)) {
      char_size = CHARSIZENORM;
      retval = 1;
   }
   if ((char_color < CHARCOLORMIN) || (char_color > CHARCOLORMAX)) {
      char_color = -1;
      retval = 1;
   }
   if ((char_bgcolor < CHARBGCOLORMIN) || (char_bgcolor > CHARBGCOLORMAX)) {
      char_bgcolor = -1;
      retval = 1;
   }
   if ((disp_format != FORMATHHMM) && (disp_format != FORMATHHMMSS)) {
      disp_format = FORMATNORM;
      retval = 1;
   }
   if ((disp_date != SHOWDATEOFF) && (disp_date != SHOWDATEON)) {
      disp_date = SHOWDATENORM;
      retval = 1;
   }
   return retval;
}


/******************************************************************************
 WriteConf
  write the configuration back to the file
 \param  : none
 \return : 0: open file to write was successly, else 1
 ******************************************************************************/
int WriteConf()
{
   FILE *fd_conf;

   // open config-file
   if (!(fd_conf = fopen(CFGPATH CFGFILE , "w"))) {
      return 1;
   }
   fprintf(fd_conf, "SHOWCLOCK=%d\n",  show_clock);
   fprintf(fd_conf, "STARTDELAY=%d\n", startdelay);
   fprintf(fd_conf, "POS_X=%d\n",      start_x);
   fprintf(fd_conf, "POS_Y=%d\n",      start_y);
   fprintf(fd_conf, "INTERVALL=%d\n",  intervall);
   fprintf(fd_conf, "SIZE=%d\n",       char_size);
   fprintf(fd_conf, "COLOR=%d\n",      char_color);
   fprintf(fd_conf, "BGROUND=%d\n",    char_bgcolor);
   fprintf(fd_conf, "FORMAT=%d\n",     disp_format);
   fprintf(fd_conf, "DATE=%d\n",       disp_date);
   fclose(fd_conf);
   return 0;
}


/******************************************************************************
 MyFaceRequester
  load font
 \param   face_id     : FTC_FaceID
 \param library       : FT_Library
 \param request_data  : FT_Pointer
 \param afacs         : FT_Face*
 \return              : FT_Error
 ******************************************************************************/
FT_Error MyFaceRequester(FTC_FaceID face_id, FT_Library library, FT_Pointer request_data, FT_Face *aface)
{
   FT_Error result = FT_New_Face(library, face_id, 0, aface);
   if (result) {
      errorlog(11);
   }
   return result;
}


/******************************************************************************
 OpenFB
  opens the connection to the framebuffer
 \param  fbdev : frame buffer
 \return       : 0: FB ready, else open FB is failed (return value is error code)
 ******************************************************************************/
int OpenFB(int fbdev)
{
   FT_Error error;

   // init framebuffer
   if (ioctl(fbdev, FBIOGET_FSCREENINFO, &fix_screeninfo)<0) {
      return 2;
   }
   if (ioctl(fbdev, FBIOGET_VSCREENINFO, &var_screeninfo)<0) {
      return 3;
   }
   if (ioctl(fbdev, FBIOPUTCMAP, (skin == 1) ? &colormap1 : &colormap2) == -1) {
      return 14;
   }
   if (!(lfb = (unsigned char*)mmap(0, fix_screeninfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbdev, 0))) {
      return 4;
   }
   // init fontlibrary
   if ((error = FT_Init_FreeType(&library))) {
      munmap(lfb, fix_screeninfo.smem_len);
      return 5;
   }
   if ((error = FTC_Manager_New(library, 1, 2, 0, &MyFaceRequester, NULL, &manager))) {
      FT_Done_FreeType(library);
      munmap(lfb, fix_screeninfo.smem_len);
      return 6;
   }
   if ((error = FTC_SBitCache_New(manager, &cache))) {
      FTC_Manager_Done(manager);
      FT_Done_FreeType(library);
      munmap(lfb, fix_screeninfo.smem_len);
      return 7;
   }
   if ((error = FTC_Manager_Lookup_Face(manager, FONT, &face))) {
      FTC_Manager_Done(manager);
      FT_Done_FreeType(library);
      munmap(lfb, fix_screeninfo.smem_len);
      return 8;
   }
   use_kerning = FT_HAS_KERNING(face);
   desc.font.face_id = FONT;
   // oder ifdef OLDFT
#if FREETYPE_MAJOR  == 2 && FREETYPE_MINOR == 0
   desc.type = ftc_image_mono;
#else
   desc.flags = FT_LOAD_MONOCHROME;
#endif
   return 0;                                                     // all initialized ok
}


/******************************************************************************
 ControlDaemon
  we do have a connection to a daemon, this daemon will signal an alarm
 \param   command  : different command-codes, see tuxclock.h for enum field
 \return           : 0:OK, FAILED  1: socket failed, 2: connection failed
 ******************************************************************************/
int ControlDaemon(int command)
{
   int       fd_sock;                                            // socket to daemon
   struct    sockaddr_un srvaddr;
   socklen_t addrlen;

   // setup connection
   srvaddr.sun_family = AF_UNIX;
   strcpy(srvaddr.sun_path, SCKFILE);
   addrlen = sizeof(srvaddr.sun_family) + strlen(srvaddr.sun_path);

   // open socket
   if ((fd_sock = socket(PF_UNIX, SOCK_STREAM, 0)) == -1) {
      errorlog(16);
      return 1;
   }

   // connect socket
   if (connect(fd_sock, (struct sockaddr*)&srvaddr, addrlen) == -1) {
      errorlog(17);
      close(fd_sock);
      return 2;
   }

   // send a command and hope for an answer. time out?
   switch (command) {
      case GET_STATUS:
         send(fd_sock, "G", 1, 0);
         recv(fd_sock, &visible, 1, 0);
         break;
      case GET_ID:
         send(fd_sock, "I", 1, 0);
         recv(fd_sock, &versioninfo_d, sizeof(versioninfo_d), 0);
         break;
      case SET_HIDDEN:
         send(fd_sock, "H", 1, 0);
         break;
      case SET_VISIBLE:
         send(fd_sock, "V", 1, 0);
         break;
      case SET_RESTART:
         send(fd_sock, "R", 1, 0);
         break;
   }
   // close connection
   close(fd_sock);
   return 0;
}


/******************************************************************************
 GetRCCode (is coming form tuxmail)
  read key from remote control
 \param   : none
 \return  : rccode or -1 is key is'nt pressed
 ******************************************************************************/
#if HAVE_DVB_API_VERSION == 3
int GetRCCode()
{
   static __u16 rc_last_key = KEY_RESERVED;

   if (read(rc, &ev, sizeof(ev)) == sizeof(ev)) {
      if (ev.value) {
         if (ev.code != rc_last_key) {
            rc_last_key = ev.code;
            switch (ev.code) {
               case KEY_UP:         rccode = RC_UP;      break;
               case KEY_DOWN:       rccode = RC_DOWN;    break;
               case KEY_LEFT:       rccode = RC_LEFT;    break;
               case KEY_RIGHT:      rccode = RC_RIGHT;   break;
               case KEY_OK:         rccode = RC_OK;      break;
               case KEY_0:          rccode = RC_0;       break;
               case KEY_1:          rccode = RC_1;       break;
               case KEY_2:          rccode = RC_2;       break;
               case KEY_3:          rccode = RC_3;       break;
               case KEY_4:          rccode = RC_4;       break;
               case KEY_5:          rccode = RC_5;       break;
               case KEY_6:          rccode = RC_6;       break;
               case KEY_7:          rccode = RC_7;       break;
               case KEY_8:          rccode = RC_8;       break;
               case KEY_9:          rccode = RC_9;       break;
               case KEY_RED:        rccode = RC_RED;     break;
               case KEY_GREEN:      rccode = RC_GREEN;   break;
               case KEY_YELLOW:     rccode = RC_YELLOW;  break;
               case KEY_BLUE:       rccode = RC_BLUE;    break;
               case KEY_VOLUMEUP:   rccode = RC_PLUS;    break;
               case KEY_VOLUMEDOWN: rccode = RC_MINUS;   break;
               case KEY_MUTE:       rccode = RC_MUTE;    break;
               case KEY_HELP:       rccode = RC_HELP;    break;
               case KEY_SETUP:      rccode = RC_DBOX;    break;
               case KEY_HOME:       rccode = RC_HOME;    break;
               case KEY_POWER:      rccode = RC_STANDBY; break;
               default:             rccode = -1;
            }
         } else { rccode = -1; }
      } else {
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

   read(rc, &rccode, sizeof(rccode));
   if (rccode != LastKey) {
      LastKey = rccode;
      if ((rccode & 0xFF00) == 0x5C00) {
         switch (rccode) {
            case RC1_UP:      rccode = RC_UP;      break;
            case RC1_DOWN:    rccode = RC_DOWN;    break;
            case RC1_LEFT:    rccode = RC_LEFT;    break;
            case RC1_RIGHT:   rccode = RC_RIGHT;   break;
            case RC1_OK:      rccode = RC_OK;      break;
            case RC1_0:       rccode = RC_0;       break;
            case RC1_1:       rccode = RC_1;       break;
            case RC1_2:       rccode = RC_2;       break;
            case RC1_3:       rccode = RC_3;       break;
            case RC1_4:       rccode = RC_4;       break;
            case RC1_5:       rccode = RC_5;       break;
            case RC1_6:       rccode = RC_6;       break;
            case RC1_7:       rccode = RC_7;       break;
            case RC1_8:       rccode = RC_8;       break;
            case RC1_9:       rccode = RC_9;       break;
            case RC1_RED:     rccode = RC_RED;     break;
            case RC1_GREEN:   rccode = RC_GREEN;   break;
            case RC1_YELLOW:  rccode = RC_YELLOW;  break;
            case RC1_BLUE:    rccode = RC_BLUE;    break;
            case RC1_PLUS:    rccode = RC_PLUS;    break;
            case RC1_MINUS:   rccode = RC_MINUS;   break;
            case RC1_MUTE:    rccode = RC_MUTE;    break;
            case RC1_HELP:    rccode = RC_HELP;    break;
            case RC1_DBOX:    rccode = RC_DBOX;    break;
            case RC1_HOME:    rccode = RC_HOME;    break;
            case RC1_STANDBY: rccode = RC_STANDBY; break;
         }
      } else { rccode &= 0x003F; }
   } else { rccode = -1; }
   return rccode;
}
#endif


/******************************************************************************
 RenderChar
  render a character to the screen
 \param currentchar  : FT_ULong
 \param sx,sy,ex     : x,y position and size
 \param color        : color
 \return             : charwidth or error if number equal 0
 ******************************************************************************/
int RenderChar(FT_ULong currentchar, int sx, int sy, int ex, int color)
{
   int row, pitch, bit, x = 0, y = 0;
   FT_Error error;
   FT_UInt glyphindex;
   FT_Vector kerning;
   FTC_Node anode;

   if (!(glyphindex = FT_Get_Char_Index(face, currentchar))) {
      errorlog(12);
      return 0;
   }
   if ((error = FTC_SBitCache_Lookup(cache, &desc, glyphindex, &sbit, &anode))) {
      errorlog(13);
      return 0;
   }
   if (use_kerning) {
      FT_Get_Kerning(face, prev_glyphindex, glyphindex, ft_kerning_default, &kerning);
      prev_glyphindex = glyphindex;
      kerning.x >>= 6;
   } else {
     kerning.x = 0;
   }
   if (color != -1) {                                            // don't render char, return charwidth only
      if (sx + sbit->xadvance >= ex) {
         return -1;                                              // limit to maxwidth
      }
      for (row = 0; row < sbit->height; row++) {
         for (pitch = 0; pitch < sbit->pitch; pitch++) {
            for (bit = 7; bit >= 0; bit--) {
               if (pitch*8 + 7-bit >= sbit->width) {
                  break;                                         // render needed bits only
               }
               if ((sbit->buffer[row * sbit->pitch + pitch]) & 1<<bit) {
                  *(lbb+ startx+ sx+ sbit->left+ kerning.x+ x+ var_screeninfo.xres*(starty+ sy- sbit->top+ y)) = color;
               }
               x++;
            }
         }
         x = 0;
         y++;
      }
   }
   return sbit->xadvance + kerning.x;
}


/******************************************************************************
 PaintBox
  paint a box
 \param sx,sy,ex,ey  : x,y position and size
 \param mode         : box mode
 \param color        : color
 \return             : none
 ******************************************************************************/
void PaintBox(int sx, int sy, int ex, int ey, int mode, int color)
{
   int i;

   if (mode == FILL) {
      for (; sy <= ey; sy++) {
         memset(lbb + startx + sx + var_screeninfo.xres*(starty + sy), color, ex-sx + 1);
      }
   } else {
      for (i = sx; i <= ex; i++) {                               // hor line
         *(lbb+ startx+ i+ var_screeninfo.xres*(sy+starty))   = color;
         *(lbb+ startx+ i+ var_screeninfo.xres*(sy+1+starty)) = color;
         *(lbb+ startx+ i+ var_screeninfo.xres*(ey-1+starty)) = color;
         *(lbb+ startx+ i+ var_screeninfo.xres*(ey+starty))   = color;
      }
     for (i = sy; i <= ey; i++) {                                // ver line
        *(lbb+ startx+ sx+ var_screeninfo.xres*(i+starty))    = color;
        *(lbb+ startx+ sx+ 1+ var_screeninfo.xres*(i+starty)) = color;
        *(lbb+ startx+ ex- 1+ var_screeninfo.xres*(i+starty)) = color;
        *(lbb+ startx+ ex+ var_screeninfo.xres*(i+starty))    = color;
     }
   }
}


/******************************************************************************
 GetStringLen
  compute string pixel-length
 \param  string  : string pointer
 \return         : string legth in pixels
 ******************************************************************************/
int GetStringLen(unsigned char *string)
{
   int stringlen = 0;

   // reset kerning
   prev_glyphindex = 0;
   // calc len
   while (*string != '\0') {
      stringlen += RenderChar(*string, -1, -1, -1, -1);
      string++;
   }
   return stringlen;
}


/******************************************************************************
 WriteString
  simple render of a character string
 \param  string         : string pointer
 \param  sx,sy,maxwith  : x,y destiantion position and maximal size
 \param  color          : string color
 \return                : none
 attn: size of character is fix constant
 ******************************************************************************/
void WriteString(unsigned char *string, int sx, int sy, int maxwidth, int layout, int color)
{
   int ex, charwidth;

   desc.font.pix_width = desc.font.pix_height = TCCHARSIZE;
   prev_glyphindex = 0;
   ex = sx + maxwidth;
   if (layout == RIGHT ) {
      int stringlen = GetStringLen(string);
      if (stringlen < maxwidth) {
         sx += maxwidth - stringlen -2;
      }
   }
   while (*string != '\0') {
      if ((charwidth = RenderChar(*string, sx, sy, ex, color)) == -1) { // string > maxwidth
         return;
      }
      sx += charwidth;
      string++;
   }
}


/******************************************************************************
 ShowMenu
  simple menu
 \param  iPos  : actually row position
 \return       : none
 ******************************************************************************/
void ShowMenu(int iPos)
{
/*
     +---------------------------------+
     | tuxclock Control                | mrows[0][lang]
     +---------------------------------+
     | Show clock                  OFF | mrows[1][lang]
     | X clock position            999 |
     | Y clock position            999 |
     | Character size               99 |
     | Char color                  999 |
     | BGround color               999 |
     | Clock type               HHMMSS |
     | Screesaver with date        YES |
     | Screensaver start now           | mrows[9][lang]
     +---------------------------------+
     | OK select, DBOX save, HOME exit | mrows[10][lang]
     +---------------------------------+
*/

   int  i;                                                       // counter
   int  iHStart  = 0;                                            // start col (210)
   int  iHSize   = 300;                                          // columns
   int  iHEnd    = iHStart+iHSize;                               // end col
   int  sy       = TCCHARSIZE+TCROWSIZE-5;                       // rows step
   char tmpstr[9];                                               // temporary var for integer conversion

   // paint window box body
   PaintBox( iHStart,             0, iHEnd, (TCROWSIZE*11-2), FILL, SKIN0);
   //  border
   PaintBox( iHStart,             0, iHEnd, (TCROWSIZE*11-2), GRID, SKIN2);
   //  inner border
   PaintBox( iHStart, (TCROWSIZE-4), iHEnd, (TCROWSIZE*10+2), GRID, SKIN2);
   //  highlighted row
   PaintBox( iHStart+4, (TCROWSIZE-1)+(iPos%10)*TCROWSIZE, iHEnd-3, (TCROWSIZE-2)*2+(iPos%10)*TCROWSIZE, FILL, SKIN2 );

   // paint plugin name
   WriteString( mrows[0][lang], iHStart+20, (TCCHARSIZE-3), iHSize-2, LEFT, ORANGE);
   // paint the rows
   int color_deamon = (socket_status == DAEMON_OFF) ? GREEN : (iPos == 0) ? YELLOW : WHITE;
   WriteString( mrows[1][lang], iHStart+8, sy, iHSize-16, LEFT, color_deamon );
   if (socket_status == DAEMON_OFF) {
      WriteString( deamonmsg[2][lang], iHStart+8, sy, iHSize-16, RIGHT, GREEN );
   } else {
      WriteString( deamonmsg[show_clock][lang], iHStart+8, sy, iHSize-16, RIGHT, color_deamon );
   }
   sy += TCROWSIZE;
   // print rows
   for (i = 2; i < 10; i++) {
      WriteString( mrows[i][lang], iHStart+8, sy, iHSize-16, LEFT, ((i-1) == iPos) ? YELLOW : WHITE );
      switch (i) {
         case 2:     // X clock position            999
            {
               sprintf(tmpstr,"%u",start_x);
               WriteString(tmpstr, iHStart+8, sy, iHSize-16, RIGHT, ((i-1) == iPos) ? YELLOW : WHITE );
            }
            break;
         case 3:     // Y clock position            999
            {
               sprintf(tmpstr,"%u",start_y);
               WriteString(tmpstr, iHStart+8, sy, iHSize-16, RIGHT, ((i-1) == iPos) ? YELLOW : WHITE );
            }
            break;
         case 4:     // Character size               99
            {
               sprintf(tmpstr,"%u",char_size);
               WriteString(tmpstr, iHStart+8, sy, iHSize-16, RIGHT, ((i-1) == iPos) ? YELLOW : WHITE );
            }
            break;
         case 5:     // Char color                  999
            {
               sprintf(tmpstr,"%u",char_color);
               WriteString(tmpstr, iHStart+8, sy, iHSize-16, RIGHT, ((i-1) == iPos) ? YELLOW : WHITE );
            }
            break;
         case 6:     // BGround color               999
            {
               sprintf(tmpstr,"%u",char_bgcolor);
               WriteString(tmpstr, iHStart+8, sy, iHSize-16, RIGHT, ((i-1) == iPos) ? YELLOW : WHITE );
            }
            break;
         case 7:     // Clock type               HHMMSS
            WriteString( timefmt[disp_format], iHStart+8, sy, iHSize-16, RIGHT, ((i-1) == iPos) ? YELLOW : WHITE );
            break;
         case 8:     // Screesaver with date        YES
            WriteString(yesno[disp_date][lang], iHStart+8, sy, iHSize-16, RIGHT, ((i-1) == iPos) ? YELLOW : WHITE );
            break;
      }
      sy += TCROWSIZE;
   }
   // paint status row
   WriteString(mrows[10][0], iHStart+8, sy, iHSize-16, LEFT, RED );
   // copy all from bufer to screen
   memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
}


/******************************************************************************
 pOK
  work function for OK Key
 \param  iPos  : row position
 \return       : none
 ******************************************************************************/
void pOK(int iPos)
{
   switch (iPos) {
      case 0:
         // show clock on/off
         if (socket_status == DAEMON_ON) {
            show_clock = (show_clock) ? 0 : 1;
         }
         break;
      case 1:
         // set x clock pos
         break;
      case 2:
         // set y clock pos
         break;
      case 3:
         // set char size
         break;
      case 4:
         // set text color
         break;
      case 5:
         // set background color
         break;
      case 6:
         // set clock type HHMMSS HHMM
         disp_format = (disp_format) ? 0 : 1;                    // toggle status
         break;
      case 7:
         // set screensaver with date
         disp_date = (disp_date) ? 0 : 1;                        // toggle status
         break;
      case 8:
         // start screensaver
         // ************* yet not implemented ;-) comming soon :-)
         break;
   }
   return;
}


/******************************************************************************
 plugin_exec
  main function of plugin
 \param  par  : plugins parameter
 \return      : none
 ******************************************************************************/
void plugin_exec(PluginParam *par) {
   int iPos;                                                     // menu position
   int tmp;                                                      // temporary variable

   fb = rc = sx = ex = sy = ey = -1;
   for (; par; par = par->next) {
      if (!strcmp(par->id, P_ID_FBUFFER))      { fb = atoi(par->val); }
      else if (!strcmp(par->id, P_ID_RCINPUT)) { rc = atoi(par->val); }
      else if (!strcmp(par->id, P_ID_OFF_X))   { sx = atoi(par->val); }
      else if (!strcmp(par->id, P_ID_END_X))   { ex = atoi(par->val); }
      else if (!strcmp(par->id, P_ID_OFF_Y))   { sy = atoi(par->val); }
      else if (!strcmp(par->id, P_ID_END_Y))   { ey = atoi(par->val); }
   }
   if (fb == -1 || rc == -1 || sx == -1 || ex == -1 || sy == -1 || ey == -1) {
      errorlog(1);
      return;
   }
   if (slog == 1) {
      openlog("tuxclock", LOG_ODELAY, LOG_USER);
   }
   slog ? syslog(LOG_USER | LOG_INFO,"%s", errormsg[0])
        : printf("%s\n", errormsg[0]);
   // create FB
   tmp = OpenFB(fb);                                             // result of OpenFB: 0 all ok, else error
   if (tmp) {
      errorlog(tmp);
      return;
   }
   if (!(lbb = malloc(var_screeninfo.xres*var_screeninfo.yres))) {
      errorlog(15);
      FTC_Manager_Done(manager);
      FT_Done_FreeType(library);
      munmap(lfb, fix_screeninfo.smem_len);
      return;
   }

   // initialize buffer with transparency color (i hope it)
   memset(lbb, 255, var_screeninfo.xres*var_screeninfo.yres-1);
   // set start of window corner
   startx = sx+40;
   starty = sy+40;

   // read, create (if file not exist) or update (if any param. not exist) config file
   if (ReadConf()) {
      if (WriteConf()) {
         errorlog(10);
      }
   }

   // get daemon status
   if (ControlDaemon(GET_STATUS)) {                              // Communication to daemon failed
      socket_status = DAEMON_OFF;
   }

   // make clock invisible - plugin change char table
   if (socket_status == DAEMON_ON) {
      ControlDaemon(SET_HIDDEN);
   }

#if HAVE_DVB_API_VERSION == 3
   read(rc, &ev, sizeof(ev));
#else
   read(rc, &rccode, sizeof(rccode));
#endif
   fcntl(rc, F_SETFL, fcntl(rc, F_GETFL) &~ O_NONBLOCK);

   // program loop
   iPos = 0;
   tmp  = 0;                                                     // now: 0 - no RESTART signal
   do {
      ShowMenu(iPos);                                            // show menu
      switch ((rccode = GetRCCode())) {                          // read key
         case RC_UP:
            if (iPos > 0) iPos--;
            break;
         case RC_DOWN:
            if (iPos < 8) iPos++;
            break;
         case RC_OK:
            pOK(iPos);
            break;
         case RC_HELP:                                           // change menu language
            lang = (lang) ? 0 : 1;
            break;
         case RC_DBOX:                                           // save config file
            // ?? future: set message on screen
            if (WriteConf()) {
               errorlog(10);
            } else {
               tmp = 1;                                          // restart daemon after loop
            }
            break;
         case RC_HOME:                                           // only exit
            break;
         case RC_PLUS:                                           // increment or toggle
         case RC_MINUS:                                          // decrement or toggle
            switch (iPos) {
               case 0:
                  // show clock on/off
                  if (socket_status == DAEMON_ON) {
                     show_clock = (show_clock) ? 0 : 1;
                  }
                  break;
               case 1:
                  // set x clock pos
                  if (rccode == RC_PLUS) {
                     if (start_x < STARTXMAX) start_x++;
                  } else {
                     if (start_x > STARTXMIN) start_x--;
                  }
                  break;
               case 2:
                  // set y clock pos
                  if (rccode == RC_PLUS) {
                     if (start_y < STARTYMAX) start_y++;
                  } else {
                     if (start_y > STARTYMIN) start_y--;
                  }
                  break;
               case 3:
                  // set char size
                  if (rccode == RC_PLUS) {
                     if (char_size < CHARSIZEMAX) char_size++;
                  } else {
                     if (char_size > CHARSIZEMIN) char_size--;
                  }
                  break;
               case 4:
                  // set text color
                  if (rccode == RC_PLUS) {
                     if (char_color < CHARCOLORMAX) char_color++;
                  } else {
                     if (char_color > CHARCOLORMIN) char_color--;
                  }
                  break;
               case 5:
                  // set background color
                  if (rccode == RC_PLUS) {
                     if (char_bgcolor < CHARBGCOLORMAX) char_bgcolor++;
                  } else {
                     if (char_bgcolor > CHARBGCOLORMIN) char_bgcolor--;
                  }
                  break;
               case 6:
                  // set clock type HHMMSS HHMM
                  disp_format = (disp_format) ? 0 : 1;           // toggle status
                  break;
               case 7:
                  // set screensaver with date
                  disp_date = (disp_date) ? 0 : 1;               // toggle status
                  break;
            }
            break;
         default:                                                // other keys
            continue;
      }
   } while (rccode != RC_HOME);                                  // end program loop

   // if tmp then restart/reload conf into daemon
   if ((socket_status == DAEMON_ON) && (tmp)) {
      ControlDaemon(SET_RESTART);
   }
   // set 
   if ((socket_status == DAEMON_ON) && (show_clock)) {
      ControlDaemon(SET_VISIBLE);
   }

   // unlink(LCKFILE);
   FTC_Manager_Done(manager);
   FT_Done_FreeType(library);
   free(lbb);
   munmap(lfb, fix_screeninfo.smem_len);
   fcntl(rc, F_SETFL, O_NONBLOCK);
   if (slog == 1) {
      closelog();
   }
   return;
}
