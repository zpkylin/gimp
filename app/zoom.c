#include <stdlib.h>
#include <malloc.h>
#include <float.h>
#include "scale.h"
#include "zoom.h"
#include "stdio.h"

/************************************************************/
/*     Internal Function Declarations                       */
/************************************************************/
static void zoom_control_close(GtkObject *wid, gpointer data);

static void zoom_update_pulldown_slider(GDisplay *disp);
static void zoom_update_slider(GDisplay *disp);
static void zoom_update_pulldown(GDisplay *disp);
static void zoom_update_scale(GDisplay *gdisp, gint scale_val);
static GDisplay *zoom_get_active_display();
static int zoom_control_activate();
static void zoom_slider_value_changed(
   GtkAdjustment *adjustment,
   gpointer user_data);
static void zoom_pulldown_value_changed(
   GtkList *list,
   GtkWidget *widget,
   gpointer user_data);
static gboolean zoom_preview_expose_event(
   GtkWidget *widget,
   GdkEventExpose *event,
   gpointer user_data);
static gboolean zoom_preview_configure_event(
   GtkWidget *widget,
   GdkEventConfigure *event,
   gpointer user_data); 
static gboolean zoom_preview_motion_notify_event(
   GtkWidget *widget,
   GdkEventMotion *event,
   gpointer user_data);
static gboolean zoom_preview_button_press_event(
   GtkWidget *widget,
   GdkEventButton *event,
   gpointer user_data);
static void zoom_clear_pixmap(GtkWidget *preview, GdkPixmap *pixmap);

/************************************************************/
/*     Global variables (yikes!)                            */
/************************************************************/
static int zoom_external_generated = 0;
static int zoom_updating_ui = 0;
static int zoom_updating_scale = 0;
ZoomControl * zoom_control = 0;
static GDisplay *zoom_gdisp = 0;

/************************************************************/
/*     Externally called functions (Notifications)          */
/************************************************************/

void zoom_view_changed(GDisplay *disp)
{
   // ignore this event if we are updating scale, because that indicates
   // that we caused the event to occur.
   if (zoom_updating_scale)
      return;

   // first get the active display if there is none
   if (!zoom_control || !zoom_control->gdisp || zoom_control->gdisp != disp) {
      return;
   }

   // indicate this event was generated externally
   zoom_external_generated = 1;
   
   // notify dialog's widgets to update zoom
   zoom_update_pulldown_slider(disp); 
   // notify drawing area to update rect.
   
   // reset state
   zoom_external_generated = 0;
}

void zoom_image_preview_changed(GImage *image)
{
   printf("Image changed\n");
}

void zoom_set_focus(GDisplay *gdisp)
{
   GDisplay *old_disp;
   old_disp = zoom_gdisp;
   zoom_gdisp = gdisp;
   if (zoom_control && zoom_gdisp && zoom_gdisp != old_disp) {
      zoom_control_activate();
   }
}

void zoom_control_close(GtkObject *wid, gpointer data)
{
   if (zoom_control) {
      zoom_control_delete(zoom_control);
      zoom_control = 0;
   }
}

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
   zoom_control_activate();
   
   // display it
   gtk_widget_show_all(zoom_control->window);
   return zoom_control;
}

ZoomControl * zoom_control_new()
{
  ZoomControl *zoom;
  GList *items = NULL;

  zoom = (ZoomControl *)malloc(sizeof(ZoomControl));
  zoom->gdisp = NULL;
  zoom->pixmap = NULL;
  zoom->window = NULL;  
  zoom->pull_down = NULL;
  zoom->slider = NULL;
  zoom->preview = NULL;
  zoom->adjust = NULL;  

  zoom->window = gtk_dialog_new();
  gtk_signal_connect(GTK_OBJECT(zoom->window), "destroy", 
                  GTK_SIGNAL_FUNC(zoom_control_close), 0);
  gtk_window_set_policy (GTK_WINDOW (zoom->window), FALSE, FALSE, FALSE);
  gtk_window_set_title (GTK_WINDOW (zoom->window), "Zoom/Pan Control");
  gtk_window_set_position(GTK_WINDOW(zoom->window), GTK_WIN_POS_MOUSE);
  gtk_object_sink(GTK_OBJECT(zoom->window));
  gtk_object_ref(GTK_OBJECT(zoom->window));

  zoom->pull_down = gtk_combo_new();

  items = g_list_append (items, "1:16");
  items = g_list_append (items, "1:15");
  items = g_list_append (items, "1:14");
  items = g_list_append (items, "1:13");
  items = g_list_append (items, "1:12");
  items = g_list_append (items, "1:11");
  items = g_list_append (items, "1:10");
  items = g_list_append (items, "1:9");
  items = g_list_append (items, "1:8");
  items = g_list_append (items, "1:7");
  items = g_list_append (items, "1:6");
  items = g_list_append (items, "1:5");
  items = g_list_append (items, "1:4");
  items = g_list_append (items, "1:3");
  items = g_list_append (items, "1:2");
  items = g_list_append (items, "1:1");
  items = g_list_append (items, "2:1");
  items = g_list_append (items, "3:1");
  items = g_list_append (items, "4:1");
  items = g_list_append (items, "5:1");
  items = g_list_append (items, "6:1");
  items = g_list_append (items, "7:1");
  items = g_list_append (items, "8:1");
  items = g_list_append (items, "9:1");
  items = g_list_append (items, "10:1");
  items = g_list_append (items, "11:1");
  items = g_list_append (items, "12:1");
  items = g_list_append (items, "13:1");
  items = g_list_append (items, "14:1");
  items = g_list_append (items, "15:1");
  items = g_list_append (items, "16:1");
  gtk_combo_set_popdown_strings (GTK_COMBO (zoom->pull_down), items);
  gtk_signal_connect(GTK_OBJECT(GTK_COMBO(zoom->pull_down)->list), "select-child", 
                  GTK_SIGNAL_FUNC(zoom_pulldown_value_changed), 0);

  zoom->adjust = GTK_ADJUSTMENT(gtk_adjustment_new(0, -16, 17, -1, 1, 1));
  zoom->slider = gtk_hscrollbar_new(zoom->adjust);
  gtk_signal_connect(GTK_OBJECT(zoom->adjust), "value-changed", 
                  GTK_SIGNAL_FUNC(zoom_slider_value_changed), 0);

  zoom->preview = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(zoom->preview), 128, 128);
  gtk_signal_connect (GTK_OBJECT (zoom->preview), "expose_event",
		      (GtkSignalFunc) zoom_preview_expose_event, NULL);
  gtk_signal_connect (GTK_OBJECT(zoom->preview),"configure_event",
		      (GtkSignalFunc) zoom_preview_configure_event, NULL);
  gtk_signal_connect (GTK_OBJECT (zoom->preview), "motion_notify_event",
		      (GtkSignalFunc) zoom_preview_motion_notify_event, NULL);
  gtk_signal_connect (GTK_OBJECT (zoom->preview), "button_press_event",
		      (GtkSignalFunc) zoom_preview_button_press_event, NULL);

 
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(zoom->window)->action_area), zoom->pull_down, TRUE, TRUE, 0); 
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(zoom->window)->action_area), zoom->slider, TRUE, TRUE, 0); 
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(zoom->window)->vbox), zoom->preview, TRUE, TRUE, 0); 

  gtk_widget_show(zoom->pull_down);
  gtk_widget_show(zoom->slider);
  gtk_widget_show(zoom->preview);
  // don't show it, it is up to the caller to manage visibility state information.

  return zoom;
}

void zoom_control_delete(ZoomControl *zoom)
{
   gtk_object_unref(GTK_OBJECT(zoom->window));
   free(zoom);
}

/************************************************************/
/*     Utility Functions                                    */
/************************************************************/

void zoom_update_pulldown_slider(GDisplay *disp)
{
   zoom_update_pulldown(disp);
   zoom_update_slider(disp);
}

void zoom_update_slider(GDisplay *disp)
{
   gfloat val;
   gint src, dst;

   zoom_updating_ui = 1;

   // extract the scale from the display, update the slider's value accordingly
   src = SCALESRC(disp);
   dst = SCALEDEST(disp);

   if (src == 1) {
      val = (-dst + 1); 
   }
   else {
      val = src - 1;
   }

   //printf("src %d dst %d val %f\n", src, dst, val);
   gtk_adjustment_set_value(zoom_control->adjust, -val);
   zoom_updating_ui = 0;
}

void zoom_update_pulldown(GDisplay *disp)
{
   GtkList *list = 0;
   gint src, dst;
   gint item;

   zoom_updating_ui = 1;
   list = GTK_LIST(GTK_COMBO(zoom_control->pull_down)->list);
   
   // extract the scale from the display, update the pulldown's value accordingly
   src = SCALESRC(disp);
   dst = SCALEDEST(disp);

   if (src == 1) {
      item = 14 + dst; 
   }
   else {
      item = 16 - src;
   }

   gtk_list_unselect_all(list);
   gtk_list_select_item(list, item);
   zoom_updating_ui = 0;
}

void zoom_update_scale(GDisplay *gdisp, gint scale_val)
{
   zoom_updating_scale = 1;
   change_scale(zoom_control->gdisp, scale_val);
   zoom_updating_scale = 0;
}



static int 
zoom_control_activate()
{
   GDisplay *disp;

   if (!zoom_control)
      return 0;
   
   // first get the active display if there is none
   disp = zoom_get_active_display();
   if (disp != zoom_control->gdisp) {
      zoom_control->gdisp = disp;
      zoom_view_changed(disp);
   }

   // if that failed, do nothing.
   if (!zoom_control->gdisp) {
      return 0;
   }

   return 1;
}

// Returns which display the zoom control should refer to
static GDisplay *
zoom_get_active_display()
{
   // if there are no displays, return 0
   if (!display_list)
      return NULL;

   // if we think we have one, check to make sure its still valid
   // if it's not still valid, forget about it
   if (zoom_gdisp && g_slist_find(display_list, (gpointer)zoom_gdisp)) {
      return zoom_gdisp; 
   }
   else {
      zoom_gdisp = 0;
   }

   // otherwise there's nothing to do, so just return the first one we find.
   zoom_gdisp = (GDisplay *)display_list->data;
   return zoom_gdisp; 
}

/************************************************************/
/*     Callback functions for the zoom dialog               */
/************************************************************/

// called when the value of the zoom slider changes
static void 
zoom_slider_value_changed(
   GtkAdjustment *adjustment,
   gpointer user_data)
{
   int scale_val;

   if (zoom_updating_ui || zoom_external_generated || zoom_updating_scale)
      return;

   if (!zoom_control_activate())
      return;

   // convert to an integer value, and round
   scale_val = (int) (-adjustment->value + .5);
   //printf("Raw value %f, int %d\n", -adjustment->value, scale_val);

   // need to pack scale_val into the weird format used by change_scale().  
   scale_val = scale_val < 0 ? (-scale_val + 1) * 100 + 1 :
                               (scale_val + 1) + 100;

    // now set the zoom factor for the display
   zoom_update_scale(zoom_control->gdisp, scale_val);
   zoom_update_pulldown(zoom_control->gdisp);
}

static void zoom_pulldown_value_changed(
   GtkList *list,
   GtkWidget *widget,
   gpointer user_data)
{
   int scale_val;
   int src_val;
   int dst_val;
   gchar src_buf[256];
   gchar dst_buf[256];
   gchar * text;

   if (zoom_updating_ui || zoom_external_generated || zoom_updating_scale)
      return;

   if (!widget)
      return;

   if (!zoom_control_activate())
      return;

   gtk_label_get(GTK_LABEL(GTK_BIN(widget)->child), &text);
   if (text[1] == ':') {
      dst_buf[0] = text[0];
      dst_buf[1] = '\0';
      strcpy(src_buf, text + 2);
   }
   else {
      dst_buf[0] = text[0];
      dst_buf[1] = text[1];
      dst_buf[2] = '\0';
      strcpy(src_buf, text + 3);
   }

   src_val = atoi(src_buf);
   dst_val = atoi(dst_buf);
   scale_val = dst_val * 100 + src_val;

   zoom_update_scale(zoom_control->gdisp, scale_val);
   zoom_update_slider(zoom_control->gdisp);
}


/************************************************************/
/*     Callback functions for preview widget events         */
/************************************************************/

static void zoom_clear_pixmap(GtkWidget *preview, GdkPixmap *pixmap)
{
   gdk_draw_rectangle(pixmap, preview->style->white_gc, TRUE, 0, 0, 
		   preview->allocation.width, preview->allocation.height);
   gdk_draw_line(pixmap,
		   preview->style->black_gc,
		   10, 10, 100,45);
}

static gboolean zoom_preview_expose_event(
   GtkWidget *widget,
   GdkEventExpose *event,
   gpointer user_data)
{
   if (!zoom_control || !zoom_control->pixmap) 
      return FALSE;

   gdk_draw_pixmap(widget->window,
                   widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                   zoom_control->pixmap,
                   event->area.x, event->area.y,
                   event->area.x, event->area.y,
                   event->area.width, event->area.height);
   return FALSE;
}

static gboolean zoom_preview_configure_event(
   GtkWidget *widget,
   GdkEventConfigure *event,
   gpointer user_data)
{
   if (!zoom_control)
      return FALSE;

   if (!zoom_control->pixmap) {
      // make one
      zoom_control->pixmap = 
	 gdk_pixmap_new(widget->window, widget->allocation.width, widget->allocation.height, -1);
      zoom_clear_pixmap(widget, zoom_control->pixmap);
   }

   return TRUE;
}

static gboolean zoom_preview_motion_notify_event(
   GtkWidget *widget,
   GdkEventMotion *event,
   gpointer user_data)
{
   return FALSE;
}

static gboolean zoom_preview_button_press_event(
   GtkWidget *widget,
   GdkEventButton *event,
   gpointer user_data)
{
   return FALSE;
}


