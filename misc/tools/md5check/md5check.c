/*
 * md5check.c - ucode + firmware checker (dbox-II-project)
 *  
 * Homepage: http://dbox2.elxsi.de
 *
 * (C) Copyright 2001
 * dennis noermann
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA  
 *
 * $Log: md5check.c,v $
 * Revision 1.2  2001/09/28 11:43:48  derget
 *
 *
 * neue avia ucodes
 * neue cam-alpha
 * cam-alpha 04F
 *
 * Revision 1.1  2001/08/19 22:17:32  derget
 * ucode und firmware checker
 * inital inport
 *
 * 
 * $Revision: 1.2 $
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "md5sum.c"

int main(int argc, char **argv)
{
char *filename;
int cnt ;
const unsigned char test[] = { 0xe0, 0x3d, 0x37, 0x6c, 0xf3, 0x8f, 0x2b, 0x2a, 0x9e, 0xff, 0x3b, 0x00, 0xe4, 0x77, 0x7e, 0xfe }; 
const unsigned char avia500v093[] = { 0xfe, 0xce, 0x1d, 0x33, 0x24, 0xe0, 0x91, 0x7b, 0x92, 0x1d, 0x81, 0x44, 0x90, 0xd8, 0xa8, 0x24 }; 
const unsigned char avia500v110[] = { 0x73 ,0x73 ,0xf3 ,0x93 ,0x42 ,0x63 ,0xb3 ,0xc3 ,0xea ,0x1d ,0x0f ,0x50 ,0x0f ,0x00 ,0x44 ,0xa5 };

const unsigned char avia600vb017[] = { 0xda, 0x49, 0x21, 0x46, 0xba, 0x7e, 0x17, 0x78, 0x83, 0xfe, 0xad, 0xaa, 0x0c, 0xf8, 0x9a, 0xa5 }; 
const unsigned char avia600vb022[] = { 0x6a ,0x74 ,0x8f ,0xb2 ,0x80 ,0x00 ,0x73 ,0x8c ,0xaf ,0xeb ,0x9e ,0x27 ,0x44 ,0x3a ,0xc6 ,0x23 };

const unsigned char ucode[] = { 0xd4, 0xc1, 0x2d, 0xf0, 0xd4, 0xce, 0x8b, 0xa9, 0xeb, 0x85, 0x85, 0x09, 0xd8, 0x32, 0xdf, 0x65 };

const unsigned char cam_01_01_001D[] = { 0xff, 0x6f, 0xaf, 0xbd, 0x2a, 0xa1, 0xf2, 0x9a, 0xfe, 0x23, 0x2a, 0x72, 0xfa, 0xc5, 0x78, 0x70 };
const unsigned char cam_01_01_004D[] = { 0xc4, 0x2d, 0x67, 0x53, 0x79, 0x4d, 0xd9, 0x51, 0x46, 0xea, 0xc3, 0x1f, 0x2a, 0x65, 0xb5, 0x16 };
const unsigned char cam_01_01_005D[] = { 0xbe, 0x4b, 0x0f, 0x38, 0x55, 0x7c, 0x41, 0x6c, 0xe0, 0x4e, 0x7f, 0xa3, 0xfa, 0x63, 0x4f, 0x95 };

const unsigned char cam_01_01_001E[] = { 0x7b, 0x9b, 0x72, 0x78, 0x66, 0x23, 0xe3, 0x75, 0x03, 0x35, 0xc7, 0x9a, 0xf0, 0x44, 0xe7, 0x18 };
const unsigned char cam_01_01_002E[] = { 0xbe, 0x7f, 0x1b, 0xeb, 0x1b, 0xb4, 0x37, 0xb7, 0xf7, 0xc9, 0x9f, 0x8e, 0x5a, 0x96, 0x88, 0x82 };
const unsigned char cam_01_01_003E[] = { 0xa8, 0x68, 0x9d, 0x88, 0xe0, 0xd2, 0xdf, 0x12, 0xa1, 0x57, 0x32, 0xc9, 0x8f, 0x86, 0x5b, 0x11 }; 
const unsigned char cam_01_01_004E[] = { 0x43, 0x36, 0xe7, 0xd3, 0xfe, 0xd4, 0x3c, 0x9e, 0x06, 0x32, 0x10, 0xbc, 0xdc, 0x95, 0xd2, 0x3e };
const unsigned char cam_01_01_005E[] = { 0x99, 0x7b, 0x1f, 0x85, 0x8f, 0x1e, 0xfe, 0xe5, 0x25, 0xe6, 0x84, 0x25, 0x58, 0xed, 0xbe, 0x3c };

const unsigned char cam_01_02_002E[] = { 0x7f, 0x56, 0xe6, 0x93, 0xa9, 0x16, 0xb3, 0x9a, 0x6e, 0x27, 0x34, 0xdc, 0x9b, 0x5a, 0xab, 0x7a };
const unsigned char cam_01_02_002D[] = { 0x19, 0x05, 0x39, 0x06, 0x36, 0xe7, 0x0c, 0x96, 0x65, 0x74, 0xa3, 0x29, 0x8a, 0x1b, 0x89, 0xc3 };

const unsigned char cam_NOKIA_PRODTEST2[] = { 0x16, 0xc5, 0xe1, 0xeb, 0xa0, 0xcf, 0xe6, 0x3f, 0x2b, 0xbc, 0x64, 0x8e, 0x16, 0x44, 0xc8, 0x83 };

const unsigned char cam_01_01_004F[] = { 0xc7, 0x34, 0x20, 0x7d, 0xde, 0xa7, 0xb8, 0xce, 0xaf, 0xa2, 0x50, 0x5f, 0x13, 0x60, 0xf3, 0xbf };
const unsigned char cam_01_01_005F[] = { 0xa5, 0x98, 0x48, 0x25, 0xff, 0x55, 0x4e, 0xa5, 0x30, 0xef, 0xc4, 0x73, 0x3f, 0xfd, 0x74, 0x73 };

const unsigned char cam_STREAMHACK[] = { 0x42, 0x0b, 0xaf, 0x44, 0x7b, 0xfd, 0x52, 0x9a, 0x79, 0x4b, 0xb3, 0x6e, 0xf8, 0x0f, 0x16, 0x52 };

unsigned char md5buffer[16]; 

char *cam_alpha_file = "/ucodes/cam-alpha.bin";
char *avia500_file = "/ucodes/avia500.ux";
char *avia600_file = "/ucodes/avia600.ux";
char *ucode_file = "/ucodes/ucode.bin";

{
  FILE* in=fopen(avia500_file, "rb");
  if (!in)
    perror(avia500_file);
  else
{
fclose(in);   
filename=avia500_file;
md5_file(filename, 1, &md5buffer);

	{
	int i;
	int check=0;
	for(i=0;i<16;i++) if(avia500v093[i]==md5buffer[i]) check++;
	if (check==16) 
		printf("avia500.ux ist OK (v093)\n");
	else
	{
        int check=0;
        for(i=0;i<16;i++) if(avia500v110[i]==md5buffer[i]) check++;
        if (check==16)
                printf("avia500.ux ist OK (v110)\n");
        else
                {
		for (cnt = 0; cnt < 16; ++cnt)
		printf ("0x%02x ,", md5buffer[cnt]);
		printf("\navia500.ux ist nicht OK\n");
		}
		}
	}

}
}

{
  FILE* in=fopen(avia600_file, "rb");
  if (!in)
    perror(avia600_file);
  else
{
fclose(in);   
filename=avia600_file;
md5_file(filename, 1, &md5buffer);
        {
        int i;
        int check=0;
        for(i=0;i<16;i++) if(avia600vb017[i]==md5buffer[i]) check++;
        if (check==16)
                printf("avia600.ux ist OK (vb017)\n");
        else
	{
        check=0;
        for(i=0;i<16;i++) if(avia600vb022[i]==md5buffer[i]) check++;
        if (check==16)
                printf("avia600.ux ist OK (vb022)\n");
        else

		{
                for (cnt = 0; cnt < 16; ++cnt)
                printf ("0x%02x ,", md5buffer[cnt]);
                printf("\navia600.ux ist NICHT OK\n");
		}
		}
        }


}
}

{
  FILE* in=fopen(ucode_file, "rb");
  if (!in)
    perror(ucode_file); 
  else
{
fclose(in);
filename=ucode_file;
md5_file(filename, 1, &md5buffer);
        {
        int i;
        int check=0;
        for(i=0;i<16;i++) if(ucode[i]==md5buffer[i]) check++;
        if (check==16)
                printf("ucode.bin ist OK\n");
        else
		{
		for (cnt = 0; cnt < 16; ++cnt)
                printf ("0x%02x ,", md5buffer[cnt]);
                printf("\nucode.bin ist NICHT OK\n");
		}
        }
}
}

{
  FILE* in=fopen(cam_alpha_file, "rb");
  if (!in)
    perror(cam_alpha_file);
  else
{
fclose(in);
filename=cam_alpha_file;
md5_file(filename, 1, &md5buffer);
        {   
        int i;
        int check=0;
        for(i=0;i<16;i++) if(cam_01_01_004D[i]==md5buffer[i]) check++;
        if (check==16)
                printf("cam-alpha.bin ist OK (01.01.004D)\n");
        	
	else
 		{
			check=0;
                       for(i=0;i<16;i++) if(cam_01_01_005E[i]==md5buffer[i]) check++;
	               if (check==16)
		       printf("cam-alpha.bin ist OK (01.01.005E)\n");
		       else
			{
			check=0;
                       for(i=0;i<16;i++) if(cam_01_01_005D[i]==md5buffer[i]) check++;
                       if (check==16)
                       printf("cam-alpha.bin ist OK (01.01.005D)\n");
                       else
			{
			check=0;
                       for(i=0;i<16;i++) if(cam_01_01_003E[i]==md5buffer[i]) check++;
                       if (check==16)
                       printf("cam-alpha.bin ist OK (01.01.003E)\n");
                       else
			{
			check=0;
                       for(i=0;i<16;i++) if(cam_01_01_001E[i]==md5buffer[i]) check++;
                       if (check==16)
                       printf("cam-alpha.bin ist OK (01.01.001E)\n");
                       else
			{
                       check=0;
		       for(i=0;i<16;i++) if(cam_01_01_004E[i]==md5buffer[i]) check++;
                       if (check==16)
                       printf("cam-alpha.bin ist OK (01.01.004E)\n");
                       else
                        {
                        check=0;
			for(i=0;i<16;i++) if(cam_NOKIA_PRODTEST2[i]==md5buffer[i]) check++;
			if (check==16)       
                       	printf("cam-alpha.bin ist OK (NOKIA_PRODTEST2)\n");
                       	else
			{
			check=0;
                        for(i=0;i<16;i++) if(cam_STREAMHACK[i]==md5buffer[i]) check++;
                        if (check==16)
                        printf("cam-alpha.bin ist OK (STREAMHACK)\n");
                        else
                        {
                        check=0;
                        for(i=0;i<16;i++) if(cam_01_01_002E[i]==md5buffer[i]) check++;
                        if (check==16)
                        printf("cam-alpha.bin ist OK (01.01.002E)\n");
                        else
                        {
                        check=0;
                        for(i=0;i<16;i++) if(cam_01_01_001D[i]==md5buffer[i]) check++;
                        if (check==16)
                        printf("cam-alpha.bin ist OK (01.01.001D)\n");
                        else
			{
                        check=0;
                        for(i=0;i<16;i++) if(cam_01_01_004F[i]==md5buffer[i]) check++;
                        if (check==16)
                        printf("cam-alpha.bin ist OK (01.01.004F)\n");
                        else
			{
                        check=0;
                        for(i=0;i<16;i++) if(cam_01_01_005F[i]==md5buffer[i]) check++;
                        if (check==16)
                        printf("cam-alpha.bin ist OK (01.01.005F)\n");
                        else
			{
                        check=0; 
                        for(i=0;i<16;i++) if(cam_01_02_002E[i]==md5buffer[i]) check++;
                        if (check==16)
                        printf("cam-alpha.bin ist OK (01.02.002E)\n");
                        else
                        {
                        check=0; 
                        for(i=0;i<16;i++) if(cam_01_02_002D[i]==md5buffer[i]) check++;
                        if (check==16)
                        printf("cam-alpha.bin ist OK (01.02.002D)\n");
                        else
			{
	                for (cnt = 0; cnt < 16; ++cnt)
        	        printf ("0x%02x, ", md5buffer[cnt]);
                      	printf("\ncam-alpha.bin ist NICHT OK\n");
                        }
				}
				}
				}			
			        }
                                }
				}
				}
				}
                                } 
                                }
				}
			}
		}
        }
}
}

}
