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



struct GFX_DATA {
	/* measurements */
	int Width;
	int Height;

	/* RGB-Data, 24bit coding @ ONE 32 bit integer, standard eastern orientation (left to right, top to bottom) */
	int *Data;
};

struct PARAMS {
	int Linear;
	int T_layers;
	int Startx;
	int Starty;
	int AA;
	int Zoom;
	int AAr;
	float Front;
	float Distance;
	float Eyeshift;
        /* not handled by the renderer itself, can be outcommented when using the renderer without stereograph */
	int T_level;
	int Invert;
	int Triangles;	
};

/* engine internals exclusively */
struct RENDERER_DATA {
	int nlayers;
	int startx;
	int starty;
	int max_change_tex_width;
	float *right_change_map;
	float *left_change_map;
	int naa;
	int nzoom;
	int nfactor;
	int *scanline;
	int **tscanline;
	float realdist;
	float internal_a;
	float eyeshift;
};


/* sets pointers of the engine interface */
int Get_GFX_Pointers(struct PARAMS **pParam, struct GFX_DATA **pBase, struct GFX_DATA **pTexture, struct GFX_DATA **pStereo);

/* sets pointers of the engine interface for rendering transparency */
int Get_T_GFX_Pointers(struct GFX_DATA ****T_pBase, struct GFX_DATA ****T_pTexture);

/* processes line number base_line */
int ProcessLine(int base_line);

/* clears the renderer */
void Clear_Renderer(void);
/* initializes renderer */
int Initialize_Renderer(void);


/*** engine internal functions ***/
int RenderLine(int *base_data, int *texture_data, int *stereo_data, int base_line);
int InitMap(float *change_map, int a, int b, float v);
int FillMapsLeftToRight(int *base_data);
float GetChange(int x, int *base_data);
int NormalScan(int *stereo_data, int *texture_data);
int AAScan(int *stereo_data, int *texture_data);
int ZoomScan(int *stereo_data, int *texture_data);
int ZoomAAScan(int *stereo_data, int *texture_data);