#include "color.h"


int convertSetupColor2RGB(unsigned char r, unsigned char g, unsigned char b)
{
	unsigned char red =  int( float(255./100.)*float(r) );
	unsigned char green =  int( float(255./100.)*float(g) );
	unsigned char blue =  int( float(255./100.)*float(b) );

	return (red << 16) | (green << 8) | blue;
}

int convertSetupAlpha2Alpha(unsigned char alpha)
{
	return int( float(0x7777/100.)*float(alpha) );
}

