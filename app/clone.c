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
#include "base_frame_manager.h"

#define TARGET_HEIGHT  15
#define TARGET_WIDTH   15


typedef enum
{
  AlignYes,
  AlignNo,
  AlignRegister
} AlignType;

typedef struct _CloneOptions CloneOptions;

/*  forward function declarations  */
static void         clone_draw            (Tool *);
static void         clone_motion          (PaintCore *, GimpDrawable *, double, double);
static int          clone_line_image      (PaintCore *, Canvas *, GimpDrawable *, 
    						GimpDrawable *, double, double);
static Argument *   clone_invoker         (Argument *);
static void         clone_painthit_setup  (PaintCore *, Canvas *);
static gint	    clone_x_offset        (GtkWidget *, gpointer); 
static gint	    clone_y_offset        (GtkWidget *, gpointer); 
static gint	    clone_x_scale         (GtkWidget *, gpointer); 
static gint	    clone_y_scale         (GtkWidget *, gpointer); 
static gint	    clone_rotate          (GtkWidget *, gpointer); 
static void         clone_set_offset      (CloneOptions*, int, int); 

static GimpDrawable *source_drawable;
static int  	    setup_successful;
static char	    clean_up = 0;
static char	    inactive_clean_up = 0;

#if 0
static GimpDrawable *non_gui_source_drawable;
static int 	    non_gui_offset_x;
static int 	    non_gui_offset_y;
#endif

extern int middle_mouse_button;
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
static int 	    src_set = FALSE;
static double       saved_x = 0;                /*                         */
static double       saved_y = 0;                /*  position of clone src  */
static double       src_x = 0;                  /*                         */
static double       src_y = 0;                  /*  position of clone src  */
static double       dest_x = 0;                 /*                         */
static double       dest_y = 0;                 /*  position of clone dst  */
static double       dest2_y = 0;                /*                         */
static double       dest2_x = 0;                /*  position of clone dst  */
static int          offset_x = 0;               /*                         */
static int          offset_y = 0;               /*  offset for cloning     */
static double       off_err_x = 0;              /*                         */
static double       off_err_y = 0;              /*  offset for cloning     */
static double	    error_x = 0;		/*			   */ 
static double	    error_y = 0;		/*  error in x and y	   */
static double       scale_x = 1;                /*                         */
static double       scale_y = 1;                /*  offset for cloning     */
static double       SCALE_X = 1;                /*                         */
static double       SCALE_Y = 1;                /*  offset for cloning     */
static double       rotate = 0;                 /*  offset for cloning     */
static char         first_down = TRUE;
static char         first_mv = TRUE;
static char         second_mv = TRUE;
static double       trans_tx, trans_ty;         /*  transformed target  */
static double       dtrans_tx, dtrans_ty;         /*  transformed target  */
static int          src_gdisp_ID = -1;          /*  ID of source gdisplay  */
static CloneOptions *clone_options = NULL;
static char	    rm_cross=0; 
static char	    expose = 0;


static void
align_type_callback (GtkWidget *w,
		     gpointer  client_data)
{
  clone_options->aligned = (AlignType) client_data;
  src_set = FALSE;
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
  GtkWidget *vbox, *hbox, *box;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *radio_box;
  GtkWidget *radio_button;
  GtkWidget *entry; 
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

  /*  the radio frame and box  */
  frame = gtk_frame_new ("Alignment");
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  
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
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);
  
  box = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_widget_show (box);
  
  options->x_offset_entry = entry = gtk_spin_button_new (
      GTK_ADJUSTMENT (gtk_adjustment_new (0, -G_MAXFLOAT, G_MAXFLOAT, 1, 1, 0)), 1.0, 0);
  gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
      (GtkSignalFunc) clone_x_offset,
      options);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
      (GtkSignalFunc) clone_x_offset,
      options);
  gtk_widget_show (entry);
  
  options->y_offset_entry = entry = gtk_spin_button_new (
      GTK_ADJUSTMENT (gtk_adjustment_new (0, -G_MAXFLOAT, G_MAXFLOAT, 1, 1, 0)), 1.0, 0);
  gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
      (GtkSignalFunc) clone_y_offset,
      options);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
      (GtkSignalFunc) clone_y_offset,
      options);
  gtk_widget_show (entry);
  
  /* transformation */
  options->trans_frame = frame = gtk_frame_new ("Transformation");
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);
 
  
  box = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_widget_show (box);
  
  /* rotate */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Rotate\t"); 
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_widget_show (label);

  options->rotate_entry = entry = gtk_entry_new ();
  gtk_widget_set_usize (GTK_WIDGET (entry), 40, 0);  
  gtk_entry_set_text (GTK_ENTRY (entry), "0"); 
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
      (GtkSignalFunc) clone_rotate, options);
  gtk_widget_show (entry);
 
  /* scale */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Scale\t\t"); 
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_widget_show (label);

  options->x_scale_entry = entry = gtk_entry_new ();
  gtk_widget_set_usize (GTK_WIDGET (entry), 40, 0);  
  gtk_entry_set_text (GTK_ENTRY (entry), "1"); 
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
      (GtkSignalFunc) clone_x_scale,
      options);
  gtk_widget_show (entry);

  options->y_scale_entry = entry = gtk_entry_new ();
  gtk_widget_set_usize (GTK_WIDGET (entry), 40, 0);  
  gtk_entry_set_text (GTK_ENTRY (entry), "1"); 
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
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
  
  double x1, y1, x2, y2, dist_x, dist_y; 
  gdisp = (GDisplay *) active_tool->gdisp_ptr;

  if (middle_mouse_button || active_tool->state == ACTIVE || active_tool->state == PAUSED)
    return NULL;

  if (!gdisp || !paint_core || !drawable)
    {
      src_set = 0; 
      return NULL;
    }
  if (clean_up)
    return NULL; 
  x1 = paint_core->curx;
  y1 = paint_core->cury;

  if (active_tool->state == INACTIVE && src_set && first_down 
      && (clone_options->aligned == AlignYes || clone_options->aligned == AlignNo))
    {
      src_gdisp = gdisplay_get_ID (src_gdisp_ID);
      if (!src_gdisp)
	{
	  src_gdisp_ID = gdisp->ID;
	  src_gdisp = gdisplay_get_ID (src_gdisp_ID);
	}

      if (src_gdisp && !first_mv)
	{
	  draw_core_pause (paint_core->core, active_tool);
	  gdisplay_transform_coords_f (src_gdisp, saved_x, saved_y, &trans_tx, &trans_ty, 1);
	  gdisplay_transform_coords_f (src_gdisp, x1, y1, &dtrans_tx, &dtrans_ty, 1);
	  draw_core_resume (paint_core->core, active_tool);
	}
      if (first_mv && src_gdisp)
	{
	  gdisplay_transform_coords_f (src_gdisp, saved_x, saved_y, &trans_tx, &trans_ty, 1);
	  gdisplay_transform_coords_f (gdisp, x1, y1, &dtrans_tx, &dtrans_ty, 1);
	  draw_core_start (paint_core->core, src_gdisp->canvas->window, active_tool);
	  first_mv = FALSE;
	}
    }
  else 
  if (active_tool->state == INACTIVE && src_set && !first_down && 
      (clone_options->aligned == AlignYes || clone_options->aligned == AlignNo)) 
    {
      if (clone_options->aligned == AlignYes)
	{
	  dist_x = ((x1 - dest2_x) * cos (rotate)) -
	           ((y1 - dest2_y) * sin (rotate));
	  dist_y = ((x1 - dest2_x) * sin (rotate)) + 
	           ((y1 - dest2_y) * cos (rotate));
	  
	  x2 = dest2_x + ((dist_x) * SCALE_X) + offset_x - off_err_x + off_err_x*SCALE_X;
	  y2 = dest2_y + ((dist_y) * SCALE_Y) + offset_y - off_err_y + off_err_y*SCALE_Y;
	}
      else
	{
	  x2 = saved_x;
	  y2 = saved_y;
	}
      
      src_gdisp = gdisplay_get_ID (src_gdisp_ID);
      if (!src_gdisp)
	{
	  src_gdisp_ID = gdisp->ID;
	  src_gdisp = gdisplay_get_ID (src_gdisp_ID);
	}

      if (src_gdisp && second_mv)
	{
	  draw_core_pause (paint_core->core, active_tool);
	  gdisplay_transform_coords_f (src_gdisp, x2, y2, &trans_tx, &trans_ty, 1);
	  gdisplay_transform_coords_f (gdisp, dest_x, dest_y, &dtrans_tx, &dtrans_ty, 1);
	  draw_core_resume (paint_core->core, active_tool);
	}
      else if (!second_mv && src_gdisp)
	{
	  gdisplay_transform_coords_f (src_gdisp, x2, y2, &trans_tx, &trans_ty, 1);
	  gdisplay_transform_coords_f (gdisp, dest_x, dest_y, &dtrans_tx, &dtrans_ty, 1);
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
  double x1, y1, x2, y2; 
  double dist_x, dist_y;
  int dont_draw=0;

  gdisp = (GDisplay *) active_tool->gdisp_ptr;

  if (!source_drawable || !drawable_gimage (source_drawable))
    {
      clean_up = 0;
      first_down = TRUE;
      first_mv = TRUE;
      second_mv = TRUE;
      rm_cross=0;
      src_set = 0;
    }
  switch (state)
    {
    case MOTION_PAINT :

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
	  src_set = TRUE;
	}
      /*  otherwise, update the target  */
      else if (src_set)
	{
	  dest_x = x1;
	  dest_y = y1;

	  if (clone_options->aligned == AlignRegister)
	    {
	      offset_x = 0;
	      offset_y = 0;
	      off_err_x = 0;
	      off_err_y = 0;
	    }
	  else if (first_down && clone_options->aligned == AlignNo)
	    {
	      offset_x = saved_x - dest_x;
	      offset_y = saved_y - dest_y;
	      off_err_x = dest_x - (int)dest_x;
	      off_err_y = dest_y - (int)dest_y;
	      dont_draw = 1; 
	      first_down = FALSE;
	    }
	  else if (first_down && clone_options->aligned == AlignYes)
	    {
	      dest2_x = dest_x;
	      dest2_y = dest_y;
	      if (bfm_onionskin (gdisp))
		{
		  offset_x += saved_x - dest_x;
		  offset_y += saved_y - dest_y;
		  bfm_onionskin_set_offset (gdisp, offset_x, offset_y); 
		}
	      else
		{
		  offset_x = saved_x - dest_x;
		  offset_y = saved_y - dest_y;
		  off_err_x = dest_x - (int)dest_x;
		  off_err_y = dest_y - (int)dest_y;

		}
	      clone_set_offset (clone_options, offset_x, offset_y); 
	      dont_draw = 1; 
	      first_down = FALSE;
	    }

	  dist_x = ((dest_x - dest2_x) * cos (rotate)) -
	           ((dest_y - dest2_y) * sin (rotate));
	  dist_y = ((dest_x - dest2_x) * sin (rotate)) + 
	           ((dest_y - dest2_y) * cos (rotate));
	 
	  src_x = ((dest2_x + (dist_x) * SCALE_X) + offset_x) - off_err_x + off_err_x*SCALE_X;
	  src_y = ((dest2_y + (dist_y) * SCALE_Y) + offset_y) - off_err_y + off_err_y*SCALE_Y;


	  if (scale_x >= 1)
	    {
	      double tmp;
	      tmp = ((int)dest_x - (int)dest2_x);
	      tmp *= SCALE_X;
	      error_x = scale_x >= 1 ?  (tmp-(int)tmp) * scale_x: 0; 
	    }
	  if (scale_y >= 1)
	    {
	      double tmp;
	      tmp = ((int)dest_y - (int)dest2_y);
	      tmp *= SCALE_Y;
	      error_y = scale_y >= 1 ?  (tmp-(int)tmp) * scale_y: 0;
	    }
	  if (scale_x < 1)
	    {
	      double tmp;
	      tmp = (dest_x - (int)dest_x);
	      tmp *= SCALE_X;
	      error_x = scale_x < 1 ?  (int)tmp: 0; 
	      error_x = error_x < 0 ? scale_x + error_x : error_x;
	    }
	  if (scale_y < 1)
	    {
	      double tmp;
	      tmp = (dest_y - (int)dest_y);
	      tmp *= SCALE_Y;
	      error_y = scale_y < 1 ?  (int)tmp: 0;
	      error_y = error_y < 0 ? scale_y + error_y : error_y;
	    }



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

	  if (!dont_draw)
	    clone_motion (paint_core, drawable, src_x, src_y);
	}

      second_mv = FALSE;
      if (src_set) 
	{
	  draw_core_pause (paint_core->core, active_tool);
	}
      break;

    case INIT_PAINT :

      if (bfm (gdisp))
	{
	  if (paint_core->state & ControlMask)
	    {
	      /* set location */
	      src_gdisp_ID = bfm_get_bg_display (gdisp); 
	      source_drawable = bfm_get_bg_drawable (gdisp);
	      saved_x = src_x = paint_core->curx;
	      saved_y = src_y = paint_core->cury;
	      src_set = TRUE;
	      first_down = TRUE;
	      first_mv = TRUE;
	      clean_up = 0;
	    }
	  else
	    if (bfm_get_bg (gdisp))
	      {
		/* get src from bfm */
		src_gdisp_ID = bfm_get_bg_display (gdisp);
		source_drawable = bfm_get_bg_drawable (gdisp);
	    }
	  else
	    {
	      /* no dest is set */
	      g_message ("Set clone point with CTRL + left mouse button.");
	      break;
	    }
	}
      else if (paint_core->state & ControlMask)
	{
	  if (!first_down)
	    draw_core_pause (paint_core->core, active_tool); 
	      
	  src_gdisp_ID = gdisp->ID;
	  source_drawable = drawable;
	  saved_x = src_x = paint_core->curx;
	  saved_y = src_y = paint_core->cury;
	  src_set = TRUE;
	  first_down = TRUE;
	  first_mv = TRUE;
	  clean_up = 0; 
	}
      else if (!src_set)
	{
	  g_message ("Set clone point with CTRL + left mouse button.");
	  break;
	}
      else
	{
	  if (clone_options->aligned == AlignYes)
	 inactive_clean_up = 1;  
	  if (!first_down && clone_options->aligned == AlignYes)
	    {
	      if (offset_x + paint_core->curx == src_x &&
		  offset_y + paint_core->cury == src_y)
		inactive_clean_up = 0;
		inactive_clean_up = 0;
	  src_x = offset_x + paint_core->curx; 
	  src_y = offset_y + paint_core->cury; 
	    }
	  else 
	    if (clone_options->aligned == AlignNo)
	      {
		src_x = saved_x;
		src_y = saved_y;
		first_down = TRUE;
	      }
	}
      break;

    case FINISH_PAINT :
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
  gdisplay_transform_coords_f (src_gdisp, src_x, src_y, &trans_tx, &trans_ty, 1);

  if (state == INIT_PAINT && src_set)
    {
      /*  Initialize the tool drawing core  */
      if (inactive_clean_up)
	{
	  gdisplay_transform_coords_f (src_gdisp, paint_core->curx, paint_core->cury, 
	      &dtrans_tx, &dtrans_ty, 1);
	  
	  draw_core_pause (paint_core->core, active_tool); 
	  inactive_clean_up = 0; 
     
	 /* 
	  draw_core_resume (paint_core->core, active_tool);
	  draw_core_stop (paint_core->core, active_tool);
	*/
	  }

      draw_core_start (paint_core->core,
	  src_gdisp->canvas->window,
	  active_tool);
  }
  else if (state == MOTION_PAINT && src_set)
    {
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

 src_set = FALSE;
 return tool;
}

void
tools_free_clone (Tool *tool)
{
  src_set = FALSE;
  paint_core_free (tool);
}

void 
clone_leave_notify ()
{
  
  draw_core_pause (((PaintCore *) active_tool->private)->core, active_tool); 
}

void 
clone_enter_notify ()
{
  draw_core_resume (((PaintCore *) active_tool->private)->core, active_tool); 
}

static void
clone_draw (Tool *tool)
{
  static int first=1;
  GDisplay *gdisplay; 
  PaintCore * paint_core = NULL;
  paint_core = (PaintCore *) tool->private;

  if (expose)
    {
      expose = 0;
      return;
    }
  if (!gdisplay_active ())
    return; 

  if (middle_mouse_button)
    {
      if (!first)
	return;
      if (first)
	first = 0;
    }
  else
    {
      if (!first)
	{
	  first = 1;
	  return;
	}
    }
  if (tool && paint_core && src_set && drawable_gimage (source_drawable))
    {
      static int radius;
      gdisplay = gdisplay_get_ID (drawable_gimage (source_drawable)->ID); 
      if (!first_down && bfm_onionskin (gdisplay))
	return ;
      if (get_active_brush () && gdisplay && get_active_brush ()->mask)
	{
	  radius = (canvas_width (get_active_brush ()->mask) > canvas_height (get_active_brush ()->mask) ?
	      canvas_width (get_active_brush ()->mask) : canvas_height (get_active_brush ()->mask))* 
	    ((double)SCALEDEST (gdisplay) / 
	     (double)SCALESRC (gdisplay));
	}
      else 
	{
	  radius = 0;
	}
      if (1)/*active_tool->state != INACTIVE && active_tool->state != INACT_PAUSED  && !inactive_clean_up)
	*/
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
	  if (((active_tool->state == INACTIVE || active_tool->state == INACT_PAUSED )
	    && first_down && clone_options->aligned == AlignYes) ||
	    (inactive_clean_up && first_down && clone_options->aligned == AlignYes))
	    {
	    gdk_draw_line (paint_core->core->win, paint_core->core->gc,
		trans_tx, trans_ty,
		dtrans_tx, dtrans_ty); 
	}
	}
      else
	{
	  inactive_clean_up = 0;
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
	  
	  if (first_down && clone_options->aligned == AlignYes)
	    gdk_draw_line (paint_core->core->win, paint_core->core->gc,
		trans_tx, trans_ty,
		dtrans_tx, dtrans_ty); 
	}
    }
}

void 
clone_expose ()
{
  if (clone_options->aligned == AlignYes || clone_options->aligned == AlignNo)
    expose = 1;

}

void
clone_delete_image (GImage *gimage)
{
  if (src_gdisp_ID == gimage->ID)
    {
      clean_up = 0;
      first_down = TRUE;
      first_mv = TRUE;
      second_mv = TRUE;
      rm_cross=0;
      src_set = FALSE;

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
               double offset_x,
               double offset_y)
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
	  src_drawable = paint_core->linked_drawable; /*source_drawable;*/ 
	  if (channels_list)
	    {
	      Channel *channel = (Channel *)(channels_list->data);
	      if (channel)
		{
		  src_drawable = GIMP_DRAWABLE(channel);
		}
	      else
		printf ("PROBLEM 2\n"); 
	      if (src_drawable == source_drawable)
		printf ("ERROR1\n"); 
	      if (drawable == source_drawable)
		printf ("ERROR\n"); 
	    }
	  else
	    printf ("PROBLEM 3\n"); 

	}
      else 
	printf ("PROBLEM\n"); 
      setup_successful = clone_line_image (paint_core, painthit,
	  drawable, src_drawable,
	  paint_core->curx + ((int)src_x-dest_x),/*offset_x*/
	  paint_core->cury + ((int)src_y-dest_y)/*fset_y*/); 
    }
}

static int 
clone_line_image  (PaintCore * paint_core,
                   Canvas * painthit,
                   GimpDrawable * drawable,
                   GimpDrawable * src_drawable,
                   double x,
                   double y)
{
  GImage * src_gimage = drawable_gimage (src_drawable);
  GImage * gimage = drawable_gimage (drawable);
  int rc = FALSE;
  int w, h, width, height, xx=0, yy=0; 

  if (gimage && src_gimage)
    {
      Matrix matrix;
      Canvas * rot=NULL; 
      Canvas * orig;
      PixelArea srcPR, destPR;
      double x1, y1, x2, y2;

      if (src_drawable != drawable) /* different windows */
	{
	  if (rotate)
	    {
	     width = canvas_width (painthit) * 2.0; 
	     height = canvas_height (painthit) * 2.0; 
	    }
	  else
	    {
	     width = canvas_width (painthit); 
	     height = canvas_height (painthit); 
	    }
          
	  if (scale_x >= 1)
	    {
	      w = width;
	      h = height;
	      xx = x - w;
	      yy = y - h;
	      x1 = BOUNDS (xx, 0, drawable_width (src_drawable));
	      y1 = BOUNDS (yy, 0, drawable_height (src_drawable));
	      x2 = BOUNDS (x + w, 0, drawable_width (src_drawable));
	      y2 = BOUNDS (y + h, 0, drawable_height (src_drawable));
	      error_x = error_x < 0 ? (scale_x + error_x)  + w + (int)(w * SCALE_X + .5): 
		error_x + w + (int)(w * SCALE_X + .5); 
	      error_y = error_y < 0 ? (scale_y + error_y)  + h + (int)(h * SCALE_Y +.5): 
		error_y + h + (int)(h * SCALE_Y + .5);
	      error_x = scale_x == 1 ? w/2.0 + .5 : error_x; 
	      error_y = scale_y == 1 ? h/2.0 + .5 : error_y;
	      xx = x1 == 0 ? -xx : 0; 
	      yy = y1 == 0 ? -yy : 0; 
	    }
	  else
	    {
	      w = (width * SCALE_X) * 0.5;
	      h = (height * SCALE_Y) * 0.5;
	      xx = x - w - error_x;
	      yy = y - h - error_y;
	      x1 = BOUNDS (xx, 0, drawable_width (src_drawable));
	      y1 = BOUNDS (yy, 0, drawable_height (src_drawable));
	      x2 = BOUNDS (x + w - error_x, 0, drawable_width (src_drawable));
	      y2 = BOUNDS (y + h - error_y, 0, drawable_height (src_drawable));	   
	    
	      error_x = error_y = 0; 
	      xx = x1 == 0 ? -xx : 0; 
	      yy = y1 == 0 ? -yy : 0; 
	    }
	  if (!(x2 - x1) || !(y2 - y1))
	    return FALSE;
	  
	  orig = paint_core_16_area_original2 (paint_core, src_drawable, x1, y1, x2, y2);

	}
      else
	{
	  if (rotate)
	    {
	     width = canvas_width (painthit) * 2.0; 
	     height = canvas_height (painthit) * 2.0; 
	     /*error_x += canvas_width (painthit) / 2.0 + .5;
	     error_y += canvas_height (painthit) / 2.0 + .5;
	    */}
	  else
	    {
	     width = canvas_width (painthit); 
	     height = canvas_height (painthit); 
	    }
          
	  if (scale_x >= 1)
	    {
	      w = width;
	      h = height;
	      xx = x - w;
	      yy = y - h;
	      x1 = BOUNDS (xx, 0, drawable_width (src_drawable));
	      y1 = BOUNDS (yy, 0, drawable_height (src_drawable));
	      x2 = BOUNDS (x + w, 0, drawable_width (src_drawable));
	      y2 = BOUNDS (y + h, 0, drawable_height (src_drawable));
	      error_x = error_x < 0 ? (scale_x + error_x)  + w + (int)(w * SCALE_X + .5): 
		error_x + w + (int)(w * SCALE_X + .5); 
	      error_y = error_y < 0 ? (scale_y + error_y)  + h + (int)(h * SCALE_Y +.5): 
		error_y + h + (int)(h * SCALE_Y + .5);
	      error_x = scale_x == 1 ? w/2.0 + .5 : error_x; 
	      error_y = scale_y == 1 ? h/2.0 + .5 : error_y; 
	      xx = x1 == 0 ? -xx : 0; 
	      yy = y1 == 0 ? -yy : 0; 
	    }
	  else
	    {
	      w = (width * SCALE_X) * 0.5;
	      h = (height * SCALE_Y) * 0.5;
	      xx = x - w - error_x;
	      yy = y - h - error_y;
	      x1 = BOUNDS (xx, 0, drawable_width (src_drawable));
	      y1 = BOUNDS (yy, 0, drawable_height (src_drawable));
	      x2 = BOUNDS (x + w - error_x, 0, drawable_width (src_drawable));
	      y2 = BOUNDS (y + h - error_y, 0, drawable_height (src_drawable));	   
	    
	      error_x = error_y = 0; 
	      xx = x1 == 0 ? -xx : 0; 
	      yy = y1 == 0 ? -yy : 0; 
	    }
	  if (!(x2 - x1) || !(y2 - y1))
	    return FALSE;
	  
	  orig = paint_core_16_area_original2 (paint_core, src_drawable, x1, y1, x2, y2);

	}


      if (scale_x == 1 && scale_y == 1 && rotate == 0)
      {
        pixelarea_init (&srcPR, orig, error_x, error_y,
            x2-x1,  y2-y1, FALSE);
      }
      else
      {
        identity_matrix (matrix);
        rotate_matrix (matrix, -rotate);
        scale_matrix (matrix, scale_x, scale_y);
        rot = transform_core_do (src_gimage, src_drawable, 
            orig, 0, matrix);
        error_x = rotate ?  (canvas_width (rot) - canvas_width (painthit)) / 2.0 + 1: error_x;
        error_y = rotate ?  (canvas_height (rot) - canvas_height (painthit)) / 2.0 : error_y;
       
        pixelarea_init (&srcPR, rot, error_x, error_y,
            width,  height, FALSE);
      }     

      pixelarea_init (&destPR, painthit,
          xx, yy, x2-x1, y2-y1, TRUE);
      
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
  offset_x = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(co->x_offset_entry));
  while (list)
    {
      if ((d=((GDisplay *) list->data))->bfm)
	bfm_onionskin_set_offset (d, offset_x, offset_y);
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
  offset_y = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(co->y_offset_entry));
  while (list)
    {
      if ((d=((GDisplay *) list->data))->bfm)
	bfm_onionskin_set_offset (d, offset_x, offset_y);
      list = g_slist_next (list);
    }

  return TRUE;
}

static gint	    
clone_x_scale (GtkWidget *w, gpointer client_data) 
{
  scale_x = atof (gtk_entry_get_text(GTK_ENTRY(w)));
  scale_x = scale_x == 0 ? 1 : scale_x;
  SCALE_X = (double) 1.0 / scale_x;   
  return TRUE; 
}

static gint	    
clone_y_scale (GtkWidget *w, gpointer client_data) 
{
  scale_y = atof (gtk_entry_get_text (GTK_ENTRY(w)));
  scale_y = scale_y == 0 ? 1 : scale_y; 
  SCALE_Y = (double) 1.0 / scale_y;   
  return TRUE; 
}

static gint	    
clone_rotate (GtkWidget *w, gpointer client_data) 
{
  rotate = (atof (gtk_entry_get_text (GTK_ENTRY(w))) / 180.0)  * M_PI;
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
  gtk_spin_button_set_value (GTK_SPIN_BUTTON(co->x_offset_entry), x);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON(co->y_offset_entry), y);
} 

void
clone_x_offset_increase ()
{
  extern GSList *display_list;
  GSList *list = display_list; 
  GDisplay *d;
  offset_x ++; 
  if (clone_options)
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(clone_options->x_offset_entry), offset_x);
  while (list)
    {
      if ((d=((GDisplay *) list->data))->bfm)
	bfm_onionskin_set_offset (d, offset_x, offset_y); 
      list = g_slist_next (list);
    }

}

void
clone_x_offset_decrease ()
{
  extern GSList *display_list;
  GSList *list = display_list; 
  GDisplay *d;
  offset_x --; 
  if (clone_options)
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(clone_options->x_offset_entry), offset_x);
  while (list)
    {
      if ((d=((GDisplay *) list->data))->bfm)
	bfm_onionskin_set_offset (d, offset_x, offset_y); 
      list = g_slist_next (list);
    }


}

void
clone_y_offset_increase ()
{
  extern GSList *display_list;
  GSList *list = display_list; 
  GDisplay *d;
  offset_y ++;
  if (clone_options)
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(clone_options->y_offset_entry), offset_y);
  while (list)
    {
      if ((d=((GDisplay *) list->data))->bfm)
	bfm_onionskin_set_offset (d, offset_x, offset_y); 
      list = g_slist_next (list);
    }
}

void
clone_y_offset_decrease ()
{
  extern GSList *display_list;
  GSList *list = display_list; 
  GDisplay *d;
  offset_y --;
  if (clone_options)
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(clone_options->y_offset_entry), offset_y);
  while (list)
    {
      if ((d=((GDisplay *) list->data))->bfm)
	bfm_onionskin_set_offset (d, offset_x, offset_y); 
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
      if ((d=((GDisplay *) list->data))->bfm)
	bfm_onionskin_set_offset (d, 0, 0);  
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

#if 0
static void *
clone_non_gui_paint_func (PaintCore *paint_core,
    GimpDrawable *drawable,
    int        state)
{
  clone_type = non_gui_clone_type;
  source_drawable = non_gui_source_drawable;

  clone_motion (paint_core, drawable, non_gui_offset_x, non_gui_offset_y);
  return NULL;
}
#endif


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


