#ifndef __GIMAGE_C__
#define __GIMAGE_C__
#include "gimpimage_pvt.h"
#include "gimage.h"
#include "gimpimage.h"
#include "gimpset.h"
#include "globals.h"

#include "channels_dialog.h"

#include "indexed_palette.h"

#include "drawable.h"
#include "gdisplay.h"


#include "palette.h"
#include "undo.h"

#include "gimpset.h"

#include "layer.h"
#include "channel.h"
static int global_gimage_ID = 1;

static void gimage_destroy_handler (GimpImage* gimage);
static void gimage_rename_handler (GimpImage* gimage);
static void gimage_resize_handler (GimpImage* gimage);

/* This is not strictly needed any more, but bloody idiotic code from
   outside declares this extern and accesses this directly! */

GSList* image_list;


GImage*
gimage_new(int width, int height, GimpImageBaseType base_type)
{
  GimpImage* gimage = gimp_image_new (width, height, base_type);

  gtk_signal_connect (GTK_OBJECT (gimage), "destroy",
		      gimage_destroy_handler, NULL);
  gtk_signal_connect (GTK_OBJECT (gimage), "rename",
		      gimage_rename_handler, NULL);
  gtk_signal_connect (GTK_OBJECT (gimage), "resize",
		      gimage_resize_handler, NULL);
  
  gimage->ref_count = 0;
  gimage->ID = global_gimage_ID ++;
  image_list = g_slist_append (image_list, gimage);
  gimp_set_add (image_set, gimage->ID, gimage);
  
  indexed_palette_update_image_list ();
  return gimage;
}

GImage*
gimage_get_ID (gint ID)
{
  
  gpointer p = gimp_set_lookup (image_set, ID);
  if (p)
    return GIMP_IMAGE (p);
  else
    return NULL;
}


/* Ack! GImages have their own ref counts! This is going to cause
   trouble.. It should be pretty easy to convert to proper GtkObject
   ref counting, though. */

void
gimage_delete (GImage *gimage)
{
  gimage->ref_count--;
   if (gimage->ref_count <= 0)
     gtk_object_unref (GTK_OBJECT(gimage));
};

static void
invalidate_cb (gpointer key, gpointer value, gpointer user_data)
{
  gimp_image_invalidate_preview (GIMP_IMAGE (value));
}


void
gimage_invalidate_previews (void)
{
  gimp_set_foreach (image_set, invalidate_cb, NULL);
}
 
static void
gimage_destroy_handler (GimpImage* gimage)
{
  /*  free the undo list  */
  undo_free (gimage);

  image_list = g_slist_remove (image_list, (void *) gimage);

  /*  remove this image from the global hash  */
  gimp_set_remove (image_set, gimage->ID);

  indexed_palette_update_image_list ();
}

static void
gimage_rename_handler (GimpImage* gimage)
{
  gdisplays_update_title (gimage->ID);
  indexed_palette_update_image_list ();
}

static void
gimage_resize_handler (GimpImage* gimage)
{
  undo_push_group_end (gimage);

  /*  shrink wrap and update all views  */
  channel_invalidate_previews (gimage->ID);
  layer_invalidate_previews (gimage->ID);
  gimp_image_invalidate_preview (gimage);
  gdisplays_update_full (gimage->ID);
  gdisplays_shrink_wrap (gimage->ID);
}

#endif
