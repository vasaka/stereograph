/* Stereograph
 * Header file;
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


#define STEREOGRAPH_QUIT  -256
#define STEREOGRAPH_ERROR  -32


/* the following struct forces to include renderer.h before being included itself  */
/* STEREOGRAPH_PARAMS
   represents all the data that is used as interface to the renderer; but not only
   for communication purposes we need this struct, it organizes all memory used by
   any stage of the rendering process:

   * first of all we fill it up with default values (done by stereograph_param_init
     which itself calls renderer_param_init and renderer_gfx_init to set up the
     default values in the RENDERER_PARAMS struct and the gfx references in the
     RENDERER_DATA struct)

   * secondly we update it with the params requested by the user
     (stereograph_parse_args)

   * finally we use the given params to stereograph_run which does all the work
     based on them, including:
     * loading of all gfx (stereograph_gfx_prepare)
     * preparation of all gfx (also done in stereograph_gfx_prepare)
     * processing (rendering) of the stereo gfx (done by stereograph_gfx_process)
     * postprocessing the stereo gfx (also in stereograph_gfx_process)
     * writing the stereo gfx to disk (again stereograph_gfx_process)
     * closing the renderer (still in stereograph_gfx_process, note that the REAL
       init for the renderer is done also in stereograph_gfx_process!)
     * unload all gfx except the stereo gfx (stereograph_gfx_close, note that the
       stereo gfx is cleared immediately after writing it to disk in
       stereograph_gfx_process by the call to Renderer_Close)


   abstract we have the following stages:
     1. set up an instance of STEREOGRAPH_PARAMS
     2. init and update the RENDERER_PARAMS struct in STEREOGRAPH_PARAMS
        (init is done by Renderer_Param_Init; you may change all values in there,
        at least those which are initialized by Renderer_Param_Init; of course you
        may set upt the manipulators marked below to suit your needs; load your gfx)
     3. init RENDERER_DATA with first Renderer_GFX_Init and then Renderer_Initialize;
        after the call to Renderer_Initialize you cannot change any param anymore;
     4. if Renderer_Initialize succeeds (returns 0) we are now ready to work down
        the base file line by line with Renderer_ProcessLine;
     5. save the data of the stereo gfx before the call of Renderer_Close which
        finalizes the RENDERER_DATA struct;
     6. unload your gfx if necessary;

   one may ask why the renderer takes control over the existence of the stereo gfx;
   the reason is simple: the size of the stereo gfx depends on the params (at the
   moment only on the zoom value); so the renderer and only the renderer decides on the
   size of the gfx;

*/
struct STEREOGRAPH_PARAMS {
        /* file io */
        char *Base_File_Name;
        char *Texture_File_Name;
        char *Stereo_File_Name;
        char **T_Base_File_Name;
        char **T_Texture_File_Name;
        int Output_Format;

        /* base manipulator */
	int Base_Invert;
	int T_Base_Level;
        /* texture generator and/or texture manipulator */
        int Texture_Width_To_Use;
        int Texture_Type;
        /* port render stereogram manipulators */
	int Stereo_Triangles;	

        struct GFX_DATA GFX_Base[RENDERER_T_MAX_LAYERS];
        struct GFX_DATA GFX_Texture[RENDERER_T_MAX_LAYERS];
        struct GFX_DATA GFX_Stereo;

        /* interface to the renderer */
        struct RENDERER_PARAMS Renderer_Params;

        /* renderer working data */
        struct RENDERER_DATA Renderer_Data;
};


/* public */
int stereograph_main(int argc, char **argv);
int stereograph_run(struct STEREOGRAPH_PARAMS *pParams);


/* private */
int stereograph_gfx_prepare(struct STEREOGRAPH_PARAMS *pParams);
int stereograph_gfx_process(struct STEREOGRAPH_PARAMS *pParams);
int stereograph_gfx_clean(struct STEREOGRAPH_PARAMS *pParams);

void stereograph_param_init(struct STEREOGRAPH_PARAMS *pParams);
int stereograph_parse_args (struct STEREOGRAPH_PARAMS *pParams, int argc, char **argv);

void stereograph_print_info(void);

