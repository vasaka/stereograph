/* Stereograph 0.30a, 26/12/2000;
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

#define STEREOGRAPH_VERSION "0.30a"

#include "renderer.h"
#include "gfxio.h"
#include "stereograph.h"
#include "globals.h"

char *base_file_name = NULL;
char *texture_file_name = NULL;
char *stereo_file_name = NULL;

char **T_base_file_name = NULL;
char **T_texture_file_name = NULL;

int output_format = 0;

/* interface to the renderer */
struct PARAMS *pParam;
struct GFX_DATA *pBase, *pTexture, *pStereo;

struct GFX_DATA ***T_pBase, ***T_pTexture;

int texture_width_to_use = 0;
int random_texture_type = TEX_GRAYSCALE;


void print_info(void) {
	printf("SYNOPSIS\n  stereograph [OPTIONS] -b [base] [-t [texture]] [-w n]\n              [-o [output]] [-f png/ppm/tga] [-l none/back/top]\n\n");
	printf("OPTIONS\n  -a anti-aliasing     -z zoom               (quality)\n  -d distance          -p front factor                    \n                       -e eye shift          (perspective)\n  -x texture insert x  -y texture insert y   (layout)\n  -w texture width to use\n  -M/-G/-C generate a monochrome, grayscale or colored\n  -S generates an experimental random texture\n           random texture (no transparency)\n  -A add a pair of triangles                 (aid)\n  -R this flag enables the anti-artefact feature\n  -L disables the linear rendering algorithm\n  -I invert the base\n  -l sets the level adjust mode for transparent rendering\n  -f defines the output format\n  -v you should know this flag :)\n  -V display version\n\n");
}



int main (int argc, char **argv) {
	int last_error = 0;

	Get_GFX_Pointers(&pParam, &pBase, &pTexture, &pStereo);

	pParam->Linear = 1;
	pParam->T_layers = 0;
	pParam->Zoom = 1;
	pParam->AA = 4;
	pParam->AAr = 0;
	pParam->Startx = 0;
	pParam->Starty = 0;
	pParam->Front = 0.4;
	pParam->Distance = 5.0;
	pParam->Eyeshift = 0.0;

	pParam->T_level = T_BACK_LEVEL;
	pParam->Invert = 0;
	pParam->Triangles = 0;

	pBase->Data = NULL;
	pTexture->Data = NULL;
	pStereo->Data = NULL;
	base_file_name = NULL;
	texture_file_name = NULL;
	stereo_file_name = NULL;

	verbose = 0;

	if(parse_args(argc, argv))
		return -1;

	if(!stereo_file_name && base_file_name && texture_file_name && verbose) {
		fprintf(stderr, "verbose disabled for standard output\n");
		verbose = 0;
	}

	if(verbose) printf("Stereograph for linux Version %s\n", STEREOGRAPH_VERSION);

	if(base_file_name && (texture_file_name || (texture_width_to_use > 0))) {
		if(pParam->T_layers <= 0) {
			/* solid (single) tracing */
			if(verbose) printf("loading base '%s'...", base_file_name);
			if(!Read_Gfx_File(base_file_name, pBase)) {
				if(!texture_file_name) {
					if(verbose) printf("generating random texture (%i*%i)...", texture_width_to_use, pBase->Height);
					if(texture_width_to_use < pBase->Width) {
						last_error = Generate_Random_Texture(pTexture, texture_width_to_use, pBase->Height, random_texture_type);
					} else {
						if(verbose) printf("FAILED\n");
						fprintf(stderr, "texture_width_to_use is too big. This may not be greater than the base width.\n");
						last_error = -2;
					}
				} else {
					if(verbose) printf("loading texture '%s'...", texture_file_name);
					last_error = Read_Gfx_File(texture_file_name, pTexture);
					if(!last_error && (texture_width_to_use > 0)) {
						if(pTexture->Width >= texture_width_to_use) {
							last_error = Resize_GFX(pTexture, texture_width_to_use);
						} else {
							fprintf(stderr, "parametre error: texture_width_to_use is too big;\nmust be less than the texture width.\n");
							last_error = -2;
						}
					}
				}
				if(!last_error && pParam->Invert) {
					if((last_error = Invert_GFX(pBase))) {
						if(verbose) printf("FAILED\n");
						fprintf(stderr, "error inverting base;\n");
					}
				}
				if(!last_error) {
					/* setting up renderer; */
					if(verbose) printf("initializing renderer...");
					if(!(last_error = Initialize_Renderer())) {
						int y, s;
						if(verbose) printf("success;\n");
						if(verbose) printf("using following parametres (zadpexy): %i %i %f %f %f %i %i;\n", pParam->Zoom, pParam->AA, pParam->Distance, pParam->Front, pParam->Eyeshift, pParam->Startx, pParam->Starty);
						if(verbose && pParam->AAr)
							printf("anti-artefacts feature enabled;\n");
						if(verbose && !pParam->Linear)
							printf("linear rendering disabled;\n");
						if(verbose && pParam->Triangles)
							printf("triangle aid enabled, a pair will be added after rendering;\n");
						if(verbose) printf("processing...");
                                	
						for(y = 0, s = 0; (y < pBase->Height); y++) {
							ProcessLine(y);
							/* print simple status points (without refresh) */
							for( ; verbose && (s < (58 * y / pBase->Height)); s++)
								printf(".");
						}
						if(verbose) printf("completed;\n");

						if(pParam->Triangles) {
							if(verbose)
								printf("adding triangles to the final image...");
							Add_Triangles(pStereo->Width/2, 20*pParam->Zoom, pTexture->Width*pParam->Zoom, pStereo);
							if(verbose)
								printf("done;\n");
						}
	
						if(verbose) printf("writing stereogram to disk...");
						if(!Write_Gfx_File(stereo_file_name, output_format, pStereo) && verbose)
							printf("completed;\n");
					} else {
						if(verbose)
							printf("FAILED;\n");
						else    	
							fprintf(stderr, "initializing renderer...FAILED;\n");
						switch (last_error) {
							case -1 :
								fprintf(stderr, "illegal parametre;\n\n");
								print_info();
								break;
							case -2 :
								fprintf(stderr, "width of texture file greater than the base;\nthis state is not processable;\ntry the -w option to resize the texture.\n");
								break;
							case -3 :
								fprintf(stderr, "could not allocate memory.\n");
						}
						return -1;
					}
					Clear_Renderer();
				}
				free(pTexture->Data);
			}
			free(pBase->Data);
			/* end of single trace */
		} else {
			/* transparent trace */
			int z, a;
			
			Get_T_GFX_Pointers(&T_pBase, &T_pTexture);
			/* allocating multiple gfx data headers */
				(*T_pBase) = (struct GFX_DATA**) malloc(sizeof(struct GFX_DATA*) * (pParam->T_layers+1));
				(*T_pTexture) = (struct GFX_DATA**) malloc(sizeof(struct GFX_DATA*) * (pParam->T_layers+1));
				a = 2;
				if((*T_pBase) && (*T_pTexture)) {
					a = 0;
					for(z = 0; (z <= pParam->T_layers) && !a; z++) {
						*((*T_pBase)+z) = (struct GFX_DATA*) malloc(sizeof(struct GFX_DATA));
						(*((*T_pBase)+z))->Data = NULL;
						*((*T_pTexture)+z) = (struct GFX_DATA*) malloc(sizeof(struct GFX_DATA));
						(*((*T_pTexture)+z))->Data = NULL;
						if(!*((*T_pBase)+z) || !*((*T_pTexture)+z))
							a++;
					}
				}
				if(!a) {
					if(verbose) printf("rendering transparent layers enabled\nloading bases:\n");
					for(z = 0; (z <= pParam->T_layers) && !a; z++) {
						if(verbose) printf("%s ", T_base_file_name[z]);
						a = Read_Gfx_File(T_base_file_name[z], *((*T_pBase)+z));
					}
					if(!a) {
						if(verbose) printf("done;\nloading textures:\n");
						for(z = 0; (z <= pParam->T_layers) && !a; z++) {
							if(verbose) printf("%s ", T_texture_file_name[z]);
							a = Read_Gfx_File(T_texture_file_name[z], *((*T_pTexture)+z));
							if((texture_width_to_use > 0) && !a) {
								if((*((*T_pTexture)+z))->Width >= texture_width_to_use) {
									a = Resize_GFX((*((*T_pTexture)+z)), texture_width_to_use);
								} else {
									fprintf(stderr, "parametre error: texture_width_to_use is too big;\nmust be less than the texture width.\n");
									a = -2;
								}
							}
						}
						if(verbose && !a) printf("done;\n");
						if(!a && pParam->T_level) {
							a = T_adjust_level(*T_pBase, pParam->T_layers, pParam->T_level);
						}
						if(!a) {
							/* setting up renderer; */
							if(verbose) printf("initializing renderer...");
							if(!(last_error = Initialize_Renderer())) {
								int y, s;
								if(verbose) printf("success;\n");
								if(verbose) printf("using following parametres (zadpexy): %i %i %f %f %f %i %i;\n", pParam->Zoom, pParam->AA, pParam->Distance, pParam->Front, pParam->Eyeshift, pParam->Startx, pParam->Starty);
								if(verbose && pParam->AAr)
									printf("anti-artefacts feature enabled;\n");
								if(verbose) printf("processing...");
                                	
								for(y = 0, s = 0; (y < (**T_pBase)->Height); y++) {
									ProcessLine(y);
									/* print simple status points (without refresh) */
									for( ; verbose && (s < (58 * y / (**T_pBase)->Height)); s++)
										printf(".");
								}
              		                	
								if(verbose) printf("completed;\n");
	
								if(pParam->Triangles) {
									if(verbose)
										printf("adding triangles to the final image...");
									Add_Triangles(pStereo->Width/2, 20*pParam->Zoom, pTexture->Width*pParam->Zoom, pStereo);
									if(verbose)
										printf("done;\n");
								}
	
								if(verbose) printf("writing stereogram to disk...");
								if(!Write_Gfx_File(stereo_file_name, output_format, pStereo) && verbose)
									printf("completed;\n");
							} else {
								if(verbose)
									printf("FAILED;\n");
								else    	
									fprintf(stderr, "initializing renderer...FAILED;\n");
								switch (last_error) {
									case -1 :
										fprintf(stderr, "illegal parametre;\n\n");
										print_info();
										break;
									case -2 :
										fprintf(stderr, "width of the texture greater than the base;\nthis state is not processable;\ntry the -w option to resize the texture.\n");
										break;
									case -3 :
										fprintf(stderr, "could not allocate memory;\n");
										break;
									case -4 :
										fprintf(stderr, "base or texture image dimensions may NOT differ;\n");
										break;
									default :
										fprintf(stderr, "illegal parametre/unable to initialize renderer;\n\n");
										print_info();
								}
								return -1;
							}
							Clear_Renderer();
						}
						for(z = 0; z <= pParam->T_layers; z++)
							free((*((*T_pTexture)+z))->Data);
					}
					for(z = 0; (z <= pParam->T_layers) && (a < 2); z++) {
						free((*((*T_pBase)+z))->Data);
						free(*((*T_pTexture)+z));
						free(*((*T_pBase)+z));
					}
				} else {
					/* malloc for the T_pBase or T_pTexture indices failed */
					fprintf(stderr, "could not allocate memory for the gfx input headers\n");
					return -1;
				}
				free(*T_pBase);
				free(*T_pTexture);
			/* enf of transparent trace */
		}
	} else {
		if(!verbose) printf("Stereograph for linux Version %s\n", STEREOGRAPH_VERSION);
		fprintf(stderr, "NOTIFY: must specify at least a base and a texture file\nor define a texture width for a random texture.\nuse the -w option to define a texture width.\nnote that you CANNOT use random textures for transparent rendering.\n\n");
		print_info();
		return -1;
	}
	return 0;
}

int parse_args (int argc, char **argv) {
	int z, s = 1;
	
	for(z = 1; z < argc; z++, s = 1)
		while((z < argc) && ((argv[z][0] == '-') && (argv[z][s] != '\0')))
			switch(argv[z][s]) {
				case 'b' :
					if (z < (argc - 1)) {
						if(!pParam->T_layers && !texture_file_name) {
							pParam->T_layers = 0;
							while(argv[z + 2 + pParam->T_layers][0] != '-')
								pParam->T_layers++;
						}
						if(pParam->T_layers) {
							T_base_file_name = argv + ++z;
							z += pParam->T_layers;
						}
						base_file_name = argv[++z];
					} else {
						fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);
						return -1;
					}
					break;
				case 't' :
					if (z < (argc - 1)) {
						if(!pParam->T_layers && !base_file_name) {
							pParam->T_layers = 0;
							while(argv[z + 2 + pParam->T_layers][0] != '-')
								pParam->T_layers++;
						}
						if(pParam->T_layers) {
							T_texture_file_name = argv + ++z;
							z += pParam->T_layers;
						}
						texture_file_name = argv[++z];
					} else {
						fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);
						return -1;
					}
					break;
				case 'o' :
					if (z < (argc - 1)) stereo_file_name = argv[++z]; else { fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);  return -1; }
					break;
				case 'f' :
					if (z < (argc - 1)) {
						if(!strcmp(argv[++z], "png"))
							output_format = GFX_PNG;
						else if(!strcmp(argv[z], "ppm"))
							output_format = GFX_PPM;
						else if(!strcmp(argv[z], "tga"))
							output_format = GFX_TARGA;
						else
							output_format = atoi(argv[z]);
					} else { fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);  return -1; }
					break;
				case 'x' :
					if (z < (argc - 1)) pParam->Startx = atoi(argv[++z]); else { fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);  return -1; }
					break;
				case 'y' :
					if (z < (argc - 1)) pParam->Starty = atoi(argv[++z]); else { fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);  return -1; }
					break;
				case 'd' :
					if (z < (argc - 1)) pParam->Distance = (float) atof(argv[++z]); else { fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);  return -1; }
					break;
				case 'p' :
					if (z < (argc - 1)) pParam->Front = (float) atof(argv[++z]); else { fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);  return -1; }
					break;
				case 'e' :
					if (z < (argc - 1)) pParam->Eyeshift = (float) atof(argv[++z]); else { fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);  return -1; }
					break;
				case 'a' :
					if (z < (argc - 1)) pParam->AA = atoi(argv[++z]); else { fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);  return -1; }
					break;
				case 'z' :
					if (z < (argc - 1)) pParam->Zoom = atoi(argv[++z]); else { fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);  return -1; }
					break;
				case 'w' :
					if (z < (argc - 1)) texture_width_to_use = atoi(argv[++z]); else { fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);  return -1; }
					break;
				case 'l' :
					if (z < (argc - 1)) {
						if(!strcmp(argv[++z], "top"))
							pParam->T_level = T_TOP_LEVEL;
						else if(!strcmp(argv[z], "back"))
							pParam->T_level = T_BACK_LEVEL;
						else if(!strcmp(argv[z], "none"))
							pParam->T_level = T_NO_LEVEL;
						else
							pParam->T_level = atoi(argv[z]);
					} else {
						fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);  return -1;
					}
					break;
				case 'I' :
					pParam->Invert = 1;
					s++;
					break;
				case 'C' :
					random_texture_type = TEX_COLORED;
					s++;
					break;
				case 'G' :
					random_texture_type = TEX_GRAYSCALE;
					s++;
					break;
				case 'M' :
					random_texture_type = TEX_BW;
					s++;
					break;
				case 'S' :
					random_texture_type = TEX_SINLINE;
					s++;
					break;
				case 'R' :
					pParam->AAr = 1;
					s++;
					break;
				case 'L' :
					pParam->Linear = 0;
					s++;
					break;
				case 'A' :
					pParam->Triangles = 1;
					s++;
					break;
				case 'v' :
					verbose = 1;
					s++;
					break;
				/* obsolete, for compatibility purpose, only temporary */
				case 'T' :
					fprintf(stderr, "the T option is obsolete, please take a look to the ChangeLog or the README.\n");
					if(z > 1) { fprintf(stderr, "invalid argument '%c';\nthe T descriptor must PRECEED any argument!\n", argv[z][s]);  return -1; }
					if (z < (argc - 1)) pParam->T_layers = atoi(argv[++z]); else { fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);  return -1; }
					break;
				case 'V' :
					printf("stereograph-%s\n", STEREOGRAPH_VERSION);
					return -2;
				default:
					fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);
					return -1;
			}
	return 0;
}
