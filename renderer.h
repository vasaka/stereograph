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


/* maximum number of layers in transparency mode */
#define RENDERER_T_MAX_LAYERS  16


/* error handling */
#define RENDERER_ERROR_INIT_ILLEGAL_PARAMS        -16
#define RENDERER_ERROR_INIT_ILLEGAL_BASE_TEX_RES  -17
#define RENDERER_ERROR_INIT_MEMORY_PROBLEM        -18
#define RENDERER_ERROR_INIT_ILLEGAL_T_LAYERS      -19


/* this struct is used to hold the graphics data used for input and output by the renderer */
struct GFX_DATA {
	/* measurements */
	int Width;
	int Height;

	/* RGB-Data, 24bit coding @ ONE 32 bit integer, standard eastern orientation
           (left to right, top to bottom) */
	int *Data;
};

/* RENDERER_PARAMS, this set or params is used as interface to the renderer,
   renderer.c -> renderer_param_init contains some useful default values      */
/* a more specialised struct (STEREOGRAPH_PARAMS) already combines all
   necessary parametres and stereograph_param_init from stereograph.c
   contains useful standard values and also manages the necessary inits of
   the following two structs. look at stereograph.h for further details       */
struct RENDERER_PARAMS {
        int Linear;
	int T_Layers;
	int StartX;
	int StartY;
	int AA;
	int Zoom;
	int AAr;
	float Front;
	float Distance;
	float Eyeshift;
};


/* engine internals exclusively */
struct RENDERER_DATA {
        int linear;
	int nlayers;
	int startx;
	int starty;
        float distance;
        float front;
	int max_change_tex_width;
	float *right_change_map;
	float *left_change_map;
        int aar;
	int naa;
	int nzoom;
	int nfactor;
	int *scanline;
	int *tscanline[RENDERER_T_MAX_LAYERS];
	float realdist;
	float internal_a;
	float eyeshift;
        struct GFX_DATA *pGFX_Base, *pGFX_Texture, *pGFX_Stereo;
};


/* sets pointers of the engine interface and initializes with standard values */
/* stereograph_param_init from stereograph.c gives you a reference
   implementation for needed inits                                            */
int Renderer_Param_Init(struct RENDERER_PARAMS *params);
int Renderer_GFX_Init(struct RENDERER_DATA *renderer, struct GFX_DATA *pGFX_Base, struct GFX_DATA *pGFX_Texture, struct GFX_DATA *pGFX_Stereo);

/* initializes renderer by setting up all necessary params in the renderer     */
int Renderer_Initialize(struct RENDERER_DATA *renderer, struct RENDERER_PARAMS *params);
/* with an initialized RENDERER_DATA you may process the image line by line
   in any order; note that you must only and really only call this function
   for every line of the defined base gfx;
   see stereograph.c -> stereograph_gfx_process for a reference implementation  */
int Renderer_ProcessLine(int base_line, struct RENDERER_DATA *renderer);
/* clears the renderer                                                          */
/* you must clear the renderer to free up all statically allocated mem used
   during the rendering process; note that you must do this even if there
   occurs an error during initialization by Renderer_Initialize                 */
void Renderer_Close(struct RENDERER_DATA *renderer);

/* the entire rendering process is presented in stereograph.c, feel free to
   reuse these functions in your implementations                                */


/* engine internals */
int Renderer_RenderLine(int *base_data, int *texture_data, int *stereo_data, int base_line, struct RENDERER_DATA *renderer);
int Renderer_InitMap(float *change_map, int a, int b, float v);
int Renderer_FillMapsLeftToRight(int *base_data, struct RENDERER_DATA *renderer);
float Renderer_GetChange(int x, int *base_data, struct RENDERER_DATA *renderer);
int Renderer_NormalScan(int *stereo_data, int *texture_data, struct RENDERER_DATA *renderer);
int Renderer_AAScan(int *stereo_data, int *texture_data, struct RENDERER_DATA *renderer);
int Renderer_ZoomScan(int *stereo_data, int *texture_data, struct RENDERER_DATA *renderer);
int Renderer_ZoomAAScan(int *stereo_data, int *texture_data, struct RENDERER_DATA *renderer);
