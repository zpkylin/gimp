/*
 * pts plug-in 
 * Loads/saves pts files from gimp.
 *
 *  
 * Contents:
 *
 *   main()                      - Main entry - just call gimp_main()...
 *   query()                     - Respond to a plug-in query...
 *   run()                       - Run the plug-in...
 */

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>



#include "libgimp/gimp.h"
#include "main-vk.h"
#include "fm_pts.h"

/* can we get rid of these globals... */
gint32 image_ID;
gint32 drawable_ID;
gint32 drawable_to_save_ID;
gint32 is_layered;
gint32 is_dirty;     
int dialog_in_use;

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

static gint32 disp_ID;

static void
query ()
{
	/* load args */
 static GParamDef load_args[] =
  {
    { PARAM_INT32 },
      { PARAM_DISPLAY, "display", "Input display"}
  };
  static gint load_nargs = sizeof (load_args) / sizeof (load_args[0]);

	/* save args */
  static GParamDef      save_args[] =                        
  {                                                     
    { PARAM_INT32,      "run_mode",     "Interactive, non-interactive" },
    { PARAM_IMAGE,      "image",        "Input image" },
    { PARAM_DRAWABLE,   "drawable",     "Input drawable" }
  };                                                    
  static GParamDef      *save_return_vals = NULL;            
  static int             save_nargs        = sizeof(save_args) / sizeof(save_args[0]);
  static int 		 save_nreturn_vals = 0; 

#if 0
      printf("process %d ", getpid());
  printf("pts query called\n");
#endif
#if 0
  gimp_install_procedure ("rhythm_and_hues_toolbox_fm_load_extension",
		    "Provides a Rhythm & Hues fm open dialog",
		    "Provides a Rhythm & Hues fm open dialog",
		    "Calvin Williamson",
		    "Calvin Williamson",
		    "1999",
		    "<Image>/Dialogs/Frame Manager/FMPTS",
		    "",
		    PROC_EXTENSION,
		    load_nargs, 0, 
		    load_args, NULL);
#endif
  gimp_install_procedure ("rhythm_and_hues_image_fm_load_extension",
		    "Provides a Rhythm & Hues fm save dialog",
		    "Provides a Rhythm & Hues fm save dialog",
		    "Calvin Williamson",
		    "Calvin Williamson",
		    "1999",
		    "<Image>/Dialogs/Frame Manager/Load Image",
		    "FLOAT16_RGB*, FLOAT16_GRAY*",
		    PROC_PLUG_IN,
		    save_nargs, save_nreturn_vals, 
		    save_args, save_return_vals);

}


static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  GStatusType status = STATUS_SUCCESS; 
  static GParam values[1];
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
  *nreturn_vals = 1;
  *return_vals = values;

  dialog_in_use = FALSE;

  if (strcmp (name, "rhythm_and_hues_toolbox_fm_load_extension") == 0 ||
	strcmp (name, "rhythm_and_hues_image_fm_load_extension") == 0 )
  {
#if 0 
    printf("process %d ", getpid());
    printf("pts run rhythm_and_hues_load_extension\n");
#endif
    disp_ID = param[1].data.d_int32;
    dialog_in_use = TRUE;
    vk_main(0);
  }
  else if (strcmp (name, "rhythm_and_hues_save_plugin") == 0 )
  {

    image_ID = param[1].data.d_int32;
    drawable_ID = param[2].data.d_int32;
    char * filename = gimp_image_get_filename (image_ID);
    drawable_to_save_ID = gimp_image_get_active_layer(image_ID);
     
#if 0 
      printf("process %d ", getpid());
    printf("pts run rhythm_and_hues_save_plugin: filename is %s\n", filename);
      printf("process %d ", getpid());
    printf("image_ID, drawable_ID %d %d\n", image_ID, drawable_ID);
#endif

    is_layered = gimp_image_is_layered (image_ID);     
    is_dirty = gimp_image_dirty_flag (image_ID); 


#if 0 
      printf("process %d ", getpid());
    printf("plugin is_layered is %d\n", is_layered);
      printf("process %d ", getpid());
    printf("plugin is_dirty is %d\n", is_dirty);
#endif

    if (is_dirty) 
    {
	if (!strcmp(filename, "Untitled") )  /* never been saved */
	{
          dialog_in_use = TRUE;
    	  vk_main(1); 
	}
	else 
	{
          dialog_in_use = FALSE;
	  file_save (filename, FALSE); 
	}
    }
#if 0 
    else
      g_warning("not saving... is_dirty FALSE\n");
#endif
	
  }
  else if (strcmp (name, "rhythm_and_hues_save_as_plugin") == 0)
  {
    image_ID = param[1].data.d_int32;
    drawable_ID = param[2].data.d_int32;
    char * filename = gimp_image_get_filename (image_ID);
    drawable_to_save_ID = gimp_image_get_active_layer(image_ID);

#if 0 
      printf("process %d ", getpid());
    printf("pts run rhythm_and_hues_save_as_plugin: filename is %s\n",filename);
      printf("process %d ", getpid());
    printf("image_ID, drawable_ID %d %d\n", image_ID, drawable_ID);
#endif

    is_layered = gimp_image_is_layered (image_ID);     
    is_dirty = gimp_image_dirty_flag (image_ID); 
#if 0 
      printf("process %d ", getpid());
    printf("plugin is_layered is %d\n", is_layered);
      printf("process %d ", getpid());
    printf("plugin is_dirty is %d\n", is_dirty);
#endif

    dialog_in_use = TRUE;
    vk_main(2);
  }
}

int file_load (const char * filename)
{
   GParam *return_vals;
   int nreturn_vals;
   gint32 image_ID;
#if 0 
      printf("process %d ", getpid());
   printf("in file_load\n");
#endif

   return_vals = gimp_run_procedure ("gimp_file_load",
			&nreturn_vals,
			PARAM_INT32, 1,
			PARAM_STRING, filename,
			PARAM_STRING, filename,
			PARAM_END);   

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
#if 0 
      printf("process %d ", getpid());
      printf("gimp_file_load successful\n");
#endif
      image_ID = return_vals[1].data.d_image;
      gimp_image_enable_undo( image_ID);
      gimp_image_clean_all( image_ID);
      gimp_display_fm (image_ID, disp_ID);
      return 0; 
    }
  else
    {
      /* Have to pop up a unsuccess dialog here */
      doWarningDialog("File load unsuccessful");
      return 1; 
    }
}



int file_save (const char * filename, int checkOverwrite)
{
  GParam *return_vals;
  int nreturn_vals;
  char * extension; 
  struct stat buf;
  int err;

#if 0 
  printf("process %d ", getpid());
  printf("in file_save\n");
#endif

  extension = strrchr(filename, '.');
  if (extension)
    extension += 1;

#if 0 
  printf("process %d ", getpid());
  printf("extension is %s\n", extension);
#endif
  if (is_layered)
    {
#if 0 
      printf("process %d ", getpid());
      printf("is_layered is %d\n", is_layered);
#endif

      if (strcmp(extension, "xcf"))
	{
	  if (dialog_in_use)
	    doWarningDialog("Cant save layered image except as xcf. Or choose flatten first.");
	  else
	    g_message("Must choose flatten first");
	  return 1;
	}
    }

  if (checkOverwrite)
    {
      int overwrite;
      err = stat(filename, &buf);
      if (err == 0)
	{
	  if (buf.st_mode & S_IFREG)
	    overwrite = doOverwrite("Overwrite existing file?"); 
	  if (!overwrite)
	    return 1;
	}
    }    

  return_vals = gimp_run_procedure ("gimp_file_save",
      &nreturn_vals,
      PARAM_INT32, 1,
      PARAM_IMAGE, image_ID,
      PARAM_DRAWABLE, drawable_to_save_ID,
      PARAM_STRING, filename,
      PARAM_STRING, filename,
      PARAM_END);   

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
#if 0
      printf("process %d ", getpid());
      printf("gimp_file_save successful\n");
#endif
      gimp_image_set_filename (image_ID, (char *)filename);
      gimp_image_clean_all(image_ID);
      return 0; 
    }
  else
    {
      /* Have to pop up a unsuccess dialog here */
      g_warning("gimp_file_save unsuccessful\n");
      doWarningDialog("Save failed.");
      return 1; 
    }
}

