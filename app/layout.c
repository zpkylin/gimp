#include <stdio.h>
#include <gtk/gtk.h>
#include "layout.h"
#include "gimprc.h"
#include "gradient.h"
#include "palette.h"
#include "zoom.h"
#include "tools.h"
#include "gimpbrushlist.h"
#include "layers_dialog.h"
#include "color_area.h"


static int g_ignore_further_updates = 0;

void layout_freeze_current_layout()
{
   g_ignore_further_updates = 1;
}

void layout_unfreeze_current_layout()
{
   g_ignore_further_updates = 0;
}

static void
layout_map(GtkWidget *widget,
	gpointer user)
{
   printf("Map received\n");
}

static void
layout_unmap(GtkWidget *widget,
	gpointer user)
{
   printf("Unmap received\n");
}

static void 
layout_configure_eventx(
   GtkWidget *widget,
   GdkEventConfigure *ev,
   gpointer user_data)
{
   int dummy;
   if (g_ignore_further_updates)
      return;

   if (widget->window)
      gdk_window_get_position(widget->window, (int *)user_data, &dummy);
}

static void 
layout_configure_eventy(
   GtkWidget *widget,
   GdkEventConfigure *ev,
   gpointer user_data)
{
   int dummy;
   if (g_ignore_further_updates)
      return;
   if (widget->window)
      gdk_window_get_position(widget->window, &dummy, (int *)user_data);
}

static void
layout_show_event(
   GtkWidget *wid,
   gpointer user_data)
{
   if (g_ignore_further_updates)
      return;
   *((int *)user_data) = 1;
}

static void
layout_hide_event(
   GtkWidget *wid,
   gpointer user_data)
{
   if (g_ignore_further_updates)
      return;
   *((int *)user_data) = 0;
}

int  
layout_save()
{
   GList *save = NULL;
   GList *dummy = NULL;

   // this is kind of a hack.  Since the images are automatically placed 10 units
   // past the previous, we want to back up when we save so the image doesn't drift
   // downward everytime we run gimp
   image_x -= 10;
   image_y -= 10;

   save = g_list_append(save, "toolbox-position");
   save = g_list_append(save, "info-position");
   save = g_list_append(save, "progress-position");
   save = g_list_append(save, "color-select-position");
   save = g_list_append(save, "tool-options-position");
   save = g_list_append(save, "framemanager-position");
   save = g_list_append(save, "palette-position");
   save = g_list_append(save, "zoom-position");
   save = g_list_append(save, "generic-position");
   save = g_list_append(save, "gradient-position");
   save = g_list_append(save, "image-position");
   save = g_list_append(save, "brusheditor-position");
   save = g_list_append(save, "brushselect-position");
   save = g_list_append(save, "layerchannel-position");
   save = g_list_append(save, "tooloptions-visible");
   save = g_list_append(save, "zoomwindow-visible");
   save = g_list_append(save, "brushselect-visible");
   save = g_list_append(save, "layerchannel-visible");
   save = g_list_append(save, "color-visible");
   save = g_list_append(save, "palette-visible");
   save = g_list_append(save, "gradient-visible");
 
   save_gimprc(&save, &dummy);

   g_list_free(save);
   g_list_free(dummy);

   return 1;
}

void 
layout_connect_window_visible(GtkWidget *widget, int *visible)
{
   gtk_signal_connect (GTK_OBJECT (widget), "show",
		      GTK_SIGNAL_FUNC (layout_show_event),
		      visible);
   gtk_signal_connect (GTK_OBJECT (widget), "hide",
		      GTK_SIGNAL_FUNC (layout_hide_event),
		      visible);
   gtk_signal_connect (GTK_OBJECT (widget), "unrealize",
		      GTK_SIGNAL_FUNC (layout_hide_event),
		      visible);

   gtk_widget_add_events(widget, GDK_VISIBILITY_NOTIFY_MASK);
}

void 
layout_connect_window_position(GtkWidget *widget, int *x_var, int *y_var)
{
   gtk_signal_connect (GTK_OBJECT (widget), "configure-event",
		      GTK_SIGNAL_FUNC (layout_configure_eventx),
		      x_var);
   gtk_signal_connect (GTK_OBJECT (widget), "configure-event",
		      GTK_SIGNAL_FUNC (layout_configure_eventy),
		      y_var);
}

void layout_restore()
{
   // go to each variable.  If the window should be visible, create it.
   if (tool_options_visible) {
      tools_options_dialog_show();
   }

   if (zoom_window_visible) {
      zoom_control_open();
   }

   if (brush_select_visible) {
      create_brush_dialog ();
   }

   if (layer_channel_visible) {
      lc_dialog_create (-1);
   }

   if (color_visible) {
      color_area_edit();
   }

   if (palette_visible) {
      palette_create();
   }

   if (gradient_visible) {
      grad_create_gradient_editor();
   }

}

