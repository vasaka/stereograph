/* Stereograph
 * Header file;
 * Copyright (c) 2000-2003 by Fabian Januszewski <fabian.linux@januszewski.de>
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


/* error handling */
#define GFX_ERROR_READ_ERROR       -1
#define GFX_ERROR_WRITE_ERROR      -2
#define GFX_ERROR_UNSUPP_FORMAT    -3
#define GFX_ERROR_MEMORY_PROBLEM   -4
#define GFX_ERROR_UNSUPP_RANDOMTEX -5
#define GFX_ERROR_UNSUPP_T_LEVEL   -6
#define GFX_ERROR_LIBPNG           -7
#define GFX_ERROR_LIBJPG           -8

/* constants for io format definitions */
#define GFX_IO_X11   0
#define GFX_IO_JPG   1
#define GFX_IO_PNG   2
#define GFX_IO_PPM   3
#define GFX_IO_TARGA 4
#define GFX_IO_C     5

/* constant types for random textures */
#define GFX_TEX_BW        0
#define GFX_TEX_COLORED   1
#define GFX_TEX_GRAYSCALE 2
#define GFX_TEX_SINLINE   3

/* consts for GFX_AdjustLevel */
#define GFX_T_NO_LEVEL   0
#define GFX_T_BACK_LEVEL 1
#define GFX_T_TOP_LEVEL  2


/* gfx io functions */
int GFX_Read_File (char *file_name, struct GFX_DATA *gfx);
int GFX_Write_File (char *file_name, int output_format, struct GFX_DATA *gfx);

/* invert a base gfx */
int GFX_Invert(struct GFX_DATA *gfx);

/* adjust back base levels */
int GFX_T_AdjustLevel(struct GFX_DATA *T_gfx, int T_layers, int T_level);

/* add two black triangles to aid */
int GFX_AddTriangles(int x, int size, int width, struct GFX_DATA *gfx);

/* generates a random texture */
int GFX_Generate_RandomTexture(struct GFX_DATA *gfx, int width, int height, int type);

/* resizes a gfx - decreases AND ONLY DECREASES the width            */
/* if the width argument is zero, no changes to the gfx are applied  */
int GFX_Resize(struct GFX_DATA *gfx, int width);


/* gfxio internals */
int GFX_Identify_File(FILE *ifile, unsigned char *check_header);

int GFX_Read_C (FILE *ifile, char *file_name, unsigned char* check_header, struct GFX_DATA *gfx);

int GFX_Read_JPG (FILE *ifile, unsigned char* check_header, struct GFX_DATA *gfx);
int GFX_Read_PNG (FILE *ifile, unsigned char* check_header, struct GFX_DATA *gfx);
int GFX_Read_PPM (FILE *ifile, unsigned char* check_header, struct GFX_DATA *gfx);
int GFX_Read_TARGA (FILE *ifile, unsigned char* check_header, struct GFX_DATA *gfx);
int GFX_Read_TARGA_RGB(FILE *ifile, int bits, int *palette, int *c);

int GFX_Write_X11 (FILE *ofile, struct GFX_DATA *gfx);
int GFX_Write_JPG (FILE *ofile, struct GFX_DATA *gfx);
int GFX_Write_PNG (FILE *ofile, struct GFX_DATA *gfx);
int GFX_Write_PPM (FILE *ofile, struct GFX_DATA *gfx);
int GFX_Write_TARGA (FILE *ofile, struct GFX_DATA *gfx);

int GFX_Generate_SinlineTexture(struct GFX_DATA *gfx);



