/*
 *
 */

#include "base_frame_manager.h"
#include "store_frame_manager.h"
#include <stdio.h>
#include "general.h"
#include <string.h>

static char
bfm_check (GDisplay *disp)
{
  if (!disp)
    return 0;

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
 disp->bfm->sfm->selected = -1;
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
bfm_set_dirty_flag (GDisplay *disp, int flag)
{
  if (!bfm_check (disp))
    return;
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

