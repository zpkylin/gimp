#ifndef __GIMP_LAYOUT_H__
#define __GIMP_LAYOUT_H__

#include <gtk/gtkwidget.h>

int  layout_save();
void layout_connect_window_position(GtkWidget *widget, int *x_var, int *y_var);
void layout_connect_window_visible(GtkWidget *widget, int *visible);


#endif

