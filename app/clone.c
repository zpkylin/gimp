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
#include <math.h> 
#include "appenv.h"
#include "gimpbrush.h"
#include "gimpbrushlist.h"
#include "canvas.h"
#include "clone.h"
#include "draw_core.h"
#include "drawable.h"
#include "gimage.h"
#include "gdisplay.h"
#include "interface.h"
#include "paint_core_16.h"
#include "paint_funcs_area.h"
#include "patterns.h"
#include "pixelarea.h"
#include "procedural_db.h"
#include "tools.h"
#include "transform_core.h"
#include "devices.h"
#include "frame_manager.h"

#define TARGET_HEIGHT  15
#define TARGET_WIDTH   15

static double clone_angle = M_PI/2.0;
static double clone_scale = 1; 
static double clone_scale_norm = 1.0; 

typedef enum
{
  AlignYes,
  AlignNo,
  AlignRegister
} AlignType;

typedef struct _CloneOptions CloneOptions;

/*  forward function declarations  */
static void         clone_draw            (Tool *);
static void         clone_motion          (PaintCore *, GimpDrawable *, int, int);
static int          clone_line_image      (PaintCore *, Canvas *, GimpDrawable *, 
    						GimpDrawable *, int, int);
static Argument *   clone_invoker         (Argument *);
static void         clone_painthit_setup  (PaintCore *, Canvas *);
static void	    create_dialog         (char type); 
static void	    create_offset_dialog  (); 
static gint	    clone_x_offset        (GtkWidget *, gpointer); 
static gint	    clone_x_offset_down   (GtkWidget *, gpointer); 
static gint	    clone_x_offset_up     (GtkWidget *, gpointer); 
static gint	    clone_y_offset        (GtkWidget *, gpointer); 
static gint	    clone_y_offset_down   (GtkWidget *, gpointer); 
static gint	    clone_y_offset_up     (GtkWidget *, gpointer); 
static gint	    clone_x_scale         (GtkWidget *, gpointer); 
static gint	    clone_y_scale         (GtkWidget *, gpointer); 
static gint	    clone_rotate          (GtkWidget *, gpointer); 
static void         clone_set_offset      (CloneOptions*, int, int); 

static GimpDrawable *non_gui_source_drawable;
static GimpDrawable *source_drawable;
static int 	    non_gui_offset_x;
static int 	    non_gui_offset_y;
static int  	    setup_successful;
static char	    draw;
static char	    clean_up = 0;

struct _CloneOptions
{
  AlignType aligned;
  GtkWidget *offset_frame;
  GtkWidget *trans_frame; 

  GtkWidget *x_offset_entry;
  GtkWidget *y_offset_entry;
  GtkWidget *x_scale_entry;
  GtkWidget *y_scale_entry;
  GtkWidget *rotate_entry;
};

/*  local variables  */
static int 	    clone_point_set = FALSE;
static int          saved_x = 0;                /*                         */
static int          saved_y = 0;                /*  position of clone src  */
static int          src_x = 0;                  /*                         */
static int          src_y = 0;                  /*  position of clone src  */
static int          dest_x = 0;                 /*                         */
static int          dest_y = 0;                 /*  position of clone dst  */
static int          dest2_y = 0;                /*                         */
static int          dest2_x = 0;                /*  position of clone dst  */
static int          offset_x = 0;               /*                         */
static int          offset_y = 0;               /*  offset for cloning     */
static int	    error_x = 0;		/*			   */ 
static int	    error_y = 0;		/*  error in x and y	   */
static double       scale_x = 1;                /*                         */
static double       scale_y = 1;                /*  offset for cloning     */
static double       SCALE_X = 1;                /*                         */
static double       SCALE_Y = 1;                /*  offset for cloning     */
static double       rotate = 0;                 /*  offset for cloning     */
static char         first_down = TRUE;
static char         first_up = TRUE;
static char         first_mv = TRUE;
static char         second_mv = TRUE;
static int          trans_tx, trans_ty;         /*  transformed target  */
static int          dtrans_tx, dtrans_ty;         /*  transformed target  */
static int          src_gdisp_ID = -1;          /*  ID of source gdisplay  */
static CloneOptions *clone_options = NULL;
static	char	    rm_cross=0; 
static char 	    clone_now = TRUE; 
static char 	    dont_dist = 1; 
static	char	    expose = 0;


static void
align_type_callback (GtkWidget *w,
		     gpointer  client_data)
{
  clone_options->aligned = (AlignType) client_data;
  clone_point_set = FALSE;
  switch (clone_options->aligned)
    {
    case AlignYes:
      gtk_widget_set_sensitive (clone_options->trans_frame, TRUE); 
      gtk_widget_set_sensitive (clone_options->offset_frame, TRUE); 
      break;
    case AlignNo:
      gtk_widget_set_sensitive (clone_options->trans_frame, FALSE); 
      gtk_widget_set_sensitive (clone_options->offset_frame, FALSE); 
      break;
    case AlignRegister:
      gtk_widget_set_sensitive (clone_options->trans_frame, FALSE); 
      gtk_widget_set_sensitive (clone_options->offset_frame, FALSE); 
      break;
    default:
      break;
    }
}


static CloneOptions *
create_clone_options (void)
{
  CloneOptions *options;
  GtkWidget *vbox, *hbox, *lvbox, *rvbox, *rotbox, *scalebox, *rhbox, *bvbox;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *radio_box;
  GtkWidget *radio_button;
  GtkWidget *separator; 
  GtkWidget *entry, *button; 
  GSList *group = NULL;
  int i;
  char *align_names[3] =
  {
    "Aligned",
    "Non Aligned",
    "Registered",
  };

  /*  the new options structure  */
  options = (CloneOptions *) g_malloc (sizeof (CloneOptions));
  options->aligned = AlignYes;

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);

  /*  the main label  */
  label = gtk_label_new ("Clone Tool Options");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /* hbox */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  lvbox = gtk_vbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (hbox), lvbox, FALSE, FALSE, 0);
  gtk_widget_show (lvbox);

  /*  the radio frame and box  */
  frame = gtk_frame_new ("Alignment");
  gtk_box_pack_start (GTK_BOX (lvbox), frame, FALSE, FALSE, 0);
  
  /*  the radio buttons  */
  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (frame), radio_box);

  group = NULL;  
  for (i = 0; i < 3; i++)
    {
      radio_button = gtk_radio_button_new_with_label (group, align_names[i]);
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  (GtkSignalFunc) align_type_callback,
			  (void *)((long) i));
      gtk_box_pack_start (GTK_BOX (radio_box), radio_button, FALSE, FALSE, 0);
      gtk_widget_show (radio_button);
    }
  gtk_widget_show (radio_box);
  gtk_widget_show (frame);
  
  /* add the offset entry */
  options->offset_frame = frame = gtk_frame_new ("Offset (x,y)");
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);
  
  rvbox = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (frame), rvbox);
  gtk_widget_show (rvbox);

  rhbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (rvbox), rhbox, FALSE, FALSE, 0);
  gtk_widget_show (rhbox);
 
 
  options->x_offset_entry = entry = gtk_entry_new ();
  gtk_widget_set_usize (GTK_WIDGET (entry), 40, 0);
  gtk_entry_set_text (GTK_ENTRY (entry), "0");
  gtk_box_pack_start (GTK_BOX (rhbox), entry, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
      (GtkSignalFunc) clone_x_offset,
      options);
  gtk_widget_show (entry);
  
  button = gtk_button_new_with_label ("UP");
  gtk_box_pack_start (GTK_BOX (rhbox), button, FALSE, FALSE, 2);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
      (GtkSignalFunc) clone_x_offset_up,
      options);
  gtk_widget_show (button);
  
  button = gtk_button_new_with_label ("DOWN");
  gtk_box_pack_start (GTK_BOX (rhbox), button, FALSE, FALSE, 2);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
      (GtkSignalFunc) clone_x_offset_down,
      options);
  gtk_widget_show (button);
  
  options->y_offset_entry = entry = gtk_entry_new ();
  gtk_widget_set_usize (GTK_WIDGET (entry), 40, 0);
  gtk_entry_set_text (GTK_ENTRY (entry), "0");
  gtk_box_pack_start (GTK_BOX (rhbox), entry, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
      (GtkSignalFunc) clone_y_offset,
      options);
  gtk_widget_show (entry);
  
  button = gtk_button_new_with_label ("UP");
  gtk_box_pack_start (GTK_BOX (rhbox), button, FALSE, FALSE, 2);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
      (GtkSignalFunc) clone_y_offset_up,
      options);
  gtk_widget_show (button);
  
  button = gtk_button_new_with_label ("DOWN");
  gtk_box_pack_start (GTK_BOX (rhbox), button, FALSE, FALSE, 2);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
      (GtkSignalFunc) clone_y_offset_down,
      options);
  gtk_widget_show (button);
 
  /* transformation */
  options->trans_frame = frame = gtk_frame_new ("Transformation");
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);
 
  
  bvbox = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (frame), bvbox);
  gtk_widget_show (bvbox);
  
  /* rotate */
  rotbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (bvbox), rotbox, FALSE, FALSE, 0);
  gtk_widget_show (rotbox);

  label = gtk_label_new ("Rotate\t"); 
  gtk_box_pack_start (GTK_BOX (rotbox), label, FALSE, FALSE, 4);
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_widget_show (label);

  options->rotate_entry = entry = gtk_entry_new ();
  gtk_widget_set_usize (GTK_WIDGET (entry), 40, 0);  
  gtk_entry_set_text (GTK_ENTRY (entry), "0"); 
  gtk_box_pack_start (GTK_BOX (rotbox), entry, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
      (GtkSignalFunc) clone_rotate, options);
  gtk_widget_show (entry);
 
  /* scale */
  scalebox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (bvbox), scalebox, FALSE, FALSE, 0);
  gtk_widget_show (scalebox);

  label = gtk_label_new ("Scale\t\t"); 
  gtk_box_pack_start (GTK_BOX (scalebox), label, FALSE, FALSE, 4);
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_widget_show (label);

  options->x_scale_entry = entry = gtk_entry_new ();
  gtk_widget_set_usize (GTK_WIDGET (entry), 40, 0);  
  gtk_entry_set_text (GTK_ENTRY (entry), "1"); 
  gtk_box_pack_start (GTK_BOX (scalebox), entry, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
      (GtkSignalFunc) clone_x_scale,
      options);
  gtk_widget_show (entry);

  options->y_scale_entry = entry = gtk_entry_new ();
  gtk_widget_set_usize (GTK_WIDGET (entry), 40, 0);  
  gtk_entry_set_text (GTK_ENTRY (entry), "1"); 
  gtk_box_pack_start (GTK_BOX (scalebox), entry, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
      (GtkSignalFunc) clone_y_scale, options);
  gtk_widget_show (entry);
  
  /*  Register this selection options widget with the main tools options dialog  */
  tools_register_options (CLONE, vbox);

  return options;
}
static void *
clone_cursor_func (PaintCore *paint_core,
		  GimpDrawable *drawable,
		  int        state)
{
  GDisplay * gdisp;
  GDisplay * src_gdisp;
  
  int x1, y1, x2, y2; 
  gdisp = (GDisplay *) active_tool->gdisp_ptr;

  if (!gdisp || !paint_core || !drawable)
    {
      clone_point_set = 0; 
      return 0;
    }
  if (clean_up)
    return 0; 
  x1 = paint_core->curx;
  y1 = paint_core->cury;
  
 
  
  if (active_tool->state == INACTIVE && clone_point_set && first_down && !first_up 
      && clone_options->aligned == AlignYes)
    {
      clone_now = FALSE; 
      src_gdisp = gdisplay_get_ID (src_gdisp_ID);
      if (!src_gdisp)
	{
	  src_gdisp_ID = gdisp->ID;
	  src_gdisp = gdisplay_get_ID (src_gdisp_ID);
	}
      if (src_gdisp != gdisp)
	dont_dist = 1;
      else
	dont_dist = 0;

      if (src_gdisp && !first_mv)
	{
	  draw = 0;
	  draw_core_pause (paint_core->core, active_tool);
	  gdisplay_transform_coords (src_gdisp, saved_x, saved_y, &trans_tx, &trans_ty, 1);
	  gdisplay_transform_coords (src_gdisp, x1, y1, &dtrans_tx, &dtrans_ty, 1);
	  draw = 1;
	  draw_core_resume (paint_core->core, active_tool);
	}
      if (first_mv && src_gdisp)
	{
	  gdisplay_transform_coords (src_gdisp, saved_x, saved_y, &trans_tx, &trans_ty, 1);
	  gdisplay_transform_coords (gdisp, x1, y1, &dtrans_tx, &dtrans_ty, 1);
	  draw = 1;
	  draw_core_start (paint_core->core, src_gdisp->canvas->window, active_tool);
	  first_mv = FALSE;
	}
    }
  else 
  if (active_tool->state == INACTIVE && clone_point_set && !first_down && 
      clone_options->aligned == AlignYes && !first_up) 
    {
      x2 = x1 + offset_x;
      y2 = y1 + offset_y;

      src_gdisp = gdisplay_get_ID (src_gdisp_ID);
      if (!src_gdisp)
	{
	  src_gdisp_ID = gdisp->ID;
	  src_gdisp = gdisplay_get_ID (src_gdisp_ID);
	}
	dont_dist = 1;

      if (src_gdisp && clone_point_set && second_mv)
	{
	  clone_now = FALSE; 
	  draw = 0;
	  draw_core_pause (paint_core->core, active_tool);
	  gdisplay_transform_coords (src_gdisp, x2, y2, &trans_tx, &trans_ty, 1);
	  gdisplay_transform_coords (gdisp, dest_x, dest_y, &dtrans_tx, &dtrans_ty, 1);
	  draw = 1;
	  draw_core_resume (paint_core->core, active_tool);
	}
      else if (!second_mv && src_gdisp)
	{
	  clone_now = FALSE; 
	  gdisplay_transform_coords (src_gdisp, x2, y2, &trans_tx, &trans_ty, 1);
	  gdisplay_transform_coords (gdisp, dest_x, dest_y, &dtrans_tx, &dtrans_ty, 1);
	  draw = 1;
	  draw_core_start (paint_core->core, src_gdisp->canvas->window, active_tool);
	  second_mv = TRUE;
	}
    }
  return NULL; 

}

static void *
clone_paint_func (PaintCore *paint_core,
		  GimpDrawable *drawable,
		  int state)
{
  GDisplay * gdisp;
  GDisplay * src_gdisp;
  int x1, y1, x2, y2, e; 
  int dist_x, dist_y, dont_draw=0;

  gdisp = (GDisplay *) active_tool->gdisp_ptr;

  if (!source_drawable || !drawable_gimage (source_drawable))
    {
      clean_up = 0;
      first_down = TRUE;
      first_up = TRUE;
      first_mv = TRUE;
      second_mv = TRUE;
      rm_cross=0;
      clone_now = TRUE;
      dont_dist = 1;
      clone_point_set = 0;
    }
  if (!first_up)
    {
      clone_now = FALSE; 
      first_up = TRUE; 
      draw = 0;
      draw_core_stop (paint_core->core, active_tool);
    }
  switch (state)
    {
    case MOTION_PAINT :
      clone_now = TRUE; 

      x1 = paint_core->curx;
      y1 = paint_core->cury;
      x2 = paint_core->lastx;
      y2 = paint_core->lasty;

      /*  If the control key is down, move the src target and return */
      if (paint_core->state & ControlMask)
	{
	  saved_x = src_x = x1;
	  saved_y = src_y = y1;
	  first_down = TRUE;
	  clone_point_set = TRUE;
	}
      /*  otherwise, update the target  */
      else if (clone_point_set)
	{
	  dest_x = x1;
	  dest_y = y1;

	  if (clone_options->aligned == AlignRegister)
	    {
	      offset_x = 0;
	      offset_y = 0;
	    }
	  else if (first_down && clone_options->aligned == AlignNo)
	    {
	      offset_x = saved_x - dest_x;
	      offset_y = saved_y - dest_y;
	      first_down = FALSE;
	    }
	  else if (first_down && clone_options->aligned == AlignYes)
	    {
	      dest2_x = dest_x;
	      dest2_y = dest_y;
	      if (gdisp->gimage->onionskin)
		{
	      offset_x += src_x - dest_x;
	      offset_y += src_y - dest_y;
		}
	      else
		{
		  offset_x = src_x - dest_x;
		  offset_y = src_y - dest_y;

		}
	      if (gdisp->gimage->onionskin)
		{
		  printf ("offset image\n"); 
		frame_manager_onionskin_offset (gdisp, offset_x, offset_y); 
		}
	      first_down = FALSE;
	      dont_draw = 1; 
	      clone_set_offset (clone_options, offset_x, offset_y); 
	    }
	  dist_x = ((dest_x - dest2_x) * cos (rotate)) -
	    ((dest_y - dest2_y) * sin (rotate));
	  dist_y = ((dest_x - dest2_x) * sin (rotate)) + 
	    ((dest_y - dest2_y) * cos (rotate));
	  src_x = (dest2_x + (dist_x) * SCALE_X) + offset_x;
	  src_y = (dest2_y + (dist_y) * SCALE_Y) + offset_y;
	 
	  
	  error_x = scale_x >= 1 ? 
	    (dest_x - dest2_x) - ((dest_x - dest2_x) / (int)scale_x) * scale_x :
	    0; 
    	    
	  error_y = scale_y >= 1 ?
	    (dest_y - dest2_y) - ((dest_y - dest2_y) / (int)scale_y) * scale_y :
	    0;

	  if (scale_x < 1 && (int)SCALE_X != SCALE_X)
	    {
	      if ((dest_x - dest2_x) - 
		  ((dest_x - dest2_x)/(int)(SCALE_X))*(int)(SCALE_X) != 0)
		{
		return NULL;
		}
	    }
	  if (scale_y < 1 && (int)SCALE_Y != SCALE_Y)
	    {
	      if ((dest_y - dest2_y) - 
		  ((dest_y - dest2_y)/(int)(SCALE_X))*(int)(SCALE_X) != 0)
		{
		 return NULL; 
		}
	    }


	  error_x = error_x < 0 ? scale_x + error_x : error_x;
	  error_y = error_y < 0 ? scale_y + error_y : error_y;


	  if (!dont_draw)
	  clone_motion (paint_core, drawable, src_x, src_y);
	}

      second_mv = FALSE;
      if (clone_point_set) 
	{
	  draw = 0;
	  draw_core_pause (paint_core->core, active_tool);
	}
      break;

    case INIT_PAINT :
      if (clone_point_set && gdisp->framemanager && 
	  source_drawable != frame_manager_bg_drawable (gdisp))
	{
	  source_drawable = frame_manager_bg_drawable (gdisp); 
	}
      if (clone_point_set && gdisp->gimage->onionskin)
	{
	 source_drawable = frame_manager_onionskin_drawable (gdisp); 
	}
      if (!clone_point_set && gdisp->framemanager  
	  && (src_gdisp_ID = frame_manager_bg_display (gdisp)))
	{
	  source_drawable = frame_manager_bg_drawable (gdisp);
	  saved_x = src_x = paint_core->curx + offset_x;
	  saved_y = src_y = paint_core->cury + offset_y;
	  clone_point_set = TRUE;
	  first_down = TRUE;
	  first_mv = TRUE;	    
	}
      if (gdisp->gimage->onionskin && !clone_point_set)
	{
	  src_gdisp_ID = frame_manager_onionskin_display (gdisp);
	  source_drawable = frame_manager_onionskin_drawable (gdisp);
	  saved_x = src_x = paint_core->curx + offset_x;
	  saved_y = src_y = paint_core->cury + offset_y;
	  clone_point_set = TRUE;
	  first_down = TRUE;
	  first_mv = TRUE;

	}
      if (paint_core->state & ControlMask)
	{
	  if (gdisp->gimage->onionskin)
	    {
	      src_gdisp_ID = frame_manager_onionskin_display (gdisp); 
	      source_drawable = frame_manager_onionskin_drawable (gdisp);
	    }
	  else
	    {
	      if (gdisp->framemanager)
		frame_manager_set_bg (gdisp); 
	      src_gdisp_ID = gdisp->ID;
	      source_drawable = drawable;
	    }
	  saved_x = src_x = paint_core->curx;
	  saved_y = src_y = paint_core->cury;
	  clone_point_set = TRUE;
	  first_down = TRUE;
	  first_mv = TRUE;
	}
      else if (clone_options->aligned == AlignNo && 
	  clone_point_set==1)
	{
	  first_down = TRUE;
	}
      else if (!clone_point_set)
	{
	g_message ("Set clone point with CTRL + left mouse button.");
	}
      first_up = TRUE;
      clone_now = TRUE; 
      break;

    case FINISH_PAINT :
      clone_now = TRUE; 
      first_up = FALSE; 
      draw = 0;
      draw_core_stop (paint_core->core, active_tool);
      return NULL;
    default :
      break;
    }

  /*  Calculate the coordinates of the target  */
  src_gdisp = gdisplay_get_ID (src_gdisp_ID);
  if (!src_gdisp)
    {
      src_gdisp_ID = gdisp->ID;
      src_gdisp = gdisplay_get_ID (src_gdisp_ID);
    }

  /*  Find the target cursor's location onscreen  */
  gdisplay_transform_coords (src_gdisp, src_x, src_y, &trans_tx, &trans_ty, 1);

  if (state == INIT_PAINT && clone_point_set)
    {
    /*  Initialize the tool drawing core  */
      draw = 1;
    draw_core_start (paint_core->core,
	src_gdisp->canvas->window,
	active_tool);
    }
  else if (state == MOTION_PAINT && clone_point_set)
    {
      draw = 1;
      draw_core_resume (paint_core->core, active_tool);
    }
  return NULL;
}

Tool *
tools_new_clone ()
{
  Tool * tool;
  PaintCore * private;

  if (! clone_options)
    clone_options = create_clone_options ();

  tool = paint_core_new (CLONE);

  private = (PaintCore *) tool->private;
  private->paint_func = clone_paint_func;
  private->cursor_func = clone_cursor_func;
  private->painthit_setup = clone_painthit_setup;
  private->core->draw_func = clone_draw;

 clone_point_set = FALSE;
 return tool;
}

void
tools_free_clone (Tool *tool)
{
  clone_point_set = FALSE;
  paint_core_free (tool);
}

static void
clone_draw (Tool *tool)
{
  GDisplay *gdisplay; 
  PaintCore * paint_core = NULL;
  paint_core = (PaintCore *) tool->private;

  
  if (tool && paint_core && clone_point_set && !expose)
    {
      static int radius;
      gdisplay = gdisplay_get_ID (drawable_gimage (source_drawable)->ID); 
      if (get_active_brush () && gdisplay && get_active_brush ()->mask && draw)
	{
	  radius = (canvas_width (get_active_brush ()->mask) > canvas_height (get_active_brush ()->mask) ?
	      canvas_width (get_active_brush ()->mask) : canvas_height (get_active_brush ()->mask))* 
	    ((double)SCALEDEST (gdisplay) / 
	     (double)SCALESRC (gdisplay));
	}
      else if (draw)
	{
	  radius = 0;
	}
      if (clone_now)
	{
	  if ((GDisplay *) tool->gdisp_ptr &&
	      !((GDisplay *) tool->gdisp_ptr)->gimage->onionskin)
	    {
	      gdk_draw_line (paint_core->core->win, paint_core->core->gc,
		  trans_tx - (TARGET_WIDTH >> 1), trans_ty,
		  trans_tx + (TARGET_WIDTH >> 1), trans_ty);
	      gdk_draw_line (paint_core->core->win, paint_core->core->gc,
		  trans_tx, trans_ty - (TARGET_HEIGHT >> 1),
		  trans_tx, trans_ty + (TARGET_HEIGHT >> 1));
	      gdk_draw_arc (paint_core->core->win, paint_core->core->gc,
		  0,
		  trans_tx - (radius+1)/2, trans_ty - (radius+1)/2,
		  radius, radius,
		  0, 23040); 
	    }		    
	}
      else
	{
	  if (!(dont_dist && ((GDisplay *) tool->gdisp_ptr)->gimage->onionskin))
	    {
	      gdk_draw_line (paint_core->core->win, paint_core->core->gc,
		  trans_tx - (TARGET_WIDTH >> 1), trans_ty - (TARGET_HEIGHT >> 1),
		  trans_tx + (TARGET_WIDTH >> 1), trans_ty + (TARGET_HEIGHT >> 1));
	      gdk_draw_line (paint_core->core->win, paint_core->core->gc,
		  trans_tx + (TARGET_WIDTH >> 1), trans_ty - (TARGET_HEIGHT >> 1),
		  trans_tx - (TARGET_WIDTH >> 1), trans_ty + (TARGET_HEIGHT >> 1));

	      gdk_draw_arc (paint_core->core->win, paint_core->core->gc,
		  0,
		  trans_tx - (radius+1)/2, trans_ty - (radius+1)/2,
		  radius, radius,
		  0, 23040);
	    }
	  if (!dont_dist && clone_options->aligned == AlignYes)
	    gdk_draw_line (paint_core->core->win, paint_core->core->gc,
		trans_tx, trans_ty,
		dtrans_tx, dtrans_ty); 
	}
    }
}

void
clone_clean_up ()
{
  if (clone_point_set && !clean_up && active_tool->state == INACTIVE
      && clone_options->aligned == AlignYes)
    {
      clone_draw (active_tool);

      clean_up = 1; 
    }
}

void 
clone_undo_clean_up ()
{
  if (clean_up && clone_point_set && active_tool->state == INACTIVE 
      && clone_options->aligned == AlignYes)
    {
      expose = 0;
      clean_up = 0;
      clone_draw (active_tool);
    }
  if (!clone_point_set)
    clean_up = 0;
  if (expose)
    {
    expose = 0; 
    clean_up = 0;
    }
}

void 
clone_expose ()
{
  if (clean_up)
  expose = 1;

}

void
clone_delete_image (GImage *gimage)
{
  if (src_gdisp_ID == gimage->ID)
    {
      clean_up = 0;
      first_down = TRUE;
      first_up = TRUE;
      first_mv = TRUE;
      second_mv = TRUE;
      rm_cross=0;
      clone_now = TRUE;
      dont_dist = 1;
      clone_point_set = FALSE;

    }
}

void
clone_flip_image ()
{

  clean_up = 1;
  expose = 1; 
}

static void 
clone_motion  (PaintCore * paint_core,
               GimpDrawable * drawable,
               int offset_x,
               int offset_y)
{
  paint_core_16_area_setup (paint_core, drawable);

  if (setup_successful)
    paint_core_16_area_paste (paint_core, drawable,
			      current_device ? paint_core->curpressure : 1.0
			      , (gfloat) gimp_brush_get_opacity (),
			      SOFT, CONSTANT,
			      gimp_brush_get_paint_mode ());
}


static void
clone_painthit_setup (PaintCore *paint_core, Canvas * painthit)
{
  
  setup_successful = FALSE;
  if (painthit)
    {
      GimpDrawable *src_drawable, *drawable;
      drawable = paint_core->drawable;
      src_drawable = source_drawable;
      if (paint_core->setup_mode == NORMAL_SETUP)
	{
	  drawable = paint_core->drawable;
	  src_drawable = source_drawable;
	}
      else if (paint_core->setup_mode == LINKED_SETUP)
	{
	  GSList * channels_list = gimage_channels (drawable_gimage (source_drawable)); 
	  drawable = paint_core->linked_drawable;
	  src_drawable = source_drawable;
	  if (channels_list)
	    {
	      Channel *channel = (Channel *)(channels_list->data);
	      if (channel)
		src_drawable = GIMP_DRAWABLE(channel);

	    }
	}
      setup_successful = clone_line_image (paint_core, painthit,
	  			drawable, src_drawable,
	  			paint_core->x + (src_x-dest_x),/*offset_x*/
	  			paint_core->y + (src_y-dest_y)/*fset_y*/); 
    }
}

static int 
clone_line_image  (PaintCore * paint_core,
                   Canvas * painthit,
                   GimpDrawable * drawable,
                   GimpDrawable * src_drawable,
                   int x,
                   int y)
{
  GImage * src_gimage = drawable_gimage (src_drawable);
  GImage * gimage = drawable_gimage (drawable);
  int rc = FALSE;
  int w, h, width, height; 

  if (gimage && src_gimage)
    {
      Matrix matrix;
      Canvas * rot=NULL; 
      Canvas * orig;
      PixelArea srcPR, destPR;
      int x1, y1, x11, y11, x2, y2, x22, y22;

      if (src_drawable != drawable) /* different windows */
	{
#if 0
	  x1 = BOUNDS (x,
	      0, drawable_width (src_drawable));
	  y1 = BOUNDS (y,
	      0, drawable_height (src_drawable));
	  x2 = BOUNDS (x + canvas_width (painthit),
	      0, drawable_width (src_drawable));
	  y2 = BOUNDS (y + canvas_height (painthit),
	      0, drawable_height (src_drawable));
#endif 
	  if (rotate)
	    {
	     width = canvas_width (painthit) * 2; 
	     height = canvas_height (painthit) * 2; 
	     error_x += canvas_width (painthit)/2;
	     error_y += canvas_height (painthit)/2;
	    }
	  else
	    {
	     width = canvas_width (painthit); 
	     height = canvas_height (painthit); 
	    }

	  if (scale_x <= 1)
	    {
	      w = (width * (SCALE_X) - 
		  canvas_width (painthit)) * 0.5;
	      h = (height * (SCALE_Y) - 
		  canvas_height (painthit)) * 0.5;
	      w = scale_x == 1 ? w: w + 1;
	      h = scale_y == 1 ? h: h + 1; 
	      x1 = BOUNDS (x - w, 0, drawable_width (src_drawable));
	      y1 = BOUNDS (y - h, 0, drawable_height (src_drawable));
	      x2 = BOUNDS (x + w + canvas_width (painthit), 0, drawable_width (src_drawable));
	      y2 = BOUNDS (y + h + canvas_height (painthit), 0, drawable_height (src_drawable));
	    }
	  else
	    {
	      w = (scale_x+1) / 2 + (width * (SCALE_X)) * 0.5;
	      h = (scale_y+1) / 2 + (height * (SCALE_Y)) * 0.5;
	      x1 = BOUNDS (x - w + canvas_width (painthit) / 2, 0, drawable_width (src_drawable));
	      y1 = BOUNDS (y - h + canvas_height (painthit) / 2, 0, drawable_height (src_drawable));
	      x2 = BOUNDS (x + w + canvas_width (painthit) / 2, 0, drawable_width (src_drawable));
	      y2 = BOUNDS (y + h + canvas_height (painthit) / 2, 0, drawable_height (src_drawable));	   
	    }
	  if (!(x2 - x1) || !(y2 - y1))
	    return FALSE;
#if 0
	  pixelarea_init (&srcPR, drawable_data (src_drawable),
	      x1, y1, (x2 - x1), (y2 - y1), FALSE);
#endif	  
	  orig = paint_core_16_area_original2 (paint_core, src_drawable, 
	      x1, y1, x2, y2);
	}
      else
	{
	  if (rotate)
	    {
	     width = canvas_width (painthit) * 2; 
	     height = canvas_height (painthit) * 2; 
	     error_x += canvas_width (painthit) / 2;
	     error_y += canvas_height (painthit) / 2;
	    }
	  else
	    {
	     width = canvas_width (painthit); 
	     height = canvas_height (painthit); 
	    }

	  if (scale_x <= 1)
	    {
	      w = (width * (SCALE_X) - 
		  canvas_width (painthit)) * 0.5;
	      h = (height * (SCALE_Y) - 
		  canvas_height (painthit)) * 0.5;
	      w = scale_x == 1 ? w : w + 1;
	      h = scale_y == 1 ? h : h + 1; 
	      x1 = BOUNDS (x - w, 0, drawable_width (drawable));
	      y1 = BOUNDS (y - h, 0, drawable_height (drawable));
	      x2 = BOUNDS (x + w + canvas_width (painthit), 0, drawable_width (drawable));
	      y2 = BOUNDS (y + h + canvas_height (painthit), 0, drawable_height (drawable));
	    }
	  else
	    {
	      w = (scale_x+1) / 2 + (width * (SCALE_X)) * 0.5;
	      h = (scale_y+1) / 2 + (height * (SCALE_Y)) * 0.5;
	      x1 = BOUNDS (x - w + canvas_width (painthit) / 2, 0, drawable_width (drawable));
	      y1 = BOUNDS (y - h + canvas_height (painthit) / 2, 0, drawable_height (drawable));
	      x2 = BOUNDS (x + w + canvas_width (painthit) / 2, 0, drawable_width (drawable));
	      y2 = BOUNDS (y + h + canvas_height (painthit) / 2, 0, drawable_height (drawable));	   
	    }
	  if (!(x2 - x1) || !(y2 - y1))
	    return FALSE;
	

	  orig = paint_core_16_area_original (paint_core, drawable, x1, y1, x2, y2);
        }


      if (scale_x == 1 && scale_y == 1 && rotate == 0)
      {
        pixelarea_init (&srcPR, orig, error_x, error_y,
            x2-x1,  y2-y1, FALSE);
      }
      else
      {
        identity_matrix (matrix);
        rotate_matrix (matrix, 2.0*M_PI-rotate);
        scale_matrix (matrix, scale_x, scale_y);
        rot = transform_core_do (src_gimage, src_drawable, 
            orig, 1, matrix);
        /*error_x +=  (canvas_width (rot) - (x2 - x1))/2;
          error_y +=  (canvas_height (rot) - (y2 - y1))/2;
         */
        pixelarea_init (&srcPR, rot, error_x, error_y,
            x2-x1,  y2-y1, FALSE);
      }     
      pixelarea_init (&destPR, painthit,
          (x1 - x), (y1 - y), x2-x1, y2-y1, TRUE);


      copy_area (&srcPR, &destPR);
      
      if (rot)
        canvas_delete (rot); 

      rc = TRUE;
    }
  return rc;
}

static gint
clone_x_offset (GtkWidget *w, gpointer client_data)
{
  extern GSList *display_list;
  GSList *list = display_list; 
  GDisplay *d;
  CloneOptions *co = (CloneOptions*) client_data;
  char *tmp;
  offset_x = atoi (gtk_entry_get_text (co->x_offset_entry));
  while (list)
    {
      if ((d=((GDisplay *) list->data))->frame_manager)
	frame_manager_onionskin_offset (d, offset_x, 0);
      list = g_slist_next (list);
    }

  return TRUE;
}

static gint	    
clone_x_offset_down (GtkWidget *w, gpointer client_data)
{
  extern GSList *display_list;
  GSList *list = display_list;
  GDisplay *d;
  char tmp[30]; 
  CloneOptions *co = (CloneOptions*) client_data;
  offset_x --;
  sprintf (tmp, "%d\0", offset_x);  
  gtk_entry_set_text (co->x_offset_entry, tmp);
  while (list)
    {
      if ((d=((GDisplay *) list->data))->frame_manager)
	frame_manager_onionskin_offset (d, offset_x, 0); 
      list = g_slist_next (list);
    }

  return TRUE; 
}

static gint	    
clone_x_offset_up (GtkWidget *w, gpointer client_data)
{
  extern GSList *display_list;
  GSList *list = display_list;
  GDisplay *d;
  char tmp[30]; 
  CloneOptions *co = (CloneOptions*) client_data;
  offset_x ++;
  sprintf (tmp, "%d\0", offset_x);  
  gtk_entry_set_text (co->x_offset_entry, tmp);
  while (list)
    {
      if ((d=((GDisplay *) list->data))->frame_manager)
	frame_manager_onionskin_offset (d, offset_x, 0); 
      list = g_slist_next (list);
    }

  return TRUE; 
}

static gint
clone_y_offset (GtkWidget *w, gpointer client_data)
{
  extern GSList *display_list;
  GSList *list = display_list;
  GDisplay *d;
  CloneOptions *co = (CloneOptions*) client_data;
  char *tmp;
  offset_y = atoi (gtk_entry_get_text (co->y_offset_entry));
  while (list)
    {
      if ((d=((GDisplay *) list->data))->frame_manager)
	frame_manager_onionskin_offset (d, 0, offset_y);
      list = g_slist_next (list);
    }

  return TRUE;
}

static gint 	    
clone_y_offset_down (GtkWidget *w, gpointer client_data) 
{
  extern GSList *display_list;
  GSList *list = display_list;
  GDisplay *d;

  char tmp[30]; 
  CloneOptions *co = (CloneOptions*) client_data;
  offset_y --;
  sprintf (tmp, "%d\0", offset_y);  
  gtk_entry_set_text (co->y_offset_entry, tmp);
  while (list)
    {
      if ((d=((GDisplay *) list->data))->frame_manager)
	frame_manager_onionskin_offset (d, 0, offset_y); 
      list = g_slist_next (list);
    }

  return TRUE; 
}

static gint 	    
clone_y_offset_up (GtkWidget *w, gpointer client_data) 
{
  extern GSList *display_list;
  GSList *list = display_list;
  GDisplay *d;
  char tmp[30]; 
  CloneOptions *co = (CloneOptions*) client_data;
  offset_y ++;
  sprintf (tmp, "%d\0", offset_y);  
  gtk_entry_set_text (co->y_offset_entry, tmp);
  while (list)
    {
      if ((d=((GDisplay *) list->data))->frame_manager)
	frame_manager_onionskin_offset (d, 0, offset_y); 
      list = g_slist_next (list);
    }

  return TRUE; 
}

static gint	    
clone_x_scale (GtkWidget *w, gpointer client_data) 
{
  
  CloneOptions *co = (CloneOptions*) client_data;
  scale_x = atof (gtk_entry_get_text (w));
  scale_x = scale_x == 0 ? 1 : scale_x;
  SCALE_X = (double) 1.0 / scale_x;   
  return TRUE; 
}

static gint	    
clone_y_scale (GtkWidget *w, gpointer client_data) 
{
  CloneOptions *co = (CloneOptions*) client_data;
  scale_y = atof (gtk_entry_get_text (w));
  scale_y = scale_y == 0 ? 1 : scale_y; 
  SCALE_Y = (double) 1.0 / scale_y;   
  return TRUE; 
}

static gint	    
clone_rotate (GtkWidget *w, gpointer client_data) 
{
  CloneOptions *co = (CloneOptions*) client_data;
  rotate = atof (gtk_entry_get_text (w)) * M_PI / 180.0;
  return TRUE; 
}

gint 
clone_is_src (GDisplay *gdisplay)
{

  if (src_gdisp_ID == gdisplay->ID)
    return 1;
  return 0; 
}

static void         
clone_set_offset (CloneOptions *co, int x, int y)
{
  char tmp[20];
  sprintf (tmp, "%d\0", x); 
  gtk_entry_set_text (co->x_offset_entry, tmp);
  sprintf (tmp, "%d\0", y); 
  gtk_entry_set_text (co->y_offset_entry, tmp);
} 

void
clone_x_offset_increase ()
{
  extern GSList *display_list;
  GSList *list = display_list; 
  GDisplay *d;
  char tmp[20];
  offset_x ++; 
  sprintf (tmp, "%d\0", offset_x);  
  if (clone_options)
    gtk_entry_set_text (clone_options->x_offset_entry, tmp);
  while (list)
    {
      if ((d=((GDisplay *) list->data))->frame_manager)
	frame_manager_onionskin_offset (d, offset_x, 0); 
      list = g_slist_next (list);
    }

}

void
clone_x_offset_decrease ()
{
  extern GSList *display_list;
  GSList *list = display_list; 
  GDisplay *d;
  char tmp[20];
  offset_x --; 
  sprintf (tmp, "%d\0", offset_x);  
  if (clone_options)
    gtk_entry_set_text (clone_options->x_offset_entry, tmp);
  while (list)
    {
      if ((d=((GDisplay *) list->data))->frame_manager)
	frame_manager_onionskin_offset (d, offset_x, 0); 
      list = g_slist_next (list);
    }


}

void
clone_y_offset_increase ()
{
  extern GSList *display_list;
  GSList *list = display_list; 
  GDisplay *d;
  char tmp[20];
  offset_y ++;
  sprintf (tmp, "%d\0", offset_y);
  if (clone_options)
    gtk_entry_set_text (clone_options->y_offset_entry, tmp);
  while (list)
    {
      if ((d=((GDisplay *) list->data))->frame_manager)
	frame_manager_onionskin_offset (d, 0, offset_y); 
      list = g_slist_next (list);
    }
}

void
clone_y_offset_decrease ()
{
  extern GSList *display_list;
  GSList *list = display_list; 
  GDisplay *d;
  char tmp[20];
  offset_y --;
  sprintf (tmp, "%d\0", offset_y);
  if (clone_options)
    gtk_entry_set_text (clone_options->y_offset_entry, tmp);
  while (list)
    {
      if ((d=((GDisplay *) list->data))->frame_manager)
	frame_manager_onionskin_offset (d, 0, offset_y); 
      list = g_slist_next (list);
    }
}

void
clone_reset_offset ()
{
  extern GSList *display_list;
  GSList *list = display_list; 
  GDisplay *d;
  offset_x = offset_y = 0; 
  if (clone_options)
    clone_set_offset (clone_options, offset_x, offset_y); 
  while (list)
    {
      if ((d=((GDisplay *) list->data))->frame_manager)
	frame_manager_onionskin_offset (d, 0, 0);  
      list = g_slist_next (list);
    }

}

int
clone_get_x_offset ()
{
  return offset_x;
}

int
clone_get_y_offset ()
{
  return offset_y;
}


static void *
clone_non_gui_paint_func (PaintCore *paint_core,
    GimpDrawable *drawable,
    int        state)
{
#if 0
  clone_type = non_gui_clone_type;
  source_drawable = non_gui_source_drawable;

  clone_motion (paint_core, drawable, non_gui_offset_x, non_gui_offset_y);
#endif
  return NULL;
}


/*  The clone procedure definition  */
ProcArg clone_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_DRAWABLE,
    "src_drawable",
    "the source drawable"
  },
  { PDB_INT32,
    "clone_type",
    "the type of clone: { IMAGE-CLONE (0), PATTERN-CLONE (1) }"
  },
  { PDB_FLOAT,
    "src_x",
    "the x coordinate in the source image"
  },
  { PDB_FLOAT,
    "src_y",
    "the y coordinate in the source image"
  },
  { PDB_INT32,
    "num_strokes",
    "number of stroke control points (count each coordinate as 2 points)"
  },
  { PDB_FLOATARRAY,
    "strokes",
    "array of stroke coordinates: {s1.x, s1.y, s2.x, s2.y, ..., sn.x, sn.y}"
  }
};


ProcRecord clone_proc =
{
  "gimp_clone",
  "Clone from the source to the dest drawable using the current brush",
  "This tool clones (copies) from the source drawable starting at the specified source coordinates to the dest drawable.  If the \"clone_type\" argument is set to PATTERN-CLONE, then the current pattern is used as the source and the \"src_drawable\" argument is ignored.  Pattern cloning assumes a tileable pattern and mods the sum of the src coordinates and subsequent stroke offsets with the width and height of the pattern.  For image cloning, if the sum of the src coordinates and subsequent stroke offsets exceeds the extents of the src drawable, then no paint is transferred.  The clone tool is capable of transforming between any image types including RGB->Indexed--although converting from any type to indexed is significantly slower.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  8,
  clone_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { clone_invoker } },
};


static Argument *
clone_invoker (Argument *args)
{
#if 0
  int success = TRUE;
  GImage *gimage;
  GimpDrawable *drawable;
  GimpDrawable *src_drawable;
  double src_x, src_y;
  int num_strokes;
  double *stroke_array;
  int i;

  drawable = NULL;
  num_strokes = 0;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  the drawable  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }
  /*  the src drawable  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      src_drawable = drawable_get_ID (int_value);
      if (src_drawable == NULL || gimage != drawable_gimage (src_drawable))
	success = FALSE;
      else
	non_gui_source_drawable = src_drawable;
    }
  /*  the clone type  */
  if (success)
    {
      int_value = args[3].value.pdb_int;
      switch (int_value)
	{
	case 0: non_gui_clone_type = ImageClone; break;
	default: success = FALSE;
	}
    }
  /*  x, y offsets  */
  if (success)
    {
      src_x = args[4].value.pdb_float;
      src_y = args[5].value.pdb_float;
    }
  /*  num strokes  */
  if (success)
    {
      int_value = args[6].value.pdb_int;
      if (int_value > 0)
	num_strokes = int_value / 2;
      else
	success = FALSE;
    }

  /*  point array  */
  if (success)
    stroke_array = (double *) args[7].value.pdb_pointer;

  if (success)
    /*  init the paint core  */
    success = paint_core_init (&non_gui_paint_core, drawable,
			       stroke_array[0], stroke_array[1]);

  if (success)
    {
      /*  set the paint core's paint func  */
      non_gui_paint_core.paint_func = clone_non_gui_paint_func;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      non_gui_offset_x = (int) (src_x - non_gui_paint_core.startx);
      non_gui_offset_y = (int) (src_y - non_gui_paint_core.starty);

      if (num_strokes == 1)
	clone_non_gui_paint_func (&non_gui_paint_core, drawable, 0);

      for (i = 1; i < num_strokes; i++)
	{
	  non_gui_paint_core.curx = stroke_array[i * 2 + 0];
	  non_gui_paint_core.cury = stroke_array[i * 2 + 1];

	  paint_core_interpolate (&non_gui_paint_core, drawable);

	  non_gui_paint_core.lastx = non_gui_paint_core.curx;
	  non_gui_paint_core.lasty = non_gui_paint_core.cury;
	}

      /*  finish the painting  */
      paint_core_finish (&non_gui_paint_core, drawable, -1);

      /*  cleanup  */
      paint_core_cleanup (&non_gui_paint_core);
    }

  return procedural_db_return_args (&clone_proc, success);
#endif
  return 0;
}


