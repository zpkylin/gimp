/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __FRAME_MANAGER_H__
#define __FRAME_MANAGER_H__

#include <gtk/gtkwidget.h>
#include "drawable.h"
#include "gdisplay.h"

typedef struct 
{
  GtkWidget *shell;
  GtkWidget *vbox; 
  GtkWidget *auto_save;
  GtkWidget *step_size;
  GtkWidget *jump_to;
  GtkWidget *trans;
  GtkWidget *flip_area;
  GtkWidget *link_menu;
  GtkWidget *link_option_menu;
  GtkWidget *store_list;
  GtkWidget *store_size;
  GtkWidget *b;
  GSList *stores; 
  char auto_save_on;
  GtkAdjustment *trans_data; 
  GDisplay *linked_display; 
  GDisplay *gdisplay; 
  GtkWidget *store_option;
}frame_manager_t;

void frame_manager_create (void);
void frame_manager_free (void);
gint frame_manager_step_forward (GtkWidget *, gpointer);
gint frame_manager_step_backwards (GtkWidget *, gpointer);
gint frame_manager_flip_forward (GtkWidget *, gpointer);
gint frame_manager_flip_backwards (GtkWidget *, gpointer);
gint frame_manager_flip_raise (GtkWidget *, gpointer);
gint frame_manager_flip_lower (GtkWidget *, gpointer);
GimpDrawable *frame_manager_onionskin_drawable (); 
int frame_manager_onionskin_display (); 
void frame_manager_onionskin_offset (int, int); 
void frame_manager_rm_onionskin (frame_manager_t *); 
void frame_manager_image_reload (GImage *, GImage*);
GimpDrawable *frame_manager_bg_drawable ();
int frame_manager_bg_display ();
void frame_manager_set_bg (GDisplay *display);

#endif /* __FRAME_MANAGER_H__ */
