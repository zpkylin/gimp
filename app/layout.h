#ifndef __GIMP_LAYOUT_H__
#define __GIMP_LAYOUT_H__

#include <gtk/gtkwidget.h>

int  layout_save();
void layout_connect_window_position(GtkWidget *widget, int *x_var, int *y_var);
void layout_connect_window_visible(GtkWidget *widget, int *visible);
void layout_freeze_current_layout();
void layout_unfreeze_current_layout();
void layout_restore(); // call after the gimprc file has been loaded


#endif

