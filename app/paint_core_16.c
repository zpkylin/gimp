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
#include "draw_core.h"
#include "drawable.h"
#include "errors.h"
#include "float16.h"
#include "gdisplay.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "layer.h"
#include "noise.h"
#include "paint_core_16.h"
#include "paint_funcs_area.h"
#include "pixelarea.h"
#include "pixelrow.h"
#include "tools.h"
#include "trace.h"
#include "undo.h"
#define    SQR(x) ((x) * (x))


/*  global variables--for use in the various paint tools  */
PaintCore16  non_gui_paint_core_16;

/*  local function prototypes  */
static void      paint_core_16_button_press    (Tool *, GdkEventButton *, gpointer);
static void      paint_core_16_button_release  (Tool *, GdkEventButton *, gpointer);
static void      paint_core_16_motion          (Tool *, GdkEventMotion *, gpointer);
static void      paint_core_16_cursor_update   (Tool *, GdkEventMotion *, gpointer);
static void      paint_core_16_control         (Tool *, int, gpointer);
static void      paint_core_16_no_draw         (Tool *);


static void      painthit_init                 (PaintCore16 *, GimpDrawable *, Canvas *);

static void
brush_to_canvas_tiles(
                          PaintCore16 * paint_core,
                          Canvas * brush_mask,
                          gfloat brush_opacity 
			);

static void
canvas_tiles_to_brush(
                          PaintCore16 * paint_core,
                          Canvas * brush_mask,
                          gfloat brush_opacity 
			);

static void
canvas_tiles_to_canvas_buf(
                          PaintCore16 * paint_core);
static void
brush_to_canvas_buf(
                             PaintCore16 * paint_core,
                             Canvas * brush_mask,
                             gfloat brush_opacity
                             );

static void      painthit_finish               (GimpDrawable *, PaintCore16 *,
                                                Canvas *);

static int       paint_core_16_init_linked  (PaintCore16 *, 
			GimpDrawable *, GimpDrawable *, double, double);

static Canvas *  brush_mask_get                (PaintCore16 *, Canvas *, int);
#ifdef BRUSH_WITH_BORDER 
static Canvas *  brush_mask_subsample          (Canvas *, double, double);
#endif
static Canvas *  brush_mask_solidify           (PaintCore16 *, Canvas *);
static Canvas *  brush_mask_noise  	       (PaintCore16 *, Canvas *);

static void brush_solidify_mask_u8 ( Canvas *, Canvas *);
static void brush_solidify_mask_u16 ( Canvas *, Canvas *);
static void brush_solidify_mask_float ( Canvas *, Canvas *);
static void brush_solidify_mask_float16 ( Canvas *, Canvas *);

static void brush_mask_noise_u8 (PaintCore16 *, Canvas *, Canvas *);
static void brush_mask_noise_u16 (PaintCore16 *, Canvas *, Canvas *);
static void brush_mask_noise_float (PaintCore16 *, Canvas *, Canvas *);
static void brush_mask_noise_float16 (PaintCore16 *, Canvas *, Canvas *);
/* ------------------------------------------------------------------------

   PaintCore16 Frontend

*/
Tool * 
paint_core_16_new  (
                    int type
                    )
{
  Tool * tool;
  PaintCore16 * private;

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (PaintCore16 *) g_malloc (sizeof (PaintCore16));

  private->core = draw_core_new (paint_core_16_no_draw);

  tool->type = type;
  tool->state = INACTIVE;
  tool->scroll_lock = 0;  /*  Allow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->gdisp_ptr = NULL;
  tool->private = (void *) private;
  tool->preserve = TRUE;
  
  tool->button_press_func = paint_core_16_button_press;
  tool->button_release_func = paint_core_16_button_release;
  tool->motion_func = paint_core_16_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = paint_core_16_cursor_update;
  tool->control_func = paint_core_16_control;

  private->noise_mode = 1.0;
  private->undo_tiles = NULL;
  private->canvas_tiles = NULL;
  private->canvas_buf = NULL;
  private->orig_buf = NULL;
  private->canvas_buf_width = 0;
  private->canvas_buf_height = 0;
  private->noise_mask = NULL;
  private->solid_mask = NULL;

  private->linked_undo_tiles = NULL;
  private->linked_canvas_buf = NULL;
  private->painthit_setup = NULL;
  private->linked_drawable = NULL;
  private->drawable = NULL;

  return tool;
}


void 
paint_core_16_free  (
                     Tool * tool
                     )
{
  PaintCore16 * paint_core;

  paint_core = (PaintCore16 *) tool->private;

  /*  Make sure the selection core is not visible  */
  if (tool->state == ACTIVE && paint_core->core)
    draw_core_stop (paint_core->core, tool);

  /*  Free the selection core  */
  if (paint_core->core)
    draw_core_free (paint_core->core);

  /*  Cleanup memory  */
  paint_core_16_cleanup (paint_core);

  /*  Free the paint core  */
  g_free (paint_core);
}

static int 
paint_core_16_init_linked  (
                     PaintCore16 * paint_core,
                     GimpDrawable *drawable,
                     GimpDrawable *linked_drawable,
                     double x,
                     double y
                     )
{
  GimpBrushP brush;
  
  paint_core->drawable = drawable; 
  paint_core->linked_drawable = linked_drawable; 
  paint_core->curx = x;
  paint_core->cury = y;

  /* get the brush mask */
  if (!(brush = get_active_brush ()))
    {
      g_message ("No brushes available for use with this tool.");
      return FALSE;
    }
 
  paint_core->spacing =
    (double) MAX (canvas_height (brush->mask), canvas_width (brush->mask)) *
    ((double) gimp_brush_get_spacing () / 100.0);

  if (paint_core->spacing < 1.0)
    paint_core->spacing = 1.0;
  
  paint_core->brush_mask = brush->mask;
  paint_core->noise_mask = NULL;
  paint_core->solid_mask = NULL;
  paint_core->subsampled_mask = NULL;

  /*  Allocate the undo structure  */

  if (paint_core->undo_tiles)
    canvas_delete (paint_core->undo_tiles);
  paint_core->undo_tiles = canvas_new (drawable_tag (drawable),
                           drawable_width (drawable),
                           drawable_height (drawable),
                           STORAGE_TILED);
  canvas_set_autoalloc (paint_core->undo_tiles, AUTOALLOC_OFF);

/* For linked painting */ 

  if (linked_drawable)
    {
      if (paint_core->linked_undo_tiles)
	canvas_delete (paint_core->linked_undo_tiles);
      paint_core->linked_undo_tiles = canvas_new (drawable_tag (linked_drawable),
			       drawable_width (linked_drawable),
			       drawable_height (linked_drawable),
			       STORAGE_TILED);
      canvas_set_autoalloc (paint_core->linked_undo_tiles, AUTOALLOC_OFF);
    }

  /*  Allocate the cumulative brush stroke mask --only need one of these  */

  if (paint_core->canvas_tiles)
    canvas_delete (paint_core->canvas_tiles);
  paint_core->canvas_tiles = canvas_new (tag_new (tag_precision (drawable_tag (drawable)),
                                      FORMAT_GRAY,
                                      ALPHA_NO),
                             drawable_width (drawable),
                             drawable_height (drawable),
                             STORAGE_TILED);

  /*  Get the initial undo extents  */
  paint_core->x1 = paint_core->x2 = paint_core->curx;
  paint_core->y1 = paint_core->y2 = paint_core->cury;

  paint_core->distance = 0.0;

  return TRUE;
}


int 
paint_core_16_init  (
                     PaintCore16 * paint_core,
                     GimpDrawable *drawable,
                     double x,
                     double y
                     )
{
  return paint_core_16_init_linked (paint_core, drawable, NULL, x, y);
}

void 
paint_core_16_interpolate  (
                            PaintCore16 * paint_core,
                            GimpDrawable *drawable
                            )
{
#define    EPSILON  0.00001
  int n;
  double dx, dy;
  double left;
  double t;
  double initial;
  double dist;
  double total;

  /* see how far we've moved */
  dx = paint_core->curx - paint_core->lastx;
  dy = paint_core->cury - paint_core->lasty;

  /* bail if we haven't moved */
  if (!dx && !dy)
    return;

  dist = sqrt (SQR (dx) + SQR (dy));
  total = dist + paint_core->distance;
  initial = paint_core->distance;

  while (paint_core->distance < total)
    {
      n = (int) (paint_core->distance / paint_core->spacing + 1.0 + EPSILON);
      left = n * paint_core->spacing - paint_core->distance;
      paint_core->distance += left;

      if (paint_core->distance <= total)
	{
	  t = (paint_core->distance - initial) / dist;

	  paint_core->curx = paint_core->lastx + dx * t;
	  paint_core->cury = paint_core->lasty + dy * t;
	  (* paint_core->paint_func) (paint_core, drawable, MOTION_PAINT);
	}
    }

  paint_core->distance = total;
  paint_core->curx = paint_core->lastx + dx;
  paint_core->cury = paint_core->lasty + dy;
}


void 
paint_core_16_finish  (
                       PaintCore16 * paint_core,
                       GimpDrawable *drawable,
                       int tool_id
                       )
{
  GImage *gimage;
  PaintUndo *pu;

  if (! (gimage = drawable_gimage (drawable)))
    return;

  /*  Determine if any part of the image has been altered--
   *  if nothing has, then just return...
   */
  if ((paint_core->x2 == paint_core->x1) || (paint_core->y2 == paint_core->y1))
    return;

  undo_push_group_start (gimage, PAINT_CORE_UNDO);

  pu = (PaintUndo *) g_malloc (sizeof (PaintUndo));
  pu->tool_ID = tool_id;
  pu->lastx = paint_core->startx;
  pu->lasty = paint_core->starty;

  /*  Push a paint undo  */
  undo_push_paint (gimage, pu);

  /*  push an undo  */
  {
    /* assign ownership of undo_tiles to undo system */
    drawable_apply_image (drawable,
                          paint_core->x1, paint_core->y1,
                          paint_core->x2, paint_core->y2,
                          paint_core->undo_tiles);
     
    paint_core->undo_tiles = NULL;
  }

  /* push a linked undo if there is one */
  if (paint_core->linked_drawable)
  {
    pu = (PaintUndo *) g_malloc (sizeof (PaintUndo));
    pu->tool_ID = tool_id;
    pu->lastx = paint_core->startx;
    pu->lasty = paint_core->starty;
    undo_push_paint (gimage, pu);
    drawable_apply_image (paint_core->linked_drawable,
			  paint_core->x1, paint_core->y1,
			  paint_core->x2, paint_core->y2,
			  paint_core->linked_undo_tiles);
     
    paint_core->linked_undo_tiles = NULL;
  }

  /*  push the group end  */
  undo_push_group_end (gimage);

  /*  invalidate the drawable--have to do it here, because
   *  it is not done during the actual painting.
   */
  drawable_invalidate_preview (drawable);
  if (paint_core->linked_drawable)
    drawable_invalidate_preview (paint_core->linked_drawable);


}


void 
paint_core_16_cleanup  (
                        PaintCore16 * paint_core 
                        )
{
  if (paint_core->undo_tiles)
    {
      canvas_delete (paint_core->undo_tiles);
      paint_core->undo_tiles = NULL;
    }

  if (paint_core->linked_undo_tiles)
    {
      canvas_delete (paint_core->linked_undo_tiles);
      paint_core->linked_undo_tiles = NULL;
    }

  if (paint_core->canvas_tiles)
    {
      canvas_delete (paint_core->canvas_tiles);
      paint_core->canvas_tiles = NULL;
    }

  if (paint_core->canvas_buf)
    {
      canvas_delete (paint_core->canvas_buf);
      paint_core->canvas_buf = NULL;
      paint_core->canvas_buf_height = 0;
      paint_core->canvas_buf_width = 0;
    }

  if (paint_core->linked_canvas_buf)
    {
      canvas_delete (paint_core->linked_canvas_buf);
      paint_core->linked_canvas_buf = NULL;
    }

  if (paint_core->orig_buf)
    {
      canvas_delete (paint_core->orig_buf);
      paint_core->orig_buf = NULL;
    }
}

static void 
paint_core_16_button_press  (
                             Tool * tool,
                             GdkEventButton * bevent,
                             gpointer gdisp_ptr
                             )
{
  PaintCore16 * paint_core;
  GDisplay * gdisp;
  GimpDrawable * drawable;
  GimpDrawable * linked_drawable;
  int draw_line = 0;
  double x, y;

  gdisp = (GDisplay *) gdisp_ptr;
  paint_core = (PaintCore16 *) tool->private;

  gdisplay_untransform_coords_f (gdisp,
                                 (double) bevent->x, (double) bevent->y,
                                 &x, &y,
                                 TRUE);
  drawable = gimage_active_drawable (gdisp->gimage);

  if (GIMP_IS_LAYER (drawable))
    linked_drawable = gimage_linked_drawable (gdisp->gimage);
  else 
    linked_drawable = NULL; 
  
  if (! paint_core_16_init_linked (paint_core, drawable, linked_drawable, x, y))
    return;

  paint_core->state = bevent->state;

  /*  if this is a new image, reinit the core vals  */
  if (gdisp_ptr != tool->gdisp_ptr ||
      ! (bevent->state & GDK_SHIFT_MASK))
    {
      /*  initialize some values  */
      paint_core->startx = paint_core->lastx = paint_core->curx;
      paint_core->starty = paint_core->lasty = paint_core->cury;
    }
  /*  If shift is down and this is not the first paint
   *  stroke, then draw a line from the last coords to the pointer
   */
  else if (bevent->state & GDK_SHIFT_MASK)
    {
      draw_line = 1;
      paint_core->startx = paint_core->lastx;
      paint_core->starty = paint_core->lasty;
    }

  tool->state = ACTIVE;
  tool->gdisp_ptr = gdisp_ptr;
  tool->paused_count = 0;

  /*  pause the current selection and grab the pointer  */
  gdisplays_selection_visibility (gdisp->gimage->ID, SelectionPause);

#if 0
  /* add motion memory if you press mod1 first */
  if (bevent->state & GDK_MOD1_MASK)
    gdk_pointer_grab (gdisp->canvas->window, FALSE,
		      GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		      NULL, NULL, bevent->time);
  else
    gdk_pointer_grab (gdisp->canvas->window, FALSE,
		      GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		      NULL, NULL, bevent->time);
#endif 

  /*  Let the specific painting function initialize itself  */
  (* paint_core->paint_func) (paint_core, drawable, INIT_PAINT);

  /*  Paint to the image  */
  if (draw_line)
    {
      paint_core_16_interpolate (paint_core, drawable);
      paint_core->lastx = paint_core->curx;
      paint_core->lasty = paint_core->cury;
    }
  else
    (* paint_core->paint_func) (paint_core, drawable, MOTION_PAINT);

  gdisplay_flush (gdisp);
}


static void 
paint_core_16_button_release  (
                               Tool * tool,
                               GdkEventButton * bevent,
                               gpointer gdisp_ptr
                               )
{
  GDisplay * gdisp;
  GImage * gimage;
  PaintCore16 * paint_core;

  gdisp = (GDisplay *) gdisp_ptr;
  gimage = gdisp->gimage;
  paint_core = (PaintCore16 *) tool->private;

  /*  resume the current selection and ungrab the pointer  */
  gdisplays_selection_visibility (gdisp->gimage->ID, SelectionResume);

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  /*  Let the specific painting function finish up  */
  (* paint_core->paint_func) (paint_core, gimage_active_drawable (gdisp->gimage), FINISH_PAINT);

  /*  Set tool state to inactive -- no longer painting */
  tool->state = INACTIVE;

  paint_core_16_finish (paint_core, gimage_active_drawable (gdisp->gimage), tool->ID);
  gdisplays_flush ();
}


static void 
paint_core_16_motion  (
                       Tool * tool,
                       GdkEventMotion * mevent,
                       gpointer gdisp_ptr
                       )
{
  GDisplay * gdisp;
  PaintCore16 * paint_core;

  gdisp = (GDisplay *) gdisp_ptr;
  paint_core = (PaintCore16 *) tool->private;

  gdisplay_untransform_coords_f (gdisp,
                                 (double) mevent->x, (double) mevent->y,
				 &paint_core->curx, &paint_core->cury,
                                 TRUE);

  paint_core->state = mevent->state;

  paint_core_16_interpolate (paint_core, gimage_active_drawable (gdisp->gimage));

  gdisplay_flush (gdisp);

  paint_core->lastx = paint_core->curx;
  paint_core->lasty = paint_core->cury;
}


static void 
paint_core_16_cursor_update  (
                              Tool * tool,
                              GdkEventMotion * mevent,
                              gpointer gdisp_ptr
                              )
{
  GDisplay *gdisp;
  Layer *layer;
  GdkCursorType ctype = GDK_TOP_LEFT_ARROW;
  int x, y;

  gdisp = (GDisplay *) gdisp_ptr;

  gdisplay_untransform_coords (gdisp,
                               mevent->x, mevent->y,
                               &x, &y,
                               FALSE, FALSE);
  
  if ((layer = gimage_get_active_layer (gdisp->gimage)))
    {
      int off_x, off_y;
      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
      if (x >= off_x && y >= off_y &&
          x < (off_x + drawable_width (GIMP_DRAWABLE(layer))) &&
          y < (off_y + drawable_height (GIMP_DRAWABLE(layer))))
        {
          /*  One more test--is there a selected region?
           *  if so, is cursor inside?
           */
          if (gimage_mask_is_empty (gdisp->gimage))
            ctype = GDK_PENCIL;
          else if (gimage_mask_value (gdisp->gimage, x, y) != 0)
            ctype = GDK_PENCIL;
        }
    }
  gdisplay_install_tool_cursor (gdisp, ctype);

  if (tool->type == CLONE)
    {
      ((PaintCore *)tool->private)->curx = x;
      ((PaintCore *)tool->private)->cury = y;  
      (*((PaintCore *)tool->private)->cursor_func) ((PaintCore *)tool->private, 
						    GIMP_DRAWABLE(layer),
						    mevent->state); 
    }
    
}


static void 
paint_core_16_control  (
                        Tool * tool,
                        int action,
                        gpointer gdisp_ptr
                        )
{
  PaintCore16 * paint_core;
  GDisplay *gdisp;
  GimpDrawable * drawable;

  gdisp = (GDisplay *) gdisp_ptr;
  paint_core = (PaintCore16 *) tool->private;
  drawable = gimage_active_drawable (gdisp->gimage);

  switch (action)
    {
    case PAUSE :
      draw_core_pause (paint_core->core, tool);
      break;
    case RESUME :
      draw_core_resume (paint_core->core, tool);
      break;
    case HALT :
      (* paint_core->paint_func) (paint_core, drawable, FINISH_PAINT);
      paint_core_16_cleanup (paint_core);
      break;
    }
}


static void 
paint_core_16_no_draw  (
                        Tool * tool
                        )
{
  return;
}


/* ------------------------------------------------------------------------

   PaintCore16 Backend

*/
void 
paint_core_16_area_setup  (
                     PaintCore16 * paint_core,
                     GimpDrawable * drawable
                     )
{
  Tag   tag;
  int   x, y;
  int   dw, dh;
  int   bw, bh;
  int   x1, y1, x2, y2;
  

  bw = canvas_width (paint_core->brush_mask);
  bh = canvas_height (paint_core->brush_mask);
  
  /* adjust the x and y coordinates to the upper left corner of the brush */  
  x = (int) paint_core->curx - bw / 2;
  y = (int) paint_core->cury - bh / 2;
  
  dw = drawable_width (drawable);
  dh = drawable_height (drawable);

#define PAINT_CORE_16_C_4_cw  
#ifdef BRUSH_WITH_BORDER 
  x1 = CLAMP (x - 1 , 0, dw);
  y1 = CLAMP (y - 1, 0, dh);
  x2 = CLAMP (x + bw + 1, 0, dw);
  y2 = CLAMP (y + bh + 1, 0, dh);
#else  
  x1 = CLAMP (x , 0, dw);
  y1 = CLAMP (y , 0, dh);
  x2 = CLAMP (x + bw , 0, dw);
  y2 = CLAMP (y + bh , 0, dh);
#endif
  /* save the boundaries of this paint hit */
  paint_core->x = x1;
  paint_core->y = y1;
  paint_core->w = x2 - x1;
  paint_core->h = y2 - y1;
  
  /* configure the canvas buffer */
  tag = tag_set_alpha (drawable_tag (drawable), ALPHA_YES);

  if (paint_core->canvas_buf)
    canvas_delete (paint_core->canvas_buf);

  paint_core->canvas_buf = canvas_new (tag,
                           (x2 - x1), (y2 - y1),
                           STORAGE_FLAT);
  paint_core->setup_mode = NORMAL_SETUP;
  (*paint_core->painthit_setup) (paint_core, paint_core->canvas_buf); 

  /* configure the linked_canvas_buffer if there is one */
  if (paint_core->linked_drawable)
  {
    tag = tag_set_alpha (drawable_tag (paint_core->linked_drawable), ALPHA_YES);

    if (paint_core->linked_canvas_buf)
      canvas_delete (paint_core->linked_canvas_buf);

    paint_core->linked_canvas_buf = canvas_new (tag,
			     (x2 - x1), (y2 - y1),
			     STORAGE_FLAT);
    paint_core->setup_mode = LINKED_SETUP;
    (*paint_core->painthit_setup) (paint_core, paint_core->linked_canvas_buf); 
  }

  paint_core->canvas_buf_width = (x2 - x1);
  paint_core->canvas_buf_height = (y2 - y1);
  
}



Canvas * 
paint_core_16_area_original  (
                              PaintCore16 * paint_core,
                              GimpDrawable *drawable,
                              int x1,
                              int y1,
                              int x2,
                              int y2
                              )
{
  PixelArea srcPR, destPR, undoPR;
  Tag tag;

  /*tag = tag_set_alpha (drawable_tag (drawable), ALPHA_YES);*/
  tag = drawable_tag (drawable);
  if (paint_core->orig_buf)
    canvas_delete (paint_core->orig_buf);
  paint_core->orig_buf = canvas_new (tag, (x2 - x1), (y2 - y1), STORAGE_TILED);

  
  x1 = CLAMP (x1, 0, drawable_width (drawable));
  y1 = CLAMP (y1, 0, drawable_height (drawable));
  x2 = CLAMP (x2, 0, drawable_width (drawable));
  y2 = CLAMP (y2, 0, drawable_height (drawable));

  
  pixelarea_init (&srcPR, drawable_data (drawable),
                  x1, y1, (x2 - x1), (y2 - y1), FALSE);
  pixelarea_init (&undoPR, paint_core->undo_tiles,
                  x1, y1, (x2 - x1), (y2 - y1), FALSE);
  pixelarea_init (&destPR, paint_core->orig_buf,
                  0, 0, (x2 - x1), (y2 - y1), TRUE);


  {
    void * pag;
    int h;
    
    for (pag = pixelarea_register_noref (3, &srcPR, &undoPR, &destPR);
         pag != NULL;
         pag = pixelarea_process (pag))
      {
        PixelRow srow, drow;
        PixelArea *s, *d;

        d = &destPR;
        {
          int x = pixelarea_x (&undoPR);
          int y = pixelarea_y (&undoPR);
          if (canvas_portion_alloced (paint_core->undo_tiles, x, y) == TRUE)
            s = &undoPR;
          else
            s = &srcPR;
        }
        
        if (pixelarea_ref (s) == TRUE)
          {
            if (pixelarea_ref (d) == TRUE)
              {
                h = pixelarea_height (s);
                while (h--)
                  {
                    pixelarea_getdata (s, &srow, h);
                    pixelarea_getdata (d, &drow, h);
                    copy_row (&srow, &drow);
                  }
                pixelarea_unref (d);
              }
            pixelarea_unref (s);
          }
      }
  }
  
  return paint_core->orig_buf;
}

Canvas * 
paint_core_16_area_original2  (
                              PaintCore16 * paint_core,
                              GimpDrawable *drawable,
                              int x1,
                              int y1,
                              int x2,
                              int y2
                              )
{
  PixelArea srcPR, destPR, undoPR;
  Tag tag;

  /*tag = tag_set_alpha (drawable_tag (drawable), ALPHA_YES);*/
  tag = drawable_tag (drawable);
  if (paint_core->orig_buf)
    canvas_delete (paint_core->orig_buf);
  paint_core->orig_buf = canvas_new (tag, (x2 - x1), (y2 - y1), STORAGE_TILED);

  
  x1 = CLAMP (x1, 0, drawable_width (drawable));
  y1 = CLAMP (y1, 0, drawable_height (drawable));
  x2 = CLAMP (x2, 0, drawable_width (drawable));
  y2 = CLAMP (y2, 0, drawable_height (drawable));

  
  pixelarea_init (&srcPR, drawable_data (drawable),
                  x1, y1, (x2 - x1), (y2 - y1), FALSE);
  pixelarea_init (&undoPR, paint_core->undo_tiles,
                  x1, y1, (x2 - x1), (y2 - y1), FALSE);
  pixelarea_init (&destPR, paint_core->orig_buf,
                  0, 0, (x2 - x1), (y2 - y1), TRUE);


  {
    void * pag;
    int h;
    
    for (pag = pixelarea_register_noref (3, &srcPR, &undoPR, &destPR);
         pag != NULL;
         pag = pixelarea_process (pag))
      {
        PixelRow srow, drow;
        PixelArea *s, *d;

        d = &destPR;
        {
          int x = pixelarea_x (&undoPR);
          int y = pixelarea_y (&undoPR);
          /*if (canvas_portion_alloced (paint_core->undo_tiles, x, y) == TRUE)
            s = &undoPR;
          else
            s = &srcPR;*/
          if (canvas_portion_alloced (drawable_data (drawable), x, y) == TRUE)
            s = &srcPR;
          else
            s = &undoPR;
        }
        
        if (pixelarea_ref (s) == TRUE)
          {
            if (pixelarea_ref (d) == TRUE)
              {
                h = pixelarea_height (s);
                while (h--)
                  {
                    pixelarea_getdata (s, &srow, h);
                    pixelarea_getdata (d, &drow, h);
                    copy_row (&srow, &drow);
                  }
                pixelarea_unref (d);
              }
            pixelarea_unref (s);
          }
      }
  }
  
  return paint_core->orig_buf;
}


void 
paint_core_16_area_paste  (
                           PaintCore16 * paint_core,
                           GimpDrawable * drawable,
                           gfloat brush_opacity,
                           gfloat image_opacity,
                           BrushHardness brush_hardness,
                           ApplyMode apply_mode,
                           int paint_mode
                           )
{
  Canvas *brush_mask;
  Canvas *undo_canvas;
  Canvas *undo_linked_canvas;

  if (! drawable_gimage (drawable))
    return;

  painthit_init (paint_core, drawable, paint_core->undo_tiles);

  if (paint_core->linked_drawable)
    painthit_init (paint_core, paint_core->linked_drawable, paint_core->linked_undo_tiles);

  if (gimp_brush_get_noise_mode()) 
    brush_mask = brush_mask_noise (paint_core, paint_core->brush_mask);
  else 
    brush_mask = paint_core->brush_mask;

  brush_mask = brush_mask_get (paint_core, brush_mask, brush_hardness);

  switch (apply_mode)
    {
    case CONSTANT:
      brush_to_canvas_tiles(paint_core,brush_mask,brush_opacity);
      canvas_tiles_to_canvas_buf(paint_core); 
      undo_canvas = paint_core->undo_tiles; 
      undo_linked_canvas = paint_core->linked_undo_tiles; 
      break;
    case INCREMENTAL:
      brush_to_canvas_buf(paint_core,brush_mask,brush_opacity);
      undo_canvas = NULL; 
      undo_linked_canvas = NULL; 
      break;
    }
  
  gimage_apply_painthit (drawable_gimage (drawable), drawable,
			 undo_canvas, paint_core->canvas_buf,
			 0, 0,
			 0, 0,
			 FALSE, image_opacity, paint_mode,
			 paint_core->x, paint_core->y);

  if (paint_core->linked_drawable)
    gimage_apply_painthit (drawable_gimage (drawable), paint_core->linked_drawable,
			   undo_linked_canvas, paint_core->linked_canvas_buf,
			   0, 0,
			   0, 0,
			   FALSE, image_opacity, paint_mode,
			   paint_core->x, paint_core->y);

  painthit_finish (drawable, paint_core, paint_core->canvas_buf);
}

void
paint_core_16_area_replace  (
                             PaintCore16 * paint_core,
                             GimpDrawable *drawable,
                             gfloat brush_opacity,
                             gfloat image_opacity,
                             BrushHardness brush_hardness,
                             ApplyMode apply_mode,
                             int paint_mode  /*this is not used here*/
                             )
{
  Canvas *brush_mask;

  if (! drawable_gimage (drawable))
    return;
  
  if (! drawable_has_alpha (drawable))
    {
      paint_core_16_area_paste (paint_core, drawable,
                                brush_opacity, image_opacity,
                                brush_hardness, apply_mode, NORMAL_MODE);
      if (paint_core->linked_drawable)
	paint_core_16_area_paste (paint_core, paint_core->linked_drawable,
				  brush_opacity, image_opacity,
				  brush_hardness, apply_mode, NORMAL_MODE);
      return;
    }
  
  painthit_init (paint_core, drawable, paint_core->undo_tiles);

  if (paint_core->linked_drawable)
    painthit_init (paint_core, paint_core->linked_drawable, paint_core->linked_undo_tiles);

  brush_mask = brush_mask_get (paint_core, paint_core->brush_mask, brush_hardness);

  
  gimage_replace_painthit (drawable_gimage (drawable), drawable,
			 paint_core->canvas_buf,
			 NULL, 
			 FALSE, image_opacity, brush_mask,
			 paint_core->x, paint_core->y);

  if (paint_core->linked_drawable)
    gimage_replace_painthit (drawable_gimage (drawable), paint_core->linked_drawable,
			   paint_core->linked_canvas_buf,
			   NULL, 
			   FALSE, image_opacity, brush_mask,
			   paint_core->x, paint_core->y);

  painthit_finish (drawable, paint_core, paint_core->canvas_buf);
}


static void 
painthit_init  (
                PaintCore16 * paint_core,
                GimpDrawable *drawable,
		Canvas * undo_tiles
                )
{

  PixelArea undo;
  PixelArea canvas;
  PixelArea src;
  PixelArea dst;
  void * pag;

  gint x = paint_core->x;  
  gint y = paint_core->y; 
  gint w = paint_core->canvas_buf_width; 
  gint h = paint_core->canvas_buf_height; 

  pixelarea_init (&undo, undo_tiles,
                  x, y, w, h, TRUE);
  pixelarea_init (&canvas, drawable_data (drawable),
                  x, y, w, h, FALSE);

  for (pag = pixelarea_register_noref (2, &undo, &canvas);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      int xx = pixelarea_x (&undo);
      int yy = pixelarea_y (&undo);

      if (canvas_portion_alloced (undo_tiles, xx, yy) == FALSE)
        {
          int ll = canvas_portion_x (undo_tiles, xx, yy);
          int tt = canvas_portion_y (undo_tiles, xx, yy);
          int ww = canvas_portion_width (undo_tiles, ll, tt);
          int hh = canvas_portion_height (undo_tiles, ll, tt);

          /* alloc the portion of the undo tiles */
          canvas_portion_alloc (undo_tiles, xx, yy);

          /* init the undo section from the original image */
          pixelarea_init (&src, drawable_data (drawable),
                          ll, tt, ww, hh, FALSE);
          pixelarea_init (&dst, undo_tiles,
                          ll, tt, ww, hh, TRUE);
         copy_area (&src, &dst);
        }
    }
}

static void
canvas_tiles_to_brush(
                             PaintCore16 * paint_core,
                             Canvas * brush_mask,
                             gfloat brush_opacity 
                             )
{
  PixelArea srcPR, maskPR;
    int xoff, yoff;
    int x, y;
      
    x = (int) paint_core->curx - (canvas_width (brush_mask) >> 1);
    y = (int) paint_core->cury - (canvas_height (brush_mask) >> 1);
    xoff = (x < 0) ? -x : 0;
    yoff = (y < 0) ? -y : 0;
    
    pixelarea_init (&srcPR, paint_core->canvas_tiles,
                    paint_core->x, paint_core->y,
                    paint_core->canvas_buf_width, paint_core->canvas_buf_height,
                    FALSE);      
    pixelarea_init (&maskPR, brush_mask,
                    xoff, yoff,
                    canvas_width (brush_mask), canvas_height (brush_mask),
                    TRUE);

    copy_area (&srcPR, &maskPR);
}

static void
brush_to_canvas_tiles(
                             PaintCore16 * paint_core,
                             Canvas * brush_mask,
                             gfloat brush_opacity 
                             )
{
  PixelArea srcPR, maskPR;
    int xoff, yoff;
    int x, y;
      
    x = (int) paint_core->curx - (canvas_width (brush_mask) >> 1);
    y = (int) paint_core->cury - (canvas_height (brush_mask) >> 1);
    xoff = (x < 0) ? -x : 0;
    yoff = (y < 0) ? -y : 0;
    
    pixelarea_init (&srcPR, paint_core->canvas_tiles,
                    paint_core->x, paint_core->y,
                    paint_core->canvas_buf_width, paint_core->canvas_buf_height,
                    TRUE);      

    pixelarea_init (&maskPR, brush_mask,
                    xoff, yoff,
                    canvas_width (brush_mask), canvas_height (brush_mask),
                    TRUE);

    combine_mask_and_area (&srcPR, &maskPR, brush_opacity);
}

static void
canvas_tiles_to_canvas_buf(
                          PaintCore16 * paint_core
                          )
{
  PixelArea srcPR, maskPR;

  /*  apply the canvas tiles to the canvas buf  */
  pixelarea_init (&srcPR, paint_core->canvas_buf,
                  0, 0,
                  paint_core->canvas_buf_width, paint_core->canvas_buf_height,
                  TRUE);
  pixelarea_init (&maskPR, paint_core->canvas_tiles,
                  paint_core->x, paint_core->y,
                  paint_core->canvas_buf_width, paint_core->canvas_buf_height,
                  FALSE);      
  apply_mask_to_area (&srcPR, &maskPR, 1.0);

  if (paint_core->linked_drawable)
  { 
    /*  apply the canvas tiles to the linked canvas buf  */
    pixelarea_init (&srcPR, paint_core->linked_canvas_buf,
		    0, 0,
		    paint_core->canvas_buf_width, paint_core->canvas_buf_height,
		    TRUE);
    pixelarea_init (&maskPR, paint_core->canvas_tiles,
		    paint_core->x, paint_core->y,
		    paint_core->canvas_buf_width, paint_core->canvas_buf_height,
		    FALSE);      
    apply_mask_to_area (&srcPR, &maskPR, 1.0);
  }
}


static void
brush_to_canvas_buf(
                             PaintCore16 * paint_core,
                             Canvas * brush_mask,
                             gfloat brush_opacity 
                             )
{
  PixelArea srcPR, maskPR;

  /*  combine the canvas buf and the brush mask to the canvas buf  */
  pixelarea_init (&srcPR, paint_core->canvas_buf,
                  0, 0,
                  paint_core->canvas_buf_width, paint_core->canvas_buf_height,
                  TRUE);
  pixelarea_init (&maskPR, brush_mask,
                  0, 0,
                  canvas_width (brush_mask), canvas_height (brush_mask),
                  TRUE);
  apply_mask_to_area (&srcPR, &maskPR, brush_opacity);

  if (paint_core->linked_drawable)
    {
	/*  combine the linked canvas buf and the brush mask to the canvas buf  */
	pixelarea_init (&srcPR, paint_core->linked_canvas_buf,
			0, 0,
			paint_core->canvas_buf_width, paint_core->canvas_buf_height,
			TRUE);
	pixelarea_init (&maskPR, brush_mask,
			0, 0,
			canvas_width (brush_mask), canvas_height (brush_mask),
			TRUE);
	apply_mask_to_area (&srcPR, &maskPR, brush_opacity);
    }
}
  


static void
painthit_finish (
                 GimpDrawable * drawable,
                 PaintCore16 * paint_core,
                 Canvas * painthit
                 )
{
  GImage *gimage;
  int offx, offy;

  /*  Update the undo extents  */
  paint_core->x1 = MIN (paint_core->x1, paint_core->x);
  paint_core->y1 = MIN (paint_core->y1, paint_core->y);
  paint_core->x2 = MAX (paint_core->x2, (paint_core->x + canvas_width (painthit)));
  paint_core->y2 = MAX (paint_core->y2, (paint_core->y + canvas_height (painthit)));
  
  /*  Update the gimage--it is important to call gdisplays_update_area
   *  instead of drawable_update because we don't want the drawable
   *  preview to be constantly invalidated
   */
  gimage = drawable_gimage (drawable);
  drawable_offsets (drawable, &offx, &offy);
  gdisplays_update_area (gimage->ID,
                         paint_core->x + offx, paint_core->y + offy,
                         canvas_width (painthit), canvas_height (painthit));

  if (paint_core->noise_mask)
   {
     canvas_delete (paint_core->noise_mask);
     paint_core->noise_mask = NULL;
   }

  if (paint_core->solid_mask)
   {
     canvas_delete (paint_core->solid_mask);
     paint_core->solid_mask = NULL;
   }

}


static Canvas * 
brush_mask_get  (
		 PaintCore16 *paint_core,
                 Canvas* brush_mask,
                 int brush_hardness
                 )
{
  Canvas * bm;

  switch (brush_hardness)
    {
    case SOFT:
#ifdef BRUSH_WITH_BORDER 
      bm = brush_mask_subsample (brush_mask,
                                 paint_core->curx, paint_core->cury);
#else
      bm = brush_mask;
#endif
      break;
      
    case HARD:
      bm = brush_mask_solidify (paint_core, brush_mask);
      break;

    case EXACT:
      bm = brush_mask;
      break;
      
    default:
      bm = NULL;
      break;
    }

  return bm;
}


#ifdef BRUSH_WITH_BORDER 
static Canvas * 
brush_mask_subsample  (
                       Canvas * mask,
                       double x,
                       double y
                       )
{
#define KERNEL_WIDTH   3
#define KERNEL_HEIGHT  3

  /*  Brush pixel subsampling kernels  */
  static int subsample[4][4][9] =
  {
    {
      { 16, 48, 0, 48, 144, 0, 0, 0, 0 },
      { 0, 64, 0, 0, 192, 0, 0, 0, 0 },
      { 0, 48, 16, 0, 144, 48, 0, 0, 0 },
      { 0, 32, 32, 0, 96, 96, 0, 0, 0 },
    },
    {
      { 0, 0, 0, 64, 192, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 256, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 192, 64, 0, 0, 0 },
      { 0, 0, 0, 0, 128, 128, 0, 0, 0 },
    },
    {
      { 0, 0, 0, 48, 144, 0, 16, 48, 0 },
      { 0, 0, 0, 0, 192, 0, 0, 64, 0 },
      { 0, 0, 0, 0, 144, 48, 0, 48, 16 },
      { 0, 0, 0, 0, 96, 96, 0, 32, 32 },
    },
    {
      { 0, 0, 0, 32, 96, 0, 32, 96, 0 },
      { 0, 0, 0, 0, 128, 0, 0, 128, 0 },
      { 0, 0, 0, 0, 96, 32, 0, 96, 32 },
      { 0, 0, 0, 0, 64, 64, 0, 64, 64 },
    },
  };
  
  static Canvas * kernel_brushes[4][4] = {
    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL}
  };

  static Canvas * last_brush = NULL;

  Canvas * dest;
  double left;
  unsigned char * m, * d;
  int * k;
  int index1, index2;
  int * kernel;
  int new_val;
  int i, j;
  int r, s;

  x += (x < 0) ? canvas_width (mask) : 0;
  left = x - ((int) x);
  index1 = (int) (left * 4);
  index1 = (index1 < 0) ? 0 : index1;

  y += (y < 0) ? canvas_height (mask) : 0;
  left = y - ((int) y);
  index2 = (int) (left * 4);
  index2 = (index2 < 0) ? 0 : index2;

  kernel = subsample[index2][index1];

  if ((mask == last_brush) && kernel_brushes[index2][index1])
    return kernel_brushes[index2][index1];
  else if (mask != last_brush)
    for (i = 0; i < 4; i++)
      for (j = 0; j < 4; j++)
	{
	  if (kernel_brushes[i][j])
	    canvas_delete (kernel_brushes[i][j]);
	  kernel_brushes[i][j] = NULL;
	}

  last_brush = mask;
  kernel_brushes[index2][index1] = canvas_new (canvas_tag (mask),
                                               canvas_width (mask) + 2,
                                               canvas_height (mask) + 2,
                                               canvas_storage (mask));
  dest = kernel_brushes[index2][index1];

  /* temp hack */
  canvas_portion_refro (mask, 0, 0);
  canvas_portion_refrw (dest, 0, 0);

  for (i = 0; i < canvas_height (mask); i++)
    {
      for (j = 0; j < canvas_width (mask); j++)
	{
          m = canvas_portion_data (mask, j, i);
	  k = kernel;
	  for (r = 0; r < KERNEL_HEIGHT; r++)
	    {
	      d = canvas_portion_data (dest, j, i+r);
	      s = KERNEL_WIDTH;
	      while (s--)
		{
		  new_val = *d + ((*m * *k++) >> 8);
		  *d++ = (new_val > 255) ? 255 : new_val;
		}
	    }
	}
    }

  /* temp hack */
  canvas_portion_unref (mask, 0, 0);
  canvas_portion_unref (dest, 0, 0);

  return dest;
}
#endif

typedef void  (*BrushMaskNoiseFunc) (PaintCore16 *,Canvas*,Canvas*);
static BrushMaskNoiseFunc brush_mask_noise_funcs[] =
{
  brush_mask_noise_u8,
  brush_mask_noise_u16,
  brush_mask_noise_float,
  brush_mask_noise_float16
};

static Canvas * 
brush_mask_noise  (
                      PaintCore16 * paint_core,
                      Canvas * brush_mask
                      )
{
  Canvas * noise_brush = NULL;
  Precision prec = tag_precision (canvas_tag (brush_mask));  

#ifdef BRUSH_WITH_BORDER  
  noise_brush = canvas_new (canvas_tag (brush_mask),
                            canvas_width (brush_mask) + 2,
                            canvas_height (brush_mask) + 2,
                            canvas_storage (brush_mask));
#else 
  noise_brush = canvas_new (canvas_tag (brush_mask),
                            canvas_width (brush_mask),
                            canvas_height (brush_mask),
                            canvas_storage (brush_mask));
#endif
  canvas_portion_refro (brush_mask, 0, 0);
  canvas_portion_refrw (noise_brush, 0, 0);
  
  (*brush_mask_noise_funcs [prec-1]) (paint_core, brush_mask, noise_brush); 

  canvas_portion_unref (noise_brush, 0, 0);
  canvas_portion_unref (brush_mask, 0, 0);

  paint_core->noise_mask = noise_brush;
  
  return noise_brush;
}

static void brush_mask_noise_u8 (
                        PaintCore16 * paint_core,
			Canvas *brush_mask,
			Canvas *noise_brush
		      )
{  
  /* get the data and advance one line into it  */
#ifdef BRUSH_WITH_BORDER
  guint8* data = (guint8*)canvas_portion_data (noise_brush, 0, 1);
#else
  guint8* data = (guint8*)canvas_portion_data (noise_brush, 0, 0);
#endif
  guint8* src = (guint8*)canvas_portion_data (brush_mask, 0, 0);
  gint i, j;

  for (i = 0; i < canvas_height (brush_mask); i++)
    {
#ifdef BRUSH_WITH_BORDER
      data++;
#endif
      for (j = 0; j < canvas_width (brush_mask); j++)
	{
	  *data++ = (*src++) ? 255 : 0;
	}
#ifdef BRUSH_WITH_BORDER
      data++;
#endif
    }
}


static void brush_mask_noise_u16 (
                        PaintCore16 * paint_core,
			Canvas *brush_mask,
			Canvas *noise_brush
		      )
{  
  /* get the data and advance one line into it  */
#ifdef BRUSH_WITH_BORDER
  guint16* data = (guint16*)canvas_portion_data (noise_brush, 0, 1);
#else
  guint16* data = (guint16*)canvas_portion_data (noise_brush, 0, 0);
#endif
  guint16* src = (guint16*)canvas_portion_data (brush_mask, 0, 0);
  gint i, j;

  for (i = 0; i < canvas_height (brush_mask); i++)
    {
#ifdef BRUSH_WITH_BORDER
      data++;
#endif
      for (j = 0; j < canvas_width (brush_mask); j++)
	{
	  *data++ = (*src++) ? 65535 : 0;
	}
#ifdef BRUSH_WITH_BORDER
      data++;
#endif
    }
}

static void brush_mask_noise_float (
                        PaintCore16 * paint_core,
			Canvas *brush_mask,
			Canvas *noise_brush
		      )
{  
  /* get the data and advance one line into it  */
#ifdef BRUSH_WITH_BORDER
  gfloat* data =(gfloat*)canvas_portion_data (noise_brush, 0, 1);
#else
  gfloat* data =(gfloat*)canvas_portion_data (noise_brush, 0, 0);
#endif
  gfloat* src = (gfloat*)canvas_portion_data (brush_mask, 0, 0);
  gint i, j;

  for (i = 0; i < canvas_height (brush_mask); i++)
    {
#ifdef BRUSH_WITH_BORDER
      data++;
#endif
      for (j = 0; j < canvas_width (brush_mask); j++)
	{
	  *data++ = (*src++) ? 1.0 : 0.0;
	}
#ifdef BRUSH_WITH_BORDER
      data++;
#endif
    }
}

static void brush_mask_noise_float16 (
                        PaintCore16 * paint_core,
			Canvas *brush_mask,
			Canvas *noise_brush
		      )
{  
  ShortsFloat u;
  float x,y;
  gint i, j;
  gint draw_w = drawable_width (paint_core->drawable);
  gint draw_h = drawable_height (paint_core->drawable);
  BrushNoiseInfo  info;
  float f;
  float start;
  float width;
  float end;
  gint w = canvas_width (brush_mask);
  gint h = canvas_height (brush_mask);
  gfloat val;
  gfloat src_val;
  gfloat xoff; 
  gfloat yoff; 
  gfloat out;
  gfloat c,s,theta,xnew,ynew;
  
#ifdef BRUSH_WITH_BORDER
  guint16* data =(guint16*)canvas_portion_data (noise_brush, 0, 1);
#else
  guint16* data =(guint16*)canvas_portion_data (noise_brush, 0, 0);
#endif
  guint16* src = (guint16*)canvas_portion_data (brush_mask, 0, 0);

  gimp_brush_get_noise_info (&info);
 
  f = info.freq * 256;

  if (f) 
  {
    xoff = (int)((rand()/32767.0)*255); 
    yoff = (int)((rand()/32767.0)*255); 
  }
  else 
  {
    xoff = 0;
    yoff = 0;
  }

  xoff = xoff + .5;
  yoff = yoff + .5;

  start = info.step_start * .7;
  width = info.step_width;

  theta = rand()/32767.0; 
  c = cos (2 * 3.14 * theta);
  s = sin (2 * 3.14 * theta);
  
  for (i = 0; i < h; i++)
    {
      y = (i/256.0)  * f + yoff;

#ifdef BRUSH_WITH_BORDER
      data++;
#endif
      for (j = 0; j < w; j++)
	{
	  x = (j/256.0) * f + xoff;
		
#if 1 
  	  xnew = c*x + s*y;
	  ynew = -s*x + c*y;
#endif

	  val = noise (xnew, ynew);   
#if 1 
	  out =  (.5 * (val + 1)); 
	  end = start + width;
          if (end>1) end = 1.0;
	  out = noise_smoothstep (start, end, out);
#endif
	  
	  src_val = FLT( *src++, u);

	  *data++ = FLT16(out *src_val , u);
	}
#ifdef BRUSH_WITH_BORDER
      data++;
#endif
    }
}

typedef void  (*BrushSolidifyMaskFunc) (Canvas*,Canvas*);
static BrushSolidifyMaskFunc brush_solidify_mask_funcs[] =
{
  brush_solidify_mask_u8,
  brush_solidify_mask_u16,
  brush_solidify_mask_float,
  brush_solidify_mask_float16
};

static Canvas * 
brush_mask_solidify  (
		      PaintCore16 * paint_core,	
                      Canvas * brush_mask
                      )
{
  Canvas * solid_brush = NULL;
  Precision prec = tag_precision (canvas_tag (brush_mask));  

#ifdef BRUSH_WITH_BORDER  
  solid_brush = canvas_new (canvas_tag (brush_mask),
                            canvas_width (brush_mask) + 2,
                            canvas_height (brush_mask) + 2,
                            canvas_storage (brush_mask));
#else 
  solid_brush = canvas_new (canvas_tag (brush_mask),
                            canvas_width (brush_mask) ,
                            canvas_height (brush_mask) ,
                            canvas_storage (brush_mask));
#endif
  canvas_portion_refro (brush_mask, 0, 0);
  canvas_portion_refrw (solid_brush, 0, 0);
  
  (*brush_solidify_mask_funcs [prec-1]) (brush_mask, solid_brush); 

  canvas_portion_unref (solid_brush, 0, 0);
  canvas_portion_unref (brush_mask, 0, 0);
  
  paint_core->solid_mask = solid_brush;
  
  return solid_brush;
}


static void brush_solidify_mask_u8 (
			Canvas *brush_mask,
			Canvas *solid_brush
		      )
{  
  /* get the data and advance one line into it  */
#ifdef BRUSH_WITH_BORDER
  guint8* data = (guint8*)canvas_portion_data (solid_brush, 0, 1);
#else
  guint8* data = (guint8*)canvas_portion_data (solid_brush, 0, 0);
#endif
  guint8* src = (guint8*)canvas_portion_data (brush_mask, 0, 0);
  gint i, j;

  for (i = 0; i < canvas_height (brush_mask); i++)
    {
#ifdef BRUSH_WITH_BORDER
      data++;
#endif
      for (j = 0; j < canvas_width (brush_mask); j++)
	{
	  *data++ = (*src++) ? 255 : 0;
	}
#ifdef BRUSH_WITH_BORDER
      data++;
#endif
    }
}


static void brush_solidify_mask_u16 (
			Canvas *brush_mask,
			Canvas *solid_brush
		      )
{  
  /* get the data and advance one line into it  */
#ifdef BRUSH_WITH_BORDER
  guint16* data = (guint16*)canvas_portion_data (solid_brush, 0, 1);
#else
  guint16* data = (guint16*)canvas_portion_data (solid_brush, 0, 0);
#endif
  guint16* src = (guint16*)canvas_portion_data (brush_mask, 0, 0);
  gint i, j;

  for (i = 0; i < canvas_height (brush_mask); i++)
    {
#ifdef BRUSH_WITH_BORDER
      data++;
#endif
      for (j = 0; j < canvas_width (brush_mask); j++)
	{
	  *data++ = (*src++) ? 65535 : 0;
	}
#ifdef BRUSH_WITH_BORDER
      data++;
#endif
    }
}

static void brush_solidify_mask_float (
			Canvas *brush_mask,
			Canvas *solid_brush
		      )
{  
  /* get the data and advance one line into it  */
#ifdef BRUSH_WITH_BORDER
  gfloat* data =(gfloat*)canvas_portion_data (solid_brush, 0, 1);
#else
  gfloat* data =(gfloat*)canvas_portion_data (solid_brush, 0, 0);
#endif
  gfloat* src = (gfloat*)canvas_portion_data (brush_mask, 0, 0);
  gint i, j;

  for (i = 0; i < canvas_height (brush_mask); i++)
    {
#ifdef BRUSH_WITH_BORDER
      data++;
#endif
      for (j = 0; j < canvas_width (brush_mask); j++)
	{
	  *data++ = (*src++) ? 1.0 : 0.0;
	}
#ifdef BRUSH_WITH_BORDER
      data++;
#endif
    }
}

static void brush_solidify_mask_float16 (
			Canvas *brush_mask,
			Canvas *solid_brush
		      )
{  
  /* get the data and advance one line into it  */
  ShortsFloat u;
#ifdef BRUSH_WITH_BORDER
  guint16* data =(guint16*)canvas_portion_data (solid_brush, 0, 1);
#else
  guint16* data =(guint16*)canvas_portion_data (solid_brush, 0, 0);
#endif
  guint16* src = (guint16*)canvas_portion_data (brush_mask, 0, 0);
  gint i, j;

  for (i = 0; i < canvas_height (brush_mask); i++)
    {
#ifdef BRUSH_WITH_BORDER
      data++;
#endif
      for (j = 0; j < canvas_width (brush_mask); j++)
	{
	  *data++ = (*src++) ? FLT16 (1.0, u) : FLT16 (0.0, u);
	}
#ifdef BRUSH_WITH_BORDER
      data++;
#endif
    }
}
