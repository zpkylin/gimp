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
#include <math.h>
#include "appenv.h"
#include "gimpbrush.h"
#include "gimpbrushlist.h"
#include "canvas.h"
#include "drawable.h"
#include "errors.h"
#include "float16.h"
#include "dodgeburn.h"
#include "gdisplay.h"
#include "paint_funcs_area.h"
#include "paint_core_16.h"
#include "pixelarea.h"
#include "pixelrow.h"
#include "selection.h"
#include "tools.h"

void dodgeburn_area ( PixelArea *, PixelArea *, gint,gint,gfloat  );
typedef void  (*DodgeburnRowFunc) (PixelArea*,PixelArea*,gint,gfloat);
void dodge_highlights_row ( PixelRow *src_row, PixelRow *dest_row, 
	gint type, gfloat exposure);
void dodge_midtones_row ( PixelRow *src_row, PixelRow *dest_row, 
	gint type, gfloat exposure);
void dodge_shadows_row ( PixelRow *src_row, PixelRow *dest_row, 
	gint type, gfloat exposure);
DodgeburnRowFunc dodgeburn_row_func (Tag, gint);

/*  the dodgeburn structures  */


typedef enum
{
  DODGE,
  BURN 
} DodgeBurnType;

typedef enum
{
  DODGEBURN_HIGHLIGHTS,
  DODGEBURN_MIDTONES,
  DODGEBURN_SHADOWS 
} DodgeBurnMode;

typedef struct _DodgeBurnOptions DodgeBurnOptions;
struct _DodgeBurnOptions
{
  DodgeBurnType  type;
  DodgeBurnType  type_d;
  GtkWidget    *type_w[2];

  DodgeBurnMode  mode;     /*highlights,midtones,shadows*/
  DodgeBurnMode  mode_d;
  GtkWidget      *mode_w[3];

  double        exposure;
  double        exposure_d;
  GtkObject    *exposure_w;
};

/*  the dodgeburn tool options  */
static DodgeBurnOptions * dodgeburn_options = NULL;

static void dodgeburn_painthit_setup (PaintCore *,Canvas *);
static void dodgeburn_scale_update (GtkAdjustment *, double *);
static DodgeBurnOptions * create_dodgeburn_options(void);
static void * dodgeburn_paint_func (PaintCore *,  GimpDrawable *, int);

static GtkWidget*
dodgeburn_radio_buttons_new (gchar*      label,
                                gpointer    toggle_val,
                                GtkWidget*  button_widget[],
                                gchar*      button_label[],
                                gint        button_value[],
                                gint        num);
static void
dodgeburn_radio_buttons_update (GtkWidget *widget,
                                   gpointer   data);

static void         dodgeburn_motion 	(PaintCore *, GimpDrawable *);
static void 	    dodgeburn_init   	(PaintCore *, GimpDrawable *);
static void         dodgeburn_finish    (PaintCore *, GimpDrawable *);

#if 0
static gfloat dodgeburn_highlights_lut_func(void *, int, int, gfloat);
static gfloat dodgeburn_midtones_lut_func(void *, int, int, gfloat);
static gfloat dodgeburn_shadows_lut_func(void *, int, int, gfloat);

/* The dodge burn lookup tables */
gfloat dodgeburn_highlights(void *, int, int, gfloat);
gfloat dodgeburn_midtones(void *, int, int, gfloat);
gfloat dodgeburn_shadows(void *, int, int, gfloat);
#endif


static void
dodgeburn_scale_update (GtkAdjustment *adjustment,
			 double        *scale_val)
{
  *scale_val = adjustment->value;
}

static DodgeBurnOptions *
create_dodgeburn_options(void)
{
  DodgeBurnOptions *options;

  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *scale;
  GtkWidget *frame;

  gchar* type_label[2] = { "Dodge", "Burn" };
  gint   type_value[2] = { DODGE, BURN };

  gchar* mode_label[3] = { "Highlights", 
			   "Midtones",
			   "Shadows" };

  gint   mode_value[3] = { DODGEBURN_HIGHLIGHTS, 
			   DODGEBURN_MIDTONES, 
			   DODGEBURN_SHADOWS };

  /*  the new dodgeburn tool options structure  */
  options = (DodgeBurnOptions *) g_malloc (sizeof (DodgeBurnOptions));
  options->type     = options->type_d     = DODGE;
  options->exposure = options->exposure_d = 50.0;
  options->mode = options->mode_d = DODGEBURN_HIGHLIGHTS;

  /*  the main vbox  */
  vbox = gtk_vbox_new(FALSE, 1);

  /*  the exposure scale  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new ("Exposure:");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  options->exposure_w =
    gtk_adjustment_new (options->exposure_d, 0.0, 100.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->exposure_w));
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);

  gtk_signal_connect (GTK_OBJECT (options->exposure_w), "value_changed",
		      (GtkSignalFunc) dodgeburn_scale_update,
		      &options->exposure);

  gtk_widget_show (scale);
  gtk_widget_show (hbox);

  /* the type (dodge or burn) */
  frame = dodgeburn_radio_buttons_new ("Type", 
					  &options->type,
					   options->type_w,
					   type_label,
					   type_value,
					   2);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (options->type_w[options->type_d]), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  mode (highlights, midtones, or shadows)  */
  frame = dodgeburn_radio_buttons_new ("Mode", 
					  &options->mode,
					   options->mode_w,
					   mode_label,
					   mode_value,
					   3);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (options->mode_w[options->mode_d]), TRUE); 
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  tools_register_options(DODGEBURN, vbox);

  return options;
}

static GtkWidget*
dodgeburn_radio_buttons_new (gchar*      label,
                                gpointer    toggle_val,
                                GtkWidget*  button_widget[],
                                gchar*      button_label[],
                                gint        button_value[],
                                gint        num)
{ 
  GtkWidget *frame;
  GtkWidget *vbox;                        
  GSList *group = NULL;                    
  gint i;                                  

  frame = gtk_frame_new (label);        

  g_return_val_if_fail (toggle_val != NULL, frame); 

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  for (i=0; i<num; i++)                    
    {                                      
      button_widget[i] = gtk_radio_button_new_with_label (group,
                                                          button_label[i]);
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (button_widget[i]));
      gtk_box_pack_start (GTK_BOX (vbox), button_widget[i], FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button_widget[i]), "toggled",
                          (GtkSignalFunc) dodgeburn_radio_buttons_update,
                          toggle_val);
      gtk_object_set_data (GTK_OBJECT (button_widget[i]), "toggle_value",
                           (gpointer)button_value[i]);                                        
      gtk_widget_show (button_widget[i]);                                                     
    }                                                                                         
  gtk_widget_show (vbox);                                                                     

  return (frame);                                                                             
}     

static void
dodgeburn_radio_buttons_update (GtkWidget *widget,
                                   gpointer   data)
{
  int *toggle_val;
    
  toggle_val = (int *) data;
  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = (int) gtk_object_get_data (GTK_OBJECT (widget), "toggle_value");
}


void *
dodgeburn_paint_func (PaintCore    *paint_core,
		     GimpDrawable *drawable,
		     int           state)
{
  switch (state)
    {
    case INIT_PAINT:
      dodgeburn_init (paint_core, drawable);
      break;
    case MOTION_PAINT:
      dodgeburn_motion (paint_core, drawable);
      break;
    case FINISH_PAINT:
      dodgeburn_finish (paint_core, drawable);
      break;
    }

  return NULL;
}

static void
dodgeburn_finish ( PaintCore *paint_core,
		 GimpDrawable * drawable)
{
  /*printf("In dodgeburn_finish\n");*/
}

static void
dodgeburn_init ( PaintCore *paint_core,
		 GimpDrawable * drawable)
{
  /*printf("In dodgeburn_init\n");*/
}

Tool *
tools_new_dodgeburn ()
{
  Tool * tool;
  PaintCore * private;

  /*  The tool options  */
  if (! dodgeburn_options)
      dodgeburn_options = create_dodgeburn_options();

  tool = paint_core_new (DODGEBURN);

  private = (PaintCore *) tool->private;
  private->paint_func = dodgeburn_paint_func;
  private->painthit_setup = dodgeburn_painthit_setup;

  return tool;
}

void
tools_free_dodgeburn (Tool *tool)
{
  paint_core_free (tool);
}

static void
dodgeburn_painthit_setup (PaintCore * paint_core,Canvas * painthit)
{
  Canvas * temp_canvas;
  Canvas * orig_canvas;
  PixelArea srcPR, destPR, tempPR;
  gfloat exposure;
  gfloat brush_opacity;
  Tag tag = drawable_tag(paint_core->drawable);
  gint x1, y1, x2, y2;

  if (!painthit)
    return;

  x1 = BOUNDS (paint_core->x, 0, drawable_width (paint_core->drawable));
  y1 = BOUNDS (paint_core->y, 0, drawable_height (paint_core->drawable));
  x2 = BOUNDS (paint_core->x + paint_core->w, 0, drawable_width (paint_core->drawable));
  y2 = BOUNDS (paint_core->y + paint_core->h, 0, drawable_height (paint_core->drawable));

  if (!(x2 - x1) || !(y2 - y1))
    return;

  /*  get the original untouched image  */

#if 1 
  orig_canvas = paint_core_16_area_original (paint_core, 
		paint_core->drawable, x1, y1, x2, y2);

  pixelarea_init (&srcPR, orig_canvas, 
		0, 0, 
		x2-x1, y2-y1, 
		FALSE);
#endif

#if 0 
  pixelarea_init (&srcPR, drawable_data (paint_core->drawable), 
		x1, y1, 
		x2, y2,
		FALSE);
#endif
  /* Get a temp buffer to hold the result of dodgeburning the orig */
  temp_canvas = canvas_new(tag, paint_core->w, paint_core->h, STORAGE_FLAT); 
  pixelarea_init (&tempPR, temp_canvas, 
		0, 0, 
		canvas_width (temp_canvas), canvas_height(temp_canvas),
		TRUE);

  brush_opacity = gimp_brush_get_opacity();
  exposure = (dodgeburn_options->exposure)/100.0;

#if 0
  printf("dodgeburn_motion: brush_opacity is %f\n", brush_opacity);
  printf("dodgeburn_motion: exposure is %f\n", exposure);
#endif
 
  dodgeburn_area (&srcPR, &tempPR,
	dodgeburn_options->type,dodgeburn_options->mode,exposure);

  pixelarea_init (&tempPR, temp_canvas, 
		0, 0, 
		canvas_width (temp_canvas), canvas_height(temp_canvas),
		FALSE);
  
  pixelarea_init (&destPR, painthit, 
		0, 0, 
		paint_core->w, paint_core->h, 
		TRUE);  
 
  if (!drawable_has_alpha (paint_core->drawable))
    add_alpha_area (&tempPR, &destPR);
  else
    copy_area(&tempPR, &destPR);

  canvas_delete(temp_canvas);
}

void
dodgeburn_area (
             PixelArea * src_area,
             PixelArea * dest_area,
	     gint type,
	     gint mode,
	     gfloat exposure
             )
{
  DodgeburnRowFunc dodgeburn_row = dodgeburn_row_func (pixelarea_tag (src_area), mode);
  void * pag;

  if (type == BURN)
    exposure = -exposure; 

  for (pag = pixelarea_register (2, src_area, dest_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src_row;
      PixelRow dest_row;
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &src_row, h);
          pixelarea_getdata (dest_area, &dest_row, h);
          (*dodgeburn_row) (&src_row, &dest_row, type, exposure);
        }
    }
}

DodgeburnRowFunc
dodgeburn_row_func (
                   Tag tag,
		   gint mode
                   )
{
  switch (tag_precision (tag))
    {
    case PRECISION_U8:
      return ;
    case PRECISION_U16:
      return NULL;
    case PRECISION_FLOAT:
      return NULL;
    case PRECISION_FLOAT16:
	if (mode == DODGEBURN_HIGHLIGHTS)
           return dodge_highlights_row; 
	else if (mode == DODGEBURN_MIDTONES)
           return dodge_midtones_row; 
	else 
           return dodge_shadows_row; 
    case PRECISION_NONE:
    default:
      g_warning ("dodgeburn_row_func: bad precision");
    }
  return NULL;
}

void dodge_highlights_row (
			    PixelRow *src_row, 
			    PixelRow *dest_row, 
			    gint type, 
			    gfloat exposure
			   )
{
  gint    b;
  guint16 *src          = (guint16*) pixelrow_data (src_row);
  guint16 *dest         = (guint16*) pixelrow_data (dest_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    num_channels1 = tag_num_channels (pixelrow_tag (dest_row));
  Tag     tag = pixelrow_tag (dest_row);
  gint    width        = pixelrow_width (dest_row);  
  gfloat  sb, db;
  ShortsFloat u;
  gfloat factor = 1.0 + exposure * (.333333);
  gint    num_color_chans;
  gint    has_alpha = (tag_alpha(tag) == ALPHA_YES) ? TRUE: FALSE;

  if (has_alpha)
    num_color_chans = num_channels-1;
  else
    num_color_chans = num_channels;

#if 0
  printf("num_color_chans is %d\n", num_color_chans);
  printf("num_chans is %d\n", num_channels);
#endif
  while (width--)
    {
      for (b = 0; b < num_color_chans; b++)
	{
          sb = FLT(src[b], u);
	  dest[b] = FLT16 (factor * sb, u);
	}
	
      if(has_alpha)
	  dest[b] = src[b];

      src += num_channels;
      dest += num_channels;
    }
}

void dodge_midtones_row (
			    PixelRow *src_row, 
			    PixelRow *dest_row, 
			    gint type, 
			    gfloat exposure
			   )
{
  gint    b;
  guint16 *src          = (guint16*) pixelrow_data (src_row);
  guint16 *dest         = (guint16*) pixelrow_data (dest_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    num_channels1 = tag_num_channels (pixelrow_tag (dest_row));
  Tag     tag = pixelrow_tag (dest_row);
  gint    width        = pixelrow_width (dest_row);  
  gfloat  sb, db;
  ShortsFloat u;
  gfloat factor;
  gint    num_color_chans;
  gint    has_alpha = (tag_alpha(tag) == ALPHA_YES) ? TRUE: FALSE;

  if (has_alpha)
    num_color_chans = num_channels-1;
  else
    num_color_chans = num_channels;

  if (exposure < 0)
    factor = 1.0 - exposure * (.333333);
  else
    factor = 1/(1.0 + exposure);
 
  while (width--)
    {
      for (b = 0; b < num_color_chans; b++)
	{
          sb = FLT(src[b], u);
	  if (sb < 1)
	    dest[b] = FLT16 (pow (sb,factor), u);
	  else
	    dest[b] = src[b];
	}

      if(has_alpha)
	  dest[b] = src[b];

      src += num_channels;
      dest += num_channels;
    }
}

void dodge_shadows_row (
			    PixelRow *src_row, 
			    PixelRow *dest_row, 
			    gint type, 
			    gfloat exposure
			   )
{
  gint    b;
  guint16 *src          = (guint16*) pixelrow_data (src_row);
  guint16 *dest         = (guint16*) pixelrow_data (dest_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    num_channels1 = tag_num_channels (pixelrow_tag (dest_row));
  Tag     tag = pixelrow_tag (dest_row);
  gint    width        = pixelrow_width (dest_row);  
  gfloat  sb, db;
  ShortsFloat u;
  gfloat factor;
  gint    num_color_chans;
  gint    has_alpha = (tag_alpha(tag) == ALPHA_YES) ? TRUE: FALSE;

  if (has_alpha)
    num_color_chans = num_channels-1;
  else
    num_color_chans = num_channels;

  if (exposure >= 0)
  {
    factor = .333333 * exposure;

    while (width--)
      {
	for (b = 0; b < num_color_chans; b++)
	  {
	    sb = FLT( src[b], u);
	    if (sb < 1)
	      dest[b] = FLT16 (factor + sb - factor*sb, u);
 	    else
	      dest[b] = src[b];
	  }

	if(has_alpha)
	    dest[b] = src[b];

	src += num_channels;
	dest += num_channels;
      }
  }
  else   /*exposure < 0*/ 
  {
    factor = -.333333 * exposure;

    while (width--)
      {
	for (b = 0; b < num_color_chans; b++)
	  {
	    sb = FLT( src[b], u);
 	    if (sb < factor)
	      dest[b] = FLT16(0,u);
	    else if (sb <1)
	      dest[b] = FLT16((sb - factor)/(1-factor), u); 
	    else 
	      dest[b] = src[b];
	  }

	if(has_alpha)
	    dest[b] = src[b];

	src += num_channels;
	dest += num_channels;
      }
  }
}

static void
dodgeburn_motion (PaintCore *paint_core,
		 GimpDrawable *drawable)
{
  GImage *gimage;
  Tag tag = drawable_tag (drawable);
 
  if (! (gimage = drawable_gimage (drawable)))
    return;
  if (tag_format (tag) == FORMAT_INDEXED )
   return; 

  /* Set up the painthit with the right thing*/ 
  paint_core_16_area_setup (paint_core, drawable);                                            
  /* Apply the painthit to the drawable */                                                    
  paint_core_16_area_replace (paint_core, drawable, 1.0, 
	gimp_brush_get_opacity(), SOFT, CONSTANT, NORMAL_MODE);
}

static void *
dodgeburn_non_gui_paint_func (PaintCore *paint_core,
			     GimpDrawable *drawable,
			     int        state)
{
  dodgeburn_motion (paint_core, drawable);

  return NULL;
}

gboolean
dodgeburn_non_gui (GimpDrawable *drawable,
    		  double        pressure,
		  int           num_strokes,
		  double       *stroke_array)
{
  int i;

  if (paint_core_init (&non_gui_paint_core, drawable,
		       stroke_array[0], stroke_array[1]))
    {
      /* Set the paint core's paint func */
      non_gui_paint_core.paint_func = dodgeburn_non_gui_paint_func;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      if (num_strokes == 1)
	dodgeburn_non_gui_paint_func (&non_gui_paint_core, drawable, 0);

      for (i = 1; i < num_strokes; i++)
	{
	  non_gui_paint_core.curx = stroke_array[i * 2 + 0];
	  non_gui_paint_core.cury = stroke_array[i * 2 + 1];

	  paint_core_interpolate (&non_gui_paint_core, drawable);

	  non_gui_paint_core.lastx = non_gui_paint_core.curx;
	  non_gui_paint_core.lasty = non_gui_paint_core.cury;
	}

      /* Finish the painting */
      paint_core_finish (&non_gui_paint_core, drawable, -1);

      /* Cleanup */
      paint_core_cleanup (&non_gui_paint_core);
      return TRUE;
    }
  else
    return FALSE;
}
