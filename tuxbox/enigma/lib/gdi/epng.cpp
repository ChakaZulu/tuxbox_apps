#include <png.h>
#include <stdio.h>
#include <lib/gdi/epng.h>
#include <unistd.h>
#include <errno.h>

gImage *loadPNG(const char *filename)
{
	__u8 header[8];
	FILE *fp;
	gImage *res=0;
	png_structp png_ptr = 0;
	png_infop info_ptr = 0;
	png_infop end_info = 0;
	
	if (!(fp = fopen(filename, "rb")))
	{
//		eDebug("[ePNG] %s not found\n", filename);
		return 0;
	}

	if (!fread(header, 8, 1, fp))
	{
		fclose(fp);
		return 0;
	}
	
	if (png_sig_cmp(header, 0, 8))
		goto pngerror;

	if (!(png_ptr=png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0)))
		goto pngerror;
	
	if (!(info_ptr=png_create_info_struct(png_ptr)))
		goto pngerror;
		
	if (!(end_info=png_create_info_struct(png_ptr)))
		goto pngerror;
		
	if (setjmp(png_ptr->jmpbuf))
		goto pngerror;
		
	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);
	// png_set_invert_alpha(png_ptr);	// has no effect on indexed images
	png_read_info(png_ptr, info_ptr);
	
	png_uint_32 width, height;
	int bit_depth;
	int color_type;
	
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, 0, 0, 0);
	
	// convert 1,2 and 4 bpp to 8bpp images that enigma can blit
	if (bit_depth < 8) {
        png_set_packing(png_ptr);
        bit_depth = 8;
    }
	
	if (bit_depth > 8)
		goto pngerror;
		
//	eDebug("%s: %dx%dx%d png, %d", filename, (int)width, (int)height, (int)bit_depth, color_type);

	switch (color_type) 
	{
		case PNG_COLOR_TYPE_GRAY:
		case PNG_COLOR_TYPE_PALETTE:
//		case PNG_COLOR_TYPE_RGB:		// in theory, gImage can handle RGB pics - but enigma can't blit them, so
										// until this is fixed, imho we should reject RGB pictures right here
										// because later on we get an fatal blitting error followed by an enigma shutdown
		{
			res=new gImage(eSize(width, height), bit_depth);
			
			png_bytep *rowptr=new png_bytep[height];
			for (unsigned int i=0; i<height; i++)
				rowptr[i]=((png_byte*)(res->data))+i*res->stride;
			png_read_rows(png_ptr, rowptr, 0, height);
	
			delete [] rowptr;
			
			if (color_type == PNG_COLOR_TYPE_PALETTE) 
			{
				// indexed pictures without a palette make no sense
				if (!png_get_valid(png_ptr, info_ptr, PNG_INFO_PLTE))
					goto pngerror;

				png_color *palette;
				int num_palette;
				png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);
				if (num_palette) 
				{
					res->clut.data=new gRGB[num_palette];
					res->clut.colors=num_palette;
				}
			
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
					png_color_16 *transparent_color;
					
					png_get_tRNS(png_ptr, info_ptr, &trans, &num_palette, &transparent_color);
					if (transparent_color)
						res->clut.data[transparent_color->index].a=255;
					if (trans)					
						for (int i=0; i<num_palette; i++)
							res->clut.data[i].a=255-trans[i];
				}
			}
			break;
		}
		default:
			eDebug("[ePNG] unsupported color_type in %s", filename);
			goto pngerror2;
	}

	png_destroy_read_struct(&png_ptr, &info_ptr,&end_info);
	fclose(fp);
	return res;

pngerror:
	eDebug("[ePNG] png structure failure in %s\n", filename);
pngerror2:
	if (res)
		delete res;
	if (png_ptr)
		png_destroy_read_struct(&png_ptr, info_ptr ? &info_ptr : (png_infopp)NULL, end_info ? &end_info : (png_infopp)NULL);
	if (fp)
		fclose(fp);
	return 0;
}

int savePNG(const char *filename, gPixmap *pixmap)
{
	FILE *fp=fopen(filename, "wb");
	if (!fp)
		return -1;
	png_structp png_ptr=png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (!png_ptr)
	{
		eDebug("write png, couldnt allocate write struct");
		fclose(fp);
		unlink(filename);
		return -2;
	}
	png_infop info_ptr=png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		eDebug("info");
		png_destroy_write_struct(&png_ptr, 0);
		fclose(fp);
		unlink(filename);
		return -3;
	}
	if (setjmp(png_ptr->jmpbuf))
	{
		eDebug("error :/");
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
		unlink(filename);
		return -4;
	}
	png_init_io(png_ptr, fp);
	png_set_filter(png_ptr, 0, PNG_FILTER_NONE|PNG_FILTER_SUB|PNG_FILTER_PAETH);
	png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);

	png_set_IHDR(png_ptr, info_ptr, pixmap->x, pixmap->y, pixmap->bpp, 
		pixmap->clut.data ? PNG_COLOR_TYPE_PALETTE : PNG_COLOR_TYPE_GRAY, 
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	if (pixmap->clut.data)
	{
		png_color palette[pixmap->clut.colors];
		png_byte trans[pixmap->clut.colors];
		for (int i=0; i<pixmap->clut.colors; ++i)
		{
			palette[i].red=pixmap->clut.data[i].r;
			palette[i].green=pixmap->clut.data[i].g;
			palette[i].blue=pixmap->clut.data[i].b;
			trans[i]=255-pixmap->clut.data[i].a;
		}
		png_set_PLTE(png_ptr, info_ptr, palette, pixmap->clut.colors);
		png_set_tRNS(png_ptr, info_ptr, trans, pixmap->clut.colors, 0);
	}
	png_write_info(png_ptr, info_ptr);
	png_set_packing(png_ptr);
	png_byte *row_pointers[pixmap->y];
	for (int i=0; i<pixmap->y; ++i)
		row_pointers[i]=((png_byte*)pixmap->data)+i*pixmap->stride;
	png_write_image(png_ptr, row_pointers);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fp);
	eDebug("wrote png ! fine !");
	return 0;
}
