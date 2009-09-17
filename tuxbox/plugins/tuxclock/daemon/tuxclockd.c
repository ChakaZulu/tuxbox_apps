/**
  tuxclockd
  =========
  Version: 0.03 by Blazej Bartyzel "blesb", at 2009-05-27
  Print page format: landscape
  program construct is based on TuxCal daemon rev 1.10, Robert "robspr1" Spreitzer 2006

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
   main loop is designed short and quick if possible. all parameter compute async. and extern main loop
   tuxclockd daemon, can automatically started from rcS on system start
    if [ -e /var/etc/.tuxclockd ]; then
       tuxclockd &
    fi
   tuxclockd can be over signals controlled. For example is tuxclockd pid xyz
    disable clock window temporary: kill -USR2 xyz
    enable clock window: kill -USR1 xyz
    terminate daemon: kill -TERM xyz (or simply kill xyz)
   over config file parameter STARTDELAY can be clock display delayed. On my Squash-Image best 45 sec
   clock display have two formats "HH:MM:SS" and "HH:MM" (config file paramter FORMAT)
   clock display can be variable refreshed (config file paramter INTERVALL)
   if config file or COLOR or BGROUND are not defined will daemon check the colortab for white and black values
   if check of colortab will duing, can be the colortab to console displaed (start option -colortab)

   Possible error code on console or in log file:
    01: "open FB failed"
    02: "FBIOGET_VSCREENINFO init error"
    03: "FBIOGET_FSCREENINFO init error"
    04: "mapping of FB failed"
    05: "FT_Init_FreeType failed"
    06: "FTC_Manager_New failed"
    07: "FTC_SBitCache_New failed"
    08: "FTC_Manager_Lookup_Face failed"
    09: "config not found, using defaults"
    10: "config file write error"
    11: "daemon is aborted"
    12: "daemon already running"
    13: "install of TERM handler failed"
    14: "install of USR1 handler failed"
    15: "install of USR2 handler failed"
    16: "install of HUP handler failed"    *)
    17: "install of ALRM handler failed"   *)
    18: "interface thread failed"
    19: "interface socket failed"
    20: "interface bind failed"
    21: "interface listen failed"
    22: "interface accept failed"
    23: "font load is failed"
    24: "FT_Get_Char_Index undefined char"
    25: "FTC_SBitCache_Lookup failed"
    26: "FBIOGETCMAP failed"
    27: "from socket signal is reload of tuxclock.conf failed"
     *) for future propose only

 (Future) functions:
  (none)

 ToDo & Ideas
  - Screensave-Mode only over Control-Plugin
  - only if needed use socket for communication with control plugin

 Open problems:
  - use search function for find and user total transparency color (?255)
*/

#include "tuxclockd.h"


/******************************************************************************
 errorlog
  write error log messages
 \param  (int) message number
 \return : none
 ******************************************************************************/
void errorlog(int num)
{
   slog ? syslog(LOG_DAEMON | LOG_INFO,"error code %d", num)
        : printf("tuxclockd error code %d\n", num);
}


/******************************************************************************
 infolog
  write info log messages
 \param  (int) message number
 \return : none
 \comment: loginfos[] is defined in tuxclockd.h
 ******************************************************************************/
void infolog(int num)
{
   slog ? syslog(LOG_DAEMON | LOG_INFO,"%s", loginfos[num])
        : printf("tuxclockd: %s\n", loginfos[num]);
}


/******************************************************************************
 ReadConf
  read configuration-file
 \param  : none
 \return :
   0 = config file is readed and all parameter are correct
   1 = config file is not found
   2 = config file have any paramater not set or over range (write config file)
   3 =  ... char_color or char_bgcolor must be search
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
   char *ptr;                                                    // string positions pointer
   int  retval = 0;                                              // predef of return value - all are ok.
   char line_buffer[256];

   // open config-file
   if (!(fd_conf = fopen(CFGPATH CFGFILE, "r"))) {
      errorlog(9);
      // set standard values,see tuxclockd.h
      show_clock   = SHOWCLOCKNORM;
      startdelay   = STARTDELAYNORM;
      startx       = STARTXNORM;
      starty       = STARTYNORM;
      intervall    = INTERVALLNORM;
      char_size    = CHARSIZENORM;
      disp_format  = FORMATNORM;
      disp_date    = SHOWDATENORM;
      // char_color and char_bgcolor must be search (later)
      return 1;
   }

   // set all parameter variables to undefined (-1)
   show_clock = startdelay = startx = starty = intervall =
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
         sscanf(ptr + 6, "%d", &startx);
      }
      else if((ptr = strstr(line_buffer, "POS_Y="))) {           // Y-axis start position
         sscanf(ptr + 6, "%d", &starty);
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
      else if((ptr = strstr(line_buffer, "FORMAT="))) {               // 0: "HH:MM", 1: "HH:MM:SS"
         sscanf(ptr + 7, "%d", &disp_format);
      }
      else if((ptr = strstr(line_buffer, "DATE="))) {                 // future function for screensaver
         sscanf(ptr + 5, "%d", &disp_date);
      }
   }
   fclose(fd_conf); // close config-file

   // check config, see clockscrd.h for -MIN,-MAX and -NORM values
   // Attn.: char_color and charbgcolor hav'nt -NORM values - colortab
   //        must be serched for white and black values
   if ((show_clock != SHOWCLOCKOFF) && (show_clock != SHOWCLOCKON)) {
      show_clock = SHOWCLOCKNORM;
      retval = 2;
   }
   if ((startdelay < STARTDELAYMIN) || (startdelay > STARTDELAYMAX)) {
      startdelay = STARTDELAYNORM;
      retval = 2;
   }
   if ((startx < STARTXMIN) || (startx > STARTXMAX)) {
      startx = STARTXNORM;
      retval = 2;
   }
   if ((starty < STARTYMIN) || (starty > STARTYMAX)) {
      starty = STARTYNORM;
      retval = 2;
   }
   if ((intervall < INTERVALLMIN) || (intervall > INTERVALLMAX)) {
      intervall = INTERVALLNORM;
      retval = 2;
   }
   if ((char_size < CHARSIZEMIN) || (char_size > CHARSIZEMAX)) {
      char_size = CHARSIZENORM;
      retval = 2;
   }
   if ((char_color < CHARCOLORMIN) || (char_color > CHARCOLORMAX)) {
      char_color = -1;
      retval = 3;
   }
   if ((char_bgcolor < CHARBGCOLORMIN) || (char_bgcolor > CHARBGCOLORMAX)) {
      char_bgcolor = -1;
      retval = 3;
   }
   if ((disp_format != FORMATHHMM) && (disp_format != FORMATHHMMSS)) {
      disp_format = FORMATNORM;
      retval = 2;
   }
   if ((disp_date != SHOWDATEOFF) && (disp_date != SHOWDATEON)) {
      disp_date = SHOWDATENORM;
      retval = 2;
   }
   fb_color_set = (retval==3) ? -1 : 0;                          // if retval=3 search for white and black ist needed
   return retval;
}


/******************************************************************************
 * WriteConf
 * write the configuration back to the file
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
   fprintf(fd_conf, "POS_X=%d\n",      startx);
   fprintf(fd_conf, "POS_Y=%d\n",      starty);
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
 ClearScreen
  clear the framebuffer
 \param   : none
 \return  : none
 ******************************************************************************/
void ClearScreen(void)
{
   if (iFB) {
      int y;
      for (y=0; y <= char_size; y++) {
         memset(lfb+startx-2+var_screeninfo.xres*(starty+y), 255, clksize); // I hope 255 is std. tranparency index
      }
   }
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
      errorlog(23);
   }
   return result;
}


/******************************************************************************
 * OpenFB
 * opens the connection to the framebuffer
 \param   : none
 \return  : 0: FB ready, else open FB is failed (return value is error code)
 ******************************************************************************/
int OpenFB(void)
{
   FT_Error error;

   // framebuffer stuff
   if ((fbdev = open("/dev/fb/0", O_RDWR))<0) {
      return 1;
   }
   // init framebuffer
   if (ioctl(fbdev, FBIOGET_VSCREENINFO, &var_screeninfo)<0) {
      close(fbdev);
      return 2;
   }
   if (ioctl(fbdev, FBIOGET_FSCREENINFO, &fix_screeninfo)<0) {
      close(fbdev);
      return 3;
   }
   if (!(lfb = (unsigned char*)mmap(0, fix_screeninfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbdev, 0))) {
      close(fbdev);
      return 4;
   }
   // init fontlibrary
   if ((error = FT_Init_FreeType(&library))) {
      munmap(lfb, fix_screeninfo.smem_len);
      close(fbdev);
      return 5;
   }
   if ((error = FTC_Manager_New(library, 1, 2, 0, &MyFaceRequester, NULL, &manager))) {
      FT_Done_FreeType(library);
      munmap(lfb, fix_screeninfo.smem_len);
      close(fbdev);
      return 6;
   }
   if ((error = FTC_SBitCache_New(manager, &cache))) {
      FTC_Manager_Done(manager);
      FT_Done_FreeType(library);
      munmap(lfb, fix_screeninfo.smem_len);
      close(fbdev);
      return 7;
   }
   if ((error = FTC_Manager_Lookup_Face(manager, FONT, &face))) {
      FTC_Manager_Done(manager);
      FT_Done_FreeType(library);
      munmap(lfb, fix_screeninfo.smem_len);
      close(fbdev);
      return 8;
   }
   use_kerning = FT_HAS_KERNING(face);
   desc.font.face_id = FONT;
#if FREETYPE_MAJOR  == 2 && FREETYPE_MINOR == 0
   desc.type = ftc_image_mono;
#else
   desc.flags = FT_LOAD_MONOCHROME;
#endif
   if ((fb_color_set==-1)||(char_color=-1)||(char_bgcolor=-1)) {
      // search for black and white in FB colortab (same function FindColor() in tuxcal)
      // Definition and code from http://tuxbox-forum.dreambox-fan.de/forum/viewtopic.php?f=18&t=40377
      // Thanks to buribu for post and dietmarw for the link
      struct fb_cmap *cmap = NULL;
      char bps = var_screeninfo.bits_per_pixel;
      if (fix_screeninfo.visual==FB_VISUAL_PSEUDOCOLOR) {
         cmap=(struct fb_cmap*)malloc(sizeof(struct fb_cmap));
         cmap->red=(__u16*)malloc(sizeof(__u16)*(1<<bps));
         cmap->green=(__u16*)malloc(sizeof(__u16)*(1<<bps));
         cmap->blue=(__u16*)malloc(sizeof(__u16)*(1<<bps));
         cmap->transp=(__u16*)malloc(sizeof(__u16)*(1<<bps));
         cmap->start=0;
         cmap->len=1<<bps;
         if (ioctl(fbdev, FBIOGETCMAP, cmap)) {
            FTC_Manager_Done(manager);
            FT_Done_FreeType(library);
            munmap(lfb, fix_screeninfo.smem_len);
            close(fbdev);
            return 26;
         }
         // local variables
         int i, l;
         __u16 *pR, *pG, *pB, *pT;
         __u16 iWmax = 0;

         pR = cmap->red;
         pG = cmap->green;
         pB = cmap->blue;
         pT = cmap->transp;
         char_color   = -1;
         char_bgcolor = -1;
         l = cmap->len;
         if (print_colortab) {
            printf("colortab have %d values\n  index   red  green  blue   transp\n",l);
         }
         for (i=0; i<l; i++) {
            if (*pT==0) {                                        // serach only non-tramparent colors
               // find the first black
               if ((char_bgcolor==-1) && (*pR==0) && (*pG==0) && (*pB==0) && (*pT==0)) {
                  char_bgcolor = i;
               }
               // find "whitest" color
               if ((*pR==*pG) && (*pR==*pB) && (*pT==0)) {
                  if (*pR>iWmax) {
                     char_color = i;
                     iWmax      = *pR;
                  }
               }
            }
            if (print_colortab) {
               printf("    %03d  %04x   %04x  %04x     %04x\n",i,*pR,*pG,*pB,*pT);
            }
            pR++; pG++; pB++; pT++;
         }
         // free colormap
         if (cmap) {
            free(cmap->red);
            free(cmap->green);
            free(cmap->blue);
            free(cmap->transp);
            free(cmap);
            cmap=NULL;
         }
         if (WriteConf()) {
            errorlog(10);
         }
         if (print_colortab) {
            printf(" found black on %d and white(%04x) on %d\n",char_bgcolor,iWmax,char_color);
         }
         fb_color_set = 0;
      }
   }
   return 0;                                                     // all initialized ok
}


/******************************************************************************
 CloseFB
  close the connection to the framebuffer
 \param   : none
 \return  : none
 ******************************************************************************/
void CloseFB(void)
{
   // clear the framebuffer
   ClearScreen();
   if (iFB) {
      FTC_Manager_Done(manager);
      FT_Done_FreeType(library);
      munmap(lfb, fix_screeninfo.smem_len);
      close(fbdev);
      iFB = 0;
   }
}


/******************************************************************************
 RenderChar
  render a character to the screen
 \param currentchar  : FT_ULong
 \param cx           : char number in the string
 \param color        : color
 \return             : charwidth or error number
 ******************************************************************************/
int RenderChar(FT_ULong currentchar, int cx, int color)
{
   int x = 0, y = 0;
   FT_Error  error;
   FT_UInt   glyphindex;
   FT_Vector kerning;
   FTC_Node  anode;

   //load char
   if (!(glyphindex = FT_Get_Char_Index(face, currentchar))) {
      errorlog(24);
      return -2;
   }
   if ((error = FTC_SBitCache_Lookup(cache, &desc, glyphindex, &sbit, &anode))) {
      errorlog(25);
      return -2;
   }
   kerning.x = 3;                                                // only manually kerning use
   // render char
   if (color != -1) {                                            // -1: don't render char, return charwidth only
      int row, pitch, bit;
      for (row = 0; row < sbit->height; row++) {
         for (pitch = 0; pitch < sbit->pitch; pitch++) {
            for (bit = 7; bit >= 0; bit--) {
               if (pitch*8 + 7-bit >= sbit->width) {
                  break;                                         // render needed bits only
               }
               if ((sbit->buffer[row * sbit->pitch + pitch]) & 1<<bit) {
                  *(lfb + startx + cx + sbit->left + kerning.x + x + var_screeninfo.xres*(starty+char_size-6 - sbit->top + y)) = color;
               }
               x++;
            }
         }
         x = 0;
         y++;
      }
   }
   return sbit->xadvance + kerning.x;                            // return charwidth
}


/******************************************************************************
 ShowClock
  simple render a clock-string to the screen
  - string are left alignment
    and have global size, color and bgcolor (char_size, char_color, char_bgcolor)
 \param  : string pointer
 \return : none
 ******************************************************************************/
int ShowClock(unsigned char *string)
{
   int counter;

   // set size
   desc.font.pix_width = desc.font.pix_height = char_size;

   prev_glyphindex = 0;                                          // reset kerning
   // paint background
   if (char_bgcolor != -1) {
      for (counter=0; counter < char_size; counter++) {
         memset(lfb+startx-2+var_screeninfo.xres*(starty+counter), char_bgcolor, clksize);
      }
   }
   // render string
   counter = 0;
   while (*string != '\0') {
      counter += RenderChar(*string, counter, char_color);
      string++;
   }
   return counter; // return the last position
}


/******************************************************************************
 SigHandler
 ******************************************************************************/
void SigHandler(int signal)
{
   switch (signal) {
      case SIGTERM:                                              // clear FB and exit daemon
         show_clock = 0;
         ClearScreen();
         intervall = 0;
         break;
      case SIGUSR1:                                              // set clock display on
         show_clock = 1;
         infolog(2);
         break;
      case SIGUSR2:                                              // set clock display off
         show_clock = 0;
         ClearScreen();
         infolog(3);
         break;
   }
}


/******************************************************************************
 InterfaceThread
  communication-thread
 \param      arg  : argument for thread
 \return          : 1:OK   0:FAILED
 ******************************************************************************/
void *InterfaceThread(void *arg)
{
   int fd_sock, fd_conn;                                         // file for socket and connection
   struct sockaddr_un srvaddr;
   socklen_t addrlen;
   char command;

   // setup connection
   unlink(SCKFILE);

   srvaddr.sun_family = AF_UNIX;
   strcpy(srvaddr.sun_path, SCKFILE);
   addrlen = sizeof(srvaddr.sun_family) + strlen(srvaddr.sun_path);

   // open connection to socket-file
   if ((fd_sock = socket(PF_UNIX, SOCK_STREAM, 0)) == -1) {
      errorlog(19);
      return 0;
   }
   // bind to socket
   if (bind(fd_sock, (struct sockaddr*)&srvaddr, addrlen) == -1) {
      errorlog(20);
      return 0;
   }
   // listen to interface
   if (listen(fd_sock, 0) == -1) {
      errorlog(21);
      return 0;
   }

   // communication loop
   while (1) {
      // check if interface is ok
      if ((fd_conn = accept(fd_sock, (struct sockaddr*)&srvaddr, &addrlen)) == -1) {
         errorlog(22);
         continue;
      }
      recv(fd_conn, &command, 1, 0);                             // request one byte from plugin
      switch (command) {
         case 'G':                                               // command GET_STATUS
            send(fd_conn, &show_clock, 1, 0);
            break;
         case 'I':                                               // command GET_ID (Version)
            send(fd_conn, &versioninfo_d, 12, 0);
            break;
         case 'H':                                               // command SET_HIDDEN
            show_clock = 0;
            break;
         case 'V':                                               // command SET_VISIBLE
            show_clock = 1;
            break;
         case 'R':                                               // command SET_RESTART
            {
               int save_show_clock = show_clock;                 // only temporary
               if (show_clock) {
                  show_clock = 0;
                  sleep(intervall);
               }
               if (ReadConf()) {                                 // ?? someone was wrong
                  show_clock = save_show_clock;
                  errorlog(27);
               }
            }
            break;
      }
   }
   return 0;
}


/******************************************************************************
 Main Program
 ******************************************************************************/
int main(int argc, char **argv)
{
   char cvs_revision[] = "$Revision: 1.1 $";
   int nodelay = 0;
   pthread_t thread_id;
   void *thread_result = 0;

   sscanf(cvs_revision, "%*s %s", versioninfo_d);
   // check commandline parameter
   if (argc > 1) {
      int param;
      for (param = 1; param < argc; param++) {
         if (!strcmp(argv[param], "-nodelay")) {
            nodelay = 1;
         }
         else if (!strcmp(argv[param], "-colortab")) {
            print_colortab = 1;
         }
         else if (!strcmp(argv[param], "-syslog")) {
            slog = 1;
         }
         else if (!strcmp(argv[param], "-v")) {
            printf("%s\r\n",versioninfo_d);
            return 0;
         }
      }
   }
   if (slog == 1) {
      openlog("tuxclockd", LOG_ODELAY, LOG_DAEMON);
   }
   infolog(0);
   // create daemon
   switch (fork()) {
      case 0:
         setsid();
         chdir("/");
         break;
      case -1:
         errorlog(11);
         return -1;
      default:
         exit(0);
   }
   // check for running daemon
   if ((fd_pid = fopen(PIDFILE, "r+"))) {
      fscanf(fd_pid, "%d", &pid);
      if (kill(pid, 0) == -1 && errno == ESRCH) {
         pid = getpid();
         rewind(fd_pid);
         fprintf(fd_pid, "%d", pid);
         fclose(fd_pid);
      }
      else {
         errorlog(12);
         fclose(fd_pid);
         return -1;
      }
   }
   else {
      pid = getpid();
      fd_pid = fopen(PIDFILE, "w");
      fprintf(fd_pid, "%d", pid);
      fclose(fd_pid);
   }
   // install signal handler
   if (signal(SIGTERM, SigHandler) == SIG_ERR) {
      errorlog(13);
      return -1;
   }
   if (signal(SIGUSR1, SigHandler) == SIG_ERR) {
      errorlog(14);
      return -1;
   }
   if (signal(SIGUSR2, SigHandler) == SIG_ERR) {
      errorlog(15);
      return -1;
   }
   // install communication interface
   if (pthread_create(&thread_id, NULL, InterfaceThread, NULL)) {
      errorlog(18);
      return -1;
   }
   // read config file, if config file not exist - create it
   if (ReadConf()) {
      if (WriteConf()) {
         errorlog(10);
      }
   }
   // startdelay
   if ((startdelay>0) && (nodelay == 0)) {
      sleep(startdelay);
   }
   int erno = OpenFB();                                          // open connection to framebuffer
   if (erno) {                                                   // if error is coming on init
      errorlog(erno);
      show_clock = 0;                                            // no frame buffer connection - no clock ;-)
      iFB = 0;                                                   // Frame Buffer is not open
   }
   else {
      iFB = 1;                                                   // Frame Buffer is open/ready
   }

  // calculate size of "00:00:00" or "00:00" for background rectange
   int c1       = char_color;                                    // save colors
   int c2       = char_bgcolor;

   char_color   = 255;                                           // no paint test clock string
   char_bgcolor = -1;
   strcat(info,(disp_format)?"00:00:00":"00:00");
   clksize      = ShowClock(info)+4;                             // calculate window size
   char_color   = c1;                                            // restore colors
   char_bgcolor = c2;

   // main loop
   do {
      if (show_clock) {                                          // check and display clock only if online
         time(&tt);                                              // read the actual time
         at = localtime(&tt);
         strftime(info,MAXCLKLEN,clkfmt[disp_format],at);
         ShowClock(info);
      }
      sleep(intervall);
   } while (intervall);                                          // Signal TERM set intervall value to zero

   // cleanup & exit
   pthread_cancel(thread_id);
   pthread_join(thread_id, thread_result);
   CloseFB();
   unlink(PIDFILE);
   unlink(SCKFILE);
   infolog(1);
   if (slog == 1) {
      closelog();
   }
   return 0;
}
