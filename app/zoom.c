#include <malloc.h>
#include <float.h>
#include "scale.h"
#include "zoom.h"
#include "stdio.h"

static GDisplay *zoom_get_active_display();
static void zoom_slider_value_change(
   GtkAdjustment *adjustment,
   gpointer user_data);

ZoomControl * zoom_control = 0;

ZoomControl * zoom_control_open()
{
   if (zoom_control && GTK_WIDGET_VISIBLE(zoom_control->window)) {
      return zoom_control;
   }

   // delete it if it exists
   if (zoom_control) {
      zoom_control_delete(zoom_control);
      zoom_control = 0;
   }

   // make a new one
   if (!zoom_control) {
      zoom_control = zoom_control_new();
   }

   // display it
   gtk_widget_show_all(zoom_control->window);
   return zoom_control;
}

void zoom_view_changed(GDisplay *disp)
{
    // first get the active display if there is none
    if (!zoom_control)
       return;
    if (!zoom_control->gdisp) {
       zoom_control->gdisp = zoom_get_active_display();
    }

    // if that failed, do nothing.
    if (!zoom_control->gdisp) {
       return;
    }

    // notify dialog's widgets to update zoom
    
    // notify drawing area to update rect.
}

ZoomControl * zoom_control_new()
{
  ZoomControl *zoom;
  GList *items = NULL;

  zoom = (ZoomControl *)malloc(sizeof(ZoomControl));
  zoom->gdisp = NULL;

  zoom->window = gtk_dialog_new();
//  gtk_window_set_wmclass (GTK_WINDOW (zoom->window), "Zoom Window", "Gimp");
  gtk_window_set_policy (GTK_WINDOW (zoom->window), FALSE, FALSE, FALSE);
  gtk_window_set_title (GTK_WINDOW (zoom->window), "Zoom/Pan Control");
  gtk_window_set_position(GTK_WINDOW(zoom->window), GTK_WIN_POS_MOUSE);
  gtk_object_sink(GTK_OBJECT(zoom->window));
  gtk_object_ref(GTK_OBJECT(zoom->window));

  zoom->pull_down = gtk_combo_new();

  items = g_list_append (items, "1:16");
  items = g_list_append (items, "1:4");
  items = g_list_append (items, "1:2");
  items = g_list_append (items, "1:1");
  items = g_list_append (items, "2:1");
  items = g_list_append (items, "4:1");
  items = g_list_append (items, "8:1");
  items = g_list_append (items, "16:1");
  gtk_combo_set_popdown_strings (GTK_COMBO (zoom->pull_down), items);

  zoom->adjust = GTK_ADJUSTMENT(gtk_adjustment_new(0, -4, 4, 1, -1, 1));
  zoom->slider = gtk_hscrollbar_new(zoom->adjust);
  gtk_signal_connect(GTK_OBJECT(zoom->adjust), "value-changed", 
                  GTK_SIGNAL_FUNC(zoom_slider_value_change), 0);

  zoom->drawing_area = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(zoom->drawing_area), 128, 128);
 
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(zoom->window)->action_area), zoom->pull_down, TRUE, TRUE, 0); 
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(zoom->window)->action_area), zoom->slider, TRUE, TRUE, 0); 
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(zoom->window)->vbox), zoom->drawing_area, TRUE, TRUE, 0); 

  gtk_widget_show(zoom->pull_down);
  gtk_widget_show(zoom->slider);
  gtk_widget_show(zoom->drawing_area);
  // don't show it, it is up to the caller to manage visibility state information.

  return zoom;
}

void zoom_control_delete(ZoomControl *zoom)
{
   gtk_object_unref(GTK_OBJECT(zoom->window));
   free(zoom);
}

/* Utility Functions */

// Returns which display the zoom control should refer to
static GDisplay *
zoom_get_active_display()
{
   // for now, just return the first one we find.
   if (!display_list)
      return NULL;
   else
      return (GDisplay *)display_list->data;
}


/* Callback functions for the zoom dialog */

// called when the value of the zoom slider changes
static void 
zoom_slider_value_change(
   GtkAdjustment *adjustment,
   gpointer user_data)
{
    int scale_val;

    // first get the active display if there is none
    if (!zoom_control->gdisp) {
       zoom_control->gdisp = zoom_get_active_display();
    }

    // if that failed, do nothing.
    if (!zoom_control->gdisp) {
       return;
    }

    // convert to an integer value, and round
    scale_val = (int) (adjustment->value + .5);
    printf("Raw value %f, int %d\n", adjustment->value, scale_val);

    // need to pack scale_val into the weird format used by change_scale().  
    scale_val = scale_val < 0 ? (0x01 << -scale_val) * 100 + 1 :
                                (0x01 << scale_val) + 100;

    // now set the zoom factor for the display
    change_scale(zoom_control->gdisp, scale_val); 
}

