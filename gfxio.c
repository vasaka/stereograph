/* Stereograph 0.30a, 26/12/2000;
 * Graphics I/O functions;
 * Copyright (c) 2000 by Fabian Januszewski <fabian.linux@januszewski.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <png.h>

/* uncomment the following line to compile for big endian machines */
//#define BIG_ENDIAN

#include "renderer.h"
#include "gfxio.h"
#include "globals.h"


#define frand1() ((float)rand() / (float)RAND_MAX)
#define PI 3.14159265358979

/* add a pair of triangles to a stereogram */
int Add_Triangles(int x, int size, int width, struct GFX_DATA *gfx) {
	int z, y;
	for(y = 0; (y < size) && (y < gfx->Height); y++)
		for(z = 0; (z < size - 2*y); z++)
			if(((y + x - width/2 + z) >= 0) && ((y + x + width/2 + z) < gfx->Width))
				gfx->Data[y * gfx->Width + y + x - width/2 + z] = gfx->Data[y * gfx->Width + y + x + width/2 + z] = 0;
	return 0;
}


/* generates a random texture */
int Generate_Random_Texture(struct GFX_DATA *gfx, int width, int height, int type) {
	int a = 0, z;
	gfx->Data = (int*) malloc(width*height*sizeof(int));
	if(gfx->Data) {
		srand((unsigned int)time(NULL));
		gfx->Width = width;
		gfx->Height = height;
		switch(type) {
			case TEX_BW:
				for(z = 0; z < (height*width); z++)
					gfx->Data[z] = (rand()&1) * 65793 * 255;
				break;
			case TEX_GRAYSCALE:
				for(z = 0; z < (height*width); z++)
					gfx->Data[z] = (rand()&255) * 65793;
				break;
			case TEX_COLORED:
				for(z = 0; z < (height*width); z++)
					gfx->Data[z] = (rand()&(65793 * 255));
				break;
                        case TEX_SINLINE:
                                a = Generate_Sinline_Texture(gfx);
                                break;
        		default:
				free(gfx->Data);
				gfx->Data = NULL;
				if (verbose) printf("FAILED\n");
				fprintf(stderr, "unsupported random texture type;\n");
				a = -1;
		}
	} else {
		if (verbose) printf("FAILED\n");
		fprintf(stderr, "could not allocate memory for random texture;\n");
		a = -3;
	}
	if(verbose && !a) printf("done;\n");
	return a;
}


/* resizes a texture to the width of width pixels */
int Resize_GFX(struct GFX_DATA *gfx, int width) {
	int y;
	int *temp_gfx;

	if(verbose) printf("resizing texture...");
	temp_gfx = (int*) malloc(gfx->Height * width * sizeof(int));
	if(!temp_gfx) {
		if(verbose) printf("FAILED;\n");
		fprintf(stderr, "error allocating memory for texture!\n");
		return -3;
	} else
		for(y = 0; (y < gfx->Height); y++)
			memcpy(temp_gfx + y * width, gfx->Data + gfx->Width * y, width*sizeof(int));
	free(gfx->Data);
	gfx->Data = temp_gfx;
	gfx->Width = width;
	if(verbose) printf("done;\n");
	return 0;
}


/* inverts a base image */
int Invert_GFX(struct GFX_DATA *gfx) {
	int z, r = 0, g = 0, b = 0;

	if(verbose) printf("inverting base...");
	for(z = 0; z < (gfx->Width * gfx->Height); z++) {
		r = 255 -  (gfx->Data[z]          & 255);
		g = 255 - ((gfx->Data[z] /   256) & 255);
		b = 255 - ((gfx->Data[z] / 65536) & 255);
		gfx->Data[z] = r + (b * 256) + (g * 65536);
	}
	if(verbose) printf("done;\n");
	return 0;
}


/* adjusts the base levels for transparent rendering */
int T_adjust_level(struct GFX_DATA **T_gfx, int T_layers, int T_level) {
	int t, z, r_t = 0, g_t = 0, b_t = 0, r = 0, g = 0, b = 0;

	if(verbose) printf("ajdusting levels...");
	for(t = 1; t < T_layers; t++)
		for(z = 0; z < (T_gfx[0]->Width * T_gfx[0]->Height); z++) {
			r =  T_gfx[t - 1]->Data[z]          & 255;
			g = (T_gfx[t - 1]->Data[z] /   256) & 255;
			b = (T_gfx[t - 1]->Data[z] / 65536) & 255;
			r_t =  T_gfx[t]->Data[z]          & 255;
			g_t = (T_gfx[t]->Data[z] /   256) & 255;
			b_t = (T_gfx[t]->Data[z] / 65536) & 255;
			switch (T_level) {
				case T_NO_LEVEL:
					break;
				case T_BACK_LEVEL:
					if(!r_t && !g_t && !r_t) {
						r_t = r;
						g_t = g;
						b_t = b;
					}
					break;
				case T_TOP_LEVEL:
					if((r_t + g_t + r_t) < (r + g + b)) {
						r_t = r;
						g_t = g;
						b_t = b;
					}
					break;
				default :
					if(verbose) printf("FAILED\n");
					fprintf(stderr, "Unimplemented transparency level;\n");
					return -1;
			}
			T_gfx[t]->Data[z] = r_t + (b_t * 256) + (g_t * 65536);
		}
	if(verbose) printf("done;\n");
	return 0;
}


/* reads a png or targa file */
int Read_Gfx_File (char *file_name, struct GFX_DATA *gfx)
{
	int a;
	FILE *ifile = NULL;
	unsigned char check_header[8];

	ifile = fopen(file_name, "r");

	if(ifile == NULL) {
		if(verbose) printf("FAILED;\n"); else fprintf(stderr, "loading gfx data...FAILED;\n");
		fprintf(stderr, "cannot open file '%s'!\n", file_name);
		return -1;
	} else {
		a = identify_GFX_file(ifile, check_header);
		switch (a) {
			case GFX_PNG :
				a = Read_PNG(ifile, check_header, gfx);
				break;
			case GFX_TARGA :
				a = Read_TARGA(ifile, check_header, gfx);
				break;
			case GFX_PPM :
				a = Read_PPM(ifile, check_header, gfx);
				break;
		}
	}

	fclose(ifile);
	ifile = NULL;
	if(!a && verbose) printf("gfx data read (%i*%i);\n", gfx->Width, gfx->Height);
	return a;
}


/* writes a png, ppm or targa file */
int Write_Gfx_File (char *file_name, int output_format, struct GFX_DATA *gfx)
{
	int a;
	FILE *ofile = NULL;


	if(verbose) printf("saving '%s' (%i*%i)...", file_name, gfx->Width, gfx->Height);

	if(!file_name)
		ofile = stdout;
	else
		ofile = fopen(file_name, "w+");

	if(ofile == NULL) {
		if(verbose) printf("FAILED;\n"); else fprintf(stderr, "writing gfx data...FAILED;\n");
		fprintf(stderr, "cannot create file %s!\n", file_name);
		return -1;
	} else

	switch (output_format) {
		case GFX_TARGA :
			a = Write_TARGA(ofile, gfx);
			break;
		case GFX_PNG :
			a = Write_PNG(ofile, gfx);
			break;
		case GFX_PPM :
			a = Write_PPM(ofile, gfx);
			break;
	        default :
			if(strlen(file_name) >= 4) {
				if(!strcmp(file_name + strlen(file_name) - 4, ".tga"))
					a = Write_TARGA(ofile, gfx);
				else if(!strcmp(file_name + strlen(file_name) - 4, ".png"))
					a = Write_PNG(ofile, gfx);
				else if(!strcmp(file_name + strlen(file_name) - 4, ".ppm"))
					a = Write_PPM(ofile, gfx);
				else {
					if(verbose) printf("FAILED;\n"); else fprintf(stderr, "writing gfx data...FAILED;\n");
					fprintf(stderr, "unsupported output format!\n");
					a = -2;
				}
			} else {
				if(verbose) printf("FAILED;\n"); else fprintf(stderr, "writing gfx data...FAILED;\n");
				fprintf(stderr, "unsupported output format!\n");
				a = -2;
			}       	
	}

	fclose(ofile);
	ofile = NULL;
	return a;
}




/*** gfxio internals ***/
int identify_GFX_file(FILE *ifile, unsigned char *check_header) {

	if(fread(check_header, sizeof(char), 2, ifile) != 2) {
		if(verbose) printf("FAILED!\n"); else fprintf(stderr, "reading gfx data...FAILED;\n");
		fprintf(stderr, "cannot read header! (file corrupt?)\n");
		return -2;
	} else if((check_header[0] == 'P') && ((check_header[1] == '3') || (check_header[1] == '6')))
		return GFX_PPM;
	else {
		if(fread(check_header + 2, sizeof(char), 6, ifile) != 6) {
			if(verbose) printf("FAILED!\n"); else fprintf(stderr, "reading gfx data...FAILED;\n");
			fprintf(stderr, "cannot read header! (file corrupt?)\n");
			return -2;
		} else {
		        if(!png_sig_cmp(check_header, 0, 8))
				return GFX_PNG;
			else
				return GFX_TARGA;
		}
	}
	if(verbose) printf("FAILED;\n"); else fprintf(stderr, "reading gfx data...FAILED\n");
	fprintf(stderr, "cannot identify gfx file, unsupported format!\n");
	return -1;
}

int skip_space_n_comments(FILE *ifile) {
	int a;
	while(isspace(a = fgetc(ifile)) || (a == '#'))
		if(a == '#')
			while(fgetc(ifile) != 10);
	ungetc(a, ifile);
        return 0;
}

int get_dec(FILE *ifile) {
	static int a, c;
	c = 0;
	
	while(isdigit(a = fgetc(ifile)))
		c = c*10 + (a - '0');
	ungetc(a, ifile);

	return c;
}

int Read_PPM (FILE *ifile, unsigned char *check_header, struct GFX_DATA *gfx)
{
	int a, z, c_max;

	skip_space_n_comments(ifile);
	gfx->Width = get_dec(ifile);
	skip_space_n_comments(ifile);
	gfx->Height = get_dec(ifile);

	gfx->Data = (int*)malloc(gfx->Width*gfx->Height*sizeof(int));
	if(!gfx->Data) {
		if(verbose) printf("FAILED;\n"); else fprintf(stderr, "reading gfx data...FAILED;\n");
		fprintf(stderr, "error allocating memory for image dimensions of %i * %i pixels in 32 bit\n", gfx->Width, gfx->Height);
		return -3;
	}

	skip_space_n_comments(ifile);
	c_max = 255 / get_dec(ifile);

	if(check_header[1] == '3')
		for(z = 0; z < (gfx->Width * gfx->Height); z++) {
			while(isspace(a = fgetc(ifile))); ungetc(a, ifile);
			gfx->Data[z] = get_dec(ifile) * c_max;
			while(isspace(a = fgetc(ifile))); ungetc(a, ifile);
			gfx->Data[z] += (get_dec(ifile) * c_max) * 256;
			while(isspace(a = fgetc(ifile))); ungetc(a, ifile);
			gfx->Data[z] += (get_dec(ifile) * c_max) * 65536;
		}
	else if (check_header[1] == '6') {
		while(isspace(a = fgetc(ifile))); ungetc(a, ifile);
		for(z = 0; z < (gfx->Width * gfx->Height); z++) {
			gfx->Data[z] = fgetc(ifile) * c_max;
			gfx->Data[z] += (fgetc(ifile) * c_max) * 256;
			gfx->Data[z] += (fgetc(ifile) * c_max) * 65536;
		}
	} else {
		if (verbose) printf("FAILED\n");
		fprintf(stderr, "unknown PPM magic!\n");
	}

	return 0;
}


int Read_PNG (FILE *ifile, unsigned char *check_header, struct GFX_DATA *gfx)
{
	png_voidp user_error_ptr = NULL;
	png_structp png_ptr;
	png_infop info_ptr;
	png_infop end_info;

	int bit_depth, color_type, interlace_type, interlace_passes;

	//unsigned char header_check[8];
	int z;

	/* checking png header */
	//fread(header_check, sizeof(char), 8, ifile);
        //} else {

		/* initializing */
		png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, user_error_ptr, NULL, NULL);
		if (!png_ptr) {
        		if(verbose) printf("FAILED;\n"); else fprintf(stderr, "reading gfx data...FAILED\n");
	        	fprintf(stderr, "cannot create png read struct!\n");
			return -1;
		}
		info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr) {
			png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        		if(verbose) printf("FAILED;\n"); else fprintf(stderr, "reading gfx data...FAILED\n");
	        	fprintf(stderr, "cannot create png info struct!\n");
			return -1;
		}
		end_info = png_create_info_struct(png_ptr);
		if (!end_info) {
			png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        		if(verbose) printf("FAILED;\n"); else fprintf(stderr, "reading gfx data...FAILED\n");
	        	fprintf(stderr, "cannot create png (end) info struct!\n");
			return -1;
		}
		
		/* libpng error handling */
		if (setjmp(png_ptr->jmpbuf)) {
			png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        		if(verbose) printf("FAILED;\n"); else fprintf(stderr, "reading gfx data...FAILED\n");
	        	fprintf(stderr, "libpng reported an error!\n");
			return -1;
		}
		

		/* setting up file pointer */
		png_init_io(png_ptr, ifile);
		/* the offset created by the png check */
		png_set_sig_bytes(png_ptr, 8);
		
		/* setting up alpha channel for non-transparency */
		/* png_set_invert_alpha(png_ptr); */
		
		/* okay, let's get the full header now */
		png_read_info(png_ptr, info_ptr);

		png_get_IHDR(png_ptr, info_ptr, NULL, NULL, &bit_depth, &color_type, &interlace_type, NULL, NULL);

		/* general format conversions */

		/* handle lower bit rates */
		/*if (color_type == PNG_COLOR_TYPE_PALETTE && bit_depth <= 8)
			png_set_expand(png_ptr);
		if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth <= 8)
			png_set_expand(png_ptr);
		if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))*/
			png_set_expand(png_ptr);
		
		/* reduce from 16 bits/channel to 8 bits */
			/* correct use of little endian */
		/*if (bit_depth == 16) {*/
		#ifndef BIG_ENDIAN
			png_set_swap(png_ptr);  // swap byte pairs to little endian
		#endif
		png_set_strip_16(png_ptr);
		/*}*/


		/* if it's a gray scale, convert it to RGB */
		/* if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) */
			png_set_gray_to_rgb(png_ptr);

		/* convert indexed to full 32 bit RGBA */
		/* if (color_type == PNG_COLOR_TYPE_RGB) */
		png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);

		/* activate libpng interlace handling */
		interlace_passes = png_set_interlace_handling(png_ptr);

		/* okay, final info set up */
		png_read_update_info(png_ptr, info_ptr);

		gfx->Width = png_get_image_width(png_ptr, info_ptr);
		gfx->Height = png_get_image_height(png_ptr, info_ptr);

		gfx->Data = (int*)malloc(gfx->Width*gfx->Height*sizeof(int));
		if(!gfx->Data) {
			png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
			if(verbose) printf("FAILED;\n"); else fprintf(stderr, "reading gfx data...FAILED;\n");
			fprintf(stderr, "error allocating memory for image dimensions of %i * %i pixels in 32 bit\n", gfx->Width, gfx->Height);
			return -3;
		}

		/* alternatively read it row by row, interlacing as expected */
		for(z = 0; z < (gfx->Height * interlace_passes); z++)
			png_read_row(png_ptr, (void *) (gfx->Data + ((z % gfx->Height) * gfx->Width)), NULL);

		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		/* that's it */		
	//}
	return 0;
}

int Write_PNG (FILE *ofile, struct GFX_DATA *gfx)
{
	png_voidp user_error_ptr = NULL;
	png_structp png_ptr;
	png_infop info_ptr;

	int z;

	/* initializing */
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, user_error_ptr, NULL, NULL);
	if (!png_ptr) {
       		if(verbose) printf("FAILED;\n"); else fprintf(stderr, "writing gfx data...FAILED\n");
        	fprintf(stderr, "cannot create png write struct!\n");
		return -1;
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
       		if(verbose) printf("FAILED;\n"); else fprintf(stderr, "writing gfx data...FAILED\n");
        	fprintf(stderr, "cannot create png info struct!\n");
		return -1;
	}
		
	/* libpng error handling */
	if (setjmp(png_ptr->jmpbuf)) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
       		if(verbose) printf("FAILED;\n"); else fprintf(stderr, "writing gfx data...FAILED\n");
        	fprintf(stderr, "libpng reported an error!\n");
		return -1;
	}
		
	/* setting up file pointer */
	png_init_io(png_ptr, ofile);

	/* setting up png info header */
	png_set_IHDR(png_ptr, info_ptr, gfx->Width, gfx->Height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	png_set_invert_alpha(png_ptr);
		
	/* let's flush the header down */
	png_write_info(png_ptr, info_ptr);

	/*
	#ifndef BIG_ENDIAN
		png_set_swap(png_ptr);  // swap bytes to big endian, for little endian machines and only useful when writing 16 bits/channel;
	#endif
	*/

	for(z = 0; z < gfx->Height; z++)
                png_write_row(png_ptr, (void *) (gfx->Data + z*gfx->Width));

	png_write_end(png_ptr, NULL);

	png_destroy_write_struct(&png_ptr, &info_ptr);
	/* that's it */		

	return 0;
}

int Read_TARGA (FILE *ifile, unsigned char *check_header, struct GFX_DATA *gfx)
{
	/* TARGA uses the Intel format for integer coding (low byte first) */

	unsigned char header[20];
	int x, y, *palette, c, l, s, z;

	/* copy the preread check header to the real header */
	memcpy(header, check_header, sizeof(char)*8);

	/* reading all header information */
	if(fread(header + 8, sizeof(char), 10, ifile) != 10) {
		if(verbose) printf("FAILED!\n"); else fprintf(stderr, "reading gfx data...FAILED;\n");
		fprintf(stderr, "cannot read header! (file corrupt?)\n");
		return -2;
	}

	/* general measurements */
	gfx->Width = (int)header[12] + (int)header[13]*256;
	gfx->Height = (int)header[14] + (int)header[15]*256;
	gfx->Data = (int*)malloc(gfx->Width*gfx->Height*sizeof(int));
	if(!gfx->Data) {
		if(verbose) printf("FAILED;\n"); else fprintf(stderr, "reading gfx data...FAILED;\n");
		fprintf(stderr, "error allocating memory for image dimensions of %i * %i pixels in 32 bit\n", gfx->Width, gfx->Height);
		fprintf(stderr, "debug header follows:\n");
		for(x = 0; x < 20; x += 4)
			fprintf(stderr, " %i, %i, %i, %i;\n", (int)header[x], (int)header[x + 1], (int)header[x + 2], (int)header[x + 3]);
		return -3;
	}

	/* reading image identification field */
	for(x = 0; x < header[0]; x++)
		getc(ifile);

	/* reading palette data */
	if((header[1] != 0) && ((header[5] + header[6]*256) > 0) && (((header[3] + header[4]*256) + (header[5] + header[6]*256)) <= 256)) {
		palette = (int*)malloc(((header[3] + header[4]*256) + (header[5] + header[6]*256)) * sizeof(int));
		for(x = header[3] + header[4]*256; (x < (header[5] + header[6]*256)); x++)
			Read_TARGA_RGB(ifile, (int)header[7], NULL, palette + x);
	} else
		palette = NULL;

	if(((header[2] == 2) && (header[16] >= 16)) || (((header[2] == 1) || header[2] == 3) && (header[16] == 8))) {
	/* type 1: 8 bit/palette, uncompressed; type 2: 16 bit++, uncompressed; type 3: 8 bit monocrome, uncompressed; */
		if(header[17] & 32) {
			for(y = 0; y < gfx->Height; y++) {
				if(header[17] & 16)
					for(x = gfx->Width - 1; x >= 0; x--)
						Read_TARGA_RGB(ifile, (int)header[16], palette, gfx->Data + (y * gfx->Width) + x);
				else
					for(x = 0; x < gfx->Width; x++)
						Read_TARGA_RGB(ifile, (int)header[16], palette, gfx->Data + (y * gfx->Width) + x);
			}
		} else {
			for(y = gfx->Height - 1; y >= 0; y--) {
				if(header[17] & 16)
					for(x = gfx->Width - 1; x >= 0; x--)
						Read_TARGA_RGB(ifile, (int)header[16], palette, gfx->Data + (y * gfx->Width) + x);
				else
					for(x = 0; x < gfx->Width; x++)
						Read_TARGA_RGB(ifile, (int)header[16], palette, gfx->Data + (y * gfx->Width) + x);
			}
		}
		/*if(verbose) printf("gfx data read (%i*%i);\n", gfx->Width, gfx->Height);*/
		free(palette);
		return 0;
	} else if(((header[2] == 10) && (header[16] >= 16)) || (((header[2] == 9) || header[2] == 11) && (header[16] == 8))) {
	/* type 9: 8 bit/palette RLE; type 10: 16 bit++, RLE; type 11: 8 bit monocrome, RLE; */
		for(z = 0; ((l = getc(ifile)) != EOF) && (z < (gfx->Height * gfx->Width)); ) {
			if(l & 128) { /* compressed data follows */
				l &= 127; /* extracting length */
				Read_TARGA_RGB(ifile, (int)header[16], palette, &c);
				for(s = 0; (s <= l); s++) {
					x = z % gfx->Width;
					y = (z - x) / gfx->Width;
					if(header[17] & 16)
						x = gfx->Width - 1 - x;
					if(!(header[17] & 32))
						y = gfx->Height - 1 - y;
					if((x < gfx->Width) && (y < gfx->Height) && (x >= 0) && (y >= 0))
						gfx->Data[(y * gfx->Width) + x] = c;
					else
						fprintf(stderr, "(%i, %i) => error\n", x, y);
					z++;
				}
			} else { /* uncompressed data follows */
				for(s = 0; (s <= l); s++) {
					x = z % gfx->Width;
					y = (z - x) / gfx->Width;
					if(header[17] & 16)
						x = gfx->Width - 1 - x;
					if(!(header[17] & 32))
						y = gfx->Height - 1 - y;
					if((x < gfx->Width) && (y < gfx->Height) && (x >= 0) && (y >= 0))
						Read_TARGA_RGB(ifile, (int)header[16], palette, gfx->Data + (y * gfx->Width) + x);
					else
						fprintf(stderr, "(%i, %i) => error\n", x, y);
					z++;
				}
			}
		}
		/*if(verbose) printf("gfx data read (%i*%i);\n", gfx->Width, gfx->Height);*/
		free(palette);
		return 0;
	} else {
		if(verbose) printf("FAILED;\n"); else fprintf(stderr, "reading gfx data...FAILED;\n");
		fprintf(stderr, "Unsupported Targa Data Format %i@%i;\nOnly TARGA types 1, 2, 3 (uncompressed) and 9, 10, 11 (RLE) are supported;\n", header[2], header[16]);
		free(palette);
		return -1;
	}
}


int Read_TARGA_RGB(FILE *ifile, int bits, int *palette, int *c) {
	static int a, r, g, b, z1, z2;
	a = 0;
	if(bits == 8) {
		if(palette)
			(*c) = palette[(int)getc(ifile)];
		else
			(*c) = (int)getc(ifile) * 65793;  /* 65793 = (1+256+65536) */
	} else if(bits == 16) {
			z1 = getc(ifile);
			z2 = getc(ifile);
			r = (int)((255.0/31.0) * (float)((z1 & 124) / 4) );  /* 124 = 64 + 32 + 16 + 8 + 4 */
			g = (int)((255.0/31.0) * (float)(((z1 & 3) * 8) | ((z2 & 224) / 32)) );  /* 224 = 128 + 64 + 32 */
			b = (int)((255.0/31.0) * (float)(z2 & 31) );
			(*c) = r + (g * 256) + (b * 65536);
			a = z1 & 128;
		} else {
			r = getc(ifile);
			g = getc(ifile);
			b = getc(ifile);
			(*c) = r + (g * 256) + (b * 65536);
			if(bits == 32)
				a = getc(ifile);
		}
	return a; /* attribute (alpha/transparency information, 32 bit only) */
}

/* writes all gfx data to stdout, preceded by the resolution of gfx data - everything 32 bit wide */
int Write_PPM (FILE *ofile, struct GFX_DATA *gfx)
{
	int a, x, y;

	fprintf(ofile, "P6\n");
	fprintf(ofile, "%i %i\n", gfx->Width, gfx->Height);
	fprintf(ofile, "255\n");
	a = 0;
	for(y = 0; (y < gfx->Height) && (a != EOF); y++)
		for(x = 0; (x < gfx->Width) && (a != EOF); x++)	{
			putc(gfx->Data[y * gfx->Width + x] & 255, ofile);
			putc((gfx->Data[y * gfx->Width + x] / 256) & 255, ofile);
			a = putc((gfx->Data[y * gfx->Width + x] / 65536) & 255, ofile);
		}
	if(a != EOF) {
		return 0;
	} else {
		if(verbose) printf("FAILED;\n"); else fprintf(stderr, "writing gfx data...FAILED;\n");
			fprintf(stderr, "cannot write to file! (disk full?)\n");
		return -1;
	}
}

int Write_TARGA (FILE *ofile, struct GFX_DATA *gfx)
{
	int a, x, y;
	/* standard TARGA header consists of 18 bytes */
	/* id tag length */
	putc(strlen("created by stereograph"), ofile);
	/* color pallette, yes/no */
	putc(0, ofile);
	/* TARGA type 2: RGB 24 bit, no pallette */
	putc(2, ofile);
	/* 5 following bytes contain only pallette information */
	putc(0, ofile);
	putc(0, ofile);
	putc(0, ofile);
	putc(0, ofile);
	putc(0, ofile);
	/* x offset */
	putc(0, ofile);
	putc(0, ofile);
	/* y offset */
	putc(0, ofile);
	putc(0, ofile);
	/* width, low byte first */
	putc(gfx->Width & 255, ofile);
	putc((gfx->Width / 256) & 255, ofile);
	/* height */
	putc(gfx->Height & 255, ofile);
	putc((gfx->Height / 256) & 255, ofile);
	/* bits per pixel */
	putc(32-8, ofile);
	/* Image Descriptor Flags */
	putc(0, ofile);
	fwrite("created by stereograph", sizeof(char), strlen("created by stereograph"), ofile);
	/* data follows */
	a = 0;
	for(y = gfx->Height - 1; (y >= 0) && (a != EOF); y--)
		for(x = 0; (x < gfx->Width) && (a != EOF); x++)	{
			putc(gfx->Data[y * gfx->Width + x] & 255, ofile);
			putc((gfx->Data[y * gfx->Width + x] / 256) & 255, ofile);
			a = putc((gfx->Data[y * gfx->Width + x] / 65536) & 255, ofile);
		}
	if(a != EOF) {
		return 0;
	} else {
		if(verbose) printf("FAILED;\n"); else fprintf(stderr, "writing gfx data...FAILED;\n");
			fprintf(stderr, "cannot write to file! (disk full?)\n");
		return -1;
	}
}


int fdata_to_GFX(float *datafield[3], struct GFX_DATA *gfx) {
	int z;
	float a, b;
	a = frand1();
	b = sqrt(a * frand1());
	for(z = 0; z < (gfx->Width*gfx->Height); z++) {
		if(datafield[3][z] > a) {
			gfx->Data[z] = (int)(datafield[2][z] * (rand()&255));
			gfx->Data[z] += (int)(datafield[0][z] * (rand()&255))*256;
			gfx->Data[z] += (int)(datafield[1][z] * (rand()&255))*65536;
		} else if (datafield[3][z] > b) {
			gfx->Data[z] = (int)(datafield[0][z] * 255.0);
			gfx->Data[z] += (int)(datafield[1][z] * 255.0)*256;
			gfx->Data[z] += (int)(datafield[2][z] * 255.0)*65536;
		} else {
			gfx->Data[z] = (int)(datafield[0][z] * datafield[4][z] * 255.0);
			gfx->Data[z] += (int)(datafield[1][z] * datafield[4][z] * 255.0)*256;
			gfx->Data[z] += (int)(datafield[2][z]  * datafield[4][z] * 255.0)*65536;
			gfx->Data[z] += (int)(datafield[1][z] * (1.0 - datafield[4][z]) * 255.0);
			gfx->Data[z] += (int)(datafield[2][z] * (1.0 - datafield[4][z]) * 255.0)*256;
			gfx->Data[z] += (int)(datafield[0][z]  * (1.0 - datafield[4][z]) * 255.0)*65536;
		}
	}
	return 0;
}


float c_sin(int a, float b) {
	static float *sincache = NULL;
	static float cb = 0.0, maxb = 0.0;

	if((int)b <= 0) {
		free(sincache);
		sincache = NULL;
		cb = 0.0;
		maxb = 0.0;
	} else {
		if(cb != b) {
			if (maxb < b) {
				sincache = (float*)realloc(sincache, (int)b * sizeof(float));
				maxb = b;
			}
			for(cb = 0.0; cb < b; cb += 1.0)
				sincache[(int)cb] = sin(PI * (1.5 + (cb / b)));
			cb = b;
		}
		return sincache[a];
	}
	return 0.0;
}


void Sinline_Fill_H(float *dataline, int x1, int x2, int width) {
	static int z;
	static float a;

	a = 0.5 * (dataline[x1] - dataline[x2 % width]);
	for(z = x1 + 1; z < x2; z++)
		//dataline[z % width] = dataline[x1] - (a * (sin(PI * (1.5 + ((float)(z - x1) / (float)(x2 - x1)))) + 1.0));
		dataline[z % width] = dataline[x1] - (a * (c_sin(z - x1, (float)(x2 - x1)) + 1.0));
}


void Sinline_Fill_H_flat(float *dataline, int x1, int x2, int width) {
	static int z;
	static float a;

	a = 0.5 * (dataline[x1] - dataline[x2 % width]);
	for(z = x1 + 1; z < x2; z++)
		dataline[z % width] = dataline[x1] - a;
}


void Sinline_Fill_datafield_H(float* datafield, int width, int height) {
	int tx, x, y;
	int lines, line_offset;
	float phase;
	float period;
	//float a;

	lines = 1 + (5*width/height) + rand() % 4;
	line_offset = (int)((float)width * frand1()) / lines;
	for(tx = 0; tx < lines; tx++) {
		//period = PI / (float)height / (((a = frand1()) == 0) ? 1.0 : a);    // to avoid division by zero
		period = (PI / (float)height) / (frand1()*0.5 + 0.1);
		phase = frand1() * 2.0 * PI;
		x = line_offset + ((width / lines) * tx);
		for(y = 0; y < height; y++) {
			datafield[(y * width) + x] = 0.5 + (0.5 * sin(phase + period * (float)y));
			if(tx > 0)
				Sinline_Fill_H(datafield + (y * width), x - (width/lines), x, width);
		}
	}
	for(y = 0; y < height; y++)
		Sinline_Fill_H(datafield + (y * width), line_offset + ((width / lines) * (lines - 1)), line_offset + width, width);
}


void Sinline_Fill_V(float *dataline, int y1, int y2, int height, int width) {
	static int z;
	static float a;

	a = 0.5 * (dataline[y1 * width] - dataline[(y2 % height) * width]);
	for(z = y1 + 1; z < y2; z++)
		//dataline[(z % height) * width] = dataline[y1*width] - (a * (sin(PI * (1.5 + ((float)(z - y1) / (float)(y2 - y1)))) + 1.0));
		dataline[(z % height) * width] = dataline[y1*width] - (a * (c_sin(z - y1, (float)(y2 - y1)) + 1.0));
}


void Sinline_Fill_V_flat(float *dataline, int y1, int y2, int height, int width) {
	static int z;
	static float a;

	a = 0.5 * (dataline[y1 * width] - dataline[(y2 % height) * width]);
	for(z = y1 + 1; z < y2; z++)
		dataline[(z % height) * width] = dataline[y1*width] - a;
}


void Sinline_Fill_datafield_V(float* datafield, int width, int height) {
	int ty, x, y;
	int lines, line_offset;
	float phase;
	float period;

	lines = 1 + (3*height/width) + rand() % 4;
	line_offset = (int)((float)height * frand1()) / lines;
	for(ty = 0; ty < lines; ty++) {
		period = 2.0*(PI / (float)width) * (rand()%4 + 1);    // to avoid division by zero
		phase = frand1() * 2.0 * PI;
		y = line_offset + ((height / lines) * ty);
		for(x = 0; x < width; x++) {
			datafield[(y * width) + x] = 0.5 + (0.5 * sin(phase + period * (float)x));
			if(ty > 0)
				Sinline_Fill_V(datafield + x, y - (height/lines), y, height, width);
		}
	}
	for(x = 0; x < width; x++)
		Sinline_Fill_V(datafield + x, line_offset + ((height / lines) * (lines - 1)), line_offset + height, height, width);
}


void Sinline_merge_datafields_add(float *datafield, float *datafield1, float *datafield2, int c) {
	int z;
	for(z = 0; z < c; z++)
		datafield[z] = 0.5 * (datafield1[z] + datafield2[z]);
}


void Sinline_merge_datafields_mul(float *datafield, float *datafield1, float *datafield2, int c) {
	int z;
	for(z = 0; z < c; z++)
		datafield[z] = sqrt(datafield1[z] * datafield2[z]);
}


int Generate_Sinline_Texture(struct GFX_DATA *gfx)
{
	int z;
	float *a, *datafield[5];

	for(z = 0; (z < 5) && (a || !z); z++)
		a = datafield[z] = (float*) malloc(gfx->Width * gfx->Height * sizeof(float));
	if(a) {

			Sinline_Fill_datafield_H(datafield[0], gfx->Width, gfx->Height);
			Sinline_Fill_datafield_V(datafield[1], gfx->Width, gfx->Height);
			Sinline_merge_datafields_add(datafield[2], datafield[0], datafield[1], gfx->Width*gfx->Height);
			Sinline_Fill_datafield_H(datafield[0], gfx->Width, gfx->Height);
			Sinline_Fill_datafield_V(datafield[1], gfx->Width, gfx->Height);
			Sinline_merge_datafields_mul(datafield[2], datafield[2], datafield[1], gfx->Width*gfx->Height);
			Sinline_merge_datafields_add(datafield[2], datafield[2], datafield[0], gfx->Width*gfx->Height);

			Sinline_Fill_datafield_H(datafield[0], gfx->Width, gfx->Height);
			Sinline_Fill_datafield_V(datafield[1], gfx->Width, gfx->Height);
			Sinline_merge_datafields_mul(datafield[3], datafield[0], datafield[1], gfx->Width*gfx->Height);
			Sinline_Fill_datafield_H(datafield[0], gfx->Width, gfx->Height);
			Sinline_Fill_datafield_V(datafield[1], gfx->Width, gfx->Height);
			Sinline_merge_datafields_add(datafield[3], datafield[3], datafield[0], gfx->Width*gfx->Height);
			Sinline_merge_datafields_add(datafield[3], datafield[3], datafield[1], gfx->Width*gfx->Height);

			Sinline_Fill_datafield_H(datafield[0], gfx->Width, gfx->Height);
			Sinline_Fill_datafield_V(datafield[1], gfx->Width, gfx->Height);
			Sinline_merge_datafields_mul(datafield[1], datafield[0], datafield[1], gfx->Width*gfx->Height);
			Sinline_Fill_datafield_H(datafield[0], gfx->Width, gfx->Height);
			Sinline_merge_datafields_add(datafield[1], datafield[0], datafield[1], gfx->Width*gfx->Height);
			Sinline_Fill_datafield_V(datafield[0], gfx->Width, gfx->Height);
			Sinline_merge_datafields_add(datafield[1], datafield[0], datafield[1], gfx->Width*gfx->Height);

			Sinline_Fill_datafield_H(datafield[4], gfx->Width, gfx->Height);
			Sinline_Fill_datafield_V(datafield[1], gfx->Width, gfx->Height);
			Sinline_merge_datafields_mul(datafield[1], datafield[4], datafield[1], gfx->Width*gfx->Height);
			Sinline_Fill_datafield_H(datafield[4], gfx->Width, gfx->Height);
			Sinline_merge_datafields_add(datafield[1], datafield[4], datafield[1], gfx->Width*gfx->Height);
			Sinline_Fill_datafield_V(datafield[4], gfx->Width, gfx->Height);
			Sinline_merge_datafields_add(datafield[1], datafield[4], datafield[1], gfx->Width*gfx->Height);

			for(z = 0; z < 4; z++) {
				Sinline_Fill_datafield_H(datafield[4], gfx->Width, gfx->Height);
				Sinline_merge_datafields_add(datafield[z], datafield[4], datafield[z], gfx->Width*gfx->Height);
				Sinline_Fill_datafield_V(datafield[4], gfx->Width, gfx->Height);
				Sinline_merge_datafields_add(datafield[z], datafield[4], datafield[z], gfx->Width*gfx->Height);
			}
			Sinline_Fill_datafield_V(datafield[4], gfx->Width, gfx->Height);

		c_sin(0, 0);
		fdata_to_GFX(datafield, gfx);
		for(z = 0; z < 5; z++)
			free(datafield[z]);
	} else {
		for(z = 0; (z < 5) && datafield[z]; z++)
			free(datafield[z]);
		free(gfx->Data);
		gfx->Data = NULL;
		if (verbose) printf("FAILED\n");
		fprintf(stderr, "could not allocate memory;\n");
		return -1;
	}
	return 0;
}
