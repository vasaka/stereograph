/* Stereograph 0.31a, 18/08/2001;
 * the renderer, stereographer's engine;
 * Copyright (c) 2000-2001 by Fabian Januszewski <fabian.linux@januszewski.de>
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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "renderer.h"


/* here we go... */


/* 1st of all: init with standard values */
int Renderer_Param_Init(struct RENDERER_PARAMS *params) {

	params->Linear = 1;
        params->T_Layers = 0;
	params->Zoom = 1;
	params->AA = 4;
	params->AAr = 0;
	params->StartX = 0;
	params->StartY = 0;
	params->Front = 0.4;
	params->Distance = 5.0;
	params->Eyeshift = 0.0;

	return 0;
}

int Renderer_GFX_Init(struct RENDERER_DATA *renderer, struct GFX_DATA *pGFX_Base, struct GFX_DATA *pGFX_Texture, struct GFX_DATA *pGFX_Stereo) {

        renderer->pGFX_Base = pGFX_Base;
        renderer->pGFX_Texture = pGFX_Texture;
        renderer->pGFX_Stereo = pGFX_Stereo;

        return 0;
}


/* render a mere line */
int Renderer_ProcessLine(int base_line, struct RENDERER_DATA *renderer) {
	if(renderer->nlayers == 0)
		return Renderer_RenderLine(
					   renderer->pGFX_Base->Data + (renderer->pGFX_Base->Width * base_line),
					   renderer->pGFX_Texture->Data + (renderer->pGFX_Texture->Width * ((base_line + renderer->starty) % renderer->pGFX_Texture->Height)),
					   renderer->pGFX_Stereo->Data + (renderer->pGFX_Stereo->Width * base_line * renderer->nzoom),
					   base_line,
					   renderer
					   );
	else {
		int z, a, x, c_r = 0, c_g = 0, c_b = 0;
		a = 0;
		for(z = 0; (z < renderer->nlayers) && !a; z++)
			a = Renderer_RenderLine(
						renderer->pGFX_Base[z].Data + renderer->pGFX_Base[z].Width * base_line,
						renderer->pGFX_Texture[z].Data + renderer->pGFX_Texture[z].Width * ((base_line + renderer->starty) % renderer->pGFX_Texture[z].Height),
						renderer->tscanline[z],
						base_line,
						renderer
						);

		for(x = 0; x < (renderer->pGFX_Stereo->Width * renderer->nzoom); x++) {
			c_r = c_g = c_b = 0;
			for(z = 0; z < renderer->nlayers; z++) {
				c_r +=  renderer->tscanline[z][x]      & 255;
				c_g += (renderer->tscanline[z][x] >> 8) & 255;
				c_b += (renderer->tscanline[z][x] >> 16) & 255;
			}
			renderer->pGFX_Stereo->Data[renderer->pGFX_Stereo->Width*base_line*renderer->nzoom + x] +=  (c_r / renderer->nlayers) & 255;
			renderer->pGFX_Stereo->Data[renderer->pGFX_Stereo->Width*base_line*renderer->nzoom + x] += ((c_g / renderer->nlayers) & 255) << 8;
			renderer->pGFX_Stereo->Data[renderer->pGFX_Stereo->Width*base_line*renderer->nzoom + x] += ((c_b / renderer->nlayers) & 255) << 16;
		}
		return a;
	}
}


/* destroy memory reservations, internals; NOTE: Stereo.Data also belongs to the renderer, must be freed outside! */
void Renderer_Close(struct RENDERER_DATA *renderer)
{
	int z;

	if(renderer->right_change_map != NULL)
	        free(renderer->right_change_map);
	if(renderer->left_change_map != NULL)
	        free(renderer->left_change_map);
	for(z = 0; z < renderer->nlayers; z++)
	        if(renderer->tscanline[z] != NULL)
		        free(renderer->tscanline[z]);
        if(renderer->scanline != NULL)
	        free(renderer->scanline);
}


/* initialize internal rendering engine data */
int Renderer_Initialize(struct RENDERER_DATA *renderer, struct RENDERER_PARAMS *params)
{
	renderer->startx = params->StartX;
	renderer->starty = -params->StartY;
	while(renderer->starty < 0)
		renderer->starty += renderer->pGFX_Texture->Height;
	renderer->distance = params->Distance;
	renderer->front = params->Front;
	renderer->max_change_tex_width = (int)((renderer->pGFX_Texture->Width - 1) * renderer->front);
	renderer->right_change_map = (float*)malloc(sizeof(float) * (renderer->pGFX_Base->Width + 1));
	renderer->left_change_map = (float*)malloc(sizeof(float) * (renderer->pGFX_Base->Width + 1));
	renderer->naa = params->AA + 0;
	if(renderer->naa == 0) renderer->naa = 1;
	renderer->nzoom = params->Zoom + 0;
	if(renderer->nzoom == 0) renderer->nzoom = 1;
	renderer->aar = params->AAr;
	renderer->linear = params->Linear;
	renderer->realdist = renderer->pGFX_Texture->Width * (params->Distance + 1);
	renderer->internal_a = (1.0 / (renderer->pGFX_Texture->Width * renderer->pGFX_Texture->Width)) - (0.25 / (renderer->realdist * renderer->realdist));
	renderer->eyeshift = params->Eyeshift * 0.5;
	renderer->nfactor = renderer->naa * renderer->nzoom;
	/* definitions check */
	if(
		((params->Distance + 1) > 1.0) && ((params->Distance + 1) <= 21.0) &&
		(renderer->eyeshift >= -0.5) && (renderer->eyeshift <= 0.5) &&
		(renderer->naa > 0) && (renderer->naa <= 32) &&
		(renderer->nzoom > 0) && (renderer->nzoom <= 32) &&
		(renderer->startx >= 0) && (renderer->startx < (renderer->pGFX_Base->Width - renderer->pGFX_Texture->Width)) &&
		(params->StartY >= 0) &&
		(params->Front > 0.0) && (params->Front <= 1.0)
	) {
	        if(!params->T_Layers) {
		        /* single trace preparations */
		        renderer->nlayers = 0;
			if(params->AA - 1)
				renderer->scanline = (int*)malloc(sizeof(int) * (renderer->pGFX_Base->Width * renderer->nfactor));
			else
				renderer->scanline = NULL;

			renderer->pGFX_Stereo->Width = renderer->pGFX_Base->Width * renderer->nzoom;
			renderer->pGFX_Stereo->Height = renderer->pGFX_Base->Height * renderer->nzoom;
			renderer->pGFX_Stereo->Data = (int*)malloc(sizeof(int) * renderer->pGFX_Stereo->Width * renderer->pGFX_Stereo->Height);

	        	if(renderer->right_change_map && renderer->left_change_map && renderer->pGFX_Stereo->Data && renderer->pGFX_Base->Data && renderer->pGFX_Texture->Data) {
				if(renderer->pGFX_Base->Width >= renderer->pGFX_Texture->Width)
				        /* if control reaches this point everything is ready */
					return 0;
				else
				        /* illegal base/texture width */
				        return RENDERER_ERROR_INIT_ILLEGAL_BASE_TEX_RES;
			} else
			        /* memory problem */
			        return RENDERER_ERROR_INIT_MEMORY_PROBLEM;
		} else {
		        /* transparency handling */
		        renderer->nlayers = params->T_Layers + 1;
			if((renderer->nlayers > 1) && (renderer->nlayers <= RENDERER_T_MAX_LAYERS)) {

			        int z;
				if(params->AA - 1)
				        renderer->scanline = (int*)malloc(sizeof(int) * (renderer->pGFX_Base->Width * renderer->nfactor));
				else
				        renderer->scanline = NULL;

				renderer->pGFX_Stereo->Width = renderer->pGFX_Base->Width * renderer->nzoom;
				renderer->pGFX_Stereo->Height = renderer->pGFX_Base->Height * renderer->nzoom;
				renderer->pGFX_Stereo->Data = (int*)malloc(sizeof(int) * renderer->pGFX_Stereo->Width * renderer->pGFX_Stereo->Height);

				for(z = 0; z < renderer->nlayers; z++)
				        renderer->tscanline[z] = (int*)malloc(sizeof(int) * renderer->pGFX_Stereo->Width * renderer->nzoom);

				for(z = 0; z < renderer->nlayers; z++) {
				        if(renderer->right_change_map && renderer->left_change_map && renderer->pGFX_Stereo->Data && renderer->pGFX_Base[z].Data && renderer->pGFX_Texture[z].Data && renderer->tscanline[z]) {
					        /* check for correct resolutions */
					        if((renderer->pGFX_Base->Width != renderer->pGFX_Base[z].Width) || (renderer->pGFX_Base->Height != renderer->pGFX_Base[z].Height) || (renderer->pGFX_Texture->Width != renderer->pGFX_Texture[z].Width))
						        return RENDERER_ERROR_INIT_ILLEGAL_BASE_TEX_RES;
					} else
					        /* memory problem */
					        return RENDERER_ERROR_INIT_MEMORY_PROBLEM;
				}

				return 0;
			} else {
			        /* number of requested layers exceeds max definition */
			        return RENDERER_ERROR_INIT_ILLEGAL_T_LAYERS;
			}

		}
	} else {
	        /* illegal definition */
		return RENDERER_ERROR_INIT_ILLEGAL_PARAMS;
	}       	
}


/* Render a mere line of data, may be completely independent of any gfx_data structure */
/* reorganized aar implementation during version span 0.24-0.25, 3rd of june 2000      */
/* added additional InitMap entry 'cause of a buggy implementation 5th of june 2000    */
int Renderer_RenderLine(int *base_data, int *texture_data, int *stereo_data, int base_line, struct RENDERER_DATA *renderer)
{
	Renderer_InitMap(renderer->left_change_map, 0, renderer->pGFX_Base->Width, (float)(renderer->pGFX_Texture->Width + 1));
	Renderer_InitMap(renderer->right_change_map, 0, renderer->pGFX_Base->Width, (float)-(renderer->pGFX_Texture->Width + 1));
	
	Renderer_FillMapsLeftToRight(base_data, renderer);

	if(renderer->aar) {
		/* tracing alternative startx */
		/* reimplemented in 0.25 */
		if(base_line & 1)
			while(((renderer->startx + (int)renderer->left_change_map[renderer->startx]) < (renderer->pGFX_Base->Width - renderer->pGFX_Texture->Width)) && ((renderer->startx + (int)renderer->left_change_map[renderer->startx]) > renderer->startx))
				renderer->startx += (int)renderer->left_change_map[renderer->startx];
		else
			while(((renderer->startx + (int)renderer->right_change_map[renderer->startx]) >= 0) && ((renderer->startx + (int)renderer->right_change_map[renderer->startx]) < renderer->startx))
				renderer->startx += (int)renderer->right_change_map[renderer->startx];
	}
	if(renderer->naa - 1)
	{
   		if(renderer->nzoom - 1)
			return Renderer_ZoomAAScan(stereo_data, texture_data, renderer);
		else
			return Renderer_AAScan(stereo_data, texture_data, renderer);
	} else
		/* small fix here (0.25) */
		if(renderer->nzoom - 1)
			return Renderer_ZoomScan(stereo_data, texture_data, renderer);
		else
			return Renderer_NormalScan(stereo_data, texture_data, renderer);
}


/* intialize changemap internals */
/* changed formal expression june 5th 2000 */
int Renderer_InitMap(float *change_map, int a, int b, float v)
{
	while(a < b)
		change_map[a++] = v;
	return 0;
}


/* set changevalues */
int Renderer_FillMapsLeftToRight(int *base_data, struct RENDERER_DATA *renderer)
{
        int z, scanx, i_c;
	float c;

	/* advanced full centered perspective; reimplemented february 4th 2000 */
	/* added eyeshift february 11th 2000 */
	/* fixed conditions here june 5th 2000 */
        for (scanx = 0; scanx < (renderer->pGFX_Base->Width + renderer->pGFX_Texture->Width); scanx++)
        {
		if((i_c = (int) (scanx + (c = Renderer_GetChange(scanx, base_data, renderer)) * (0.5 - renderer->eyeshift)) + (renderer->pGFX_Texture->Width * renderer->eyeshift * 2.0)) >= 0) {

			if (i_c < renderer->pGFX_Base->Width) {
		                renderer->right_change_map[i_c] = -c;
        		        z = i_c + renderer->right_change_map[i_c];
                		if ((z >= 0) && (z < renderer->pGFX_Base->Width))
                        		if (renderer->left_change_map[z] > c)
                                		renderer->left_change_map[z] = c;
			}
		}
        }

	/* checking for construction errors - `black holes' */
        if (renderer->left_change_map[0] > renderer->pGFX_Texture->Width)
                renderer->left_change_map[0] = renderer->pGFX_Texture->Width;
        for (z = 1; z < renderer->pGFX_Base->Width; z++)
                if (renderer->left_change_map[z] > renderer->pGFX_Texture->Width)
                        renderer->left_change_map[z] = renderer->left_change_map[z - 1];

	/* fixed these 'black holes' june 5th 2000 */
	/* checking right change map */
        if ((-renderer->right_change_map[renderer->pGFX_Base->Width - 1]) > renderer->pGFX_Texture->Width)
                renderer->right_change_map[renderer->pGFX_Base->Width - 1] = -(renderer->pGFX_Texture->Width);
        for (z = renderer->pGFX_Base->Width - 2; z >= 0; z--)
                if (-(renderer->right_change_map[z]) > renderer->pGFX_Texture->Width)
                        renderer->right_change_map[z] = renderer->right_change_map[z + 1];

	return 0;
}


/* This is float because of high precision effects such as zoom'n'anti-aliasing */
/* In a simple reference implementation an integer value should be suffisant */
float Renderer_GetChange(int x, int *base_data, struct RENDERER_DATA *renderer)
{
	float s, b, d;
	/* extracting out of 24 bit RGB data the brightness/intensitiy indicating information */
	/* accuracy range was extended from 0-255 up to 0-765 */
	if(renderer->linear) {
		s = (1.0 - ((base_data[x] & 255) + ((base_data[x] / 256) & 255) + ((base_data[x] / 65536) & 255)) / 766.0 * renderer->front);
		b = (float) renderer->pGFX_Texture->Width * (1.0 + renderer->distance) / (1.0 + (renderer->distance / s));
	} else {
		s = ((base_data[x] & 255) + ((base_data[x] / 256) & 255) + ((base_data[x] / 65536) & 255)) / 765.0;
		d = renderer->realdist - (float) renderer->max_change_tex_width * s;
		b = 1.0 / sqrt(renderer->internal_a + 0.25 / (d * d));
        }
	return b;
}


/* reference renderer without any special effects */
int Renderer_NormalScan(int *stereo_data, int *texture_data, struct RENDERER_DATA *renderer)
{
	float z;
	int scanx;

	/* insert the texture at x */
	memcpy(stereo_data + renderer->startx, texture_data, renderer->pGFX_Texture->Width * sizeof(int));

	/* we've to scan into two directions, beginning @ startx */
	for (scanx = renderer->startx; scanx < (renderer->startx + renderer->pGFX_Texture->Width); scanx++)
	{
		z = scanx + renderer->right_change_map[scanx];
		if (((int) z >= renderer->startx) && ((int) z < renderer->pGFX_Base->Width))
			stereo_data[scanx] = stereo_data[(int) z];
		/* no critical code, nothing to correct */
	}
	for (scanx = renderer->startx + renderer->pGFX_Texture->Width; scanx < renderer->pGFX_Base->Width; scanx++)
	{
		z = scanx + renderer->right_change_map[scanx];
		if( ((int) z < renderer->pGFX_Base->Width) && ((int) z >= 0))
			stereo_data[scanx] = stereo_data[(int) z];
	}
	for (scanx = renderer->startx - 1; scanx >= 0; scanx--)
	{
		z = scanx + renderer->left_change_map[scanx];
		/* 'cause of problems without the round function - internal round, it's a cut - it is NOT symmetric */
		if (z != ((int) z))
			z++;
		if (((int) z >= 0) && ((int) z < renderer->pGFX_Base->Width))
			stereo_data[scanx] = stereo_data[(int) z];
	}
	return 0;
}


/* First and most effective of all special: anti-aliasing */
int Renderer_AAScan(int *stereo_data, int *texture_data, struct RENDERER_DATA *renderer)
{
	int scanx,a;
	float z;
	int r, g, b;

        /* insert the texture without any special fx */
	for (scanx = 0; scanx < renderer->pGFX_Texture->Width; scanx++)
		for (a = 0; a < renderer->nfactor; a++)
			renderer->scanline[((scanx + renderer->startx) * renderer->nfactor) + a] = texture_data[scanx];

	/* scanning... */
	for (scanx = renderer->startx; scanx < (renderer->startx + renderer->pGFX_Texture->Width); scanx++)
	{
		z = scanx + renderer->right_change_map[scanx];
		if ((z >= renderer->startx) && (z < renderer->pGFX_Base->Width))
			for (a = 0; a < renderer->nfactor; a++)
				renderer->scanline[(scanx * renderer->nfactor) + a] = renderer->scanline[((int) (z * renderer->nfactor) + a)];
		else if (z > (renderer->startx - 1) && (z < renderer->startx))
        		for (a = (-z + (float) renderer->startx) * renderer->nfactor; a < renderer->nfactor; a++)
				/* fix in the following line: '+ renderer->nfactor' should correct the critical code which wasn't recognizable here */
				renderer->scanline[(scanx * renderer->nfactor) + a] = renderer->scanline[((int) (z * renderer->nfactor) + renderer->nfactor)];
	}
	for (scanx = renderer->startx + renderer->pGFX_Texture->Width; scanx < renderer->pGFX_Base->Width; scanx++)
	{
		z = scanx + renderer->right_change_map[scanx];
		if( (z < renderer->pGFX_Base->Width) && (z >= 0))
			for (a = 0; a < renderer->nfactor; a++)
				renderer->scanline[(scanx * renderer->nfactor) + a] = renderer->scanline[((int) (z * renderer->nfactor) + a)];
	}
	for (scanx = renderer->startx - 1; scanx >= 0; scanx--)
	{
		z = scanx + renderer->left_change_map[scanx];
		if((z < renderer->pGFX_Base->Width) && (z >= 0))
		{
			z *= renderer->nfactor;
			if (z != ((int) z))
				z++;
			for (a = 0; a < renderer->nfactor; a++)
				renderer->scanline[(scanx * renderer->nfactor) + a] = renderer->scanline[((int) z + a)];
		}
	}

	/* flush mem back into stereo_data, fixed */
	for (scanx = 0; scanx < renderer->pGFX_Base->Width; scanx++)
	{
		stereo_data[scanx] = r = g = b = 0;
		for (a = 0; a < renderer->naa; a++)
		{
			r +=  renderer->scanline[scanx * renderer->naa + a]        & 255;
			g += (renderer->scanline[scanx * renderer->naa + a] >>  8) & 255;
			b += (renderer->scanline[scanx * renderer->naa + a] >> 16) & 255;
		}
		stereo_data[scanx] +=  (r / renderer->naa) & 255;
		stereo_data[scanx] += ((g / renderer->naa) & 255) << 8;
		stereo_data[scanx] += ((b / renderer->naa) & 255) << 16;
	}

	return 0;
}


/* Second effect: zoom */
int Renderer_ZoomScan(int *stereo_data, int *texture_data, struct RENDERER_DATA *renderer)
{
	int scanx, a;
	float z;

        /* insert the texture without any special fx */
	for (scanx = 0; scanx < renderer->pGFX_Texture->Width; scanx++)
		for (a = 0; a < renderer->nfactor; a++)
			stereo_data[((scanx + renderer->startx) * renderer->nfactor) + a] = texture_data[scanx];

        /* already scanning, nothing to prepare */
	for (scanx = renderer->startx; scanx < (renderer->startx + renderer->pGFX_Texture->Width); scanx++)
	{
		z = scanx + renderer->right_change_map[scanx];
		if ((z >= renderer->startx) && (z < renderer->pGFX_Base->Width))
			for (a = 0; a < renderer->nfactor; a++)
				stereo_data[(scanx * renderer->nfactor) + a] = stereo_data[(int) (z * renderer->nfactor) + a];
		else if ((z > (renderer->startx - 1)) && (z < renderer->startx))
        		for (a = (-z + (float) renderer->startx) * renderer->nfactor; a < renderer->nfactor; a++)
				/* fix in the following line: '+ renderer->nfactor' should correct the critical code */
				stereo_data[(scanx * renderer->nfactor) + a] = stereo_data[((int) (z * renderer->nfactor) + renderer->nfactor)];
	}
	for (scanx = renderer->startx + renderer->pGFX_Texture->Width; scanx < renderer->pGFX_Base->Width; scanx++)
	{
		z = scanx + renderer->right_change_map[scanx];
		if((z < renderer->pGFX_Base->Width) && (z >= 0))
			for (a = 0; a < renderer->nfactor; a++)
				stereo_data[(scanx * renderer->nfactor) + a] = stereo_data[(int) (z * renderer->nfactor) + a];
	}

	for (scanx = renderer->startx - 1; scanx >= 0; scanx--)
	{
		z = scanx + renderer->left_change_map[scanx];
		if((z < renderer->pGFX_Base->Width) && (z >= 0))
		{
			z *= renderer->nfactor;
			if (z != ((int) z))
				z++;
			for (a = 0; a < renderer->nfactor; a++)
				stereo_data[(scanx * renderer->nfactor) + a] = stereo_data[(int) z + a];  /* attention : z is was already multiplied above!!! */
		}
	}

	/* fill the comple row down with the same values to construct square pixels */
	for (a = 1; a < renderer->nfactor; a++)
		memcpy(stereo_data + renderer->pGFX_Stereo->Width * a, stereo_data, renderer->pGFX_Stereo->Width * sizeof(int));

	return 0;
}


/* ZOOM'N'AA */
/* Final effects: fx pair in cooperation */
int Renderer_ZoomAAScan(int *stereo_data, int *texture_data, struct RENDERER_DATA *renderer)
{
	int scanx, a;
	float z;
	int r, g, b;

        /* insert the texture without any special fx */
	/* the same as anti-aliasing implementation above */
	for (scanx = 0; scanx < renderer->pGFX_Texture->Width; scanx++)
		for (a = 0; a < renderer->nfactor; a++)
			renderer->scanline[((scanx + renderer->startx) * renderer->nfactor) + a] = texture_data[scanx];

	/* scanning */
	for (scanx = renderer->startx; scanx < (renderer->startx + renderer->pGFX_Texture->Width); scanx++)
	{
		z = scanx + renderer->right_change_map[scanx];
		if ((z >= renderer->startx) && (z < renderer->pGFX_Base->Width))
			for (a = 0; a < renderer->nfactor; a++)
				renderer->scanline[(scanx * renderer->nfactor) + a] = renderer->scanline[(int) (z * renderer->nfactor) + a];
		else if (z > (renderer->startx - 1) && (z < renderer->startx))
        		for (a = (-z + (float) renderer->startx) * renderer->nfactor; a < renderer->nfactor; a++)
				/* fix in the following line: '+ renderer->nfactor' should correct the critical code */
				renderer->scanline[(scanx * renderer->nfactor) + a] = renderer->scanline[((int) (z * renderer->nfactor) + renderer->nfactor)];
	}
	for (scanx = renderer->startx + renderer->pGFX_Texture->Width; scanx < renderer->pGFX_Base->Width; scanx++)
	{
		z = scanx + renderer->right_change_map[scanx];
		if ((z >= 0) && (z < renderer->pGFX_Base->Width))
			for (a = 0; a < renderer->nfactor; a++)
				renderer->scanline[(scanx * renderer->nfactor) + a] = renderer->scanline[(int) (z * renderer->nfactor) + a];
	}
	for (scanx = renderer->startx - 1; scanx >= 0; scanx--)
	{
		z = scanx + renderer->left_change_map[scanx];
		if ((z >= 0) && (z < renderer->pGFX_Base->Width))
		{
			z *= renderer->nfactor;
			if (z != ((int) z))
				z++;
			for (a = 0; a < renderer->nfactor; a++)
				renderer->scanline[(scanx * renderer->nfactor) + a] = renderer->scanline[(int) z + a];  /* z was already multiplied above; */
		}
	}

	/* flush it down, fixed */
	for (scanx = 0; scanx < renderer->pGFX_Stereo->Width; scanx++)
	{
		stereo_data[scanx] = r = g = b = 0;
		for (a = 0; a < renderer->naa; a++)
		{
			r +=  renderer->scanline[scanx * renderer->naa + a]        & 255;
			g += (renderer->scanline[scanx * renderer->naa + a] >>  8) & 255;
			b += (renderer->scanline[scanx * renderer->naa + a] >> 16) & 255;
		}
		stereo_data[scanx] +=  (r / renderer->naa) & 255;
		stereo_data[scanx] += ((g / renderer->naa) & 255) << 8;
		stereo_data[scanx] += ((b / renderer->naa) & 255) << 16;
	}


	/* fill it down to produce square pixels */
	for (a = 1; a < renderer->nzoom; a++)
		memcpy(stereo_data + renderer->pGFX_Stereo->Width * a, stereo_data, renderer->pGFX_Stereo->Width * sizeof(int));
	return 0;
}
