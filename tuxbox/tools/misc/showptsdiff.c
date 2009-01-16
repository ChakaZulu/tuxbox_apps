/*
 * quick hack to show the current difference of the video and
 * audio PTS as seen in /proc/bus/bitstream
 *
 * Copyright (C) 2007 Stefan Seyfried
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	FILE *f;
	char *s = (char *)malloc(128);
	unsigned int pic_pts = 0, aud_pts = 0;
	unsigned int pic_ms, aud_ms, tmp;
	while (1)
	{
		f = fopen("/proc/bus/bitstream", "r");
		while ((s = fgets(s, 128, f)))
		{
			if (sscanf(s, "MR_PIC_PTS: 0x%x", &tmp) == 1) {
				pic_pts = tmp;
			} else if (sscanf(s, "MR_AUD_PTS: 0x%x", &tmp) == 1) {
				aud_pts = tmp;
				break;
			}
		}
		pic_ms = pic_pts / 45;
		aud_ms = aud_pts / 45;
		fclose(f);
		printf("pic_pts: %02d:%02d:%02d.%03d aud_pts: %02d:%02d:%02d.%03d, diff (ms): %d\n",
			pic_ms/3600000, (pic_ms/60000)%60,((pic_ms)/1000)%60, pic_ms%1000,
			aud_ms/3600000, (aud_ms/60000)%60,((aud_ms)/1000)%60, aud_ms%1000,
			pic_ms-aud_ms);
		sleep (1);
	}
}

