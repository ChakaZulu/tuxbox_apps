#include <config.h>
#include "newclock.h"

#define maxWidth  24
#define maxHeigth 48

static bool				digits[maxWidth*maxHeigth * 10];
static unsigned char	BMPWidth;
static unsigned char	BMPHeight;

void InitNewClock()
{
	FILE *fd;
	char filename_usr[] = CONFIGDIR "/lcdd/clock/0.bmp";
	char filename_std[] = DATADIR   "/lcdd/clock/0.bmp";
	char *filename = &filename_usr[0];
	int	 digit_pos = sizeof(filename_usr)-6;
	char line_buffer[4];
	int	 row, byte, bit;

	while(filename[digit_pos] <= '9')
	{
		if((fd = fopen(filename, "rb")) == 0)
		{
			printf("[lcdd] skin not found - using default!\n");
			filename = &filename_std[0];
			digit_pos = sizeof(filename_std)-6;
			continue;
		}

		fseek(fd, 0x12, SEEK_SET);
		fread(&BMPWidth, 1, 1, fd);
		fseek(fd, 0x16, SEEK_SET);
		fread(&BMPHeight, 1, 1, fd);

		if(BMPWidth > maxWidth || BMPHeight > maxHeigth)
		{
			printf("[lcdd] skin not supported - using default!\n");
			fclose(fd);
			filename = &filename_std[0];
			digit_pos = sizeof(filename_std)-6;
			continue;
		}

		fseek(fd, 0x3E, SEEK_SET);

		for(row = BMPHeight-1; row >= 0; row--)
		{
			fread(&line_buffer, 1, sizeof(line_buffer), fd);

			for(byte = 0; byte < maxWidth/8; byte++)
			{
				for(bit = 7; bit >= 0; bit--)
				{
					if(line_buffer[byte] & 1<<bit)	digits[7-bit + byte*8 + row*maxWidth + (filename[digit_pos] - '0')*maxWidth*maxHeigth] = 1;
					else							digits[7-bit + byte*8 + row*maxWidth + (filename[digit_pos] - '0')*maxWidth*maxHeigth] = 0;
				}
			}
		}

		fclose(fd);

		filename[digit_pos]++;
	}
}

void RenderDigit(CLCDDisplay* display, int digit, int position)
{
	int x, y;

	for(y = 0; y < BMPHeight; y++)
	{
		for(x = 0; x < BMPWidth; x++)
		{
			if(digits[x + y*maxWidth + digit*maxWidth*maxHeigth])	display->draw_point(position + x, (64-BMPHeight)/2 + y, CLCDDisplay::PIXEL_ON);
			else													display->draw_point(position + x, (64-BMPHeight)/2 + y, CLCDDisplay::PIXEL_OFF);
		}
	}
}

void ShowNewClock(CLCDDisplay* display, int h, int m)
{
	RenderDigit(display, h/10, 55 - 2*BMPWidth);
	RenderDigit(display, h%10, 57 - BMPWidth);
	RenderDigit(display, m/10, 63);
	RenderDigit(display, m%10, 65 + BMPWidth);

	display->draw_point(59, 29, CLCDDisplay::PIXEL_ON);
	display->draw_point(60, 29, CLCDDisplay::PIXEL_ON);
	display->draw_point(59, 30, CLCDDisplay::PIXEL_ON);
	display->draw_point(60, 30, CLCDDisplay::PIXEL_ON);
	display->draw_point(59, 33, CLCDDisplay::PIXEL_ON);
	display->draw_point(60, 33, CLCDDisplay::PIXEL_ON);
	display->draw_point(59, 34, CLCDDisplay::PIXEL_ON);
	display->draw_point(60, 34, CLCDDisplay::PIXEL_ON);
}
