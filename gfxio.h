/* Stereograph
 * Header file;
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


#define GFX_PNG   0
#define GFX_PPM   1
#define GFX_TARGA 2

#define TEX_BW        0
#define TEX_COLORED   1
#define TEX_GRAYSCALE 2
#define TEX_SINLINE   3

#define T_NO_LEVEL   0
#define T_BACK_LEVEL 1
#define T_TOP_LEVEL  2


/* gfx io functions */
int Read_Gfx_File (char *file_name, struct GFX_DATA *gfx);
int Write_Gfx_File (char *file_name, int output_format, struct GFX_DATA *gfx);

/* invert a base gfx */
int Invert_GFX(struct GFX_DATA *gfx);

/* adjust back base levels */
int T_adjust_level(struct GFX_DATA **T_gfx, int T_layers, int T_level);

/* add two black triangles to aid */
int Add_Triangles(int x, int size, int width, struct GFX_DATA *gfx);

/* generates a random texture */
int Generate_Random_Texture(struct GFX_DATA *gfx, int width, int height, int type);

/* resizes a gfx - decreases the width */
int Resize_GFX(struct GFX_DATA *gfx, int width);



/*** gfxio internals ***/
int identify_GFX_file(FILE *ifile, unsigned char *check_header);

int Read_PNG (FILE *ifile, unsigned char* check_header, struct GFX_DATA *gfx);
int Read_TARGA (FILE *ifile, unsigned char* check_header, struct GFX_DATA *gfx);
int Read_TARGA_RGB(FILE *ifile, int bits, int *palette, int *c);

int Write_PPM (FILE *ofile, struct GFX_DATA *gfx);
int Write_PNG (FILE *ofile, struct GFX_DATA *gfx);
int Write_TARGA (FILE *ofile, struct GFX_DATA *gfx);

int Generate_Sinline_Texture(struct GFX_DATA *gfx);