#include "minimize.h"
#include <stdio.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <gdk/gdkx.h>

static gboolean 
minimize_prop_notify(
   GtkWidget *widget,
   GdkEventProperty *event,
   gpointer user_data);

static void
minimize_delete_cb(gpointer data);

// The list of windows that will be minimzed when the toolbox is minimized
static GList *minimize_windows = NULL;

//*************************************************************
// External Functions 
//*************************************************************
void minimize_register(GtkWidget *widget)
{
   // register to be notified when it is deleted
   gtk_object_weakref(GTK_OBJECT(widget), minimize_delete_cb, (gpointer) widget);

   // add to list of things to minimize
   minimize_windows = g_list_append(minimize_windows, (gpointer) widget);
}

void minimize_all()
{
   GtkWidget *window;
   GList *cur = minimize_windows;

   while (cur != NULL) {
      window = (GtkWidget *)cur->data;

      if (window && window->window) {
         // go to all registered widgets and force them to minimize
         XIconifyWindow(GDK_WINDOW_XDISPLAY(window->window),
                        GDK_WINDOW_XWINDOW(window->window),
                        DefaultScreen (GDK_DISPLAY ()));
      }
      else {
         printf("Skipping window\n");
      }

      cur = cur->next;
   }
}

void minimize_track_toolbox(GtkWidget *toolbox)
{
   gtk_signal_connect(GTK_OBJECT(toolbox), "property-notify-event",
		      GTK_SIGNAL_FUNC(minimize_prop_notify), 0);
   /*
   gtk_signal_connect(GTK_OBJECT( widget), "map",
		      GTK_SIGNAL_FUNC(layout_map), 0);
   gtk_signal_connect(GTK_OBJECT( widget), "unmap",
		      GTK_SIGNAL_FUNC(layout_unmap), 0);
   */
   gtk_widget_add_events(toolbox, GDK_PROPERTY_CHANGE_MASK );
}


//*************************************************************
// Callbacks
//*************************************************************

gboolean 
minimize_prop_notify(
   GtkWidget *widget,
   GdkEventProperty *event,
   gpointer user_data)
{
   GdkAtom value;
   unsigned long *property = 0;
   gint format;
   gint length;
   gint status;

   // this is so ugly!  GTK doesn't provide any reasonable way 
   // of tracking this state, so you have to intercept X Windows property
   // changes, and check for change in the WM_STATE property.
   
   if (strcmp ("WM_STATE", gdk_atom_name(event->atom))==0) {
      // gdk_property_get is one of the ugliest functions in GTK, but this
      // call seems to work...
      status = gdk_property_get(widget->window, event->atom, 0, 0, 1, FALSE, 
		      &value, &format, &length, (guchar **) &property);
      if (status && property != 0) {
         if (*property == 3) {
	    // IconicState (figured out from trial and error)
	    minimize_all();
	 }
	 // *property == 1 means un-minimized.  I have no idea what other
	 // values mean.  - jcohen

         g_free(property);
      }
   }

   return FALSE;
}

void
minimize_delete_cb(gpointer data)
{
   minimize_windows = g_list_remove(minimize_windows, data);
}

