/*
 *
 */

#include "base_frame_manager.h"
#include "store_frame_manager.h"
#include "clone.h"
#include "tools.h"
#include <stdio.h>
#include "general.h"
#include <string.h>
#include "layer_pvt.h"

static int success; 


static char
bfm_check (GDisplay *disp)
{
  if (!disp)
    return 0;
  if (disp->bfm)
    return 1;
  return 0;
}

static char
bfm_check_disp (GDisplay *disp)
{
  if (!disp)
    return 0;
  if (!disp->gimage)
    return 0;
  return 1;

  if (!disp->gimage->filename)
    return 0;
  return 1;
}

/*
 * CREATE & DELETE
 */
typedef void (*CheckCallback) (GtkWidget *, gpointer);

static void
bfm_init_varibles (GDisplay *disp)
{
  disp->bfm->onionskin = 0;
  disp->bfm->fg = 0;
  disp->bfm->bg = 0;
}

void
bfm_create_sfm (GDisplay *disp)
{

  double scale;
  
 if(!bfm_check_disp (disp))
   return;

 disp->bfm = (base_frame_manager*) g_malloc (sizeof(base_frame_manager)); 
 disp->bfm->sfm = (store_frame_manager*) g_malloc (sizeof(store_frame_manager));
 disp->bfm->cfm = 0;
 bfm_init_varibles (disp);

 disp->bfm->sfm->stores = 0;
 disp->bfm->sfm->bg = -1;
 disp->bfm->sfm->fg = 0;
 disp->bfm->sfm->add_dialog = 0;
 disp->bfm->sfm->chg_frame_dialog = 0;
 disp->bfm->sfm->readonly = 0; 
 disp->bfm->sfm->play = 0; 
 disp->bfm->sfm->load_smart = 0; 

 scale = ((double) SCALESRC (disp) / (double)SCALEDEST (disp));
 disp->bfm->sfm->s_x = disp->bfm->sfm->sx = 0;
 disp->bfm->sfm->s_y = disp->bfm->sfm->sy = 0;
 disp->bfm->sfm->e_x = disp->bfm->sfm->ex = disp->gimage->width;
 disp->bfm->sfm->e_y = disp->bfm->sfm->ey = disp->gimage->height;
 /* GUI */
 sfm_create_gui (disp);

}

void
bfm_create_cfm (GDisplay *disp)
{
}

void 
bfm_delete_sfm (GDisplay *disp)
{
  if (!bfm_check (disp))
    return;
}

void 
bfm_delete_cfm (GDisplay *disp)
{
  if (!bfm_check (disp))
    return;
}

  
void 
bfm_dirty (GDisplay *disp)
{
  if (!bfm_check (disp))
    return;
  if(disp->bfm->cfm)
    {
      /*cfm_onionskin_set_offset (disp->bfm->cfm, x, y);*/
    }
  else
    if(disp->bfm->sfm)
      sfm_dirty (disp);
}

int
bfm (GDisplay *disp)
{
  if (disp->bfm)
    return 1;
  return 0; 
}
/*
 * ONIONSKINNIG
 */

char 
bfm_onionskin (GDisplay *disp)
{
  if (!bfm_check (disp))
    return 0;
  return disp->bfm->onionskin;
}

void 
bfm_onionskin_set_offset (GDisplay *disp, int x, int y)
{
  if (!bfm_check (disp)  || !disp->bfm->onionskin)
    return;
  
  if(disp->bfm->cfm)
    {
      /*cfm_onionskin_set_offset (disp->bfm->cfm, x, y);*/
    }
  else
    if(disp->bfm->sfm)
      sfm_onionskin_set_offset (disp, x, y);
}

void 
bfm_onionskin_rm (GDisplay *disp)
{
  if (!bfm_check (disp) || !disp->bfm->onionskin)
    return;

  if(disp->bfm->cfm)
    {
      /*cfm_onionskin_rm (disp->bfm->cfm);*/
    }
  else
    if(disp->bfm->sfm)
      {
	sfm_onionskin_rm (disp);

	disp->bfm->fg->active_layer->opacity = 1;
	gimage_remove_layer (disp->bfm->fg, (disp->bfm->bg->active_layer));

	GIMP_DRAWABLE (disp->bfm->bg->active_layer)->offset_x = 0;
	GIMP_DRAWABLE (disp->bfm->bg->active_layer)->offset_y = 0;
	GIMP_DRAWABLE(disp->bfm->bg->active_layer)->gimage_ID = disp->bfm->bg->ID;

	disp->bfm->onionskin = 0;
      }
}

GImage* 
bfm_get_fg (GDisplay *disp)
{
  if (!bfm_check (disp))
    return NULL;

  return disp->bfm->fg;
}

GImage* 
bfm_get_bg (GDisplay *disp)
{
  if (!bfm_check (disp))
    return NULL;
  return disp->bfm->bg;
}

GimpDrawable* 
bfm_get_fg_drawable (GDisplay *disp)
{
  if (!bfm_check (disp))
    return NULL;
  return GIMP_DRAWABLE (disp->bfm->fg->active_layer); 
}

GimpDrawable* 
bfm_get_bg_drawable (GDisplay *disp)
{
  if (!bfm_check (disp))
    return NULL;
  return GIMP_DRAWABLE (disp->bfm->bg->active_layer); 
}

int 
bfm_get_fg_display (GDisplay *disp)
{
   if (!bfm_check (disp))
         return -1;
   return disp->bfm->fg->ID;
}

int 
bfm_get_bg_display (GDisplay *disp)
{
   if (!bfm_check (disp))
         return -1;
   return disp->bfm->bg->ID;
}

void
bfm_set_fg (GDisplay *disp, GImage *fg)
{
  if (!bfm_check (disp))
        return;

  disp->bfm->fg = fg;

}

void
bfm_set_bg (GDisplay *disp, GImage *bg)
{
  if (!bfm_check (disp))
        return;

  disp->bfm->bg = bg;

}

void
bfm_onionskin_unset (GDisplay *disp)
{
  disp->bfm->onionskin = 0; 
}

void 
bfm_onionskin_display (GDisplay *disp, double val, int sx, int sy, int ex, int ey)
{
  Layer *layer;
  int x=0, y=0;

  if (!disp->bfm->fg || !disp->bfm->bg)
    return;

  disp->bfm->fg->active_layer->opacity = val;

  if (!disp->bfm->onionskin)
    {
      if (active_tool->type == CLONE)
	{
	  x = clone_get_x_offset ();
	  y = clone_get_y_offset ();
	}
      layer = disp->bfm->fg->active_layer;
      gimage_add_layer2 (disp->bfm->fg, disp->bfm->bg->active_layer,
	  1, sx, sy, ex, ey);
      gimage_set_active_layer (disp->bfm->fg, layer);
      sfm_onionskin_set_offset (disp, x, y);
      disp->bfm->onionskin = 1;
    }

  drawable_update (GIMP_DRAWABLE(disp->bfm->fg->active_layer),
      sx, sy, ex, ey);
  gdisplays_flush ();

}

/*
 * FILE STUFF
 */

void
bfm_this_filename (char *filepath, char *filename, char *whole, char *raw, int frame)
{
  char *name, *num, *ext;
  char frame_num[10];
  int old_l, cur_l;

  name  = strtok (strdup(filename), ".");
  num = strtok (NULL, ".");
  ext = strtok (NULL, ".");

  sprintf (frame_num, "%d", frame);

  old_l = strlen (num);
  cur_l = strlen (frame_num);

  switch (old_l-cur_l)
    {
    case 0:
      break;
    case 1:
      sprintf (frame_num, "0%d", frame);
      break;
    case 2:
      sprintf (frame_num, "00%d", frame);
      break;
    case 3:
      sprintf (frame_num, "000%d", frame);
      break;
    case 4:
      sprintf (frame_num, "0000%d", frame);
      break;
    default:
      break; 
    }
  sprintf (raw, "%s.%s.%s", name, frame_num, ext);

  sprintf (whole, "%s/%s", filepath, raw);

}

char*
bfm_get_name (char *filename)
{

  char raw[256], whole[256];
  char *tmp;

  strcpy (whole, filename);
  strcpy (raw, prune_filename (whole));

  tmp = strdup (strtok (raw, "."));

  return tmp;

}

char*
bfm_get_frame (char *filename)
{

  char raw[256], whole[256];

  strcpy (whole, filename);
  strcpy (raw, prune_filename (whole));

  strtok (raw, ".");

  return strdup (strtok (NULL, "."));

}

char*
bfm_get_ext (char *filename)
{

  char raw[256], whole[256];

  strcpy (whole, filename);
  strcpy (raw, prune_filename (whole));

  strtok (raw, ".");
  strtok (NULL, ".");
  return strdup (strtok (NULL, "."));

}

GDisplay* 
bfm_load_image_into_fm (GDisplay *disp, GImage *img)
{
  if (!bfm_check (disp))
    return disp;
  if (disp->bfm->sfm)
    return sfm_load_image_into_fm (disp, img);
  return disp; 
}

/*
 * PLUG-IN STUFF
 */

void 
bfm_set_dir_dest (GDisplay *disp, char *filename)
{
  if (!bfm_check (disp))
    return;

  disp->bfm->dest_dir = strdup (prune_pathname(filename));
  disp->bfm->dest_name = strdup (prune_filename(filename));

  if (disp->bfm->sfm)
    sfm_set_dir_dest (disp);
}

static Argument *
bfm_set_dir_dest_invoker (Argument *args)
{
  GDisplay *disp; 
  gint int_value;

  success = TRUE;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (!(disp = gdisplay_get_ID (int_value)))
	success = FALSE;
    }

  if (success)
    bfm_set_dir_dest (disp, (char *) args[1].value.pdb_pointer);

  /*  create the new image  */
  return  procedural_db_return_args (&bfm_set_dir_dest_proc, success);

}

/*  The procedure definition  */
ProcArg bfm_set_dir_args[] =
{
};

ProcArg bfm_set_dir_out_args[] =
{
  { PDB_DISPLAY, "display", "The display"}, {PDB_STRING, "filename", "The name of the file to load." }
};

ProcRecord bfm_set_dir_dest_proc =
{
  "gimp_bfm_set_dir_dest",
  "Get a fileset from the user",
  "Get a fileset from the user",
  "Caroline Dahllof",
  "Caroline Dahllof",
  "2002",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  bfm_set_dir_args,

  /*  Output arguments  */
  0,
  bfm_set_dir_out_args,

  /*  Exec method  */
    { { bfm_set_dir_dest_invoker } },
};


void 
bfm_set_dir_src (GDisplay *disp, char *filename)
{
  if (!bfm_check (disp))
    return;

  disp->bfm->src_dir = strdup (prune_pathname(filename));
  disp->bfm->src_name = strdup (prune_filename(filename));

  if (disp->bfm->sfm)
    sfm_set_dir_src (disp);
}

static Argument *
bfm_set_dir_src_invoker (Argument *args)
{
  GDisplay *disp; 
  gint int_value;

  success = TRUE;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (!(disp = gdisplay_get_ID (int_value)))
	success = FALSE;
    }

  if (success)
    bfm_set_dir_src (disp, (char *) args[1].value.pdb_pointer);

  /*  create the new image  */
  return  procedural_db_return_args (&bfm_set_dir_src_proc, success);

}

/*  The procedure definition  */

ProcRecord bfm_set_dir_src_proc =
{
  "gimp_bfm_set_dir_src",
  "Get a fileset from the user",
  "Get a fileset from the user",
  "Caroline Dahllof",
  "Caroline Dahllof",
  "2002",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  bfm_set_dir_args,

  /*  Output arguments  */
  0,
  bfm_set_dir_out_args,

  /*  Exec method  */
    { { bfm_set_dir_src_invoker } },
};

