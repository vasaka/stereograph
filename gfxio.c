/* Stereograph 0.33b, 17/11/2003;
 * Graphics I/O functions;
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
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <dlfcn.h>
#include <endian.h>

#include <png.h>
#include <jpeglib.h>

#ifdef X11GUI
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysymdef.h>
#include <X11/keysym.h>
typedef unsigned long Pixel;
#endif /* X11GUI */

#include "renderer.h"
#include "gfxio.h"
#include "globals.h"

#define frand1() ((float)rand() / (float)RAND_MAX)
#define PI 3.14159265358979
#define DIRECT 0
#define AUXIL  1

/* add a pair of triangles to a stereogram */
int GFX_AddTriangles(int x, int size, int width, struct GFX_DATA *gfx) {
	int z, y;
	for(y = 0; (y < size) && (y < gfx->Height); y++)
		for(z = 0; (z < (size - (y * 2))); z++)
			if(((y + x - (width/2) + z) >= 0) && ((y + x + width/2 + z) < gfx->Width))
				gfx->Data[(y * gfx->Width) + y + x - width/2 + z] = gfx->Data[(y * gfx->Width) + y + x + width/2 + z] = 0;

	return 0;
}


/* generates a random texture */
int GFX_Generate_RandomTexture(struct GFX_DATA *gfx, int width, int height, int type) {
	int a = 0, z;
	gfx->Data = (int*) malloc(width * height * sizeof(int));
	if(gfx->Data) {
		srand((unsigned int)time(NULL));
		gfx->Width = width;
		gfx->Height = height;
		switch(type) {
			case GFX_TEX_BW:
				for(z = 0; z < (height * width); z++)
					gfx->Data[z] = (rand()&1) * 65793 * 255;
				break;
			case GFX_TEX_GRAYSCALE:
				for(z = 0; z < (height * width); z++)
					gfx->Data[z] = (rand()&255) * 65793;
				break;
			case GFX_TEX_COLORED:
				for(z = 0; z < (height * width); z++)
					gfx->Data[z] = (rand()&(65793 * 255));
				break;
                        case GFX_TEX_SINLINE:
                                a = GFX_Generate_SinlineTexture(gfx);
                                break;
        		default:
				free(gfx->Data);
				gfx->Data = NULL;
				if (stereograph_verbose) printf("FAILED\n");
				fprintf(stderr, "unsupported random texture type;\n");
				a = GFX_ERROR_UNSUPP_RANDOMTEX;
		}
	} else {
		if (stereograph_verbose) printf("FAILED\n");
		fprintf(stderr, "could not allocate memory for random texture;\n");
		a = GFX_ERROR_MEMORY_PROBLEM;
	}
	if(stereograph_verbose && !a) printf("done;\n");

	return a;
}


/* resizes a texture to the width of width pixels */
int GFX_Resize(struct GFX_DATA *gfx, int width) {
	int y;
	int *temp_gfx;

	/* only resize the gfx if necessary */
	if((width > 0) && (gfx->Width >= width)) {

	        if(stereograph_verbose) printf("resizing texture...");
		temp_gfx = (int*) malloc(gfx->Height * width * sizeof(int));
		if(!temp_gfx) {
		        if(stereograph_verbose) printf("FAILED;\n");
			fprintf(stderr, "error allocating memory for texture!\n");
			return GFX_ERROR_MEMORY_PROBLEM;
		} else
		        for(y = 0; (y < gfx->Height); y++)
			        memcpy(temp_gfx + y * width, gfx->Data + gfx->Width * y, width * sizeof(int));
		free(gfx->Data);
		gfx->Data = temp_gfx;
		gfx->Width = width;
		if(stereograph_verbose) printf("done;\n");

	}

	return 0;
}


/* inverts a base (gray scaled) image */
int GFX_Invert(struct GFX_DATA *gfx) {
	int z, r = 0, g = 0, b = 0;

	if(stereograph_verbose) printf("inverting base...");
	for(z = 0; z < (gfx->Width * gfx->Height); z++) {
		r = 255 -  (gfx->Data[z]        & 255);
		g = 255 - ((gfx->Data[z] >>  8) & 255);
		b = 255 - ((gfx->Data[z] >> 16) & 255);
		gfx->Data[z] = r + (g << 8) + (b << 16);
	}
	if(stereograph_verbose) printf("done;\n");

	return 0;
}


/* adjusts the base levels for transparent rendering */
int GFX_T_AdjustLevel(struct GFX_DATA *T_gfx, int T_layers, int T_level) {
	int t, z, r_t = 0, g_t = 0, b_t = 0, r = 0, g = 0, b = 0;

	if(stereograph_verbose) printf("ajdusting levels...");
	for(t = 1; t < T_layers; t++)
		for(z = 0; z < (T_gfx[0].Width * T_gfx[0].Height); z++) {
			r =  T_gfx[t - 1].Data[z]        & 255;
			g = (T_gfx[t - 1].Data[z] >>  8) & 255;
			b = (T_gfx[t - 1].Data[z] >> 16) & 255;
			r_t =  T_gfx[t].Data[z]        & 255;
			g_t = (T_gfx[t].Data[z] >>  8) & 255;
			b_t = (T_gfx[t].Data[z] >> 16) & 255;
			switch (T_level) {
				case GFX_T_NO_LEVEL:
					break;
				case GFX_T_BACK_LEVEL:
					if(!r_t && !g_t && !r_t) {
						r_t = r;
						g_t = g;
						b_t = b;
					}
					break;
				case GFX_T_TOP_LEVEL:
					if((r_t + g_t + r_t) < (r + g + b)) {
						r_t = r;
						g_t = g;
						b_t = b;
					}
					break;
				default :
					if(stereograph_verbose) printf("FAILED\n");
					fprintf(stderr, "Unimplemented transparency level;\n");
					return GFX_ERROR_UNSUPP_T_LEVEL;
			}
			T_gfx[t].Data[z] = r_t + (g_t << 8) + (b_t << 16);
		}
	if(stereograph_verbose) printf("done;\n");

	return 0;
}


/* reads a png or targa file */
int GFX_Read_File (char *file_name, struct GFX_DATA *gfx)
{
	int a;
	FILE *ifile = NULL;
	unsigned char check_header[8];

	ifile = fopen(file_name, "r");

	if(ifile == NULL) {
		if(stereograph_verbose) printf("FAILED;\n"); else fprintf(stderr, "loading gfx data...FAILED;\n");
		fprintf(stderr, "cannot open file '%s'!\n", file_name);
		return GFX_ERROR_READ_ERROR;
	} else {
		a = GFX_Identify_File(ifile, check_header);
		switch (a) {
			case GFX_IO_JPG :
				a = GFX_Read_JPG(ifile, check_header, gfx);
				break;		   
			case GFX_IO_PNG :
				a = GFX_Read_PNG(ifile, check_header, gfx);
				break;
			case GFX_IO_TARGA :
				a = GFX_Read_TARGA(ifile, check_header, gfx);
				break;
			case GFX_IO_PPM :
				a = GFX_Read_PPM(ifile, check_header, gfx);
				break;
			case GFX_IO_C :
				a = GFX_Read_C(ifile, file_name, check_header, gfx);
				break;		   
		}
	}

	fclose(ifile);
	ifile = NULL;
	if(!a && stereograph_verbose) printf("gfx data read (%i*%i);\n", gfx->Width, gfx->Height);

	return a;
}


/* writes a png, ppm or targa file */
int GFX_Write_File (char *file_name, int output_format, struct GFX_DATA *gfx)
{
	int a;
	FILE *ofile = NULL;

	if(stereograph_verbose) printf("saving '%s' (%i*%i)...", file_name, gfx->Width, gfx->Height);

	if(!file_name)
		ofile = stdout;
	else
		ofile = fopen(file_name, "w+");

	if(ofile == NULL) {
		if(stereograph_verbose) printf("FAILED;\n"); else fprintf(stderr, "writing gfx data...FAILED;\n");
		fprintf(stderr, "cannot create file %s!\n", file_name);
		return -1;
	} else

	switch (output_format) {
#ifdef X11GUI	   
	        case GFX_IO_X11 :
		        a = GFX_Write_X11(ofile, gfx);
			break;
#endif /* X11GUI */	   
		case GFX_IO_TARGA :
			a = GFX_Write_TARGA(ofile, gfx);
			break;
		case GFX_IO_JPG :
			a = GFX_Write_JPG(ofile, gfx);
			break;
		case GFX_IO_PNG :
			a = GFX_Write_PNG(ofile, gfx);
			break;	   
		case GFX_IO_PPM :
			a = GFX_Write_PPM(ofile, gfx);
			break;
	        default :
			if(strlen(file_name) >= 4) {
				if(!strcmp(file_name + strlen(file_name) - 4, ".tga"))
					a = GFX_Write_TARGA(ofile, gfx);
				else if(!strcmp(file_name + strlen(file_name) - 4, ".png"))
					a = GFX_Write_PNG(ofile, gfx);
				else if(!strcmp(file_name + strlen(file_name) - 4, ".ppm"))
					a = GFX_Write_PPM(ofile, gfx);
				else {
					if(stereograph_verbose) printf("FAILED;\n"); else fprintf(stderr, "writing gfx data...FAILED;\n");
					fprintf(stderr, "unsupported output format!\n");
					a = GFX_ERROR_UNSUPP_FORMAT;
				}
			} else {
				if(stereograph_verbose) printf("FAILED;\n"); else fprintf(stderr, "writing gfx data...FAILED;\n");
				fprintf(stderr, "unsupported output format!\n");
				a = GFX_ERROR_UNSUPP_FORMAT;
			}       	
	}

	fclose(ofile);
	ofile = NULL;

	return a;
}




/*** gfxio internals ***/
int GFX_Identify_File(FILE *ifile, unsigned char *check_header) {

	if(fread(check_header, sizeof(char), 2, ifile) != 2) {
		if(stereograph_verbose) printf("FAILED!\n"); else fprintf(stderr, "reading gfx data...FAILED;\n");
		fprintf(stderr, "cannot read header! (file corrupt?)\n");
		return GFX_ERROR_READ_ERROR;
	} else if((check_header[0] == 'P') && ((check_header[1] == '3') || (check_header[1] == '6')))
		return GFX_IO_PPM;
        else if((check_header[0] == '/') && ((check_header[1] == '*')))
		return GFX_IO_C;					     
	else {
		if(fread(check_header + 2, sizeof(char), 6, ifile) != 6) {
			if(stereograph_verbose) printf("FAILED!\n"); else fprintf(stderr, "reading gfx data...FAILED;\n");
			fprintf(stderr, "cannot read header! (file corrupt?)\n");
			return GFX_ERROR_READ_ERROR;
		} else {
		        if(!png_sig_cmp(check_header, 0, 8))
				return GFX_IO_PNG;
			else
		        if(check_header[0] == 0xff && check_header[1] == 0xd8 &&
			   (!strncasecmp(check_header+6, "jf", 2) ||
                            !strncasecmp(check_header+6, "ex", 2)))
				return GFX_IO_JPG;
                        else
				return GFX_IO_TARGA;
		}
	}
	if(stereograph_verbose) printf("FAILED;\n"); else fprintf(stderr, "reading gfx data...FAILED\n");
	fprintf(stderr, "cannot identify gfx file, unsupported format!\n");

	return GFX_ERROR_UNSUPP_FORMAT;
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
		c = c * 10 + (a - '0');
	ungetc(a, ifile);

	return c;
}

typedef double point[3];
typedef void (*BFunction)();
typedef double (*ZFunction)(double x, double y);
typedef void  (*PFunction)(point p, double x, double y);

double CONV_RAD=M_PI/180;

int color_range=765;
int auxil=DIRECT;

int width=0, height=0;
int *pixel_color = NULL;
struct GFX_DATA *global_gfx;
double X1=-1.0, X2=1.0, Y1=-1.0, Y2=1.0, Z1=-1.0, Z2=1.0;
double ratio_x=1.0, ratio_y=1.0, ratio_z=1.0;
double m_rot[12]={1,0,0,0,0,1,0,0,0,0,1,0};

double max(double x, double y)
{
   if(x>=y) return x; else return y;
}

double min(double x, double y)
{
   if(x>=y) return x; else return y;
}

void init_graphic_window(int w, int h, int mode)
{
     int s;

     width = w;
     height = h;
     auxil = mode;
     s = width * height * sizeof(int);
      
     global_gfx->Width = width;
     global_gfx->Height = height;
     global_gfx->Data = (int*)malloc(s);
     memset(global_gfx->Data, 0, s);
   
     if (auxil) {
	 pixel_color = (int *)malloc(s);
         memset(pixel_color, 0, s);
     } else
         pixel_color = global_gfx->Data;
}

void translation(double a, double b, double c)
{
   m_rot[3] = a;   
   m_rot[7] = b;   
   m_rot[11] = c;   
}

void scale(double a)
{
int i;

   for (i=0; i<=11; i++) m_rot[i] *= a;   
}

void rotation_angles(double theta, double phi, double psi)
{
double ca,cb,cc, sa,sb,sc;

   ca=theta*CONV_RAD;
   cb=phi*CONV_RAD;
   cc=psi*CONV_RAD;

   sa=sin(ca); ca=cos(ca);
   sb=sin(cb); cb=cos(cb);
   sc=sin(cc); cc=cos(cc);

   m_rot[0] = ca*cb;
   m_rot[1] = -sa*cc-ca*sb*sc;
   m_rot[2] = sa*sc-ca*sb*cc;

   m_rot[4] = sa*cb;
   m_rot[5] = ca*cc-sa*sb*sc;
   m_rot[6] = -ca*sc-sa*sb*cc;

   m_rot[8] = sb;
   m_rot[9] = cb*sc;
   m_rot[10]= cb*cc;

}

void rotate(point p, point q)
{
int i,j;
   for(i=0; i<=2; i++)
     {
     j=4*i;
     q[i] = m_rot[j]*p[0]+m_rot[j+1]*p[1]+m_rot[j+2]*p[2]+m_rot[j+3];
     }
}

void mesh(point *p, int h, int i, int j)
{
point a[3];
double ca,cb,cc,cd, delta, s,t, adds,addt, x, y, z0,z1,z2;
int u1, u2, v1, v2;
int k,l,m;

   rotate(p[h],a[0]);
   rotate(p[i],a[1]);
   rotate(p[j],a[2]);

   u1=width; u2=-1;
   v1=height; v2=-1;

   for(k=0; k<=2; k++)
     {
     l=(int)((a[k][0]-X1)/ratio_x);
     if(l<u1) u1=l;
     if(l>u2) u2=l;
     l=(int)((Y2-a[k][1])/ratio_y);
     if(l<v1) v1=l;
     if(l>v2) v2=l;
     }

    if(u1<0) u1=0;
    if(u2>=width) u2=width-1;
    if(v1<0) v1=0;
    if(v2>=height) v2=height-1;
    if(u1>u2 || v1>v2) return;

    z0 = a[0][2];
    z1 = a[1][2]-z0;
    z2 = a[2][2]-z0;

    z0 = (z0-Z1)*ratio_z;
    z1 = z1*ratio_z;
    z2 = z2*ratio_z;

    delta = (a[1][0]-a[0][0])*(a[2][1]-a[0][1]) 
                      - (a[1][1]-a[0][1])*(a[2][0]-a[0][0]);
    if (fabs(delta)<1E-5) return;
    
    delta=1/delta;
    ca = (a[2][1]-a[0][1])*delta;
    cb = (a[0][0]-a[2][0])*delta;
    cc = (a[0][1]-a[1][1])*delta;
    cd = (a[1][0]-a[0][0])*delta;
    adds = ca*ratio_x;
    addt = cc*ratio_x;

    for(l=v1; l<=v2; l++) 
      {
      x = X1 + u1*ratio_x - a[0][0];
      y = Y2 - l*ratio_y - a[0][1];
      s = ca*x + cb*y;
      t = cc*x + cd*y;
      for(k=u1; k<=u2; k++)
	{
        if (s>=-0.0001 && t>=-0.0001 && s+t<=1.0001)
	  {
          m = (int) (z0 + s*z1 + t*z2);
          if (m<0) m=0;
          if (m>color_range) m=color_range;
          if (m>pixel_color[k+l*width]) pixel_color[k+l*width] = m;
	  }
        s = s + adds;
        t = t + addt;
        }
      }
}

extern void put_pixel(int x, int y, int r, int g, int b)
{
	pixel_color[x+y*width] = r + (g<<8) + (b<<16);
}

void build(BFunction bfunct)
{
   bfunct();
}

void light(ZFunction zfunct)
{
int i, j;
int k;
double x,y;

   for(j=0; j<height; j++) 
   {
     y = Y2-ratio_y*j;
     for(i=0; i<width; i++)
     {
        x = ratio_x*i+X1;
        k = (int)((zfunct(x,y)-Z1)*ratio_z);
        if (k<0) k=0;
        if (k>color_range) k=color_range;
        if (k>pixel_color[i+j*width]) pixel_color[i+j*width] = k;
     }
   }
}

void graph(PFunction pfunct, double xi, double xs, int nx, 
                             double yi, double ys, int ny)
{
int i, j, l;
double hx,hy;
point p[4];

   if(nx<=0 || ny<=0) return;

   hx = (xs-xi)/nx;
   hy = (ys-yi)/ny;

   for(j=0; j<ny; j++) 
     {
     for(l=0; l<=1; l++)
         pfunct(p[l],xi,yi+(j+l)*hy);

     for(i=1; i<=nx; i++)
         {
	 memcpy(p[2], p[0], sizeof(point));
         memcpy(p[3], p[1], sizeof(point)); 
         for(l=0; l<=1; l++)
            pfunct(p[l], xi+i*hx,yi+(j+l)*hy);
         mesh(p,0,1,2); mesh(p,1,2,3);
         }
     }
}

void set_range(char c, double u1, double u2)
{
     if (width == 0 || height == 0) {
	fprintf(stderr, "init_graphic_window() should be used first !!\n");
	return;
     }
   
     if (c == 'x') {
	X1 = u1;
   	X2 = u2;
        ratio_x = (X2-X1)/(double)width;
        if (ratio_x == 0)
	    fprintf(stderr, "Values x1=%g, x2=%g invalid\n", X1, X2);
     } else
     if (c == 'y') {
	Y1 = u1;
   	Y2 = u2;
        ratio_y = (Y2-Y1)/(double)height;
        if (ratio_y == 0)
	    fprintf(stderr, "Values y1=%g, y2=%g invalid\n", Y1, Y2);
     } else
     if (c == 'z') {
	Z1 = u1;
   	Z2 = u2;
        ratio_z = Z2-Z1;
        if (ratio_z<=0)
	    fprintf(stderr, "Values z1=%g, z2=%g invalid\n", Z1, Z2);	
        else 
            ratio_z = ((double)color_range)/ratio_z;
     }   
}

void set_grey_levels()
{
    int x, y, k, p;
   
    for (y=0; y<height; y++)
    for (x=0; x<width; x++) {
        k = x + y*width;
        p = pixel_color[k];
        if (p)
            global_gfx->Data[k] = (p/3) + (((p+1)/3)<<8) + (((p+2)/3)<<16);
    }
}

void set_color_levels()
{
    if (!auxil) return;
    memcpy(global_gfx->Data, pixel_color, width*height*sizeof(int));
}


int GFX_Read_C (FILE *ifile, char *file_name, unsigned char *check_header, struct GFX_DATA *gfx)
{
    void *dl_handle = NULL;
    void *(* iproc)();
    char *ptr;
    char cmd[4096];

    global_gfx = gfx;
   
    ptr = rindex(file_name, '/');
    if (ptr) 
        ptr += 1;
    else
        ptr = file_name;
    sprintf(cmd, "gcc -c -I. -I./include -I%s %s -o /tmp/stereo-%s.o ; cd /tmp ; "
                 "gcc -shared -Wl,-soname,stereo-%s.so stereo-%s.o -o stereo-%s.so\n",
	         ((stereograph_include_dir)?
		  stereograph_include_dir:"."),
	         file_name, ptr, ptr, ptr, ptr);
    system(cmd);
    
    sprintf(cmd, "/tmp/stereo-%s.so", ptr);
    dl_handle = dlopen(cmd, RTLD_LAZY);
    system("rm -f /tmp/stereo-*");
    if (!dl_handle) {
       fprintf(stderr, "Couldn't create valid dynamic procedure !!\n");
       return -1;
    }
   
    iproc = dlsym(dl_handle, "process");
    if ((ptr = dlerror()) != NULL || !iproc) {
       if (ptr) fprintf(stderr, "Error: %s\n", ptr);
       fprintf(stderr, "Couldn't load process() in dynamic procedure !!\n");
       return -1;       
    }
   
    iproc();
    dlclose(dl_handle);

    if (auxil && pixel_color) {
       free(pixel_color);
       pixel_color = NULL;
    }
   
    return 0;
}

struct error_mgr {
    struct jpeg_error_mgr pub;	/* "public" fields */
    jmp_buf setjmp_buffer;	/* for return to caller */
};

static struct error_mgr jerr;
typedef struct error_mgr *error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */
static void
error_exit(j_common_ptr cinfo)
{
    char buf[JMSG_LENGTH_MAX];
      
    /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
    error_ptr err = (error_ptr) cinfo->err;

    cinfo->err->format_message(cinfo, buf);
    fprintf(stderr, "%s\n", buf);

    /* Return control to the setjmp point */
    longjmp(err->setjmp_buffer, 1);
}

int GFX_Read_JPG (FILE *ifile, unsigned char *check_header, struct GFX_DATA *gfx)
{
    int x, y, z;
    struct jpeg_decompress_struct cinfo;
    int row_stride, ncolors;
    JSAMPROW scanline[1];
    char *buffer;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = error_exit;

    if (setjmp(jerr.setjmp_buffer)) {
	/* If we get here, the JPEG code has signaled an error.
	 * We need to clean up the JPEG object, close the input file,
	 * and return.
	 */
	jpeg_destroy_decompress(&cinfo);
	return GFX_ERROR_LIBJPG;
    }
    jpeg_create_decompress(&cinfo);
    fseek(ifile, 0, SEEK_SET);
    jpeg_stdio_src(&cinfo, ifile);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    gfx->Width = cinfo.output_width;
    gfx->Height = cinfo.output_height;

    if (cinfo.jpeg_color_space == JCS_GRAYSCALE) {
	ncolors = cinfo.desired_number_of_colors;
	row_stride = gfx->Width;
    } else {
	row_stride = gfx->Width * 3;
    }

    buffer = malloc(row_stride);
    scanline[0] = (JSAMPROW) buffer;
    gfx->Data = (int*)malloc(gfx->Width*gfx->Height*sizeof(int));
    if (!buffer || !gfx->Data)
        return GFX_ERROR_MEMORY_PROBLEM;

    cinfo.quantize_colors = FALSE;
    z = 0;
    while (cinfo.output_scanline < gfx->Height) {
	(void) jpeg_read_scanlines(&cinfo, scanline, (JDIMENSION) 1);
        y = 0;
        /* fprintf(stderr, "JPEG : %d\n", cinfo.output_scanline); */
        if (cinfo.jpeg_color_space == JCS_GRAYSCALE)
	for(x = 0; x < gfx->Width; x++) {
	   gfx->Data[z] = buffer[y++] * 65793; /* 65536 + 256 + 1 */
	   z++;
	}
       	else
	for(x = 0; x < gfx->Width; x++) {
	   gfx->Data[z] = buffer[y++];
	   gfx->Data[z] += buffer[y++] << 8;	   
	   gfx->Data[z] += buffer[y++] << 16;
	   z++;
        }
    }
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    free(buffer);

    if (jerr.pub.num_warnings > 0) {	/* XXX */
	fprintf(stderr, "JPEG image may be corrupted\n");
	longjmp(jerr.setjmp_buffer, 1);
        return GFX_ERROR_LIBJPG;
    }
    return 0;   
}

   
int GFX_Read_PPM (FILE *ifile, unsigned char *check_header, struct GFX_DATA *gfx)
{
	int a, z, c_max;

	skip_space_n_comments(ifile);
	gfx->Width = get_dec(ifile);
	skip_space_n_comments(ifile);
	gfx->Height = get_dec(ifile);

	gfx->Data = (int*)malloc(gfx->Width*gfx->Height*sizeof(int));
	if(!gfx->Data) {
		if(stereograph_verbose) printf("FAILED;\n"); else fprintf(stderr, "reading gfx data...FAILED;\n");
		fprintf(stderr, "error allocating memory for image dimensions of %i * %i pixels in 32 bit\n", gfx->Width, gfx->Height);
		return GFX_ERROR_MEMORY_PROBLEM;
	}

	skip_space_n_comments(ifile);
	c_max = 255 / get_dec(ifile);

	if(check_header[1] == '3')
		for(z = 0; z < (gfx->Width * gfx->Height); z++) {
			while(isspace(a = fgetc(ifile))); ungetc(a, ifile);
			gfx->Data[z] = get_dec(ifile) * c_max;
			while(isspace(a = fgetc(ifile))); ungetc(a, ifile);
			gfx->Data[z] += (get_dec(ifile) * c_max) << 8;
			while(isspace(a = fgetc(ifile))); ungetc(a, ifile);
			gfx->Data[z] += (get_dec(ifile) * c_max) << 16;
		}
	else if (check_header[1] == '6') {
		while(isspace(a = fgetc(ifile))); ungetc(a, ifile);
		for(z = 0; z < (gfx->Width * gfx->Height); z++) {
			gfx->Data[z] = fgetc(ifile) * c_max;
			gfx->Data[z] += (fgetc(ifile) * c_max) << 8;
			gfx->Data[z] += (fgetc(ifile) * c_max) << 16;
		}
	} else {
		if (stereograph_verbose) printf("FAILED\n");
		fprintf(stderr, "unknown PPM magic!\n");
		return GFX_ERROR_UNSUPP_FORMAT;
	}

	return 0;

}


int GFX_Read_PNG (FILE *ifile, unsigned char *check_header, struct GFX_DATA *gfx)
{
	png_voidp user_error_ptr = NULL;
	png_structp png_ptr;
	png_infop info_ptr;
	png_infop end_info;
	int bit_depth, color_type, interlace_type, interlace_passes;

	//unsigned char header_check[8];
	int z;
#if __BYTE_ORDER == __BIG_ENDIAN
        int p;
        unsigned char r, g, b;
#endif
        /*
	int p;
        unsigned char r, g, b;
	 */

	/* checking png header */
	//fread(header_check, sizeof(char), 8, ifile);
        //} else {

		/* initializing */
		png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, user_error_ptr, NULL, NULL);
		if (!png_ptr) {
        		if(stereograph_verbose) printf("FAILED;\n"); else fprintf(stderr, "reading gfx data...FAILED\n");
	        	fprintf(stderr, "cannot create png read struct!\n");
			return GFX_ERROR_LIBPNG;
		}
		info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr) {
			png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        		if(stereograph_verbose) printf("FAILED;\n"); else fprintf(stderr, "reading gfx data...FAILED\n");
	        	fprintf(stderr, "cannot create png info struct!\n");
			return GFX_ERROR_LIBPNG;
		}
		end_info = png_create_info_struct(png_ptr);
		if (!end_info) {
			png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        		if(stereograph_verbose) printf("FAILED;\n"); else fprintf(stderr, "reading gfx data...FAILED\n");
	        	fprintf(stderr, "cannot create png (end) info struct!\n");
			return GFX_ERROR_LIBPNG;
		}
		
		/* libpng error handling */
		if (setjmp(png_ptr->jmpbuf)) {
			png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        		if(stereograph_verbose) printf("FAILED;\n"); else fprintf(stderr, "reading gfx data...FAILED\n");
	        	fprintf(stderr, "libpng reported an error!\n");
			return GFX_ERROR_LIBPNG;
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
		/* if (bit_depth == 16) { */
		#if __BYTE_ORDER != __BIG_ENDIAN
		png_set_swap(png_ptr);  // swap byte pairs to little endian
		#endif
		png_set_strip_16(png_ptr);
		/* } */


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

		gfx->Data = (int*)malloc( gfx->Width * gfx->Height * sizeof(int) );
		if(!gfx->Data) {
			png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
			if(stereograph_verbose) printf("FAILED;\n"); else fprintf(stderr, "reading gfx data...FAILED;\n");
			fprintf(stderr, "error allocating memory for image dimensions of %i * %i pixels in 32 bit\n", gfx->Width, gfx->Height);
			return GFX_ERROR_MEMORY_PROBLEM;
		}

		/* alternatively read it row by row, interlacing as expected */
		for(z = 0; z < (gfx->Height * interlace_passes); z++)
			png_read_row(png_ptr, (void *) (gfx->Data + ((z % gfx->Height) * gfx->Width)), NULL);
#if __BYTE_ORDER == __BIG_ENDIAN
		for(z = 0; z < (gfx->Width * gfx->Height); z++) {
		        p = gfx->Data[z];
		        r = (p>>24)&255;
		        g = (p>>16)&255;
		        b = p>>8;
			gfx->Data[z] = r + (g<<8) + (b<<16);
		}
#endif

		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		/* that's it */		
	//}
	return 0;
}

int GFX_Write_JPG (FILE *ofile, struct GFX_DATA *gfx)
{
    struct jpeg_compress_struct cinfo;
    int i, z, p, row_stride;
    JSAMPROW scanline[1];
    unsigned char *d, *buffer = NULL;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = error_exit;

    if (setjmp(jerr.setjmp_buffer)) {
	/* If we get here, the JPEG code has signaled an error.
	 * We need to clean up the JPEG object, close the output file,
	 * and return.
	 */
	jpeg_destroy_compress(&cinfo);
	return GFX_ERROR_LIBJPG;
    }
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, ofile);

    cinfo.image_width = gfx->Width;
    cinfo.image_height = gfx->Height;

    cinfo.in_color_space = JCS_RGB;
    cinfo.input_components = 3;
    buffer = malloc(gfx->Width * 3);
    if (!buffer)
        return GFX_ERROR_MEMORY_PROBLEM;
   
    jpeg_set_defaults(&cinfo);
    jpeg_start_compress(&cinfo, TRUE);
    row_stride = gfx->Width * cinfo.input_components;
    scanline[0] = buffer;
    z = 0;

    while (cinfo.next_scanline < cinfo.image_height) {
	/*
	 * If we have a greyscale or TrueColor image, just feed
	 * the raw data to the JPEG routine. Otherwise, we must
	 * build an array of RGB triples in 'buffer'.
	 */
        d = buffer;
	for (i = 0; i < gfx->Width; ++i) {
	    p = gfx->Data[z];
	    z++;
	    *d++ = p&255;
	    *d++ = (p>>8)&255;
	    *d++ = (p>>16)&255;
        }
	jpeg_write_scanlines(&cinfo, scanline, (JDIMENSION) 1);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    if (buffer)
	free(buffer);

    if (jerr.pub.num_warnings > 0) {	/* XXX */
	fprintf(stderr, "JPEG warning, image may be corrupted\n");
	longjmp(jerr.setjmp_buffer, 1);
    }
    return 0;   
}

int GFX_Write_PNG (FILE *ofile, struct GFX_DATA *gfx)
{
	png_voidp user_error_ptr = NULL;
	png_structp png_ptr;
	png_infop info_ptr;

	int z;
#if __BYTE_ORDER == __BIG_ENDIAN
        int p, h, u;
        unsigned char *ptr;
#endif

	/* initializing */
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, user_error_ptr, NULL, NULL);
	if (!png_ptr) {
       		if(stereograph_verbose) printf("FAILED;\n"); else fprintf(stderr, "writing gfx data...FAILED\n");
        	fprintf(stderr, "cannot create png write struct!\n");
		return GFX_ERROR_LIBPNG;
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
       		if(stereograph_verbose) printf("FAILED;\n"); else fprintf(stderr, "writing gfx data...FAILED\n");
        	fprintf(stderr, "cannot create png info struct!\n");
		return GFX_ERROR_LIBPNG;
	}
		
	/* libpng error handling */
	if (setjmp(png_ptr->jmpbuf)) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
       		if(stereograph_verbose) printf("FAILED;\n"); else fprintf(stderr, "writing gfx data...FAILED\n");
        	fprintf(stderr, "libpng reported an error!\n");
		return GFX_ERROR_LIBPNG;
	}
		
	/* setting up file pointer */
	png_init_io(png_ptr, ofile);

	/* setting up png info header */
	png_set_IHDR(png_ptr, info_ptr, gfx->Width, gfx->Height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	png_set_invert_alpha(png_ptr);
		
	/* let's flush the header down */
	png_write_info(png_ptr, info_ptr);

	/*
	#if __BYTE_ORDER = __LITTLE_ENDIAN
		png_set_swap(png_ptr);  // swap bytes to big endian, for little endian machines and only useful when writing 16 bits/channel;
	#endif
	*/

#if __BYTE_ORDER == __BIG_ENDIAN
        ptr = (unsigned char *)malloc(4*gfx->Width);
	for(h = 0; h < gfx->Height; h++) {
	  u = -1;
          for (z = 0; z < gfx->Width; z++) {
            p = gfx->Data[h * gfx->Width + z];
            ptr[++u] = p&255;
            ptr[++u] = (p>>8)&255;     
            ptr[++u] = (p>>16)&255;
            ptr[++u] = 0;
          }  
          png_write_row(png_ptr, (void *) ptr);
	}
        free(ptr);
#else
	for(z = 0; z < gfx->Height; z++)
                png_write_row(png_ptr, (void *) (gfx->Data + z * gfx->Width));
#endif   

	png_write_end(png_ptr, NULL);

	png_destroy_write_struct(&png_ptr, &info_ptr);
	/* that's it */		

	return 0;
}

int GFX_Read_TARGA (FILE *ifile, unsigned char *check_header, struct GFX_DATA *gfx)
{
	/* TARGA uses the Intel format for integer coding (low byte first) */

	unsigned char header[20];
	int x, y, *palette, c, l, s, z;

	/* copy the preread check header to the real header */
	memcpy(header, check_header, sizeof(char)*8);

	/* reading all header information */
	if(fread(header + 8, sizeof(char), 10, ifile) != 10) {
		if(stereograph_verbose) printf("FAILED!\n"); else fprintf(stderr, "reading gfx data...FAILED;\n");
		fprintf(stderr, "cannot read header! (file corrupt?)\n");
		return GFX_ERROR_READ_ERROR;
	}

	/* general measurements */
	gfx->Width = (int)header[12] + (int)header[13]*256;
	gfx->Height = (int)header[14] + (int)header[15]*256;
	gfx->Data = (int*)malloc(gfx->Width*gfx->Height*sizeof(int));
	if(!gfx->Data) {
		if(stereograph_verbose) printf("FAILED;\n"); else fprintf(stderr, "reading gfx data...FAILED;\n");
		fprintf(stderr, "error allocating memory for image dimensions of %i * %i pixels in 32 bit\n", gfx->Width, gfx->Height);
		fprintf(stderr, "debug header follows:\n");
		for(x = 0; x < 20; x += 4)
			fprintf(stderr, " %i, %i, %i, %i;\n", (int)header[x], (int)header[x + 1], (int)header[x + 2], (int)header[x + 3]);
		return GFX_ERROR_MEMORY_PROBLEM;
	}

	/* reading image identification field */
	for(x = 0; x < header[0]; x++)
		getc(ifile);

	/* reading palette data */
	if((header[1] != 0) && ((header[5] + (header[6] << 8)) > 0) && (((header[3] + (header[4] << 8)) + (header[5] + (header[6] << 8))) <= 256)) {
		palette = (int*)malloc(((header[3] + (header[4] << 8)) + (header[5] + (header[6] << 8))) * sizeof(int));
		for(x = header[3] + (header[4] << 8); (x < (header[5] + (header[6] << 8))); x++)
			GFX_Read_TARGA_RGB(ifile, (int)header[7], NULL, palette + x);
	} else
		palette = NULL;

	if(((header[2] == 2) && (header[16] >= 16)) || (((header[2] == 1) || header[2] == 3) && (header[16] == 8))) {
	/* type 1: 8 bit/palette, uncompressed; type 2: 16 bit++, uncompressed; type 3: 8 bit monocrome, uncompressed; */
		if(header[17] & 32) {
			for(y = 0; y < gfx->Height; y++) {
				if(header[17] & 16)
					for(x = gfx->Width - 1; x >= 0; x--)
						GFX_Read_TARGA_RGB(ifile, (int)header[16], palette, gfx->Data + (y * gfx->Width) + x);
				else
					for(x = 0; x < gfx->Width; x++)
						GFX_Read_TARGA_RGB(ifile, (int)header[16], palette, gfx->Data + (y * gfx->Width) + x);
			}
		} else {
			for(y = gfx->Height - 1; y >= 0; y--) {
				if(header[17] & 16)
					for(x = gfx->Width - 1; x >= 0; x--)
						GFX_Read_TARGA_RGB(ifile, (int)header[16], palette, gfx->Data + (y * gfx->Width) + x);
				else
					for(x = 0; x < gfx->Width; x++)
						GFX_Read_TARGA_RGB(ifile, (int)header[16], palette, gfx->Data + (y * gfx->Width) + x);
			}
		}
		/*if(stereograph_verbose) printf("gfx data read (%i*%i);\n", gfx->Width, gfx->Height);*/
		free(palette);
		return 0;
	} else if(((header[2] == 10) && (header[16] >= 16)) || (((header[2] == 9) || header[2] == 11) && (header[16] == 8))) {
	/* type 9: 8 bit/palette RLE; type 10: 16 bit++, RLE; type 11: 8 bit monocrome, RLE; */
		for(z = 0; ((l = getc(ifile)) != EOF) && (z < (gfx->Height * gfx->Width)); ) {
			if(l & 128) { /* compressed data follows */
				l &= 127; /* extracting length */
				GFX_Read_TARGA_RGB(ifile, (int)header[16], palette, &c);
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
						GFX_Read_TARGA_RGB(ifile, (int)header[16], palette, gfx->Data + (y * gfx->Width) + x);
					else
						fprintf(stderr, "(%i, %i) => error\n", x, y);
					z++;
				}
			}
		}
		/*if(stereograph_verbose) printf("gfx data read (%i*%i);\n", gfx->Width, gfx->Height);*/
		free(palette);
		return 0;
	} else {
		if(stereograph_verbose) printf("FAILED;\n"); else fprintf(stderr, "reading gfx data...FAILED;\n");
		fprintf(stderr, "Unsupported Targa Data Format %i@%i;\nOnly TARGA types 1, 2, 3 (uncompressed) and 9, 10, 11 (RLE) are supported;\n", header[2], header[16]);
		free(palette);
		return GFX_ERROR_UNSUPP_FORMAT;
	}
}


int GFX_Read_TARGA_RGB(FILE *ifile, int bits, int *palette, int *c) {
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
			g = (int)((255.0/31.0) * (float)(((z1 & 3) << 3) | ((z2 & 224) / 32)) );  /* 224 = 128 + 64 + 32 */
			b = (int)((255.0/31.0) * (float)(z2 & 31) );
#if __BYTE_ORDER == __BIG_ENDIAN	   
			(*c) = (r << 16) + (g << 8) + b;
#else	   
			(*c) = r + (g << 8) + (b << 16);
#endif	   
			a = z1 & 128;
		} else {
			r = getc(ifile);
			g = getc(ifile);
			b = getc(ifile);
#if __BYTE_ORDER == __BIG_ENDIAN	   
			(*c) = (r << 16) + (g << 8) + b;
#else	   
			(*c) = r + (g << 8) + (b << 16);
#endif	   
			if(bits == 32)
				a = getc(ifile);
		}
	return a; /* attribute (alpha/transparency information, 32 bit only) */
}

/* writes all gfx data to stdout, preceded by the resolution of gfx data - everything 32 bit wide */
int GFX_Write_PPM (FILE *ofile, struct GFX_DATA *gfx)
{
	int a, x, y;

	fprintf(ofile, "P6\n");
	fprintf(ofile, "%i %i\n", gfx->Width, gfx->Height);
	fprintf(ofile, "255\n");
	a = 0;
	for(y = 0; (y < gfx->Height) && (a != EOF); y++)
		for(x = 0; (x < gfx->Width) && (a != EOF); x++)	{
			putc(gfx->Data[y * gfx->Width + x] & 255, ofile);
			putc((gfx->Data[y * gfx->Width + x] >> 8) & 255, ofile);
			a = putc((gfx->Data[y * gfx->Width + x] >> 16) & 255, ofile);
		}
	if(a != EOF) {
		return 0;
	} else {
		if(stereograph_verbose) printf("FAILED;\n"); else fprintf(stderr, "writing gfx data...FAILED;\n");
			fprintf(stderr, "cannot write to file! (disk full?)\n");
		return GFX_ERROR_WRITE_ERROR;
	}
}

int GFX_Write_TARGA (FILE *ofile, struct GFX_DATA *gfx)
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
	putc((gfx->Width >> 8) & 255, ofile);
	/* height */
	putc(gfx->Height & 255, ofile);
	putc((gfx->Height >> 8) & 255, ofile);
	/* bits per pixel */
	putc(32-8, ofile);
	/* Image Descriptor Flags */
	putc(0, ofile);
	fwrite("created by stereograph", sizeof(char), strlen("created by stereograph"), ofile);
	/* data follows */
	a = 0;
	for(y = gfx->Height - 1; (y >= 0) && (a != EOF); y--)
		for(x = 0; (x < gfx->Width) && (a != EOF); x++)	{
#if __BYTE_ORDER == __BIG_ENDIAN		   
			putc((gfx->Data[y * gfx->Width + x] >>16) & 255, ofile);
			putc((gfx->Data[y * gfx->Width + x] >> 8) & 255, ofile);
			a = putc(gfx->Data[y * gfx->Width + x] & 255, ofile);
#else		   
			putc(gfx->Data[y * gfx->Width + x] & 255, ofile);
			putc((gfx->Data[y * gfx->Width + x] >> 8) & 255, ofile);
			a = putc((gfx->Data[y * gfx->Width + x] >> 16) & 255, ofile);
#endif		   
		}
	if(a != EOF) {
		return 0;
	} else {
		if(stereograph_verbose) printf("FAILED;\n"); else fprintf(stderr, "writing gfx data...FAILED;\n");
			fprintf(stderr, "cannot write to file! (disk full?)\n");
		return GFX_ERROR_WRITE_ERROR;
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
			gfx->Data[z] += (int)(datafield[0][z] * (rand()&255)) << 8;
			gfx->Data[z] += (int)(datafield[1][z] * (rand()&255)) << 16;
		} else if (datafield[3][z] > b) {
			gfx->Data[z] = (int)(datafield[0][z] * 255.0);
			gfx->Data[z] += (int)(datafield[1][z] * 255.0) << 8;
			gfx->Data[z] += (int)(datafield[2][z] * 255.0) << 16;
		} else {
			gfx->Data[z] = (int)(datafield[0][z] * datafield[4][z] * 255.0);
			gfx->Data[z] += (int)(datafield[1][z] * datafield[4][z] * 255.0) << 8;
			gfx->Data[z] += (int)(datafield[2][z]  * datafield[4][z] * 255.0) << 16;
			gfx->Data[z] += (int)(datafield[1][z] * (1.0 - datafield[4][z]) * 255.0);
			gfx->Data[z] += (int)(datafield[2][z] * (1.0 - datafield[4][z]) * 255.0) << 8;
			gfx->Data[z] += (int)(datafield[0][z]  * (1.0 - datafield[4][z]) * 255.0) << 16;
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


int GFX_Generate_SinlineTexture(struct GFX_DATA *gfx)
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
		if (stereograph_verbose) printf("FAILED\n");
		fprintf(stderr, "could not allocate memory;\n");
		return GFX_ERROR_MEMORY_PROBLEM;
	}

	return 0;
}

#ifdef X11GUI

Bool evpred(d, e, a)
Display *              d;
XEvent *               e;
XPointer               a;
{
        return (True);
}

int GFX_Write_X11 (FILE * ofile, struct GFX_DATA *gfx)
{
        static char *gfx_msg[GFX_IO_C+1] = {
	   "Hide menu",
	   "Save as .jpg",
	   "Save as .png",	     
	   "Save as .ppm",
	   "Save as .tga",
	   "Edit in xpaint"
	};
        char name[256];
	Display *dpy;
        Window root, win;
        XImage * image;
        Pixmap pixmap;
        Visual * visual;
        Colormap cmap;
        GC gc;
        int scr;
        int bigendian;
        int p, x, y, z, t;
        int depth, color_pad;
        int do_print = 0;
        int i, j;
        unsigned char *ind = NULL;
        unsigned char r, g, b;
        Pixel black, white;
        XGCValues gcv;
        XFontStruct *font;
        XEvent ev;
        XColor xc;
        Atom wm_delete_window, wm_protocols;
        KeySym keysym;
        char                    buffer[1];

	dpy = XOpenDisplay(NULL);
        if (dpy == (Display *)NULL) {
                fprintf(stderr, "X11: can't open display\n");
                exit(-1);
        }
        bigendian = (ImageByteOrder(dpy) == MSBFirst);
        scr = DefaultScreen(dpy);
        visual = DefaultVisual(dpy, scr);
        root = RootWindow(dpy, scr);

        black = BlackPixel(dpy, scr);
        white = WhitePixel(dpy, scr);
        font = XLoadQueryFont(dpy, "7x13");
   
	gcv.background = black;
	gcv.foreground = white;
        gc = XCreateGC(dpy, root, GCForeground | GCBackground, &gcv);
        XSetFont(dpy, gc, font->fid);

        depth = DefaultDepth(dpy, scr);
        if (depth > 16)
            color_pad = 32;
        else if (depth > 8)
            color_pad = 16;
        else
            color_pad = 8;
        
        wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
        wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

        win = XCreateSimpleWindow(dpy, root,
			    50, 50, gfx->Width, gfx->Height, 0,
		            white, black);
        pixmap = XCreatePixmap(dpy, root, gfx->Width, gfx->Height, depth);
        image = XCreateImage(dpy, visual, depth,
                    ZPixmap, 0, NULL, 
                    gfx->Width, gfx->Height, color_pad, 0);
        z = 0;
        image->data = malloc(gfx->Height*image->bytes_per_line);
   
        if (depth<=8) {
            cmap = XCreateColormap(dpy, root, visual, AllocNone);
	    ind = (char *) malloc(252);
            xc.flags = DoRed | DoGreen | DoBlue;
	    for (b = 0; b < 6; b++)
	    for (g = 0; g < 7; g++)
	    for (r = 0; r < 6; r++) {
	        x = b*42 + g*6 + r;
	        xc.red = (r*51)<<8;
	        xc.green = (g*42+g/2)<<8;
	        xc.blue = (b*51)<<8;
	        XAllocColor(dpy, cmap, &xc);
	        ind[x] = xc.pixel;
	    }
	    black = ind[0];
	    white = ind[251];
	    XSetWindowColormap(dpy, win, cmap);
	}

        if (depth == 16 && visual->green_mask==992) depth = 15;
        if (depth > 16)
        for (y = 0; y < gfx->Height; y++)
	  for (x = 0; x < gfx->Width; x++) {
	      p = gfx->Data[z];
              t = y * image->bytes_per_line + x * 4;
              if (bigendian) {
                image->data[t+1] = p&255;	     
                image->data[t+2] = (p>>8)&255;
                image->data[t+3] = (p>>16)&255;
	      } else {
              image->data[t] = (p>>16)&255;
              image->data[t+1] = (p>>8)&255;
              image->data[t+2] = p&255;
	      }
              z++;
	  }
        if (depth == 16)
        for (y = 0; y < gfx->Height; y++)
	  for (x = 0; x < gfx->Width; x++) {
	      p = gfx->Data[z];
              t = y * image->bytes_per_line + x * 2;
	      r = p&255;
	      g = (p>>8)&255;
	      b = (p>>16)&255;
              if (bigendian) {	     
                image->data[t+1] = (b&248)>>3 | (g&28)<<3;
                image->data[t] = (g&224)>>5 | (r&248);
	      } else {
                image->data[t] = (b&248)>>3 | (g&28)<<3;
                image->data[t+1] = (g&224)>>5 | (r&248);
	      }
              z++;
	  }
        if (depth == 15)
        for (y = 0; y < gfx->Height; y++)
	  for (x = 0; x < gfx->Width; x++) {
	      p = gfx->Data[z];
              t = y * image->bytes_per_line + x * 2;
	      r = p&255;
	      g = (p>>8)&255;
	      b = (p>>16)&255;
              if (bigendian) {
                image->data[t+1] = (b&248)>>3 | (g&56)<<2;
                image->data[t] = (g&192)>>6 | (r&248)>>1;
	      } else {
                image->data[t] = (b&248)>>3 | (g&56)<<2;
                image->data[t+1] = (g&192)>>6 | (r&248)>>1;
	      }
              z++;
	  }
        if (depth <= 8)
        for (y = 0; y < gfx->Height; y++)
	  for (x = 0; x < gfx->Width; x++) {
	      p = gfx->Data[z];
              t = y * image->bytes_per_line + x;
	      r = p&255;
	      g = (p>>8)&255;
	      b = (p>>16)&255;
              image->data[t] = ind[((b+24)/51)*42+((2*g+41)/85)*6+(r+24)/51];
              z++;
	  }
        XStoreName(dpy, win, "Stereograph");
	XSetWMProtocols(dpy, win, &wm_delete_window, 1);
        XSelectInput(dpy, win, ButtonPressMask | ButtonReleaseMask |
		               KeyReleaseMask | ExposureMask);
        XPutImage(dpy, pixmap, gc, image, 0, 0, 0, 0, gfx->Width, gfx->Height);
        XDestroyImage(image);
        XSetWindowBackgroundPixmap(dpy, win, pixmap);
        XMapRaised(dpy, win);
	XFlush(dpy);

        for (;;) {
	    if (XCheckIfEvent(dpy, &ev, evpred, (XPointer)0)) {
                if (ev.type == ClientMessage &&
                    ev.xclient.message_type == wm_protocols &&
                    ev.xclient.format == 32 &&
                    ev.xclient.data.l[0] == wm_delete_window) {
		finish:
		    XDestroyWindow(dpy, win);
		    XFreePixmap(dpy, pixmap);
                    XCloseDisplay(dpy);
	            exit(0);
		}
                if (ev.type == ButtonPress) {
		    if (!do_print) do_print = 1;
		    else
		    if (do_print) do_print = 2;
		}
		if (ev.type == ButtonRelease && do_print) {
		    if (do_print == 1) 
		        do_print = 2;
		    else {
		        j = -1;
		        for (i=0; i<=GFX_IO_C; i++)
			    if (ev.xbutton.x>10 && ev.xbutton.x<130 &&
			        ev.xbutton.y>30*i+10 && ev.xbutton.y<30*i+30)
			        j = i;
		        if (j == 0) do_print = 0;
		        else
			if (j>0 && j<GFX_IO_C) {
			    sprintf(name, "Saving into : stereo%s", gfx_msg[j]+8);
		            XSetForeground(dpy, gc, black);
		            XFillRectangle(dpy, win, gc, 0, 
					   gfx->Height-30, 190, 30);
		            XSetForeground(dpy, gc, white);
			    XDrawString(dpy, win, gc, 10, gfx->Height-10,
					name, strlen(name));
			    XFlush(dpy);
			    GFX_Write_File(name+14, j, gfx);			   
		        }
			if (j==GFX_IO_C) {
			    GFX_Write_File("stereo.png", GFX_IO_PNG, gfx);
			    sprintf(name, "xpaint stereo.png ; rm -f stereo.png &");
			    system(name);
			}
		        if (do_print) continue;			   
		    }
		}
		if (ev.type == KeyRelease) {
                    XLookupString((XKeyEvent *) &ev, buffer, 1, &keysym, NULL);
		    if (keysym == XK_p || keysym == XK_P)
		        do_print = 1;
		    if (keysym == XK_q || keysym == XK_Q ||
                        keysym == XK_Escape) goto finish;
		}
	        if (do_print) {
		   XSetForeground(dpy, gc, black);
		   XFillRectangle(dpy, win, gc, 0, 0, 140, 190);
		   XSetForeground(dpy, gc, white);				  
		   for (i=0; i<=GFX_IO_C; i++) {
		       XDrawRectangle(dpy, win, gc, 10, 30*i+10, 120, 20);
		       XDrawString(dpy, win, gc, 20, 30*i+24,
				   gfx_msg[i], strlen(gfx_msg[i]));
		   }
		   
		} else
		   XClearWindow(dpy, win);
	   } else
	     usleep(50000);
	}
        exit(-1);
}

#endif /* X11GUI */
