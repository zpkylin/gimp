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
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "appenv.h"
#include "actionarea.h"
#include "buildmenu.h"
#include "datafiles.h"
#include "displaylut.h"
#include "errors.h"
#include "general.h"
#include "gimprc.h"
#include "interface.h"
#include "paint_funcs_area.h"
#include "frame_manager.h"
#include "tag.h"
#include "gdisplay.h"
#include "gimage.h"
#include "tools.h"
#include "rect_selectP.h"

#define ENTRY_WIDTH  14
#define ENTRY_HEIGHT 10
#define SPACING 1
#define COLUMNS 16
#define ROWS 16

#define PREVIEW_WIDTH ((ENTRY_WIDTH * COLUMNS) + (SPACING * (COLUMNS + 1)))
#define PREVIEW_HEIGHT ((ENTRY_HEIGHT * ROWS) + (SPACING * (ROWS + 1)))

#define FRAME_MANAGER_EVENT_MASK GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_ENTER_NOTIFY_MASK

typedef struct _Frame_Manager _Frame_Manager, *Frame_ManagerP;

struct _Frame_Manager 
{
  GtkWidget *shell;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *frame_num;
  GtkWidget *increment;
  GtkWidget *save_mode; 
  GtkWidget *flip_mode; 
  GtkWidget *loop_mode; 
  GtkWidget *flip_num; 
  GtkWidget *flip_area; 
  GtkWidget *pre_label; 
  GtkWidget *suf_label; 
  int updating;
};



static void frame_manager_flip_area_callback (GtkWidget *, gpointer);
static void frame_manager_update_whole_area_callback (GtkWidget *, gpointer);
static void frame_manager_load_new_frame_callback (GtkWidget *, gpointer);
static void frame_manager_close_callback (GtkWidget *, gpointer);
static void frame_manager_set_frame_num (GtkWidget *, gpointer);
static void frame_manager_set_increment (GtkWidget *, gpointer);
static void frame_manager_save_mode (GtkWidget *, gpointer);
static void frame_manager_flip_mode (GtkWidget *, gpointer);
static void frame_manager_crop_mode (GtkWidget *, gpointer);
static void frame_manager_loop_mode (GtkWidget *, gpointer);
static void frame_manager_save_image (GImage *image);
static void frame_manager_add_frame (GImage *image, char* frame_num, char dir);

typedef struct f_frame
{
  GImage *gimage;
  char frame_num[10];
}frame_t;

static Frame_ManagerP         frame_manager = NULL;
static char save_mode;
static char flip_mode;
static char crop_mode;
static char loop_mode;
static GDisplay *cur_gdisplay; 
static int f_cur=0;
static int f_num=0;
static frame_t *f_array;
static int s_x, s_y, e_x, e_y;

static int F_MAX;

static ActionAreaItem action_items[] =
{
  { "Update Whole Area", frame_manager_update_whole_area_callback, NULL, NULL },
  { "New Flip Area", frame_manager_flip_area_callback, NULL, NULL },
  { "Load New Frame", frame_manager_load_new_frame_callback, NULL, NULL },
  { "Backwards", frame_manager_backwards_callback, NULL, NULL },
  { "Forward", frame_manager_forward_callback, NULL, NULL },
  { "Close", frame_manager_close_callback, NULL, NULL }  
};

void
frame_manager_free ()
{
  if (frame_manager)
    {
      g_free (frame_manager);

      frame_manager = NULL;
    }
}

void
frame_manager_create ()
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *hboxL;
  GtkWidget *hboxR;
  GtkWidget *frame;
  GtkWidget *label; 
  int i;
  char raw[256];
  char *name;
  char *frame_num;
  char *exten;  
  char tmp[256];

  if (!frame_manager)
    {
      cur_gdisplay = gdisplay_active ();
      if (!cur_gdisplay)
	{
	  frame_manager = NULL;
	  return; 
	}
      if (!(cur_gdisplay->gimage->filename))
	{
	  frame_manager = NULL;
	  return; 
	}

      frame_manager = g_malloc (sizeof (_Frame_Manager));


      /*  The shell and main vbox  */
      frame_manager->shell = gtk_dialog_new ();
      gtk_window_set_wmclass (GTK_WINDOW (frame_manager->shell), "frame_manager", "Gimp");
      gtk_window_set_policy (GTK_WINDOW (frame_manager->shell), FALSE, FALSE, FALSE);
      gtk_window_set_title (GTK_WINDOW (frame_manager->shell), "Frame_Manager");
      vbox = gtk_vbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (frame_manager->shell)->vbox), vbox, TRUE, TRUE, 0);


      /* save mode */
      frame_manager->save_mode = gtk_check_button_new_with_label ("auto save mode");
      gtk_box_pack_start (GTK_BOX (vbox), frame_manager->save_mode, FALSE, FALSE, 2);
      gtk_signal_connect (GTK_OBJECT (frame_manager->save_mode), "toggled",
	  (GtkSignalFunc) frame_manager_save_mode,
	  frame_manager);
      gtk_widget_show (frame_manager->save_mode);  
      save_mode = 0;

      /* flip mode */
      hbox = gtk_hbox_new (FALSE, 1); 
      gtk_container_border_width (GTK_CONTAINER (hbox), 1);
      gtk_box_set_spacing (GTK_BOX (hbox), 40); 
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);     
      
      frame_manager->flip_mode = gtk_check_button_new_with_label ("flip mode");
      gtk_box_pack_start (GTK_BOX (hbox), frame_manager->flip_mode, FALSE, FALSE, 2);
      gtk_signal_connect (GTK_OBJECT (frame_manager->flip_mode), "toggled",
	  (GtkSignalFunc) frame_manager_flip_mode,
	  frame_manager);
      gtk_widget_show (frame_manager->flip_mode);  
      flip_mode = 0;
      
      
      label = gtk_label_new ("Flip Number:   "); 
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
      gtk_widget_show (label);
      
      frame_manager->flip_num = gtk_entry_new ();
      gtk_widget_set_usize (GTK_WIDGET (frame_manager->flip_num), 50, 0);  
      gtk_entry_set_text (GTK_ENTRY (frame_manager->flip_num), "3"); 
      gtk_box_pack_start (GTK_BOX (hbox), frame_manager->flip_num, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (frame_manager->flip_num), "changed",
	  (GtkSignalFunc) frame_manager_set_increment,
	  frame_manager);
      gtk_widget_show (frame_manager->flip_num);

      
      frame_manager->loop_mode = gtk_check_button_new_with_label ("loop mode");
      gtk_box_pack_start (GTK_BOX (hbox), frame_manager->loop_mode, FALSE, FALSE, 2);
      gtk_signal_connect (GTK_OBJECT (frame_manager->loop_mode), "toggled",
	  (GtkSignalFunc) frame_manager_loop_mode,
	  frame_manager);
      gtk_widget_show (frame_manager->loop_mode);  
      loop_mode = 0;
      
      /*frame_manager->crop_mode = gtk_check_button_new_with_label ("crop mode");
      gtk_box_pack_start (GTK_BOX (hbox), frame_manager->crop_mode, FALSE, FALSE, 2);
      gtk_signal_connect (GTK_OBJECT (frame_manager->crop_mode), "toggled",
	  (GtkSignalFunc) frame_manager_crop_mode,
	  frame_manager);
      gtk_widget_show (frame_manager->crop_mode);  
      crop_mode = 0;*/
      
      hbox = gtk_hbox_new (FALSE, 1); 
      gtk_container_border_width (GTK_CONTAINER (hbox), 1);
      gtk_box_set_spacing (GTK_BOX (hbox), 40); 
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);     
      hboxL = gtk_hbox_new (FALSE, 1); 
      gtk_container_border_width (GTK_CONTAINER (hboxL), 1);
      gtk_box_pack_start (GTK_BOX (hbox), hboxL, TRUE, TRUE, 0);
      gtk_widget_show (hboxL);     

      /* label */
      label = gtk_label_new ("Frame Number:   "); 
      gtk_box_pack_start (GTK_BOX (hboxL), label, FALSE, FALSE, 4);
      gtk_widget_show (label);

      /* text window */
      strcpy (raw, prune_filename (cur_gdisplay->gimage->filename));

      name = strtok (raw, ".");
      frame_num = strtok (NULL, ".");
      exten = strtok (NULL, ".");
      frame_manager->pre_label = gtk_label_new (name); 
      gtk_box_pack_start (GTK_BOX (hboxL), frame_manager->pre_label, FALSE, FALSE, 0);
      gtk_widget_show (frame_manager->pre_label);
      frame_manager->frame_num = gtk_entry_new ();
      gtk_widget_set_usize (GTK_WIDGET (frame_manager->frame_num), 50, 0);  
      gtk_entry_set_text (GTK_ENTRY (frame_manager->frame_num), frame_num);
      gtk_box_pack_start (GTK_BOX (hboxL), frame_manager->frame_num, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (frame_manager->frame_num), "changed",
	  (GtkSignalFunc) frame_manager_set_frame_num,
	  frame_manager);
      gtk_widget_show (frame_manager->frame_num);
      sprintf (tmp, ".%s", exten); 
      frame_manager->suf_label = gtk_label_new (tmp); 
      gtk_box_pack_start (GTK_BOX (hboxL), frame_manager->suf_label, FALSE, FALSE, 0);
      gtk_widget_show (frame_manager->suf_label);

      hboxR = gtk_hbox_new (FALSE, 1); 
      gtk_container_border_width (GTK_CONTAINER (hboxR), 1);
      gtk_box_pack_end (GTK_BOX (hbox), hboxR, TRUE, TRUE, 0);
      gtk_widget_show (hboxR);     
      label = gtk_label_new ("Increment Step:   "); 
      gtk_box_pack_start (GTK_BOX (hboxR), label, FALSE, FALSE, 4);
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);

      frame_manager->increment = gtk_entry_new ();
      gtk_widget_set_usize (GTK_WIDGET (frame_manager->increment), 50, 0);  
      gtk_entry_set_text (GTK_ENTRY (frame_manager->increment), "1"); 
      gtk_box_pack_start (GTK_BOX (hboxR), frame_manager->increment, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (frame_manager->increment), "changed",
	  (GtkSignalFunc) frame_manager_set_increment,
	  frame_manager);
      gtk_widget_show (label);
      gtk_widget_show (frame_manager->increment);


      /*  The action area  */
      action_items[0].user_data = frame_manager;
      action_items[1].user_data = frame_manager;
      action_items[2].user_data = frame_manager;
      action_items[3].user_data = frame_manager;
      action_items[4].user_data = frame_manager;
      action_items[5].user_data = frame_manager;
      build_action_area (GTK_DIALOG (frame_manager->shell), action_items, 6, 0);

      gtk_widget_show (vbox);
      gtk_widget_show (frame_manager->shell);

    }
  else
    {
      if (!GTK_WIDGET_VISIBLE (frame_manager->shell))
	{
	  cur_gdisplay = gdisplay_active ();
	  if (cur_gdisplay)
	    {
	      strcpy (raw, prune_filename (cur_gdisplay->gimage->filename));

	      name = strtok (raw, ".");
	      frame_num = strtok (NULL, ".");
	      exten = strtok (NULL, ".");
	      gtk_entry_set_text (GTK_ENTRY (frame_manager->frame_num), frame_num);
	      gtk_label_set (frame_manager->pre_label, name); 
	      sprintf (tmp, ".%s", exten); 
	      gtk_label_set (frame_manager->suf_label, tmp); 
	      gtk_widget_show (frame_manager->shell);
	    }
	}
      else
	{
	  cur_gdisplay = gdisplay_active ();
	  if (cur_gdisplay)
	    {
	      strcpy (raw, prune_filename (cur_gdisplay->gimage->filename));

	      name = strtok (raw, ".");
	      frame_num = strtok (NULL, ".");
	      exten = strtok (NULL, ".");
	      gtk_entry_set_text (GTK_ENTRY (frame_manager->frame_num), frame_num);
	      gtk_label_set (frame_manager->pre_label, name); 
	      sprintf (tmp, ".%s", exten); 
	      gtk_label_set (frame_manager->suf_label, tmp); 
	      gdk_window_raise(frame_manager->shell->window);
	    }
	}
    }
}


static void
frame_manager_set_frame_num (GtkWidget *w,
    gpointer   client_data)
{
}


static void
frame_manager_set_increment (GtkWidget *w,
    gpointer   client_data)
{
  Frame_ManagerP frame_manager;

  frame_manager = client_data;
  if (frame_manager)
    {
    }
}

static void
frame_manager_load_new_frame_callback (GtkWidget *w,
    gpointer   client_data)
{
  Frame_ManagerP frame_manager;
  GImage *gimage;
  char raw[256];
  char whole[256];
  char *name;
  char *frame;
  char *exten;
  char *dir;
  char tmp[256]; 
  int dir_len; 
      
  GDisplay *cdisp = gdisplay_active ();


  frame_manager = client_data;
  if (frame_manager && cdisp && cdisp == cur_gdisplay && !loop_mode)
    {

      /* get filename */
      gimage = cur_gdisplay->gimage;
      strcpy (whole, cur_gdisplay->gimage->filename);
      strcpy (raw, prune_filename (whole));
      dir_len = strlen (whole) - strlen (raw); 

      if (save_mode) 
	file_save (cur_gdisplay->gimage->ID, whole, raw);
     
      
      name = strtok (raw, ".");
      frame = strtok (NULL, ".");
      exten = strtok (NULL, ".");

      sprintf (tmp, "%s.%s.%s", name, GTK_ENTRY (frame_manager->frame_num)->text, exten);
      sprintf (&(whole[dir_len]), "%s", tmp);    
	  

      if (file_load (whole, tmp, cur_gdisplay))  
	{
	  cur_gdisplay->ID = cur_gdisplay->gimage->ID; 
	 if (!flip_mode) 
	    gimage_delete (gimage);
	  if (flip_mode)
	    frame_manager_add_frame (cur_gdisplay->gimage, GTK_ENTRY (frame_manager->frame_num)->text, 1);
	}
    }

}

void
frame_manager_backwards_callback (GtkWidget *w,
    gpointer   client_data)
{
  Frame_ManagerP f_m;
  GImage *gimage;
  char raw[256];
  char whole[256];
  char *name;
  char *frame;
  char *exten;
  char *dir;
  int num;
  char new_frame[256];
  char tmp[256]; 
  int dir_len, len, org_len; 
  GDisplay *cdisp = gdisplay_active ();

  menus_set_sensitive ("<Image>/Dialogs/Frame Manager...", TRUE); 


  f_m = client_data;
  if (frame_manager && cdisp && cdisp == cur_gdisplay)
    {

      if (flip_mode && loop_mode)
	{
	  f_cur --;
	  if (f_cur < 0) f_cur = f_num-1;
	  cur_gdisplay->gimage = f_array[f_cur].gimage;

	  gdisplays_update_title (cur_gdisplay->gimage->ID);

	  
	  gtk_entry_set_text (GTK_ENTRY (frame_manager->frame_num), f_array[f_cur].frame_num);
	  /*gdisplays_update_full (cur_gdisplay->gimage->ID);*/
	  gdisplay_add_update_area (cur_gdisplay, s_x, s_y, e_x, e_y);
	  cur_gdisplay->ID = cur_gdisplay->gimage->ID; 
	  gdisplays_flush ();   
	  return;
	}
      
      if (flip_mode && f_cur > 0)
	{
	  /*if (f_cur == f_num && cur_gdisplay->gimage != f_array[f_num-1].gimage)
	    frame_manager_add_frame (cur_gdisplay->gimage, "s", 0);
*/
	  f_cur --;
	  if (f_cur < 0) f_cur = 0;

	  cur_gdisplay->gimage = f_array[f_cur].gimage;

	  
	 

	 gdisplays_update_title (cur_gdisplay->gimage->ID);
 	 
	  gtk_entry_set_text (GTK_ENTRY (frame_manager->frame_num), f_array[f_cur].frame_num);
	 /*gdisplays_update_full (cur_gdisplay->gimage->ID);*/
	  cur_gdisplay->ID = cur_gdisplay->gimage->ID; 
	 gdisplay_add_update_area (cur_gdisplay, s_x, s_y, e_x, e_y);	
	
	 gdisplays_flush ();   
       	 return; 
	}

      /* get filename */
      gimage = cur_gdisplay->gimage; 
      strcpy (whole, cur_gdisplay->gimage->filename);
      strcpy (raw, prune_filename (whole));
      dir_len = strlen (whole) - strlen (raw); 

      if (save_mode  && cur_gdisplay->gimage->dirty) 
	file_save (cur_gdisplay->gimage->ID, whole, raw);


      name = strtok (raw, ".");
      frame = strtok (NULL, ".");
      exten = strtok (NULL, ".");
      org_len = strlen (frame);
      num = atoi (frame);       
      num -= atoi (GTK_ENTRY (frame_manager->increment)->text);
      sprintf (tmp, "%d", num);  
      len = strlen (tmp);
      switch (org_len)
	{
	case 2:
	  if (len == 1)
	    sprintf (new_frame, "0%s", tmp);
	  else 
	    sprintf (new_frame, "%s", tmp);
	  break; 
	case 3:
	  switch (len)
	    {
	    case 1:
	      sprintf (new_frame, "00%s", tmp);
	      break;
	    case 2:
	      sprintf (new_frame, "0%s", tmp);
	      break;
	    default:
	      sprintf (new_frame, "%s", tmp);
	      break;
	    }
	  break; 
	case 4:
	  switch (len)
	    {
	    case 1:
	      sprintf (new_frame, "000%s", tmp);
	      break;
	    case 2:
	      sprintf (new_frame, "00%s", tmp);
	      break;
	    case 3:
	      sprintf (new_frame, "0%s", tmp);
	      break;
	    default:
	      sprintf (new_frame, "%s", tmp);
	      break;  
	    }
	  break; 
	default:
	  sprintf (new_frame, "%s", tmp);
	  break;
	}
      sprintf (tmp, "%s.%s.%s", name, new_frame, exten);
      sprintf (&(whole[dir_len]), "%s", tmp);    
      gtk_entry_set_text (GTK_ENTRY (frame_manager->frame_num), new_frame);  
      
      
      if (file_load (whole, tmp, cur_gdisplay))  
	{
	  cur_gdisplay->ID = cur_gdisplay->gimage->ID; 
	  if (!flip_mode)
	    gimage_delete (gimage);
	  if (flip_mode)
	    frame_manager_add_frame (cur_gdisplay->gimage, new_frame, 0);
	}
    }
}


void
frame_manager_forward_callback (GtkWidget *w,
    gpointer   client_data)
{
  Frame_ManagerP f_m;
  GImage *gimage; 
  char raw[256];
  char whole[256];
  char *name;
  char *frame;
  char *exten;
  char *dir;
  int num;
  char new_frame[256];
  char tmp[256]; 
  int dir_len, len, org_len; 
  GDisplay *cdisp = gdisplay_active ();

  f_m = client_data;
  if (frame_manager && cdisp && cdisp == cur_gdisplay)
    {
      
      if (flip_mode && loop_mode)
	{
	  f_cur ++;
	  if (f_cur >= f_num) f_cur = 0;
	  cur_gdisplay->gimage = f_array[f_cur].gimage;
	
	  gdisplays_update_title (cur_gdisplay->gimage->ID);

	  gtk_entry_set_text (GTK_ENTRY (frame_manager->frame_num), f_array[f_cur].frame_num);
	  
	  /*gdisplays_update_full (cur_gdisplay->gimage->ID);*/
	  cur_gdisplay->ID = cur_gdisplay->gimage->ID; 
	  gdisplay_add_update_area (cur_gdisplay, s_x, s_y, e_x, e_y);
	  gdisplays_flush ();   
	  return;
	}
      
      if (flip_mode && f_cur+1 < f_num)
	{
	 
	  f_cur++; 
	  cur_gdisplay->gimage = f_array[f_cur].gimage;
	 
	  gdisplays_update_title (cur_gdisplay->gimage->ID); 
	  gtk_entry_set_text (GTK_ENTRY (frame_manager->frame_num), f_array[f_cur].frame_num);
	  /*gdisplays_update_full (cur_gdisplay->gimage->ID);*/
	 

	  cur_gdisplay->ID = cur_gdisplay->gimage->ID; 
	 gdisplay_add_update_area (cur_gdisplay, s_x, s_y, e_x, e_y);	

	gdisplays_flush ();   

	  return; 
	}
      /* get filename */
      gimage = cur_gdisplay->gimage; 
      strcpy (whole, cur_gdisplay->gimage->filename);
      strcpy (raw, prune_filename (whole));
      dir_len = strlen (whole) - strlen (raw); 

      if (save_mode  && cur_gdisplay->gimage->dirty) 
	file_save (cur_gdisplay->gimage->ID, whole, raw); 
      name = strtok (raw, ".");
      frame = strtok (NULL, ".");
      exten = strtok (NULL, ".");
      org_len = strlen (frame); 
      num = atoi (frame);       
      num += atoi (GTK_ENTRY (frame_manager->increment)->text);
      sprintf (tmp, "%d", num);  
      len = strlen (tmp);
      switch (org_len)
	{
	case 2:
	  if (len == 1)
	    sprintf (new_frame, "0%s", tmp);
	  else 
	    sprintf (new_frame, "%s", tmp);
	  break; 
	case 3:
	  switch (len)
	    {
	    case 1:
	      sprintf (new_frame, "00%s", tmp);
	      break;
	    case 2:
	      sprintf (new_frame, "0%s", tmp);
	      break;
	    default:
	      sprintf (new_frame, "%s", tmp);
	      break;
	    }
	  break; 
	case 4:
	  switch (len)
	    {
	    case 1:
	      sprintf (new_frame, "000%s", tmp);
	      break;
	    case 2:
	      sprintf (new_frame, "00%s", tmp);
	      break;
	    case 3:
	      sprintf (new_frame, "0%s", tmp);
	      break;
	    default:
	      sprintf (new_frame, "%s", tmp);
	      break;  
	    }
	  break; 
	default:
	  sprintf (new_frame, "%s", tmp);
	  break;
	}
      sprintf (tmp, "%s.%s.%s", name, new_frame, exten);
      sprintf (&(whole[dir_len]), "%s", tmp);    
      gtk_entry_set_text (GTK_ENTRY (frame_manager->frame_num), new_frame);
	  

      if (file_load (whole, tmp, cur_gdisplay))  
	{
	  cur_gdisplay->ID = cur_gdisplay->gimage->ID; 
	  if (!flip_mode) 
	    gimage_delete (gimage);
	  if (flip_mode)
	    frame_manager_add_frame (cur_gdisplay->gimage, new_frame, 1);
    	
	}
    }
}


static void
frame_manager_close_callback (GtkWidget *w,
    gpointer   client_data)
{
  Frame_ManagerP frame_manager;
  int i;
  GDisplay *cdisp = gdisplay_active ();

  
  frame_manager = client_data;
  if (frame_manager)
    {
      if (GTK_WIDGET_VISIBLE (frame_manager->shell))
	gtk_widget_hide (frame_manager->shell); 

      if (cdisp && cdisp == cur_gdisplay)
	{
	  for (i=0; i<f_num; i++)
	    {

	      if (f_array[i].gimage != cur_gdisplay->gimage)
		{

		  if (save_mode)
		    frame_manager_save_image (f_array[i].gimage); 
		  gimage_delete (f_array[i].gimage);

		}

	    }
	  free (f_array);
	}
      f_num = f_cur = 0; 

      flip_mode = 0;

    }
}

static void
frame_manager_update_whole_area_callback (GtkWidget *w,
        gpointer   client_data)
{
  Frame_ManagerP frame_manager;
  int i;
  RectSelect * rect_sel;
  GDisplay *cdisp = gdisplay_active ();


  frame_manager = client_data;
  if (frame_manager && cdisp && cdisp == cur_gdisplay)
    {
	  gdisplays_update_full (cur_gdisplay->gimage->ID);
	  /*
      gdisplay_add_update_area (cur_gdisplay, 0, 0, 
	  canvas_width (cur_gdisplay->gimage->projection), 
	  canvas_height (cur_gdisplay->gimage->projection));
	  */
      gdisplays_flush ();   
    }
}


static void
frame_manager_flip_area_callback (GtkWidget *w,
    gpointer   client_data)
{
  Frame_ManagerP frame_manager;
  int i;
  RectSelect * rect_sel;
  GDisplay *cdisp = gdisplay_active ();

  
  frame_manager = client_data;
  if (frame_manager && cdisp && cdisp == cur_gdisplay)
    {
	  
      if (active_tool->type == RECT_SELECT)
	{
	  rect_sel = (RectSelect *) active_tool->private;
	  s_x = MINIMUM (rect_sel->x, rect_sel->x + rect_sel->w);
	  s_y = MINIMUM (rect_sel->y, rect_sel->y + rect_sel->h);
	  e_x = MAXIMUM (rect_sel->x, rect_sel->x + rect_sel->w) - s_x;
	  e_y = MAXIMUM (rect_sel->y, rect_sel->y + rect_sel->h) - s_y;


	  if (!s_x && !s_y && !e_x && !e_y)
	    {
	      s_x = 0;
	      s_y = 0;
	      e_x = cur_gdisplay->disp_width;
	      e_y = cur_gdisplay->disp_height;

	    }  
	}
      else
	{
	  s_x = 0;
	  s_y = 0;
	  e_x = cur_gdisplay->disp_width;
	  e_y = cur_gdisplay->disp_height;  

	  return; 
	}


    }
}


static void 
frame_manager_save_mode (GtkWidget *w, gpointer client_data)
{

  Frame_ManagerP frame_manager;

  frame_manager = client_data;

  if (frame_manager)
    {
      if (GTK_TOGGLE_BUTTON (w)->active)
	save_mode = 1;
      else
	save_mode = 0;     
    }

}

static void
frame_manager_flip_mode (GtkWidget *w, gpointer client_data)
{
  
  Frame_ManagerP frame_manager;
  int i;
  RectSelect * rect_sel;
  GDisplay *cdisp = gdisplay_active ();

  char whole[256], raw[256], *name, *frame; 

  frame_manager = client_data;

  if (frame_manager && cdisp && cdisp == cur_gdisplay)
    {
      if (GTK_TOGGLE_BUTTON (w)->active)
	{
	  f_cur = -1;
	  f_num = 0;
	  flip_mode = 1;

	  F_MAX = atoi(GTK_ENTRY (frame_manager->flip_num)->text); 
	  f_array = (frame_t*) malloc (sizeof (frame_t) * F_MAX);  

	  strcpy (whole, cur_gdisplay->gimage->filename);
	  strcpy (raw, prune_filename (whole));

	  name = strtok (raw, ".");
	  frame = strtok (NULL, ".");

	  frame_manager_add_frame (cur_gdisplay->gimage, frame, 1);


	  if (active_tool->type == RECT_SELECT)
	    {
	      rect_sel = (RectSelect *) active_tool->private;
	      s_x = MINIMUM (rect_sel->x, rect_sel->x + rect_sel->w);
	      s_y = MINIMUM (rect_sel->y, rect_sel->y + rect_sel->h);
	      e_x = MAXIMUM (rect_sel->x, rect_sel->x + rect_sel->w) - s_x;
	      e_y = MAXIMUM (rect_sel->y, rect_sel->y + rect_sel->h) - s_y;


	      if (!s_x && !s_y && !e_x && !e_y)
		{
		  s_x = 0;
		  s_y = 0;
		  e_x = cur_gdisplay->disp_width;
		  e_y = cur_gdisplay->disp_height;

		}  
	    }
	  else
	    {
	      s_x = 0;
	      s_y = 0;
	      e_x = cur_gdisplay->disp_width;
	      e_y = cur_gdisplay->disp_height;  

	      return; 
	    }
	  
	}
      else
	{

	  for (i=0; i<f_num; i++)
	    {
	      if (f_array[i].gimage != cur_gdisplay->gimage)
		{
		  if (save_mode)
		    frame_manager_save_image (f_array[i].gimage); 
		  gimage_delete (f_array[i].gimage);
		}
	    }
	  free (f_array);
	  f_num = f_cur = 0; 
	  
	  flip_mode = 0;
	}
    }

}

static void 
frame_manager_loop_mode (GtkWidget *w, gpointer client_data)
{

  Frame_ManagerP frame_manager;

  frame_manager = client_data;

  if (frame_manager)
    {
      if (GTK_TOGGLE_BUTTON (w)->active)
	loop_mode = 1;
      else
	loop_mode = 0;     
    }
}

static void 
frame_manager_crop_mode (GtkWidget *w, gpointer client_data)
{

  Frame_ManagerP frame_manager;

  frame_manager = client_data;

  if (frame_manager)
    {
      if (GTK_TOGGLE_BUTTON (w)->active)
	crop_mode = 1;
      else
	crop_mode = 0;     
    }
}

static void
frame_manager_save_image (GImage *gimage)
{
  char whole[256], raw[256];
  
  /* get filename */
  strcpy (whole, gimage->filename);
  strcpy (raw, prune_filename (whole));
    
  file_save (gimage->ID, whole, raw); 

}

static void
frame_manager_add_frame (GImage *gimage, char *frame_num, char dir)
{

  int i;

  if (dir)
    {
      if (f_num == F_MAX)
	{
	  if (save_mode) 
	    frame_manager_save_image (f_array[0].gimage); 
	  gimage_delete (f_array[0].gimage);
	  for (i=0; i<F_MAX-1; i++)
	    {
	      f_array[i].gimage = f_array[i+1].gimage;
	      strcpy (f_array[i].frame_num, f_array[i+1].frame_num); 
	    }
	  f_num --;      
	  f_cur --;  
	}


      f_array[f_num  ].gimage = gimage;  
      strcpy (f_array[f_num++].frame_num, frame_num);  

      f_cur += 1; 
    }
  else
    {
    
      if (f_cur == 0)
	{
	  if (f_num == F_MAX)
	    {

	      if (save_mode) 
		frame_manager_save_image (f_array[f_num-1].gimage); 
	      gimage_delete (f_array[f_num-1].gimage);
	      f_num --;   
	    }

	  for (i=f_num; i>0; i--)
	    {
	      f_array[i].gimage = f_array[i-1].gimage;
	      strcpy (f_array[i].frame_num, f_array[i-1].frame_num);
	    }
	}

      f_array[0].gimage = gimage;
      strcpy (f_array[0].frame_num, frame_num);
	
      f_num ++; 
    }
}


