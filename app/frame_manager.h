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
  GtkWidget *istep;
  GtkWidget *iflip;
  GtkWidget *ibg;
  GtkWidget *isave;
  GtkWidget *label;
  GImage *gimage;
  GtkWidget *list_item;
  char flip;
  char bg;
  char save;
  char step;
  char selected;
  char active;
  GImage *bg_image;
}store_t;


struct _frame_manager_t 
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
  GtkWidget *store_option;
  GtkWidget *change_frame_num; 
  GtkWidget *warning;
  char onionskin; 

};

void frame_manager_create (GDisplay *gdisplay);
void frame_manager_free (GDisplay *gdisplay);
gint frame_manager_step_forward (GtkWidget *, gpointer);
gint frame_manager_step_backwards (GtkWidget *, gpointer);
gint frame_manager_flip_forward (GtkWidget *, gpointer);
gint frame_manager_flip_backwards (GtkWidget *, gpointer);
gint frame_manager_flip_raise (GtkWidget *, gpointer);
gint frame_manager_flip_lower (GtkWidget *, gpointer);
GimpDrawable *frame_manager_onionskin_drawable (); 
int frame_manager_onionskin_display (); 
void frame_manager_onionskin_offset (GDisplay *, int, int); 
void frame_manager_rm_onionskin (GDisplay *); 
void frame_manager_image_reload (GDisplay *, GImage *, GImage*);
GimpDrawable *frame_manager_bg_drawable (GDisplay *gdisplay);
int frame_manager_bg_display (GDisplay *);
void frame_manager_set_bg (GDisplay *);
void frame_manager_store_add (GImage *, GDisplay *, int);
store_t* frame_manager_store_new (GDisplay *gdisplay, GImage *gimage, char active);
GDisplay* frame_manager_load (GDisplay *, GImage *);
void frame_manager_set_dirty_flag (GDisplay *gdisplay, int); 

#endif /* __FRAME_MANAGER_H__ */
