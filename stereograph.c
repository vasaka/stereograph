/* Stereograph 0.33a, 16/11/2003;
 * a stereogram generator;
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STEREOGRAPH_VERSION "0.33a"

#include "renderer.h"
#include "gfxio.h"
#include "stereograph.h"
#include "globals.h"
#ifndef STEREOGRAPH_ONLY
#include "../config.h"
#endif

#ifndef PACKAGE_DATA_DIR
#define PACKAGE_DATA_DIR "/usr/share/stereograph"
#endif


/* control for the default binary */
int main (int argc, char **argv) {

        return stereograph_main(argc, argv);

}



/* regard this function including the following as a reference implementation for a versatile interface to the renderer */
int stereograph_main(int argc, char **argv) {

        struct STEREOGRAPH_PARAMS stereograph_params;

	stereograph_verbose = 0;
	stereograph_error = 0;
        stereograph_include_dir = malloc(strlen(PACKAGE_DATA_DIR)+20);
        sprintf(stereograph_include_dir, "%s/include", PACKAGE_DATA_DIR);
		
	stereograph_param_init(&stereograph_params);

	if( (stereograph_error = stereograph_parse_args(&stereograph_params, argc, argv)) ) {
	        if(stereograph_error == STEREOGRAPH_QUIT)
		        return 0;
		else
		        return stereograph_error;
	}

	return stereograph_run(&stereograph_params);

}


/* this function manages all necessary inits and calls to all modules */
int stereograph_run(struct STEREOGRAPH_PARAMS *pParams) {

        int last_error;

	/* check the output channel that forces to be quiet if gfx out equals stdout */
	if(!pParams->Stereo_File_Name && pParams->Base_File_Name && pParams->Texture_File_Name && stereograph_verbose) {
		fprintf(stderr, "stereograph_verbose disabled for standard output\n");
		stereograph_verbose = 0;
	}

	if(stereograph_verbose) printf("Stereograph for linux Version %s\n", STEREOGRAPH_VERSION);

	if(!(last_error = stereograph_gfx_prepare(pParams)))
	        /* if all gfx data is ready, we process it */
	        last_error = stereograph_gfx_process(pParams);

	stereograph_gfx_clean(pParams);

	return last_error;

}


int stereograph_gfx_prepare(struct STEREOGRAPH_PARAMS *pParams) {
	int last_error = 0;

        /* let's see if there is a base file and a texture well defined */
        if(pParams->Base_File_Name && (pParams->Texture_File_Name || (pParams->Texture_Width_To_Use > 0))) {
	        if(pParams->Renderer_Params.T_Layers <= 0) {
		        /* single layer base and texture preparation */

		        /* first of all, load the base file */
		        if(stereograph_verbose) printf("loading base '%s'...", pParams->Base_File_Name);
			if(!GFX_Read_File(pParams->Base_File_Name, pParams->GFX_Base)) {

			        /* on success, load or generate the texutre */
			        if(!pParams->Texture_File_Name) {
			                /* generate it */
				        if(stereograph_verbose) printf("generating random texture (%i*%i)...", pParams->Texture_Width_To_Use, pParams->GFX_Base->Height);
					if(pParams->Texture_Width_To_Use < pParams->GFX_Base->Width) {
					        last_error = GFX_Generate_RandomTexture(pParams->GFX_Texture, pParams->Texture_Width_To_Use, pParams->GFX_Base->Height, pParams->Texture_Type);
					} else {
					        if(stereograph_verbose) printf("FAILED\n");
						fprintf(stderr, "Texture_Width_To_Use is too big. This parameter may not be greater than the base width.\n");
						last_error = STEREOGRAPH_ERROR;
					}
				} else {
				        /* load it */
				        if(stereograph_verbose) printf("loading texture '%s'...", pParams->Texture_File_Name);
					last_error = GFX_Read_File(pParams->Texture_File_Name, pParams->GFX_Texture);
					if(!last_error && (pParams->Texture_Width_To_Use > 0))
					        last_error = GFX_Resize(pParams->GFX_Texture, pParams->Texture_Width_To_Use);
				}
				/* invert the base if necessary */
				if(!last_error && pParams->Base_Invert) {
				        if( (last_error = GFX_Invert(pParams->GFX_Base)) ) {
					        if(stereograph_verbose) printf("FAILED\n");
						fprintf(stderr, "error inverting base;\n");
					}
				}

			} else { /* gfx base read error */
			        return STEREOGRAPH_ERROR; // ?
			}
		} else {
		        /* multiple layer preparation */
			int z;
			
			if(stereograph_verbose) printf("rendering transparent layers enabled\nloading bases:\n");
			for(z = 0; (z <= pParams->Renderer_Params.T_Layers) && !last_error; z++) {
			        if(stereograph_verbose) printf("%s ", pParams->T_Base_File_Name[z]);
				last_error = GFX_Read_File(pParams->T_Base_File_Name[z], pParams->GFX_Base + z);
			}

			if(!last_error) {
			        if(stereograph_verbose) printf("done;\nloading textures:\n");
				for(z = 0; (z <= pParams->Renderer_Params.T_Layers) && !last_error; z++) {
				        if(stereograph_verbose) printf("%s ", pParams->T_Texture_File_Name[z]);
					last_error = GFX_Read_File(pParams->T_Texture_File_Name[z], pParams->GFX_Texture + z);
					if(!last_error)
					        last_error = GFX_Resize(pParams->GFX_Texture + z, pParams->Texture_Width_To_Use);
				}

				if(stereograph_verbose && !last_error) printf("done;\n");
				if(!last_error && pParams->T_Base_Level) {
				        last_error = GFX_T_AdjustLevel(pParams->GFX_Base, pParams->Renderer_Params.T_Layers, pParams->T_Base_Level);
				}
			}
		}

	} else {

	        /* parameter error */
	        if(!stereograph_verbose) printf("Stereograph for linux Version %s\n", STEREOGRAPH_VERSION);
		fprintf(stderr, "NOTIFY: must specify at least a base and a texture file\nor define a texture width for a random texture.\nuse the -w option to define a texture width.\nnote that you CANNOT use random textures for transparent rendering.\n\n");
		stereograph_print_info();
		return STEREOGRAPH_ERROR;
	}

	return last_error;
}


int stereograph_gfx_clean(struct STEREOGRAPH_PARAMS *pParams) {

        if(pParams->Renderer_Params.T_Layers <= 0) {

                if(pParams->GFX_Texture->Data != NULL) {
	                free(pParams->GFX_Texture->Data);
		        pParams->GFX_Texture->Data = NULL;
	        }        
                if(pParams->GFX_Base->Data != NULL) {
	                free(pParams->GFX_Base->Data);
		        pParams->GFX_Base->Data = NULL;
                }

        } else {
	        int z;

		for(z = 0; z <= pParams->Renderer_Params.T_Layers; z++) {
		        if(pParams->GFX_Texture[z].Data != NULL) {
			        free(pParams->GFX_Texture[z].Data);
				pParams->GFX_Texture[z].Data = NULL;
			}        
			if(pParams->GFX_Base[z].Data != NULL) {
			        free(pParams->GFX_Base[z].Data);
				pParams->GFX_Base[z].Data = NULL;
			}
		}

        }

	return 0;
}


int stereograph_gfx_process(struct STEREOGRAPH_PARAMS *pParams) {
	int last_error = 0;

	/* setting up renderer; */
        if(stereograph_verbose) printf("initializing renderer...");
	if(!(last_error = Renderer_Initialize( &(pParams->Renderer_Data), &(pParams->Renderer_Params)  ) )) {
		int y, s;

		if(stereograph_verbose) printf("success;\n");

		/* verbose parameter info */
		if(stereograph_verbose) printf("using following parameters (zadpexy): %i %i %f %f %f %i %i;\n", pParams->Renderer_Params.Zoom, pParams->Renderer_Params.AA, pParams->Renderer_Params.Distance, pParams->Renderer_Params.Front, pParams->Renderer_Params.Eyeshift, pParams->Renderer_Params.StartX, pParams->Renderer_Params.StartY);
		if(stereograph_verbose && pParams->Renderer_Params.AAr)
			printf("anti-artefacts feature enabled;\n");
		if(stereograph_verbose && !pParams->Renderer_Params.Linear)
			printf("linear rendering disabled;\n");
		if(stereograph_verbose && pParams->Stereo_Triangles)
			printf("triangle aid enabled, a pair will be added after rendering;\n");

		/* here we actually render the stereogram */
		/* the calls are similar for single and mutiple layers */
		if(stereograph_verbose) printf("processing...");
		for(y = 0, s = 0; (y < pParams->GFX_Base->Height); y++) {
			Renderer_ProcessLine(y, &(pParams->Renderer_Data) );
			/* print simple status points (without refresh) */
			for( ; stereograph_verbose && (s < (58 * y / pParams->GFX_Base->Height)); s++)
				printf(".");
		}
		if(stereograph_verbose) printf("completed;\n");

	} else {
	        /* initializing renderer failed if control reaches this point */
	        if(stereograph_verbose)
			printf("FAILED;\n");
		else    	
			fprintf(stderr, "initializing renderer...FAILED;\n");
		switch (last_error) {
			case RENDERER_ERROR_INIT_ILLEGAL_PARAMS :
				fprintf(stderr, "illegal parameter;\n\n");
				stereograph_print_info();
				break;
			case RENDERER_ERROR_INIT_ILLEGAL_BASE_TEX_RES :
				fprintf(stderr, "width of texture file greater than the base;\nthis state is not processable;\ntry the -w option to resize the texture.\n");
				break;
			case RENDERER_ERROR_INIT_MEMORY_PROBLEM :
				fprintf(stderr, "could not allocate memory.\n");
				break;
			case RENDERER_ERROR_INIT_ILLEGAL_T_LAYERS :
				fprintf(stderr, "number of transparent layers exceeds max definition.\n");
				break;
		        default:
				fprintf(stderr, "unkown error.\n");
		}

		/* we need to close the renderer anyway */
		Renderer_Close( &(pParams->Renderer_Data) );

		return STEREOGRAPH_ERROR;
	}

	/* post rendering manipulations */
	if(pParams->Stereo_Triangles) {
	        if(stereograph_verbose)
			printf("adding triangles to the final image...");
		GFX_AddTriangles(pParams->GFX_Stereo.Width / 2, 20 * pParams->Renderer_Params.Zoom, pParams->GFX_Texture->Width * pParams->Renderer_Params.Zoom, &(pParams->GFX_Stereo) );
		if(stereograph_verbose)
		        printf("done;\n");
	}

	if(stereograph_verbose) printf("writing stereogram to disk...");
	if(!GFX_Write_File(pParams->Stereo_File_Name, pParams->Output_Format, &(pParams->GFX_Stereo) ) && stereograph_verbose)
		printf("completed;\n");

	/* the job is done, clean the mem */
	Renderer_Close( &(pParams->Renderer_Data) );

	return 0;
}


/* fill up parameter list with standard values */
void stereograph_param_init(struct STEREOGRAPH_PARAMS *pParams) {
        int z;

	/* file io */
        pParams->Base_File_Name = NULL;
        pParams->Texture_File_Name = NULL;
        pParams->Stereo_File_Name = NULL;
        pParams->T_Base_File_Name = NULL;
        pParams->T_Texture_File_Name = NULL;
   
#ifdef X11GUI   
        pParams->Output_Format = GFX_IO_X11;
#else
        pParams->Output_Format = GFX_IO_PNG;
#endif
	/* GFX manipulators, generators */
	pParams->Base_Invert = 0;
	pParams->T_Base_Level = GFX_T_BACK_LEVEL;
        pParams->Texture_Width_To_Use = 0;
        pParams->Texture_Type = GFX_TEX_GRAYSCALE;
	pParams->Stereo_Triangles = 0;
	/* GFX datas */
	for(z = 0; z < RENDERER_T_MAX_LAYERS; z++) {
	        pParams->GFX_Base[z].Data = NULL;
        	pParams->GFX_Texture[z].Data = NULL;
	}
       	pParams->GFX_Stereo.Data = NULL;
        /* renderer, interface */
        Renderer_Param_Init( &(pParams->Renderer_Params) );
        Renderer_GFX_Init( &(pParams->Renderer_Data) , pParams->GFX_Base, pParams->GFX_Texture, &(pParams->GFX_Stereo) );
}


int stereograph_parse_args (struct STEREOGRAPH_PARAMS *pParams, int argc, char **argv) {
	int z, s;
	
	for(z = 1; z < argc; z++) {
	        s = 1;
		while((z < argc) && ((argv[z][0] == '-') && (argv[z][s] != '\0')))
			switch(argv[z][s]) {
				case 'b' :
				        /* read base file name(s) */
					if (z < (argc - 1)) {
						if(!(pParams->Renderer_Params.T_Layers) && !(pParams->Texture_File_Name)) {
						        /* for transparent rendering first of all the base files must be given */
							pParams->Renderer_Params.T_Layers = 0;
							while(z + 2 + (pParams->Renderer_Params.T_Layers) < argc && argv[z + 2 + (pParams->Renderer_Params.T_Layers)][0] != '-')
								pParams->Renderer_Params.T_Layers++;
						}
						if(pParams->Renderer_Params.T_Layers) {
							pParams->T_Base_File_Name = argv + (++z);
							z += pParams->Renderer_Params.T_Layers;
						}
						pParams->Base_File_Name = argv[++z];
					} else {
						fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);
						return STEREOGRAPH_ERROR;
					}
					break;
				case 't' :
				        /* read the texture file name(s) */
					if (z < (argc - 1)) {
						if(!(pParams->Renderer_Params.T_Layers) && !(pParams->Base_File_Name)) {
							pParams->Renderer_Params.T_Layers = 0;
							while(argv[z + 2 + (pParams->Renderer_Params.T_Layers)][0] != '-')
								pParams->Renderer_Params.T_Layers++;
						}
						if(pParams->Renderer_Params.T_Layers) {
							pParams->T_Texture_File_Name = argv + (++z);
							z += pParams->Renderer_Params.T_Layers;
						}
						pParams->Texture_File_Name = argv[++z];
					} else {
						fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);
						return STEREOGRAPH_ERROR;
					}
					break;
				case 'o' :
				        /* get output file name */
					if (z < (argc - 1))
					        pParams->Stereo_File_Name = argv[++z];
					else {
					        fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);
						return STEREOGRAPH_ERROR;
					}
					break;
				case 'f' :
				        /* get optional file output format */
					if (z < (argc - 1)) {
						if(!strcasecmp(argv[++z], "x11"))
							pParams->Output_Format = GFX_IO_X11;
						else if(!strcasecmp(argv[z], "jpg"))
							pParams->Output_Format = GFX_IO_JPG;
					        else if(!strcasecmp(argv[z], "png"))
							pParams->Output_Format = GFX_IO_PNG;
						else if(!strcasecmp(argv[z], "ppm"))
							pParams->Output_Format = GFX_IO_PPM;
						else if(!strcasecmp(argv[z], "tga"))
							pParams->Output_Format = GFX_IO_TARGA;
						else
							pParams->Output_Format = atoi(argv[z]);
					} else {
					        fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);
						return STEREOGRAPH_ERROR;
					}
					break;
				case 'I' :
				        /* get C script include dir */
					if (z < (argc - 1))
			     			stereograph_include_dir = argv[++z];
					else {
					        fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);
						return STEREOGRAPH_ERROR;
					}
					break;

			 	case 'x' :
					if (z < (argc - 1))
					        pParams->Renderer_Params.StartX = atoi(argv[++z]);
					else {
					        fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);
						return STEREOGRAPH_ERROR;
					}
					break;
				case 'y' :
					if (z < (argc - 1))
					        pParams->Renderer_Params.StartY = atoi(argv[++z]);
					else {
					        fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);
						return STEREOGRAPH_ERROR;
					}
					break;
				case 'd' :
					if (z < (argc - 1))
					        pParams->Renderer_Params.Distance = (float) atof(argv[++z]);
					else {
					        fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);
						return STEREOGRAPH_ERROR;
					}
					break;
				case 'p' :
					if (z < (argc - 1))
					        pParams->Renderer_Params.Front = (float) atof(argv[++z]);
					else {
					        fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);
						return STEREOGRAPH_ERROR;
					}
					break;
				case 'e' :
					if (z < (argc - 1))
					        pParams->Renderer_Params.Eyeshift = (float) atof(argv[++z]);
					else {
					        fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);
						return STEREOGRAPH_ERROR;
					}
					break;
				case 'a' :
					if (z < (argc - 1))
					        pParams->Renderer_Params.AA = atoi(argv[++z]);
					else {
					        fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);
						return STEREOGRAPH_ERROR;
					}
					break;
				case 'z' :
					if (z < (argc - 1))
					        pParams->Renderer_Params.Zoom = atoi(argv[++z]);
					else {
					        fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);
						return STEREOGRAPH_ERROR;
					}
					break;
				case 'w' :
					if (z < (argc - 1))
					        pParams->Texture_Width_To_Use = atoi(argv[++z]);
					else {
					        fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);
						return STEREOGRAPH_ERROR;
					}
					break;
				case 'l' :
					if (z < (argc - 1)) {
						if(!strcmp(argv[++z], "top"))
							pParams->T_Base_Level = GFX_T_TOP_LEVEL;
						else if(!strcmp(argv[z], "back"))
							pParams->T_Base_Level = GFX_T_BACK_LEVEL;
						else if(!strcmp(argv[z], "none"))
							pParams->T_Base_Level = GFX_T_NO_LEVEL;
						else
							pParams->T_Base_Level = atoi(argv[z]);
					} else {
						fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);  return STEREOGRAPH_ERROR;
					}
					break;
				case 'i' :
					pParams->Base_Invert = 1;
					s++;
					break;
			        /* get random texture params */
				case 'C' :
					pParams->Texture_Type = GFX_TEX_COLORED;
					s++;
					break;
				case 'G' :
					pParams->Texture_Type = GFX_TEX_GRAYSCALE;
					s++;
					break;
				case 'M' :
					pParams->Texture_Type = GFX_TEX_BW;
					s++;
					break;
				case 'S' :
					pParams->Texture_Type = GFX_TEX_SINLINE;
					s++;
					break;
				/* random textures end here */
				case 'R' :
					pParams->Renderer_Params.AAr = 1;
					s++;
					break;
				case 'L' :
					pParams->Renderer_Params.Linear = 0;
					s++;
					break;
				case 'A' :
					pParams->Stereo_Triangles = 1;
					s++;
					break;
				case 'v' :
					stereograph_verbose = 1;
					s++;
					break;
				/* obsolete, for compatibility purpose, only temporary */
				case 'T' :
					fprintf(stderr, "the T option is obsolete, please take a look to the ChangeLog or the README.\n");
					if(z > 1) { fprintf(stderr, "invalid argument '%c';\nthe T descriptor must PRECEED any argument!\n", argv[z][s]);  return STEREOGRAPH_ERROR; }
					if (z < (argc - 1)) pParams->Renderer_Params.T_Layers = atoi(argv[++z]); else { fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);  return STEREOGRAPH_ERROR; }
					break;
				case 'V' :
				        /* print version info */
					printf("stereograph-%s\n", STEREOGRAPH_VERSION);
					return STEREOGRAPH_QUIT;
				case 'h':
				case 'H':
			   		stereograph_print_info();
			   		exit(0);
				default:
				        /* default invalid argument message */
					fprintf(stderr, "invalid argument '%c';\n", argv[z][s]);
					return STEREOGRAPH_ERROR;
			}
	}
	return 0;
}



void stereograph_print_info(void) {
	printf("SYNOPSIS\n\
stereograph [OPTIONS] -b [base] [-t [texture]] [-w n]\n\
            [-o [output]] [-f x11/jpg/png/ppm/tga] [-l none/back/top]\n\n");
	printf("OPTIONS\n\
\t-a anti-aliasing     -z zoom               (quality)\n\
\t-d distance          -p front factor                    \n\
\t-e eye shift                               (perspective)\n\
\t-x texture insert x  -y texture insert y   (layout)\n\
\t-w texture width to use\n\
\t-M/-G/-C generate a monochrome, grayscale or colored\n\
\t         random background                 (no transparency)\n\
\t-S generate an experimental random texture (no transparency)\n\
\t-A add a pair of triangles                 (as an Aid to vision)\n\
\t-R this flag enables the anti-artefact feature\n\
\t-L disables the linear rendering algorithm\n\
\t-l sets the level adjust mode for transparent rendering\n\
\t-i invert the base\n\
\t-I defines the include directory path\n\
\t-f defines the output format\n\
\t-v verbose mode\n\
\t-V display version\n\n");
}


