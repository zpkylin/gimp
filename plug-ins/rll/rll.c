/*
 * rll plug-in 
 * Loads/saves rll files from gimp.
 * I think that _DO_FILM_ should not be set since gimp can do
 * r&h film display correctly.
 *
 *  
 * Contents:
 *
 *   main()                      - Main entry - just call gimp_main()...
 *   query()                     - Respond to a plug-in query...
 *   run()                       - Run the plug-in...
 *   load_image()                - Load an rll file into gimp.
 *   save_image()                - Save gimp image to an rll file.
 */
#include <stdtypes.h>
#include <stdmacros.h>
#include <stddefs.h>
#include <buf.h>
#include <sys/stat.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include <glib.h>
#include "libgimp/gimp.h"

/* local data */
#define ONE_FLOAT16 16256
#define HALF_FLOAT16 16128
#define ZERO_FLOAT16 0

guint16 f16_shorts_in[2] = {0,0};
const gfloat *f16_float_in = (gfloat *)&f16_shorts_in;
gfloat f16_float_out;
const guint16 *f16_shorts_out = (guint16 *)&f16_float_out;

#define FLT(x) (*f16_shorts_in = (x), *f16_float_in)
#define FLT16(x) (f16_float_out = (x), *f16_shorts_out) 

/* local functions */
static void   query      ( void);
static void   run        (char    *name,
    int      nparams,
    GParam  *param,
    int     *nreturn_vals,
    GParam **return_vals);
static gint32 load_image (char   *filename);
static gint   save_image (char   *filename,
    gint32  image_ID,
    gint32  drawable_ID);
/* rll float data to gimp data routines*/
static unsigned short floatToShort( 
    float in 
				  );
static unsigned short floatToChar( 
    float in 
				 );

/* gimp data to rll float data routines*/ 
static float charToFloat(
    unsigned char in
			);
static float shortToFloat(
    unsigned short in
			 );

/* Check R&H film env variable */
static int isFilmMode(
    void
		     );
GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

int main (int argc, char *argv[]) 
{ 
  return gimp_main (argc, argv); 
}

#if 0
/* This needs to be globally available. */
CHAR    *IM_Extns[] = { ".rll", ".rla", ".rlb", ".jpg", ".jpl",
      ".tga", ".vst", ".gif", ".arc", ".rlf",
      ".mpr", ".cin", ".dpt", ".buf", ".cuf",
      ".jpeg", ".rgb", ".s16", ".yuv",
      ".qtl", ".tim", ".ppm" };
      INT     IM_NumExtns;
#endif

static void abbrev(STRING *s, int j)
{
  int i;
  int num = strlen(*s);
  for(i=0; i<num-1; i++){
	(*s)[i] = (*s)[i+1];
  }
  (*s)[num-1] = '\0';

}

static void
query ()
{
  static GParamDef load_args[] =
    {
      { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
	{ PARAM_STRING, "filename", "The name of the file to load" },
	  { PARAM_STRING, "raw_filename", "The name of the file to load" },
    };
  static GParamDef load_return_vals[] =
    {
      { PARAM_IMAGE, "image", "Output image" },
    };
  static int nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static int nload_return_vals = sizeof (load_return_vals) / sizeof (load_return_vals[0]);

  static GParamDef save_args[] =
    {
      { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
	{ PARAM_IMAGE, "image", "Input image" },
	  { PARAM_DRAWABLE, "drawable", "Drawable to save" },
	    { PARAM_STRING, "filename", "The name of the file to save the image in" },
	      { PARAM_STRING, "raw_filename", "The name of the file to save the image in" },
    };
  static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  STRING      load_routine_str;
  STRING      load_does_what_str;
  STRING      load_does_what_sentence_str;
  STRING      load_menu_str;

  STRING      save_routine_str;
  STRING      save_does_what_str;
  STRING      save_does_what_sentence_str;
  STRING      save_menu_str;
  int i;
  STRING      ext;

  i = 0; 
  while ( i < IM_NumExtns)
    {
      strcpy (ext, IM_Extns[i]);
      abbrev (&ext, 1); 
      sprintf(load_routine_str, "file_%s_load", ext);
      sprintf(load_does_what_str, "loads %s file format", ext);
      sprintf(load_does_what_sentence_str, 
	  "This loads %s format using r & h software", ext);
      sprintf(load_menu_str, "<Load>/%s", ext);

#if 0
      printf("%s\n", load_routine_str);
      printf("%s\n", load_does_what_str);
      printf("%s\n", load_does_what_sentence_str);
      printf("%s\n", load_menu_str);
#endif

      gimp_install_procedure (load_routine_str,
		      load_does_what_str, 
		      load_does_what_sentence_str,
		      "Calvin Williamson",
		      "Calvin Williamson",
	  "1998",
	  load_menu_str,
	  NULL,
	  PROC_PLUG_IN,
	  nload_args, nload_return_vals,
	  load_args, load_return_vals);

      sprintf(save_routine_str, "file_%s_save", ext);
      sprintf(save_does_what_str, "saves %s file format", ext);
      sprintf(save_does_what_sentence_str, 
	  "This saves %s format using r & h software", ext);
      sprintf(save_menu_str, "<Save>/%s", ext);

      gimp_install_procedure (save_routine_str,
	  save_does_what_str,
	  save_does_what_sentence_str,
	  "Calvin Williamson",
	  "Calvin Williamson",
	  "1998",
	  save_menu_str,
	  "FLOAT16_RGB*, FLOAT16_GRAY*",
	  PROC_PLUG_IN,
	  nsave_args, 0,
	  save_args, NULL);

      gimp_register_magic_load_handler (load_routine_str, ext, "", "20,string,GIMP");
      gimp_register_save_handler (save_routine_str, ext, "");
      i++;
    }
}

static void
run (char    *name,
    int      nparams,
    GParam  *param,
    int     *nreturn_vals,
    GParam **return_vals)
{
      	static GParam values[2];
  GRunModeType run_mode;
  gint32 image_ID;
  GStatusType status = STATUS_SUCCESS;
  STRING      ext;
  STRING      load_routine_str;
  STRING      save_routine_str;
  int i;


  run_mode = (GRunModeType) param[0].data.d_int32;

  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;
  values[1].type = PARAM_IMAGE;
  values[1].data.d_image = -1;


  i = 0; 
  while ( i < IM_NumExtns)
    {
      strcpy (ext, IM_Extns[i]);
      abbrev (&ext, 1); 

      sprintf(load_routine_str, "file_%s_load", ext);
      sprintf(save_routine_str, "file_%s_save", ext);

      if (strcmp (name, load_routine_str) == 0)
	{
	  image_ID = load_image (param[1].data.d_string);

	  if (image_ID != -1) 
	    {
	      values[0].data.d_status = STATUS_SUCCESS;
	      values[1].data.d_image = image_ID;
	    } 
	  else 
	    {
	      values[0].data.d_status = STATUS_EXECUTION_ERROR;
	    }
	  *nreturn_vals = 2;
	  return;
	}
      else if (strcmp (name, save_routine_str) == 0)
	{
	  switch (run_mode) 
	    {
	    case RUN_INTERACTIVE:
	      /* If you need an extra dialog for info put it up here 
		 --see the other plugin examples*/
	      break;
	    case RUN_NONINTERACTIVE:
	      if (nparams != 5)
		status = STATUS_CALLING_ERROR;
	      break;
	    case RUN_WITH_LAST_VALS:
	      break;
	    }

	  if (save_image (param[3].data.d_string, 
		param[1].data.d_int32,
		param[2].data.d_int32)) 
	    values[0].data.d_status = STATUS_SUCCESS;
	  else
	    values[0].data.d_status = STATUS_EXECUTION_ERROR;
	  *nreturn_vals = 1;
	  return;
	}
      i++;
    }
}


static gint32 load_image (char *filename) 
{
  char            *temp;
  gint32          image_ID, layer_ID, channel_ID;
  GImageType      image_type; 
  GDrawableType   layer_type; 
  GDrawable       *drawable;
  GDrawable       *chan_drawable;
  GPixelRgn       *pixel_rgn;
  GPixelRgn       *chan_pixel_rgn;
  guchar          *b = NULL;
  guchar          **buffer = NULL;
  guchar          *aux_buffer1=NULL, **aux_buffer=NULL;
  guchar          **bptr=NULL;
  guchar          **aux_bptr=NULL;
  guint16         *sptr;
  guint16         *aux_sptr;
  gfloat          floatVal;
  gint            y;
  gint            i;
  gint            j;
  gint            k;
  gint            width;
  gint            height;
  gint            rowbytes;
  gint            aux_rowbytes;
  gint            yend;
  gint            strip_height;
  gint            row;
  struct stat     statbuf;
  gint		  alpha=0;

  gint            auxChans=0, imageChans=0, nlayers=0, matteChans=0; 

  /* Rhythm and Hues file stuff */
  WF_I_FILE               *filehandle = 0;
  WF_I_HEAD               rllHead;
  char                    **result;

  /* debug stuff */
#if  0 /*DBX_DEBUG*/
  getchar();
  printf("Load image!!!\n");
#endif
#ifdef _DOFILM_
  int filmMode = isFilmMode();
#endif

  /* Put up a progress bar */
  temp = (char*) g_malloc(strlen (filename) + 11);
  sprintf(temp, "Loading %s:", filename);
  gimp_progress_init(temp);
  g_free (temp);

  /* Check for the existence of the file */
  if(stat(filename, &statbuf) == -1){
	g_warning("rll load_image: cant find file \"%s\"\n", filename); 	
	return -1;
  }

  /*Use Rhythm and Hues routines to open and read rll image file */
  filehandle = IM_OpenFFile ( filename, &rllHead, "r", TRUE ); 

  if(!filehandle){
	g_warning("rll load_image: cant open file \"%s\"\n", filename); 	
	return -1;
  }

  /* Set the image type */
  if(!filehandle->numChans){
	g_warning("rll load_image: rll has no channels\n");
	IM_CloseFile(filehandle);
	return -1;
  }

  imageChans = filehandle->header->imageChans;
  if (imageChans)
    nlayers = 1; 
  if(imageChans>3){
	auxChans   = imageChans - 3;
	imageChans = 3;
  }                
  matteChans = filehandle->header->matteChans;
  auxChans  += filehandle->header->auxChans;
 
  if (auxChans && strcmp("fur", filehandle->header->program))
    {
      result = (char**) IM_GetAuxChannelNames (&rllHead);
      if(result == NULL){
	    g_warning("rll load_image: problems with read aux names\n");
	    IM_CloseFile(filehandle);
	    return -1;
      }
      
      for (i=0; i<auxChans; i++)
	{
	  switch (imageChans)
	    {
	    case 1:
	      if (!strcmp("gray", result[i+1]))
		nlayers ++;
	      else 
		if (!strcmp("gimpAlpha", result[i+1]))
                  alpha = 1; 
	      else
		i=auxChans;	
	      break;
	    case 3:
	      if (!strcmp("red", result[i+1]))
		{

		  i += 2;
		  nlayers ++;
		}
	      else 
		if (!strcmp("gimpAlpha", result[i+1]))
		  {
                  alpha = 1;
		  } 
	      else
		i = auxChans;
	      break;
	    }
	}
    }
  auxChans -= (nlayers-1) * imageChans;

#ifndef FLOAT16_AUX
#define FLOAT16_AUX FLOAT16_GRAY
#endif
#ifndef FLOAT16_AUX_IMAGE
#define FLOAT16_AUX_IMAGE FLOAT16_GRAY_IMAGE
#endif
#ifndef FLOAT16_TWO
#define FLOAT16_TWO FLOAT16_RGB
#endif
#ifndef FLOAT16_TWO_IMAGE
#define FLOAT16_TWO_IMAGE FLOAT16_RGB_IMAGE
#endif

  switch(imageChans){
      case 0:
	image_type = FLOAT16_AUX;
	layer_type = FLOAT16_AUX_IMAGE;
	break;
      case 1:
	image_type = FLOAT16_GRAY;
	layer_type = FLOAT16_GRAY_IMAGE;
	break;
      case 2:
	image_type = FLOAT16_TWO;
	layer_type = FLOAT16_TWO_IMAGE;
	break;
      case 3:
	if(!filehandle->header->matteChans){
	      image_type = FLOAT16_RGB;
	      layer_type = FLOAT16_RGB_IMAGE;
	}else{
	      image_type = FLOAT16_RGB;
	      layer_type = FLOAT16_RGB_IMAGE;
	}
	break;
  } 

  /* set with and height */
  width  = filehandle->across;
  height = filehandle->down;

  /* Create gimp IMAGE */
  image_ID = gimp_image_new(width, height, image_type);
  gimp_image_set_filename(image_ID, filename);



  /* Create gimp LAYER */
  if(!strcmp("fur", filehandle->header->program))
    {
      pixel_rgn = (GPixelRgn*) g_malloc(sizeof(GPixelRgn)); 
      layer_ID = gimp_layer_new(image_ID, "fur", width, height,
	  layer_type, 100, NORMAL_MODE);
      gimp_image_add_layer(image_ID, layer_ID, 0); 
      drawable = gimp_drawable_get(layer_ID);
      gimp_pixel_rgn_init(&(pixel_rgn[0]), drawable,
	  0, 0, drawable->width, drawable->height,
	  TRUE, FALSE);
    }
  else
    {
      pixel_rgn = (GPixelRgn*) g_malloc(sizeof(GPixelRgn) * nlayers); 
      layer_ID = gimp_layer_new(image_ID, "Background", width, height,
	  layer_type, 100, NORMAL_MODE);
      gimp_image_add_layer(image_ID, layer_ID, nlayers-1);
      if (alpha)
	gimp_layer_add_alpha (layer_ID); 
      drawable = gimp_drawable_get(layer_ID);
      gimp_pixel_rgn_init(&(pixel_rgn[0]), drawable, 
	  0, 0, drawable->width, drawable->height, 
	  TRUE, FALSE);
      for (i=1; i<nlayers; i++)
	{
	  layer_ID = gimp_layer_new(image_ID, "layer", width, height,
	      layer_type, 100, NORMAL_MODE);
	  gimp_layer_add_alpha (layer_ID); 
	  if (alpha)
	    gimp_image_add_layer(image_ID, layer_ID, nlayers-1-i);
	  drawable = gimp_drawable_get(layer_ID);
	  gimp_pixel_rgn_init(&(pixel_rgn[i]), drawable, 
	      0, 0, drawable->width, drawable->height, 
	      TRUE, FALSE);

	}
    }


  /* create gimp CHANNELS */
  chan_pixel_rgn = (GPixelRgn*) g_malloc(sizeof(GPixelRgn) * (matteChans + auxChans));
  if(!strcmp("fur", filehandle->header->program)){
	result = (char**) IM_GetAuxChannelNames (&rllHead);
	if(result == NULL){
	      g_warning("rll load_image: problems with read aux names\n");
	      IM_CloseFile(filehandle);
	      return -1;
	}
	for(i=0; i<auxChans+1; i++)
	      
	for(i=0; i<auxChans; i++){
	      guchar color[3] = {0, 0, 255};
	      layer_ID = gimp_layer_new(image_ID, result[i+1], width, height,
		  layer_type, 100, NORMAL_MODE);
	      gimp_image_add_layer(image_ID, layer_ID, 0);
	      chan_drawable = gimp_drawable_get(layer_ID);
	      gimp_pixel_rgn_init(&(chan_pixel_rgn[i]), chan_drawable, 0, 0, 
		  chan_drawable->width, chan_drawable->height, 
		  TRUE, FALSE);
	}
  }
  else
    {
      for(i=0; i<matteChans; i++){
	    guchar color[3] = {0, 0, 255};
	    channel_ID = gimp_channel_new(image_ID, "Matte", width, height,
		100, color );
	    gimp_image_add_channel(image_ID, channel_ID, 0);
	    gimp_image_set_active_channel(image_ID, -1);
	    gimp_channel_set_visible(channel_ID, 0);
	    chan_drawable = gimp_drawable_get(channel_ID);
	    gimp_pixel_rgn_init(&(chan_pixel_rgn[i]), chan_drawable, 0, 0,
		chan_drawable->width, chan_drawable->height,
		TRUE, FALSE);
      }
      for(i=matteChans; i<auxChans+matteChans; i++){
	    guchar color[3] = {0, 0, 255};
	    channel_ID = gimp_channel_new(image_ID, "aux", width, height,
		100, color );
	    gimp_image_add_channel(image_ID, channel_ID, 0);
	    gimp_image_set_active_channel(image_ID, -1);
	    gimp_channel_set_visible(channel_ID, 0);
	    chan_drawable = gimp_drawable_get(channel_ID);
	    gimp_pixel_rgn_init(&(chan_pixel_rgn[i]), chan_drawable, 0, 0,
		chan_drawable->width, chan_drawable->height,
		TRUE, FALSE);
      }
    }
  /* Allocate buffer for a strip of the intensity channels */
  strip_height = gimp_tile_height();
  if (imageChans && nlayers)
    {
      rowbytes     = drawable->width * drawable->bpp;
      b            = (guchar*) g_malloc(strip_height * rowbytes * nlayers);
      buffer       = (guchar**) g_malloc(sizeof (guchar*) * nlayers);
      if(!buffer){
	    g_warning("rll load_image: couldnt allocate scanline strip buffer\n");
	    IM_CloseFile(filehandle);	  
	    return -1;
      }
      for(i=0; i<nlayers; i++)
	buffer[i] = &b[i*strip_height * rowbytes];
      bptr = (guchar**) g_malloc(nlayers*sizeof(guchar*));
    }

  /* Allocate buffer for a strip of the aux chan */
  if (auxChans + matteChans) {
	aux_rowbytes = chan_drawable->width * chan_drawable->bpp; 
	aux_buffer1 = (guchar*) g_malloc((matteChans + auxChans)*sizeof(guchar)*strip_height * aux_rowbytes);
	if(!aux_buffer1){
	      g_warning("rll load_image: couldnt allocate matte scanline strip buffer\n");
	      g_free(buffer);
	      IM_CloseFile(filehandle);
	      return -1;
	}
	aux_buffer  = (guchar**) g_malloc((matteChans + auxChans)*sizeof(guchar*));
	if(!aux_buffer){
	      g_warning("rll load_image: couldnt allocate matte scanline strip buffer\n");
	      g_free(aux_buffer);
	      g_free(buffer);
	      IM_CloseFile(filehandle);
	      return -1;
	}

	aux_bptr = (guchar**) g_malloc((matteChans + auxChans)*sizeof(guchar*));
	if(!aux_bptr){
	      g_warning("rll load_image: couldnt allocate matte scanline strip buffer\n");
	      g_free(aux_buffer1); 
	      g_free(aux_buffer);
	      g_free(buffer);
	      IM_CloseFile(filehandle);
	      return -1;
	}

	aux_rowbytes = chan_drawable->width * chan_drawable->bpp;
  }

  for(i=0; i<auxChans+matteChans; i++)
    aux_buffer[i] = &aux_buffer1[i*strip_height * aux_rowbytes];
printf ("2\n");

  /* Read strips of the image in order bottom to top and give them to gimp */	
  for(y=0; y<height; y=yend){
	yend = y + strip_height;
	yend = MIN(yend, height);
	for (row=y; row<yend; row++){
	      /* rlls are ordered from bottom to top */
	      if(!IM_ReadScan(filehandle, row)){
#ifdef _DOFILM_
		    /* check for film mode */
		    if(filmMode)
		      for(i=0; i<filehandle->numChans; i++)
			IM_DoUnFilmConversion(filehandle->rBufPtr[i],
			    filehandle->across);
#endif

		    /* set data pointer to reverse the order of scanlines in the strip */
		    for(i=0; i<nlayers && imageChans; i++)  
		      bptr[i] = buffer[i] + (yend - (row+1)) * rowbytes ;

		    /* set data pointer to reverse the order of scanlines in the matte strip */
		    for(i=0; i<auxChans+matteChans; i++)  
		      aux_bptr[i] = aux_buffer[i] + (yend - (row+1)) * aux_rowbytes ;

		    /* convert to unsigned shorts */
		    for(j=0; j<width; j++){
			  for (k=0; k<1/*nlayers*/; k++){
			  for(i=0; i<imageChans; i++){ 
				floatVal = filehandle->rBufPtr[i+(k*imageChans)][j];
				sptr = (guint16 *)&floatVal;
				*((guint16 *)(bptr[k])) = sptr[0];
				bptr[k] += sizeof(guint16);
			  }
			  if (alpha)
			    {
				floatVal = 1;
				sptr = (guint16 *)&floatVal;
				*((guint16 *)(bptr[0])) = sptr[0];
				bptr[0] += sizeof(guint16);
			    }
			  }

			  for(i=0; i<matteChans; i++){
				floatVal = filehandle->rBufPtr[imageChans+i][j];
				aux_sptr = (guint16 *)&floatVal;
				*((guint16 *)(aux_bptr[i])) = aux_sptr[0];
				aux_bptr[i] += sizeof(guint16);
			  }
			  for (k=1; k<nlayers; k++) {
			  for(i=0; i<imageChans+alpha; i++){
				/*
				   if (i != imageChans)
				*/
				   {	  
				floatVal = filehandle->rBufPtr[i+(k*imageChans)+matteChans][j];
				sptr = (guint16 *)&floatVal;
				*((guint16 *)(bptr[k])) = sptr[0];
				bptr[k] += sizeof(guint16);
			  }
			  }
			  }

			  for(i=0; i<auxChans; i++){
				floatVal = filehandle->rBufPtr[(imageChans*nlayers+matteChans)+i][j];
				aux_sptr = (guint16 *)&floatVal;
				*((guint16 *)(aux_bptr[i+matteChans])) = aux_sptr[0];
				aux_bptr[i+matteChans] += sizeof(guint16);
			  }

		    }
	      } 
	      else{
		    g_warning("rll load_image couldnt read scanline %d from rll\n", row);
		    IM_CloseFile (filehandle);
		    g_free(buffer);
		    if(aux_buffer){ 
			  g_free(aux_buffer);
			  g_free(aux_buffer1);
			  g_free(aux_bptr);
		    }
		    return -1;
	      }
	}

	/* Send the strip to gimp and update progress */

	/*  Gimp images are ordered top to bottom, so this strips rectangle (x,y,w,h) is 
	    given by (0, height - yend, width, yend -y). 
	 */   
        for(i=0; i<nlayers; i++)	
	  gimp_pixel_rgn_set_rect (&(pixel_rgn[i]), buffer[i], 
	      0, height-yend , width, yend - y);
	for(i=0; i<auxChans+matteChans; i++)
	  gimp_pixel_rgn_set_rect (&(chan_pixel_rgn[i]), aux_buffer[i], 
	      0, height-yend , width, yend - y);
	gimp_progress_update ((double) row / (double) height);
  }

  /* flush the drawable and clean up */	
  gimp_drawable_flush(drawable);
  IM_CloseFile (filehandle);
  g_free(buffer);
  if(aux_buffer){
	g_free(aux_buffer);
	g_free(aux_buffer1);
	g_free(aux_bptr);
	g_free(chan_pixel_rgn); 
  }

  return image_ID;
}


static gint save_image (
    char *filename, 
    gint32 image_ID, 
    gint32 drawable_ID
		       ) 
{
  guchar          *fg_b=NULL, **fg_buffer=NULL;
  guchar          *bg_b=NULL, **bg_buffer=NULL;
  guchar          *aux_buffer1=NULL, **aux_buffer=NULL;
  GDrawable       *fg_drawable=NULL;
  GDrawable       *bg_drawable=NULL;
  GDrawable       *chan_drawable=NULL;
  gint            y;
  gint            i;
  gint            j;
  gint            k;
  GPixelRgn       *fg_pixel_rgn=NULL;
  GPixelRgn       *bg_pixel_rgn=NULL;
  GPixelRgn       *chan_pixel_rgn=NULL;
  char            *temp=NULL;
  guchar          **fg_bptr=NULL;
  guint16         **fg_sptr=NULL;
  gfloat          **fg_fptr=NULL;
  guchar          **bg_bptr=NULL;
  guint16         **bg_sptr=NULL;
  gfloat          **bg_fptr=NULL;
  guchar          **aux_bptr=NULL;
  guint16         **aux_sptr=NULL;
  gfloat          **aux_fptr=NULL;
  char            fmtString[256];
  int	          bytes_per_channel;
  gint            width;
  gint            height;
  gint            yend;
  gint            row;
  gint            fg_rowbytes, bg_rowbytes;
  gint            strip_height;
  gint32          *channels=NULL;
  gint32          *layers=NULL;
  gint            fg_nlayers, bg_nlayers, nlayers; 
  gint            nchannels;
  gint            aux_rowbytes;
  gint            channel_size;
  char            *bg_name=NULL;
  gint		  fg_bpp=0, bg_bpp=0;
  gint            fg_cpp=0, bg_cpp=0;

  /* Rhythm and Hues file stuff */
  WF_I_HEAD       header;
  WF_I_FILE*      filehandle;
  char            *a_n=NULL, **aux_name=NULL; 
  int		aux_name_tsize=300, aux_name_size=30; 

#ifdef _DOFILM_
  int   filmMode = isFilmMode();
#endif

  /* Put up a progress bar */
  temp = (char*) g_malloc(strlen (filename) + 10);
  sprintf(temp, "Saving %s:", filename);
  gimp_progress_init(temp);
  g_free(temp);


  strip_height = gimp_tile_height();

  channels = gimp_image_get_channels (image_ID, &nchannels);
  layers = gimp_image_get_layers (image_ID, &nlayers);
  bg_name = gimp_layer_get_name(layers[nlayers-1]);

  bg_nlayers = nlayers ? 1 : 0;
  fg_nlayers = nlayers > 1 ? nlayers-1 : 0;

  /* Get the drawable from gimp and init the region we need */
  bg_pixel_rgn = (GPixelRgn*) g_malloc(sizeof(GPixelRgn) * bg_nlayers);
  if (!bg_pixel_rgn && bg_nlayers)
    {
      g_warning("rll save_image: couldnt allocate\n");
      return -1;
    }
  fg_pixel_rgn = (GPixelRgn*) g_malloc(sizeof(GPixelRgn) * fg_nlayers);
  if (!fg_pixel_rgn && fg_nlayers)
    {
      g_warning("rll save_image: couldnt allocate\n");
      if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
      return -1;
    }
  for(i=0; i<bg_nlayers; i++)
    {
      bg_drawable = gimp_drawable_get(layers[nlayers-1]);
      gimp_pixel_rgn_init(&(bg_pixel_rgn[i]), bg_drawable, 0, 0, bg_drawable->width,
	  bg_drawable->height, FALSE, FALSE);
      bg_bpp = bg_drawable->bpp;
    }
  for(i=0; i<fg_nlayers; i++)
    {
      fg_drawable = gimp_drawable_get(layers[nlayers-2-i]);
      gimp_pixel_rgn_init(&(fg_pixel_rgn[i]), fg_drawable, 0, 0, fg_drawable->width,
	  fg_drawable->height, FALSE, FALSE);
      fg_bpp = fg_drawable->bpp;
    }

  /* Stuff the rll header with information.*/
  sprintf(fmtString, "f%dx%d", bg_drawable->width, bg_drawable->height);
  printf ("%d %d\n", bg_drawable->width, bg_drawable->height); 
  if (!strcmp(bg_name, "fur")) 
  {
	  if (IM_StuffHeader(&header, fmtString, "fur", "RLL") == -1)
	  {
		  g_warning("rll save_image: couldnt allocate\n");
		  if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
		  if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
		  return -1;
	  }
  }
  else if (IM_StuffHeader(&header, fmtString, "gimp", "RLL") == -1)
  {
	  g_warning("rll save_image: couldnt allocate\n");
	  if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
	  if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
	  return -1;
  }


  scanf ("%d"); 

  /* Figure out some header info and 
     channel depth from gimp drawable type */	
  switch (gimp_drawable_type(drawable_ID))
    {
      /* 8bit gimp types */
    case GRAY_IMAGE:
      header.imageChans = 1;
      header.matteChans = 0;
      bytes_per_channel = 1;
      fg_cpp = fg_nlayers ? fg_bpp / bytes_per_channel : 0; 
      bg_cpp = bg_nlayers ? bg_bpp / bytes_per_channel : 0; 
      header.auxChans   = fg_cpp * (fg_nlayers) + (nchannels - header.matteChans); 
      break;
    case GRAYA_IMAGE:
      header.imageChans = 1;
      header.matteChans = 1;
      bytes_per_channel = 1;
      fg_cpp = fg_nlayers ? fg_bpp / bytes_per_channel : 0; 
      bg_cpp = bg_nlayers ? bg_bpp / bytes_per_channel : 0; 
      header.auxChans   = fg_cpp * (fg_nlayers) + (nchannels - header.matteChans); 
      break;
    case RGB_IMAGE:
      header.imageChans = 3;
      header.matteChans = 0;
      bytes_per_channel = 1;
      fg_cpp = fg_nlayers ? fg_bpp / bytes_per_channel : 0; 
      bg_cpp = bg_nlayers ? bg_bpp / bytes_per_channel : 0; 
      header.auxChans   = fg_cpp * (fg_nlayers) + (nchannels - header.matteChans); 
      break;
    case RGBA_IMAGE:
      header.imageChans = 3;
      header.matteChans = 1;
      bytes_per_channel = 1;
      fg_cpp = fg_nlayers ? fg_bpp / bytes_per_channel : 0; 
      bg_cpp = bg_nlayers ? bg_bpp / bytes_per_channel : 0; 
      header.auxChans   = fg_cpp * (fg_nlayers) + (nchannels - header.matteChans); 
      break;

      /* 16bit gimp types */
    case U16_GRAY_IMAGE:
      header.imageChans = 1;
      header.matteChans = 0;
      bytes_per_channel = 2;
      fg_cpp = fg_nlayers ? fg_bpp / bytes_per_channel : 0; 
      bg_cpp = bg_nlayers ? bg_bpp / bytes_per_channel : 0; 
      header.auxChans   = fg_cpp * (fg_nlayers) + (nchannels - header.matteChans); 
      break;
    case U16_GRAYA_IMAGE:
      header.imageChans = 1;
      header.matteChans = 1;
      bytes_per_channel = 2;
      fg_cpp = fg_nlayers ? fg_bpp / bytes_per_channel : 0; 
      bg_cpp = bg_nlayers ? bg_bpp / bytes_per_channel : 0; 
      header.auxChans   = fg_cpp * (fg_nlayers) + (nchannels - header.matteChans); 
      break;
    case U16_RGB_IMAGE:
      header.imageChans = 3;
      header.matteChans = 0;
      bytes_per_channel = 2;
      fg_cpp = fg_nlayers ? fg_bpp / bytes_per_channel : 0; 
      bg_cpp = bg_nlayers ? bg_bpp / bytes_per_channel : 0; 
      header.auxChans   = fg_cpp * (fg_nlayers) + (nchannels - header.matteChans); 
      break;
    case U16_RGBA_IMAGE:
      header.imageChans = 3;
      header.matteChans = 1;
      bytes_per_channel = 2;
      fg_cpp = fg_nlayers ? fg_bpp / bytes_per_channel : 0; 
      bg_cpp = bg_nlayers ? bg_bpp / bytes_per_channel : 0; 
      header.auxChans   = fg_cpp * (fg_nlayers) + (nchannels - header.matteChans); 
      break;

      /* float gimp types */
    case FLOAT_GRAY_IMAGE:
      header.imageChans = 1;
      header.matteChans = 0;
      bytes_per_channel = 4;
      fg_cpp = fg_nlayers ? fg_bpp / bytes_per_channel : 0; 
      bg_cpp = bg_nlayers ? bg_bpp / bytes_per_channel : 0; 
      header.auxChans   = fg_cpp * (fg_nlayers) + (nchannels - header.matteChans); 
      break;
    case FLOAT_GRAYA_IMAGE:
      header.imageChans = 1;
      header.matteChans = 1;
      bytes_per_channel = 4;
      fg_cpp = fg_nlayers ? fg_bpp / bytes_per_channel : 0; 
      bg_cpp = bg_nlayers ? bg_bpp / bytes_per_channel : 0; 
      header.auxChans   = fg_cpp * (fg_nlayers) + (nchannels - header.matteChans); 
      break;
    case FLOAT_RGB_IMAGE:
      header.imageChans = 3;
      header.matteChans = 0;
      bytes_per_channel = 4;
      fg_cpp = fg_nlayers ? fg_bpp / bytes_per_channel : 0; 
      bg_cpp = bg_nlayers ? bg_bpp / bytes_per_channel : 0; 
      header.auxChans   = fg_cpp * (fg_nlayers) + (nchannels - header.matteChans); 
      break;
    case FLOAT_RGBA_IMAGE:
      header.imageChans = 3;
      header.matteChans = 1;
      bytes_per_channel = 4;
      fg_cpp = fg_nlayers ? fg_bpp / bytes_per_channel : 0; 
      bg_cpp = bg_nlayers ? bg_bpp / bytes_per_channel : 0; 
      header.auxChans   = fg_cpp * (fg_nlayers) + (nchannels - header.matteChans); 
      break;

      /* float16 gimp types */
    case FLOAT16_GRAY_IMAGE:
      if(strcmp("fur", bg_name)){
	    if(!nchannels){
		  header.imageChans = 1;
		  header.matteChans = 0;
		  bytes_per_channel = 2;
		  fg_cpp = fg_nlayers ? fg_bpp / bytes_per_channel : 0; 
		  bg_cpp = bg_nlayers ? bg_bpp / bytes_per_channel : 0; 
		  header.auxChans   = fg_cpp * (fg_nlayers) + (nchannels - header.matteChans); 
	    } else {
		  header.imageChans = 1;
		  header.matteChans = 1;
		  bytes_per_channel = 2;
		  fg_cpp = fg_nlayers ? fg_bpp / bytes_per_channel : 0; 
		  bg_cpp = bg_nlayers ? bg_bpp / bytes_per_channel : 0; 
		  header.auxChans   = fg_cpp * (fg_nlayers) + (nchannels - header.matteChans); 
	    }
      }else{

	    header.imageChans = 0;
	    header.matteChans = 0;
	    header.auxChans   = fg_nlayers;
	    bytes_per_channel = 2;
	    fg_cpp = fg_nlayers ? fg_bpp / bytes_per_channel : 0; 
	    bg_cpp = bg_nlayers ? bg_bpp / bytes_per_channel : 0; 
      }
      break;
    case FLOAT16_GRAYA_IMAGE:
      header.imageChans = 1;
      header.matteChans = 1;
      bytes_per_channel = 2;
      fg_cpp = fg_nlayers ? fg_bpp / bytes_per_channel : 0; 
      bg_cpp = bg_nlayers ? bg_bpp / bytes_per_channel : 0; 
      header.auxChans   = fg_cpp * (fg_nlayers) + (nchannels - header.matteChans); 
      break;
    case FLOAT16_RGB_IMAGE:
      if(!nchannels){
	    header.imageChans = 3;
	    header.matteChans = 0;
	    bytes_per_channel = 2;
	    fg_cpp = fg_nlayers ? fg_bpp / bytes_per_channel : 0; 
	    bg_cpp = bg_nlayers ? bg_bpp / bytes_per_channel : 0; 
	    header.auxChans   = fg_cpp * (fg_nlayers) + (nchannels - header.matteChans); 
      }else{
	    header.imageChans = 3;
	    header.matteChans = 1;
	    bytes_per_channel = 2;
	    fg_cpp = fg_nlayers ? fg_bpp / bytes_per_channel : 0; 
	    bg_cpp = bg_nlayers ? bg_bpp / bytes_per_channel : 0; 
	    header.auxChans   = fg_cpp * (fg_nlayers) + (nchannels - header.matteChans); 
      } 
      break;

    case FLOAT16_RGBA_IMAGE:
      if(!nchannels){
	    header.imageChans = 3;
	    header.matteChans = 0;
	    bytes_per_channel = 2; 
	    fg_cpp = fg_nlayers ? fg_bpp / bytes_per_channel : 0; 
	    bg_cpp = bg_nlayers ? bg_bpp / bytes_per_channel : 0; 
	    header.auxChans   = fg_cpp * (fg_nlayers) + (nchannels - header.matteChans);
      }
      else
	{
	  header.imageChans = 3;
	  header.matteChans = 1;
	  bytes_per_channel = 2;
	  fg_cpp = fg_nlayers ? fg_bpp / bytes_per_channel : 0; 
	  bg_cpp = bg_nlayers ? bg_bpp / bytes_per_channel : 0; 
	  header.auxChans   = fg_cpp * (fg_nlayers) + (nchannels - header.matteChans); 
	}
      break;

    default:
      g_print("rll save_image: cant save this gimp drawable type\n");
      return -1;
    } 

#if 1	
  printf("type is %d %d %d %d\n", gimp_drawable_type(drawable_ID), fg_cpp, bg_cpp, bytes_per_channel);
  printf("imagechans is %d\n", header.imageChans);
  printf("mattechans is %d\n", header.matteChans);
  printf("mattechans is %d\n", header.auxChans);
#endif

  aux_name_tsize = aux_name_size * (1+header.auxChans); 
  /* store the fur channel names*/
  if(!strcmp(bg_name, "fur")){
	a_n = (char*) g_malloc(sizeof(char)*aux_name_tsize);
	if (!a_n)
	  {
	    g_warning("rll save_image: couldnt allocate\n");
	    if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
	    if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
	    return -1;
	  }
	aux_name = (char**) g_malloc(sizeof(char*)*(1+header.auxChans));
	if (!aux_name)
	  {
	    g_warning("rll save_image: couldnt allocate\n");
	    if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
	    if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
	    if (a_n) g_free (a_n); 
	    return -1;
	  }
	for(i=0; i<header.auxChans+1; i++)
	  aux_name[i] = &a_n[i*aux_name_size];
	strcpy(aux_name[0], "fur"); 
	for(i=0; i<header.auxChans; i++){
	      strcpy(aux_name[i+1], gimp_layer_get_name(layers[i]));
	} 
	IM_SetAuxChannelIds(&header, header.auxChans, aux_name); 	
	free(aux_name);
	free(a_n);
	bg_nlayers = 0; 	
  } 

  /* store the aux and extra layer names */
  if(header.auxChans && strcmp(bg_name, "fur")){
	aux_name_tsize = aux_name_size * (header.auxChans + 1); 
	a_n = (char*) g_malloc(sizeof(char)*aux_name_tsize);
	if (!a_n)
	  {
	    g_warning("rll save_image: couldnt allocate\n");
	    if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
	    if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
	    return -1;
	  }
	aux_name = (char**) g_malloc(sizeof(char*)*(header.auxChans+1));
	if (!aux_name)
	  {
	    g_warning("rll save_image: couldnt allocate\n");
	    if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
	    if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
	    if (a_n) g_free (a_n); 
	    return -1;
	  }
	for(i=0; i<header.auxChans+1; i++)
	  aux_name[i] = &a_n[i*aux_name_size];
	strcpy(aux_name[0], "gimp"); 
	for(i=1; i<(fg_nlayers)*fg_cpp+1; i++){
	      switch (fg_cpp)
		{
		case 4:
		  strcpy(aux_name[i], strdup("red"));
		  i ++;
		  strcpy(aux_name[i], strdup("green"));
		  i ++;
		  strcpy(aux_name[i], strdup("blue"));
		  i ++;
		  strcpy(aux_name[i], strdup("gimpAlpha"));
		  break;
		case 3: 
		  strcpy(aux_name[i], strdup("red"));
		  i ++;
		  strcpy(aux_name[i], strdup("green"));
		  i ++;
		  strcpy(aux_name[i], strdup("blue"));
		  break;
		case 2:
		  strcpy(aux_name[i], strdup("gray"));
		  i ++;
		  strcpy(aux_name[i], strdup("gimpAlpha"));
		  break; 
		case 1:
		  strcpy(aux_name[i], strdup("gray"));
		  break; 
		}
	}
	for(i=(fg_nlayers)*fg_cpp+1; i<header.auxChans+1; i++)
	  {
	    strcpy(aux_name[i], "aux"); 
	  }
	if (IM_SetAuxChannelIds(&header, header.auxChans, aux_name) == -1)
	  {
	    g_warning("rll save_image: couldnt allocate\n");
	    if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
	    if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
	    if (a_n) g_free (a_n); 
	    if (aux_name) g_free(aux_name);
	    return -1;
	  }

	free(aux_name);
	free(a_n); 
  }

  if (nchannels) {
	chan_pixel_rgn = (GPixelRgn*) g_malloc(sizeof(GPixelRgn) * nchannels);
	if (!chan_pixel_rgn)
	  {
	    g_warning("rll save_image: couldnt allocate\n");
	    if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
	    if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
	    if (a_n) g_free (a_n); 
	    if (aux_name) g_free(aux_name);
	    return -1;
	  }

	for(i=0; i<nchannels; i++){
	      /* guess the zeroth auxiliary channel is our matte */
	      chan_drawable = gimp_drawable_get(channels[i]);
	      gimp_pixel_rgn_init(&(chan_pixel_rgn[i]), chan_drawable, 0, 0, chan_drawable->width,
		  chan_drawable->height, FALSE, FALSE);
	}
	aux_rowbytes = chan_drawable->width * chan_drawable->bpp; 
	aux_buffer1 = (guchar*) g_malloc(nchannels*sizeof(guchar)*strip_height * aux_rowbytes);

	if(!aux_buffer1)
	  {
	    g_warning("rll load_image: couldnt allocate matte scanline strip buffer\n");
	    if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
	    if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
	    if (a_n) g_free (a_n); 
	    if (aux_name) g_free(aux_name);
	    if (chan_pixel_rgn) g_free (chan_pixel_rgn);
	    return -1;
	  }
	aux_buffer  = (guchar**) g_malloc(nchannels*sizeof(guchar*));
	if(!aux_buffer)
	  {
	    g_warning("rll load_image: couldnt allocate matte scanline strip buffer\n");
	    if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
	    if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
	    if (a_n) g_free (a_n); 
	    if (aux_name) g_free(aux_name);
	    if (chan_pixel_rgn) g_free (chan_pixel_rgn);
	    if (aux_buffer1) g_free (aux_buffer1);
	    return -1;
	  }

	for(i=0; i<nchannels; i++)
	  aux_buffer[i] = &(aux_buffer1[i * strip_height * aux_rowbytes]);
	aux_bptr = (guchar**) g_malloc(nchannels*sizeof(guchar*));
	if (!aux_bptr)
	  {
	    g_warning("rll load_image: couldnt allocate matte scanline strip buffer\n");
	    if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
	    if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
	    if (a_n) g_free (a_n); 
	    if (aux_name) g_free(aux_name);
	    if (chan_pixel_rgn) g_free (chan_pixel_rgn);
	    if (aux_buffer1) g_free (aux_buffer1);
	    if (aux_buffer) g_free (aux_buffer);
	    return -1;
	  }

	aux_sptr = (guint16**) g_malloc(nchannels * sizeof(guint16*));
	if (!aux_sptr)
	  {
	    g_warning("rll load_image: couldnt allocate matte scanline strip buffer\n");
	    if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
	    if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
	    if (a_n) g_free (a_n); 
	    if (aux_name) g_free(aux_name);
	    if (chan_pixel_rgn) g_free (chan_pixel_rgn);
	    if (aux_buffer1) g_free (aux_buffer1);
	    if (aux_buffer) g_free (aux_buffer);
	    if (aux_bptr) g_free (aux_bptr);
	    return -1;
	  }
	aux_fptr = (gfloat**) g_malloc(nchannels * sizeof(gfloat*));
	if (!aux_fptr)
	  {
	    g_warning("rll load_image: couldnt allocate matte scanline strip buffer\n");
	    if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
	    if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
	    if (a_n) g_free (a_n); 
	    if (aux_name) g_free(aux_name);
	    if (chan_pixel_rgn) g_free (chan_pixel_rgn);
	    if (aux_buffer1) g_free (aux_buffer1);
	    if (aux_buffer) g_free (aux_buffer);
	    if (aux_sptr) g_free (aux_sptr);
	    if (aux_bptr) g_free (aux_bptr);
	    return -1;
	  }
  }

  scanf ("%d");

  printf ("LALA \n"); 
  filehandle = IM_OpenFFile ( filename, &header, "w", FALSE );
  printf ("LALAL\n"); 
  if(!filehandle)
    {
      g_warning("rll save_image: cant open file %s\n", filename); 	
      if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
      if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
      if (a_n) g_free (a_n); 
      if (aux_name) g_free(aux_name);
      if (chan_pixel_rgn) g_free (chan_pixel_rgn);
      if (aux_buffer1) g_free (aux_buffer1);
      if (aux_buffer) g_free (aux_buffer);
      if (aux_sptr) g_free (aux_sptr);
      if (aux_bptr) g_free (aux_bptr);
      if (aux_fptr) g_free (aux_fptr);
      return -1;
    } 


  if (fg_nlayers)
    {
      fg_rowbytes = fg_drawable->width * fg_bpp;
      fg_b = (guchar*) g_malloc(fg_rowbytes * strip_height * fg_nlayers);
      if(!fg_b)
	{
	  g_warning("rll save_image: cant allocate scanline strip buffer\n"); 	
	  if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
	  if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
	  if (a_n) g_free (a_n); 
	  if (aux_name) g_free(aux_name);
	  if (chan_pixel_rgn) g_free (chan_pixel_rgn);
	  if (aux_buffer1) g_free (aux_buffer1);
	  if (aux_buffer) g_free (aux_buffer);
	  if (aux_sptr) g_free (aux_sptr);
	  if (aux_bptr) g_free (aux_bptr);
	  if (aux_fptr) g_free (aux_fptr);
	  IM_CloseFile (filehandle);
	  return -1;
	}
      fg_buffer = (guchar**) g_malloc(sizeof (guchar*) * fg_nlayers);
      if(!fg_buffer)
	{
	  g_warning("rll save_image: cant allocate scanline strip buffer\n"); 	
	  if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
	  if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
	  if (a_n) g_free (a_n); 
	  if (aux_name) g_free(aux_name);
	  if (chan_pixel_rgn) g_free (chan_pixel_rgn);
	  if (aux_buffer1) g_free (aux_buffer1);
	  if (aux_buffer) g_free (aux_buffer);
	  if (aux_sptr) g_free (aux_sptr);
	  if (aux_bptr) g_free (aux_bptr);
	  if (aux_fptr) g_free (aux_fptr);
	  IM_CloseFile (filehandle);
	  if (fg_b) g_free (fg_b);
	  return -1;
	}
      for(i=0; i<fg_nlayers; i++)
	fg_buffer[i] = &fg_b[i*fg_rowbytes * strip_height];
    }

  if (bg_nlayers)
    { 
      bg_rowbytes = bg_drawable->width * bg_bpp;
      bg_b = (guchar*) g_malloc(bg_rowbytes * strip_height * bg_nlayers);
      if(!bg_b)
	{
	  g_warning("rll save_image: cant allocate scanline strip buffer\n"); 	
	  if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
	  if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
	  if (a_n) g_free (a_n); 
	  if (aux_name) g_free(aux_name);
	  if (chan_pixel_rgn) g_free (chan_pixel_rgn);
	  if (aux_buffer1) g_free (aux_buffer1);
	  if (aux_buffer) g_free (aux_buffer);
	  if (aux_sptr) g_free (aux_sptr);
	  if (aux_bptr) g_free (aux_bptr);
	  if (aux_fptr) g_free (aux_fptr);
	  IM_CloseFile (filehandle);
	  if (fg_b) g_free (fg_b);
	  if (fg_buffer) g_free (fg_buffer);
	  return -1;
	}
      bg_buffer = (guchar**) g_malloc(sizeof (guchar*) * bg_nlayers);
      if(!bg_buffer)
	{
	  g_warning("rll save_image: cant allocate scanline strip buffer\n"); 	
	  if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
	  if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
	  if (a_n) g_free (a_n); 
	  if (aux_name) g_free(aux_name);
	  if (chan_pixel_rgn) g_free (chan_pixel_rgn);
	  if (aux_buffer1) g_free (aux_buffer1);
	  if (aux_buffer) g_free (aux_buffer);
	  if (aux_sptr) g_free (aux_sptr);
	  if (aux_bptr) g_free (aux_bptr);
	  if (aux_fptr) g_free (aux_fptr);
	  IM_CloseFile (filehandle);
	  if (fg_b) g_free (fg_b);
	  if (fg_buffer) g_free (fg_buffer);
	  if (bg_b) g_free (bg_b);
	  return -1;
	}
      for(i=0; i<bg_nlayers; i++)
	bg_buffer[i] = &bg_b[i * bg_rowbytes * strip_height];

    }

  fg_bptr = (guchar**) g_malloc (fg_nlayers * sizeof(guchar*));
  if(!fg_bptr && fg_nlayers)
    {
      g_warning("rll save_image: cant allocate scanline strip buffer\n"); 	
      if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
      if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
      if (a_n) g_free (a_n); 
      if (aux_name) g_free(aux_name);
      if (chan_pixel_rgn) g_free (chan_pixel_rgn);
      if (aux_buffer1) g_free (aux_buffer1);
      if (aux_buffer) g_free (aux_buffer);
      if (aux_sptr) g_free (aux_sptr);
      if (aux_bptr) g_free (aux_bptr);
      if (aux_fptr) g_free (aux_fptr);
      IM_CloseFile (filehandle);
      if (fg_b) g_free (fg_b);
      if (fg_buffer) g_free (fg_buffer);
      if (bg_b) g_free (bg_b);
      if (bg_buffer) g_free (bg_buffer);
      return -1;
    }
  fg_sptr = (guint16**) g_malloc (fg_nlayers * sizeof(guint16*));
  if(!fg_sptr && fg_nlayers)
    {
      g_warning("rll save_image: cant allocate scanline strip buffer\n"); 	
      if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
      if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
      if (a_n) g_free (a_n); 
      if (aux_name) g_free(aux_name);
      if (chan_pixel_rgn) g_free (chan_pixel_rgn);
      if (aux_buffer1) g_free (aux_buffer1);
      if (aux_buffer) g_free (aux_buffer);
      if (aux_sptr) g_free (aux_sptr);
      if (aux_bptr) g_free (aux_bptr);
      if (aux_fptr) g_free (aux_fptr);
      IM_CloseFile (filehandle);
      if (fg_b) g_free (fg_b);
      if (fg_buffer) g_free (fg_buffer);
      if (bg_b) g_free (bg_b);
      if (bg_buffer) g_free (bg_buffer);
      if (fg_bptr) g_free (fg_bptr);
      return -1;
    }
  fg_fptr = (gfloat**) g_malloc (fg_nlayers * sizeof(gfloat*));
  if(!fg_fptr && fg_nlayers)
    {
      g_warning("rll save_image: cant allocate scanline strip buffer\n"); 	
      if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
      if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
      if (a_n) g_free (a_n); 
      if (aux_name) g_free(aux_name);
      if (chan_pixel_rgn) g_free (chan_pixel_rgn);
      if (aux_buffer1) g_free (aux_buffer1);
      if (aux_buffer) g_free (aux_buffer);
      if (aux_sptr) g_free (aux_sptr);
      if (aux_bptr) g_free (aux_bptr);
      if (aux_fptr) g_free (aux_fptr);
      IM_CloseFile (filehandle);
      if (fg_b) g_free (fg_b);
      if (fg_buffer) g_free (fg_buffer);
      if (bg_b) g_free (bg_b);
      if (bg_buffer) g_free (bg_buffer);
      if (fg_bptr) g_free (fg_bptr);
      if (fg_sptr) g_free (fg_sptr);
      return -1;
    }

  bg_bptr = (guchar**) g_malloc (bg_nlayers * sizeof(guchar*));
  if(!bg_bptr && bg_nlayers)
    {
      g_warning("rll save_image: cant allocate scanline strip buffer\n"); 	
      if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
      if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
      if (a_n) g_free (a_n); 
      if (aux_name) g_free(aux_name);
      if (chan_pixel_rgn) g_free (chan_pixel_rgn);
      if (aux_buffer1) g_free (aux_buffer1);
      if (aux_buffer) g_free (aux_buffer);
      if (aux_sptr) g_free (aux_sptr);
      if (aux_bptr) g_free (aux_bptr);
      if (aux_fptr) g_free (aux_fptr);
      IM_CloseFile (filehandle);
      if (fg_b) g_free (fg_b);
      if (fg_buffer) g_free (fg_buffer);
      if (bg_b) g_free (bg_b);
      if (bg_buffer) g_free (bg_buffer);
      if (fg_bptr) g_free (fg_bptr);
      if (fg_sptr) g_free (fg_sptr);
      if (fg_fptr) g_free (fg_fptr);
      return -1;
    }
  bg_sptr = (guint16**) g_malloc (bg_nlayers * sizeof(guint16*));
  if(!bg_sptr && bg_nlayers)
    {
      g_warning("rll save_image: cant allocate scanline strip buffer\n"); 	
      if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
      if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
      if (a_n) g_free (a_n); 
      if (aux_name) g_free(aux_name);
      if (chan_pixel_rgn) g_free (chan_pixel_rgn);
      if (aux_buffer1) g_free (aux_buffer1);
      if (aux_buffer) g_free (aux_buffer);
      if (aux_sptr) g_free (aux_sptr);
      if (aux_bptr) g_free (aux_bptr);
      if (aux_fptr) g_free (aux_fptr);
      IM_CloseFile (filehandle);
      if (fg_b) g_free (fg_b);
      if (fg_buffer) g_free (fg_buffer);
      if (bg_b) g_free (bg_b);
      if (bg_buffer) g_free (bg_buffer);
      if (fg_bptr) g_free (fg_bptr);
      if (fg_sptr) g_free (fg_sptr);
      if (fg_fptr) g_free (fg_fptr);
      if (bg_bptr) g_free (bg_bptr);
      return -1;
    }
  bg_fptr = (gfloat**) g_malloc (bg_nlayers * sizeof(gfloat*));
  if(!bg_fptr && bg_nlayers)
    {
      g_warning("rll save_image: cant allocate scanline strip buffer\n"); 	
      if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
      if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
      if (a_n) g_free (a_n); 
      if (aux_name) g_free(aux_name);
      if (chan_pixel_rgn) g_free (chan_pixel_rgn);
      if (aux_buffer1) g_free (aux_buffer1);
      if (aux_buffer) g_free (aux_buffer);
      if (aux_sptr) g_free (aux_sptr);
      if (aux_bptr) g_free (aux_bptr);
      if (aux_fptr) g_free (aux_fptr);
      IM_CloseFile (filehandle);
      if (fg_b) g_free (fg_b);
      if (fg_buffer) g_free (fg_buffer);
      if (bg_b) g_free (bg_b);
      if (bg_buffer) g_free (bg_buffer);
      if (fg_bptr) g_free (fg_bptr);
      if (fg_sptr) g_free (fg_sptr);
      if (fg_fptr) g_free (fg_fptr);
      if (bg_bptr) g_free (bg_bptr);
      if (bg_sptr) g_free (bg_sptr);
      return -1;
    }


  height = bg_drawable->height;
  width = bg_drawable->width;



  /* Get strips from gimp image moving from bottom to top */
  for(y=0; y<height; y=yend){
	yend = y + strip_height;
	yend = MIN (yend, height);

	/* Gimp images are ordered top to bottom, so the strips rectangle x,y,w,h is 
	   0, height - yend, width, yend -y 
	 */   
	for(i=0; i<bg_nlayers; i++)
	  gimp_pixel_rgn_get_rect (&(bg_pixel_rgn[i]), bg_buffer[i], 0, height - yend , width, yend - y );
	for(i=0; i<fg_nlayers; i++)
	  gimp_pixel_rgn_get_rect (&(fg_pixel_rgn[i]), fg_buffer[i], 0, height - yend , width, yend - y );
	for(i=0; i<nchannels; i++)
	  gimp_pixel_rgn_get_rect (&(chan_pixel_rgn[i]), aux_buffer[i], 
	      0, height - yend , width, yend - y );

	for (row = y; row < yend; row++){
	      /* Set the data pointers to reverse scanlines in the strip */
	      for(i=0; i<fg_nlayers; i++) 
		{
		  fg_bptr[i] = fg_buffer[i] + (yend - (row + 1)) * fg_rowbytes;
		  fg_sptr[i] = (guint16 *) fg_bptr[i];
		  fg_fptr[i] = (gfloat *) fg_bptr[i];
		}	
	      for(i=0; i<bg_nlayers; i++)
		{
		  bg_bptr[i] = bg_buffer[i] + (yend - (row + 1)) * bg_rowbytes;
		  bg_sptr[i] = (guint16 *) bg_bptr[i];
		  bg_fptr[i] = (gfloat *) bg_bptr[i];
		}	
	      for(i=0; i<nchannels; i++){
		    aux_bptr[i] = aux_buffer[i] + (yend - (row + 1)) * aux_rowbytes;
		    aux_sptr[i] = (guint16 *) aux_bptr[i];
		    aux_fptr[i] = (gfloat *) aux_bptr[i];
	      }

	      /* Translate gimp data to rll floats */
	      for(j=0; j<filehandle->across; j++){
		    /* save the first layer */
		    for(k=0; k<bg_nlayers; k++)
		      for(i=0; i<header.imageChans; i++){
			    switch(bytes_per_channel){
				case 4:
				  filehandle->rBufPtr[i+(k*header.imageChans)][j] = pow (*(bg_fptr[k])++, 2.2);
				  break;
				case 2:
				  filehandle->rBufPtr[i+(k*header.imageChans)][j] = FLT (*(bg_sptr[k])++);
				  break;
				case 1:
				  filehandle->rBufPtr[i+(k*header.imageChans)][j] = charToFloat( *(bg_bptr[k])++ );
				  break;
				default:
				  g_warning("rll save_image: unknown bytes per channel\n"); 	
				  IM_CloseFile(filehandle);
				  if (aux_buffer)
				    g_free (aux_buffer);
				  g_free(bg_buffer);
				  break;
			    }
			    if (i==header.imageChans-1 && bg_cpp != header.imageChans)
			      {
				(bg_fptr[k])++;
				(bg_sptr[k])++;
				(bg_bptr[k])++;
			      }
		      }

		    /* do the matte channel here if there is one*/
		    for(i=0; i<header.matteChans; i++){
			  switch(bytes_per_channel){
			      case 4:
				filehandle->rBufPtr[i+bg_nlayers*header.imageChans][j] = 
				  pow(*(aux_fptr[i])++, 2.2);
				break;
			      case 2:
				filehandle->rBufPtr[i+bg_nlayers*header.imageChans][j] = 
				  FLT(*(aux_sptr[i])++);
				break;
			      case 1:
				filehandle->rBufPtr[i+bg_nlayers*header.imageChans][j] = 
				  charToFloat(*(aux_bptr[i])++);
				break;
			      default:
				g_warning("rll save_image: unknown bytes per channel\n"); 	
				IM_CloseFile(filehandle);
				g_free(aux_buffer);
				break;
			  }
		    }
		    /* save the rest of the layers in the aux channels */
		    for(k=0; k<fg_nlayers; k++)
		      {
			for(i=0; i<fg_cpp; i++){
			      switch(bytes_per_channel){
				  case 4:
				    filehandle->rBufPtr[i+((k*fg_cpp)+(bg_nlayers*header.imageChans)+header.matteChans)][j] = 
				      pow (*(fg_fptr[k])++, 2.2);
				    break;
				  case 2:
				    filehandle->rBufPtr[i+((k*fg_cpp)+(bg_nlayers*header.imageChans)+header.matteChans)][j] = 
				      FLT (*(fg_sptr[k])++);
				    break;
				  case 1:
				    filehandle->rBufPtr[i+((k*fg_cpp)+(bg_nlayers*header.imageChans)+header.matteChans)][j] = 
				      charToFloat( *(fg_bptr[k])++ );
				    break;
				  default:
				    g_warning("rll save_image: unknown bytes per channel\n"); 	
				    IM_CloseFile(filehandle);
				    if (aux_buffer)
				      g_free (aux_buffer);
				    g_free(bg_buffer);
				    break;

			      }
			}


		      }
		    for(i=header.matteChans; i<nchannels; i++){
			  switch(bytes_per_channel){
			      case 4:
				filehandle->rBufPtr[i+header.imageChans*nlayers][j] = 
				  pow(*(aux_fptr[i])++, 2.2);
				break;
			      case 2:
				filehandle->rBufPtr[i+header.imageChans*nlayers][j] = 
				  FLT(*(aux_sptr[i])++);
				break;
			      case 1:
				filehandle->rBufPtr[i+header.imageChans*nlayers][j] = 
				  charToFloat(*(aux_bptr[i])++);
				break;
			      default:
				g_warning("rll save_image: unknown bytes per channel\n"); 	
				IM_CloseFile(filehandle);
				g_free(aux_buffer);
				break;
			  }
		    }

		    /* save the aux channels */	
	      }


#ifdef _DOFILM_
	      /* check for film mode */
	      if(filmMode)
		for(i=0; i<filehandle->numChans; i++)
		  IM_DoToFilmConversion(filehandle->rBufPtr[i], 
		      filehandle->across);
#endif
	      /* write the rowth scanline to the rll file */
	      if(IM_WriteScan(filehandle)){
		    g_warning("rll save_image: couldnt write scanline %d\n", row); 	
		    IM_CloseFile(filehandle);
		    if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
		    if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
		    if (a_n) g_free (a_n); 
		    if (aux_name) g_free(aux_name);
		    if (chan_pixel_rgn) g_free (chan_pixel_rgn);
		    if (aux_buffer1) g_free (aux_buffer1);
		    if (aux_buffer) g_free (aux_buffer);
		    if (aux_sptr) g_free (aux_sptr);
		    if (aux_bptr) g_free (aux_bptr);
		    if (aux_fptr) g_free (aux_fptr);
		    IM_CloseFile (filehandle);
		    if (fg_b) g_free (fg_b);
		    if (fg_buffer) g_free (fg_buffer);
		    if (bg_b) g_free (bg_b);
		    if (bg_buffer) g_free (bg_buffer);
		    if (fg_bptr) g_free (fg_bptr);
		    if (fg_sptr) g_free (fg_sptr);
		    if (fg_fptr) g_free (fg_fptr);
		    if (bg_bptr) g_free (bg_bptr);
		    if (bg_sptr) g_free (bg_sptr);
		    if (bg_fptr) g_free (bg_fptr);
		    return -1;
	      }
	}
	gimp_progress_update((double)row / (double)height);
  }

  /* clean up */	
  if (bg_pixel_rgn) g_free (bg_pixel_rgn); 
  if (fg_pixel_rgn) g_free (fg_pixel_rgn); 
  if (a_n) g_free (a_n); 
  if (aux_name) g_free(aux_name);
  if (chan_pixel_rgn) g_free (chan_pixel_rgn);
  if (aux_buffer1) g_free (aux_buffer1);
  if (aux_buffer) g_free (aux_buffer);
  if (aux_sptr) g_free (aux_sptr);
  if (aux_bptr) g_free (aux_bptr);
  if (aux_fptr) g_free (aux_fptr);
  if (fg_b) g_free (fg_b);
  if (fg_buffer) g_free (fg_buffer);
  if (bg_b) g_free (bg_b);
  if (bg_buffer) g_free (bg_buffer);
  if (fg_bptr) g_free (fg_bptr);
  if (fg_sptr) g_free (fg_sptr);
  if (fg_fptr) g_free (fg_fptr);
  if (bg_bptr) g_free (bg_bptr);
  if (bg_sptr) g_free (bg_sptr);
  if (bg_fptr) g_free (bg_fptr);

  return IM_CloseFile(filehandle);
}


  static int isFilmMode(
      void
		       )
    {

      int   filmMode = 0;
      char *filmEnv;

      /* Determine if we are a film job */
      if ( (filmEnv = getenv( "FILM" )) )
	{
	  if ( atoi( filmEnv ) != 0 )
	    filmMode = 1;
	  else
	    filmMode = 0;
	}
      return filmMode;
    }

    /*----------------------------------------------------------------------
      |                         floatToShort
     *----------------------------------------------------------------------
     |
     |  Programmer: Brian J. Green
     |
     |  Date:       Jan. 29, 1998
     |
     |  Name:
     |
     |  Function:
     |
     |  usage:
     |
     |  Return:
     |
     |  Notes:
     */
    static unsigned short floatToShort( 
	float in 
				      )
      {
	float fVal = in;
	short sp0;
	short sp1;
	unsigned short ans;

	if ( fVal > 1.0 )
	  fVal = 1.0;
	if ( fVal < 0.0 )
	  fVal = 0.0;

	/* use lookup table for speed */
	sp0 = ((short *)&fVal)[0];
	sp1 = ((short *)&fVal)[1];
	if ( sp1 & 32768 )
	  ++sp0;
	if ( sp0 < IM_SGAMMA22LOOKUP_START )
	  ans = 0;
	else if ( sp0 > IM_SGAMMA22LOOKUP_END )
	  ans = 65535;
	else
	  ans = 
	    IM_SGamma22Lookup[(sp0-IM_SGAMMA22LOOKUP_START)];

	return ans;

      }


  
    /*----------------------------------------------------------------------
      |                         floatToChar
     *----------------------------------------------------------------------
     |
     |  Programmer: Brian J. Green
     |
     |  Date:       Jan. 29, 1998
     |
     |  Name:
     |
     |  Function:
     |
     |  usage:
     |
     |  Return:
     |
     |  Notes:
     */
    static unsigned short floatToChar( 
	float in 
				     )
      {
	float ans;
	float fVal = in;

	if ( fVal > 1.0 )
	  fVal = 1.0;
	if ( fVal < 0.0 )
	  fVal = 0.0;

	/* gamma conversion */
	ans = pow( fVal, 1.0/2.2) * 255.0;
	return ( unsigned short ) ans;
      }


  
    /*----------------------------------------------------------------------
      |                         shortToFloat
     *----------------------------------------------------------------------
     |
     |  Programmer: Brian J. Green
     |
     |  Date:       Jan. 29, 1998
     |
     |  Name:
     |
     |  Function:
     |
     |  usage:
     |
     |  Return:
     |
     |  Notes:
     */
    static float shortToFloat(
	unsigned short in
			     )
      {
	/* use a lookup table for speed */
	return IM_UnSGamma22Lookup[ in ];
      }


  
    /*----------------------------------------------------------------------
      |                        charToFloat
     *----------------------------------------------------------------------
     |
     |  Programmer: Brian J. Green
     |
     |  Date:       Jan. 29, 1998
     |
     |  Name:
     |
     |  Function:
     |
     |  usage:
     |
     |  Return:
     |
     |  Notes:
     */
    static float charToFloat(
	unsigned char in
			    )
      {
	float fVal = (float) in;
	return pow( fVal / 255.0, 2.2 );
      }
