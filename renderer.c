/* Stereograph 0.29a, 14/07/2000;
 * renderer, stereographer's engine;
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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "renderer.h"


/* here we go... */

struct GFX_DATA Base, Texture, Stereo;
struct GFX_DATA **T_Base = NULL, **T_Texture = NULL, **T_Stereo = NULL;
struct RENDERER_DATA Renderer;
struct PARAMS Param;


/* 1st of all: send all releavant data (pointers) to foreign functions */
int Get_GFX_Pointers(struct PARAMS **pParam, struct GFX_DATA **pBase, struct GFX_DATA **pTexture, struct GFX_DATA **pStereo) {
	(*pParam) = &Param;
	(*pBase) = &Base;
	(*pTexture) = &Texture;
	(*pStereo) = &Stereo;
	return 0;
}


/* 2nd (transparency only): allocate T_Bases and T_Textures */
int Get_T_GFX_Pointers(struct GFX_DATA ****T_pBase, struct GFX_DATA ****T_pTexture) {
	(*T_pBase) = &T_Base;
	(*T_pTexture) = &T_Texture;
	return 0;
}


/* render a mere line */
int ProcessLine(int base_line) {
	if(Param.T_layers <= 0)
		return RenderLine(Base.Data + Base.Width * base_line, Texture.Data + Texture.Width * ((base_line + Renderer.starty) % Texture.Height), Stereo.Data + Stereo.Width*base_line*Renderer.nzoom, base_line);
	else {
		int z, a, x, c_r = 0, c_g = 0, c_b = 0;
		a = 0;
		for(z = 0; (z < Renderer.nlayers) && !a; z++)
			a = RenderLine(T_Base[z]->Data + T_Base[z]->Width * base_line, T_Texture[z]->Data + T_Texture[z]->Width * ((base_line + Renderer.starty) % T_Texture[z]->Height), Renderer.tscanline[z], base_line);

		for(x = 0; x < (Stereo.Width * Renderer.nzoom); x++) {
			c_r = c_g = c_b = 0;
			for(z = 0; z < Renderer.nlayers; z++) {
				c_r +=  Renderer.tscanline[z][x]      & 255;
				c_g += (Renderer.tscanline[z][x] / 256) & 255;
				c_b += (Renderer.tscanline[z][x] / 65536) & 255;
			}
			Stereo.Data[Stereo.Width*base_line*Renderer.nzoom + x] +=  (c_r / Renderer.nlayers) & 255;
			Stereo.Data[Stereo.Width*base_line*Renderer.nzoom + x] += ((c_g / Renderer.nlayers) & 255) * 367;
			Stereo.Data[Stereo.Width*base_line*Renderer.nzoom + x] += ((c_b / Renderer.nlayers) & 255) * 65536;
		}
		return a;
	}
}


/* destroy memory reservations, internals; NOTE: Stereo also belongs to the renderer! */
void Clear_Renderer(void)
{
	int z;
	free(Renderer.right_change_map);
	free(Renderer.left_change_map);
	for(z = 0; z < Param.T_layers; z++)
		free(Renderer.tscanline[z]);
	free(Renderer.tscanline);
	free(Renderer.scanline);
	free(Stereo.Data);
}


/* initialize internal rendering engine data */
int Initialize_Renderer(void)
{
	if(!Param.T_layers) {
		Renderer.nlayers = 0;
		Renderer.startx = Param.Startx;
		Renderer.starty = -Param.Starty;
		while(Renderer.starty < 0)
			Renderer.starty += Texture.Height;
		Renderer.max_change_tex_width = (int)((Texture.Width - 1) * Param.Front);
		Renderer.right_change_map = (float*)malloc(sizeof(float) * (Base.Width + 1));
		Renderer.left_change_map = (float*)malloc(sizeof(float) * (Base.Width + 1));
		Renderer.naa = Param.AA + 0;
		if(Renderer.naa == 0) Renderer.naa = 1;
		Renderer.nzoom = Param.Zoom + 0;
		if(Renderer.nzoom == 0) Renderer.nzoom = 1;
		Renderer.realdist = Texture.Width * (Param.Distance + 1);
		Renderer.internal_a = (1.0 / (Texture.Width * Texture.Width)) - (0.25 / (Renderer.realdist * Renderer.realdist));
		Renderer.eyeshift = Param.Eyeshift*0.5;
		if(
			((Param.Distance + 1) > 1.0) && ((Param.Distance + 1) <= 9.0) &&
			(Renderer.eyeshift >= -0.5) && (Renderer.eyeshift <= 0.5) &&
			(Renderer.naa > 0) && (Renderer.naa <= 32) &&
			(Renderer.nzoom > 0) && (Renderer.nzoom <= 32) &&
			(Renderer.startx >= 0) && (Renderer.startx < (Base.Width - Texture.Width)) &&
			(Param.Starty >= 0) &&
			(Param.Front > 0.0) && (Param.Front <= 1.0)
		) {
			Renderer.nfactor = Renderer.naa * Renderer.nzoom;
			if(Param.AA - 1)
				Renderer.scanline = (int*)malloc(sizeof(int) * (Base.Width * Renderer.nfactor));
			else
				Renderer.scanline = NULL;

			Stereo.Width = Base.Width * Renderer.nzoom;
			Stereo.Height = Base.Height * Renderer.nzoom;
			Stereo.Data = (int*)malloc(sizeof(int) * Stereo.Width * Stereo.Height);

	        	if(Renderer.right_change_map && Renderer.left_change_map && Stereo.Data && Base.Data && Texture.Data) {
				if(Base.Width >= Texture.Width)
					return 0;
				else
					return -2;
			} else
				return -3;
		} else {
			return -1;
		}       	
	} else {
		Base.Width = T_Base[0]->Width;
		Base.Height = T_Base[0]->Height;
		Texture.Width = T_Texture[0]->Width;
		Texture.Height = T_Texture[0]->Height;

		Renderer.nlayers = Param.T_layers + 1;
		Renderer.startx = Param.Startx;
		Renderer.max_change_tex_width = (int)((T_Texture[0]->Width - 1) * Param.Front);
		Renderer.right_change_map = (float*)malloc(sizeof(float) * (T_Base[0]->Width + 1));
		Renderer.left_change_map = (float*)malloc(sizeof(float) * (T_Base[0]->Width + 1));
		Renderer.naa = Param.AA + 0;
		if(Renderer.naa == 0) Renderer.naa = 1;
		Renderer.nzoom = Param.Zoom + 0;
		if(Renderer.nzoom == 0) Renderer.nzoom = 1;
		Renderer.realdist = T_Texture[0]->Width * (Param.Distance + 1);
		Renderer.internal_a = (1.0 / (Texture.Width * T_Texture[0]->Width)) - (0.25 / (Renderer.realdist * Renderer.realdist));
		Renderer.eyeshift = Param.Eyeshift*0.5;
		if(
			((Param.Distance + 1) > 1.0) && ((Param.Distance + 1) <= 9.0) &&
			(Renderer.eyeshift >= -0.5) && (Renderer.eyeshift <= 0.5) &&
			(Renderer.naa > 0) && (Renderer.naa <= 32) &&
			(Renderer.nzoom > 0) && (Renderer.nzoom <= 32) &&
			(Renderer.startx >= 0) && (Renderer.startx < (T_Base[0]->Width - T_Texture[0]->Width)) &&
			(Param.Starty >= 0) &&
			(Param.Front > 0.0) && (Param.Front <= 1.0)
		) {
			int a = 0, z;
			Renderer.nfactor = Renderer.naa * Renderer.nzoom;
			if(Param.AA - 1)
				Renderer.scanline = (int*)malloc(sizeof(int) * (T_Base[0]->Width * Renderer.nfactor));
			else
				Renderer.scanline = NULL;

			Stereo.Width = T_Base[0]->Width * Renderer.nzoom;
			Stereo.Height = T_Base[0]->Height * Renderer.nzoom;
			Stereo.Data = (int*)malloc(sizeof(int) * Stereo.Width * Stereo.Height);

			Renderer.tscanline = (int**)malloc(sizeof(int*) * Renderer.nlayers);
			if(Renderer.tscanline) {
				for(z = 0; (z < Renderer.nlayers) && !a; z++)
					Renderer.tscanline[z] = (int*)malloc(sizeof(int) * Stereo.Width * Renderer.nzoom);
			} else
				return -3;

			for(z = 0; (z < Renderer.nlayers) && !a; z++) {
		        	if(Renderer.right_change_map && Renderer.left_change_map && Stereo.Data && T_Base[z]->Data && T_Texture[z]->Data && Renderer.tscanline[z]) {
					if((T_Base[0]->Width != T_Base[z]->Width) || (T_Base[0]->Height != T_Base[z]->Height) || (T_Texture[0]->Width != T_Texture[z]->Width))
						 a -= 4;
				} else
					return -3;
			}
			return a;
		} else {
			return -1;
		}       	

	}
}


/* Render a mere line of data, may be completely independent of any gfx_data structure */
/* reorganized aar implementation during versions 0.24-0.25, 3rd of june 2000 */
/* added additional InitMap entry 'cause of a buggy implementation 5th of june 2000 */
int RenderLine(int *base_data, int *texture_data, int *stereo_data, int base_line)
{
	InitMap(Renderer.left_change_map, 0, Base.Width, (float)(Texture.Width + 1));
	InitMap(Renderer.right_change_map, 0, Base.Width, (float)-(Texture.Width + 1));
	
	FillMapsLeftToRight(base_data);

	if(Param.AAr) {
		/* tracing alternative startx */
		/* reimplemented in 0.25 */
		if(base_line & 1)
			while(((Renderer.startx + (int)Renderer.left_change_map[Renderer.startx]) < (Base.Width - Texture.Width)) && ((Renderer.startx + (int)Renderer.left_change_map[Renderer.startx]) > Renderer.startx))
				Renderer.startx += (int)Renderer.left_change_map[Renderer.startx];
		else
			while(((Renderer.startx + (int)Renderer.right_change_map[Renderer.startx]) >= 0) && ((Renderer.startx + (int)Renderer.right_change_map[Renderer.startx]) < Renderer.startx))
				Renderer.startx += (int)Renderer.right_change_map[Renderer.startx];
	}
	if(Param.AA - 1)
	{
   		if(Param.Zoom - 1)
			return ZoomAAScan(stereo_data, texture_data);
		else
			return AAScan(stereo_data, texture_data);;
	} else
		/* small fix here (0.25) */
		if(Param.Zoom - 1)
			return ZoomScan(stereo_data, texture_data);
		else
			return NormalScan(stereo_data, texture_data);
}


/* intialize changemap internals */
/* changed formal expression june 5th 2000 */
int InitMap(float *change_map, int a, int b, float v)
{
	while(a < b)
		change_map[a++] = v;
	return 0;
}


/* set changevalues */
int FillMapsLeftToRight(int *base_data)
{
        int z, scanx, i_c;
	float c;

	/* advanced full centered perspective; reimplemented february 4th 2000 */
	/* added eyeshift february 11th 2000 */
	/* fixed conditions here june 5th 2000 */
        for (scanx = 0; scanx < (Base.Width + Texture.Width); scanx++)
        {
		if((i_c = (int) (scanx + (c = GetChange(scanx, base_data))*(0.5 - Renderer.eyeshift)) + (Texture.Width*Renderer.eyeshift*2.0)) >= 0) {
			;
			if (i_c < Base.Width) {
		                Renderer.right_change_map[i_c] = -c;
        		        z = i_c + Renderer.right_change_map[i_c];
                		if ((z >= 0) && (z < Base.Width))
                        		if (Renderer.left_change_map[z] > c)
                                		Renderer.left_change_map[z] = c;
			}
		}
        }

	/* checking for construction errors - `black holes' */
        if (Renderer.left_change_map[0] > Texture.Width)
                Renderer.left_change_map[0] = Texture.Width;
        for (z = 1; z < Base.Width; z++)
                if (Renderer.left_change_map[z] > Texture.Width)
                        Renderer.left_change_map[z] = Renderer.left_change_map[z - 1];

	/* fixed these 'black holes' june 5th 2000 */
	/* checking right change map */
        if ((-Renderer.right_change_map[Base.Width - 1]) > Texture.Width)
                Renderer.right_change_map[Base.Width - 1] = -Texture.Width;
        for (z = Base.Width - 2; z >= 0; z--)
                if ((-Renderer.right_change_map[z]) > Texture.Width)
                        Renderer.right_change_map[z] = Renderer.right_change_map[z + 1];

	return 0;
}


/* This is float because of high precision effects such as zoom'n'anti-aliasing */
/* In a simple reference implementation an integer value should be suffisant */
float GetChange(int x, int *base_data)
{
	float s, b, d;
	/* extracting out of 24 bit RGB data the brightness/intensitiy indicating information */
	/* value is extended from 0-255 up to 0-765 */
	if(Param.Linear) {
		s = (1.0 - ((base_data[x] & 255) + ((base_data[x] / 256) & 255) + ((base_data[x] / 65536) & 255)) / 765.0);
		b = ((Param.Distance + s)*(Param.Distance + 2.0) / (Param.Distance + s + 1.0) / (Param.Distance + 1.0)) * Renderer.max_change_tex_width;
	} else {
		s = ((base_data[x] & 255) + ((base_data[x] / 256) & 255) + ((base_data[x] / 65536) & 255)) / 765.0;
		d = Renderer.realdist - (float) Renderer.max_change_tex_width * s;
		b = 1.0 / sqrt(Renderer.internal_a + 0.25 / (d * d));
        }
	return b;
}


/* reference renderer without any special effects */
int NormalScan(int *stereo_data, int *texture_data)
{
	float z;
	int scanx;

	/* insert the texture at x */
	memcpy(stereo_data + Renderer.startx, texture_data, Texture.Width * sizeof(int));

	/* we've to scan into two directions, beginning @ startx */
	for (scanx = Renderer.startx; scanx < (Renderer.startx + Texture.Width); scanx++)
	{
		z = scanx + Renderer.right_change_map[scanx];
		if (((int) z >= Renderer.startx) && ((int) z < Base.Width))
			stereo_data[scanx] = stereo_data[(int) z];
		/* no critical code, nothing to correct */
	}
	for (scanx = Renderer.startx + Texture.Width; scanx < Base.Width; scanx++)
	{
		z = scanx + Renderer.right_change_map[scanx];
		if( ((int) z < Base.Width) && ((int) z >= 0))
			stereo_data[scanx] = stereo_data[(int) z];
	}
	for (scanx = Renderer.startx - 1; scanx >= 0; scanx--)
	{
		z = scanx + Renderer.left_change_map[scanx];
		/* 'cause of problems without the round function - internal round, it's a cut - it is NOT symmetric */
		if (z != ((int) z))
			z++;
		if (((int) z >= 0) && ((int) z < Base.Width))
			stereo_data[scanx] = stereo_data[(int) z];
	}
	return 0;
}


/* First and most effective of all special: anti-aliasing */
int AAScan(int *stereo_data, int *texture_data)
{
	int scanx,a;
	float z;
	int r, g, b;

        /* insert the texture without any special fx */
	for (scanx = 0; scanx < Texture.Width; scanx++)
		for (a = 0; a < Renderer.nfactor; a++)
			Renderer.scanline[((scanx + Renderer.startx) * Renderer.nfactor) + a] = texture_data[scanx];

	/* scanning... */
	for (scanx = Renderer.startx; scanx < (Renderer.startx + Texture.Width); scanx++)
	{
		z = scanx + Renderer.right_change_map[scanx];
		if ((z >= Renderer.startx) && (z < Base.Width))
			for (a = 0; a < Renderer.nfactor; a++)
				Renderer.scanline[(scanx * Renderer.nfactor) + a] = Renderer.scanline[((int) (z * Renderer.nfactor) + a)];
		else if (z > (Renderer.startx - 1) && (z < Renderer.startx))
        		for (a = (-z + (float) Renderer.startx) * Renderer.nfactor; a < Renderer.nfactor; a++)
				/* fix in the following line: '+ Renderer.nfactor' should correct the critical code which wasn't recognizable here */
				Renderer.scanline[(scanx * Renderer.nfactor) + a] = Renderer.scanline[((int) (z * Renderer.nfactor) + Renderer.nfactor)];
	}
	for (scanx = Renderer.startx + Texture.Width; scanx < Base.Width; scanx++)
	{
		z = scanx + Renderer.right_change_map[scanx];
		if( (z < Base.Width) && (z >= 0))
			for (a = 0; a < Renderer.nfactor; a++)
				Renderer.scanline[(scanx * Renderer.nfactor) + a] = Renderer.scanline[((int) (z * Renderer.nfactor) + a)];
	}
	for (scanx = Renderer.startx - 1; scanx >= 0; scanx--)
	{
		z = scanx + Renderer.left_change_map[scanx];
		if((z < Base.Width) && (z >= 0))
		{
			z *= Renderer.nfactor;
			if (z != ((int) z))
				z++;
			for (a = 0; a < Renderer.nfactor; a++)
				Renderer.scanline[(scanx * Renderer.nfactor) + a] = Renderer.scanline[((int) z + a)];
		}
	}

	/* flush mem back into stereo_data, fixed */
	for (scanx = 0; scanx < Base.Width; scanx++)
	{
		stereo_data[scanx] = r = g = b = 0;
		for (a = 0; a < Renderer.naa; a++)
		{
			r +=  Renderer.scanline[scanx * Renderer.naa + a]          & 255;
			g += (Renderer.scanline[scanx * Renderer.naa + a] /   256) & 255;
			b += (Renderer.scanline[scanx * Renderer.naa + a] / 65536) & 255;
		}
		stereo_data[scanx] +=  (r / Renderer.naa) & 255;
		stereo_data[scanx] += ((g / Renderer.naa) & 255) * 256;
		stereo_data[scanx] += ((b / Renderer.naa) & 255) * 65536;
	}
	return 0;
}


/* Second effect: zoom */
int ZoomScan(int *stereo_data, int *texture_data)
{
	int scanx, a;
	float z;

        /* insert the texture without any special fx */
	for (scanx = 0; scanx < Texture.Width; scanx++)
		for (a = 0; a < Renderer.nfactor; a++)
			stereo_data[((scanx + Renderer.startx) * Renderer.nfactor) + a] = texture_data[scanx];

        /* already scanning, nothing to prepare */
	for (scanx = Renderer.startx; scanx < (Renderer.startx + Texture.Width); scanx++)
	{
		z = scanx + Renderer.right_change_map[scanx];
		if ((z >= Renderer.startx) && (z < Base.Width))
			for (a = 0; a < Renderer.nfactor; a++)
				stereo_data[(scanx * Renderer.nfactor) + a] = stereo_data[(int) (z * Renderer.nfactor) + a];
		else if ((z > (Renderer.startx - 1)) && (z < Renderer.startx))
        		for (a = (-z + (float) Renderer.startx) * Renderer.nfactor; a < Renderer.nfactor; a++)
				/* fix in the following line: '+ Renderer.nfactor' should correct the critical code */
				stereo_data[(scanx * Renderer.nfactor) + a] = stereo_data[((int) (z * Renderer.nfactor) + Renderer.nfactor)];
	}
	for (scanx = Renderer.startx + Texture.Width; scanx < Base.Width; scanx++)
	{
		z = scanx + Renderer.right_change_map[scanx];
		if((z < Base.Width) && (z >= 0))
			for (a = 0; a < Renderer.nfactor; a++)
				stereo_data[(scanx * Renderer.nfactor) + a] = stereo_data[(int) (z * Renderer.nfactor) + a];
	}

	for (scanx = Renderer.startx - 1; scanx >= 0; scanx--)
	{
		z = scanx + Renderer.left_change_map[scanx];
		if((z < Base.Width) && (z >= 0))
		{
			z *= Renderer.nfactor;
			if (z != ((int) z))
				z++;
			for (a = 0; a < Renderer.nfactor; a++)
				stereo_data[(scanx * Renderer.nfactor) + a] = stereo_data[(int) z + a];  /* attention : z is was already multiplied above!!! */
		}
	}

	/* fill the comple row down with the same values to construct square pixels */
	for (a = 1; a < Renderer.nfactor; a++)
		memcpy(stereo_data + Stereo.Width*a, stereo_data, Stereo.Width * sizeof(int));
	return 0;
}


/* ZOOM'N'AA */
/* Final effects: fx pair in cooperation */
int ZoomAAScan(int *stereo_data, int *texture_data)
{
	int scanx, a;
	float z;
	int r, g, b;

        /* insert the texture without any special fx */
	/* the same as anti-aliasing implementation above */
	for (scanx = 0; scanx < Texture.Width; scanx++)
		for (a = 0; a < Renderer.nfactor; a++)
			Renderer.scanline[((scanx + Renderer.startx) * Renderer.nfactor) + a] = texture_data[scanx];

	/* scanning */
	for (scanx = Renderer.startx; scanx < (Renderer.startx + Texture.Width); scanx++)
	{
		z = scanx + Renderer.right_change_map[scanx];
		if ((z >= Renderer.startx) && (z < Base.Width))
			for (a = 0; a < Renderer.nfactor; a++)
				Renderer.scanline[(scanx * Renderer.nfactor) + a] = Renderer.scanline[(int) (z * Renderer.nfactor) + a];
		else if (z > (Renderer.startx - 1) && (z < Renderer.startx))
        		for (a = (-z + (float) Renderer.startx) * Renderer.nfactor; a < Renderer.nfactor; a++)
				/* fix in the following line: '+ Renderer.nfactor' should correct the critical code */
				Renderer.scanline[(scanx * Renderer.nfactor) + a] = Renderer.scanline[((int) (z * Renderer.nfactor) + Renderer.nfactor)];
	}
	for (scanx = Renderer.startx + Texture.Width; scanx < Base.Width; scanx++)
	{
		z = scanx + Renderer.right_change_map[scanx];
		if ((z >= 0) && (z < Base.Width))
			for (a = 0; a < Renderer.nfactor; a++)
				Renderer.scanline[(scanx * Renderer.nfactor) + a] = Renderer.scanline[(int) (z * Renderer.nfactor) + a];
	}
	for (scanx = Renderer.startx - 1; scanx >= 0; scanx--)
	{
		z = scanx + Renderer.left_change_map[scanx];
		if ((z >= 0) && (z < Base.Width))
		{
			z *= Renderer.nfactor;
			if (z != ((int) z))
				z++;
			for (a = 0; a < Renderer.nfactor; a++)
				Renderer.scanline[(scanx * Renderer.nfactor) + a] = Renderer.scanline[(int) z + a];  /* z was already multiplied above; */
		}
	}

	/* flush it down, fixed */
	for (scanx = 0; scanx < Stereo.Width; scanx++)
	{
		stereo_data[scanx] = r = g = b = 0;
		for (a = 0; a < Renderer.naa; a++)
		{
			r +=  Renderer.scanline[scanx * Renderer.naa + a]          & 255;
			g += (Renderer.scanline[scanx * Renderer.naa + a] /   256) & 255;
			b += (Renderer.scanline[scanx * Renderer.naa + a] / 65536) & 255;
		}
		stereo_data[scanx] +=  (r / Renderer.naa) & 255;
		stereo_data[scanx] += ((g / Renderer.naa) & 255) * 256;
		stereo_data[scanx] += ((b / Renderer.naa) & 255) * 65536;
	}


	/* fill it down to produce square pixels */
	for (a = 1; a < Renderer.nzoom; a++)
		memcpy(stereo_data + Stereo.Width*a, stereo_data, Stereo.Width*sizeof(int));
	return 0;
}
