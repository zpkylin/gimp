
#include "globals.h"
#include <gtk/gtkobject.h>
#include "gimpset.h"
#include "tools.h"

/* For now, this file has lots of dirt that has been taken off from
   other files.. they become more modular and the app-specific stuff
   gets here.. for now */

GimpSet* image_set;

static void
tool_dirty_handler (GImage* gimage)
{
  if (active_tool && !active_tool->preserve) {
    GDisplay* gdisp = active_tool->gdisp_ptr;
    if (gdisp)
      if (gdisp->gimage->ID == gimage->ID)
	tools_initialize (active_tool->type, gdisp);
      else
	active_tool_control(DESTROY, gdisp);
    else
      active_tool_control(DESTROY, gdisp);
  }
}

static void
tool_new_image_handler (GimpSet* set, GImage* image, gpointer data)
{
  gtk_signal_connect (GTK_OBJECT (image), "dirty",
		      GTK_SIGNAL_FUNC(tool_dirty_handler), NULL);
}

void
globals_init (void)
{
  image_set = gimp_set_new ();
  gtk_signal_connect (GTK_OBJECT (image_set), "add",
		      GTK_SIGNAL_FUNC(tool_new_image_handler), NULL);
}

void
globals_fini (void)
{
  gtk_object_unref (GTK_OBJECT (image_set));
}
