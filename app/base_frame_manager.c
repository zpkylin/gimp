/*
 *
 */

#include "base_frame_manager.h"
#include "store_frame_manager.h"
#include <stdio.h>
#include "general.h"
#include <string.h>
#include "layer_pvt.h"

static char
bfm_check (GDisplay *disp)
{
  if (!disp)
    return 0;

  return 1;
  if (disp->frame_manager)
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
  
 if(!bfm_check_disp (disp))
   return;

 disp->bfm = (base_frame_manager*) g_malloc (sizeof(base_frame_manager)); 
 disp->bfm->sfm = (store_frame_manager*) g_malloc (sizeof(store_frame_manager));
 disp->bfm->cfm = 0;
 bfm_init_varibles (disp);

 disp->bfm->sfm->stores = 0;
 disp->bfm->sfm->bg = -1;
 disp->bfm->sfm->fg = -1;
 disp->bfm->sfm->add_dialog = 0;
 disp->bfm->sfm->readonly = 0; 
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

/*
 * ONIONSKINNIG
 */

char 
bfm_onionskin (GDisplay *disp)
{
  if (!bfm_check (disp))
    return -1;
  return disp->bfm->onionskin;
}

void 
bfm_onionskin_set_offset (GDisplay *disp, int x, int y)
{
  if (!bfm_check (disp))
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
  if (!bfm_check (disp))
    return;

  if(disp->bfm->cfm)
    {
      /*cfm_onionskin_rm (disp->bfm->cfm);*/
    }
  else
    if(disp->bfm->sfm)
      sfm_onionskin_rm (disp);

  disp->bfm->fg->active_layer->opacity = 1;
  gimage_remove_layer (disp->bfm->fg, (disp->bfm->bg->active_layer));

  GIMP_DRAWABLE (disp->bfm->bg->active_layer)->offset_x = 0;
  GIMP_DRAWABLE (disp->bfm->bg->active_layer)->offset_y = 0;
  GIMP_DRAWABLE(disp->bfm->bg->active_layer)->gimage_ID = disp->bfm->bg->ID;

  disp->bfm->onionskin = 0;

}

void
bfm_onionskin_set_fg (GDisplay *disp, GImage *fg)
{
  if (!bfm_check (disp))
        return;

  disp->bfm->fg = fg;

}

void
bfm_onionskin_set_bg (GDisplay *disp, GImage *bg)
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

  if (!disp->bfm->fg || !disp->bfm->bg)
    return;

  disp->bfm->fg->active_layer->opacity = val;

  if (!disp->bfm->onionskin)
    {
      layer = disp->bfm->fg->active_layer;
      gimage_add_layer2 (disp->bfm->fg, disp->bfm->bg->active_layer,
	  1, sx, sy, ex, ey);
      gimage_set_active_layer (disp->bfm->fg, layer);
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
bfm_this_filename (GImage *gimage, char **whole, char **raw, char *num)
{

  char rawname[256], *name, *frame, *ext;
  int len, org_len;

  if (!gimage)
    return;

  strcpy (*whole, gimage->filename);
  strcpy (rawname, prune_filename (*whole));

  len = strlen (*whole);
  org_len = strlen (rawname);


  name  = strtok (rawname, ".");
  frame = strtok (NULL, ".");
  ext = strtok (NULL, ".");


  sprintf (*raw, "%s.%s.%s\0", name, num, ext);

  sprintf (&((*whole)[len-org_len]), "%s\0", *raw);

}

char*
bfm_get_name (GImage *gimage)
{

  char raw[256], whole[256];
  char *tmp;

  if (!gimage)
    return "ERROR";

  strcpy (whole, gimage->filename);
  strcpy (raw, prune_filename (whole));

  tmp = strdup (strtok (raw, "."));

  return tmp;

}

char*
bfm_get_frame (GImage *gimage)
{

  char raw[256], whole[256];

  if (!gimage)
    return "ERROR";

  strcpy (whole, gimage->filename);
  strcpy (raw, prune_filename (whole));

  strtok (raw, ".");

  return strdup (strtok (NULL, "."));

}

char*
bfm_get_ext (GImage *gimage)
{

  char raw[256], whole[256];

  if (!gimage)
    return "ERROR";

  strcpy (whole, gimage->filename);
  strcpy (raw, prune_filename (whole));

  strtok (raw, ".");
  strtok (NULL, ".");
  return strdup (strtok (NULL, "."));

}

