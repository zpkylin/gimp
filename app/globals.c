
#include "globals.h"
#include <gtk/gtkobject.h>
#include "gimpset.h"

GimpSet* image_set;

void
globals_init (void)
{
  image_set = gimp_set_new ();
}

void
globals_fini (void)
{
  gtk_object_unref (GTK_OBJECT(image_set));
}
