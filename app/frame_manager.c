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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "frame_manager.h"
#include "gimage.h"
#include "ops_buttons.h"
#include "general.h"
#include "interface.h"
#include "actionarea.h"
#include "tools.h"
#include "rect_selectP.h"
#include "layer.h"
#include "layer_pvt.h"
#include "clone.h"
#include "fileops.h"
#include "layout.h"
#include "gimprc.h"

#include "tools/forward.xpm"
#include "tools/forward_is.xpm"
#include "tools/backwards.xpm"
#include "tools/backwards_is.xpm"
#include "tools/raise.xpm"
#include "tools/raise_is.xpm"
#include "tools/lower.xpm"
#include "tools/lower_is.xpm"
#include "tools/delete.xpm"
#include "tools/delete_is.xpm"
#include "tools/area.xpm"
#include "tools/area_is.xpm"
#include "tools/update.xpm"
#include "tools/update_is.xpm"
#include "tools/istep.xbm"
#include "tools/iflip.xbm"
#include "tools/ibg.xbm"
#include "tools/isave.xbm"

#define STORE_LIST_WIDTH  	200
#define STORE_LIST_HEIGHT	150 
#define	STORE_MAX_NUM		6
#define BUTTON_EVENT_MASK  GDK_EXPOSURE_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | \
GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK


#define NORMAL 0
#define SELECTED 1
#define INSENSITIVE 2



/* functions */
static gint frame_manager_step_size (GtkWidget *, gpointer);

static gint frame_manager_jump_to (GtkWidget *, gpointer);
static void frame_manager_jump_to_update (char *, GDisplay*);
static gint frame_manager_auto_save (GtkWidget *, gpointer);

static gint frame_manager_flip_area (GtkWidget *, gpointer);
static gint frame_manager_update_whole_area (GtkWidget *, gpointer);
static gint frame_manager_flip_delete (GtkWidget *, gpointer);

static void frame_manager_next_filename (GImage *, char **, char **, char, GDisplay *);
static char* frame_manager_get_name (GImage *image);
static char* frame_manager_get_frame (GImage *image);
static char* frame_manager_get_ext (GImage *image);
static void frame_manager_this_filename (GImage *, char **, char **, char *);

static GtkWidget* frame_manager_create_menu (GImage *);
static void frame_manager_update_menu (GDisplay *gdisplay);
static gint frame_manager_link_menu (GtkWidget *, GdkEvent*, gpointer);

static gint frame_manager_store_list (GtkWidget *, GdkEvent *);
static gint frame_manager_store_size (GtkWidget *, gpointer);
static gint frame_manager_store_select_update (GtkWidget *, gpointer);
static gint frame_manager_store_deselect_update (GtkWidget *, gpointer);
static gint frame_manager_store_step_button (GtkWidget *, GdkEvent *);
static gint frame_manager_store_flip_button (GtkWidget *, GdkEvent *);
static gint frame_manager_store_bg_button (GtkWidget *, GdkEvent *, gpointer);
static gint frame_manager_store_save_button (GtkWidget *, GdkEvent *);
static void frame_manager_store_delete (store_t*, GDisplay*, char);
static void frame_manager_store_unselect (GDisplay*);
static void frame_manager_store_load (store_t *item, GDisplay*);
static void frame_manager_store_clear (GDisplay*);
static void frame_manager_store_new_option (GtkWidget *, gpointer);

static void frame_manager_flip_redraw (store_t *);
static void frame_manager_step_redraw (store_t *);
static void frame_manager_bg_redraw (store_t *);
static void frame_manager_save_redraw (store_t *);
static void frame_manager_save (GImage *, GDisplay*);

static gint frame_manager_transparency (GtkAdjustment *, gpointer);
static gint frame_manager_opacity_00 (GtkWidget *, gpointer);
static gint frame_manager_opacity_25 (GtkWidget *, gpointer);
static gint frame_manager_opacity_05 (GtkWidget *, gpointer);
static gint frame_manager_opacity_75 (GtkWidget *, gpointer);
static gint frame_manager_opacity_10 (GtkWidget *, gpointer);

static gint frame_manager_close (GtkWidget *, gpointer);

static void frame_manager_link_forward (GDisplay*);
static void frame_manager_link_backward (GDisplay*);

static int frame_manager_check (GDisplay*);
static int frame_manager_get_bg_id (GDisplay*);

static int create_warning (char*, GDisplay*);
static gint warning_close (GtkWidget *, gpointer);
static gint warning_ok (GtkWidget *, gpointer);

static void step_forward (GDisplay *);
static void step_backwards (GDisplay *);
static void flip_delete (GDisplay *);

static ActionAreaItem action_items[] =
{
  { "Close", frame_manager_close, NULL, NULL }  
};


static OpsButton step_button[] =
{
  { backwards_xpm, backwards_is_xpm, frame_manager_step_backwards, "Step Backwards", NULL, NULL, NULL, NULL, NULL, NULL },
    { forward_xpm, forward_is_xpm, frame_manager_step_forward, "Step Forward", NULL, NULL, NULL, NULL, NULL, NULL },
      { NULL, 0, 0, NULL, NULL, NULL, NULL }
};

static OpsButton flip_button[] =
{
  { backwards_xpm, backwards_is_xpm, frame_manager_flip_backwards, "Flip Backwards", NULL, NULL, NULL, NULL, NULL, NULL },
  { forward_xpm, forward_is_xpm, frame_manager_flip_forward, "Flip Forward", NULL, NULL, NULL, NULL, NULL, NULL },
  { raise_xpm, raise_is_xpm, frame_manager_flip_raise, "Raise", NULL, NULL, NULL, NULL, NULL, NULL },
  { lower_xpm, lower_is_xpm, frame_manager_flip_lower, "Lower", NULL, NULL, NULL, NULL, NULL, NULL },
  { area_xpm, area_is_xpm, frame_manager_flip_area, "Set Area", NULL, NULL, NULL, NULL, NULL, NULL },
  { update_xpm, update_is_xpm, frame_manager_update_whole_area, "Update Whole Area", NULL, NULL, NULL, NULL, NULL, NULL },
  { delete_xpm, delete_is_xpm, frame_manager_flip_delete, "Delete", NULL, NULL, NULL, NULL, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};


/* variables */
static store_t store[STORE_MAX_NUM];
static GdkPixmap *istep_pixmap[3] = { NULL, NULL, NULL };
static GdkPixmap *iflip_pixmap[3] = { NULL, NULL, NULL };
static GdkPixmap *ibg_pixmap[3] = { NULL, NULL, NULL };
static GdkPixmap *isave_pixmap[3] = { NULL, NULL, NULL };
static int s_x, s_y, e_x, e_y;
static int NEW_OPTION=0;
static char fm_continue = 0;
static char dont_change_frame=0;

typedef struct
{
  int img_id;
  GDisplay *gdisplay; 
}menu_t;

void
frame_manager_free (GDisplay *gdisplay)
{
  if (gdisplay->frame_manager)
    {
      frame_manager_rm_onionskin (gdisplay); 
      frame_manager_store_clear (gdisplay); 

      if (GTK_WIDGET_VISIBLE (gdisplay->frame_manager->shell))
	gtk_widget_hide (gdisplay->frame_manager->shell);
      g_free (gdisplay->frame_manager);

      gdisplay->frame_manager = NULL;
    }
}

void
frame_manager_create (GDisplay *gdisplay)
{

  GtkWidget *vbox, *hbox, *lvbox, *rvbox, *label, *separator, *button_box, *listbox,
  *utilbox, *slider, *button, *abox;

  double scale;
  char tmp[256], *temp;

  if (!gdisplay->frame_manager)
    {
      if (!gdisplay)
	{
	  gdisplay->frame_manager = NULL;
	  return; 
	}

      if (!(gdisplay->gimage->filename))
	{
	  gdisplay->frame_manager = NULL;
	  return; 
	}


      gdisplay->frame_manager = (frame_manager_t*) g_malloc (sizeof (frame_manager_t));

      if (gdisplay->frame_manager == NULL)
	return; 

      gdisplay->framemanager = 1; 

      gdisplay->frame_manager->store_option = NULL;
      gdisplay->frame_manager->change_frame_num = NULL;
      gdisplay->frame_manager->warning = NULL;
      gdisplay->frame_manager->onionskin = 0;

      scale = ((double) SCALESRC (gdisplay) / (double)SCALEDEST (gdisplay));
      s_x = 0;
      s_y = 0;
      e_x = gdisplay->disp_width*scale;
      e_y = gdisplay->disp_height*scale;

      /* the shell */
      gdisplay->frame_manager->shell = gtk_dialog_new ();
      gtk_window_set_wmclass (GTK_WINDOW (gdisplay->frame_manager->shell), "gdisplay->frame_manager", "Gimp");
      gtk_window_set_policy (GTK_WINDOW (gdisplay->frame_manager->shell), FALSE, TRUE, FALSE);
      sprintf (tmp, "Frame Manager for %s\0", gdisplay->gimage->filename); 
      gtk_window_set_title (GTK_WINDOW (gdisplay->frame_manager->shell), tmp);
      gtk_widget_set_uposition(gdisplay->frame_manager->shell, frame_manager_x, frame_manager_y);
      layout_connect_window_position(gdisplay->frame_manager->shell, &frame_manager_x, &frame_manager_y);

      /* vbox */
      vbox = gtk_vbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (gdisplay->frame_manager->shell)->vbox), vbox, TRUE, TRUE, 0);
      gtk_widget_show (vbox);

      /* hbox */
      hbox = gtk_hbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (hbox), 1);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);

      /* vbox */
      lvbox = gtk_vbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (lvbox), 1);
      gtk_box_pack_start (GTK_BOX (hbox), lvbox, TRUE, TRUE, 0);
      gtk_widget_show (lvbox);

      rvbox = gtk_vbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (rvbox), 1);
      gtk_box_pack_start (GTK_BOX (hbox), rvbox, TRUE, TRUE, 0);
      gtk_widget_show (rvbox);


      /*** left side ***/
      /* auto save */
      gdisplay->frame_manager->auto_save = gtk_toggle_button_new_with_label ("auto save");
      gtk_box_pack_start (GTK_BOX (lvbox), gdisplay->frame_manager->auto_save, FALSE, FALSE, 2);
      gtk_signal_connect (GTK_OBJECT (gdisplay->frame_manager->auto_save), "toggled",
	  (GtkSignalFunc) frame_manager_auto_save,
	  gdisplay);
      gtk_widget_show (gdisplay->frame_manager->auto_save);  
      gdisplay->frame_manager->auto_save_on = 0; 

      /* line */
      separator = gtk_hseparator_new (); 
      gtk_box_pack_start (GTK_BOX (lvbox), separator, FALSE, FALSE, 4);
      gtk_widget_show (separator); 

      /* step size */
      hbox = gtk_hbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (hbox), 1);
      gtk_box_pack_start (GTK_BOX (lvbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Step Size"); 
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
      gtk_widget_show (label);

      /* arrow buttons */
      gdisplay->frame_manager->step_size = gtk_spin_button_new (gtk_adjustment_new (1, 1, 100, 1, 1, 0), 
	  1.0, 0);
      gtk_box_pack_start (GTK_BOX (hbox), gdisplay->frame_manager->step_size, FALSE, FALSE, 2);
      /*gtk_widget_set_events (gdisplay->frame_manager->step_size, GDK_ENTER_NOTIFY_MASK|GDK_LEAVE_NOTIFY_MASK);
	gtk_signal_connect (GTK_OBJECT (gdisplay->frame_manager->step_size), "event",
	(GtkSignalFunc) frame_manager_step_size,
	gdisplay);
       */gtk_widget_show (gdisplay->frame_manager->step_size);


      /* step buttons */
      button_box = ops_button_box_new2 (gdisplay->frame_manager->shell, tool_tips, 
	  step_button, gdisplay);
      gtk_box_pack_start (GTK_BOX (lvbox), button_box, FALSE, FALSE, 2);
      gtk_widget_show (button_box);




      /* line */
      separator = gtk_hseparator_new (); 
      gtk_box_pack_start (GTK_BOX (lvbox), separator, FALSE, FALSE, 4);
      gtk_widget_show (separator); 

      /* link */
      hbox = gtk_hbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (hbox), 1);
      gtk_box_pack_start (GTK_BOX (lvbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Link:");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
      gtk_widget_show (label);

      gdisplay->frame_manager->link_option_menu = gtk_option_menu_new ();

      gdisplay->frame_manager->link_menu = frame_manager_create_menu (gdisplay->gimage);
      gtk_signal_connect (GTK_OBJECT (gdisplay->frame_manager->link_option_menu), 
	  "button_press_event",
	  (GtkSignalFunc) frame_manager_link_menu, 
	  gdisplay);

      gtk_box_pack_start (GTK_BOX (hbox), 
	  gdisplay->frame_manager->link_option_menu, TRUE, TRUE, 2);
      gtk_widget_show (gdisplay->frame_manager->link_option_menu);
      gtk_option_menu_set_menu (GTK_OPTION_MENU (gdisplay->frame_manager->link_option_menu), 
	  gdisplay->frame_manager->link_menu);

      gdisplay->frame_manager->linked_display = NULL;


      /*** right side ***/

      /* store list */
      gdisplay->frame_manager->stores = NULL; 

      /* store area */
      listbox = gtk_scrolled_window_new (NULL, NULL);
      gtk_widget_set_usize (listbox, STORE_LIST_WIDTH, STORE_LIST_HEIGHT);
      gtk_box_pack_start (GTK_BOX (rvbox), listbox, TRUE, TRUE, 2);

      gdisplay->frame_manager->store_list = gtk_list_new ();
      gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (listbox), gdisplay->frame_manager->store_list);
      gtk_list_set_selection_mode (GTK_LIST (gdisplay->frame_manager->store_list), GTK_SELECTION_SINGLE);
      gtk_signal_connect (GTK_OBJECT (gdisplay->frame_manager->store_list), "event",
	  (GtkSignalFunc) frame_manager_store_list,
	  gdisplay);
      gtk_container_set_focus_vadjustment (GTK_CONTAINER (gdisplay->frame_manager->store_list),
	  gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (listbox)));
      GTK_WIDGET_UNSET_FLAGS (GTK_SCROLLED_WINDOW (listbox)->vscrollbar, GTK_CAN_FOCUS);

      gtk_widget_show (gdisplay->frame_manager->store_list);
      gtk_widget_show (listbox);

      /* line */
      separator = gtk_hseparator_new (); 
      gtk_box_pack_start (GTK_BOX (lvbox), separator, FALSE, FALSE, 4);
      gtk_widget_show (separator); 

      /* trans slider */
      utilbox = gtk_hbox_new (FALSE, 1);
      gtk_box_pack_start (GTK_BOX (rvbox), utilbox, FALSE, FALSE, 0);
      gtk_widget_show (utilbox);

      label = gtk_label_new ("Opacity:");
      gtk_box_pack_start (GTK_BOX (utilbox), label, FALSE, FALSE, 2);
      gtk_widget_show (label);

      gdisplay->frame_manager->trans_data = GTK_ADJUSTMENT (gtk_adjustment_new (1.0, 0.0, 1.0, .01, .01, 0.0));
      slider = gtk_hscale_new (gdisplay->frame_manager->trans_data);
      gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_CONTINUOUS);
      gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_RIGHT);
      gtk_box_pack_start (GTK_BOX (utilbox), slider, TRUE, TRUE, 0);
      gtk_signal_connect (GTK_OBJECT (gdisplay->frame_manager->trans_data), "value_changed",
	  (GtkSignalFunc) frame_manager_transparency,
	  gdisplay);
      gtk_widget_show (slider);

      /* opacity buttons */
      hbox = gtk_hbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (hbox), 1);
      gtk_box_pack_start (GTK_BOX (rvbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);

      button = gtk_button_new_with_label ("0.0");
      gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
      gtk_widget_show (button);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
	  (GtkSignalFunc) frame_manager_opacity_00,
	  gdisplay);
      button = gtk_button_new_with_label (".25");
      gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
      gtk_widget_show (button);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
	  (GtkSignalFunc) frame_manager_opacity_25,
	  gdisplay);
      button = gtk_button_new_with_label ("0.5");
      gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
      gtk_widget_show (button);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
	  (GtkSignalFunc) frame_manager_opacity_05,
	  gdisplay);
      button = gtk_button_new_with_label (".75");
      gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
      gtk_widget_show (button);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
	  (GtkSignalFunc) frame_manager_opacity_75,
	  gdisplay);
      button = gtk_button_new_with_label ("1.0");
      gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
      gtk_widget_show (button);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
	  (GtkSignalFunc) frame_manager_opacity_10,
	  gdisplay);


      /* line */
      separator = gtk_hseparator_new (); 
      gtk_box_pack_start (GTK_BOX (rvbox), separator, FALSE, FALSE, 4);
      gtk_widget_show (separator); 

      /* num of store */
      hbox = gtk_hbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (hbox), 1);
      gtk_box_pack_start (GTK_BOX (rvbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Number of store to add"); 
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
      gtk_widget_show (label);

      /* arrow buttons */
      gdisplay->frame_manager->store_size = gtk_spin_button_new (gtk_adjustment_new (1, 1, 100, 1, 1, 0), 
	  1.0, 0);
      gtk_box_pack_start (GTK_BOX (hbox), gdisplay->frame_manager->store_size, FALSE, FALSE, 2);

      gtk_signal_connect (GTK_OBJECT (gdisplay->frame_manager->store_size), "activate",
	  (GtkSignalFunc) frame_manager_store_size,
	  gdisplay);
      gtk_widget_show (gdisplay->frame_manager->store_size);

      button = gtk_button_new_with_label ("Apply");
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 2);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
	  (GtkSignalFunc) frame_manager_store_size,
	  gdisplay);
      gtk_widget_show (button);


      /* buttons */
      button_box = ops_button_box_new2 (gdisplay->frame_manager->shell, tool_tips, 
	  flip_button, gdisplay);
      gtk_box_pack_start (GTK_BOX (rvbox), button_box, FALSE, FALSE, 2);
      gtk_widget_show (button_box);

      /* line */
      separator = gtk_hseparator_new (); 
      gtk_box_pack_start (GTK_BOX (rvbox), separator, FALSE, FALSE, 4);
      gtk_widget_show (separator); 

      /* Close */
      action_items[0].user_data = gdisplay;
      build_action_area (GTK_DIALOG (gdisplay->frame_manager->shell), action_items, 1, 0);


      dont_change_frame = 0; 
      frame_manager_store_add (gdisplay->gimage, gdisplay, 0);
      dont_change_frame = 1; 
      gtk_widget_show (gdisplay->frame_manager->shell);

      if (active_tool->type == CLONE)
	{
	  if (clone_is_src (gdisplay))
	    frame_manager_set_bg (gdisplay); 
	}

    }
  else
    {
      if (!gdisplay)
	{
	  return; 
	}
      if (!GTK_WIDGET_VISIBLE (gdisplay->frame_manager->shell))
	{
	  if (gdisplay)
	    {
	      gdisplay->framemanager = 1;
	      gtk_widget_show (gdisplay->frame_manager->shell);
	      dont_change_frame = 0; 
	      frame_manager_store_add (gdisplay->gimage, gdisplay, 0); 
	      dont_change_frame = 1; 
	      if (active_tool->type == CLONE)
		{
		  if (clone_is_src (gdisplay))
		    frame_manager_set_bg (gdisplay); 
		}


	    }
	}
      else
	{
	  if (gdisplay)
	    {
	      gdk_window_raise(gdisplay->frame_manager->shell->window);
	    }
	}
    }
}

static gint 
frame_manager_auto_save (GtkWidget *w, gpointer client_data)
{
  frame_manager_rm_onionskin (((GDisplay*) client_data)); 
  return TRUE; 
}

static GImage *
frame_manager_find_gimage (char *whole, GDisplay *gdisplay)
{
  GSList *list=NULL;
  store_t *item;

  list = gdisplay->frame_manager->stores;
  while (list)
    {
      item = (store_t*) list->data;
      if (strcmp (item->gimage->filename, whole))
	return item->gimage;
    }
  return NULL; 
}

gint 
frame_manager_step_forward (GtkWidget *w, gpointer client_data)
{
  GSList *list=NULL;
  store_t *item;
  char flag=0;
  GDisplay *gdisplay = (GDisplay*) client_data;

  frame_manager_t *fm = gdisplay->frame_manager;

  if (!frame_manager_check (gdisplay))
    {
      return 1; 
    }
  frame_manager_rm_onionskin (gdisplay); 



  /* warning dialog */
  if (!gtk_toggle_button_get_active((GtkToggleButton*)fm->auto_save))
    {
      list = fm->stores;
      while (list)
	{
	  item = (store_t*) list->data;
	  if (item->gimage->dirty)
	    flag = 1;
	  list = g_slist_next (list);	
	}
    }
  if (flag)
    {
      fm_continue = 0; 
      create_warning ("Not all stores are saved. Do you really want to step forward\0",
	  gdisplay);
    }
  else 
    {
      step_forward (gdisplay); 
    }
  return TRUE; 

}

static void 
step_forward (GDisplay *gdisplay)
{
  char *whole, *raw, autosave;
  int num=0, i=0, j=0, gi[7];
  GSList *list=NULL;
  store_t *item;
  GImage *gimages[7], *gimage; 

  frame_manager_t *fm = gdisplay->frame_manager;
  
  autosave = gtk_toggle_button_get_active((GtkToggleButton*)fm->auto_save);


  whole = (char*) malloc (sizeof(char)*255);
  raw = (char*) malloc (sizeof(char)*255);

  /*
     loop through the stores
     if step is checked
     exchange that store
   */
  list = fm->stores;
  if (gtk_toggle_button_get_active((GtkToggleButton*)fm->auto_save))
    {
      while (list)
	{
	  item = (store_t*) list->data;
	  gi[i] = 0; 
	  gimages[i++] = item->gimage;
	  list = g_slist_next (list);
	}
    }
  list = fm->stores;
  while (list)
    {
      item = (store_t*) list->data;
      if (item->step)
	{
	  frame_manager_next_filename (item->gimage, &whole, &raw, 1, gdisplay);
	  gimage = NULL; 
	  for (j=0; j<i; j++)
	    {
	      if (!strcmp (gimages[j]->filename, whole))
		{
		  gimage = gimages[j];
		  gi[j] = 1; 
		  j = i;
		}
	    }
	  if (item->selected)
	    {
	      if (gimage)
		{
		  gdisplay->gimage = gimage;
		  gdisplay->ID = gdisplay->gimage->ID;
		  dont_change_frame = 0;
		  frame_manager_store_add (gdisplay->gimage, gdisplay, num);
		  dont_change_frame = 1; 
/*		  frame_manager_link_forward (gdisplay);
*/		}
	      else
		if (file_load (whole, raw, gdisplay))
		  {
		    gdisplay->ID = gdisplay->gimage->ID;
		    dont_change_frame = 0; 
		    frame_manager_store_add (gdisplay->gimage, gdisplay, num);
		    dont_change_frame = 1; 
/*		    frame_manager_link_forward (gdisplay);
*/		  }
		else
		  {
		    printf ("ERROR\n");
		  }
	    }
	  else
	    {
	      if (gimage)
		{
		  dont_change_frame = 0; 
		  frame_manager_store_add (gimage, gdisplay, num);
		  dont_change_frame = 1; 
		}
	      else
		if ((gimage = file_load_without_display (whole, raw, gdisplay)))
		  {
		    dont_change_frame = 0; 
		    frame_manager_store_add (gimage, gdisplay, num);
		    dont_change_frame = 1; 
		  }
		else
		  {
		    printf ("ERROR\n");
		  }
	    }
	}
      num ++; 
      list = g_slist_next (list); 
    }
  frame_manager_link_forward (gdisplay);
  for (j=0; j<i; j++)
    {
      if (!gi[j])
	{
	  file_save (gimages[j]->ID, gimages[j]->filename,
	      prune_filename (gimages[j]->filename));
	  gimage_delete (gimages[j]); 	
	}
    }
  free (whole);
  free (raw); 


}

gint 
frame_manager_step_backwards (GtkWidget *w, gpointer client_data)
{
  GSList *list=NULL;
  store_t *item;
  char flag=0;
  GDisplay *gdisplay = (GDisplay*) client_data;

  frame_manager_t *fm = gdisplay->frame_manager;

  if (!frame_manager_check (gdisplay))
    {
      return 1; 
    }
  frame_manager_rm_onionskin (gdisplay); 



  /* warning dialog */
  if (!gtk_toggle_button_get_active((GtkToggleButton*)fm->auto_save))
    {
      list = fm->stores;
      while (list)
	{
	  item = (store_t*) list->data;
	  if (item->gimage->dirty)
	    flag = 1;
	  list = g_slist_next (list);	
	}
    }
  if (flag)
    {

      fm_continue = 1; 
      create_warning ("Not all stores are saved. Do you really want to step forward\0",
	  gdisplay);
    }
  else 
    {
      step_backwards (gdisplay); 
    }
  return TRUE; 

}

static void
step_backwards (GDisplay *gdisplay)
{
  frame_manager_t *fm = gdisplay->frame_manager; 
  char *whole, *raw, autosave;
  int num=0, i=0, j=0, gi[7];
  GSList *list=NULL;
  store_t *item;
  GImage *gimages[7], *gimage; 

  autosave = gtk_toggle_button_get_active((GtkToggleButton*)fm->auto_save);


  whole = (char*) malloc (sizeof(char)*255);
  raw = (char*) malloc (sizeof(char)*255);

  /*
     loop through the stores
     if step is checked
     exchange that store
   */
  list = fm->stores;
  if (gtk_toggle_button_get_active((GtkToggleButton*)fm->auto_save))
    {
      while (list)
	{
	  item = (store_t*) list->data;
	  gi[i] = 0; 
	  gimages[i++] = item->gimage;
	  list = g_slist_next (list);
	}
    }
  list = fm->stores;
  while (list)
    {
      item = (store_t*) list->data;
      if (item->step)
	{
	  frame_manager_next_filename (item->gimage, &whole, &raw, 0, gdisplay);
	  gimage = NULL; 
	  for (j=0; j<i; j++)
	    {
	      if (!strcmp (gimages[j]->filename, whole))
		{
		  gimage = gimages[j];
		  gi[j] = 1; 
		  j = i;
		}
	    }
	  if (item->selected)
	    {
	      if (gimage)
		{
		  gdisplay->gimage = gimage;
		  gdisplay->ID = gdisplay->gimage->ID;
		  dont_change_frame = 0; 
		  frame_manager_store_add (gdisplay->gimage, gdisplay, num);
		  dont_change_frame = 1; 
/*		  frame_manager_link_backward (gdisplay);
*/		}
	      else
		if (file_load (whole, raw, gdisplay))
		  {
		    gdisplay->ID = gdisplay->gimage->ID;
		    dont_change_frame = 0; 
		    frame_manager_store_add (gdisplay->gimage, gdisplay, num);
		    dont_change_frame = 1; 
/*		    frame_manager_link_backward (gdisplay);
*/		  }
		else
		  {
		    printf ("ERROR\n");
		  }
	    }
	  else
	    {
	      if (gimage)
		{
		  dont_change_frame = 0; 
		  frame_manager_store_add (gimage, gdisplay, num);
		  dont_change_frame = 1; 
		}
	      else
		if ((gimage = file_load_without_display (whole, raw, gdisplay)))
		  {
		    dont_change_frame = 0; 
		    frame_manager_store_add (gimage, gdisplay, num);
		    dont_change_frame = 1; 
		  }
		else
		  {
		    printf ("ERROR\n");
		  }
	    }
	}
      num ++; 
      list = g_slist_next (list); 
    }
  for (j=0; j<i; j++)
    {
      if (!gi[j])
	{
	  file_save (gimages[j]->ID, gimages[j]->filename,
	      prune_filename (gimages[j]->filename));
	  gimage_delete (gimages[j]); 	
	}
    }
  frame_manager_link_backward (gdisplay);
  free (whole);
  free (raw); 


}

static gint 
frame_manager_step_size (GtkWidget *w, gpointer client_data)
{
  frame_manager_rm_onionskin (((GDisplay*)client_data)); 
  return TRUE; 

}

static gint 
frame_manager_jump_to (GtkWidget *w, gpointer client_data)
{
  char *whole, *raw;
  int num=1, i;
  store_t *item; 
  GDisplay *gdisplay = (GDisplay*) client_data;
  frame_manager_t *fm = gdisplay->frame_manager;
  GSList *item_list=NULL;
  if (!frame_manager_check (gdisplay))
    return 1; 
  frame_manager_rm_onionskin (gdisplay); 



  whole = (char*) malloc (sizeof(char)*255);
  raw = (char*) malloc (sizeof(char)*255);

  frame_manager_this_filename (gdisplay->gimage, &whole, &raw, 
      gtk_entry_get_text (fm->jump_to));
  if (file_load (whole, raw, gdisplay))
    {
      gdisplay->ID = gdisplay->gimage->ID;
      item_list =  (GList*) GTK_LIST(
	  gdisplay->frame_manager->store_list)->selection;
      if (item_list)
	{
	  i = gtk_list_child_position (GTK_LIST (fm->store_list), item_list->data);
	  item = (store_t*) g_slist_nth (fm->stores, i)->data;
	  dont_change_frame = 0; 
	  if (gtk_toggle_button_get_active((GtkToggleButton*)fm->auto_save) &&
	      item->gimage->dirty)
	    file_save (item->gimage->ID, item->gimage->filename,
		prune_filename (item->gimage->filename));
	  frame_manager_store_add (gdisplay->gimage, gdisplay, i);  
	  dont_change_frame = 1; 
	}
    }
  else
    {
      printf ("ERROR\n");
    }
  free (whole);
  free (raw); 

  return TRUE; 
}

static void
frame_manager_jump_to_update (char *i, GDisplay *gdisplay)
{
  gtk_entry_set_text (gdisplay->frame_manager->jump_to, i); 
}


gint 
frame_manager_flip_forward (GtkWidget *w, gpointer client_data)
{

  GSList *list=NULL;
  int flag=0;
  GList *item_list=NULL; 
  store_t *item, *first=NULL, *cur;
  int num;

  GDisplay *gdisplay = (GDisplay*) client_data;

  frame_manager_t *fm = gdisplay->frame_manager;
  if (!frame_manager_check (gdisplay))
    return; 


  if (fm->onionskin)
    {
      item_list =  (GList*) GTK_LIST(fm->store_list)->selection;
      num = gtk_list_child_position (GTK_LIST (fm->store_list), item_list->data);
      item = (store_t*) g_slist_nth (fm->stores, num)->data;
      item->gimage->active_layer->opacity = item->gimage->active_layer->opacity ? 0 : 1;
      drawable_update (GIMP_DRAWABLE(item->gimage->active_layer),
	  s_x, s_y, e_x, e_y);
      gdisplays_flush ();
      gtk_adjustment_set_value (fm->trans_data, item->gimage->active_layer->opacity);
      return 1;
    }

  list = fm->stores; 

  /* find the next active item */ 

  while (list)
    {
      item = (store_t*) list->data;
      if (!first && item->flip)
	{
	  first = item;
	}
      if (flag && item->flip)
	{
	  item->selected = 1;
	  frame_manager_store_load (item, gdisplay); 
	  return; 
	}
      if (item->selected)
	{
	  item->selected = 0;
	  cur = item;  
	  flag = 1;
	}
      list = g_slist_next (list);

    }
  if (first)
    {
      first->selected = 1;
      frame_manager_store_load (first, gdisplay);
    }
  else
    {
      cur->selected = 1;
    }

  return TRUE;
}

gint 
frame_manager_flip_backwards (GtkWidget *w, gpointer client_data)
{
  GSList *list=NULL;
  int flag=0, num;
  store_t *item, *prev=NULL, *next=NULL, *cur;
  GDisplay *gdisplay = (GDisplay*) client_data; 
  frame_manager_t *fm = gdisplay->frame_manager;
  GList *item_list=NULL;


  if (!frame_manager_check (gdisplay))
    return; 

  if (fm->onionskin)
    {
      item_list =  (GList*) GTK_LIST(fm->store_list)->selection;
      num = gtk_list_child_position (GTK_LIST (fm->store_list), item_list->data);
      item = (store_t*) g_slist_nth (fm->stores, num)->data;
      item->gimage->active_layer->opacity = item->gimage->active_layer->opacity ? 0 : 1;
      drawable_update (GIMP_DRAWABLE(item->gimage->active_layer),
	  s_x, s_y, e_x, e_y);
      gdisplays_flush ();
      gtk_adjustment_set_value (fm->trans_data, item->gimage->active_layer->opacity);
      return 1;
    }
  list = fm->stores; 

  /* find the next active item */ 
  while (list)
    {
      item = (store_t*) list->data;
      if (flag  && item->flip)
	{
	  next = item;
	}
      if (item->selected)
	{
	  item->selected = 0;
	  cur = item; 
	  flag = 1;
	}
      if (!flag && item->flip)
	{
	  prev = item;
	}
      list = g_slist_next (list);
    }
  if (prev)
    {
      prev->selected = 1;
      frame_manager_store_load (prev, gdisplay);
    }
  else if (next)
    {
      next->selected = 1;
      frame_manager_store_load (next, gdisplay); 
    }
  else
    {
      cur->selected = 1;
    }
  return TRUE;
}

gint 
frame_manager_flip_raise (GtkWidget *w, gpointer client_data)
{
  int i, j;
  store_t *item;
  GList *item_list=NULL, *list=NULL;
  GDisplay *gdisplay = (GDisplay*) client_data;
  frame_manager_t *fm = gdisplay->frame_manager;
  if (!frame_manager_check (gdisplay))
    return FALSE; 
  frame_manager_rm_onionskin (gdisplay); 
  item_list =  (GList*) GTK_LIST(fm->store_list)->selection;

  dont_change_frame = 0;
  
  if (item_list && g_slist_length (fm->stores) > 1)
    {
      i = gtk_list_child_position (GTK_LIST (fm->store_list), item_list->data);
      j = i - 1 > 0 ? i - 1 : 0;
      item = (store_t*) g_slist_nth (fm->stores, i)->data;
      list = g_list_append (list, item->list_item);
      gtk_list_remove_items (GTK_LIST (fm->store_list), list);
      fm->stores = g_slist_remove(fm->stores, item);
      gtk_list_insert_items (GTK_LIST (fm->store_list), list, j);
      fm->stores = g_slist_insert(fm->stores, item, j);
      gtk_list_select_item (GTK_LIST (fm->store_list), j); 
    }
  dont_change_frame = 1;
  return TRUE; 
}

gint 
frame_manager_flip_lower (GtkWidget *w, gpointer client_data)
{
  int i, j;
  store_t *item;
  GList *item_list=NULL, *list=NULL;
  GDisplay *gdisplay = (GDisplay*) client_data;
  frame_manager_t *fm = gdisplay->frame_manager;
  if (!frame_manager_check (gdisplay))
    return FALSE; 
  frame_manager_rm_onionskin (gdisplay); 
  item_list =  (GList*) GTK_LIST(fm->store_list)->selection;

  dont_change_frame = 0;
  if (item_list && g_slist_length (fm->stores) > 1)
    {
      i = gtk_list_child_position (GTK_LIST (fm->store_list), item_list->data);
      j = i + 1 < g_slist_length (fm->stores) ? i + 1 : i;  
      item = (store_t*) g_slist_nth (fm->stores, i)->data;
      list = g_list_append (list, item->list_item);
      gtk_list_remove_items (GTK_LIST (fm->store_list), list);
      fm->stores = g_slist_remove(fm->stores, item);
      gtk_list_insert_items (GTK_LIST (fm->store_list), list, j);
      fm->stores = g_slist_insert(fm->stores, item, j);
      gtk_list_select_item (GTK_LIST (fm->store_list), j); 
    }
  dont_change_frame = 1;
  return TRUE;

}

static gint 
frame_manager_flip_area (GtkWidget *w, gpointer client_data)
{
  RectSelect * rect_sel;
  double scale;
  GDisplay *gdisplay = (GDisplay*) client_data;
  frame_manager_t *fm = gdisplay->frame_manager; 
  if (!frame_manager_check (gdisplay))
    return FALSE; 

  if (active_tool->type == RECT_SELECT)
    {
      rect_sel = (RectSelect *) active_tool->private;
      s_x = MIN (rect_sel->x, rect_sel->x + rect_sel->w);
      s_y = MIN (rect_sel->y, rect_sel->y + rect_sel->h);
      e_x = MAX (rect_sel->x, rect_sel->x + rect_sel->w) - s_x;
      e_y = MAX (rect_sel->y, rect_sel->y + rect_sel->h) - s_y;

      if ((!s_x && !s_y && !e_x && !e_y) ||
	  (!rect_sel->w && !rect_sel->h))
	{
          scale = ((double) SCALESRC (gdisplay) / (double)SCALEDEST (gdisplay));
	  s_x = 0;
	  s_y = 0;
	  e_x = gdisplay->disp_width*scale;
	  e_y = gdisplay->disp_height*scale;

	}  
    }
  else
    {
      scale = ((double) SCALESRC (gdisplay) / (double)SCALEDEST (gdisplay));
      s_x = 0;
      s_y = 0;
      e_x = gdisplay->disp_width * scale;
      e_y = gdisplay->disp_height *scale;  
    }

  return TRUE;
}

static gint 
frame_manager_update_whole_area (GtkWidget *w, gpointer client_data)
{
  double scale;
  RectSelect * rect_sel;
  GDisplay *gdisplay = (GDisplay*) client_data;
  frame_manager_t *fm = gdisplay->frame_manager; 
  if (!frame_manager_check (gdisplay))
    return FALSE; 

  scale = ((double) SCALESRC (gdisplay) / (double)SCALEDEST(gdisplay));
  gdisplay_add_update_area (gdisplay, gdisplay->offset_x*scale, 
      gdisplay->offset_y*scale, 
      gdisplay->disp_width*scale, gdisplay->disp_height*scale);
  gdisplays_flush ();

  return TRUE;
}


static gint 
frame_manager_flip_delete (GtkWidget *w, gpointer client_data)
{
  frame_manager_t *fm = ((GDisplay*)client_data)->frame_manager; 

  fm_continue = 2;
  create_warning ("Do you wanna delete the store", (GDisplay*)client_data);
  return TRUE;

}

static void
flip_delete (GDisplay *gdisplay)
{
  frame_manager_t *fm = gdisplay->frame_manager; 
  int index;
  store_t *item;
  GList *item_list=NULL, *list=NULL;

  if (!frame_manager_check (gdisplay))
    return;
  frame_manager_rm_onionskin (gdisplay);
  item_list =  (GList*) GTK_LIST(fm->store_list)->selection;

  if (item_list && g_slist_length (fm->stores) > 1)
    {
      index = gtk_list_child_position (GTK_LIST (fm->store_list), item_list->data);
      item = (store_t*) g_slist_nth (fm->stores, index)->data;
      list = g_list_append (list, item->list_item);
      gtk_list_remove_items (GTK_LIST (fm->store_list), list);
      frame_manager_store_delete (item, gdisplay, 1);
      fm->stores = g_slist_remove(fm->stores, item);
      g_free (item);
      item = NULL;
      if (fm->stores)
	{
	  dont_change_frame = 0;
	  item = (store_t*) g_slist_nth (fm->stores, 0)->data;
	  if (item)
	    frame_manager_store_load (item, gdisplay);
	  dont_change_frame = 1;
	}
    }
  return;

}



gint 
frame_manager_close (GtkWidget *w, gpointer client_data)
{
  frame_manager_t *fm = ((GDisplay*)client_data)->frame_manager; 
  frame_manager_rm_onionskin ((GDisplay*)client_data); 
  frame_manager_store_clear ((GDisplay*)client_data); 
  
  if (GTK_WIDGET_VISIBLE (fm->shell))
    gtk_widget_hide (fm->shell);

  return TRUE; 
}

static void
frame_manager_update_menu (GDisplay *gdisplay)
{
  gdisplay->frame_manager->link_menu = 
    frame_manager_create_menu (gdisplay->gimage);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (
	gdisplay->frame_manager->link_option_menu), 
      gdisplay->frame_manager->link_menu);
}

static gint 
frame_manager_change_menu (GtkWidget *w, gpointer client_data)
{

  menu_t *mitem = (menu_t*) client_data; 
  if (mitem->img_id) 
    mitem->gdisplay->frame_manager->linked_display = gdisplay_find_display (mitem->img_id); 
  else
    mitem->gdisplay->frame_manager->linked_display = NULL; 

  return 1; 
}

static GtkWidget* 
frame_manager_create_menu (GImage *gimage)
{
  extern GSList *display_list;
  GSList *list = display_list;
  GImage *img; 
  GtkMenuItem *menu_item;
  GtkWidget *menu;
  int num_items=0;
  char *name, label[255]; 
  menu_t *mitem;
  GDisplay *gdisplay = gdisplay_get_ID (gimage->ID);

  
  if (!frame_manager_check (gdisplay))
    return; 
  menu = gtk_menu_new ();

  if (!num_items)
    {
      mitem = (menu_t*) malloc (sizeof (menu_t)); 
      mitem->img_id = 0;
      mitem->gdisplay = gdisplay;
      menu_item = gtk_menu_item_new_with_label ("NONE");
      gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
	  (GtkSignalFunc) frame_manager_change_menu,
	  mitem);
      gtk_container_add (GTK_CONTAINER (menu), menu_item);
      gtk_widget_show (menu_item);
	
    }
  
  while (list)
    {
      img = ((GDisplay*) list->data)->gimage;
 
      if (img != gimage)
	{
	  mitem = (menu_t*) malloc (sizeof (menu_t));
	  mitem->img_id = img->ID;
	  mitem->gdisplay = gdisplay;
	  name = prune_filename (gimage_filename (img));
	  sprintf (label, "%s-%d", name, img->ID);
	  menu_item = gtk_menu_item_new_with_label (label); 

	  
	  gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
	      (GtkSignalFunc) frame_manager_change_menu,
	      mitem);
	  gtk_container_add (GTK_CONTAINER (menu), menu_item);
	  gtk_widget_show (menu_item);

	  num_items ++;
	}
      
      list = g_slist_next (list); 
    }
 

  return menu; 
}

static int 
frame_manager_link_menu (GtkWidget *w, GdkEvent *event, gpointer client_data)
{
  GDisplay *gdisplay = (GDisplay*) client_data; 
 
  if (!frame_manager_check (gdisplay))
    return; 
  frame_manager_rm_onionskin (gdisplay); 
  frame_manager_update_menu (gdisplay);
  
  return 1; 
}


static gint 
frame_manager_store_list (GtkWidget *w, GdkEvent *e)
{
/*
  if (!frame_manager_check (frame_manager))
    return 0; 
*/
  return 0; 
}

void
frame_manager_store_add (GImage *gimage, GDisplay *gdisplay, int num)
{
  frame_manager_t *fm = gdisplay->frame_manager; 
  store_t *item;
  GList *item_list = NULL; 
  char frame[20];
  char selected, flip, bg, flag=0, save;

  if (!gimage || num<0)
    return;

  if (!g_slist_length (fm->stores))
    {
      item = frame_manager_store_new (gdisplay, gimage, 1);
      fm->stores = g_slist_insert (fm->stores, item, num);
      gtk_list_insert_items (GTK_LIST (fm->store_list),
	  g_list_append(item_list, item->list_item), num);
      strcpy (frame, frame_manager_get_frame (gimage));
      frame_manager_store_unselect (gdisplay);
      gtk_list_select_item (GTK_LIST (fm->store_list), num);
      item->selected = 1;
      item->save = item->gimage->dirty;
    }
  else
    if (g_slist_length (fm->stores) <= num)
      {
	/* just add the store to the end */
	item = frame_manager_store_new (gdisplay, gimage, 0);
	fm->stores = g_slist_append (fm->stores, item); 
	gtk_list_insert_items (GTK_LIST (fm->store_list),
	    g_list_append(item_list, item->list_item), num);

	
      }
    else 
      {
	
	/* rm the item and then add new item in place */
	item = (store_t*) g_slist_nth (fm->stores, num)->data;
	selected = item->selected; 
	flip = item->flip; 
	bg = item->bg;
	save = gimage->dirty;
	if (!item->active)
	  {
	    selected = 1;
	    flip = 1;
	    flag = 1;
	  }	    
	item_list = g_list_append (item_list, item->list_item);
	gtk_list_remove_items (GTK_LIST (fm->store_list), item_list);
	frame_manager_store_delete (item, gdisplay, 0);
	fm->stores = g_slist_remove(fm->stores, item);
	g_free (item);
	/* if item selected, keep selected */
	item_list = NULL; 	
	item = frame_manager_store_new (gdisplay, gimage, 1);
	item->flip = flip; 
	item->bg = bg; 
	item->save = save; 
	fm->stores = g_slist_insert (fm->stores, item, num);
	gtk_list_insert_items (GTK_LIST (fm->store_list),
	    g_list_append(item_list, item->list_item), num);
	frame_manager_save_redraw (item);

	if (selected)
	  {
	    strcpy (frame, frame_manager_get_frame (gimage));
	    frame_manager_store_unselect (gdisplay);
	    gtk_list_select_item (GTK_LIST (fm->store_list), num);
	  }
	item->selected = selected; 
	
	if (flag)
	  {
	    frame_manager_store_load (item, gdisplay);
	  }
      }
}

store_t* 
frame_manager_store_new (GDisplay *gdisplay, GImage *gimage, char active)
{

  store_t *item; 
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *alignment;
  GtkWidget *list_item; 

  item = (store_t*) malloc (sizeof (store_t));
  item->list_item = list_item = gtk_list_item_new (); 
  item->gimage = gimage;
  if (active)
    {
      item->step = 1;
      item->flip = 1;
      item->bg = 0;
      item->save = 0;
      item->selected = 0;
      item->active = 1;
    }
  else
    {
      item->step = 0;
      item->flip = 0;
      item->bg = 0;
      item->save = 0;
      item->selected = 0;
      item->active = 0;
    }
  
  gtk_object_set_user_data (GTK_OBJECT (list_item), item);

  gtk_signal_connect (GTK_OBJECT (list_item), "select",
      (GtkSignalFunc) frame_manager_store_select_update,
      gdisplay);
  gtk_signal_connect (GTK_OBJECT (list_item), "deselect",
      (GtkSignalFunc) frame_manager_store_deselect_update,
      item);
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (list_item), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 1);
  gtk_widget_show (hbox);

  /* Create the visibility toggle button */
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, TRUE, 2);
  gtk_widget_show (alignment);
  
  item->istep = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (item->istep), istep_width, istep_height);
  gtk_widget_set_events (item->istep, BUTTON_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (item->istep), "event",
      (GtkSignalFunc) frame_manager_store_step_button,
      item);
  gtk_object_set_user_data (GTK_OBJECT (item->istep), item);
  gtk_container_add (GTK_CONTAINER (alignment), item->istep);
  gtk_widget_show (item->istep);
  
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, TRUE, 2);
  gtk_widget_show (alignment);
  
  item->iflip = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (item->iflip), iflip_width, iflip_height);
  gtk_widget_set_events (item->iflip, BUTTON_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (item->iflip), "event",
      (GtkSignalFunc) frame_manager_store_flip_button,
      item);
  gtk_object_set_user_data (GTK_OBJECT (item->iflip), item);
  gtk_container_add (GTK_CONTAINER (alignment), item->iflip);
  gtk_widget_show (item->iflip);
  
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, TRUE, 2);
  gtk_widget_show (alignment);
  
  item->ibg = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (item->ibg), ibg_width, ibg_height);
  gtk_widget_set_events (item->ibg, BUTTON_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (item->ibg), "event",
      (GtkSignalFunc) frame_manager_store_bg_button,
      gdisplay);
  gtk_object_set_user_data (GTK_OBJECT (item->ibg), item);
  gtk_container_add (GTK_CONTAINER (alignment), item->ibg);
  gtk_widget_show (item->ibg);
  
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, TRUE, 2);
  gtk_widget_show (alignment);
  
  item->isave = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (item->isave), isave_width, isave_height);
  gtk_widget_set_events (item->isave, BUTTON_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (item->isave), "event",
      (GtkSignalFunc) frame_manager_store_save_button,
      item);
  gtk_object_set_user_data (GTK_OBJECT (item->isave), item);
  gtk_container_add (GTK_CONTAINER (alignment), item->isave);
  gtk_widget_show (item->isave);
  
  /*  the channel name label */
  if (active)
    item->label = gtk_label_new (prune_filename (item->gimage->filename)); 
  else
    item->label = gtk_label_new ("nothing");

  gtk_box_pack_start (GTK_BOX (hbox), item->label, FALSE, FALSE, 2);
  gtk_widget_show (item->label);

  gtk_widget_show (list_item);
  gtk_widget_ref (item->list_item);

  return item;
  
}

static void
frame_manager_store_delete (store_t *store, GDisplay *gdisplay, char flag)
{
  frame_manager_t *fm = gdisplay->frame_manager; 
  if (!store->active)
    return;

  /* get rid of gimage */
  if (gtk_toggle_button_get_active((GtkToggleButton*)fm->auto_save))
    {
      /* save image */
      if (flag)
      file_save (store->gimage->ID, store->gimage->filename, 
	  prune_filename (store->gimage->filename));
      if (active_tool->type == CLONE)
	clone_delete_image (store->gimage);
      if (flag)
	gimage_delete (store->gimage);
    }
  else
    {
      if (active_tool->type == CLONE)
	clone_delete_image (store->gimage);
      gimage_delete (store->gimage);  
    }
}

static void 
frame_manager_save (GImage *gimage, GDisplay *gdisplay)
{
  frame_manager_t *fm = gdisplay->frame_manager; 
  frame_manager_rm_onionskin (gdisplay); 
  if (gtk_toggle_button_get_active((GtkToggleButton*)fm->auto_save))
    {
      /* save image */
      file_save (gimage->ID, gimage->filename, prune_filename (gimage->filename));
    }

}


static void
frame_manager_rm_store (GDisplay *gdisplay)
{
  frame_manager_t *fm = gdisplay->frame_manager;
  if (!frame_manager_check (gdisplay))
    return; 
  frame_manager_rm_onionskin (gdisplay); 
}

static void 
frame_manager_store_unselect (GDisplay *gdisplay)
{
  frame_manager_t *fm = gdisplay->frame_manager; 
  store_t *item;  
  GSList *list = fm->stores;

  frame_manager_rm_onionskin (gdisplay); 
  while (list)
    {
      item = (store_t*) list->data;
      if (item->selected)
	{
	  item->selected = 0;
	  gtk_list_unselect_item (GTK_LIST (fm->store_list), 
	      g_slist_index (fm->stores, item)); 
	}
      list = g_slist_next (list);
    }
  
}

static void
frame_manager_store_load (store_t *item, GDisplay *gdisplay)
{
  frame_manager_t *fm = gdisplay->frame_manager; 
  item->selected = 1;
  gtk_list_select_item (GTK_LIST (fm->store_list), 
      g_slist_index (fm->stores, item));
  gdisplay->gimage = item->gimage;
  gdisplay->ID = item->gimage->ID; 
  gdisplays_update_title (gdisplay->gimage->ID);
  gdisplay_add_update_area (gdisplay, s_x, s_y, e_x, e_y);
  if (active_tool->type == CLONE)
    clone_flip_image (); 
  gdisplays_flush ();

}

static void
frame_manager_store_add_stores (GDisplay *gdisplay)
{
  frame_manager_t *fm = gdisplay->frame_manager; 
  int num, cur_num, i, f, l;
  GList *item_list=NULL, *list=NULL;
  store_t *item;
  char *whole, *raw, tmp[256], tmp2[256], *frame;
  GImage *gimage;

  cur_num = g_slist_length (fm->stores);
  item_list =  (GList*) GTK_LIST(fm->store_list)->selection;
  if(!item_list)
    return;
  
  i = gtk_list_child_position (GTK_LIST (fm->store_list), item_list->data);
  item = (store_t*) g_slist_nth (fm->stores, i)->data;

 switch (NEW_OPTION)
   {
   case 3: /* do nothing */
    num = gtk_spin_button_get_value_as_int (fm->store_size); 
    for (i=0; i<num; i++)
      {
	dont_change_frame = 0; 
	frame_manager_store_add (item->gimage, gdisplay, cur_num+i); 
	dont_change_frame = 1; 
      }
    break;
   case 0: /* add prev */
    num = gtk_spin_button_get_value_as_int (fm->store_size); 
    frame = strdup (frame_manager_get_frame (item->gimage));
    l = strlen (frame);
    f = atoi (frame);  
    whole = (char*) malloc (sizeof(char)*255);
    raw = (char*) malloc (sizeof(char)*255);

    for (i=0; i<num; i++)
      {
	
	sprintf (tmp, "%d\0", f-(i+1));
	while (strlen (tmp) != l)
	  {
	    sprintf (tmp2, "0%s\0", tmp);
	    strcpy (tmp, tmp2);
	  }
	frame_manager_this_filename (item->gimage, &whole, &raw, tmp);

	/* find the current */
	if ((gimage=file_load_without_display (whole, raw, gdisplay)))
	  {
	    item = frame_manager_store_new (gdisplay, gimage, 1);
	    item->selected = 0;
	    fm->stores = g_slist_append (fm->stores, item);
	    gtk_list_append_items (GTK_LIST (fm->store_list),
		g_list_append(list, item->list_item));

	    list=NULL; 
	  }
      }
    break;
   case 1: /* add next */
    num = gtk_spin_button_get_value_as_int (fm->store_size); 
    frame = strdup (frame_manager_get_frame (item->gimage));
    l = strlen (frame);
    f = atoi (frame);  
    whole = (char*) malloc (sizeof(char)*255);
    raw = (char*) malloc (sizeof(char)*255);

    for (i=0; i<num; i++)
      {
	sprintf (tmp, "%d\0", f+i+1);
	while (strlen (tmp) != l)
	  {
	    sprintf (tmp2, "0%s\0", tmp);
	    strcpy (tmp, tmp2);
	  }
	frame_manager_this_filename (item->gimage, &whole, &raw, tmp);

	/* find the current */
	if ((gimage=file_load_without_display (whole, raw, gdisplay)))
	  {
	    item = frame_manager_store_new (gdisplay, gimage, 1);
	    item->selected = 0;
	    fm->stores = g_slist_append (fm->stores, item);
	    gtk_list_append_items (GTK_LIST (fm->store_list),
		g_list_append(list, item->list_item));

	    list=NULL; 
	  }
      }
    break;
   case 2: /* cp */
    num = gtk_spin_button_get_value_as_int (fm->store_size);

    gimage = item->gimage;

    for (i=0; i<num; i++)
      {
	item = frame_manager_store_new (gdisplay, gimage_copy (gimage), 1);
	item->selected = 0;
	fm->stores = g_slist_append (fm->stores, item);
	gtk_list_append_items (GTK_LIST (fm->store_list),
	                    g_list_append(list, item->list_item));
	list=NULL;
      }
    break;
   default:
    printf ("ERROR: \n");
    break;
   } 
}

GDisplay*
frame_manager_load (GDisplay *gdisplay, GImage *gimage)
{
  store_t *item;
  GSList *item_list=NULL;
  int i;

  printf ("frame_manager_load\n");
  if (!gdisplay || !gdisplay->frame_manager)
    {
      printf ("Problem\n");
      return NULL;
    }

  /* get the selected item */
  /* make sure it if inactive */
  /* add the new stuff to it */

  item_list =  (GList*) GTK_LIST(gdisplay->frame_manager->store_list)->selection;

  if (item_list)
    {
      i = gtk_list_child_position (GTK_LIST (gdisplay->frame_manager->store_list), 
	  item_list->data);
      item = (store_t*) g_slist_nth (gdisplay->frame_manager->stores, i)->data;
      if (!item->active)
	{
      dont_change_frame = 0; 
	frame_manager_store_add (gimage, gdisplay, i); 
      dont_change_frame = 1; 
    }
    }

  return gdisplay;

}
static void
frame_manager_store_option_close (GtkWidget *w, gpointer client_data)
{
  frame_manager_t *fm = ((GDisplay*) client_data)->frame_manager;

  if (fm)
    {
      if (GTK_WIDGET_VISIBLE (fm->store_option))
	{
	  gtk_widget_hide (fm->store_option); 
	  dont_change_frame = 0; 

	  frame_manager_store_add_stores ((GDisplay*) client_data); 
	  dont_change_frame = 1; 
	}
    }
}

char *options[4] =
{
  "Load prev frames",
  "Load next frames",
  "Load copies of cur frame", 
  "Do nothing"
};

static ActionAreaItem offset_action_items[] =
{
  { "Close",         frame_manager_store_option_close, NULL, NULL },
};

static void 
frame_manager_store_option_option (GtkWidget *w, gpointer client_data)
{
  NEW_OPTION = (int) client_data; 
}

static void
frame_manager_store_new_options (GDisplay *gdisplay)
{
  frame_manager_t *fm = gdisplay->frame_manager; 
  int i;
  GtkWidget *vbox, *radio_button;

  GSList *group = NULL;

  if (!fm->store_option)
    { 
      fm->store_option = gtk_dialog_new ();
      gtk_object_ref(GTK_OBJECT(fm->store_option));
      gtk_window_set_wmclass (GTK_WINDOW (fm->store_option), "Store Option", "Gimp");
      gtk_window_set_policy (GTK_WINDOW (fm->store_option), FALSE, FALSE, FALSE);
      gtk_window_set_title (GTK_WINDOW (fm->store_option), "New Store Option");

      vbox = gtk_vbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (fm->store_option)->vbox),
	  vbox, TRUE, TRUE, 0);

      for (i = 0; i < 4; i++)
	{
	  radio_button = gtk_radio_button_new_with_label (group, options[i]);
	  group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
	  gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
	      (GtkSignalFunc) frame_manager_store_option_option,
	      (void *)((long) i));
	  gtk_box_pack_start (GTK_BOX (vbox), radio_button, FALSE, FALSE, 0);
	  gtk_widget_show (radio_button);
	}

      offset_action_items[0].user_data = gdisplay;
      build_action_area (GTK_DIALOG (fm->store_option), offset_action_items, 1, 0);

      gtk_widget_show (vbox);
      gtk_widget_show (fm->store_option); 
    }
  else
    if (!GTK_WIDGET_VISIBLE (fm->store_option))
      {
	gtk_widget_show (fm->store_option);
      }
}

static int
frame_manager_change_frame_close (GtkWidget *w, gpointer client_data)
{
  char *whole, *raw;
  int num=1, i;
  GSList *item_list=NULL;
  frame_manager_t *fm = ((GDisplay*) client_data)->frame_manager;

  if (fm)
    {
      if (GTK_WIDGET_VISIBLE (fm->change_frame_num))
	{
	  gtk_widget_hide (fm->change_frame_num); 
	}
    }
}

static ActionAreaItem change_action_items[] =
{
  { "Close",         frame_manager_change_frame_close, NULL, NULL },
};

static void
frame_manager_change_frame_num (GDisplay *gdisplay, store_t *item)
{

  frame_manager_t *fm = gdisplay->frame_manager; 
  int i;
  GtkWidget *vbox, *radio_button, *hbox, *label;
  GList *item_list;

  char tmp[256], *temp; 
  
  GSList *group = NULL;

  if (!dont_change_frame)
    return;
  if (!fm->change_frame_num && GTK_WIDGET_VISIBLE (fm->shell))
    {
#if 1 
      fm->change_frame_num = gtk_dialog_new ();
      gtk_window_set_wmclass (GTK_WINDOW (fm->change_frame_num), "Change Frame Num", "Gimp");
      gtk_window_set_policy (GTK_WINDOW (fm->change_frame_num), FALSE, FALSE, FALSE);
      gtk_window_set_title (GTK_WINDOW (fm->change_frame_num), "Change Frame Num");

      vbox = gtk_vbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (fm->change_frame_num)->vbox),
	  vbox, TRUE, TRUE, 0);
      
      /* jump to */
      hbox = gtk_hbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (hbox), 1);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);

      sprintf (&(tmp[0]), "%s.\0", frame_manager_get_name (item->gimage));
      label = gtk_label_new (tmp);     
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
      gtk_widget_show (label);
      

      fm->jump_to = gtk_entry_new (); 
      temp = strdup (frame_manager_get_frame (item->gimage)); 
      gtk_entry_set_text (GTK_ENTRY (fm->jump_to), temp); 
      gtk_box_pack_start (GTK_BOX (hbox), fm->jump_to, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (fm->jump_to), "activate",
	  (GtkSignalFunc) frame_manager_jump_to,
	  gdisplay);
      gtk_widget_show (fm->jump_to);
     
      sprintf (&(tmp[0]), ".%s\0", frame_manager_get_ext (item->gimage));
      label = gtk_label_new (tmp);     
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
      gtk_widget_show (label);

      change_action_items[0].user_data = gdisplay;
      build_action_area (GTK_DIALOG (fm->change_frame_num), change_action_items, 1, 0);

      gtk_widget_show (vbox);
      gtk_widget_show (fm->change_frame_num); 
#endif 
    }
  else
    if (GTK_WIDGET_VISIBLE (fm->shell) && !GTK_WIDGET_VISIBLE (fm->change_frame_num))
      {
	temp = strdup (frame_manager_get_frame (item->gimage));
	gtk_entry_set_text (GTK_ENTRY (fm->jump_to), temp);
	gtk_widget_show (fm->change_frame_num);
      }
}

static gint 
frame_manager_store_size (GtkWidget *w, gpointer client_data)
{
  GDisplay *gdisplay = (GDisplay*) client_data;
  frame_manager_t *fm = gdisplay->frame_manager;

  if (!frame_manager_check (gdisplay))
    {
    return;
    } 
  frame_manager_rm_onionskin (gdisplay); 

  frame_manager_store_new_options (gdisplay); 
  return TRUE; 
}


static gint 
frame_manager_store_select_update (GtkWidget *w, gpointer client_pointer)
{
  GList *item_list;
  int i;
  store_t *cur, *item = (store_t*) gtk_object_get_user_data (GTK_OBJECT (w));
  GDisplay *gdisplay = (GDisplay*) client_pointer;
  
  if (!gdisplay)
    return 0;

  frame_manager_rm_onionskin (gdisplay);

  item_list =  (GList*) GTK_LIST(gdisplay->frame_manager->store_list)->selection;
  if (!item_list)
    {
      if (item->active)
	{
	  item->selected = 1;
	  frame_manager_change_frame_num (gdisplay, item);
	}
    }
  else
    if (item->active)
      {
	item->selected = 1;
	frame_manager_store_load (item, gdisplay); 
      }

  return 1; 
}

static gint 
frame_manager_store_deselect_update (GtkWidget *w, gpointer client_pointer)
{
  store_t *item = (store_t*) client_pointer;

  item->selected = 0;
  return 1; 
}

static gint 
frame_manager_store_step_button (GtkWidget *w, GdkEvent *event)
{
  store_t *item;
  
  item = (store_t*) gtk_object_get_user_data (GTK_OBJECT (w));

  switch (event->type)
    {
    case GDK_EXPOSE:
      frame_manager_step_redraw (item);
      return 1;

    case GDK_BUTTON_PRESS:
      item->step = item->active ? item->step ? 0 : 1 : 0;
      frame_manager_step_redraw (item);
      return 1;
    default:
      break;
    }
  return 0;
}

static gint 
frame_manager_store_flip_button (GtkWidget *w, GdkEvent *event)
{
  store_t *item;
  
  item = (store_t*) gtk_object_get_user_data (GTK_OBJECT (w));

  switch (event->type)
    {
    case GDK_EXPOSE:
      frame_manager_flip_redraw (item);
      return 1;

    case GDK_BUTTON_PRESS:
      item->flip = item->active ? item->flip ? 0 : 1 :0;
      frame_manager_flip_redraw (item);
      return 1;
    default:
      break;
    }
  return 0;
}

static gint 
frame_manager_store_bg_button (GtkWidget *w, GdkEvent *event, gpointer pointer_data)
{
  store_t *item, *i;
  GList *list=NULL; 
  GDisplay *gdisplay=NULL;

  item = (store_t*) gtk_object_get_user_data (GTK_OBJECT (w));
  gdisplay = (GDisplay*) pointer_data;  
  if (!gdisplay)
    return 0;

  list = gdisplay->frame_manager->stores;

  switch (event->type)
    {
    case GDK_EXPOSE:
      frame_manager_bg_redraw (item);
      return 1;

    case GDK_BUTTON_PRESS:
      if (!item->bg)
	{
	  while (list)
	    {
	      i = (store_t*) list->data; 
	      if (i->bg)
		{
		  i->bg = 0;
		  frame_manager_bg_redraw (i);
		}	   
	      list = g_slist_next (list);   
	    }
	}
      item->bg = item->active ? item->bg ? 0 : 1 : 0;
      frame_manager_bg_redraw (item);
      return 1;
    default:
      break;
    }
  return 0;
}

static gint 
frame_manager_store_save_button (GtkWidget *w, GdkEvent *event)
{
  store_t *item, *i;
  
  item = (store_t*) gtk_object_get_user_data (GTK_OBJECT (w));

  switch (event->type)
    {
    case GDK_EXPOSE:
      frame_manager_save_redraw (item);
      return 1;
    default:
      break;
    }
  return 0;
}


static void 
frame_manager_step_redraw (store_t *item)
{
  GdkPixmap *pixmap;
  GdkColor *color;
  GtkStateType state;
  int visible;

  state = item->list_item->state;

  if (GTK_WIDGET_IS_SENSITIVE (item->list_item))
    {
      if (state == GTK_STATE_SELECTED)
	color = &item->istep->style->bg[GTK_STATE_SELECTED];
      else
	color = &item->istep->style->white;
    }
  else
    color = &item->istep->style->bg[GTK_STATE_INSENSITIVE];

  gdk_window_set_background (item->istep->window, color);


  if (item->step)
    {
      if (!istep_pixmap[NORMAL])
	{
	  istep_pixmap[NORMAL] =
	    gdk_pixmap_create_from_data (item->istep->window,
		(gchar*) istep_bits, istep_width, istep_height, -1,
		&item->istep->style->fg[GTK_STATE_NORMAL],
		&item->istep->style->white);
	  istep_pixmap[SELECTED] =
	    gdk_pixmap_create_from_data (item->istep->window,
		(gchar*) istep_bits, istep_width, istep_height, -1,
		&item->istep->style->fg[GTK_STATE_SELECTED],
		&item->istep->style->bg[GTK_STATE_SELECTED]);
	  istep_pixmap[INSENSITIVE] =
	    gdk_pixmap_create_from_data (item->istep->window,
		(gchar*) istep_bits, istep_width, istep_height, -1,
		&item->istep->style->fg[GTK_STATE_INSENSITIVE],
		&item->istep->style->bg[GTK_STATE_INSENSITIVE]);
	}

      if (GTK_WIDGET_IS_SENSITIVE (item->list_item))
	{
	  if (state == GTK_STATE_SELECTED)
	    pixmap = istep_pixmap[SELECTED];
	  else
	    pixmap = istep_pixmap[NORMAL];
	}
      else
	pixmap = istep_pixmap[INSENSITIVE];


      gdk_draw_pixmap (item->istep->window,
	  item->istep->style->black_gc,
	  pixmap, 0, 0, 0, 0, istep_width, istep_height);
    }
  else
    {
      gdk_window_clear (item->istep->window);
    }
}

static void 
frame_manager_flip_redraw (store_t *item)
{
  GdkPixmap *pixmap;
  GdkColor *color;
  GtkStateType state;
  int visible;

  state = item->list_item->state;

  if (GTK_WIDGET_IS_SENSITIVE (item->list_item))
    {
      if (state == GTK_STATE_SELECTED)
	color = &item->iflip->style->bg[GTK_STATE_SELECTED];
      else
	color = &item->iflip->style->white;
    }
  else
    color = &item->iflip->style->bg[GTK_STATE_INSENSITIVE];

  gdk_window_set_background (item->iflip->window, color);


  if (item->flip)
    {
      if (!iflip_pixmap[NORMAL])
	{
	  iflip_pixmap[NORMAL] =
	    gdk_pixmap_create_from_data (item->iflip->window,
		(gchar*) iflip_bits, iflip_width, iflip_height, -1,
		&item->iflip->style->fg[GTK_STATE_NORMAL], 
		&item->iflip->style->white);
	  iflip_pixmap[SELECTED] =
	    gdk_pixmap_create_from_data (item->iflip->window,
		(gchar*) iflip_bits, iflip_width, iflip_height, -1,
		&item->iflip->style->fg[GTK_STATE_SELECTED],
		&item->iflip->style->bg[GTK_STATE_SELECTED]);
	  iflip_pixmap[INSENSITIVE] =
	    gdk_pixmap_create_from_data (item->iflip->window,
		(gchar*) iflip_bits, iflip_width, iflip_height, -1,
		&item->iflip->style->fg[GTK_STATE_INSENSITIVE], 
		&item->iflip->style->bg[GTK_STATE_INSENSITIVE]);
	}

      if (GTK_WIDGET_IS_SENSITIVE (item->list_item))
	{
	  if (state == GTK_STATE_SELECTED)
	    pixmap = iflip_pixmap[SELECTED];
	  else
	    pixmap = iflip_pixmap[NORMAL];
	}
      else
	pixmap = iflip_pixmap[INSENSITIVE];


      gdk_draw_pixmap (item->iflip->window,
	  item->iflip->style->black_gc,
	  pixmap, 0, 0, 0, 0, iflip_width, iflip_height);
    }
  else
    {
      gdk_window_clear (item->iflip->window);
    }
}

static void 
frame_manager_bg_redraw (store_t *item)
{
  GdkPixmap *pixmap;
  GdkColor *color;
  GtkStateType state;
  int visible;

  state = item->list_item->state;

  if (GTK_WIDGET_IS_SENSITIVE (item->list_item))
    {
      if (state == GTK_STATE_SELECTED)
	color = &item->ibg->style->bg[GTK_STATE_SELECTED];
      else
	color = &item->ibg->style->white;
    }
  else
    color = &item->ibg->style->bg[GTK_STATE_INSENSITIVE];

  gdk_window_set_background (item->ibg->window, color);


  if (item->bg)
    {
      if (!ibg_pixmap[NORMAL])
	{
	  ibg_pixmap[NORMAL] =
	    gdk_pixmap_create_from_data (item->ibg->window,
		(gchar*) ibg_bits, ibg_width, ibg_height, -1,
		&item->ibg->style->fg[GTK_STATE_NORMAL],
		&item->ibg->style->white);
	  ibg_pixmap[SELECTED] =
	    gdk_pixmap_create_from_data (item->ibg->window,
		(gchar*) ibg_bits, ibg_width, ibg_height, -1,
		&item->ibg->style->fg[GTK_STATE_SELECTED],
		&item->ibg->style->bg[GTK_STATE_SELECTED]);
	  ibg_pixmap[INSENSITIVE] =
	    gdk_pixmap_create_from_data (item->ibg->window,
		(gchar*) ibg_bits, ibg_width, ibg_height, -1,

		&item->ibg->style->fg[GTK_STATE_INSENSITIVE],
		&item->ibg->style->bg[GTK_STATE_INSENSITIVE]);
	}

      if (GTK_WIDGET_IS_SENSITIVE (item->list_item))
	{
	  if (state == GTK_STATE_SELECTED)
	    pixmap = ibg_pixmap[SELECTED];
	  else
	    pixmap = ibg_pixmap[NORMAL];
	}
      else
	pixmap = ibg_pixmap[INSENSITIVE];


      gdk_draw_pixmap (item->ibg->window,
	  item->ibg->style->black_gc,
	  pixmap, 0, 0, 0, 0, ibg_width, ibg_height);
    }
  else
    {
      gdk_window_clear (item->ibg->window);
    }
}

static void 
frame_manager_save_redraw (store_t *item)
{
  GdkPixmap *pixmap;
  GdkColor *color;
  GtkStateType state;
  int visible;

  state = item->list_item->state;

  if (GTK_WIDGET_IS_SENSITIVE (item->list_item))
    {
      if (state == GTK_STATE_SELECTED)
	color = &item->isave->style->bg[GTK_STATE_SELECTED];
      else
	color = &item->isave->style->white;
    }
  else
    color = &item->isave->style->bg[GTK_STATE_INSENSITIVE];

  gdk_window_set_background (item->isave->window, color);


  if (item->save)
    {
      if (!isave_pixmap[NORMAL])
	{
	  isave_pixmap[NORMAL] =
	    gdk_pixmap_create_from_data (item->isave->window,
		(gchar*) isave_bits, isave_width, isave_height, -1,
		&item->isave->style->fg[GTK_STATE_NORMAL],
		&item->isave->style->white);
	  isave_pixmap[SELECTED] =
	    gdk_pixmap_create_from_data (item->isave->window,
		(gchar*) isave_bits, isave_width, isave_height, -1,
		&item->isave->style->fg[GTK_STATE_SELECTED],
		&item->isave->style->bg[GTK_STATE_SELECTED]);
	  isave_pixmap[INSENSITIVE] =
	    gdk_pixmap_create_from_data (item->isave->window,
		(gchar*) isave_bits, isave_width, isave_height, -1,

		&item->isave->style->fg[GTK_STATE_INSENSITIVE],
		&item->isave->style->bg[GTK_STATE_INSENSITIVE]);
	}

      if (GTK_WIDGET_IS_SENSITIVE (item->list_item))
	{
	  if (state == GTK_STATE_SELECTED)
	    pixmap = isave_pixmap[SELECTED];
	  else
	    pixmap = isave_pixmap[NORMAL];
	}
      else
	pixmap = isave_pixmap[INSENSITIVE];


      gdk_draw_pixmap (item->isave->window,
	  item->isave->style->black_gc,
	  pixmap, 0, 0, 0, 0, isave_width, isave_height);
    }
  else
    {
      gdk_window_clear (item->isave->window);
    }
}

static gint 
frame_manager_transparency (GtkAdjustment *a, gpointer client_data)
{
  float opacity;
  store_t *fg, *bg;
  int i, j;
  GList *item_list;
  GDisplay *gdisplay = (GDisplay*) client_data; 
  frame_manager_t *fm = gdisplay->frame_manager;

  if (!frame_manager_check (gdisplay))
    {
      return TRUE; 
    }
  if (fm->onionskin == 2)
    {
     fm->onionskin = 0;
    return TRUE;  
    }
  /* display */
  opacity = a->value;

  if (g_slist_length (fm->stores) > 1)
    {
      item_list =  (GList*) GTK_LIST(fm->store_list)->selection;

      if (item_list)
	{
	  i = gtk_list_child_position (GTK_LIST (fm->store_list), item_list->data);
	  j = frame_manager_get_bg_id (gdisplay);
	  if (i != j && j < g_slist_length (fm->stores))
	    {
	      fg = (store_t*) g_slist_nth (fm->stores, i)->data;

	      fg->gimage->active_layer->opacity = opacity; 


	      if (!fg->gimage->onionskin)
		{
		  Layer *layer = fg->gimage->active_layer;

		  bg = (store_t*) g_slist_nth (fm->stores, j)->data; 
		  fg->gimage->onionskin = 1;
		  fm->onionskin=1;
		  fg->bg_image = bg->gimage; 
		  gimage_add_layer2 (fg->gimage, bg->gimage->active_layer, 
		      1, s_x, s_y, e_x, e_y);
		  gimage_set_active_layer (fg->gimage, layer); 

		  if (active_tool->type == CLONE)
		    {
		      frame_manager_onionskin_offset (gdisplay, clone_get_x_offset (), 
			  clone_get_y_offset ());
		    }
		}
	      drawable_update (GIMP_DRAWABLE(fg->gimage->active_layer),
		  s_x, s_y, e_x, e_y);
  	      /*gdisplay_add_update_area (gdisplay, s_x, s_y, e_x, e_y);
	      */gdisplays_flush ();
	    }	      
	} 

    }
  return TRUE; 
}

static gint 
frame_manager_opacity_00 (GtkWidget *w, gpointer client_data)
{
  store_t *fg, *bg;
  int i, j;
  GList *item_list;

  GDisplay *gdisplay = (GDisplay*) client_data; 
  frame_manager_t *fm = gdisplay->frame_manager;

  if (!frame_manager_check (gdisplay))
    {
      return TRUE; 
    }
  if (fm->onionskin == 2)
    {
     fm->onionskin = 0;
    return TRUE;  
    }
  /* display */

  if (g_slist_length (fm->stores) > 1)
    {
      item_list =  (GList*) GTK_LIST(fm->store_list)->selection;

      if (item_list)
	{
	  i = gtk_list_child_position (GTK_LIST (fm->store_list), item_list->data);
	  j = frame_manager_get_bg_id (gdisplay);
	  if (i != j && j < g_slist_length (fm->stores))
	    {
	      fg = (store_t*) g_slist_nth (fm->stores, i)->data;

	      fg->gimage->active_layer->opacity = 0.0; 


	      if (!fg->gimage->onionskin)
		{
		  Layer *layer = fg->gimage->active_layer;

		  bg = (store_t*) g_slist_nth (fm->stores, j)->data; 
		  fg->gimage->onionskin = 1;
		  fm->onionskin=1;
		  fg->bg_image = bg->gimage; 
		  gimage_add_layer2 (fg->gimage, bg->gimage->active_layer, 
		      1, s_x, s_y, e_x, e_y);
		  gimage_set_active_layer (fg->gimage, layer); 
		  if (active_tool->type == CLONE)
		    {
		      frame_manager_onionskin_offset (gdisplay, clone_get_x_offset (), 
			  clone_get_y_offset ());
		    }
		}
	      gtk_adjustment_set_value (fm->trans_data, 0.0);
	     drawable_update (GIMP_DRAWABLE(fg->gimage->active_layer),
		  s_x, s_y, e_x, e_y);
  	      /*gdisplay_add_update_area (gdisplay, s_x, s_y, e_x, e_y);
	      */gdisplays_flush ();
	    }	      
	} 

    }
  return TRUE; 
}

static gint 
frame_manager_opacity_25 (GtkWidget *w, gpointer client_data)
{
  store_t *fg, *bg;
  int i, j;
  GList *item_list;
  GDisplay *gdisplay = (GDisplay*) client_data; 
  frame_manager_t *fm = gdisplay->frame_manager;

  if (!frame_manager_check (gdisplay))
    {
      return TRUE; 
    }
  if (fm->onionskin == 2)
    {
     fm->onionskin = 0;
    return TRUE;  
    }
  /* display */

  if (g_slist_length (fm->stores) > 1)
    {
      item_list =  (GList*) GTK_LIST(fm->store_list)->selection;

      if (item_list)
	{
	  i = gtk_list_child_position (GTK_LIST (fm->store_list), item_list->data);
	  j = frame_manager_get_bg_id (gdisplay);
	  if (i != j && j < g_slist_length (fm->stores))
	    {
	      fg = (store_t*) g_slist_nth (fm->stores, i)->data;

	      fg->gimage->active_layer->opacity = 0.25; 


	      if (!fg->gimage->onionskin)
		{
		  Layer *layer = fg->gimage->active_layer;

		  bg = (store_t*) g_slist_nth (fm->stores, j)->data; 
		  fg->gimage->onionskin = 1;
		  fm->onionskin=1;
		  fg->bg_image = bg->gimage; 
		  gimage_add_layer2 (fg->gimage, bg->gimage->active_layer, 
		      1, s_x, s_y, e_x, e_y);
		  gimage_set_active_layer (fg->gimage, layer); 
		  if (active_tool->type == CLONE)
		    {
		      frame_manager_onionskin_offset (gdisplay, clone_get_x_offset (), 
			  clone_get_y_offset ());
		    }
		}
	      gtk_adjustment_set_value (fm->trans_data, 0.25);
	      drawable_update (GIMP_DRAWABLE(fg->gimage->active_layer),
		 s_x, s_y, e_x, e_y);
	     /*gdisplay_add_update_area (gdisplay, s_x, s_y, e_x, e_y);
	      */gdisplays_flush ();
	    }	      
	} 

    }
  return TRUE; 
}

static gint 
frame_manager_opacity_05 (GtkWidget *w, gpointer client_data)
{
  store_t *fg, *bg;
  int i, j;
  GList *item_list;
  GDisplay *gdisplay = (GDisplay*) client_data; 
  frame_manager_t *fm = gdisplay->frame_manager;

  if (!frame_manager_check (gdisplay))
    {
      return TRUE; 
    }
  if (fm->onionskin == 2)
    {
      fm->onionskin = 0;
      return TRUE;  
    }
  /* display */

  if (g_slist_length (fm->stores) > 1)
    {
      item_list =  (GList*) GTK_LIST(fm->store_list)->selection;

      if (item_list)
	{
	  i = gtk_list_child_position (GTK_LIST (fm->store_list), item_list->data);
	  j = frame_manager_get_bg_id (gdisplay);
	  if (i != j && j < g_slist_length (fm->stores))
	    {
	      fg = (store_t*) g_slist_nth (fm->stores, i)->data;

	      fg->gimage->active_layer->opacity = 0.5; 


	      if (!fg->gimage->onionskin)
		{
		  Layer *layer = fg->gimage->active_layer;

		  bg = (store_t*) g_slist_nth (fm->stores, j)->data; 
		  fg->gimage->onionskin = 1;
		  fm->onionskin=1;
		  fg->bg_image = bg->gimage; 
		  gimage_add_layer2 (fg->gimage, bg->gimage->active_layer, 
		      1, s_x, s_y, e_x, e_y);
		  gimage_set_active_layer (fg->gimage, layer); 
		  if (active_tool->type == CLONE)
		    {
		      frame_manager_onionskin_offset (gdisplay, clone_get_x_offset (), 
			  clone_get_y_offset ());
		    }
		}
	      gtk_adjustment_set_value (fm->trans_data, 0.50);
	      drawable_update (GIMP_DRAWABLE(fg->gimage->active_layer),
		  s_x, s_y, e_x, e_y);
	      /*gdisplay_add_update_area (gdisplay, s_x, s_y, e_x, e_y);
	       */gdisplays_flush ();
	    }	      
	} 

    }
  return TRUE; 
}

static gint 
frame_manager_opacity_75 (GtkWidget *w, gpointer client_data)
{
  store_t *fg, *bg;
  int i, j;
  GList *item_list;
  GDisplay *gdisplay = (GDisplay*) client_data; 
  frame_manager_t *fm = gdisplay->frame_manager;

  if (!frame_manager_check (gdisplay))
    {
      return TRUE; 
    }
  if (fm->onionskin == 2)
    {
      fm->onionskin = 0;
      return TRUE;  
    }
  /* display */

  if (g_slist_length (fm->stores) > 1)
    {
      item_list =  (GList*) GTK_LIST(fm->store_list)->selection;

      if (item_list)
	{
	  i = gtk_list_child_position (GTK_LIST (fm->store_list), item_list->data);
	  j = frame_manager_get_bg_id (gdisplay);
	  if (i != j && j < g_slist_length (fm->stores))
	    {
	      fg = (store_t*) g_slist_nth (fm->stores, i)->data;

	      fg->gimage->active_layer->opacity = 0.75; 


	      if (!fg->gimage->onionskin)
		{
		  Layer *layer = fg->gimage->active_layer;

		  bg = (store_t*) g_slist_nth (fm->stores, j)->data; 
		  fg->gimage->onionskin = 1;
		  fm->onionskin=1;
		  fg->bg_image = bg->gimage; 
		  gimage_add_layer2 (fg->gimage, bg->gimage->active_layer, 
		      1, s_x, s_y, e_x, e_y);
		  gimage_set_active_layer (fg->gimage, layer); 
		  if (active_tool->type == CLONE)
		    {
		      frame_manager_onionskin_offset (gdisplay, clone_get_x_offset (), 
			  clone_get_y_offset ());
		    }
		}
	      gtk_adjustment_set_value (fm->trans_data, 0.75);
	      drawable_update (GIMP_DRAWABLE(fg->gimage->active_layer),
		  s_x, s_y, e_x, e_y);
	      /*gdisplay_add_update_area (gdisplay, s_x, s_y, e_x, e_y);
	       */gdisplays_flush ();
	    }	      
	} 

    }
  return TRUE; 
}

static gint 
frame_manager_opacity_10 (GtkWidget *w, gpointer client_data)
{
  store_t *fg, *bg;
  int i, j;
  GList *item_list;
  GDisplay *gdisplay = (GDisplay*) client_data; 
  frame_manager_t *fm = gdisplay->frame_manager;

  if (!frame_manager_check (gdisplay))
    {
      return TRUE; 
    }
  if (fm->onionskin == 2)
    {
      fm->onionskin = 0;
      return TRUE;  
    }
  /* display */

  if (g_slist_length (fm->stores) > 1)
    {
      item_list =  (GList*) GTK_LIST(fm->store_list)->selection;

      if (item_list)
	{
	  i = gtk_list_child_position (GTK_LIST (fm->store_list), item_list->data);
	  j = frame_manager_get_bg_id (gdisplay);
	  if (i != j && j < g_slist_length (fm->stores))
	    {
	      fg = (store_t*) g_slist_nth (fm->stores, i)->data;

	      fg->gimage->active_layer->opacity = 1.0; 


	      if (!fg->gimage->onionskin)
		{
		  Layer *layer = fg->gimage->active_layer;

		  bg = (store_t*) g_slist_nth (fm->stores, j)->data; 
		  fg->gimage->onionskin = 1;
		  fm->onionskin=1;
		  fg->bg_image = bg->gimage; 
		  gimage_add_layer2 (fg->gimage, bg->gimage->active_layer, 
		      1, s_x, s_y, e_x, e_y);
		  gimage_set_active_layer (fg->gimage, layer); 
		  if (active_tool->type == CLONE)
		    {
		      frame_manager_onionskin_offset (gdisplay, clone_get_x_offset (), 
			  clone_get_y_offset ());
		    }
		}
	      gtk_adjustment_set_value (fm->trans_data, 1.0);
	      drawable_update (GIMP_DRAWABLE(fg->gimage->active_layer),
		  s_x, s_y, e_x, e_y);
	      /*gdisplay_add_update_area (gdisplay, s_x, s_y, e_x, e_y);
	       */gdisplays_flush ();
	    }	      
	} 

    }
  return TRUE; 
}

static void
frame_manager_next_filename (GImage *gimage, char **whole, char **raw, char dir,
    GDisplay *gdisplay)
{

  char rawname[256], tmp[30], frame_num[30], *name, *frame, *ext;
  int i, num, org_len, cur_len, len1, len2;  
  frame_manager_t *fm = gdisplay->frame_manager; 

  if (!gimage)
    return;

  strcpy (*whole, gimage->filename);
  strcpy (rawname, prune_filename (*whole));

  len1 = strlen (*whole);
  len2 = strlen (rawname);

  name  = strdup (strtok (rawname, "."));
  frame = strdup (strtok (NULL, "."));
  ext = strdup (strtok (NULL, "."));


  org_len = strlen (frame); 
  num = atoi (frame);
  num = dir ? num + gtk_spin_button_get_value_as_int (fm->step_size) :
    num - gtk_spin_button_get_value_as_int (fm->step_size); 


  sprintf (tmp, "%d\0", num); 
  cur_len = strlen (tmp);
  sprintf (&(frame_num[org_len-cur_len]), "%s", tmp); 

  for (i=0; i<org_len-cur_len; i++)
    {
      frame_num[i] = '0'; 
    } 


  sprintf ((*raw), "%s.%s.%s\0", name, frame_num, ext);  

  sprintf (&((*whole)[len1-len2]), "%s\0", *raw);

}

static void
frame_manager_this_filename (GImage *gimage, char **whole, char **raw, char *num)
{

  char rawname[256], *name, *frame, *ext;
  int len, org_len;

  if (!gimage)
    return;

  strcpy (*whole, gimage->filename);
  strcpy (rawname, prune_filename (*whole));

  len = strlen (*whole);
  org_len = strlen (rawname);


  name  = strtok (rawname, ".");
  frame = strtok (NULL, ".");
  ext = strtok (NULL, ".");


  sprintf (*raw, "%s.%s.%s\0", name, num, ext);

  sprintf (&((*whole)[len-org_len]), "%s\0", *raw);

}

static char* 
frame_manager_get_name (GImage *gimage)
{

  char raw[256], whole[256];
  char *tmp;

  if (!gimage)
    return "ERROR";

  strcpy (whole, gimage->filename);
  strcpy (raw, prune_filename (whole));

  tmp = strdup (strtok (raw, "."));

  return tmp; 

}

static char* 
frame_manager_get_frame (GImage *gimage)
{

  char raw[256], whole[256];

  if (!gimage)
    return "ERROR";

  strcpy (whole, gimage->filename);
  strcpy (raw, prune_filename (whole));

  strtok (raw, ".");

  return strdup (strtok (NULL, ".")); 

}

static char* 
frame_manager_get_ext (GImage *gimage)
{

  char raw[256], whole[256];

  if (!gimage)
    return "ERROR";

  strcpy (whole, gimage->filename);
  strcpy (raw, prune_filename (whole));

  strtok (raw, ".");
  strtok (NULL, ".");
  return strdup (strtok (NULL, "."));

}

void
frame_manager_rm_onionskin (GDisplay *gdisplay)
{
  GSList *list=NULL;
  store_t *item;

  frame_manager_t *fm = gdisplay->frame_manager;

  list = fm->stores;
  if (fm->onionskin)
    {
      fm->onionskin = 2;
      while (list)
	{

	  item = (store_t*) list->data;
	  if (item->gimage->onionskin)
	    {
	      item->gimage->active_layer->opacity = 1; 
	      item->gimage->onionskin = 0;
	      gimage_remove_layer (item->gimage, item->bg_image->active_layer);

	      GIMP_DRAWABLE (item->bg_image->active_layer)->offset_x = 0; 
	      GIMP_DRAWABLE (item->bg_image->active_layer)->offset_y = 0; 
	      GIMP_DRAWABLE(item->bg_image->active_layer)->gimage_ID = item->bg_image->ID;
	      gtk_adjustment_set_value (fm->trans_data, 1);
	    }
	  list = g_slist_next (list);
	}   

    }

}

static void
frame_manager_store_clear (GDisplay *gdisplay)
{

  GSList *l=NULL; 
  store_t *item;
  frame_manager_t *fm = gdisplay->frame_manager;

  while (fm->stores)
    {
      item = (store_t*) fm->stores->data;
      l = g_list_append (l, item->list_item);
      if (gtk_toggle_button_get_active((GtkToggleButton*)fm->auto_save))
	{
	  /* save image */
	  file_save (item->gimage->ID, item->gimage->filename, 
	      prune_filename (item->gimage->filename));
	}
      if (!item->selected)
	{
	  if (active_tool->type == CLONE)
	    clone_delete_image (item->gimage);
	  gimage_delete (item->gimage);
	}
      fm->stores = g_slist_remove(fm->stores, item);
      g_free (item);

      item = NULL; 

    }

  gtk_list_remove_items (GTK_LIST (fm->store_list), l);

  gdisplay->framemanager=0; 

}

GimpDrawable*
frame_manager_onionskin_drawable (GDisplay *gdisplay)
{
  GSList *list=NULL;
  store_t *item;

  if (!gdisplay->frame_manager || !gdisplay->frame_manager->onionskin)
    return NULL;

  list = gdisplay->frame_manager->stores;
  while (list)
    {
      item = (store_t*) list->data;
      if (item->gimage->onionskin)
	{
	  return GIMP_DRAWABLE (item->bg_image->active_layer);
	}
      list = g_slist_next (list);
    }   
  printf ("ERROR : something is wrong with onionskin 1\n"); 
  return NULL;

}

int
frame_manager_onionskin_display (GDisplay *gdisplay)
{

  GSList *list=NULL;
  store_t *item;

  if (!gdisplay->frame_manager || !gdisplay->frame_manager->onionskin)
    return NULL;

  list = gdisplay->frame_manager->stores;
  while (list)
    {
      item = (store_t*) list->data;
      if (item->gimage->onionskin)
	{
	  return item->bg_image->ID;
	}
      list = g_slist_next (list);
    }
  printf ("ERROR : something is wrong with onionskin 2\n"); 

  return NULL; 

}

GimpDrawable*
frame_manager_bg_drawable (GDisplay *gdisplay)
{
  GSList *list=NULL;
  store_t *item;

  if (!gdisplay->frame_manager)
    return NULL;

  list = gdisplay->frame_manager->stores;
  while (list)
    {
      item = (store_t*) list->data;
      if (item->bg)
	{
	  return GIMP_DRAWABLE (item->gimage->active_layer);
	}
      list = g_slist_next (list);
    }   
  return NULL;

}

int
frame_manager_bg_display (GDisplay *gdisplay)
{

  GSList *list=NULL;
  store_t *item;

  if (!gdisplay->frame_manager)
    return NULL;

  list = gdisplay->frame_manager->stores;
  while (list)
    {
      item = (store_t*) list->data;
      if (item->bg)
	{
	  return item->gimage->ID;
	}
      list = g_slist_next (list);
    }
  return NULL; 

}

static int
frame_manager_get_bg_id (GDisplay *gdisplay)
{
  GSList *list=NULL;
  store_t *item;
  int i=0;
  frame_manager_t *fm = gdisplay->frame_manager; 

  if (!fm)
    return NULL;

  list = fm->stores;
  while (list)
    {
      item = (store_t*) list->data;
      if (item->bg)
	{
	  return i;
	}
      list = g_slist_next (list);
      i ++; 
    }
  return NULL; 

}

void
frame_manager_set_bg (GDisplay *gdisplay)
{

  GSList *list=NULL;
  store_t *item, *i;

  if (!gdisplay->frame_manager)
    return;

  list = gdisplay->frame_manager->stores;
  while (list)
    {
      i = (store_t*) list->data; 
      if (i->bg)
	{
	  i->bg = 0;
	  frame_manager_bg_redraw (i);
	}          
      list = g_slist_next (list);   
    }
  list = gdisplay->frame_manager->stores;
  while (list)
    {
      item = (store_t*) list->data;
      if (item->gimage->ID == gdisplay->ID)
	{
	  item->bg = 1;
	  frame_manager_bg_redraw (item); 
	}
      list = g_slist_next (list);
    }

}

void
frame_manager_onionskin_offset (GDisplay *gdisplay, int x, int y)
{
  GSList *list=NULL;
  store_t *item;

  if (!gdisplay->frame_manager)
    {
      printf ("onion skin offset problems \n"); 
      return;
    }
  if (!gdisplay->frame_manager->onionskin)
    {
      printf ("onion skin offset problems 2 \n");
      return;
    }

  list = gdisplay->frame_manager->stores;
  while (list)
    {
      item = (store_t*) list->data;
      if (item->gimage->onionskin)
	{
	  layer_translate2 (item->bg_image->active_layer, -x, -y, 
	      s_x, s_y, e_x, e_y);
	  return; 
	}
      list = g_slist_next (list);
    }
  printf ("ERROR : something is wrong with onionskin 3\n");


}

static void
frame_manager_link_forward (GDisplay *gdisplay)
{
  GImage *gimage; 
  char *whole, *raw;
  int num=1;
  frame_manager_t *fm = gdisplay->frame_manager;


  if (fm->linked_display)
    {
      whole = (char*) malloc (sizeof(char)*255);
      raw = (char*) malloc (sizeof(char)*255);

      /* ** save image ** */

      gimage = fm->linked_display->gimage; 
      frame_manager_next_filename (fm->linked_display->gimage, &whole, &raw, 1, 
	  gdisplay);
      if (file_load (whole, raw, fm->linked_display))
	{
	  if (gtk_toggle_button_get_active((GtkToggleButton*)fm->auto_save))
	    file_save (gimage->ID, gimage->filename,
		prune_filename (gimage->filename));
	  if (active_tool->type == CLONE)
	    clone_delete_image (gimage);
	  gimage_delete (gimage); 	 
	  /*gdisplay->frame_manager->link_menu = 
	    frame_manager_create_menu (gdisplay->gimage, 
		gtk_menu_get_active (gdisplay->frame_manager->link_menu)); 
	 gtk_option_menu_set_menu (GTK_OPTION_MENU (
	              gdisplay->frame_manager->link_option_menu), 
	          gdisplay->frame_manager->link_menu); 
*/	}
      else
	{
	  printf ("ERROR\n");
	}
      gdisplays_flush ();
      free (whole);
      free (raw);
    } 
}

static void
frame_manager_link_backward (GDisplay *gdisplay)
{
  GImage *gimage; 
  char *whole, *raw;
  int num=1;

  frame_manager_t *fm = gdisplay->frame_manager;

  if (fm->linked_display)
    {
      whole = (char*) malloc (sizeof(char)*255);
      raw = (char*) malloc (sizeof(char)*255);

      /* ** save image ** */

      gimage = fm->linked_display->gimage; 
      frame_manager_next_filename (fm->linked_display->gimage, &whole, &raw, 0, 
	  gdisplay);
      if (file_load (whole, raw, fm->linked_display))
	{
	  if (gtk_toggle_button_get_active((GtkToggleButton*)fm->auto_save))
	    file_save (gimage->ID, gimage->filename,
		prune_filename (gimage->filename));
	  if (active_tool->type == CLONE)
	    clone_delete_image (gimage);
	  gimage_delete (gimage); 	  
	}
      else
	{
	  printf ("ERROR\n");
	}
      gdisplays_flush ();
      free (whole);
      free (raw);
    } 
}

static int
frame_manager_check (GDisplay *gdisplay)
{
  if (!gdisplay || gdisplay->frame_manager==NULL)
    return 0;
  return 1;
}

void
frame_manager_image_reload (GDisplay *gdisplay, GImage *old, GImage *new)
{
  GSList *list=NULL;
  store_t *item;

  if (!gdisplay->frame_manager)
    return;

  list = gdisplay->frame_manager->stores;
  while (list)
    {
      item = (store_t*) list->data;
      if (item->gimage == old)
	{
	  item->gimage = new;
	}
      list = g_slist_next (list);
    }
  printf ("ERROR : something is wrong with onionskin 2\n"); 


}
static ActionAreaItem warning_action_items[] =
{
  { "Close", warning_close, NULL, NULL },   
    { "OK", warning_ok, NULL, NULL }  
};

static int 
create_warning (char* warning, GDisplay *gdisplay)
{
  GtkWidget *label, *vbox;
  frame_manager_t *fm = gdisplay->frame_manager;
  if (!fm->warning)
    {
      fm->warning = gtk_dialog_new ();
      gtk_window_set_wmclass (GTK_WINDOW (fm->warning), "Warning", "Gimp");
      gtk_window_set_policy (GTK_WINDOW (fm->warning), FALSE, FALSE, FALSE);
      gtk_window_set_title (GTK_WINDOW (fm->warning), "Warning");

      vbox = gtk_vbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (fm->warning)->vbox),
	  vbox, TRUE, TRUE, 0);

      label = gtk_label_new (warning); 
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 4);
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
      gtk_widget_show (label);

      warning_action_items[0].user_data = gdisplay;
      warning_action_items[1].user_data = gdisplay;
      build_action_area (GTK_DIALOG (fm->warning), warning_action_items, 2, 0);

      gtk_widget_show (vbox);
      gtk_widget_show (fm->warning);
    }
  else
    if (!GTK_WIDGET_VISIBLE (fm->warning))
      {
	gtk_widget_show (fm->warning);
      }

  return 1; 

}

static gint 
warning_close (GtkWidget *w, gpointer client_pointer)
{
  GDisplay *gdisplay = (GDisplay*) client_pointer; 
  if (GTK_WIDGET_VISIBLE (gdisplay->frame_manager->warning))
    gtk_widget_hide (gdisplay->frame_manager->warning);
  return TRUE;
}

static gint 
warning_ok (GtkWidget *w, gpointer client_pointer)
{
  GDisplay *gdisplay = (GDisplay*) client_pointer; 
  switch (fm_continue)
    {
    case 0:
      step_forward (gdisplay); 
      break;
    case 1:
      step_backwards (gdisplay); 
      break;

    case 2:
      flip_delete (gdisplay);
      break;
    }
  if (GTK_WIDGET_VISIBLE (gdisplay->frame_manager->warning))
    gtk_widget_hide (gdisplay->frame_manager->warning);
  return TRUE;
}

void
frame_manager_set_dirty_flag (GDisplay *gdisplay, int flag)
{
  store_t *item;
  int i;
  GSList *item_list;


  if (!gdisplay || !gdisplay->frame_manager)
  return;

  item_list =  (GList*) GTK_LIST(gdisplay->frame_manager->store_list)->selection;

  if (item_list)
    {

      i = gtk_list_child_position (GTK_LIST (gdisplay->frame_manager->store_list), item_list->data);
      item = (store_t*) g_slist_nth (gdisplay->frame_manager->stores, i)->data;


      item->save = flag ? 0 : item->gimage->dirty; 

      frame_manager_save_redraw (item); 
    }
}

