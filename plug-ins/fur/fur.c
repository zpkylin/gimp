/*
  Fur plugin. Reads .fur files and puts the channels into 
 */

#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpfloat16.h"


/* Declare local data types
 */

typedef struct
{
  int magicNumber;
  int version;
  int width;
  int height;
  int numberOfLayers;
}FurHeader;

int run_flag = 0;

/* Declare some local functions.  */
static void   query      (void);
static void   run        (char    *name,
                          int      nparams,
                          GParam  *param,
                          int     *nreturn_vals,
                          GParam **return_vals);
static gint32 load_image (char   *filename);
static gint   save_image (char   *filename,
                          gint32  image_ID,
                          gint32  drawable_ID);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

/* functions to convert 32 bit floats to 16bit floats and vice versa*/
static void 
float_to_float16 (gfloat *buffer_float, guint16 * buffer_float16, gint number);
static void 
float16_to_float (guint16 *buffer_float16, gfloat * buffer_float, gint number);
  
int main (int argc, char *argv[]) 
{ 
  return gimp_main (argc, argv); 
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

  gimp_install_procedure ("file_fur_load",
                          "loads files of the .fur file format",
                          "This loads multichannel files as white grayscale + aux channels images",
                          "Calvin Williamson",
                          "Calvin Williamson",
                          "1999",
                          "<Load>/FUR",
                          NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_install_procedure ("file_fur_save",
                          "saves files in the .fur file format",
                          "",
                          "Calvin Williamson",
                          "Calvin Williamson",
                          "1997",
                          "<Save>/FUR",
                          "GRAY*, U16_GRAY*, FLOAT_GRAY*,FLOAT16_GRAY*",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_magic_load_handler ("file_fur_load", "fur", "", "");
  gimp_register_save_handler ("file_fur_save", "fur", "");
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

  run_mode = param[0].data.d_int32;

  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;
  values[1].type = PARAM_IMAGE;
  values[1].data.d_image = -1;

  if (strcmp (name, "file_fur_load") == 0) 
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
  }
  else if (strcmp (name, "file_fur_save") == 0) 
  {
      switch (run_mode) 
      {
	case RUN_INTERACTIVE:
	case RUN_NONINTERACTIVE:
	  if (nparams != 5)
	    status = STATUS_CALLING_ERROR;
	  break;
	case RUN_WITH_LAST_VALS:
	  break;
      }

      if (save_image (param[3].data.d_string, param[1].data.d_int32,
		      param[2].data.d_int32)) 
	values[0].data.d_status = STATUS_SUCCESS;
      else
	values[0].data.d_status = STATUS_EXECUTION_ERROR;
      *nreturn_vals = 1;
  }
}


static gint32 
load_image (char *filename) 
{
	char *temp;
	int fd;
	gfloat *buffer_float = NULL;
	guint16 *buffer_float16 = NULL;
	gint32 image_ID, layer_ID, channel_ID;
	GDrawable *drawable;
	gint line;
	GPixelRgn pixel_rgn;
	FurHeader furHeader;
	gfloat progress;
	guchar color[3] = {0,0,255};
	gint i;
    	char s[255];
	long filePos;

        printf("in load_image for fur\n");

	temp = g_malloc(strlen (filename) + 11);
	sprintf(temp, "Loading %s:", filename);
	gimp_progress_init(temp);
	g_free (temp);

	fd = open(filename, O_RDONLY);
	if (fd == -1) 
	{
          return -1;
	}

#if 0
typedef struct
{
  int magicNumber;
  int version;
  int width;
  int height;
  int numberOfLayers;
}FurHeader;
#endif
	/* FIXME This is just for testing--should get this from actually reading the fur file */

        furHeader.magicNumber = 0; 
	furHeader.version = 0; 
	furHeader.width = 100; 
	furHeader.height = 100; 
	furHeader.numberOfLayers = 1;

	read(fd, &furHeader.magicNumber, sizeof(int));	
	read(fd, &furHeader.version, sizeof(int));	
	read(fd, &furHeader.width, sizeof(int));	
	read(fd, &furHeader.height, sizeof(int));	
	read(fd, &furHeader.numberOfLayers, sizeof(int));	

	printf("size: %d x %d ,  layers: %d , version: %d\n", furHeader.width, furHeader.height, furHeader.numberOfLayers, furHeader.version);

#if 0
	if (read(fd, &furHeader, sizeof(furHeader)) != sizeof(furHeader)) 
	{
	  image_ID = -1;
	  goto finish_load_image;
	}
#endif

       /*
 	* Create a new image of the proper size and associate the filename with it.
        */
	image_ID = gimp_image_new(furHeader.width, furHeader.height, FLOAT16_GRAY);
	gimp_image_set_filename(image_ID, filename);

       /*
 	* Create a Background layer for the image.
        */
	layer_ID = gimp_layer_new(image_ID, "Background", furHeader.width, furHeader.height, 
			FLOAT16_GRAY_IMAGE, 100, NORMAL_MODE);
	gimp_image_add_layer(image_ID, layer_ID, 0);

	/* Allocate a buffer for a float scanline of data from the fur file -- 4 bytes per float*/
	buffer_float = (gfloat *)g_malloc(furHeader.width * 4);

	/* Allocate a buffer for a float16 scanline of data to pass to gimp-- 2 bytes per float16*/
	buffer_float16 = (guint16 *)g_malloc(furHeader.width * 2);


	/* Loop through the grayscale data channels of fur file and load each of these
  	   into a auxiliary channel of gimp */ 

	for(i= 0; i < furHeader.numberOfLayers; i++)
	{
	    /* read the channel header */
	    int n, bits;
	    read(fd, &n, sizeof(int));
	    read(fd, &s[0], sizeof(char)*n);	
	    printf("%d: [%s]\n", i, s);

	    read(fd, &bits, sizeof(int));
	    printf("   bits = %d\n", bits);
	
	  /* create the channel */ 

	  /* FIXME note: Need code here to make correct name --instead of "FixMeName" */
	  channel_ID = gimp_channel_new (image_ID, s , furHeader.width, furHeader.height, 100, color);
	  /* channel_ID = gimp_channel_new (image_ID, "FixMeName" , furHeader.width, furHeader.height, 100, color); */
					
	  /* add it to the list of auxiliary channels */
	  gimp_image_add_channel(image_ID, channel_ID, i);

	  /* init the channel and region we'll be copying data to below */
	  drawable = gimp_drawable_get(channel_ID);
	  gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width,
			  drawable->height, TRUE, FALSE);

	  /* Loop through each scanlines of this channel -- read them into the float buffer */ 
	  /* strange thing though, we have to read them upside down */
	  filePos = lseek(fd, 0, 1); /* get the current position */
	  
	  for (line = 0; line < furHeader.height; line++)
	  {
	    lseek(fd, filePos+((furHeader.height - line - 1)*furHeader.width*4), 0);

	    if (read(fd, buffer_float, furHeader.width * 4 ) != furHeader.width * 4) 
	    {
	      image_ID = -1;
	      goto finish_load_image;
	    }

	    /* convert the float buffer to 16bit float */
	    float_to_float16 (buffer_float, buffer_float16, furHeader.width); 
	
	    /* copy the 16bit float data to the channel */
	    gimp_pixel_rgn_set_row(&pixel_rgn, buffer_float16, 0, line, furHeader.width);
	
	    progress = (furHeader.height * i + line)/(float)(furHeader.height * furHeader.numberOfLayers);

	    gimp_progress_update( progress );
	  }

	  lseek(fd, filePos+furHeader.height*furHeader.width*4, 0);
	}
	gimp_drawable_flush(drawable);

finish_load_image:
	if (buffer_float)
	  g_free(buffer_float);
	if (buffer_float16)
	  g_free(buffer_float16);
	close (fd);
	return image_ID;
}

static gint 
save_image (char *filename, gint32 image_ID, gint32 drawable_ID) 
{
	int fd;
	FurHeader furHeader;
	gfloat *buffer_float = NULL;
	guint16 *buffer_float16 = NULL;
	GDrawable *drawable;
	gint line;
	GPixelRgn pixel_rgn;
	char *temp;
	gint return_val;
	gint type = gimp_drawable_type (drawable_ID);
	int n, bits;
	long filePos;

	gint32 * channels;     /* pointer to the different fur channels */
	gint32 num_channels;   /* number of fur channels */
	gchar * name;
	gint i;
	gfloat progress;
	
        printf("in save_image for fur\n");

	temp = g_malloc(strlen (filename) + 10);
	sprintf(temp, "Saving %s:", filename);
	gimp_progress_init(temp);
	g_free(temp);

	fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0644);

	if (fd == -1) {
		printf("Unable to open %s\n", filename);
		return_val = 0;
		goto finish_save_image;
	}

	drawable = gimp_drawable_get(drawable_ID);
	channels = gimp_image_get_channels (image_ID, &num_channels);

        furHeader.magicNumber = 0; 
	furHeader.version = 0; 
	furHeader.width = drawable->width; 
	furHeader.height = drawable->height; 
	furHeader.numberOfLayers = num_channels;

	if (write(fd, &furHeader, sizeof(furHeader)) != sizeof(furHeader)) {
		return_val = 0;
		goto finish_save_image;
	}

	/* Allocate a buffer for a float scanline of data from the fur file -- 4 bytes per float*/
	buffer_float = (gfloat *)g_malloc(furHeader.width * 4);

	/* Allocate a buffer for a float16 scanline of data to pass to gimp-- 2 bytes per float16*/
	buffer_float16 = (guint16 *)g_malloc(furHeader.width * 2);

	/* Loop through the grayscale channels of the image and read their data into
  	   into the file */ 

	for(i= 0; i < num_channels; i++)
	{
	  name =  gimp_channel_get_name (channels[i]);

	  n = strlen(name)+1;
	  write(fd, &n, sizeof(int));
	  write(fd, &name[0], sizeof(char)*n);	
	  bits = sizeof(float)*8;
	  write(fd, &bits, sizeof(int));

	  /* init the channel and region we'll copy data from */ 
	  drawable = gimp_drawable_get(channels[i]);
	  gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width,
			drawable->height, FALSE, FALSE);
	
	  filePos = lseek(fd, 0, 1); /* get the current position */
	  /* Loop through each scanlines of this channel -- read them into the float16 buffer */ 
	  for (line = 0; line < drawable->height; line++) 
	    {
		  gimp_pixel_rgn_get_row(&pixel_rgn, buffer_float16, 0, line, drawable->width);

	          /* convert the float16 buffer to 32bit floats */
		  float16_to_float (buffer_float16, buffer_float, drawable->width);

		  lseek(fd, filePos+((furHeader.height - line - 1)*furHeader.width*4), 0);
	          /* copy the float data to the file */
		  if (write(fd, buffer_float, drawable->width * 4) != drawable->width * 4) {
			return_val = 0;
			goto finish_save_image;
	    		}
	      progress = (drawable->height * i + line)/(float)(drawable->height * num_channels);
	      gimp_progress_update( progress );
	    }
	  lseek(fd, filePos+furHeader.height*furHeader.width*4, 0);
	}

	return_val = 1;

finish_save_image:
	if (buffer_float)
	  g_free(buffer_float);
	if (buffer_float16)
	  g_free(buffer_float16);
	close (fd);
	return return_val;
}

static void 
float_to_float16 (gfloat *buffer_float, guint16 * buffer_float16, gint number)
{
  gint i;
  ShortsFloat u;

  /* This uses a macro that converts from float to float16 ie. 
	from float to an unsigned short */
  for (i = 0; i < number; i++)
   buffer_float16[i] = FLT16 (buffer_float[i], u);
}

static void 
float16_to_float (guint16 *buffer_float16, gfloat * buffer_float, gint number)
{
  gint i;
  ShortsFloat u;

  /* This uses a macro that converts from float to float16 ie. 
	from float to an unsigned short */
  for (i = 0; i < number; i++)
   buffer_float[i] = FLT(buffer_float16[i], u);
}
