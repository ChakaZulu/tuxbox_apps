#include <png.h>
#include <stdio.h>
#include "epng.h"

gImage *loadPNG(const char *filename)
{
	__u8 header[8];
	FILE *fp=fopen(filename, "rb");
	
	gImage *res=0;
	
	if (!fp)
	{
		eDebug("couldn't open");
		return 0;
	}
	if (!fread(header, 8, 1, fp))
	{
		eDebug("couldn't read");
		fclose(fp);
		return 0;
	}
	if (png_sig_cmp(header, 0, 8))
	{
		fclose(fp);
		return 0;
	}
	png_structp png_ptr=png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (!png_ptr)
	{
		eDebug("no pngptr");
		fclose(fp);
		return 0;
	}
	png_infop info_ptr=png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		eDebug("no info ptr");
		png_destroy_read_struct(&png_ptr, (png_infopp)0, (png_infopp)0);
		fclose(fp);
		return 0;
	}
	png_infop end_info = png_create_info_struct(png_ptr);
	if (!end_info)
	{
		eDebug("no end");
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		fclose(fp);
		return 0;
	 }
	if (setjmp(png_ptr->jmpbuf))
	{
		eDebug("das war wohl nix");
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		fclose(fp);
		if (res)
			delete res;
		return 0;
	}
	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);
	png_set_invert_alpha(png_ptr);
	png_read_info(png_ptr, info_ptr);
	
	png_uint_32 width, height;
	int bit_depth;
	int color_type;
	
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, 0, 0, 0);
	
	eDebug("%s: %dx%dx%d png, %d", filename, (int)width, (int)height, (int)bit_depth, color_type);
	
	if (color_type != 6)
	{
		res=new gImage(eSize(width, height), bit_depth);
	
		png_bytep *rowptr=new png_bytep[height];
	
		for (unsigned int i=0; i<height; i++)
			rowptr[i]=((png_byte*)(res->data))+i*res->stride;
		png_read_rows(png_ptr, rowptr, 0, height);
	
		delete rowptr;
	
		if (png_get_valid(png_ptr, info_ptr, PNG_INFO_PLTE))
		{
			png_color *palette;
			int num_palette;
			png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);
			if (num_palette)
				res->clut.data=new gRGB[num_palette];
			else
				res->clut.data=0;
			res->clut.colors=num_palette;
			
			for (int i=0; i<num_palette; i++)
			{
				res->clut.data[i].a=0;
				res->clut.data[i].r=palette[i].red;
				res->clut.data[i].g=palette[i].green;
				res->clut.data[i].b=palette[i].blue;
			}
			if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
			{
				png_byte *trans;
				png_get_tRNS(png_ptr, info_ptr, &trans, &num_palette, 0);
				for (int i=0; i<num_palette; i++)
					res->clut.data[i].a=255-trans[i];
			}
		} else
		{
			res->clut.data=0;
			res->clut.colors=0;
		}
		png_read_end(png_ptr, end_info);
	} else
		res=0;

	png_destroy_read_struct(&png_ptr, &info_ptr,&end_info);
	fclose(fp);
	return res;
}
