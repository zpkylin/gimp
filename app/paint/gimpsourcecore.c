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

#define TARGET_HEIGHT  15
#define TARGET_WIDTH   15

static double clone_angle = M_PI/2.0;
static double clone_scale = 1; 
static double clone_scale_norm = 1.0; 
static int rotate = 0;

typedef enum
{
  ImageClone,
  PatternClone
} CloneType;

typedef enum
{
  AlignNo,
  AlignYes,
  AlignRegister,
  AlignRotate,
  AlignScale 
} AlignType;

/*  forward function declarations  */
static void         clone_draw            (Tool *);
static void         clone_motion          (PaintCore *, GimpDrawable *, int, int);
static int          clone_line_image    (PaintCore *, Canvas *, GimpDrawable *, GimpDrawable *, int, int);
static int          clone_line_pattern  (PaintCore *, Canvas *, GimpDrawable *, GPatternP, int, int);
static Argument *   clone_invoker         (Argument *);
static void         clone_painthit_setup (PaintCore *, Canvas *);
static void	    create_dialog (char type); 
static void	    create_offset_dialog (); 

static GimpDrawable *non_gui_source_drawable;
static GimpDrawable *source_drawable;
static int non_gui_offset_x;
static int non_gui_offset_y;
static CloneType clone_type;
static CloneType non_gui_clone_type;
static int setup_successful;

struct offset_dialog {
      GtkWidget *shell;
      GtkWidget *vbox;
      GtkWidget *x_offset;
      GtkWidget *y_offset;
};

static struct offset_dialog	*offset_dialog=NULL;

typedef struct _CloneOptions CloneOptions;
struct _CloneOptions
{
  CloneType type;
  AlignType aligned;
};

/*  local variables  */
static int 	    clone_point_set = FALSE;
static int          saved_x = 0;                /*                         */
static int          saved_y = 0;                /*  position of clone src  */
static double       src_x = 0;                /*                         */
static double       src_y = 0;                /*  position of clone src  */
static int          dest_x = 0;               /*                         */
static int          dest2_y = 0;               /*  position of clone src  */
static int          dest2_x = 0;               /*                         */
static int          dest_y = 0;               /*  position of clone src  */
static int          offset_x = 0;             /*                         */
static int          offset_y = 0;             /*  offset for cloning     */
static int          first = TRUE;
static int          trans_tx, trans_ty;       /*  transformed target  */
static int          src_gdisp_ID = -1;        /*  ID of source gdisplay  */
static CloneOptions *clone_options = NULL;
static	char	    rm_cross=0; 

static void
clone_type_callback (GtkWidget *w,
		     gpointer   client_data)
{
  clone_options->type =(CloneType) client_data;
  clone_point_set = FALSE;
}

static void
align_type_callback (GtkWidget *w,
		     gpointer   client_data)
{
  static int toggle = 1;
  static int toggle2 = 1;
  toggle *= -1;
  toggle2 *= -1; 
  clone_options->aligned =(AlignType) client_data;
  clone_point_set = FALSE;
  rotate = 0;

  if ((clone_options->aligned == AlignRotate || clone_options->aligned == AlignScale) && toggle == 1)
    {
      create_dialog ((clone_options->aligned == AlignRotate)?1:0);
      rotate = (clone_options->aligned == AlignRotate)?1:2;

    }
 
  if (clone_options->aligned == AlignYes && toggle2 == 1)
    {
      create_offset_dialog (); 
    }
  

}

static CloneOptions *
create_clone_options (void)
{
  CloneOptions *options;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *radio_frame;
  GtkWidget *radio_box;
  GtkWidget *radio_button;
  GSList *group = NULL;
  int i;
  char *button_names[2] =
  {
    "Image Source",
    "Pattern Source"
  };
  char *align_names[5] =
  {
    "Non Aligned",
    "Aligned",
    "Registered",
    "Rotate",    
    "Scale"
  };

  /*  the new options structure  */
  options = (CloneOptions *) g_malloc (sizeof (CloneOptions));
  options->type = ImageClone;
  options->aligned = AlignNo;

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);

  /*  the main label  */
  label = gtk_label_new ("Clone Tool Options");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  the radio frame and box  */
#if 0
  radio_frame = gtk_frame_new ("Source");
  gtk_box_pack_start (GTK_BOX (vbox), radio_frame, FALSE, FALSE, 0);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (radio_frame), radio_box);

  /*  the radio buttons  */
  for (i = 0; i < 2; i++)
    {
      radio_button = gtk_radio_button_new_with_label (group, button_names[i]);
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  (GtkSignalFunc) clone_type_callback,
			  (void *)((long) i));
      gtk_box_pack_start (GTK_BOX (radio_box), radio_button, FALSE, FALSE, 0);
      gtk_widget_show (radio_button);
    }
  gtk_widget_show (radio_box);
  gtk_widget_show (radio_frame);
#endif

  /*  the radio frame and box  */
  radio_frame = gtk_frame_new ("Alignment");
  gtk_box_pack_start (GTK_BOX (vbox), radio_frame, FALSE, FALSE, 0);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (radio_frame), radio_box);

  /*  the radio buttons  */
  group = NULL;  
  for (i = 0; i < 5; i++)
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
  gtk_widget_show (radio_frame);
  
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
    return;

  x1 = paint_core->curx;
  y1 = paint_core->cury;
  if (clone_point_set && !first && clone_options->aligned == AlignYes) 
    {
      x2 = x1 + offset_x;
      y2 = y1 + offset_y;

      draw_core_pause (paint_core->core, active_tool);
      src_gdisp = gdisplay_get_ID (src_gdisp_ID);
      if (src_gdisp && clone_point_set)
	{
	  gdisplay_transform_coords (src_gdisp, x2, y2, &trans_tx, &trans_ty, 1);
	  draw_core_resume (paint_core->core, active_tool);
	  rm_cross = 1; 
	}
    }


}

static void *
clone_paint_func (PaintCore *paint_core,
		  GimpDrawable *drawable,
		  int        state)
{
  GDisplay * gdisp;
  GDisplay * src_gdisp;
  int x1, y1, x2, y2;

  
  gdisp = (GDisplay *) active_tool->gdisp_ptr;

  switch (state)
    {
    case MOTION_PAINT :
#if 0
	printf("MOTION_PAINT\n");
#endif

      x1 = paint_core->curx;
      y1 = paint_core->cury;
      x2 = paint_core->lastx;
      y2 = paint_core->lasty;

      /*  If the control key is down, move the src target and return */
      if (paint_core->state & ControlMask)
	{
	  saved_x = src_x = x1;
	  saved_y = src_y = y1;
	  dest2_x = dest2_y = 0; 
	  first = TRUE;
	  clone_point_set = TRUE;
	}
      /*  otherwise, update the target  */
      else if (clone_point_set)
	{
	  dest2_x = dest_x;
	  dest2_y = dest_y; 
	  dest_x = x1;
	  dest_y = y1;

          if (clone_options->aligned == AlignRegister)
            {
	      offset_x = 0;
	      offset_y = 0;
            }
          else if (first && clone_options->aligned == AlignNo)
	    {
	      offset_x = saved_x - dest_x;
	      offset_y = saved_y - dest_y;
	      first = FALSE;
	    }
          else if (first && clone_options->aligned == AlignYes)
	    {
	  
	      char tmp[10];
	      offset_x = src_x - dest_x;
	      offset_y = src_y - dest_y;
	      first = FALSE;
	      if (offset_dialog)
		{
		  sprintf (tmp, "%d\0", offset_x);
		  gtk_entry_set_text (GTK_ENTRY (offset_dialog->x_offset), tmp);
		  sprintf (tmp, "%d\0", offset_y);
		  gtk_entry_set_text (GTK_ENTRY (offset_dialog->y_offset), tmp);
		}
	    }

	  else if (first && clone_options->aligned == AlignRotate)
	    {
	      offset_x = saved_x - dest_x;
	      offset_y = saved_y - dest_y;
	     
	      src_x = saved_x;
	      src_y = saved_y; 
	      dest2_x = dest_x;
	      dest2_y = dest_y; 
	      first = FALSE;
	      
	    }
	  else if (first && clone_options->aligned == AlignScale)
	    {
	      offset_x = saved_x - dest_x;
	      offset_y = saved_y - dest_y;
	      first = FALSE;
	      
	      src_x = saved_x;
	      src_y = saved_y;
	      dest2_x = dest_x;
	      dest2_y = dest_y; 
	    }
	  
	 
	 
	  if (clone_options->aligned == AlignRotate)
	    {
	      src_x = src_x + 
		(dest_x-dest2_x) * cos (clone_angle) - 
		(dest_y-dest2_y) * sin (clone_angle);
	      src_y = src_y +  
		(dest_x-dest2_x) * sin (clone_angle) + 
		(dest_y-dest2_y) * cos (clone_angle);
	    
	      offset_x = src_x - dest_x;
	      offset_y = src_y - dest_y; 
	    }
	  else if (clone_options->aligned == AlignScale)
	    {
	   
	      src_x = src_x + (dest_x-dest2_x) * (clone_scale_norm);
	      src_y = src_y + (dest_y-dest2_y) * (clone_scale_norm);

	      offset_x = src_x - dest_x;
	      offset_y = src_y - dest_y; 
	      
	    }
	  else  
	    {
	      src_x = dest_x + offset_x;
	      src_y = dest_y + offset_y;
	    }

	  clone_motion (paint_core, drawable, src_x, src_y);
	}

      if (clone_point_set) 
       draw_core_pause (paint_core->core, active_tool);
      break;

    case INIT_PAINT :
#if 0
	printf("INIT_PAINT called\n");
#endif
      if (paint_core->state & ControlMask)
	{
	  src_gdisp_ID = gdisp->ID;
	  source_drawable = drawable;
	  saved_x = src_x = paint_core->curx;
	  saved_y = src_y = paint_core->cury;
	  dest2_x = dest2_y = 0; 
	  clone_point_set = TRUE;
	  first = TRUE;
	}
      else if (clone_options->aligned == AlignNo && clone_point_set)
	first = TRUE;
      else if (clone_options->aligned == AlignScale && clone_point_set)
	first = TRUE;
      else if (clone_options->aligned == AlignRotate && clone_point_set)
	first = TRUE;
      else if (!clone_point_set)
	  g_message ("Set clone point with CTRL + left mouse button.");

      if (clone_options->type == PatternClone)
	if (!get_active_pattern ())
	  g_message ("No patterns available for this operation.");
      if (rm_cross)
	{
	  /*paint_core->core->draw_state = INVISIBLE;
	        clone_draw (active_tool);*/
	  draw_core_stop (paint_core->core, active_tool); 
	  rm_cross = 0;
	}
	break;

    case FINISH_PAINT :
#if 0
	printf("FINISH_PAINT called\n");	
#endif
      draw_core_stop (paint_core->core, active_tool);
      if (rm_cross)
	{
	  paint_core->core->draw_state = INVISIBLE;
	  clone_draw (active_tool);
	  /*draw_core_stop (paint_core->core, active_tool);
	  */rm_cross = 0;
	}
      return NULL;
      break;

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
    /*  Initialize the tool drawing core  */
    draw_core_start (paint_core->core,
		     src_gdisp->canvas->window,
		     active_tool);
  else if (state == MOTION_PAINT && clone_point_set)
    draw_core_resume (paint_core->core, active_tool);

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
  PaintCore * paint_core = NULL;

  paint_core = (PaintCore *) tool->private;

  if (tool && paint_core && clone_options->type == ImageClone)
    {
      gdk_draw_line (paint_core->core->win, paint_core->core->gc,
		     trans_tx - (TARGET_WIDTH >> 1), trans_ty,
		     trans_tx + (TARGET_WIDTH >> 1), trans_ty);
      gdk_draw_line (paint_core->core->win, paint_core->core->gc,
		     trans_tx, trans_ty - (TARGET_HEIGHT >> 1),
		     trans_tx, trans_ty + (TARGET_HEIGHT >> 1));
    }
}


static void 
clone_motion  (
               PaintCore * paint_core,
               GimpDrawable * drawable,
               int offset_x,
               int offset_y
               )
{
  paint_core_16_area_setup (paint_core, drawable);

  if (setup_successful)
    paint_core_16_area_paste (paint_core, drawable,
			      1.0, (gfloat) gimp_brush_get_opacity (),
			      SOFT, CONSTANT,
			      gimp_brush_get_paint_mode ());
}


static void
clone_painthit_setup (PaintCore *paint_core, Canvas * painthit)
{
  setup_successful = FALSE;
  if (painthit)
    {
      switch (clone_type)
	{
	case ImageClone:
	  {
	    GimpDrawable *src_drawable, *drawable;
	    drawable = paint_core->drawable;
	    src_drawable = source_drawable;
#if 1 
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
#endif
	    setup_successful = clone_line_image (paint_core, painthit,
		drawable, src_drawable,
		paint_core->x + offset_x,
		paint_core->y + offset_y);
	  }
	  break;
	case PatternClone:
	    {
	      GPatternP pattern;
	      if ((pattern = get_active_pattern ()))
		setup_successful = clone_line_pattern (paint_core, painthit,
		    paint_core->drawable, pattern,
		    paint_core->x + offset_x,
		    paint_core->y + offset_y);

		  break;
	    }
	}
    }

}

  static int 
clone_line_image  (
                     PaintCore * paint_core,
                     Canvas * painthit,
                     GimpDrawable * drawable,
                     GimpDrawable * src_drawable,
                     int x,
                     int y
                     )
{
  GImage * src_gimage = drawable_gimage (src_drawable);
  GImage * gimage = drawable_gimage (drawable);
  int rc = FALSE;

  if (gimage && src_gimage)
    {
      Matrix matrix;
      Canvas * rot; 
      Canvas * orig;
      PixelArea srcPR, destPR;
      int x1, y1, x11, y11, x2, y2, x22, y22;
    
      if (src_drawable != drawable)
        {
	  if (rotate == 1)
	    {
	     
	     if (x >= drawable_width (src_drawable) ||
		x < 0 ||
		y >= drawable_height (src_drawable) ||
		y < 0){
	      return FALSE;
	     }

	      x11 = BOUNDS (x - canvas_width (painthit)-1,
		  0, drawable_width (src_drawable));
	      y11 = BOUNDS (y - canvas_height (painthit)-1,
		  0, drawable_height (src_drawable));
	      x22 = BOUNDS (x + canvas_width (painthit)*2,
		  0, drawable_width (src_drawable));
	      y22 = BOUNDS (y + canvas_height (painthit)*2,
		  0, drawable_height (src_drawable));
	      if (x22 - x11 < canvas_width (painthit)*3+1 || 
		  y22 - y11 < canvas_height (painthit)*3+1)
		return FALSE;

	      orig = paint_core_16_area_original2 (paint_core, src_drawable,
		  x11, y11, x22, y22);
	      x1 = BOUNDS (x,
		  0, drawable_width (src_drawable));
	      y1 = BOUNDS (y,
		  0, drawable_height (src_drawable));
	      x2 = BOUNDS (x + canvas_width (painthit),
		  0, drawable_width (src_drawable));
	      y2 = BOUNDS (y + canvas_height (painthit),
		  0, drawable_height (src_drawable));
	      if (!(x2 - x1) || !(y2 - y1))
		return FALSE;
	    }
	  else if (rotate == 2)
	    {
	      x11 = BOUNDS (x - canvas_width (painthit) * ((int)(clone_scale_norm-.5)) /2.0,
		  0, drawable_width (drawable));
	      y11 = BOUNDS (y - canvas_height (painthit) * ((int)(clone_scale_norm-.5)) /2.0,
		  0, drawable_height (drawable));
	      x22 = BOUNDS (x + canvas_width (painthit) * (((int)(clone_scale_norm +1.5)) /2.0 +1),
		  0, drawable_width (drawable));
	      y22 = BOUNDS (y + canvas_height (painthit) * (((int)(clone_scale_norm+1.5)) /2.0 +1),
		  0, drawable_height (drawable));
	      if ((x22 - x11) <  canvas_width (painthit) * (int)clone_scale_norm ||
		  (y22 - y11) <  canvas_height (painthit) * (int)clone_scale_norm)
		return FALSE;
	      orig = paint_core_16_area_original2 (paint_core, src_drawable,
		  x11, y11, x22, y22);
		

	      x1 = BOUNDS (x,
		  0, drawable_width (src_drawable));
	      y1 = BOUNDS (y,
		  0, drawable_height (src_drawable));
	      x2 = BOUNDS (x + canvas_width (painthit),
		  0, drawable_width (src_drawable));
	      y2 = BOUNDS (y + canvas_height (painthit),
		  0, drawable_height (src_drawable));		  
	      if (!(x2 - x1) || !(y2 - y1))
		return FALSE;
	    }
	  else
	    {
          x1 = BOUNDS (x,
                       0, drawable_width (src_drawable));
          y1 = BOUNDS (y,
                       0, drawable_height (src_drawable));
          x2 = BOUNDS (x + canvas_width (painthit),
                       0, drawable_width (src_drawable));
          y2 = BOUNDS (y + canvas_height (painthit),
                       0, drawable_height (src_drawable));
          if (!(x2 - x1) || !(y2 - y1))
            return FALSE;
         
          pixelarea_init (&srcPR, drawable_data (src_drawable),
                          x1, y1, (x2 - x1), (y2 - y1), FALSE);
        
	    }
	}
      else
        {
         
	  if (rotate == 1)
	    {
	     
	     if (x >= drawable_width (drawable) ||
		x < 0 ||
		y >= drawable_height (drawable) ||
		y < 0){
	      return FALSE;
	     }

	      x11 = BOUNDS (x - canvas_width (painthit)-1,
		  0, drawable_width (drawable));
	      y11 = BOUNDS (y - canvas_height (painthit)-1,
		  0, drawable_height (drawable));
	      x2 = BOUNDS (x + canvas_width (painthit)*2,
		  0, drawable_width (drawable));
	      y2 = BOUNDS (y + canvas_height (painthit)*2,
		  0, drawable_height (drawable));
	      if (x2 - x11 < canvas_width (painthit)*3+1 || 
		  y2 - y11 < canvas_height (painthit)*3+1)
		return FALSE;

	      orig = paint_core_16_area_original (paint_core, drawable,
		  x11, y11, x2, y2);
	      x1 = BOUNDS (x,
		  0, drawable_width (drawable));
	      y1 = BOUNDS (y,
		  0, drawable_height (drawable));
	      x2 = BOUNDS (x + canvas_width (painthit),
		  0, drawable_width (drawable));
	      y2 = BOUNDS (y + canvas_height (painthit),
		  0, drawable_height (drawable));
	      if (!(x2 - x1) || !(y2 - y1))
		return FALSE;
	    }
	  else if (rotate == 2)
	    {
	     /* 
	      x11 = BOUNDS (x - canvas_width (painthit) * (int)(clone_scale_norm/2.0 - .5),
		  0, drawable_width (drawable));
	      y11 = BOUNDS (y - canvas_height (painthit) * (int)(clone_scale_norm/2.0 - .5),
		  0, drawable_height (drawable));
	      x2 = BOUNDS (x + canvas_width (painthit) * (int)(clone_scale_norm/2.0 + .5),
		  0, drawable_width (drawable));
	      y2 = BOUNDS (y + canvas_height (painthit) * (int)(clone_scale_norm/2.0 + .5),
		  0, drawable_height (drawable));
	      if ((x2 - x11) < (double) canvas_width (painthit) * clone_scale_norm || 
		  (y2 - y11) < (double) canvas_height (painthit) * clone_scale_norm)
		return FALSE;
		*/

	      x11 = BOUNDS (x - canvas_width (painthit) * ((int)(clone_scale_norm-.5)) /2.0,
		  0, drawable_width (drawable));
	      y11 = BOUNDS (y - canvas_height (painthit) * ((int)(clone_scale_norm-.5)) /2.0,
		  0, drawable_height (drawable));
	      x22 = BOUNDS (x + canvas_width (painthit) * (((int)(clone_scale_norm +1.5)) /2.0 +1),
		  0, drawable_width (drawable));
	      y22 = BOUNDS (y + canvas_height (painthit) * (((int)(clone_scale_norm+1.5)) /2.0 +1),
		  0, drawable_height (drawable));
	      if ((x22 - x11) < canvas_width (painthit) * ((int) clone_scale_norm  )||
		  (y22 - y11) < canvas_height (painthit) * ((int) clone_scale_norm))
		return FALSE;
	      orig = paint_core_16_area_original (paint_core, drawable,
		  x11, y11, x22, y22);

	      x1 = BOUNDS (x,
		  0, drawable_width (drawable));
	      y1 = BOUNDS (y,
		  0, drawable_height (drawable));
	      x2 = BOUNDS (x + canvas_width (painthit),
		  0, drawable_width (drawable));
	      y2 = BOUNDS (y + canvas_height (painthit),
		  0, drawable_height (drawable));		  
	      if (!(x2 - x1) || !(y2 - y1))
		return FALSE;
	    }
	  else
	    {
	      x1 = BOUNDS (x,
		  0, drawable_width (drawable));
	      y1 = BOUNDS (y,
		  0, drawable_height (drawable));
	      x2 = BOUNDS (x + canvas_width (painthit),
		  0, drawable_width (drawable));
	      y2 = BOUNDS (y + canvas_height (painthit),
		  0, drawable_height (drawable));
	      if (!(x2 - x1) || !(y2 - y1))
		return FALSE;
	      orig = paint_core_16_area_original (paint_core, drawable,
		  x1, y1, x2, y2);
	      pixelarea_init (&srcPR, orig,
		  0, 0, (x2 - x1), (y2 - y1), FALSE);
	    }
	}

      pixelarea_init (&destPR, painthit,
                      (x1 - x), (y1 - y), (x2 - x1), (y2 - y1), TRUE);

      /* rotate by angle */
      if (rotate == 1)
	{
	  identity_matrix (matrix); 
	  rotate_matrix (matrix, 2*M_PI-clone_angle); 
	  rot = transform_core_do (src_gimage, drawable, orig, 0, matrix); 

	  pixelarea_init (&srcPR, rot,
	      x1-x11, y1-y11, (x2 - x1), (y2 - y1), TRUE);

	}
      else if (rotate == 2)  
	{
	  identity_matrix (matrix);
	  scale_matrix (matrix, clone_scale, clone_scale);



	  rot = transform_core_do (src_gimage, src_drawable, 
	      orig, 0, matrix);

	 pixelarea_init (&srcPR, rot,
	     0, 0, (x2 - x1), (y2 - y1), FALSE);	
	}
      copy_area (&srcPR, &destPR);
      
      rc = TRUE;
    }
  return rc;
}


static int 
clone_line_pattern  (
                       PaintCore * paint_core,
                       Canvas * painthit,
                       GimpDrawable * drawable,
                       GPatternP pattern,
                       int x,
                       int y
                       )
{
  GImage * gimage;
  int rc = FALSE;
  
  gimage = drawable_gimage (drawable);
  if (gimage)
    {
      PixelArea painthit_area;
      void * pag;
      gint pattern_width = canvas_width (pattern->mask);
      gint pattern_height = canvas_height (pattern->mask);

      pixelarea_init (&painthit_area, painthit,
                      0, 0, 0, 0, TRUE);
      for (pag = pixelarea_register (1, &painthit_area);
           pag != NULL;
           pag = pixelarea_process (pag))
        {
	   PixelArea painthit_portion_area, pat_area;
	   GdkRectangle pat_rect;
	   GdkRectangle painthit_portion_rect;
	   GdkRectangle r;
	   gint x_pat, y_pat;
	   gint x_pat_start, y_pat_start;
	   gint x_portion, y_portion, w_portion, h_portion;
	   gint extra;
	   x_portion = pixelarea_x ( &painthit_area );
	   y_portion = pixelarea_y ( &painthit_area );
	   w_portion = pixelarea_width ( &painthit_area );
	   h_portion = pixelarea_height ( &painthit_area );
	   
           if (x + x_portion >= 0)
             extra = (x + x_portion) % pattern_width;
           else
	   {
	     extra = (x + x_portion) % pattern_width;
	     extra += pattern_width;
	   }
	   x_pat_start = (x + x_portion) - extra; 
           
	   if (y + y_portion >= 0)
             extra = (y + y_portion) % pattern_height;
           else
	   {
	     extra = (y + y_portion) % pattern_height;
	     extra += pattern_height;
	   }
	   y_pat_start = (y + y_portion) - extra;
	   
	   x_pat = x_pat_start;
	   y_pat = y_pat_start;
	   /* tile the portion with the pattern */ 
	   while (1)
	   {
	     pat_rect.x = x_pat;
	     pat_rect.y = y_pat;
	     pat_rect.width = pattern_width;
	     pat_rect.height = pattern_height;

	     painthit_portion_rect.x =  x + x_portion;
	     painthit_portion_rect.y =  y + y_portion;
	     painthit_portion_rect.width = w_portion;
	     painthit_portion_rect.height = h_portion;
	     
	     gdk_rectangle_intersect (&pat_rect, &painthit_portion_rect, &r);

	     pixelarea_init ( &painthit_portion_area, painthit, 
				r.x - painthit_portion_rect.x, 
				r.y - painthit_portion_rect.y, 
				r.width, r.height, TRUE);	 
	     pixelarea_init ( &pat_area, pattern->mask, 
				r.x - pat_rect.x, r.y - pat_rect.y, 
				r.width, r.height, FALSE);	 
	     
	     copy_area (&pat_area, &painthit_portion_area);
	     
             x_pat += pattern_width;
	     if (x_pat >= (x + x_portion) + w_portion)
	     {
		x_pat = x_pat_start;
		y_pat += pattern_height;
		if ( y_pat >= (y + y_portion) + h_portion )
		  break;
	     }
	   }
        }
      rc = TRUE;
    }

  return rc;
}


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
  int success = TRUE;
  GImage *gimage;
  GimpDrawable *drawable;
  GimpDrawable *src_drawable;
  double src_x, src_y;
  int num_strokes;
  double *stroke_array;
  CloneType int_value;
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
	case 1: non_gui_clone_type = PatternClone; break;
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
}

/*
   DIALOGS
*/

#include "actionarea.h"

static void dialog_ok_callback (GtkWidget *, gpointer);
static void dialog_close_callback (GtkWidget *, gpointer);

static ActionAreaItem action_items[] =
{
{ "OK", dialog_ok_callback, NULL, NULL },
{ "Close", dialog_close_callback, NULL, NULL },
};

struct _dialog {
      GtkWidget *shell;
      GtkWidget *vbox;
      GtkWidget *clone_angle_scale;
};

static struct _dialog	*dialog=NULL;

void
create_dialog (char type)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *label;  


  if (!dialog)
    {
      dialog = g_malloc (sizeof (struct _dialog));


      /*  The shell and main vbox  */
      dialog->shell = gtk_dialog_new ();
      gtk_window_set_wmclass (GTK_WINDOW (dialog->shell), "Clone Angle Scale", "Gimp");
      gtk_window_set_policy (GTK_WINDOW (dialog->shell), FALSE, FALSE, FALSE);
      if (type)
	gtk_window_set_title (GTK_WINDOW (dialog->shell), "what angle?");
      else 
	gtk_window_set_title (GTK_WINDOW (dialog->shell), "what scale?");
      vbox = gtk_vbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->shell)->vbox), vbox, TRUE, TRUE, 0);

      hbox = gtk_hbox_new (FALSE, 1); 
      gtk_container_border_width (GTK_CONTAINER (hbox), 1);
      gtk_box_set_spacing (GTK_BOX (hbox), 40); 
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);     


      label = gtk_label_new ("Value:   "); 
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);

      dialog->clone_angle_scale = gtk_entry_new ();
      gtk_widget_set_usize (GTK_WIDGET (dialog->clone_angle_scale), 50, 0);  
      if (type)
	gtk_entry_set_text (GTK_ENTRY (dialog->clone_angle_scale), "90");
      else
	gtk_entry_set_text (GTK_ENTRY (dialog->clone_angle_scale), "1.0"); 	
      gtk_box_pack_start (GTK_BOX (hbox), dialog->clone_angle_scale, FALSE, FALSE, 0);
      gtk_widget_show (label);
      gtk_widget_show (dialog->clone_angle_scale);


      /*  The action area  */
      action_items[0].user_data = dialog;
      action_items[1].user_data = dialog;
      build_action_area (GTK_DIALOG (dialog->shell), action_items, 2, 0);

      gtk_widget_show (vbox);
      gtk_widget_show (dialog->shell);

    }
  else
    {
      if (!GTK_WIDGET_VISIBLE (dialog->shell))
	{
	  char tmp[10];
	  double v;
	  if (type)
	    {
	      v = clone_angle * 180.0 / M_PI;
	      sprintf (tmp, "%f\0", v);
      
	      gtk_window_set_title (GTK_WINDOW (dialog->shell), "what angle?");
	    }
	  else
	    {
	      sprintf (tmp, "%f\0", clone_scale); 
      
	      gtk_window_set_title (GTK_WINDOW (dialog->shell), "what scale?");
	    }
	  gtk_entry_set_text (GTK_ENTRY (dialog->clone_angle_scale), tmp);
	  gtk_widget_show (dialog->shell);

	}
      else
	{
	  char tmp[10];
	  double v;
	  if (type)
	    {
	      v = clone_angle * 180.0 / M_PI; 
	      sprintf (tmp, "%f\0", v); 
      
	      gtk_window_set_title (GTK_WINDOW (dialog->shell), "what angle?");
	    }
	  else
	    {
	      sprintf (tmp, "%f\0", clone_scale);
      
	      gtk_window_set_title (GTK_WINDOW (dialog->shell), "what scale?");
	    }
	  gtk_entry_set_text (GTK_ENTRY (dialog->clone_angle_scale), tmp);
	  gdk_window_raise(dialog->shell->window);
	}
    }

}

void 

dialog_ok_callback (GtkWidget *w, gpointer client_data)
{
  struct _dialog *fm;
  double tmp;


  fm = client_data;

  if (fm)
    {

  if (rotate == 1)
    {
      sscanf (GTK_ENTRY (fm->clone_angle_scale)->text, "%lf", &tmp); 
      clone_angle = (double) tmp * M_PI / 180.0;
	
    }
  else
    {
      sscanf ((GTK_ENTRY (fm->clone_angle_scale)->text), "%lf", &clone_scale);

      clone_scale_norm = 1.0 / clone_scale; 
    }  
    }
    }

void 
dialog_close_callback (GtkWidget *w, gpointer client_data)
{
  struct _dialog *fm;
  int tmp;


  fm = client_data;

  if (fm)
    {
      if (GTK_WIDGET_VISIBLE (fm->shell))
	gtk_widget_hide (fm->shell);            
    }

  tmp = atoi (GTK_ENTRY (fm->clone_angle_scale)->text);

}


/*
   DIALOG FOR OFFSET
*/

#include "actionarea.h"

static void offset_dialog_ok_callback (GtkWidget *, gpointer);
static void offset_dialog_close_callback (GtkWidget *, gpointer);

static ActionAreaItem offset_action_items[] =
{
{ "Change Offset", offset_dialog_ok_callback, NULL, NULL },
{ "Close", 	   offset_dialog_close_callback, NULL, NULL },
};


void
create_offset_dialog ()
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *label;  


  if (!offset_dialog)
    {
      offset_dialog = g_malloc (sizeof (struct _dialog));


      /*  The shell and main vbox  */
      offset_dialog->shell = gtk_dialog_new ();
      gtk_window_set_wmclass (GTK_WINDOW (offset_dialog->shell), "Offset", "Gimp");
      gtk_window_set_policy (GTK_WINDOW (offset_dialog->shell), FALSE, FALSE, FALSE);
      gtk_window_set_title (GTK_WINDOW (offset_dialog->shell), "what offset?");
      vbox = gtk_vbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (offset_dialog->shell)->vbox), vbox, TRUE, TRUE, 0);

      hbox = gtk_hbox_new (FALSE, 1); 
      gtk_container_border_width (GTK_CONTAINER (hbox), 1);
      gtk_box_set_spacing (GTK_BOX (hbox), 40); 
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);     

      label = gtk_label_new ("x offset:   "); 
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);

      offset_dialog->x_offset = gtk_entry_new ();
      gtk_widget_set_usize (GTK_WIDGET (offset_dialog->x_offset), 50, 0);  
      gtk_entry_set_text (GTK_ENTRY (offset_dialog->x_offset), "0");
      gtk_box_pack_start (GTK_BOX (hbox), offset_dialog->x_offset, FALSE, FALSE, 0);
      gtk_widget_show (label);
      gtk_widget_show (offset_dialog->x_offset);

      hbox = gtk_hbox_new (FALSE, 1); 
      gtk_container_border_width (GTK_CONTAINER (hbox), 1);
      gtk_box_set_spacing (GTK_BOX (hbox), 40); 
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);     

      label = gtk_label_new ("y offset:   "); 
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);

      offset_dialog->y_offset = gtk_entry_new ();
      gtk_widget_set_usize (GTK_WIDGET (offset_dialog->y_offset), 50, 0);  
      gtk_entry_set_text (GTK_ENTRY (offset_dialog->y_offset), "0");
      gtk_box_pack_start (GTK_BOX (hbox), offset_dialog->y_offset, FALSE, FALSE, 0);
      gtk_widget_show (label);
      gtk_widget_show (offset_dialog->y_offset);

      /*  The action area  */
      offset_action_items[0].user_data = offset_dialog;
      offset_action_items[1].user_data = offset_dialog;
      build_action_area (GTK_DIALOG (offset_dialog->shell), offset_action_items, 2, 0);

      gtk_widget_show (vbox);
      gtk_widget_show (offset_dialog->shell);

    }
  else
    {
      if (!GTK_WIDGET_VISIBLE (offset_dialog->shell))
	{
	  char tmp[10];
	  sprintf (tmp, "%d\0", offset_x);
	  gtk_entry_set_text (GTK_ENTRY (offset_dialog->x_offset), tmp);
	  sprintf (tmp, "%d\0", offset_y);
	  gtk_entry_set_text (GTK_ENTRY (offset_dialog->y_offset), tmp);
	  gtk_widget_show (offset_dialog->shell);

	}
      else
	{
	  char tmp[10];
	  sprintf (tmp, "%d\0", offset_x);
	  gtk_entry_set_text (GTK_ENTRY (offset_dialog->x_offset), tmp);
	  sprintf (tmp, "%d\0", offset_y);
	  gtk_entry_set_text (GTK_ENTRY (offset_dialog->y_offset), tmp);
	  gdk_window_raise(offset_dialog->shell->window);
	}
    }

}

void 
offset_dialog_ok_callback (GtkWidget *w, gpointer client_data)
{
  struct offset_dialog *o;

  o = client_data; 

  if (o)
    {
      offset_x = atoi (GTK_ENTRY (o->x_offset)->text);
      offset_y = atoi (GTK_ENTRY (o->y_offset)->text);
     
      first = 0;  
    }

}

void 
offset_dialog_close_callback (GtkWidget *w, gpointer client_data)
{
  struct offset_dialog *o;
  int tmp;

  o = client_data;

  if (o)
    {
      if (GTK_WIDGET_VISIBLE (o->shell))
	gtk_widget_hide (o->shell);            
    }


}



