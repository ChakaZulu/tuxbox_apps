#ifndef __color__
#define __color__

#define COL_INFOBAR_SHADOW		254-8*5-1
#define COL_INFOBAR			254-8*5
#define COL_MENUHEAD			254-8*4
#define COL_MENUCONTENT			254-8*3
#define COL_MENUCONTENTSELECTED		254-8*2
#define COL_MENUCONTENTINACTIVE		254-8*1

#define COL_BACKGROUND 			255

int convertSetupColor2RGB(unsigned char r, unsigned char g, unsigned char b);
int convertSetupAlpha2Alpha(unsigned char alpha);


#endif

