
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "tuxtxt_def.h"
#if TUXTXT_COMPRESS == 1
#include <zlib.h>
#endif


/* shapes */
enum
{
	S_END = 0,
	S_FHL, /* full horizontal line: y-offset */
	S_FVL, /* full vertical line: x-offset */
	S_BOX, /* rectangle: x-offset, y-offset, width, height */
	S_TRA, /* trapez: x0, y0, l0, x1, y1, l1 */
	S_BTR, /* trapez in bgcolor: x0, y0, l0, x1, y1, l1 */
	S_INV, /* invert */
	S_LNK, /* call other shape: shapenumber */
	S_CHR, /* Character from freetype hibyte, lowbyte */
	S_ADT, /* Character 2F alternating raster */
	S_FLH, /* flip horizontal */
	S_FLV  /* flip vertical */
};

/* shape coordinates */
enum
{
	S_W13 = 5, /* width*1/3 */
	S_W12, /* width*1/2 */
	S_W23, /* width*2/3 */
	S_W11, /* width */
	S_WM3, /* width-3 */
	S_H13, /* height*1/3 */
	S_H12, /* height*1/2 */
	S_H23, /* height*2/3 */
	S_H11, /* height */
	S_NrShCoord
};

/* G3 characters */
unsigned char aG3_20[] = { S_TRA, 0, S_H23, 1, 0, S_H11, S_W12, S_END };
unsigned char aG3_21[] = { S_TRA, 0, S_H23, 1, 0, S_H11, S_W11, S_END };
unsigned char aG3_22[] = { S_TRA, 0, S_H12, 1, 0, S_H11, S_W12, S_END };
unsigned char aG3_23[] = { S_TRA, 0, S_H12, 1, 0, S_H11, S_W11, S_END };
unsigned char aG3_24[] = { S_TRA, 0, 0, 1, 0, S_H11, S_W12, S_END };
unsigned char aG3_25[] = { S_TRA, 0, 0, 1, 0, S_H11, S_W11, S_END };
unsigned char aG3_26[] = { S_INV, S_LNK, 0x66, S_END };
unsigned char aG3_27[] = { S_INV, S_LNK, 0x67, S_END };
unsigned char aG3_28[] = { S_INV, S_LNK, 0x68, S_END };
unsigned char aG3_29[] = { S_INV, S_LNK, 0x69, S_END };
unsigned char aG3_2a[] = { S_INV, S_LNK, 0x6a, S_END };
unsigned char aG3_2b[] = { S_INV, S_LNK, 0x6b, S_END };
unsigned char aG3_2c[] = { S_INV, S_LNK, 0x6c, S_END };
unsigned char aG3_2d[] = { S_INV, S_LNK, 0x6d, S_END };
unsigned char aG3_2e[] = { S_BOX, 2, 0, 3, S_H11, S_END };
unsigned char aG3_2f[] = { S_ADT };
unsigned char aG3_30[] = { S_LNK, 0x20, S_FLH, S_END };
unsigned char aG3_31[] = { S_LNK, 0x21, S_FLH, S_END };
unsigned char aG3_32[] = { S_LNK, 0x22, S_FLH, S_END };
unsigned char aG3_33[] = { S_LNK, 0x23, S_FLH, S_END };
unsigned char aG3_34[] = { S_LNK, 0x24, S_FLH, S_END };
unsigned char aG3_35[] = { S_LNK, 0x25, S_FLH, S_END };
unsigned char aG3_36[] = { S_INV, S_LNK, 0x76, S_END };
unsigned char aG3_37[] = { S_INV, S_LNK, 0x77, S_END };
unsigned char aG3_38[] = { S_INV, S_LNK, 0x78, S_END };
unsigned char aG3_39[] = { S_INV, S_LNK, 0x79, S_END };
unsigned char aG3_3a[] = { S_INV, S_LNK, 0x7a, S_END };
unsigned char aG3_3b[] = { S_INV, S_LNK, 0x7b, S_END };
unsigned char aG3_3c[] = { S_INV, S_LNK, 0x7c, S_END };
unsigned char aG3_3d[] = { S_INV, S_LNK, 0x7d, S_END };
unsigned char aG3_3e[] = { S_LNK, 0x2e, S_FLH, S_END };
unsigned char aG3_3f[] = { S_BOX, 0, 0, S_W11, S_H11, S_END };
unsigned char aG3_40[] = { S_BOX, 0, S_H13, S_W11, S_H13, S_LNK, 0x7e, S_END };
unsigned char aG3_41[] = { S_BOX, 0, S_H13, S_W11, S_H13, S_LNK, 0x7e, S_FLV, S_END };
unsigned char aG3_42[] = { S_LNK, 0x50, S_BOX, S_W12, S_H13, S_W12, S_H13, S_END };
unsigned char aG3_43[] = { S_LNK, 0x50, S_BOX, 0, S_H13, S_W12, S_H13, S_END };
unsigned char aG3_44[] = { S_LNK, 0x48, S_FLV, S_LNK, 0x48, S_END };
unsigned char aG3_45[] = { S_LNK, 0x44, S_FLH, S_END };
unsigned char aG3_46[] = { S_LNK, 0x47, S_FLV, S_END };
unsigned char aG3_47[] = { S_LNK, 0x48, S_FLH, S_LNK, 0x48, S_END };
unsigned char aG3_48[] = { S_TRA, 0, 0, S_W23, 0, S_H23, 0, S_BTR, 0, 0, S_W13, 0, S_H13, 0, S_END };
unsigned char aG3_49[] = { S_LNK, 0x48, S_FLH, S_END };
unsigned char aG3_4a[] = { S_LNK, 0x48, S_FLV, S_END };
unsigned char aG3_4b[] = { S_LNK, 0x48, S_FLH, S_FLV, S_END };
unsigned char aG3_4c[] = { S_LNK, 0x50, S_BOX, 0, S_H13, S_W11, S_H13, S_END };
unsigned char aG3_4d[] = { S_CHR, 0x25, 0xE6 };
unsigned char aG3_4e[] = { S_CHR, 0x25, 0xCF };
unsigned char aG3_4f[] = { S_CHR, 0x25, 0xCB };
unsigned char aG3_50[] = { S_BOX, S_W12, 0, 2, S_H11, S_FLH, S_BOX, S_W12, 0, 2, S_H11,S_END };
unsigned char aG3_51[] = { S_BOX, 0, S_H12, S_W11, 2, S_FLV, S_BOX, 0, S_H12, S_W11, 2,S_END };
unsigned char aG3_52[] = { S_LNK, 0x55, S_FLH, S_FLV, S_END };
unsigned char aG3_53[] = { S_LNK, 0x55, S_FLV, S_END };
unsigned char aG3_54[] = { S_LNK, 0x55, S_FLH, S_END };
unsigned char aG3_55[] = { S_LNK, 0x7e, S_FLV, S_BOX, 0, S_H12, S_W12, 2, S_FLV, S_BOX, 0, S_H12, S_W12, 2, S_END };
unsigned char aG3_56[] = { S_LNK, 0x57, S_FLH, S_END};
unsigned char aG3_57[] = { S_LNK, 0x55, S_LNK, 0x50 , S_END};
unsigned char aG3_58[] = { S_LNK, 0x59, S_FLV, S_END};
unsigned char aG3_59[] = { S_LNK, 0x7e, S_LNK, 0x51 , S_END};
unsigned char aG3_5a[] = { S_LNK, 0x50, S_LNK, 0x51 , S_END};
unsigned char aG3_5b[] = { S_CHR, 0x21, 0x92};
unsigned char aG3_5c[] = { S_CHR, 0x21, 0x90};
unsigned char aG3_5d[] = { S_CHR, 0x21, 0x91};
unsigned char aG3_5e[] = { S_CHR, 0x21, 0x93};
unsigned char aG3_5f[] = { S_CHR, 0x00, 0x20};
unsigned char aG3_60[] = { S_INV, S_LNK, 0x20, S_END };
unsigned char aG3_61[] = { S_INV, S_LNK, 0x21, S_END };
unsigned char aG3_62[] = { S_INV, S_LNK, 0x22, S_END };
unsigned char aG3_63[] = { S_INV, S_LNK, 0x23, S_END };
unsigned char aG3_64[] = { S_INV, S_LNK, 0x24, S_END };
unsigned char aG3_65[] = { S_INV, S_LNK, 0x25, S_END };
unsigned char aG3_66[] = { S_LNK, 0x20, S_FLV, S_END };
unsigned char aG3_67[] = { S_LNK, 0x21, S_FLV, S_END };
unsigned char aG3_68[] = { S_LNK, 0x22, S_FLV, S_END };
unsigned char aG3_69[] = { S_LNK, 0x23, S_FLV, S_END };
unsigned char aG3_6a[] = { S_LNK, 0x24, S_FLV, S_END };
unsigned char aG3_6b[] = { S_BOX, 0, 0, S_W11, S_H13, S_TRA, 0, S_H13, S_W11, 0, S_H23, 1, S_END };
unsigned char aG3_6c[] = { S_TRA, 0, 0, 1, 0, S_H12, S_W12, S_FLV, S_TRA, 0, 0, 1, 0, S_H12, S_W12, S_BOX, 0, S_H12, S_W12,1, S_END };
unsigned char aG3_6d[] = { S_TRA, 0, 0, S_W12, S_W12, S_H12, 0, S_FLH, S_TRA, 0, 0, S_W12, S_W12, S_H12, 0, S_END };
unsigned char aG3_6e[] = { S_CHR, 0x00, 0x20};
unsigned char aG3_6f[] = { S_CHR, 0x00, 0x20};
unsigned char aG3_70[] = { S_INV, S_LNK, 0x30, S_END };
unsigned char aG3_71[] = { S_INV, S_LNK, 0x31, S_END };
unsigned char aG3_72[] = { S_INV, S_LNK, 0x32, S_END };
unsigned char aG3_73[] = { S_INV, S_LNK, 0x33, S_END };
unsigned char aG3_74[] = { S_INV, S_LNK, 0x34, S_END };
unsigned char aG3_75[] = { S_INV, S_LNK, 0x35, S_END };
unsigned char aG3_76[] = { S_LNK, 0x66, S_FLH, S_END };
unsigned char aG3_77[] = { S_LNK, 0x67, S_FLH, S_END };
unsigned char aG3_78[] = { S_LNK, 0x68, S_FLH, S_END };
unsigned char aG3_79[] = { S_LNK, 0x69, S_FLH, S_END };
unsigned char aG3_7a[] = { S_LNK, 0x6a, S_FLH, S_END };
unsigned char aG3_7b[] = { S_LNK, 0x6b, S_FLH, S_END };
unsigned char aG3_7c[] = { S_LNK, 0x6c, S_FLH, S_END };
unsigned char aG3_7d[] = { S_LNK, 0x6d, S_FLV, S_END };
unsigned char aG3_7e[] = { S_BOX, S_W12, 0, 2, S_H12, S_FLH, S_BOX, S_W12, 0, 2, S_H12, S_END };// help char, not printed directly (only by S_LNK)

unsigned char *aShapes[] =
{
	aG3_20, aG3_21, aG3_22, aG3_23, aG3_24, aG3_25, aG3_26, aG3_27, aG3_28, aG3_29, aG3_2a, aG3_2b, aG3_2c, aG3_2d, aG3_2e, aG3_2f,
	aG3_30, aG3_31, aG3_32, aG3_33, aG3_34, aG3_35, aG3_36, aG3_37, aG3_38, aG3_39, aG3_3a, aG3_3b, aG3_3c, aG3_3d, aG3_3e, aG3_3f,
	aG3_40, aG3_41, aG3_42, aG3_43, aG3_44, aG3_45, aG3_46, aG3_47, aG3_48, aG3_49, aG3_4a, aG3_4b, aG3_4c, aG3_4d, aG3_4e, aG3_4f,
	aG3_50, aG3_51, aG3_52, aG3_53, aG3_54, aG3_55, aG3_56, aG3_57, aG3_58, aG3_59, aG3_5a, aG3_5b, aG3_5c, aG3_5d, aG3_5e, aG3_5f,
	aG3_60, aG3_61, aG3_62, aG3_63, aG3_64, aG3_65, aG3_66, aG3_67, aG3_68, aG3_69, aG3_6a, aG3_6b, aG3_6c, aG3_6d, aG3_6e, aG3_6f,
	aG3_70, aG3_71, aG3_72, aG3_73, aG3_74, aG3_75, aG3_76, aG3_77, aG3_78, aG3_79, aG3_7a, aG3_7b, aG3_7c, aG3_7d, aG3_7e
};

tuxtxt_cache_struct tuxtxt_cache;
static pthread_mutex_t tuxtxt_cache_lock = PTHREAD_MUTEX_INITIALIZER;
int tuxtxt_get_zipsize(int p,int sp)
{
    tstCachedPage* pg = tuxtxt_cache.astCachetable[p][sp];
    if (!pg) return 0;
#if TUXTXT_COMPRESS == 1
	return pg->ziplen;
#elif TUXTXT_COMPRESS == 2
	pthread_mutex_lock(&tuxtxt_cache_lock);
	int zipsize = 0,i,j;
	for (i = 0; i < 23*5; i++)
		for (j = 0; j < 8; j++)
		zipsize += pg->bitmask[i]>>j & 0x01;

	zipsize+=23*5;//bitmask
	pthread_mutex_unlock(&tuxtxt_cache_lock);
	return zipsize;
#else
	return 23*40;
#endif
}
void tuxtxt_compress_page(int p, int sp, unsigned char* buffer)
{
	pthread_mutex_lock(&tuxtxt_cache_lock);
    tstCachedPage* pg = tuxtxt_cache.astCachetable[p][sp];
    if (!pg)
    {
		printf("tuxtxt: trying to compress a not allocated page!!\n");
		pthread_mutex_unlock(&tuxtxt_cache_lock);
		return;
    }

#if TUXTXT_COMPRESS == 1
    unsigned char pagecompressed[23*40];
	uLongf comprlen = 23*40;
	if (compress2(pagecompressed,&comprlen,buffer,23*40,Z_BEST_SPEED) == Z_OK)
	{
		if (pg->pData) free(pg->pData);//realloc(pg->pData,j); realloc scheint nicht richtig zu funktionieren?
		pg->pData = malloc(comprlen);
		pg->ziplen = 0;
		if (pg->pData)
		{
			pg->ziplen = comprlen;
			memcpy(pg->pData,pagecompressed,comprlen);
		}
	}
#elif TUXTXT_COMPRESS == 2
    int i,j=0;
    unsigned char cbuf[23*40];
    memset(pg->bitmask,0,sizeof(pg->bitmask));
    for (i = 0; i < 23*40; i++)
    {
		if (i && buffer[i] == buffer[i-1])
		    continue;
		pg->bitmask[i>>3] |= 0x80>>(i&0x07);
		cbuf[j++]=buffer[i];
    }
    if (pg->pData) free(pg->pData);//realloc(pg->pData,j); realloc scheint nicht richtig zu funktionieren?
    pg->pData = malloc(j);
	if (pg->pData)
	{
	    memcpy(pg->pData,cbuf,j);
  	}
  	else
  	    memset(pg->bitmask,0,sizeof(pg->bitmask));

#else
	memcpy(pg->data,buffer,23*40);
#endif
	pthread_mutex_unlock(&tuxtxt_cache_lock);

}
void tuxtxt_decompress_page(int p, int sp, unsigned char* buffer)
{
	pthread_mutex_lock(&tuxtxt_cache_lock);
    tstCachedPage* pg = tuxtxt_cache.astCachetable[p][sp];
	memset(buffer,' ',23*40);
    if (!pg)
    {
		printf("tuxtxt: trying to decompress a not allocated page!!\n");
		pthread_mutex_unlock(&tuxtxt_cache_lock);
		return;
    }
	if (pg->pData)
	{
#if TUXTXT_COMPRESS == 1
		if (pg->ziplen)
		{
			uLongf comprlen = 23*40;
			uncompress(buffer,&comprlen,pg->pData,pg->ziplen);
		}

#elif TUXTXT_COMPRESS == 2
		int i,j=0;
		char c=0x20;
		for (i = 0; i < 23*40; i++)
		{
		    if (pg->bitmask[i>>3] & 0x80>>(i&0x07))
				c = pg->pData[j++];
		    buffer[i] = c;
		}
#else
		memcpy(buffer,pg->data,23*40);
#endif
	}
	pthread_mutex_unlock(&tuxtxt_cache_lock);
}
void tuxtxt_next_dec(int *i) /* skip to next decimal */
{
	(*i)++;

	if ((*i & 0x0F) > 0x09)
		*i += 0x06;

	if ((*i & 0xF0) > 0x90)
		*i += 0x60;

	if (*i > 0x899)
		*i = 0x100;
}

void tuxtxt_prev_dec(int *i)           /* counting down */
{
	(*i)--;

	if ((*i & 0x0F) > 0x09)
		*i -= 0x06;

	if ((*i & 0xF0) > 0x90)
		*i -= 0x60;

	if (*i < 0x100)
		*i = 0x899;
}

int tuxtxt_is_dec(int i)
{
	return ((i & 0x00F) <= 9) && ((i & 0x0F0) <= 0x90);
}

int tuxtxt_next_hex(int i) /* return next existing non-decimal page number */
{
	int startpage = i;
	if (startpage < 0x100)
		startpage = 0x100;

	do
	{
		i++;
		if (i > 0x8FF)
			i = 0x100;
		if (i == startpage)
			break;
	}  while ((tuxtxt_cache.subpagetable[i] == 0xFF) || tuxtxt_is_dec(i));
	return i;
}
#define number2char(c) ((c) + (((c) <= 9) ? '0' : ('A' - 10)))
/* print hex-number into string, s points to last digit, caller has to provide enough space, no termination */
void tuxtxt_hex2str(char *s, unsigned int n)
{
	do {
		char c = (n & 0xF);
		*s-- = number2char(c);
		n >>= 4;
	} while (n);
}
/*
 * TOP-Text
 * Info entnommen aus videotext-0.6.19991029,
 * Copyright (c) 1994-96 Martin Buck  <martin-2.buck@student.uni-ulm.de>
 */
void tuxtxt_decode_btt()
{
	/* basic top table */
	int i, current, b1, b2, b3, b4;
	unsigned char btt[23*40];

	if (tuxtxt_cache.subpagetable[0x1f0] == 0xff || 0 == tuxtxt_cache.astCachetable[0x1f0][tuxtxt_cache.subpagetable[0x1f0]]) /* not yet received */
		return;
	tuxtxt_decompress_page(0x1f0,tuxtxt_cache.subpagetable[0x1f0],btt);
	if (btt[799] == ' ') /* not completely received or error */
		return;

	current = 0x100;
	for (i = 0; i < 800; i++)
	{
		b1 = btt[i];
		if (b1 == ' ')
			b1 = 0;
		else
		{
			b1 = dehamming[b1];
			if (b1 == 0xFF) /* hamming error in btt */
			{
				btt[799] = ' '; /* mark btt as not received */
				return;
			}
		}
		tuxtxt_cache.basictop[current] = b1;
		tuxtxt_next_dec(&current);
	}
	/* page linking table */
	tuxtxt_cache.maxadippg = -1; /* rebuild table of adip pages */
	for (i = 0; i < 10; i++)
	{
		b1 = dehamming[btt[800 + 8*i +0]];

		if (b1 == 0xE)
			continue; /* unused */
		else if (b1 == 0xF)
			break; /* end */

		b4 = dehamming[btt[800 + 8*i +7]];

		if (b4 != 2) /* only adip, ignore multipage (1) */
			continue;

		b2 = dehamming[btt[800 + 8*i +1]];
		b3 = dehamming[btt[800 + 8*i +2]];

		if (b1 == 0xFF || b2 == 0xFF || b3 == 0xFF)
		{
			printf("TuxTxt <Biterror in btt/plt index %d>\n", i);
			btt[799] = ' '; /* mark btt as not received */
			return;
		}

		b1 = b1<<8 | b2<<4 | b3; /* page number */
		tuxtxt_cache.adippg[++tuxtxt_cache.maxadippg] = b1;
	}
#if DEBUG
	printf("TuxTxt <BTT decoded>\n");
#endif
	tuxtxt_cache.bttok = 1;
}

void tuxtxt_decode_adip() /* additional information table */
{
	int i, p, j, b1, b2, b3, charfound;
	unsigned char padip[23*40];

	for (i = 0; i <= tuxtxt_cache.maxadippg; i++)
	{
		p = tuxtxt_cache.adippg[i];
		if (!p || tuxtxt_cache.subpagetable[p] == 0xff || 0 == tuxtxt_cache.astCachetable[p][tuxtxt_cache.subpagetable[p]]) /* not cached (avoid segfault) */
			continue;

		tuxtxt_decompress_page(p,tuxtxt_cache.subpagetable[p],padip);
		for (j = 0; j < 44; j++)
		{
			b1 = dehamming[padip[20*j+0]];
			if (b1 == 0xE)
				continue; /* unused */

			if (b1 == 0xF)
				break; /* end */

			b2 = dehamming[padip[20*j+1]];
			b3 = dehamming[padip[20*j+2]];

			if (b1 == 0xFF || b2 == 0xFF || b3 == 0xFF)
			{
				printf("TuxTxt <Biterror in ait %03x %d %02x %02x %02x %02x %02x %02x>\n", p, j,
						 padip[20*j+0],
						 padip[20*j+1],
						 padip[20*j+2],
						 b1, b2, b3
						 );
				return;
			}

			if (b1>8 || b2>9 || b3>9) /* ignore extries with invalid or hex page numbers */
			{
				continue;
			}

			b1 = b1<<8 | b2<<4 | b3; /* page number */
			charfound = 0; /* flag: no printable char found */

			for (b2 = 11; b2 >= 0; b2--)
			{
				b3 = deparity[padip[20*j + 8 + b2]];
				if (b3 < ' ')
					b3 = ' ';

				if (b3 == ' ' && !charfound)
					tuxtxt_cache.adip[b1][b2] = '\0';
				else
				{
					tuxtxt_cache.adip[b1][b2] = b3;
					charfound = 1;
				}
			}
		} /* next link j */
		tuxtxt_cache.adippg[i] = 0; /* completely decoded: clear entry */
#if DEBUG
		printf("TuxTxt <ADIP %03x decoded>\n", p);
#endif
	} /* next adip page i */

	while (!tuxtxt_cache.adippg[tuxtxt_cache.maxadippg] && (tuxtxt_cache.maxadippg >= 0)) /* and shrink table */
		tuxtxt_cache.maxadippg--;
}
/******************************************************************************
 * GetSubPage                                                                 *
 ******************************************************************************/
int tuxtxt_GetSubPage(int page, int subpage, int offset)
{
	int loop;


	for (loop = subpage + offset; loop != subpage; loop += offset)
	{
		if (loop < 0)
			loop = 0x79;
		else if (loop > 0x79)
			loop = 0;
		if (loop == subpage)
			break;

		if (tuxtxt_cache.astCachetable[page][loop])
		{
#if DEBUG
			printf("TuxTxt <NextSubPage: %.3X-%.2X>\n", page, subpage);
#endif
			return loop;
		}
	}

#if DEBUG
	printf("TuxTxt <NextSubPage: no other SubPage>\n");
#endif
	return subpage;
}

/******************************************************************************
 * clear_cache                                                                *
 ******************************************************************************/

void tuxtxt_clear_cache()
{
	pthread_mutex_lock(&tuxtxt_cache_lock);
	int clear_page, clear_subpage, d26;
	tuxtxt_cache.maxadippg  = -1;
	tuxtxt_cache.bttok      = 0;
	tuxtxt_cache.cached_pages  = 0;
	tuxtxt_cache.page_receiving = -1;
	tuxtxt_cache.vtxtpid = -1;
	memset(&tuxtxt_cache.subpagetable, 0xFF, sizeof(tuxtxt_cache.subpagetable));
	memset(&tuxtxt_cache.basictop, 0, sizeof(tuxtxt_cache.basictop));
	memset(&tuxtxt_cache.adip, 0, sizeof(tuxtxt_cache.adip));
	memset(&tuxtxt_cache.flofpages, 0 , sizeof(tuxtxt_cache.flofpages));
	memset(&tuxtxt_cache.timestring, 0x20, 8);
 	unsigned char magazine;
	for (magazine = 1; magazine < 9; magazine++)
	{
		tuxtxt_cache.current_page  [magazine] = -1;
		tuxtxt_cache.current_subpage [magazine] = -1;
	}

	for (clear_page = 0; clear_page < 0x900; clear_page++)
		for (clear_subpage = 0; clear_subpage < 0x80; clear_subpage++)
			if (tuxtxt_cache.astCachetable[clear_page][clear_subpage])
			{
				tstPageinfo *p = &(tuxtxt_cache.astCachetable[clear_page][clear_subpage]->pageinfo);
				if (p->p24)
					free(p->p24);
				if (p->ext)
				{
					if (p->ext->p27)
						free(p->ext->p27);
					for (d26=0; d26 < 16; d26++)
						if (p->ext->p26[d26])
							free(p->ext->p26[d26]);
					free(p->ext);
				}
#if TUXTXT_COMPRESS >0
				if (tuxtxt_cache.astCachetable[clear_page][clear_subpage]->pData)
					free(tuxtxt_cache.astCachetable[clear_page][clear_subpage]->pData);
#endif
				free(tuxtxt_cache.astCachetable[clear_page][clear_subpage]);
				tuxtxt_cache.astCachetable[clear_page][clear_subpage] = 0;
			}
	for (clear_page = 0; clear_page < 9; clear_page++)
	{
		if (tuxtxt_cache.astP29[clear_page])
		{
		    if (tuxtxt_cache.astP29[clear_page]->p27)
			free(tuxtxt_cache.astP29[clear_page]->p27);
		    for (d26=0; d26 < 16; d26++)
			if (tuxtxt_cache.astP29[clear_page]->p26[d26])
			    free(tuxtxt_cache.astP29[clear_page]->p26[d26]);
		    free(tuxtxt_cache.astP29[clear_page]);
		    tuxtxt_cache.astP29[clear_page] = 0;
		}
		tuxtxt_cache.current_page  [clear_page] = -1;
		tuxtxt_cache.current_subpage [clear_page] = -1;
	}
	memset(&tuxtxt_cache.astCachetable, 0, sizeof(tuxtxt_cache.astCachetable));
	memset(&tuxtxt_cache.astP29, 0, sizeof(tuxtxt_cache.astP29));
#if DEBUG
	printf("TuxTxt cache cleared\n");
#endif
	pthread_mutex_unlock(&tuxtxt_cache_lock);
}
/******************************************************************************
 * init_demuxer                                                               *
 ******************************************************************************/

int tuxtxt_init_demuxer()
{
	/* open demuxer */
	if ((tuxtxt_cache.dmx = open(DMX, O_RDWR)) == -1)
	{
		perror("TuxTxt <open DMX>");
		return 0;
	}


	if (ioctl(tuxtxt_cache.dmx, DMX_SET_BUFFER_SIZE, 64*1024) < 0)
	{
		perror("TuxTxt <DMX_SET_BUFFERSIZE>");
		return 0;
	}
#if DEBUG
	printf("TuxTxt: initialized\n");
#endif
	/* init successfull */

	return 1;
}
/******************************************************************************
 * CacheThread support functions                                              *
 ******************************************************************************/

void tuxtxt_decode_p2829(unsigned char *vtxt_row, tstExtData **ptExtData)
{
	int bitsleft, colorindex;
	unsigned char *p;
	int t1 = deh24(&vtxt_row[7-4]);
	int t2 = deh24(&vtxt_row[10-4]);

	if (t1 < 0 || t2 < 0)
	{
#if DEBUG
		printf("TuxTxt <Biterror in p28>\n");
#endif
		return;
	}

	if (!(*ptExtData))
		(*ptExtData) = calloc(1, sizeof(tstExtData));
	if (!(*ptExtData))
		return;

	(*ptExtData)->p28Received = 1;
	(*ptExtData)->DefaultCharset = (t1>>7) & 0x7f;
	(*ptExtData)->SecondCharset = ((t1>>14) & 0x0f) | ((t2<<4) & 0x70);
	(*ptExtData)->LSP = !!(t2 & 0x08);
	(*ptExtData)->RSP = !!(t2 & 0x10);
	(*ptExtData)->SPL25 = !!(t2 & 0x20);
	(*ptExtData)->LSPColumns = (t2>>6) & 0x0f;

	bitsleft = 8; /* # of bits not evaluated in val */
	t2 >>= 10; /* current data */
	p = &vtxt_row[13-4];	/* pointer to next data triplet */
	for (colorindex = 0; colorindex < 16; colorindex++)
	{
		if (bitsleft < 12)
		{
			t2 |= deh24(p) << bitsleft;
			if (t2 < 0)	/* hamming error */
				break;
			p += 3;
			bitsleft += 18;
		}
		(*ptExtData)->bgr[colorindex] = t2 & 0x0fff;
		bitsleft -= 12;
		t2 >>= 12;
	}
	if (t2 < 0 || bitsleft != 14)
	{
#if DEBUG
		printf("TuxTxt <Biterror in p28/29 t2=%d b=%d>\n", t2, bitsleft);
#endif
		(*ptExtData)->p28Received = 0;
		return;
	}
	(*ptExtData)->DefScreenColor = t2 & 0x1f;
	t2 >>= 5;
	(*ptExtData)->DefRowColor = t2 & 0x1f;
	(*ptExtData)->BlackBgSubst = !!(t2 & 0x20);
	t2 >>= 6;
	(*ptExtData)->ColorTableRemapping = t2 & 0x07;
}

void tuxtxt_erase_page(int magazine)
{
	pthread_mutex_lock(&tuxtxt_cache_lock);
    tstCachedPage* pg = tuxtxt_cache.astCachetable[tuxtxt_cache.current_page[magazine]][tuxtxt_cache.current_subpage[magazine]];
	if (pg)
	{
		memset(&(pg->pageinfo), 0, sizeof(tstPageinfo));	/* struct pageinfo */
		memset(pg->p0, ' ', 24);
#if TUXTXT_COMPRESS == 1
    	if (pg->pData) {free(pg->pData); pg->pData = NULL;}
#elif TUXTXT_COMPRESS == 2
		memset(pg->bitmask, 0, 23*5);
#else
		memset(pg->data, ' ', 23*40);
#endif
	}
	pthread_mutex_unlock(&tuxtxt_cache_lock);
}

void tuxtxt_allocate_cache(int magazine)
{
	/* check cachetable and allocate memory if needed */
	if (tuxtxt_cache.astCachetable[tuxtxt_cache.current_page[magazine]][tuxtxt_cache.current_subpage[magazine]] == 0)
	{

		tuxtxt_cache.astCachetable[tuxtxt_cache.current_page[magazine]][tuxtxt_cache.current_subpage[magazine]] = malloc(sizeof(tstCachedPage));
		if (tuxtxt_cache.astCachetable[tuxtxt_cache.current_page[magazine]][tuxtxt_cache.current_subpage[magazine]] )
		{
#if TUXTXT_COMPRESS >0
			tuxtxt_cache.astCachetable[tuxtxt_cache.current_page[magazine]][tuxtxt_cache.current_subpage[magazine]]->pData = 0;
#endif
			tuxtxt_erase_page(magazine);
			tuxtxt_cache.cached_pages++;
		}
	}
}
/******************************************************************************
 * CacheThread                                                                *
 ******************************************************************************/

void *tuxtxt_CacheThread(void *arg)
{
	const unsigned char rev_lut[32] = {
		0x00,0x08,0x04,0x0c, /*  upper nibble */
		0x02,0x0a,0x06,0x0e,
		0x01,0x09,0x05,0x0d,
		0x03,0x0b,0x07,0x0f,
		0x00,0x80,0x40,0xc0, /*  lower nibble */
		0x20,0xa0,0x60,0xe0,
		0x10,0x90,0x50,0xd0,
		0x30,0xb0,0x70,0xf0 };
	unsigned char pes_packet[184];
	unsigned char vtxt_row[42];
	int line, byte/*, bit*/;
	int b1, b2, b3, b4;
	int packet_number;
	int doupdate=0;
	unsigned char magazine = 0xff;
	unsigned char pagedata[9][23*40];
	tstPageinfo *pageinfo_thread;

	printf("TuxTxt running thread...(%03x)\n",tuxtxt_cache.vtxtpid);
	tuxtxt_cache.receiving = 1;
	nice(3);
	while (1)
	{
		/* check stopsignal */
		pthread_testcancel();

		if (!tuxtxt_cache.receiving) continue;

		/* read packet */
		ssize_t readcnt;
		readcnt = read(tuxtxt_cache.dmx, &pes_packet, sizeof(pes_packet));

		if (readcnt != sizeof(pes_packet))
		{
#if DEBUG
			printf ("TuxTxt: readerror\n");
#endif
			continue;
		}

		/* analyze it */
		for (line = 0; line < 4; line++)
		{
			unsigned char *vtx_rowbyte = &pes_packet[line*0x2e];
			if ((vtx_rowbyte[0] == 0x02 || vtx_rowbyte[0] == 0x03) && (vtx_rowbyte[1] == 0x2C))
			{
				/* clear rowbuffer */
				/* convert row from lsb to msb (begin with magazin number) */
				for (byte = 4; byte < 46; byte++)
				{
					unsigned char upper,lower;
					upper = (vtx_rowbyte[byte] >> 4) & 0xf;
					lower = vtx_rowbyte[byte] & 0xf;
					vtxt_row[byte-4] = (rev_lut[upper]) | (rev_lut[lower+16]);
				}

				/* get packet number */
				b1 = dehamming[vtxt_row[0]];
				b2 = dehamming[vtxt_row[1]];

				if (b1 == 0xFF || b2 == 0xFF)
				{
#if DEBUG
					printf("TuxTxt <Biterror in Packet>\n");
#endif
					continue;
				}

				b1 &= 8;

				packet_number = b1>>3 | b2<<1;

				/* get magazine number */
				magazine = dehamming[vtxt_row[0]] & 7;
				if (!magazine) magazine = 8;

				if (packet_number == 0 && tuxtxt_cache.current_page[magazine] != -1 && tuxtxt_cache.current_subpage[magazine] != -1)
 				    tuxtxt_compress_page(tuxtxt_cache.current_page[magazine],tuxtxt_cache.current_subpage[magazine],pagedata[magazine]);

//printf("receiving packet %d %03x/%02x\n",packet_number, tuxtxt_cache.current_page[magazine],tuxtxt_cache.current_subpage[magazine]);

				/* analyze row */
				if (packet_number == 0)
				{
    					/* get pagenumber */
					b2 = dehamming[vtxt_row[3]];
					b3 = dehamming[vtxt_row[2]];

					if (b2 == 0xFF || b3 == 0xFF)
					{
						tuxtxt_cache.current_page[magazine] = tuxtxt_cache.page_receiving = -1;
#if DEBUG
						printf("TuxTxt <Biterror in Page>\n");
#endif
						continue;
					}

					tuxtxt_cache.current_page[magazine] = tuxtxt_cache.page_receiving = magazine<<8 | b2<<4 | b3;

					if (b2 == 0x0f && b3 == 0x0f)
					{
						tuxtxt_cache.current_subpage[magazine] = -1; /* ?ff: ignore data transmissions */
						continue;
					}

					/* get subpagenumber */
					b1 = dehamming[vtxt_row[7]];
					b2 = dehamming[vtxt_row[6]];
					b3 = dehamming[vtxt_row[5]];
					b4 = dehamming[vtxt_row[4]];

					if (b1 == 0xFF || b2 == 0xFF || b3 == 0xFF || b4 == 0xFF)
					{
#if DEBUG
						printf("TuxTxt <Biterror in SubPage>\n");
#endif
						tuxtxt_cache.current_subpage[magazine] = -1;
						continue;
					}

					b1 &= 3;
					b3 &= 7;

					if (tuxtxt_is_dec(tuxtxt_cache.page_receiving)) /* ignore other subpage bits for hex pages */
					{
#if 0	/* ? */
						if (b1 != 0 || b2 != 0)
						{
#if DEBUG
							printf("TuxTxt <invalid subpage data p%03x %02x %02x %02x %02x>\n", tuxtxt_cache.page_receiving, b1, b2, b3, b4);
#endif
							tuxtxt_cache.current_subpage[magazine] = -1;
							continue;
						}
						else
#endif
							tuxtxt_cache.current_subpage[magazine] = b3<<4 | b4;
					}
					else
						tuxtxt_cache.current_subpage[magazine] = b4; /* max 16 subpages for hex pages */

					/* store current subpage for this page */
					tuxtxt_cache.subpagetable[tuxtxt_cache.current_page[magazine]] = tuxtxt_cache.current_subpage[magazine];

					tuxtxt_allocate_cache(magazine);
					tuxtxt_decompress_page(tuxtxt_cache.current_page[magazine],tuxtxt_cache.current_subpage[magazine],pagedata[magazine]);
					pageinfo_thread = &(tuxtxt_cache.astCachetable[tuxtxt_cache.current_page[magazine]][tuxtxt_cache.current_subpage[magazine]]->pageinfo);

					if ((tuxtxt_cache.page_receiving & 0xff) == 0xfe) /* ?fe: magazine organization table (MOT) */
						pageinfo_thread->function = FUNC_MOT;

					/* check controlbits */
					if (dehamming[vtxt_row[5]] & 8)   /* C4 -> erase page */
					{
#if TUXTXT_COMPRESS == 1
						tuxtxt_cache.astCachetable[tuxtxt_cache.current_page[magazine]][tuxtxt_cache.current_subpage[magazine]]->ziplen = 0;
#elif TUXTXT_COMPRESS == 2
						memset(tuxtxt_cache.astCachetable[tuxtxt_cache.current_page[magazine]][tuxtxt_cache.current_subpage[magazine]]->bitmask, 0, 23*5);
#else
						memset(tuxtxt_cache.astCachetable[tuxtxt_cache.current_page[magazine]][tuxtxt_cache.current_subpage[magazine]]->data, ' ', 23*40);
#endif
						memset(pagedata[magazine],' ', 23*40);
					}
					if (dehamming[vtxt_row[9]] & 8)   /* C8 -> update page */
						doupdate = tuxtxt_cache.page_receiving;

					pageinfo_thread->boxed = !!(dehamming[vtxt_row[7]] & 0x0c);

					/* get country control bits */
					b1 = dehamming[vtxt_row[9]];
					if (b1 == 0xFF)
					{
#if DEBUG
						printf("TuxTxt <Biterror in CountryFlags>\n");
#endif
					}
					else
					{
						pageinfo_thread->nationalvalid = 1;
						pageinfo_thread->national = rev_lut[b1] & 0x07;
					}

					/* check parity, copy line 0 to cache (start and end 8 bytes are not needed and used otherwise) */
					unsigned char *p = tuxtxt_cache.astCachetable[tuxtxt_cache.current_page[magazine]][tuxtxt_cache.current_subpage[magazine]]->p0;
					for (byte = 10; byte < 42-8; byte++)
						*p++ = deparity[vtxt_row[byte]];

					if (!tuxtxt_is_dec(tuxtxt_cache.page_receiving))
						continue; /* valid hex page number: just copy headline, ignore timestring */

					/* copy timestring */
					p = tuxtxt_cache.timestring;
					for (; byte < 42; byte++)
						*p++ = deparity[vtxt_row[byte]];
				} /* (packet_number == 0) */
				else if (packet_number == 29 && dehamming[vtxt_row[2]]== 0) /* packet 29/0 replaces 28/0 for a whole magazine */
				{
					tuxtxt_decode_p2829(vtxt_row, &(tuxtxt_cache.astP29[magazine]));
				}
				else if (tuxtxt_cache.current_page[magazine] != -1 && tuxtxt_cache.current_subpage[magazine] != -1)
					/* packet>0, 0 has been correctly received, buffer allocated */
				{
					pageinfo_thread = &(tuxtxt_cache.astCachetable[tuxtxt_cache.current_page[magazine]][tuxtxt_cache.current_subpage[magazine]]->pageinfo);
					/* pointer to current info struct */

					if (packet_number <= 25)
					{
						unsigned char *p = NULL;
						if (packet_number < 24)
							p = pagedata[magazine] + 40*(packet_number-1);
						else
						{
							if (!(pageinfo_thread->p24))
								pageinfo_thread->p24 = calloc(2, 40);
							if (pageinfo_thread->p24)
								p = pageinfo_thread->p24 + (packet_number - 24) * 40;
						}
						if (p)
						{
							if (tuxtxt_is_dec(tuxtxt_cache.current_page[magazine]))
								for (byte = 2; byte < 42; byte++)
									*p++ = deparity[vtxt_row[byte]]; /* check/remove parity bit */
							else if ((tuxtxt_cache.current_page[magazine] & 0xff) == 0xfe)
								for (byte = 2; byte < 42; byte++)
									*p++ = dehamming[vtxt_row[byte]]; /* decode hamming 8/4 */
							else /* other hex page: no parity check, just copy */
								memcpy(p, &vtxt_row[2], 40);
						}
					}
					else if (packet_number == 27)
					{
						int descode = dehamming[vtxt_row[2]]; /* designation code (0..15) */

						if (descode == 0xff)
						{
#if DEBUG
							printf("TuxTxt <Biterror in p27>\n");
#endif
							continue;
						}
						if (descode == 0) // reading FLOF-Pagelinks
						{
							b1 = dehamming[vtxt_row[0]];
							if (b1 != 0xff)
							{
								b1 &= 7;

								for (byte = 0; byte < FLOFSIZE; byte++)
								{
									b2 = dehamming[vtxt_row[4+byte*6]];
									b3 = dehamming[vtxt_row[3+byte*6]];

									if (b2 != 0xff && b3 != 0xff)
									{
										b4 = ((b1 ^ (dehamming[vtxt_row[8+byte*6]]>>1)) & 6) |
											((b1 ^ (dehamming[vtxt_row[6+byte*6]]>>3)) & 1);
										if (b4 == 0)
											b4 = 8;
										if (b2 <= 9 && b3 <= 9)
											tuxtxt_cache.flofpages[tuxtxt_cache.current_page[magazine] ][byte] = b4<<8 | b2<<4 | b3;
									}
								}

								/* copy last 2 links to adip for TOP-Index */
								if (pageinfo_thread->p24) /* packet 24 received */
								{
									int a, a1, e=39, l=3;
									char *p = pageinfo_thread->p24;
									do
									{
										for (;
											  l >= 2 && 0 == tuxtxt_cache.flofpages[tuxtxt_cache.current_page[magazine]][l];
											  l--)
											; /* find used linkindex */
										for (;
											  e >= 1 && !isalnum(p[e]);
											  e--)
											; /* find end */
										for (a = a1 = e - 1;
											  a >= 0 && p[a] >= ' ';
											  a--) /* find start */
											if (p[a] > ' ')
											a1 = a; /* first non-space */
										if (a >= 0 && l >= 2)
										{
											strncpy(tuxtxt_cache.adip[tuxtxt_cache.flofpages[tuxtxt_cache.current_page[magazine]][l]],
													  &p[a1],
													  12);
											if (e-a1 < 11)
												tuxtxt_cache.adip[tuxtxt_cache.flofpages[tuxtxt_cache.current_page[magazine]][l]][e-a1+1] = '\0';
#if 0 //DEBUG
											printf(" %03x/%02x %d %d %d %d %03x %s\n",
													 tuxtxt_cache.current_page[magazine], tuxtxt_cache.current_subpage[magazine],
													 l, a, a1, e,
													 tuxtxt_cache.flofpages[tuxtxt_cache.current_page[magazine]][l],
													 tuxtxt_cache.adip[tuxtxt_cache.flofpages[tuxtxt_cache.current_page[magazine]][l]]
													 );
#endif
										}
										e = a - 1;
										l--;
									} while (l >= 2);
								}
							}
						}
						else if (descode == 4)	/* level 2.5 links (ignore level 3.5 links of /4 and /5) */
						{
							int i;
							tstp27 *p;

							if (!pageinfo_thread->ext)
								pageinfo_thread->ext = calloc(1, sizeof(tstExtData));
							if (!pageinfo_thread->ext)
								continue;
							if (!(pageinfo_thread->ext->p27))
								pageinfo_thread->ext->p27 = calloc(4, sizeof(tstp27));
							if (!(pageinfo_thread->ext->p27))
								continue;
							p = pageinfo_thread->ext->p27;
							for (i = 0; i < 4; i++)
							{
								int d1 = deh24(&vtxt_row[6*i + 3]);
								int d2 = deh24(&vtxt_row[6*i + 6]);
								if (d1 < 0 || d2 < 0)
								{
#if DEBUG
									printf("TuxTxt <Biterror in p27/4-5>\n");
#endif
									continue;
								}
								p->local = i & 0x01;
								p->drcs = !!(i & 0x02);
								p->l25 = !!(d1 & 0x04);
								p->l35 = !!(d1 & 0x08);
								p->page =
									(((d1 & 0x000003c0) >> 6) |
									 ((d1 & 0x0003c000) >> (14-4)) |
									 ((d1 & 0x00003800) >> (11-8))) ^
									(dehamming[vtxt_row[0]] << 8);
								if (p->page < 0x100)
									p->page += 0x800;
								p->subpage = d2 >> 2;
								if ((p->page & 0xff) == 0xff)
									p->page = 0;
								else if (p->page > 0x899)
								{
									// workaround for crash on RTL Shop ...
									// sorry.. i dont understand whats going wrong here :)
									printf("[TuxTxt] page > 0x900 ... ignore!!!!!!\n");
									continue;
								}
								else if (tuxtxt_cache.astCachetable[p->page][0])	/* link valid && linked page cached */
								{
									tstPageinfo *pageinfo_link = &(tuxtxt_cache.astCachetable[p->page][0]->pageinfo);
									if (p->local)
										pageinfo_link->function = p->drcs ? FUNC_DRCS : FUNC_POP;
									else
										pageinfo_link->function = p->drcs ? FUNC_GDRCS : FUNC_GPOP;
								}
								p++; /*  */
							}
						}
					}

					else if (packet_number == 26)
					{
						int descode = dehamming[vtxt_row[2]]; /* designation code (0..15) */

						if (descode == 0xff)
						{
#if DEBUG
							printf("TuxTxt <Biterror in p26>\n");
#endif
							continue;
						}
						if (!pageinfo_thread->ext)
							pageinfo_thread->ext = calloc(1, sizeof(tstExtData));
						if (!pageinfo_thread->ext)
							continue;
						if (!(pageinfo_thread->ext->p26[descode]))
							pageinfo_thread->ext->p26[descode] = malloc(13 * 3);
						if (pageinfo_thread->ext->p26[descode])
							memcpy(pageinfo_thread->ext->p26[descode], &vtxt_row[3], 13 * 3);
#if 0//DEBUG
						int i, t, m;

						printf("P%03x/%02x %02d/%x",
								 tuxtxt_cache.current_page[magazine], tuxtxt_cache.current_subpage[magazine],
								 packet_number, dehamming[vtxt_row[2]]);
						for (i=7-4; i <= 45-4; i+=3) /* dump all triplets */
						{
							t = deh24(&vtxt_row[i]); /* mode/adr/data */
							m = (t>>6) & 0x1f;
							printf(" M%02xA%02xD%03x", m, t & 0x3f, (t>>11) & 0x7f);
							if (m == 0x1f)	/* terminator */
								break;
						}
						putchar('\n');
#endif
					}
					else if (packet_number == 28)
					{
						int descode = dehamming[vtxt_row[2]]; /* designation code (0..15) */

						if (descode == 0xff)
						{
#if DEBUG
							printf("TuxTxt <Biterror in p28>\n");
#endif
							continue;
						}
						if (descode != 2)
						{
							int t1 = deh24(&vtxt_row[7-4]);
							pageinfo_thread->function = t1 & 0x0f;
							if (!pageinfo_thread->nationalvalid)
							{
								pageinfo_thread->nationalvalid = 1;
								pageinfo_thread->national = (t1>>4) & 0x07;
							}
						}

						switch (descode) /* designation code */
						{
						case 0: /* basic level 1 page */
						{
							tuxtxt_decode_p2829(vtxt_row, &(pageinfo_thread->ext));
							break;
						}
						case 1: /* G0/G1 designation for older decoders, level 3.5: DCLUT4/16, colors for multicolored bitmaps */
						{
							break; /* ignore */
						}
						case 2: /* page key */
						{
							break; /* ignore */
						}
						case 3: /* types of PTUs in DRCS */
						{
							break; /* TODO */
						}
						case 4: /* CLUTs 0/1, only level 3.5 */
						{
							break; /* ignore */
						}
						default:
						{
							break; /* invalid, ignore */
						}
						} /* switch designation code */
					}
					else if (packet_number == 30)
					{
#if 0//DEBUG
						int i;

						printf("p%03x/%02x %02d/%x ",
								 tuxtxt_cache.current_page[magazine], tuxtxt_cache.current_subpage[magazine],
								 packet_number, dehamming[vtxt_row[2]]);
						for (i=26-4; i <= 45-4; i++) /* station ID */
							putchar(deparity[vtxt_row[i]]);
						putchar('\n');
#endif
					}
				}
				/* set update flag */
				if (tuxtxt_cache.current_page[magazine] == tuxtxt_cache.page && tuxtxt_cache.current_subpage[magazine] != -1)
				{
 				    tuxtxt_compress_page(tuxtxt_cache.current_page[magazine],tuxtxt_cache.current_subpage[magazine],pagedata[magazine]);
					tuxtxt_cache.pageupdate = 1+(doupdate == tuxtxt_cache.page ? 1: 0);
					doupdate=0;
					if (!tuxtxt_cache.zap_subpage_manual)
						tuxtxt_cache.subpage = tuxtxt_cache.current_subpage[magazine];
				}
			}
		}
	}
	return 0;
}
/******************************************************************************
 * start_thread                                                               *
 ******************************************************************************/
int tuxtxt_start_thread()
{
	if (tuxtxt_cache.vtxtpid == -1) return 0;


	tuxtxt_cache.thread_starting = 1;
	struct dmx_pes_filter_params dmx_flt;

	/* set filter & start demuxer */
	dmx_flt.pid      = tuxtxt_cache.vtxtpid;
	dmx_flt.input    = DMX_IN_FRONTEND;
	dmx_flt.output   = DMX_OUT_TAP;
	dmx_flt.pes_type = DMX_PES_OTHER;
	dmx_flt.flags    = DMX_IMMEDIATE_START;

	if (tuxtxt_cache.dmx == -1) tuxtxt_init_demuxer();

	if (ioctl(tuxtxt_cache.dmx, DMX_SET_PES_FILTER, &dmx_flt) == -1)
	{
		perror("TuxTxt <DMX_SET_PES_FILTER>");
		tuxtxt_cache.thread_starting = 0;
		return 0;
	}

	/* create decode-thread */
	if (pthread_create(&tuxtxt_cache.thread_id, NULL, tuxtxt_CacheThread, NULL) != 0)
	{
		perror("TuxTxt <pthread_create>");
		tuxtxt_cache.thread_starting = 0;
		tuxtxt_cache.thread_id = 0;
		return 0;
	}
#if 1//DEBUG
	printf("TuxTxt service started %x\n", tuxtxt_cache.vtxtpid);
#endif
	tuxtxt_cache.receiving = 1;
	tuxtxt_cache.thread_starting = 0;
	return 1;
}
/******************************************************************************
 * stop_thread                                                                *
 ******************************************************************************/

int tuxtxt_stop_thread()
{

	/* stop decode-thread */
	if (tuxtxt_cache.thread_id != 0)
	{
		if (pthread_cancel(tuxtxt_cache.thread_id) != 0)
		{
			perror("TuxTxt <pthread_cancel>");
			return 0;
		}

		if (pthread_join(tuxtxt_cache.thread_id, &tuxtxt_cache.thread_result) != 0)
		{
			perror("TuxTxt <pthread_join>");
			return 0;
		}
		tuxtxt_cache.thread_id = 0;
	}
	if (tuxtxt_cache.dmx != -1)
	{
		ioctl(tuxtxt_cache.dmx, DMX_STOP);
//        close(tuxtxt_cache.dmx);
  	}
//	tuxtxt_cache.dmx = -1;
#if 1//DEBUG
	printf("TuxTxt stopped service %x\n", tuxtxt_cache.vtxtpid);
#endif
	return 1;
}

/******************************************************************************
 * decode Level2.5                                                            *
 ******************************************************************************/
int tuxtxt_eval_triplet(int iOData, tstCachedPage *pstCachedPage,
					  unsigned char *pAPx, unsigned char *pAPy,
					  unsigned char *pAPx0, unsigned char *pAPy0,
					  unsigned char *drcssubp, unsigned char *gdrcssubp,
					  signed char *endcol, tstPageAttr *attrPassive, unsigned char* pagedata, unsigned char* page_char, tstPageAttr* page_atrb);

/* get object data */
/* in: absolute triplet number (0..506, start at packet 3 byte 1) */
/* in: pointer to cache struct of page data */
/* out: 18 bit triplet data, <0 if invalid number, not cached, or hamming error */
int tuxtxt_iTripletNumber2Data(int iONr, tstCachedPage *pstCachedPage, unsigned char* pagedata)
{
	if (iONr > 506 || 0 == pstCachedPage)
		return -1;

	unsigned char *p;
	int packet = (iONr / 13) + 3;
	int packetoffset = 3 * (iONr % 13);

	if (packet <= 23)
		p = pagedata + 40*(packet-1) + packetoffset + 1;
	else if (packet <= 25)
	{
		if (0 == pstCachedPage->pageinfo.p24)
			return -1;
		p = pstCachedPage->pageinfo.p24 + 40*(packet-24) + packetoffset + 1;
	}
	else
	{
		int descode = packet - 26;
		if (0 == pstCachedPage->pageinfo.ext)
			return -1;
		if (0 == pstCachedPage->pageinfo.ext->p26[descode])
			return -1;
		p = pstCachedPage->pageinfo.ext->p26[descode] + packetoffset;	/* first byte (=designation code) is not cached */
	}
	return deh24(p);
}

#define RowAddress2Row(row) ((row == 40) ? 24 : (row - 40))

/* dump interpreted object data to stdout */
/* in: 18 bit object data */
/* out: termination info, >0 if end of object */
void tuxtxt_eval_object(int iONr, tstCachedPage *pstCachedPage,
					  unsigned char *pAPx, unsigned char *pAPy,
					  unsigned char *pAPx0, unsigned char *pAPy0,
					  tObjType ObjType, unsigned char* pagedata, unsigned char* page_char, tstPageAttr* page_atrb)
{
	int iOData;
	int iONr1 = iONr + 1; /* don't terminate after first triplet */
	unsigned char drcssubp=0, gdrcssubp=0;
	signed char endcol = -1; /* last column to which to extend attribute changes */
	tstPageAttr attrPassive = { white  , black , C_G0P, 0, 0, 1 ,0, 0, 0, 0, 0, 0, 0, 0x3f}; /* current attribute for passive objects */

	do
	{
		iOData = tuxtxt_iTripletNumber2Data(iONr, pstCachedPage,pagedata);	/* get triplet data, next triplet */
		if (iOData < 0) /* invalid number, not cached, or hamming error: terminate */
			break;
#if TUXTXT_DEBUG
		if (dumpl25)
			printf("  t%03d ", iONr);
#endif
		if (endcol < 0)
		{
			if (ObjType == OBJ_ACTIVE)
			{
				endcol = 40;
			}
			else if (ObjType == OBJ_ADAPTIVE) /* search end of line */
			{
				int i;
				for (i = iONr; i <= 506; i++)
				{
					int iTempOData = tuxtxt_iTripletNumber2Data(i, pstCachedPage,pagedata); /* get triplet data, next triplet */
					int iAddress = (iTempOData      ) & 0x3f;
					int iMode    = (iTempOData >>  6) & 0x1f;
					//int iData    = (iTempOData >> 11) & 0x7f;
					if (iTempOData < 0 || /* invalid number, not cached, or hamming error: terminate */
						 (iAddress >= 40	/* new row: row address and */
						 && (iMode == 0x01 || /* Full Row Color or */
							  iMode == 0x04 || /* Set Active Position */
							  (iMode >= 0x15 && iMode <= 0x17) || /* Object Definition */
							  iMode == 0x17))) /* Object Termination */
						break;
					if (iAddress < 40 && iMode != 0x06)
						endcol = iAddress;
				}
#if TUXTXT_DEBUG
				if (dumpl25)
					printf("  endcol %02d", endcol);
#endif
			}
		}
		iONr++;
	}
	while (0 == tuxtxt_eval_triplet(iOData, pstCachedPage, pAPx, pAPy, pAPx0, pAPy0, &drcssubp, &gdrcssubp, &endcol, &attrPassive, pagedata, page_char, page_atrb)
			 || iONr1 == iONr); /* repeat until termination reached */
}

void tuxtxt_eval_NumberedObject(int p, int s, int packet, int triplet, int high,
								 unsigned char *pAPx, unsigned char *pAPy,
								 unsigned char *pAPx0, unsigned char *pAPy0, unsigned char* page_char, tstPageAttr* page_atrb)
{
	if (!packet || 0 == tuxtxt_cache.astCachetable[p][s])
		return;
	unsigned char pagedata[23*40];
	tuxtxt_decompress_page(p, s,pagedata);


	int idata = deh24(pagedata + 40*(packet-1) + 1 + 3*triplet);
	int iONr;

	if (idata < 0)	/* hamming error: ignore triplet */
		return;
	if (high)
		iONr = idata >> 9; /* triplet number of odd object data */
	else
		iONr = idata & 0x1ff; /* triplet number of even object data */
	if (iONr <= 506)
	{
#if TUXTXT_DEBUG
		if (dumpl25)
			printf("P%xT%x%c %8s %c#%03d@%03d\n", packet, triplet, "LH"[!!high],	/* pointer pos, type, number, data pos */
					 ObjectType[triplet % 3], "PCD"[triplet % 3], 8*packet + 2*(triplet-1)/3, iONr);

#endif
		tuxtxt_eval_object(iONr, tuxtxt_cache.astCachetable[p][s], pAPx, pAPy, pAPx0, pAPy0, (tObjType)(triplet % 3),pagedata, page_char, page_atrb);
	}
}

int tuxtxt_eval_triplet(int iOData, tstCachedPage *pstCachedPage,
					  unsigned char *pAPx, unsigned char *pAPy,
					  unsigned char *pAPx0, unsigned char *pAPy0,
					  unsigned char *drcssubp, unsigned char *gdrcssubp,
					  signed char *endcol, tstPageAttr *attrPassive, unsigned char* pagedata, unsigned char* page_char, tstPageAttr* page_atrb)
{
	int iAddress = (iOData      ) & 0x3f;
	int iMode    = (iOData >>  6) & 0x1f;
	int iData    = (iOData >> 11) & 0x7f;

	if (iAddress < 40) /* column addresses */
	{
		int offset;	/* offset to page_char and page_atrb */

		if (iMode != 0x06)
			*pAPx = iAddress;	/* new Active Column */
		offset = (*pAPy0 + *pAPy) * 40 + *pAPx0 + *pAPx;	/* offset to page_char and page_atrb */
#if TUXTXT_DEBUG
		if (dumpl25)
			printf("  M%02xC%02xD%02x %d r:%d ch:%02x", iMode, iAddress, iData, *endcol,*pAPy0 + *pAPy,page_char[offset]);
#endif

		switch (iMode)
		{
		case 0x00:
			if (0 == (iData>>5))
			{
				int newcolor = iData & 0x1f;
				if (*endcol < 0) /* passive object */
					attrPassive->fg = newcolor;
				else if (*endcol == 40) /* active object */
				{
					tstPageAttr *p = &page_atrb[offset];
					int oldcolor = (p)->fg; /* current color (set-after) */
					int c = *pAPx0 + *pAPx;	/* current column absolute */
					do
					{
						p->fg = newcolor;
						p++;
						c++;
					} while (c < 40 && p->fg == oldcolor);	/* stop at change by level 1 page */
				}
				else /* adaptive object */
				{
					tstPageAttr *p = &page_atrb[offset];
					int c = *pAPx;	/* current column relative to object origin */
					do
					{
						p->fg = newcolor;
						p++;
						c++;
					} while (c <= *endcol);
				}
#if TUXTXT_DEBUG
				if (dumpl25)
					printf("  ,%02d FGCol T%x#%x", iAddress, (iData>>3)&0x03, iData&0x07);
#endif
			}
			break;
		case 0x01:
			if (iData >= 0x20)
			{
#if TUXTXT_DEBUG
				if (dumpl25)
					printf("  ,%02d BlockMosaic G1 #%02x", iAddress, iData);
#endif
				page_char[offset] = iData;
				if (*endcol < 0) /* passive object */
				{
					attrPassive->charset = C_G1C; /* FIXME: separated? */
					page_atrb[offset] = *attrPassive;
				}
				else if (page_atrb[offset].charset != C_G1S)
					page_atrb[offset].charset = C_G1C; /* FIXME: separated? */
			}
			break;
		case 0x02:
		case 0x0b:
#if TUXTXT_DEBUG
			if (dumpl25)
				printf("  ,%02d G3 #%02x f%db%d", iAddress, iData,attrPassive->fg, attrPassive->bg);
#endif
			page_char[offset] = iData;
			if (*endcol < 0) /* passive object */
			{
				attrPassive->charset = C_G3;
				page_atrb[offset] = *attrPassive;
			}
			else
				page_atrb[offset].charset = C_G3;
			break;
		case 0x03:
			if (0 == (iData>>5))
			{
				int newcolor = iData & 0x1f;
				if (*endcol < 0) /* passive object */
					attrPassive->bg = newcolor;
				else if (*endcol == 40) /* active object */
				{
					tstPageAttr *p = &page_atrb[offset];
					int oldcolor = (p)->bg; /* current color (set-after) */
					int c = *pAPx0 + *pAPx;	/* current column absolute */
					do
					{
						p->bg = newcolor;
						if (newcolor == black)
							p->IgnoreAtBlackBgSubst = 1;
						p++;
						c++;
					} while (c < 40 && p->bg == oldcolor);	/* stop at change by level 1 page */
				}
				else /* adaptive object */
				{
					tstPageAttr *p = &page_atrb[offset];
					int c = *pAPx;	/* current column relative to object origin */
					do
					{
						p->bg = newcolor;
						if (newcolor == black)
							p->IgnoreAtBlackBgSubst = 1;
						p++;
						c++;
					} while (c <= *endcol);
				}
#if TUXTXT_DEBUG
				if (dumpl25)
					printf("  ,%02d BGCol T%x#%x", iAddress, (iData>>3)&0x03, iData&0x07);
#endif
			}
			break;
		case 0x06:
#if TUXTXT_DEBUG
			if (dumpl25)
				printf("  PDC");
#endif
			/* ignore */
			break;
		case 0x07:
#if TUXTXT_DEBUG
			if (dumpl25)
				printf("  ,%02d Flash M%xP%x", iAddress, iData & 0x03, (iData >> 2) & 0x07);
#endif
			if ((iData & 0x60) != 0) break; // reserved data field
			if (*endcol < 0) /* passive object */
			{
				attrPassive->flashing=iData & 0x1f;
				page_atrb[offset] = *attrPassive;
			}
			else
				page_atrb[offset].flashing=iData & 0x1f;
			break;
		case 0x08:
#if TUXTXT_DEBUG
			if (dumpl25)
				printf("  ,%02d G0+G2 set #%02x (p105)", iAddress, iData);
#endif
			if (*endcol < 0) /* passive object */
			{
				attrPassive->setG0G2=iData & 0x3f;
				page_atrb[offset] = *attrPassive;
			}
			else
				page_atrb[offset].setG0G2=iData & 0x3f;
			break;
		case 0x09:
#if TUXTXT_DEBUG
			if (dumpl25)
				printf("  ,%02d G0 #%02x '%c'", iAddress, iData, iData);
#endif
			page_char[offset] = iData;
			if (*endcol < 0) /* passive object */
			{
				attrPassive->charset = C_G0P; /* FIXME: secondary? */
				attrPassive->setX26  = 1;
				page_atrb[offset] = *attrPassive;
			}
			else
			{
				page_atrb[offset].charset = C_G0P; /* FIXME: secondary? */
				page_atrb[offset].setX26  = 1;
			}
			break;
//		case 0x0b: (see 0x02)
		case 0x0c:
		{
#if TUXTXT_DEBUG
			if (dumpl25)
				printf("  ,%02d Attribute%s%s%s%s%s%s", iAddress,
						 (iData & 0x40) ? " DoubleWidth" : "",
						 (iData & 0x20) ? " UnderlineSep" : "",
						 (iData & 0x10) ? " InvColour" : "",
						 (iData & 0x04) ? " Conceal" : "",
						 (iData & 0x02) ? " Boxing" : "",
						 (iData & 0x01) ? " DoubleHeight" : "");
#endif
			int conc = (iData & 0x04);
			int inv  = (iData & 0x10);
			int dw   = (iData & 0x40 ?1:0);
			int dh   = (iData & 0x01 ?1:0);
			int sep  = (iData & 0x20);
			int bw   = (iData & 0x02 ?1:0);
			if (*endcol < 0) /* passive object */
			{
				if (conc)
				{
					attrPassive->concealed = 1;
					attrPassive->fg = attrPassive->bg;
				}
				attrPassive->inverted = (inv ? 1- attrPassive->inverted : 0);
				attrPassive->doubleh = dh;
				attrPassive->doublew = dw;
				attrPassive->boxwin = bw;
				if (bw) attrPassive->IgnoreAtBlackBgSubst = 0;
				if (sep)
				{
					if (attrPassive->charset == C_G1C)
						attrPassive->charset = C_G1S;
					else
						attrPassive->underline = 1;
				}
				else
				{
					if (attrPassive->charset == C_G1S)
						attrPassive->charset = C_G1C;
					else
						attrPassive->underline = 0;
				}
			}
			else
			{

				int c = *pAPx0 + (*endcol == 40 ? *pAPx : 0);	/* current column */
				int c1 = offset;
				tstPageAttr *p = &page_atrb[offset];
				do
				{
					p->inverted = (inv ? 1- p->inverted : 0);
					if (conc)
					{
						p->concealed = 1;
						p->fg = p->bg;
					}
					if (sep)
					{
						if (p->charset == C_G1C)
							p->charset = C_G1S;
						else
							p->underline = 1;
					}
					else
					{
						if (p->charset == C_G1S)
							p->charset = C_G1C;
						else
							p->underline = 0;
					}
					p->doublew = dw;
					p->doubleh = dh;
					p->boxwin = bw;
					if (bw) p->IgnoreAtBlackBgSubst = 0;
					p++;
					c++;
					c1++;
				} while (c < *endcol);
			}
			break;
		}
		case 0x0d:
#if TUXTXT_DEBUG
			if (dumpl25)
				printf("  ,%02d %cDRCS #%02x", iAddress, (iData & 0x40) ? ' ' : 'G', iData & 0x3f);
#endif
			page_char[offset] = iData & 0x3f;
			if (*endcol < 0) /* passive object */
			{
				attrPassive->charset = C_OFFSET_DRCS + ((iData & 0x40) ? (0x10 + *drcssubp) : *gdrcssubp);
				page_atrb[offset] = *attrPassive;
			}
			else
				page_atrb[offset].charset = C_OFFSET_DRCS + ((iData & 0x40) ? (0x10 + *drcssubp) : *gdrcssubp);
			break;
		case 0x0f:
#if TUXTXT_DEBUG
			if (dumpl25)
				printf("  ,%02d G2 #%02x", iAddress, iData);
#endif
			page_char[offset] = iData;
			if (*endcol < 0) /* passive object */
			{
				attrPassive->charset = C_G2;
				page_atrb[offset] = *attrPassive;
			}
			else
				page_atrb[offset].charset = C_G2;
			break;
		default:
			if (iMode == 0x10 && iData == 0x2a)
				iData = '@';
			if (iMode >= 0x10)
			{
#if TUXTXT_DEBUG
				if (dumpl25)
					printf("  ,%02d G0 #%02x %c +diacr. %x", iAddress, iData, iData, iMode & 0x0f);
#endif
				page_char[offset] = iData;
				if (*endcol < 0) /* passive object */
				{
					attrPassive->charset = C_G0P;
					attrPassive->diacrit = iMode & 0x0f;
					attrPassive->setX26  = 1;
					page_atrb[offset] = *attrPassive;
				}
				else
				{
					page_atrb[offset].charset = C_G0P;
					page_atrb[offset].diacrit = iMode & 0x0f;
					page_atrb[offset].setX26  = 1;
				}
			}
			break; /* unsupported or not yet implemented mode: ignore */
		} /* switch (iMode) */
	}
	else /* ================= (iAddress >= 40): row addresses ====================== */
	{
#if TUXTXT_DEBUG
		if (dumpl25)
			printf("  M%02xR%02xD%02x", iMode, iAddress, iData);
#endif
		switch (iMode)
		{
		case 0x00:
			if (0 == (iData>>5))
			{
#if TUXTXT_DEBUG
				if (dumpl25)
					printf("  FScrCol T%x#%x", (iData>>3)&0x03, iData&0x07);
#endif
				tuxtxt_cache.FullScrColor = iData & 0x1f;
			}
			break;
		case 0x01:
			if (*endcol == 40) /* active object */
			{
				*pAPy = RowAddress2Row(iAddress);	/* new Active Row */

				int color = iData & 0x1f;
				int row = *pAPy0 + *pAPy;
				int maxrow;
#if TUXTXT_DEBUG
				if (dumpl25)
				{
					printf("  AP=%d,0", RowAddress2Row(iAddress));
					if (0 == (iData>>5))
						printf("  FRowCol T%x#%x", (iData>>3)&0x03, iData&0x07);
					else if (3 == (iData>>5))
						printf("  FRowCol++ T%x#%x", (iData>>3)&0x03, iData&0x07);
				}
#endif
				if (row <= 24 && 0 == (iData>>5))
					maxrow = row;
				else if (3 == (iData>>5))
					maxrow = 24;
				else
					maxrow = -1;
				for (; row <= maxrow; row++)
					tuxtxt_cache.FullRowColor[row] = color;
				*endcol = -1;
			}
			break;
		case 0x04:
#if TUXTXT_DEBUG
			if (dumpl25)
				printf(" AP=%d,%d", RowAddress2Row(iAddress), iData);
#endif
			*pAPy = RowAddress2Row(iAddress); /* new Active Row */
			if (iData < 40)
				*pAPx = iData;	/* new Active Column */
			*endcol = -1; /* FIXME: check if row changed? */
			break;
		case 0x07:
#if TUXTXT_DEBUG
			if (dumpl25)
			{
				if (iAddress == 0x3f)
					printf("  AP=0,0");
				if (0 == (iData>>5))
					printf("  Address Display R0 FRowCol T%x#%x", (iData>>3)&0x03, iData&0x07);
				else if (3 == (iData>>5))
					printf("  Address Display R0->24 FRowCol T%x#%x", (iData>>3)&0x03, iData&0x07);
			}
#endif
			if (iAddress == 0x3f)
			{
				*pAPx = *pAPy = 0; /* new Active Position 0,0 */
				if (*endcol == 40) /* active object */
				{
					int color = iData & 0x1f;
					int row = *pAPy0; // + *pAPy;
					int maxrow;

					if (row <= 24 && 0 == (iData>>5))
						maxrow = row;
					else if (3 == (iData>>5))
						maxrow = 24;
					else
						maxrow = -1;
					for (; row <= maxrow; row++)
						tuxtxt_cache.FullRowColor[row] = color;
				}
				*endcol = -1;
			}
			break;
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x0f:
#if TUXTXT_DEBUG
			if (dumpl25)
				printf("  PDC");
#endif
			/* ignore */
			break;
		case 0x10:
#if TUXTXT_DEBUG
			if (dumpl25)
				printf("  AP=%d,%d  temp. Origin Modifier", iAddress - 40, iData);
#endif
			tuxtxt_cache.tAPy = iAddress - 40;
			tuxtxt_cache.tAPx = iData;
			break;
		case 0x11:
		case 0x12:
		case 0x13:
			if (iAddress & 0x10)	/* POP or GPOP */
			{
				unsigned char APx = 0, APy = 0;
				unsigned char APx0 = *pAPx0 + *pAPx + tuxtxt_cache.tAPx, APy0 = *pAPy0 + *pAPy + tuxtxt_cache.tAPy;
				int triplet = 3 * ((iData >> 5) & 0x03) + (iMode & 0x03);
				int packet = (iAddress & 0x03) + 1;
				int subp = iData & 0x0f;
				int high = (iData >> 4) & 0x01;


				if (APx0 < 40) /* not in side panel */
				{
#if TUXTXT_DEBUG
					if (dumpl25)
						printf("  Object Invocation %5s %8s S%xP%xT%x%c %c#%03d\n---",
								 ObjectSource[(iAddress >> 3) & 0x03], ObjectType[iMode & 0x03],
								 subp,
							 packet,
								 triplet,
								 "LH"[high], /* low/high */
								 "PCD"[triplet % 3],
								 8*packet + 2*(triplet-1)/3 + 1);
#endif
					tuxtxt_eval_NumberedObject((iAddress & 0x08) ? tuxtxt_cache.gpop : tuxtxt_cache.pop, subp, packet, triplet, high, &APx, &APy, &APx0, &APy0, page_char,page_atrb);
#if TUXTXT_DEBUG
					if (dumpl25)
						printf("---");
#endif
				}
#if TUXTXT_DEBUG
				else if (dumpl25)
					printf("Object Invocation for Side Panel - ignored");
#endif
			}
			else if (iAddress & 0x08)	/* local: eval invoked object */
			{
				unsigned char APx = 0, APy = 0;
				unsigned char APx0 = *pAPx0 + *pAPx + tuxtxt_cache.tAPx, APy0 = *pAPy0 + *pAPy + tuxtxt_cache.tAPy;
				int descode = ((iAddress & 0x01) << 3) | (iData >> 4);
				int triplet = iData & 0x0f;

				if (APx0 < 40) /* not in side panel */
				{
#if TUXTXT_DEBUG
					if (dumpl25)
						printf("  local Object Invocation %5s %8s D%xT%x:\n---",
								 ObjectSource[(iAddress >> 3) & 0x03], ObjectType[iMode & 0x03], descode, triplet);
#endif
					tuxtxt_eval_object(13 * 23 + 13 * descode + triplet, pstCachedPage, &APx, &APy, &APx0, &APy0, (tObjType)(triplet % 3), pagedata, page_char, page_atrb);
#if TUXTXT_DEBUG
					if (dumpl25)
						printf("---");
#endif
				}
#if TUXTXT_DEBUG
				else if (dumpl25)
					printf("local Object Invocation for Side Panel - ignored");
#endif
			}
			break;
		case 0x15:
		case 0x16:
		case 0x17:
			if (0 == (iAddress & 0x08))	/* Object Definition illegal or only level 3.5 */
				break; /* ignore */
#if TUXTXT_DEBUG
			if (dumpl25)
			{
				printf("  Object Definition %8s", ObjectType[iMode & 0x03]);
				{ /* *POP */
					int triplet = 3 * ((iData >> 5) & 0x03) + (iMode & 0x03);
					int packet = (iAddress & 0x03) + 1;
					printf("  S%xP%xT%x%c %c#%03d",
							 iData & 0x0f,	/* subpage */
							 packet,
							 triplet,
							 "LH"[(iData >> 4) & 0x01], /* low/high */
							 "PCD"[triplet % 3],
							 8*packet + 2*(triplet-1)/3 + 1);
				}
				{ /* local */
					int descode = ((iAddress & 0x03) << 3) | (iData >> 4);
					int triplet = iData & 0x0f;
					printf("  D%xT%x", descode, triplet);
				}
				putchar('\n');
			}
#endif
			tuxtxt_cache.tAPx = tuxtxt_cache.tAPy = 0;
			*endcol = -1;
			return 0xFF; /* termination by object definition */
			break;
		case 0x18:
			if (0 == (iData & 0x10)) /* DRCS Mode reserved or only level 3.5 */
				break; /* ignore */
#if TUXTXT_DEBUG
			if (dumpl25)
				printf("  %cDRCS S%x", (iData & 0x40) ? ' ' : 'G', iData & 0x0f);	/* subpage */
#endif
			if (iData & 0x40)
				*drcssubp = iData & 0x0f;
			else
				*gdrcssubp = iData & 0x0f;
			break;
		case 0x1f:
#if TUXTXT_DEBUG
			if (dumpl25)
				printf("  Termination Marker %x\n", iData);	/* subpage */
#endif
			tuxtxt_cache.tAPx = tuxtxt_cache.tAPy = 0;
			*endcol = -1;
			return 0x80 | iData; /* explicit termination */
			break;
		default:
			break; /* unsupported or not yet implemented mode: ignore */
		} /* switch (iMode) */
	} /* (iAddress >= 40): row addresses */
#if TUXTXT_DEBUG
	if (dumpl25 && iAddress < 40)
		putchar('\n');
#endif
	if (iAddress < 40 || iMode != 0x10) /* leave temp. AP-Offset unchanged only immediately after definition */
		tuxtxt_cache.tAPx = tuxtxt_cache.tAPy = 0;


	return 0; /* normal exit, no termination */
}
int tuxtxt_setnational(unsigned char sec)
{
	switch (sec)
	{
		case 0x08:
			return NAT_PL; //polish
		case 0x16:
		case 0x36:
			return NAT_TR; //turkish
		case 0x1d:
			return NAT_SR; //serbian, croatian, slovenian
		case 0x20:
		case 0x24:
		case 0x25:
			return NAT_RU; // cyrillic
		case 0x22:
			return NAT_ET; // estonian
		case 0x23:
			return NAT_LV; // latvian, lithuanian
		case 0x37:
			return NAT_GR; // greek
		case 0x47:
		case 0x57:
			// TODO : arabic
			break;
		case 0x55:
			// TODO : hebrew
			break;

	}
	return countryconversiontable[sec & 0x07];
}
/* evaluate level 2.5 information */
void tuxtxt_eval_l25(unsigned char* page_char, tstPageAttr *page_atrb, int hintmode)
{
	memset(tuxtxt_cache.FullRowColor, 0, sizeof(tuxtxt_cache.FullRowColor));
	tuxtxt_cache.FullScrColor = black;
	tuxtxt_cache.colortable = NULL;

	if (!tuxtxt_cache.astCachetable[tuxtxt_cache.page][tuxtxt_cache.subpage])
		return;

#if TUXTXT_DEBUG
	if (dumpl25)
		printf("=== %03x/%02x %d/%d===\n", tuxtxt_cache.page, tuxtxt_cache.subpage,tuxtxt_cache.astCachetable[tuxtxt_cache.page][tuxtxt_cache.subpage]->pageinfo.nationalvalid,tuxtxt_cache.astCachetable[tuxtxt_cache.page][tuxtxt_cache.subpage]->pageinfo.national);
#endif


	/* normal page */
	if (tuxtxt_is_dec(tuxtxt_cache.page))
	{
		unsigned char APx0, APy0, APx, APy;
		tstPageinfo *pi = &(tuxtxt_cache.astCachetable[tuxtxt_cache.page][tuxtxt_cache.subpage]->pageinfo);
		tstCachedPage *pmot = tuxtxt_cache.astCachetable[(tuxtxt_cache.page & 0xf00) | 0xfe][0];
		int p26Received = 0;
		int BlackBgSubst = 0;
		int ColorTableRemapping = 0;


		tuxtxt_cache.pop = tuxtxt_cache.gpop = tuxtxt_cache.drcs = tuxtxt_cache.gdrcs = 0;


		if (pi->ext)
		{
			tstExtData *e = pi->ext;

			if (e->p26[0])
				p26Received = 1;

			if (e->p27)
			{
				tstp27 *p27 = e->p27;
				if (p27[0].l25)
					tuxtxt_cache.gpop = p27[0].page;
				if (p27[1].l25)
					tuxtxt_cache.pop = p27[1].page;
				if (p27[2].l25)
					tuxtxt_cache.gdrcs = p27[2].page;
				if (p27[3].l25)
					tuxtxt_cache.drcs = p27[3].page;
			}

			if (e->p28Received)
			{
				tuxtxt_cache.colortable = e->bgr;
				BlackBgSubst = e->BlackBgSubst;
				ColorTableRemapping = e->ColorTableRemapping;
				memset(tuxtxt_cache.FullRowColor, e->DefRowColor, sizeof(tuxtxt_cache.FullRowColor));
				tuxtxt_cache.FullScrColor = e->DefScreenColor;
				tuxtxt_cache.national_subset = tuxtxt_setnational(e->DefaultCharset);
				tuxtxt_cache.national_subset_secondary = tuxtxt_setnational(e->SecondCharset);
#if TUXTXT_DEBUG
				if (dumpl25)
				{
					int c; /* color */
					printf("p28/0: DefCharset %02x Sec %02x SidePanel %c%c%x DefScrCol %02x DefRowCol %02x BlBgSubst %x Map %x\n CBGR",
							 e->DefaultCharset,
							 e->SecondCharset,
							 e->LSP ? (e->SPL25 ? 'L' : 'l') : '-',	/* left panel (small: only in level 3.5) */
							 e->RSP ? (e->SPL25 ? 'R' : 'r') : '-',	/* right panel (small: only in level 3.5) */
							 e->LSPColumns,
							 e->DefScreenColor,
							 e->DefRowColor,
							 e->BlackBgSubst,
							 e->ColorTableRemapping);
					for (c = 0; c < 16; c++)
						printf(" %x%03x", c, e->bgr[c]);
					putchar('\n');
				}
#endif
			} /* e->p28Received */
		}

		if (!tuxtxt_cache.colortable && tuxtxt_cache.astP29[tuxtxt_cache.page >> 8])
		{
			tstExtData *e = tuxtxt_cache.astP29[tuxtxt_cache.page >> 8];
			tuxtxt_cache.colortable = e->bgr;
			BlackBgSubst = e->BlackBgSubst;
			ColorTableRemapping = e->ColorTableRemapping;
			memset(tuxtxt_cache.FullRowColor, e->DefRowColor, sizeof(tuxtxt_cache.FullRowColor));
			tuxtxt_cache.FullScrColor = e->DefScreenColor;
			tuxtxt_cache.national_subset = tuxtxt_setnational(e->DefaultCharset);
			tuxtxt_cache.national_subset_secondary = tuxtxt_setnational(e->SecondCharset);
#if TUXTXT_DEBUG
			if (dumpl25)
			{
				int c; /* color */
				printf("p29/0: DefCharset %02x Sec %02x SidePanel %c%c%x DefScrCol %02x DefRowCol %02x BlBgSubst %x Map %x\n CBGR",
						 e->DefaultCharset,
						 e->SecondCharset,
						 e->LSP ? (e->SPL25 ? 'L' : 'l') : '-',	/* left panel (small: only in level 3.5) */
						 e->RSP ? (e->SPL25 ? 'R' : 'r') : '-',	/* right panel (small: only in level 3.5) */
						 e->LSPColumns,
						 e->DefScreenColor,
						 e->DefRowColor,
						 e->BlackBgSubst,
						 e->ColorTableRemapping);
				for (c = 0; c < 16; c++)
					printf(" %x%03x", c, e->bgr[c]);
				putchar('\n');
			}
#endif

		}

		if (ColorTableRemapping)
		{
			int i;
			for (i = 0; i < 25*40; i++)
			{
				page_atrb[i].fg += MapTblFG[ColorTableRemapping - 1];
				if (!BlackBgSubst || page_atrb[i].bg != black || page_atrb[i].IgnoreAtBlackBgSubst)
					page_atrb[i].bg += MapTblBG[ColorTableRemapping - 1];
			}
		}

		/* determine ?pop/?drcs from MOT */
		if (pmot)
		{
			unsigned char pmot_data[23*40];
			tuxtxt_decompress_page((tuxtxt_cache.page & 0xf00) | 0xfe,0,pmot_data);

			unsigned char *p = pmot_data; /* start of link data */
			int o = 2 * (((tuxtxt_cache.page & 0xf0) >> 4) * 10 + (tuxtxt_cache.page & 0x0f));	/* offset of links for current page */
			int opop = p[o] & 0x07;	/* index of POP link */
			int odrcs = p[o+1] & 0x07;	/* index of DRCS link */
			unsigned char obj[3*4*4]; // types* objects * (triplet,packet,subp,high)
			unsigned char type,ct, tstart = 4*4;
			memset(obj,0,sizeof(obj));


			if (p[o] & 0x08) /* GPOP data used */
			{
				if (!tuxtxt_cache.gpop || !(p[18*40] & 0x08)) /* no p27 data or higher prio of MOT link */
				{
					tuxtxt_cache.gpop = ((p[18*40] << 8) | (p[18*40+1] << 4) | p[18*40+2]) & 0x7ff;
					if ((tuxtxt_cache.gpop & 0xff) == 0xff)
						tuxtxt_cache.gpop = 0;
					else
					{
						if (tuxtxt_cache.gpop < 0x100)
							tuxtxt_cache.gpop += 0x800;
						if (!p26Received)
						{
							ct=2;
							while (ct)
							{
								ct--;
								type = (p[18*40+5] >> 2*ct) & 0x03;

								if (type == 0) continue;
							    obj[(type-1)*(tstart)+ct*4  ] = 3 * ((p[18*40+7+ct*2] >> 1) & 0x03) + type; //triplet
							    obj[(type-1)*(tstart)+ct*4+1] = ((p[18*40+7+ct*2] & 0x08) >> 3) + 1       ; //packet
							    obj[(type-1)*(tstart)+ct*4+2] = p[18*40+6+ct*2] & 0x0f                    ; //subp
							    obj[(type-1)*(tstart)+ct*4+3] = p[18*40+7+ct*2] & 0x01                    ; //high

#if TUXTXT_DEBUG
								if (dumpl25)
									printf("GPOP  DefObj%d S%xP%xT%x%c %c#%03d\n"
										,2-ct
										, obj[(type-1)*(tstart)+ct*4+2]
										, obj[(type-1)*(tstart)+ct*4+1]
										, obj[(type-1)*(tstart)+ct*4]
										, "LH"[obj[(type-1)*(tstart)+ct*4+3]]
										, "-CDP"[type]
										, 8*(obj[(type-1)*(tstart)+ct*4+1]-1) + 2*(obj[(type-1)*(tstart)+ct*4]-1)/3 + 1);
#endif
							}
						}
					}
				}
			}
			if (opop) /* POP data used */
			{
				opop = 18*40 + 10*opop;	/* offset to POP link */
				if (!tuxtxt_cache.pop || !(p[opop] & 0x08)) /* no p27 data or higher prio of MOT link */
				{
					tuxtxt_cache.pop = ((p[opop] << 8) | (p[opop+1] << 4) | p[opop+2]) & 0x7ff;
					if ((tuxtxt_cache.pop & 0xff) == 0xff)
						tuxtxt_cache.pop = 0;
					else
					{
						if (tuxtxt_cache.pop < 0x100)
							tuxtxt_cache.pop += 0x800;
						if (!p26Received)
						{
							ct=2;
							while (ct)
							{
								ct--;
								type = (p[opop+5] >> 2*ct) & 0x03;

								if (type == 0) continue;
							    obj[(type-1)*(tstart)+(ct+2)*4  ] = 3 * ((p[opop+7+ct*2] >> 1) & 0x03) + type; //triplet
							    obj[(type-1)*(tstart)+(ct+2)*4+1] = ((p[opop+7+ct*2] & 0x08) >> 3) + 1       ; //packet
							    obj[(type-1)*(tstart)+(ct+2)*4+2] = p[opop+6+ct*2]                           ; //subp
							    obj[(type-1)*(tstart)+(ct+2)*4+3] = p[opop+7+ct*2] & 0x01                    ; //high
#if TUXTXT_DEBUG
								if (dumpl25)
									printf("POP  DefObj%d S%xP%xT%x%c %c#%03d\n"
										, 2-ct
										, obj[(type-1)*(tstart)+(ct+2)*4+2]
										, obj[(type-1)*(tstart)+(ct+2)*4+1]
										, obj[(type-1)*(tstart)+(ct+2)*4]
										, "LH"[obj[(type-1)*(tstart)+(ct+2)*4+3]]
										, "-CDP"[type], 8*(obj[(type-1)*(tstart)+(ct+2)*4+1]-1) + 2*(obj[(type-1)*(tstart)+(ct+2)*4]-1)/3 + 1);
#endif
							}
						}
					}
				}
			}
			// eval default objects in correct order
			for (ct = 0; ct < 12; ct++)
			{
#if TUXTXT_DEBUG
								if (dumpl25)
									printf("eval  DefObjs : %d S%xP%xT%x%c %c#%03d\n"
										, ct
										, obj[ct*4+2]
										, obj[ct*4+1]
										, obj[ct*4]
										, "LH"[obj[ct*4+3]]
										, "-CDP"[obj[ct*4 % 3]]
										, 8*(obj[ct*4+1]-1) + 2*(obj[ct*4]-1)/3 + 1);
#endif
				if (obj[ct*4] != 0)
				{
					APx0 = APy0 = APx = APy = tuxtxt_cache.tAPx = tuxtxt_cache.tAPy = 0;
					tuxtxt_eval_NumberedObject(ct % 4 > 1 ? tuxtxt_cache.pop : tuxtxt_cache.gpop, obj[ct*4+2], obj[ct*4+1], obj[ct*4], obj[ct*4+3], &APx, &APy, &APx0, &APy0, page_char, page_atrb);
				}
			}

			if (p[o+1] & 0x08) /* GDRCS data used */
			{
				if (!tuxtxt_cache.gdrcs || !(p[20*40] & 0x08)) /* no p27 data or higher prio of MOT link */
				{
					tuxtxt_cache.gdrcs = ((p[20*40] << 8) | (p[20*40+1] << 4) | p[20*40+2]) & 0x7ff;
					if ((tuxtxt_cache.gdrcs & 0xff) == 0xff)
						tuxtxt_cache.gdrcs = 0;
					else if (tuxtxt_cache.gdrcs < 0x100)
						tuxtxt_cache.gdrcs += 0x800;
				}
			}
			if (odrcs) /* DRCS data used */
			{
				odrcs = 20*40 + 4*odrcs;	/* offset to DRCS link */
				if (!tuxtxt_cache.drcs || !(p[odrcs] & 0x08)) /* no p27 data or higher prio of MOT link */
				{
					tuxtxt_cache.drcs = ((p[odrcs] << 8) | (p[odrcs+1] << 4) | p[odrcs+2]) & 0x7ff;
					if ((tuxtxt_cache.drcs & 0xff) == 0xff)
						tuxtxt_cache.drcs = 0;
					else if (tuxtxt_cache.drcs < 0x100)
						tuxtxt_cache.drcs += 0x800;
				}
			}
			if (tuxtxt_cache.astCachetable[tuxtxt_cache.gpop][0])
				tuxtxt_cache.astCachetable[tuxtxt_cache.gpop][0]->pageinfo.function = FUNC_GPOP;
			if (tuxtxt_cache.astCachetable[tuxtxt_cache.pop][0])
				tuxtxt_cache.astCachetable[tuxtxt_cache.pop][0]->pageinfo.function = FUNC_POP;
			if (tuxtxt_cache.astCachetable[tuxtxt_cache.gdrcs][0])
				tuxtxt_cache.astCachetable[tuxtxt_cache.gdrcs][0]->pageinfo.function = FUNC_GDRCS;
			if (tuxtxt_cache.astCachetable[tuxtxt_cache.drcs][0])
				tuxtxt_cache.astCachetable[tuxtxt_cache.drcs][0]->pageinfo.function = FUNC_DRCS;
		} /* if mot */

#if TUXTXT_DEBUG
		if (dumpl25)
			printf("gpop %03x pop %03x gdrcs %03x drcs %03x p28/0: Func %x Natvalid %x Nat %x Box %x\n",
					 tuxtxt_cache.gpop, tuxtxt_cache.pop, tuxtxt_cache.gdrcs, tuxtxt_cache.drcs,
					 pi->function, pi->nationalvalid, pi->national, pi->boxed);
#endif

		/* evaluate local extension data from p26 */
		if (p26Received)
		{
#if TUXTXT_DEBUG
			if (dumpl25)
				printf("p26/x:\n");
#endif
			APx0 = APy0 = APx = APy = tuxtxt_cache.tAPx = tuxtxt_cache.tAPy = 0;
			tuxtxt_eval_object(13 * (23-2 + 2), tuxtxt_cache.astCachetable[tuxtxt_cache.page][tuxtxt_cache.subpage], &APx, &APy, &APx0, &APy0, OBJ_ACTIVE, &page_char[40], page_char, page_atrb); /* 1st triplet p26/0 */
		}

		{
			int r, c;
			int o = 0;


			for (r = 0; r < 25; r++)
				for (c = 0; c < 39; c++)
				{
					if (BlackBgSubst && page_atrb[o].bg == black && !(page_atrb[o].IgnoreAtBlackBgSubst))
					{
						if (tuxtxt_cache.FullRowColor[r] == 0x08)
							page_atrb[o].bg = tuxtxt_cache.FullScrColor;
						else
							page_atrb[o].bg = tuxtxt_cache.FullRowColor[r];
					}
					o++;
				}
		}

		if (!hintmode)
		{
			int i;
			for (i = 0; i < 25*40; i++)
			{
				if (page_atrb[i].concealed) page_atrb[i].fg = page_atrb[i].bg;
			}
		}

	} /* is_dec(page) */

}

/******************************************************************************
 * DecodePage                                                                 *
 ******************************************************************************/

tstPageinfo* tuxtxt_DecodePage(int showl25, // 1=decode Level2.5-graphics
				 unsigned char* page_char, // page buffer, min. 24*40 
				 tstPageAttr *page_atrb, // attribut buffer, min 24*40
				 int hintmode,// 1=show hidden information
				 int showflof // 1=decode FLOF-line
				 )
{
	int row, col;
	int hold, dhset;
	int foreground, background, doubleheight, doublewidth, charset, mosaictype, IgnoreAtBlackBgSubst, concealed, flashmode, boxwin;
	unsigned char held_mosaic, *p;
	tstCachedPage *pCachedPage;

	/* copy page to decode buffer */
	if (tuxtxt_cache.subpagetable[tuxtxt_cache.page] == 0xff) /* not cached: do nothing */
		return NULL;
	if (tuxtxt_cache.zap_subpage_manual)
		pCachedPage = tuxtxt_cache.astCachetable[tuxtxt_cache.page][tuxtxt_cache.subpage];
	else
		pCachedPage = tuxtxt_cache.astCachetable[tuxtxt_cache.page][tuxtxt_cache.subpagetable[tuxtxt_cache.page]];
	if (!pCachedPage)	/* not cached: do nothing */
		return NULL;

	tuxtxt_decompress_page(tuxtxt_cache.page,tuxtxt_cache.subpage,&page_char[40]);

	memcpy(&page_char[8], pCachedPage->p0, 24); /* header line without timestring */

	tstPageinfo* pageinfo = &(pCachedPage->pageinfo);
	if (pageinfo->p24)
		memcpy(&page_char[24*40], pageinfo->p24, 40); /* line 25 for FLOF */

	/* copy timestring */
	memcpy(&page_char[32], &tuxtxt_cache.timestring, 8);

	int boxed;
	/* check for newsflash & subtitle */
	if (pageinfo->boxed && tuxtxt_is_dec(tuxtxt_cache.page))
		boxed = 1;
	else
		boxed = 0;


	/* modify header */
	if (boxed)
		memset(page_char, ' ', 40);
	else
	{
		memset(page_char, ' ', 8);
		tuxtxt_hex2str(page_char+3, tuxtxt_cache.page);
		if (tuxtxt_cache.subpage)
		{
			*(page_char+4) ='/';
			*(page_char+5) ='0';
			tuxtxt_hex2str(page_char+6, tuxtxt_cache.subpage);
		}

	}

	if (!tuxtxt_is_dec(tuxtxt_cache.page))
	{
		tstPageAttr atr = { white  , black , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f};
		if (pageinfo->function == FUNC_MOT) /* magazine organization table */
		{
#if TUXTXT_DEBUG
			printf("TuxTxt <decoding MOT %03x/%02x %d>\n", tuxtxt_cache.page, tuxtxt_cache.subpage, pageinfo->function);
#endif
			for (col = 0; col < 24*40; col++)
				page_atrb[col] = atr;
			for (col = 40; col < 24*40; col++)
				page_char[col] = number2char(page_char[col]);
			boxed = 0;
			return pageinfo; /* don't interpret irregular pages */
		}
		else if (pageinfo->function == FUNC_GPOP || pageinfo->function == FUNC_POP) /* object definitions */
		{
#if TUXTXT_DEBUG
			printf("TuxTxt <decoding *POP %03x/%02x %d>\n", tuxtxt_cache.page, tuxtxt_cache.subpage, pageinfo->function);
#endif
			for (col = 0; col < 24*40; col++)
				page_atrb[col] = atr;
			p = page_char + 40;
			for (row = 1; row < 12; row++)
			{
				*p++ = number2char(row); /* first column: number (0-9, A-..) */
				for (col = 1; col < 40; col += 3)
				{
					int d = deh24(p);
					if (d < 0)
					{
						memcpy(p, "???", 3);
					p += 3;
					}
					else
					{
						*p++ = number2char((d >> 6) & 0x1f); /* mode */
						*p++ = number2char(d & 0x3f); /* address */
						*p++ = number2char((d >> 11) & 0x7f); /* data */
					}
				}
			}
			boxed = 0;
			return pageinfo; /* don't interpret irregular pages */
		}
		else if (pageinfo->function == FUNC_GDRCS || pageinfo->function == FUNC_DRCS) /* character definitions */
		{
			boxed = 0;
			return pageinfo; /* don't interpret irregular pages */
		}
		else
		{
			int i;
			int h, parityerror = 0;

			for (i = 0; i < 8; i++)
				page_atrb[i] = atr;

			/* decode parity/hamming */
			for (i = 40; i < sizeof(page_char); i++)
			{
				page_atrb[i] = atr;
				p = page_char + i;
				h = dehamming[*p];
				if (parityerror && h != 0xFF)	/* if no regular page (after any parity error) */
					tuxtxt_hex2str(p, h);	/* first try dehamming */
				else
				{
					if (*p == ' ' || deparity[*p] != ' ') /* correct parity */
						*p &= 127;
					else
					{
						parityerror = 1;
						if (h != 0xFF)	/* first parity error: try dehamming */
							tuxtxt_hex2str(p, h);
						else
							*p = ' ';
					}
				}
			}
			if (parityerror)
			{
				boxed = 0;
				return pageinfo; /* don't interpret irregular pages */
			}
		}
	}

	/* decode */
	for (row = 0;
		  row < ((showflof && pageinfo->p24) ? 25 : 24);
		  row++)
	{
		/* start-of-row default conditions */
		foreground   = white;
		background   = black;
		doubleheight = 0;
		doublewidth  = 0;
		charset      = C_G0P;
		mosaictype   = 0;
		concealed    = 0;
		flashmode    = 0;
		hold         = 0;
		boxwin		 = 0;
		held_mosaic  = ' ';
		dhset        = 0;
		IgnoreAtBlackBgSubst = 0;

		if (boxed && memchr(&page_char[row*40], start_box, 40) == 0)
		{
			foreground = transp;
			background = transp;
		}

		for (col = 0; col < 40; col++)
		{
			int index = row*40 + col;

			page_atrb[index].fg = foreground;
			page_atrb[index].bg = background;
			page_atrb[index].charset = charset;
			page_atrb[index].doubleh = doubleheight;
			page_atrb[index].doublew = (col < 39 ? doublewidth : 0);
			page_atrb[index].IgnoreAtBlackBgSubst = IgnoreAtBlackBgSubst;
			page_atrb[index].concealed = concealed;
			page_atrb[index].flashing  = flashmode;
			page_atrb[index].boxwin    = boxwin;
			page_atrb[index].inverted  = 0; // only relevant for Level 2.5
			page_atrb[index].underline = 0; // only relevant for Level 2.5
			page_atrb[index].diacrit   = 0; // only relevant for Level 2.5
			page_atrb[index].setX26    = 0; // only relevant for Level 2.5
			page_atrb[index].setG0G2   = 0x3f; // only relevant for Level 2.5

			if (page_char[index] < ' ')
			{
				switch (page_char[index])
				{
				case alpha_black:
				case alpha_red:
				case alpha_green:
				case alpha_yellow:
				case alpha_blue:
				case alpha_magenta:
				case alpha_cyan:
				case alpha_white:
					concealed = 0;
					foreground = page_char[index] - alpha_black + black;
					if (col == 0 && page_char[index] == alpha_white)
						page_atrb[index].fg = black; // indicate level 1 color change on column 0; (hack)
					charset = C_G0P;
					break;

				case flash:
					flashmode = 1;
					break;
				case steady:
					flashmode = 0;
					page_atrb[index].flashing = 0;
					break;
				case end_box:
					boxwin = 0;
					IgnoreAtBlackBgSubst = 0;
/*					if (boxed)
					{
						foreground = transp;
						background = transp;
						IgnoreAtBlackBgSubst = 0;
					}
*/
					break;

				case start_box:
					if (!boxwin)
					{
						boxwin = 1;
						//background = 0x08;
					}
/*					if (boxed)
					{
						int rowstart = row * 40;
						if (col > 0)
							memset(&page_char[rowstart], ' ', col);
						for (clear = 0; clear < col; clear++)
						{
							page_atrb[rowstart + clear].fg = page_atrb[rowstart + clear].bg = transp;
							page_atrb[rowstart + clear].IgnoreAtBlackBgSubst = 0;
						}
					}
*/
					break;

				case normal_size:
					doubleheight = 0;
					doublewidth = 0;
					page_atrb[index].doubleh = doubleheight;
					page_atrb[index].doublew = doublewidth;
					break;

				case double_height:
					if (row < 23)
					{
						doubleheight = 1;
						dhset = 1;
					}
					doublewidth = 0;

					break;

				case double_width:
					if (col < 39)
						doublewidth = 1;
					doubleheight = 0;
					break;

				case double_size:
					if (row < 23)
					{
						doubleheight = 1;
						dhset = 1;
					}
					if (col < 39)
						doublewidth = 1;
					break;

				case mosaic_black:
				case mosaic_red:
				case mosaic_green:
				case mosaic_yellow:
				case mosaic_blue:
				case mosaic_magenta:
				case mosaic_cyan:
				case mosaic_white:
					concealed = 0;
					foreground = page_char[index] - mosaic_black + black;
					charset = mosaictype ? C_G1S : C_G1C;
					break;

				case conceal:
					page_atrb[index].concealed = 1;
					concealed = 1;
					if (!hintmode)
					{
						foreground = background;
						page_atrb[index].fg = foreground;
					}
					break;

				case contiguous_mosaic:
					mosaictype = 0;
					if (charset == C_G1S)
					{
						charset = C_G1C;
						page_atrb[index].charset = charset;
					}
					break;

				case separated_mosaic:
					mosaictype = 1;
					if (charset == C_G1C)
					{
						charset = C_G1S;
						page_atrb[index].charset = charset;
					}
					break;

				case esc:
					if (charset == C_G0P)
						charset = C_G0S;
					else if (charset == C_G0S)
						charset = C_G0P;
					break;

				case black_background:
					background = black;
					IgnoreAtBlackBgSubst = 0;
					page_atrb[index].bg = background;
					page_atrb[index].IgnoreAtBlackBgSubst = IgnoreAtBlackBgSubst;
					break;

				case new_background:
					background = foreground;
					if (background == black)
						IgnoreAtBlackBgSubst = 1;
					else
						IgnoreAtBlackBgSubst = 0;
					page_atrb[index].bg = background;
					page_atrb[index].IgnoreAtBlackBgSubst = IgnoreAtBlackBgSubst;
					break;

				case hold_mosaic:
					hold = 1;
					break;

				case release_mosaic:
					hold = 2;
					break;
				}

				/* handle spacing attributes */
				if (hold && (page_atrb[index].charset == C_G1C || page_atrb[index].charset == C_G1S))
					page_char[index] = held_mosaic;
				else
					page_char[index] = ' ';

				if (hold == 2)
					hold = 0;
			}
			else /* char >= ' ' */
			{
				/* set new held-mosaic char */
				if ((charset == C_G1C || charset == C_G1S) &&
					 ((page_char[index]&0xA0) == 0x20))
					held_mosaic = page_char[index];
				if (page_atrb[index].doubleh)
					page_char[index + 40] = 0xFF;

			}
			if (!(charset == C_G1C || charset == C_G1S))
				held_mosaic = ' '; /* forget if outside mosaic */

		} /* for col */

		/* skip row if doubleheight */
		if (row < 23 && dhset)
		{
			for (col = 0; col < 40; col++)
			{
				int index = row*40 + col;
				page_atrb[index+40].bg = page_atrb[index].bg;
				page_atrb[index+40].fg = white;
				if (!page_atrb[index].doubleh)
					page_char[index+40] = ' ';
				page_atrb[index+40].flashing = 0;
				page_atrb[index+40].charset = C_G0P;
				page_atrb[index+40].doubleh = 0;
				page_atrb[index+40].doublew = 0;
				page_atrb[index+40].IgnoreAtBlackBgSubst = 0;
				page_atrb[index+40].concealed = 0;
				page_atrb[index+40].flashing  = 0;
				page_atrb[index+40].boxwin    = page_atrb[index].boxwin;
			}
			row++;
		}
	} /* for row */
	tuxtxt_cache.FullScrColor = black;

	if (showl25)
		tuxtxt_eval_l25(page_char,page_atrb, hintmode);


	/* handle Black Background Color Substitution and transparency (CLUT1#0) */
	{
		int r, c;
		int o = 0;
		char bitmask ;



		for (r = 0; r < 25; r++)
		{
			for (c = 0; c < 40; c++)
			{
				bitmask = (page_atrb[o].bg == 0x08 ? 0x08 : 0x00) | (tuxtxt_cache.FullRowColor[r] == 0x08 ? 0x04 : 0x00) | (page_atrb[o].boxwin <<1) | boxed;
				switch (bitmask)
				{
					case 0x08:
					case 0x0b:
						if (tuxtxt_cache.FullRowColor[r] == 0x08)
							page_atrb[o].bg = tuxtxt_cache.FullScrColor;
						else
							page_atrb[o].bg = tuxtxt_cache.FullRowColor[r];
						break;
					case 0x01:
					case 0x05:
					case 0x09:
					case 0x0a:
					case 0x0c:
					case 0x0d:
					case 0x0e:
					case 0x0f:
						page_atrb[o].bg = transp;
						break;
				}
				bitmask = (page_atrb[o].fg  == 0x08 ? 0x08 : 0x00) | (tuxtxt_cache.FullRowColor[r] == 0x08 ? 0x04 : 0x00) | (page_atrb[o].boxwin <<1) | boxed;
				switch (bitmask)
				{
					case 0x08:
					case 0x0b:
						if (tuxtxt_cache.FullRowColor[r] == 0x08)
							page_atrb[o].fg = tuxtxt_cache.FullScrColor;
						else
							page_atrb[o].fg = tuxtxt_cache.FullRowColor[r];
						break;
					case 0x01:
					case 0x05:
					case 0x09:
					case 0x0a:
					case 0x0c:
					case 0x0d:
					case 0x0e:
					case 0x0f:
						page_atrb[o].fg = transp;
						break;
				}
				o++;
			}
		}
	}
	return pageinfo;
}
void tuxtxt_FillRect(unsigned char *lfb, int xres, int x, int y, int w, int h, int color)
{
	unsigned char *p = lfb + x + y * xres;

	if (w > 0)
		for ( ; h > 0 ; h--)
		{
			memset(p, color, w);
			p += xres;
		}
}

void tuxtxt_RenderDRCS(int xres,
	unsigned char *s,	/* pointer to char data, parity undecoded */
	unsigned char *d,	/* pointer to frame buffer of top left pixel */
	unsigned char *ax, /* array[0..12] of x-offsets, array[0..10] of y-offsets for each pixel */
	unsigned char fgcolor, unsigned char bgcolor)
{
	int bit, x, y;
	unsigned char *ay = ax + 13; /* array[0..10] of y-offsets for each pixel */

	for (y = 0; y < 10; y++) /* 10*2 bytes a 6 pixels per char definition */
	{
		unsigned char c1 = deparity[*s++];
		unsigned char c2 = deparity[*s++];
		int h = ay[y+1] - ay[y];

		if (!h)
			continue;
		if (((c1 == ' ') && (*(s-2) != ' ')) || ((c2 == ' ') && (*(s-1) != ' '))) /* parity error: stop decoding FIXME */
			return;
		for (bit = 0x20, x = 0;
			  bit;
			  bit >>= 1, x++)	/* bit mask (MSB left), column counter */
		{
			int i, f1, f2;

			f1 = (c1 & bit) ? fgcolor : bgcolor;
			f2 = (c2 & bit) ? fgcolor : bgcolor;
			for (i = 0; i < h; i++)
			{
				if (ax[x+1] > ax[x])
					memset(d + ax[x], f1, ax[x+1] - ax[x]);
				if (ax[x+7] > ax[x+6])
					memset(d + ax[x+6], f2, ax[x+7] - ax[x+6]); /* 2nd byte 6 pixels to the right */
				d += xres;
			}
			d -= h * xres;
		}
		d += h * xres;
	}
}


void tuxtxt_DrawVLine(unsigned char *lfb, int xres, int x, int y, int l, int color)
{
	unsigned char *p = lfb + x + y * xres;

	for ( ; l > 0 ; l--)
	{
		*p = color;
		p += xres;
	}
}

void tuxtxt_DrawHLine(unsigned char* lfb,int xres,int x, int y, int l, int color)
{
	if (l > 0)
		memset(lfb + x + y * xres, color, l);
}

void tuxtxt_FillRectMosaicSeparated(unsigned char *lfb, int xres,int x, int y, int w, int h, int fgcolor, int bgcolor, int set)
{
	tuxtxt_FillRect(lfb,xres,x, y, w, h, bgcolor);
	if (set)
	{
		tuxtxt_FillRect(lfb,xres,x+1, y+1, w-2, h-2, fgcolor);
	}
}

void tuxtxt_FillTrapez(unsigned char *lfb, int xres,int x0, int y0, int l0, int xoffset1, int h, int l1, int color)
{
	unsigned char *p = lfb + x0 + y0 * xres;
	int xoffset, l;
	int yoffset;

	for (yoffset = 0; yoffset < h; yoffset++)
	{
		l = l0 + ((l1-l0) * yoffset + h/2) / h;
		xoffset = (xoffset1 * yoffset + h/2) / h;
		if (l > 0)
			memset(p + xoffset, color, l);
		p += xres;
	}
}
void tuxtxt_FlipHorz(unsigned char *lfb, int xres,int x, int y, int w, int h)
{
	unsigned char buf[w];
	unsigned char *p = lfb + x + y * xres;
	int w1,h1;

	for (h1 = 0 ; h1 < h ; h1++)
	{
		memcpy(buf,p,w);
		for (w1 = 0 ; w1 < w ; w1++)
		{
			*(p+w1) = buf[w-(w1+1)];
		}
		p += xres;
	}
}
void tuxtxt_FlipVert(unsigned char *lfb, int xres,int x, int y, int w, int h)
{
	unsigned char buf[w];
	unsigned char *p = lfb + x + y * xres, *p1, *p2;
	int h1;

	for (h1 = 0 ; h1 < h/2 ; h1++)
	{
		p1 = (p+(h1*xres));
		p2 = (p+(h-(h1+1))*xres);
		memcpy(buf,p1,w);
		memcpy(p1,p2,w);
		memcpy(p2,buf,w);
	}
}

int tuxtxt_ShapeCoord(int param, int curfontwidth, int curfontheight)
{
	switch (param)
	{
	case S_W13:
		return curfontwidth/3;
	case S_W12:
		return curfontwidth/2;
	case S_W23:
		return curfontwidth*2/3;
	case S_W11:
		return curfontwidth;
	case S_WM3:
		return curfontwidth-3;
	case S_H13:
		return curfontheight/3;
	case S_H12:
		return curfontheight/2;
	case S_H23:
		return curfontheight*2/3;
	case S_H11:
		return curfontheight;
	default:
		return param;
	}
}

void tuxtxt_DrawShape(unsigned char *lfb, int xres,int x, int y, int shapenumber, int curfontwidth, int fontheight, int curfontheight, int fgcolor, int bgcolor, int clear)
{
	if (shapenumber < 0x20 || shapenumber > 0x7e || (shapenumber == 0x7e && clear))
		return;

	unsigned char *p = aShapes[shapenumber - 0x20];

	if (*p == S_INV)
	{
		int t = fgcolor;
		fgcolor = bgcolor;
		bgcolor = t;
		p++;
	}

	if (clear)
		tuxtxt_FillRect(lfb,xres,x, y, curfontwidth, fontheight, bgcolor);
	while (*p != S_END)
		switch (*p++)
		{
		case S_FHL:
		{
			int offset = tuxtxt_ShapeCoord(*p++, curfontwidth, curfontheight);
			tuxtxt_DrawHLine(lfb,xres,x, y + offset, curfontwidth, fgcolor);
			break;
		}
		case S_FVL:
		{
			int offset = tuxtxt_ShapeCoord(*p++, curfontwidth, curfontheight);
			tuxtxt_DrawVLine(lfb,xres,x + offset, y, fontheight, fgcolor);
			break;
		}
		case S_FLH:
			tuxtxt_FlipHorz(lfb,xres,x,y,curfontwidth, fontheight);
			break;
		case S_FLV:
			tuxtxt_FlipVert(lfb,xres,x,y,curfontwidth, fontheight);
			break;
		case S_BOX:
		{
			int xo = tuxtxt_ShapeCoord(*p++, curfontwidth, curfontheight);
			int yo = tuxtxt_ShapeCoord(*p++, curfontwidth, curfontheight);
			int w = tuxtxt_ShapeCoord(*p++, curfontwidth, curfontheight);
			int h = tuxtxt_ShapeCoord(*p++, curfontwidth, curfontheight);
			tuxtxt_FillRect(lfb,xres,x + xo, y + yo, w, h, fgcolor);
			break;
		}
		case S_TRA:
		{
			int x0 = tuxtxt_ShapeCoord(*p++, curfontwidth, curfontheight);
			int y0 = tuxtxt_ShapeCoord(*p++, curfontwidth, curfontheight);
			int l0 = tuxtxt_ShapeCoord(*p++, curfontwidth, curfontheight);
			int x1 = tuxtxt_ShapeCoord(*p++, curfontwidth, curfontheight);
			int y1 = tuxtxt_ShapeCoord(*p++, curfontwidth, curfontheight);
			int l1 = tuxtxt_ShapeCoord(*p++, curfontwidth, curfontheight);
			tuxtxt_FillTrapez(lfb,xres,x + x0, y + y0, l0, x1-x0, y1-y0, l1, fgcolor);
			break;
		}
		case S_BTR:
		{
			int x0 = tuxtxt_ShapeCoord(*p++, curfontwidth, curfontheight);
			int y0 = tuxtxt_ShapeCoord(*p++, curfontwidth, curfontheight);
			int l0 = tuxtxt_ShapeCoord(*p++, curfontwidth, curfontheight);
			int x1 = tuxtxt_ShapeCoord(*p++, curfontwidth, curfontheight);
			int y1 = tuxtxt_ShapeCoord(*p++, curfontwidth, curfontheight);
			int l1 = tuxtxt_ShapeCoord(*p++, curfontwidth, curfontheight);
			tuxtxt_FillTrapez(lfb,xres,x + x0, y + y0, l0, x1-x0, y1-y0, l1, bgcolor);
			break;
		}
		case S_LNK:
		{
			tuxtxt_DrawShape(lfb,xres,x, y, tuxtxt_ShapeCoord(*p, curfontwidth, curfontheight), curfontwidth, fontheight, curfontheight, fgcolor, bgcolor, 0);
			//p = aShapes[ShapeCoord(*p, curfontwidth, curfontheight) - 0x20];
			break;
		}
		default:
			break;
		}
		
}


/******************************************************************************
 * RenderChar                                                                 *
 ******************************************************************************/

int tuxtxt_RenderChar(unsigned char *lfb, // pointer to render buffer, min. fontheight*2*xres
		      int xres,// length of 1 line in render buffer
		      int Char,// character to render
		      int *pPosX,// left border for rendering relative to *lfb, will be set to right border after rendering
		      int PosY,// vertical position of char in *lfb
		      tstPageAttr *Attribute,// Attributes of Char
		      int zoom,// 1= character will be rendered in double height
		      int curfontwidth,// rendering width of character
		      int curfontwidth2,// rendering width of next character (needed for doublewidth)
		      int fontheight,// height of character
		      int transpmode,// 1= transparent display
		      unsigned char *axdrcs,// width and height of DRCS-chars
		      int ascender // ascender of font
		      )
{
	int Row;
	int bgcolor, fgcolor;
	int factor, xfactor;
	int national_subset_local = tuxtxt_cache.national_subset;
	int ymosaic[4];
	ymosaic[0] = 0; /* y-offsets for 2*3 mosaic */
	ymosaic[1] = (fontheight + 1) / 3;
	ymosaic[2] = (fontheight * 2 + 1) / 3;
	ymosaic[3] = fontheight;


	if (Attribute->setX26)
	{
		national_subset_local = 0; // no national subset
	}

	// G0+G2 set designation
	if (Attribute->setG0G2 != 0x3f)
	{
		switch (Attribute->setG0G2)
		{
			case 0x20 :
			case 0x24 :
			case 0x25 :
				national_subset_local = NAT_RU;
				break;
			case 0x37:
				national_subset_local = NAT_GR;
				break;
				//TODO: arabic and hebrew
			default:
				national_subset_local = countryconversiontable[Attribute->setG0G2 & 0x07];
				break;

		}

	}
	if (Attribute->charset == C_G0S) // use secondary charset
		national_subset_local = tuxtxt_cache.national_subset_secondary;
	if (zoom && Attribute->doubleh)
		factor = 4;
	else if (zoom || Attribute->doubleh)
		factor = 2;
	else
		factor = 1;

	if (Attribute->doublew)
	{
		curfontwidth += curfontwidth2;
		xfactor = 2;
	}
	else
		xfactor = 1;

	if (Char == 0xFF)	/* skip doubleheight chars in lower line */
	{
		*pPosX += curfontwidth;
		return -1;
	}

	/* get colors */
	if (Attribute->inverted)
	{
		int t = Attribute->fg;
		Attribute->fg = Attribute->bg;
		Attribute->bg = t;
	}
	fgcolor = Attribute->fg;
	if (transpmode == 1 && PosY < 24*fontheight)
	{
		if (fgcolor == transp) /* outside boxed elements (subtitles, news) completely transparent */
			bgcolor = transp;
		else
			bgcolor = transp2;
	}
	else
		bgcolor = Attribute->bg;

	/* handle mosaic */
	if ((Attribute->charset == C_G1C || Attribute->charset == C_G1S) &&
		 ((Char&0xA0) == 0x20))
	{
		int w1 = (curfontwidth / 2 ) *xfactor;
		int w2 = (curfontwidth - w1) *xfactor;
		int y;

		Char = (Char & 0x1f) | ((Char & 0x40) >> 1);
		if (Attribute->charset == C_G1S) /* separated mosaic */
			for (y = 0; y < 3; y++)
			{
				tuxtxt_FillRectMosaicSeparated(lfb,xres,*pPosX,      PosY +  ymosaic[y]*factor, w1, (ymosaic[y+1] - ymosaic[y])*factor, fgcolor, bgcolor, Char & 0x01);
				tuxtxt_FillRectMosaicSeparated(lfb,xres,*pPosX + w1, PosY +  ymosaic[y]*factor, w2, (ymosaic[y+1] - ymosaic[y])*factor, fgcolor, bgcolor, Char & 0x02);
				Char >>= 2;
			}
		else
			for (y = 0; y < 3; y++)
			{
				tuxtxt_FillRect(lfb,xres,*pPosX,      PosY + ymosaic[y]*factor, w1, (ymosaic[y+1] - ymosaic[y])*factor, (Char & 0x01) ? fgcolor : bgcolor);
				tuxtxt_FillRect(lfb,xres,*pPosX + w1, PosY + ymosaic[y]*factor, w2, (ymosaic[y+1] - ymosaic[y])*factor, (Char & 0x02) ? fgcolor : bgcolor);
				Char >>= 2;
			}

		*pPosX += curfontwidth;
		return 0;;
	}

	if (Attribute->charset == C_G3)
	{
		if (Char < 0x20 || Char > 0x7d)
		{
			Char = 0x20;
		}
		else
		{
			if (*aShapes[Char - 0x20] == S_CHR)
			{
				unsigned char *p = aShapes[Char - 0x20];
				Char = (*(p+1) <<8) + (*(p+2));
			}
			else if (*aShapes[Char - 0x20] == S_ADT)
			{
				int x,y,f,c;
				unsigned char* p = lfb + *pPosX + PosY* xres;
				for (y=0; y<fontheight;y++)
				{
					for (f=0; f<factor; f++)
					{
						for (x=0; x<curfontwidth*xfactor;x++)
						{
							c = (y&4 ? (x/3)&1 :((x+3)/3)&1);
							*(p+x) = (c ? fgcolor : bgcolor);
						}
						p += xres;
					}
				}
				*pPosX += curfontwidth;
				return 0;
			}
			else
			{
				tuxtxt_DrawShape(lfb,xres,*pPosX, PosY, Char, curfontwidth, fontheight, factor*fontheight, fgcolor, bgcolor,1);
				*pPosX += curfontwidth;
				return 0;
			}
		}
	}
	else if (Attribute->charset >= C_OFFSET_DRCS)
	{

		tstCachedPage *pcache = tuxtxt_cache.astCachetable[(Attribute->charset & 0x10) ? tuxtxt_cache.drcs : tuxtxt_cache.gdrcs][Attribute->charset & 0x0f];
		if (pcache)
		{
			unsigned char drcs_data[23*40];
			tuxtxt_decompress_page((Attribute->charset & 0x10) ? tuxtxt_cache.drcs : tuxtxt_cache.gdrcs,Attribute->charset & 0x0f,drcs_data);
			unsigned char *p;
			if (Char < 23*2)
				p = drcs_data + 20*Char;
			else if (pcache->pageinfo.p24)
				p = pcache->pageinfo.p24 + 20*(Char - 23*2);
			else
			{
				tuxtxt_FillRect(lfb,xres,*pPosX, PosY, curfontwidth, factor*fontheight, bgcolor);
				*pPosX += curfontwidth;
				return 0;
			}
			axdrcs[12] = curfontwidth; /* adjust last x-offset according to position, FIXME: double width */
			tuxtxt_RenderDRCS(xres,p,
						  lfb + *pPosX + PosY * xres,
						  axdrcs, fgcolor, bgcolor);
		}
		else
			tuxtxt_FillRect(lfb,xres,*pPosX, PosY, curfontwidth, factor*fontheight, bgcolor);
		*pPosX += curfontwidth;
		return 0;
	}
	else if (Attribute->charset == C_G2 && Char >= 0x20 && Char <= 0x7F)
	{
		if (national_subset_local == NAT_GR)
			Char = G2table[2][Char-0x20];
		else if (national_subset_local == NAT_RU)
			Char = G2table[1][Char-0x20];
		else
			Char = G2table[0][Char-0x20];

		if (Char == 0x7F)
		{
			tuxtxt_FillRect(lfb,xres,*pPosX, PosY, curfontwidth, factor*ascender, fgcolor);
			tuxtxt_FillRect(lfb,xres,*pPosX, PosY + factor*ascender, curfontwidth, factor*(fontheight-ascender), bgcolor);
			*pPosX += curfontwidth;
			return 0;
		}
	}
	else if (national_subset_local == NAT_GR && Char >= 0x40 && Char <= 0x7E)	/* remap complete areas for greek */
		Char += 0x390 - 0x40;
	else if (national_subset_local == NAT_GR && Char == 0x3c)
		Char = '«';
	else if (national_subset_local == NAT_GR && Char == 0x3e)
		Char = '»';
	else if (national_subset_local == NAT_GR && Char >= 0x23 && Char <= 0x24)
		Char = nationaltable23[NAT_DE][Char-0x23]; /* #$ as in german option */
	else if (national_subset_local == NAT_RU && Char >= 0x40 && Char <= 0x7E) /* remap complete areas for cyrillic */
		Char = G0tablecyrillic[Char-0x20];
	else
	{
		/* load char */
		switch (Char)
		{
		case 0x00:
		case 0x20:
			tuxtxt_FillRect(lfb,xres,*pPosX, PosY, curfontwidth, factor*fontheight, bgcolor);
			*pPosX += curfontwidth;
			return -3;
		case 0x23:
		case 0x24:
			Char = nationaltable23[national_subset_local][Char-0x23];
			break;
		case 0x40:
			Char = nationaltable40[national_subset_local];
			break;
		case 0x5B:
		case 0x5C:
		case 0x5D:
		case 0x5E:
		case 0x5F:
		case 0x60:
			Char = nationaltable5b[national_subset_local][Char-0x5B];
			break;
		case 0x7B:
		case 0x7C:
		case 0x7D:
		case 0x7E:
			Char = nationaltable7b[national_subset_local][Char-0x7B];
			break;
		case 0x7F:
			tuxtxt_FillRect(lfb,xres,*pPosX, PosY , curfontwidth, factor*ascender, fgcolor);
			tuxtxt_FillRect(lfb,xres,*pPosX, PosY + factor*ascender, curfontwidth, factor*(fontheight-ascender), bgcolor);
			*pPosX += curfontwidth;
			return 0;
		case 0xE0: /* |- */
			tuxtxt_DrawHLine(lfb,xres,*pPosX, PosY, curfontwidth, fgcolor);
			tuxtxt_DrawVLine(lfb,xres,*pPosX, PosY +1, fontheight -1, fgcolor);
			tuxtxt_FillRect(lfb,xres,*pPosX +1, PosY +1, curfontwidth-1, fontheight-1, bgcolor);
			*pPosX += curfontwidth;
			return 0;
		case 0xE1: /* - */
			tuxtxt_DrawHLine(lfb,xres,*pPosX, PosY, curfontwidth, fgcolor);
			tuxtxt_FillRect(lfb,xres,*pPosX, PosY +1, curfontwidth, fontheight-1, bgcolor);
			*pPosX += curfontwidth;
			return 0;
		case 0xE2: /* -| */
			tuxtxt_DrawHLine(lfb,xres,*pPosX, PosY, curfontwidth, fgcolor);
			tuxtxt_DrawVLine(lfb,xres,*pPosX + curfontwidth -1, PosY +1, fontheight -1, fgcolor);
			tuxtxt_FillRect(lfb,xres,*pPosX, PosY +1, curfontwidth-1, fontheight-1, bgcolor);
			*pPosX += curfontwidth;
			return 0;
		case 0xE3: /* |  */
			tuxtxt_DrawVLine(lfb,xres,*pPosX, PosY, fontheight, fgcolor);
			tuxtxt_FillRect(lfb,xres,*pPosX +1, PosY, curfontwidth -1, fontheight, bgcolor);
			*pPosX += curfontwidth;
			return 0;
		case 0xE4: /*  | */
			tuxtxt_DrawVLine(lfb,xres,*pPosX + curfontwidth -1, PosY, fontheight, fgcolor);
			tuxtxt_FillRect(lfb,xres,*pPosX, PosY, curfontwidth -1, fontheight, bgcolor);
			*pPosX += curfontwidth;
			return 0;
		case 0xE5: /* |_ */
			tuxtxt_DrawHLine(lfb,xres,*pPosX, PosY + fontheight -1, curfontwidth, fgcolor);
			tuxtxt_DrawVLine(lfb,xres,*pPosX, PosY, fontheight -1, fgcolor);
			tuxtxt_FillRect(lfb,xres,*pPosX +1, PosY, curfontwidth-1, fontheight-1, bgcolor);
			*pPosX += curfontwidth;
			return 0;
		case 0xE6: /* _ */
			tuxtxt_DrawHLine(lfb,xres,*pPosX, PosY + fontheight -1, curfontwidth, fgcolor);
			tuxtxt_FillRect(lfb,xres,*pPosX, PosY, curfontwidth, fontheight-1, bgcolor);
			*pPosX += curfontwidth;
			return 0;
		case 0xE7: /* _| */
			tuxtxt_DrawHLine(lfb,xres,*pPosX, PosY + fontheight -1, curfontwidth, fgcolor);
			tuxtxt_DrawVLine(lfb,xres,*pPosX + curfontwidth -1, PosY, fontheight -1, fgcolor);
			tuxtxt_FillRect(lfb,xres,*pPosX, PosY, curfontwidth-1, fontheight-1, bgcolor);
			*pPosX += curfontwidth;
			return 0;
		case 0xE8: /* Ii */
			tuxtxt_FillRect(lfb,xres,*pPosX +1, PosY, curfontwidth -1, fontheight, bgcolor);
			for (Row=0; Row < curfontwidth/2; Row++)
				tuxtxt_DrawVLine(lfb,xres,*pPosX + Row, PosY + Row, fontheight - Row, fgcolor);
			*pPosX += curfontwidth;
			return 0;
		case 0xE9: /* II */
			tuxtxt_FillRect(lfb,xres,*pPosX, PosY, curfontwidth/2, fontheight, fgcolor);
			tuxtxt_FillRect(lfb,xres,*pPosX + curfontwidth/2, PosY, (curfontwidth+1)/2, fontheight, bgcolor);
			*pPosX += curfontwidth;
			return 0;
		case 0xEA: /* °  */
			tuxtxt_FillRect(lfb,xres,*pPosX, PosY, curfontwidth, fontheight, bgcolor);
			tuxtxt_FillRect(lfb,xres,*pPosX, PosY, curfontwidth/2, curfontwidth/2, fgcolor);
			*pPosX += curfontwidth;
			return 0;
		case 0xEB: /* ¬ */
			tuxtxt_FillRect(lfb,xres,*pPosX, PosY +1, curfontwidth, fontheight -1, bgcolor);
			for (Row=0; Row < curfontwidth/2; Row++)
				tuxtxt_DrawHLine(lfb,xres,*pPosX + Row, PosY + Row, curfontwidth - Row, fgcolor);
			*pPosX += curfontwidth;
			return 0;
		case 0xEC: /* -- */
			tuxtxt_FillRect(lfb,xres,*pPosX, PosY, curfontwidth, curfontwidth/2, fgcolor);
			tuxtxt_FillRect(lfb,xres,*pPosX, PosY + curfontwidth/2, curfontwidth, fontheight - curfontwidth/2, bgcolor);
			*pPosX += curfontwidth;
			return 0;
		case 0xED:
		case 0xEE:
		case 0xEF:
		case 0xF0:
		case 0xF1:
		case 0xF2:
		case 0xF3:
		case 0xF4:
		case 0xF5:
		case 0xF6:
			Char = arrowtable[Char - 0xED];
			break;
		default:
			break;
		}
	}
	if (Char <= 0x20)
	{
#if TUXTXT_DEBUG
		printf("TuxTxt found control char: %x \"%c\" \n", Char, Char);
#endif
		tuxtxt_FillRect(lfb,xres,*pPosX, PosY, curfontwidth, factor*fontheight, bgcolor);
		*pPosX += curfontwidth;
		return -2;
	}
	return Char; // Char is an alphanumeric unicode character
}
