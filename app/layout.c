#include <gtk/gtk.h>
#include "layout.h"
#include "gimprc.h"

static void 
layout_configure_eventx(
   GtkWidget *widget,
   GdkEventConfigure *ev,
   gpointer user_data)
{
   int dummy;
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
   if (widget->window)
      gdk_window_get_position(widget->window, &dummy, (int *)user_data);
}

static void
layout_show_event(
   GtkWidget *wid,
   gpointer user_data)
{
   *((int *)user_data) = 1;
}

static void
layout_hide_event(
   GtkWidget *wid,
   gpointer user_data)
{
   *((int *)user_data) = 0;
}

int  
layout_save()
{
   GList *save = NULL;
   GList *dummy = NULL;

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
   save = g_list_append(save, "brushedit-visible");
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

