#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <memory.h>
#include <time.h>

#include <asm/types.h>
#include "subtitle.h"

void bitstream_init(struct bitstream *bit, void *buffer, int size)
{
	bit->data = (__u8*) buffer;
	bit->size = size;
	bit->avail = 8;
	bit->consumed = 0;
}

int bitstream_get(struct bitstream *bit)
{
	int val;
	bit->avail -= bit->size;
	val = ((*bit->data) >> bit->avail) & ((1<<bit->size) - 1);
	if (!bit->avail)
	{
		bit->data++;
		bit->consumed++;
		bit->avail = 8;
	}
	return val;
}

void subtitle_process_line(struct subtitle_ctx *sub, struct subtitle_page *page, int object_id, int line, __u8 *data, int len)
{
	struct subtitle_region *region = page->regions;
//	printf("line for %d:%d\n", page->page_id, object_id);
	while (region)
	{
		struct subtitle_region_object *object = region->region_objects;
		while (object)
		{
			if (object->object_id == object_id)
			{
				int x = object->object_horizontal_position;
				int y = object->object_vertical_position + line;
				if (x + len > region->region_width)
				{
					printf("[SUB] !!!! XCLIP %d + %d > %d\n", x, len, region->region_width);
					len = region->region_width - x;
				}
				if (len < 0)
					break;
				if (y >= region->region_height)
				{
					printf("[SUB] !!!! YCLIP %d >= %d\n", y, region->region_height);
					break;
				}
//				printf("inserting %d bytes (into region %d)\n", len, region->region_id);
				memcpy(region->region_buffer + region->region_width * y + x, data, len);
			}
			object = object->next;
		}
		region = region->next;
	}
}

int subtitle_process_pixel_data(struct subtitle_ctx *sub, struct subtitle_page *page, int object_id, int *linenr, int *linep, __u8 *data)
{
	int data_type = *data++;
	static __u8 line[720];
	switch (data_type)
	{
	case 0x10: // 2bit pixel data
		printf("[SUB]  2 bit pixel data\n");
		exit(0);
		break;
	case 0x11:
	{
		struct bitstream bit;
//		printf("  4 bit pixel data\n");
		bitstream_init(&bit, data, 4);
		while (1)
		{
			int len=0, col=0;
			int code = bitstream_get(&bit);
			if (code)
			{
				col = code;
				len = 1;
			} else
			{
				code = bitstream_get(&bit);
				if (!code)
					break;
				else if (code == 0xC)
				{
					col = 0;
					len = 1;
				} else if (code == 0xD)
				{
					col = 0;
					len = 2;
				} else if (code < 8)
				{
					col = 0;
					len = (code & 7) + 2;
				} else if ((code & 0xC) == 0x8)
				{
					col = bitstream_get(&bit);
					len = (code & 3) + 4;
				} else if (code == 0xE)
				{
					len = bitstream_get(&bit) + 9;
					col = bitstream_get(&bit);
				} else if (code == 0xF)
				{
					len  = bitstream_get(&bit) << 4;
					len |= bitstream_get(&bit);
					len += 25;
					col  = bitstream_get(&bit);
				}
			}
			while (len && ((*linep) < 720))
			{
				line[(*linep)++] = col | 0xF0;
				len--;
			}
		}
		while (bit.avail != 8)
			bitstream_get(&bit);
		return bit.consumed + 1;
		break;
	}
	case 0x12:
		printf("[SUB]  8 bit pixel data\n");
		exit(0);
		break;
	case 0x20:
	case 0x21:
	case 0x22:
		printf("[SUB] maps.\n");
		break;
	case 0xF0:
		subtitle_process_line(sub, page, object_id, *linenr, line, *linep);
/*		{
			int i;
			for (i=0; i<720; ++i)
				printf("%d ", line[i]);
			printf("\n");
		} */
		(*linenr)+=2; // interlaced
		*linep = 0;
//		printf("[SUB] EOL\n");
		return 1;
	default:
		printf("subtitle_process_pixel_data: invalid data_type %02x\n", data_type);
		return -1;
	}
	return 0;
}

int subtitle_process_segment(struct subtitle_ctx *sub, __u8 *segment)
{
	int segment_type, page_id, segment_length, processed_length;
	if (*segment++ !=  0x0F)
	{
		printf("out of sync.\n");
		return -1;
	}
	segment_type = *segment++;
	page_id  = *segment++ << 8;
	page_id |= *segment++;
	segment_length  = *segment++ << 8;
	segment_length |= *segment++;
	if (segment_type == 0xFF)
		return segment_length + 6;
//	printf("have %d bytes of segment data\n", segment_length);
	
//	printf("page_id %d, segtype %02x\n", page_id, segment_type);
	
	struct subtitle_page *page, **ppage;
		
	page = sub->pages; ppage = &sub->pages;
		
	while (page)
	{
		if (page->page_id == page_id)
			break;
		ppage = &page->next;
		page = page->next;
	}

	processed_length = 0;
	
	switch (segment_type)
	{
	case 0x10: // page composition segment
	{
		int page_time_out = *segment++; processed_length++;
		int page_version_number = *segment >> 4;
		int page_state = (*segment >> 2) & 0x3;
		printf("pcs with %d bytes data (%d:%d:%d)\n", segment_length, page_id, page_version_number, page_state);
		segment++;
		processed_length++;
		
	//	printf("page time out: %d\n", page_time_out);
		printf("page_version_number: %d\n" ,page_version_number);
		printf("page_state: %d\n", page_state);
		
		if (!page)
		{
			printf("page not found\n");
			page = (struct subtitle_page*)malloc(sizeof(struct subtitle_page));
			page->page_regions = 0;
			page->regions = 0;
			page->page_id = page_id;
			page->cluts = 0;
			page->next = 0;
			*ppage = page;
		} else
		{
			if (page->pcs_size != segment_length)
				page->page_version_number = -1;
				// if no update, just skip this data.
			if (page->page_version_number == page_version_number)
				break;
		}

		printf("page updated: old: %d, new: %d\n", page->page_version_number, page_version_number);
			// when acquisition point or mode change: remove all displayed pages.
		if ((page_state == 1) || (page_state == 2))
		{
			while (page->page_regions)
			{
				struct subtitle_page_region *p = page->page_regions->next;
				free(page->page_regions);
				page->page_regions = p;
			}
		}
		
		printf("new page.. (%d)\n", page_state);
		
		page->page_time_out = time(0) + page_time_out;
		page->page_version_number = page_version_number;
		
		struct subtitle_page_region **r = &page->page_regions;
		
		printf("%d  / %d data left\n", processed_length, segment_length);
		
			// go to last entry
		while (*r)
			r = &(*r)->next;
		
		while (processed_length < segment_length)
		{
			struct subtitle_page_region *pr;
			
				// append new entry to list
			pr = (struct subtitle_page_region*) malloc(sizeof(struct subtitle_page_region));
			pr->next = 0;
			*r = pr;
			r = &pr->next;
			
			pr->region_id = *segment++; processed_length++;
			segment++; processed_length++;

			pr->region_horizontal_address  = *segment++ << 8; 
			pr->region_horizontal_address |= *segment++;
			processed_length += 2;
			
			pr->region_vertical_address  = *segment++ << 8;
			pr->region_vertical_address |= *segment++;
			processed_length += 2;
			
			printf("appended active region\n");
		}
		
		if (processed_length != segment_length)
			printf("%d != %d\n", processed_length, segment_length);
		break;
	}
	case 0x11: // region composition segment
	{
		int region_id = *segment++; processed_length++;
		int region_version_number = *segment >> 4;
		int region_fill_flag = !!(*segment & 0x40);
		segment++; processed_length++;
		
			// if we didn't yet received the pcs for this page, drop the region
		if (!page)
		{
			printf("ignoring region %x, since page %02x doesn't yet exist.\n", region_id, page_id);
			break;
		}
		
		struct subtitle_region *region, **pregion;
		
		region = page->regions; pregion = &page->regions;

		while (region)
		{
			fflush(stdout);
			if (region->region_id == region_id)
				break;
			pregion = &region->next;
			region = region->next;
		}
		
		if (!region)
		{
			*pregion = region = (struct subtitle_region*) malloc(sizeof(struct subtitle_region));
			region->next = 0;
		} else if (region->region_version_number != region_version_number)
		{
			struct subtitle_region_object *objects = region->region_objects;
			while (objects)
			{
				struct subtitle_region_object *n = objects->next;
				free(objects);
				objects = n;
			}
			free(region->region_buffer);
		} else 
			break;
		
		
		printf("region %d:%d update\n", page_id, region_id);
			
		region->region_id = region_id;
		region->region_version_number = region_version_number;
		
		region->region_width  = *segment++ << 8;
		region->region_width |= *segment++;
		processed_length += 2;
		
		region->region_height  = *segment++ << 8;
		region->region_height |= *segment++;
		processed_length += 2;
		
		region->region_buffer = (__u8*)malloc(region->region_height * region->region_width);
		
		int region_level_of_compatibility, region_depth;
		
		region_level_of_compatibility = (*segment >> 5) & 7;
		region_depth = (*segment++ >> 2) & 7;
		processed_length ++;
		
		int CLUT_id;
		CLUT_id = *segment++; processed_length++;
		
		region->clut_id = CLUT_id;
		
		int region_8bit_pixel_code, region_4bit_pixel_code, region_2bit_pixel_code;
		region_8bit_pixel_code = *segment++; processed_length++;
		region_4bit_pixel_code = *segment >> 4;
		region_2bit_pixel_code = (*segment++ >> 2) & 3;	
		processed_length++;
		
		if (!region_fill_flag)
		{
			region_2bit_pixel_code = region_4bit_pixel_code = region_8bit_pixel_code = 0;
			region_fill_flag = 1;
		}
		
		if (region_fill_flag)
		{
			if (region_depth == 1)
				memset(region->region_buffer, region_2bit_pixel_code, region->region_height * region->region_width);
			else if (region_depth == 2)
				memset(region->region_buffer, region_4bit_pixel_code, region->region_height * region->region_width);
			else if (region_depth == 3)
				memset(region->region_buffer, region_8bit_pixel_code, region->region_height * region->region_width);
			else
				printf("!!!! invalid depth\n");
		}
		
		printf("region %02x, version %d, %dx%d\n", region->region_id, region->region_version_number, region->region_width, region->region_height);
		
		region->region_objects = 0;
		struct subtitle_region_object **pobject = &region->region_objects;
		
		while (processed_length < segment_length)
		{
			
			struct subtitle_region_object *object;
			
			object = (struct subtitle_region_object*)malloc(sizeof(struct subtitle_region_object));

			*pobject = object;
			object->next = 0;
			pobject = &object->next;
			
			object->object_id  = *segment++ << 8;
			object->object_id |= *segment++; processed_length += 2;
			
			object->object_type = *segment >> 6;
			object->object_provider_flag = (*segment >> 4) & 3;
			object->object_horizontal_position  = (*segment++ & 0xF) << 8;
			object->object_horizontal_position |= *segment++; 
			processed_length += 2;
			
			object->object_vertical_position  = *segment++ << 8;
			object->object_vertical_position |= *segment++ ;
			processed_length += 2;
			
			if ((object->object_type == 1) || (object->object_type == 2))
			{
				object->foreground_pixel_value = *segment++;
				object->background_pixel_value = *segment++;
				processed_length += 2;
			}
		}
		
		if (processed_length != segment_length)
			printf("too less data! (%d < %d)\n", segment_length, processed_length);
		
		break;
	}
	case 0x12: // CLUT definition segment
	{
		int CLUT_id, CLUT_version_number;
		struct subtitle_clut *clut, **pclut;
		
		if (!page)
			break;

		printf("CLUT: %02x\n", *segment);
		CLUT_id = *segment++;
		
		CLUT_version_number = *segment++ >> 4;
		processed_length += 2;

		printf("page %d, CLUT %02x, version %d\n", page->page_id, CLUT_id, CLUT_version_number);

		clut = page->cluts; pclut = &page->cluts;
		
		while (clut)
		{
			if (clut->clut_id == CLUT_id)
				break;
			pclut = &clut->next;
			clut = clut->next;
		}
		
		if (!clut)
		{
			*pclut = clut = (struct subtitle_clut*) malloc(sizeof(struct subtitle_clut));
			clut->next = 0;
		} else if (clut->CLUT_version_number == CLUT_version_number)
			break;
			
		clut->clut_id = CLUT_id;

			/* invalidate CLUT if updated. */			
		if ((sub->current_clut_page_id == page_id) && (sub->current_clut_id == CLUT_id))
			sub->current_clut_id = -1;
			
		clut->size = 16;
			
		printf("new clut\n");
		while (processed_length < segment_length)
		{
			int CLUT_entry_id, entry_CLUT_flag, full_range;
			int v_Y, v_Cr, v_Cb, v_T;
			
			CLUT_entry_id = *segment++;
			entry_CLUT_flag = *segment >> 5;
			if (!(entry_CLUT_flag & 6)) // if no 4 or 16 color entry, skip it
			{
				printf("skipped 8bit CLUT entry\n");
				continue;
			}
			full_range = *segment++ & 1;
			processed_length += 2;
			
			if (full_range)
			{
				v_Y  = *segment++;
				v_Cr = *segment++;
				v_Cb = *segment++;
				v_T  = *segment++;
				processed_length += 4;
			} else
			{
				v_Y   = *segment & 0xFC;
				v_Cr  = (*segment++ & 3) << 6;
				v_Cr |= (*segment & 0xC0) >> 4;
				v_Cb  = (*segment & 0x3C) << 2;
				v_T   = (*segment++ & 3) << 6;
				processed_length += 2;
			}
			
			clut->entries[CLUT_entry_id].Y = v_Y; 
			clut->entries[CLUT_entry_id].Cr = v_Cr; 
			clut->entries[CLUT_entry_id].Cb = v_Cb; 
			clut->entries[CLUT_entry_id].T = v_T; 
			printf("  %04x %02x %02x %02x %02x\n", CLUT_entry_id, v_Y, v_Cb, v_Cr, v_T);
		}
		break;
	}
	case 0x13: // object data segment
	{
		int object_id, object_version_number, object_coding_method, non_modifying_color_flag;
		
		object_id  = *segment++ << 8;
		object_id |= *segment++;
		processed_length += 2;
		
		object_version_number = *segment >> 4;
		object_coding_method  = (*segment >> 2) & 3;
		non_modifying_color_flag = !!(*segment++ & 2);
		processed_length++;
		
		printf("object id %04x, version %d, object_coding_method %d (page_id %d)\n", object_id, object_version_number, object_coding_method, page_id);
		
		if (object_coding_method == 0)
		{
			int top_field_data_blocklength, bottom_field_data_blocklength;
			int i, line, linep;
			
			top_field_data_blocklength  = *segment++ << 8;
			top_field_data_blocklength |= *segment++;
			
			bottom_field_data_blocklength  = *segment++ << 8;
			bottom_field_data_blocklength |= *segment++;
			printf("%d / %d bytes\n", top_field_data_blocklength, bottom_field_data_blocklength);
			processed_length += 4;
			
			i = 0;
			line = 0;
			linep = 0;
			while (i < top_field_data_blocklength)
			{
				int len;
				len = subtitle_process_pixel_data(sub, page, object_id, &line, &linep, segment);
				if (len < 0)
					return -1;
				segment += len;
				processed_length += len;
				i += len;
			}
			
			line = 1;
			linep = 0;

			if (bottom_field_data_blocklength)
			{
				i = 0;
				while (i < bottom_field_data_blocklength)
				{
					int len;
					len = subtitle_process_pixel_data(sub, page, object_id, &line, &linep, segment);
					if (len < 0)
						return -1;
					segment += len;
					processed_length += len;
					i += len;
				}
			} else if (top_field_data_blocklength)
			{
				printf("!!!! unimplemented: no bottom field! (%d : %d)\n", top_field_data_blocklength, bottom_field_data_blocklength);
			}
			
			if ((top_field_data_blocklength + bottom_field_data_blocklength) & 1)
			{
				segment++; processed_length++;
			}
		} else if (object_coding_method == 1)
		{
			printf("---- object_coding_method 1 unsupported!\n");
		}
		
		break;
	}
	case 0x80: // end of display set segment
	{
		if (sub->screen_enabled)
			subtitle_redraw_all(sub);
		break;
	}
	case 0xFF: // stuffing
		break;
	default:
		printf("unhandled segment type %02x\n", segment_type);
	}
	
	return segment_length + 6;
}

void subtitle_process_pes(struct subtitle_ctx *sub, void *buffer, int len)
{
	__u8 *pkt = (__u8*)buffer;
	if (pkt[0] || pkt[1] || (pkt[2] != 1))
	{
		printf("%s: invalid startcode %02x %02x %02x\n", __FUNCTION__, pkt[0], pkt[1], pkt[2]);
		return;
	}
	if (pkt[3] != 0xBD)
	{
		printf("%s: invalid stream_id %02x\n", __FUNCTION__, pkt[3]);
		return;
	}
	pkt += 6; len -= 6;
	// skip PES header
	pkt++; len--;
	pkt++; len--;
	
	int hdr_len = *pkt++; len--;

	pkt+=hdr_len; len-=hdr_len;

	if (*pkt != 0x20)
	{
		printf("data identifier is 0x%02x, but not 0x20\n", *pkt);
		return;
	}
	pkt++; len--; // data identifier
	*pkt++; len--; // stream id;
	
	if (len <= 0)
	{
		printf("no data left (%d)\n", len);
		return;
	}
	
	while (len && *pkt == 0x0F)
	{
		int l = subtitle_process_segment(sub, pkt);
		if (l < 0)
			break;
		pkt += l;
		len -= l;
	}
	if (len && *pkt != 0xFF)
		printf("strange data at the end\n");
}

void subtitle_clear_screen(struct subtitle_ctx *sub)
{
		/* clear bbox */
	int y;
	
	printf("BBOX clear %d:%d -> %d:%d\n", sub->bbox_left, sub->bbox_top, sub->bbox_right, sub->bbox_bottom);
	
	if (sub->bbox_right > sub->bbox_left)
		for (y=sub->bbox_top; y < sub->bbox_bottom; ++y)
			memset(sub->screen_buffer + y * sub->screen_width, 0, sub->bbox_right - sub->bbox_left);
		
	sub->bbox_right = 0;
	sub->bbox_left = sub->screen_width;
	sub->bbox_top = sub->screen_height;
	sub->bbox_bottom = 0;
}

void subtitle_redraw_all(struct subtitle_ctx *sub)
{
	subtitle_clear_screen(sub);
	
	struct subtitle_page *page = sub->pages;
	printf("----------- end of display set\n");
	printf("active pages:\n");
	while (page)
	{
		printf("  page_id %02x\n", page->page_id);
		printf("  page_version_number: %d\n", page->page_version_number);
		printf("  active regions:\n");
		{
			struct subtitle_page_region *region = page->page_regions;
			while (region)
			{
				printf("    region_id: %04x\n", region->region_id);
				printf("    region_horizontal_address: %d\n", region->region_horizontal_address);
				printf("    region_vertical_address: %d\n", region->region_vertical_address);
				
				region = region->next;
			}
		}
		
		subtitle_redraw(sub, page->page_id);
		printf("defined regions:\n");
		struct subtitle_region *region = page->regions;
		while (region)
		{
			printf("  region_id %04x, version %d, %dx%d\n", region->region_id, region->region_version_number, region->region_width, region->region_height);
			
			struct subtitle_region_object *object = region->region_objects;
			while (object)
			{
				printf("  object %02x, type %d, %d:%d\n", object->object_id, object->object_type, object->object_horizontal_position, object->object_vertical_position);
				object = object->next;
			}				
				
			region = region->next;
		}
		page = page->next;
	}
}

void subtitle_redraw(struct subtitle_ctx *sub, int page_id)
{
	struct subtitle_page *page = sub->pages;
	int main_clut_id = -1;
	
	printf("displaying page id %d\n", page_id);
	
	while (page)
	{
		if (page->page_id == page_id)
			break;
		page = page->next;
	}
	if (!page)
	{
		printf("page not found\n");
		return;
	}
	
	
	printf("iterating regions..\n");
		/* iterate all regions in this pcs */
	struct subtitle_page_region *region = page->page_regions;
	while (region)
	{
		printf("region %d\n", region->region_id);
			/* find corresponding region */
		struct subtitle_region *reg = page->regions;
		while (reg)
		{
			if (reg->region_id == region->region_id)
				break;
			reg = reg->next;
		}
		if (reg)
		{
			int y;
			int clut_id = reg->clut_id;
			printf("clut %d, %d\n", clut_id, main_clut_id);
			if (main_clut_id != -1)
			{
				if (main_clut_id != clut_id)
				{
					printf("MULTIPLE CLUTS IN USE! prepare for pixelmuell!\n");
//					exit(0);
				}
			}
			main_clut_id = clut_id;
				
				
			printf("copy region %d to %d, %d\n", region->region_id, region->region_horizontal_address, region->region_vertical_address);
				
			int x0 = region->region_horizontal_address;
			int y0 = region->region_vertical_address;
			int x1 = x0 + reg->region_width;
			int y1 = y0 + reg->region_height;
				
			if ((x0 < 0) || (y0 < 0) || (x0 > sub->screen_width) || (x0 > sub->screen_height))
				continue;

				/* adjust bbox */
			if (x0 < sub->bbox_left)
				sub->bbox_left = x0;
			if (y0 < sub->bbox_top)
				sub->bbox_top = y0;
			if (x1 > sub->bbox_right)
				sub->bbox_right = x1;
			if (y1 > sub->bbox_bottom)
				sub->bbox_bottom = y1;
				
				/* copy to screen */
			for (y=0; y < reg->region_height; ++y)
			{
				memcpy(sub->screen_buffer + 
					sub->screen_width * (y + region->region_vertical_address) + 
					region->region_horizontal_address, 
					reg->region_buffer + reg->region_width * y, reg->region_width);
			}
		} else
			printf("region not found\n");
		region = region->next;
	}
	printf("schon gut.\n");
	
	if (main_clut_id == -1)
		return; /* most probably no display active */
	
	if ((sub->current_clut_id == main_clut_id) && (sub->current_clut_page_id == page_id))
		return;
	
	printf("updating clut..\n");
			/* find corresponding clut */
	struct subtitle_clut *clut = page->cluts;
	while (clut)
	{
		printf("have %d, want %d\n", clut->clut_id, main_clut_id);
		if (clut->clut_id == main_clut_id)
			break;
		break;
		clut = clut->next;
	}
	if (clut)
		sub->set_palette(clut);
	else
		printf("[SUB] CLUT NOT FOUND.\n");
}

void subtitle_screen_enable(struct subtitle_ctx *sub, int enable)
{
	if (sub->screen_enabled == enable)
		return;
	sub->screen_enabled = enable;
	if (enable)
		subtitle_redraw_all(sub);
	else
		subtitle_clear_screen(sub);
}
