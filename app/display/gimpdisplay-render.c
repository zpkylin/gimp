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
#include "appenv.h"
#include "canvas.h"
#include "channel.h"
#include "colormaps.h"
#include "errors.h"
#include "float16.h"
#include "gimprc.h"
#include "gximage.h"
#include "image_render.h"
#include "paint_funcs_area.h"
#include "pixelarea.h"
#include "pixelrow.h"
#include "scale.h"
#include "tag.h"
#include "trace.h"
#include "displaylut.h"

#define MAX_CHANNELS 4

#define ALL_INTEN_VISIBLE 0
#define NO_INTEN_VISIBLE 1
#define EXACTLY_ONE_INTEN_VISIBLE 2
#define TWO_OR_MORE_NOT_ALL_INTEN_VISIBLE 3  

#define EXACTLY_ONE_AUX_VISIBLE 0
#define NO_AUX_VISIBLE 1 
#define ONE_OR_MORE_AUX_VISIBLE 2 

TraceTimer trace_timer;

typedef struct _RenderInfo  RenderInfo;
typedef void (*RenderFunc) (RenderInfo *info);

struct _RenderInfo
{
  GDisplay *gdisp;
  Canvas *src_canvas;
  guint src_width;
  guint *alpha;
  guchar *scale;
  guchar *src;
  guchar *dest;
  int x, y;
  int w, h;
  int scalesrc;
  int scaledest;
  int src_x, src_y;
  int src_w, src_h;
  int src_bpp;
  int src_num_channels;
  int visible[MAX_CHANNELS];
  int intensity_visible;  
  int dest_bpp;
  int dest_bpl;
  int dest_width;
  GSList *channels;
  Channel *single_visible_channel;
  int aux_channels_visible;  
};


/*  accelerate transparency of image scaling  */
guchar *blend_dark_check = NULL;
guchar *blend_light_check = NULL;
guchar *tile_buf = NULL;
guchar *check_buf = NULL;
guchar *empty_buf = NULL;
guchar *temp_buf = NULL;

Canvas *comp_canvas = NULL;

static guint   check_mod;
static guint   check_shift;
static guchar  check_combos[6][2] =
{
  { 204, 255 },
  { 102, 153 },
  { 0, 51 },
  { 255, 255 },
  { 127, 127 },
  { 0, 0 }
};



void
render_setup (int check_type,
	      int check_size)
{
  int i, j;

  /*  allocate a buffer for arranging information from a row of tiles  */
  if (!tile_buf)
    tile_buf = g_new (guchar, GXIMAGE_WIDTH * MAX_CHANNELS * 4);
  
  if (check_type < 0 || check_type > 5)
    g_error ("invalid check_type argument to render_setup: %d", check_type);
  if (check_size < 0 || check_size > 2)
    g_error ("invalid check_size argument to render_setup: %d", check_size);

  if (!blend_dark_check)
    blend_dark_check = g_new (guchar, 65536);
  if (!blend_light_check)
    blend_light_check = g_new (guchar, 65536);

  for (i = 0; i < 256; i++)
    for (j = 0; j < 256; j++)
      {
	blend_dark_check [(i << 8) + j] = (guchar)
	  ((j * i + check_combos[check_type][0] * (255 - i)) / 255);
	blend_light_check [(i << 8) + j] = (guchar)
	  ((j * i + check_combos[check_type][1] * (255 - i)) / 255);
      }

  switch (check_size)
    {
    case SMALL_CHECKS:
      check_mod = 0x3;
      check_shift = 2;
      break;
    case MEDIUM_CHECKS:
      check_mod = 0x7;
      check_shift = 3;
      break;
    case LARGE_CHECKS:
      check_mod = 0xf;
      check_shift = 4;
      break;
    }

  /*  calculate check buffer for previews  */
  if (preview_size)
    {
      if (check_buf)
	g_free (check_buf);
      if (empty_buf)
	g_free (empty_buf);
      if (temp_buf)
	g_free (temp_buf);

      check_buf = (unsigned char *) g_malloc ((preview_size + 4) * 3);
      for (i = 0; i < (preview_size + 4); i++)
	{
	  if (i & 0x4)
	    {
	      check_buf[i * 3 + 0] = blend_dark_check[0];
	      check_buf[i * 3 + 1] = blend_dark_check[0];
	      check_buf[i * 3 + 2] = blend_dark_check[0];
	    }
	  else
	    {
	      check_buf[i * 3 + 0] = blend_light_check[0];
	      check_buf[i * 3 + 1] = blend_light_check[0];
	      check_buf[i * 3 + 2] = blend_light_check[0];
	    }
	}
      empty_buf = (unsigned char *) g_malloc ((preview_size + 4) * 3);
      memset (empty_buf, 0, (preview_size + 4) * 3);
      temp_buf = (unsigned char *) g_malloc ((preview_size + 4) * 3);
    }
  else
    {
      check_buf = NULL;
      empty_buf = NULL;
      temp_buf = NULL;
    }
}

void
render_free (void)
{
  g_free (tile_buf);
  g_free (check_buf);
}

/*  Render Image functions  */

static void    render_image_indexed   (RenderInfo *info);
static void    render_image_indexed_a (RenderInfo *info);
static void    render_image_gray      (RenderInfo *info);
static void    render_image_gray_a    (RenderInfo *info);
static void    render_image_rgb       (RenderInfo *info);
static void    render_image_rgb_a     (RenderInfo *info);

static void    render_image_gray_u16      (RenderInfo *info);
static void    render_image_gray_a_u16    (RenderInfo *info);
static void    render_image_rgb_u16       (RenderInfo *info);
static void    render_image_rgb_a_u16     (RenderInfo *info);

static void    render_image_gray_float      (RenderInfo *info);
static void    render_image_gray_a_float    (RenderInfo *info);
static void    render_image_rgb_float       (RenderInfo *info);
static void    render_image_rgb_a_float     (RenderInfo *info);

static void    render_image_gray_float16      (RenderInfo *info);
static void    render_image_gray_a_float16    (RenderInfo *info);
static void    render_image_rgb_float16       (RenderInfo *info);
static void    render_image_rgb_a_float16     (RenderInfo *info);

static void    render_image_init_info          (RenderInfo   *info,
						GDisplay     *gdisp,
						int           x,
						int           y,
						int           w,
						int           h);
static guint*  render_image_init_alpha         (gfloat        opacity);
static guchar* render_image_accelerate_scaling (int           width,
						int           start,
						int           bpp,
						int           scalesrc,
						int           scaledest);
static guchar* render_image_tile_fault         (RenderInfo   *info);
static guchar* render_image_tile_fault1        (RenderInfo   *info);


static RenderFunc render_funcs[6] =
{
  render_image_rgb,
  render_image_rgb_a,
  render_image_gray,
  render_image_gray_a,
  render_image_indexed,
  render_image_indexed_a,
};

static RenderFunc render_funcs_u16[4] =
{
  render_image_rgb_u16,
  render_image_rgb_a_u16,
  render_image_gray_u16,
  render_image_gray_a_u16,
};

static RenderFunc render_funcs_float[4] =
{
  render_image_rgb_float,
  render_image_rgb_a_float,
  render_image_gray_float,
  render_image_gray_a_float,
};

static RenderFunc render_funcs_float16[4] =
{
  render_image_rgb_float16,
  render_image_rgb_a_float16,
  render_image_gray_float16,
  render_image_gray_a_float16,
};

/*****************************************************************/
/*  This function is the core of the display--it offsets and     */
/*  scales the image according to the current parameters in the  */
/*  gdisp object.  It handles color, grayscale, 8, 15, 16, 24,   */
/*  & 32 bit output depths.                                      */
/*****************************************************************/

void
render_image (GDisplay *gdisp,
	      int       x,
	      int       y,
	      int       w,
	      int       h)
{
  RenderInfo info;
  Tag t;
  
  render_image_init_info (&info, gdisp, x, y, w, h);

  t = canvas_tag (info.src_canvas);

  /*allocate a row for aux channel compositing and intensity visibility */

  comp_canvas = canvas_new (t, info.src_w, 1, STORAGE_FLAT);

  switch (tag_format (t))
    {
    case FORMAT_RGB:
    case FORMAT_GRAY:
      break;

    case FORMAT_INDEXED:
      if (tag_precision (t) != PRECISION_U8)
        {
          g_warning ("indexed images only supported in 8 bit mode");
          return;
        }
        break;

    case FORMAT_NONE:
    default:
      g_warning ("unsupported gimage projection type");
      return;
    }

  if ((info.dest_bpp < 1) || (info.dest_bpp > 4))
    {
      g_message ("unsupported destination bytes per pixel: %d", info.dest_bpp);
      return;
    }
  
  {
    int image_type = tag_to_drawable_type (tag_set_precision (t, PRECISION_U8));
  
    switch (tag_precision (t))
      {
      case PRECISION_U8:
        (* render_funcs[image_type]) (&info);
        break;
      case PRECISION_U16:
        (* render_funcs_u16[image_type]) (&info);
  	break;
      case PRECISION_FLOAT:
        (* render_funcs_float[image_type]) (&info);
  	break;
      case PRECISION_FLOAT16:
        (* render_funcs_float16[image_type]) (&info);
  	break;
      case PRECISION_NONE:
        break;
      }
  }
  canvas_delete (comp_canvas); 
}



/*************************/
/*  8 Bit functions      */
/*************************/

static void
render_image_indexed (RenderInfo *info)
{
  guchar *src;
  guchar *dest;
  guchar *cmap;
  gulong val;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  cmap = gimage_cmap (info->gdisp->gimage);

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src = info->src;
	  dest = info->dest;

	  g_return_if_fail (src != NULL);
	  
	  for (x = info->x; x < xe; x++)
	    {
	      val = src[INDEXED_PIX] * 3;
	      src += 1;

	      dest[0] = cmap[val+0];
	      dest[1] = cmap[val+1];
	      dest[2] = cmap[val+2];
	      dest += 3;
	    }
	}

      info->dest += info->dest_bpl;
 
      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_indexed_a (RenderInfo *info)
{
  guchar *src;
  guchar *dest;
  guint *alpha;
  guchar *cmap;
  gulong r, g, b;
  gulong val;
  guint a;
  int dark_light;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  cmap = gimage_cmap (info->gdisp->gimage);
  alpha = info->alpha;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0) && (y & check_mod))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src = info->src;
	  dest = info->dest;

	  dark_light = (y >> check_shift) + (info->x >> check_shift);

	  g_return_if_fail (src != NULL);

	  for (x = info->x; x < xe; x++)
	    {
	      a = alpha[src[ALPHA_I_PIX]];
	      val = src[INDEXED_PIX] * 3;
	      src += 2;

	      if (dark_light & 0x1)
		{
		  r = blend_dark_check[(a | cmap[val+0])];
		  g = blend_dark_check[(a | cmap[val+1])];
		  b = blend_dark_check[(a | cmap[val+2])];
		}
	      else
		{
		  r = blend_light_check[(a | cmap[val+0])];
		  g = blend_light_check[(a | cmap[val+1])];
		  b = blend_light_check[(a | cmap[val+2])];
		}

	      dest[0] = r;
	      dest[1] = g;
	      dest[2] = b;
	      dest += 3;

	      if (((x + 1) & check_mod) == 0)
		dark_light += 1;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_gray (RenderInfo *info)
{
  guchar *src;
  guchar *dest;
  gulong val;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src = info->src;
	  dest = info->dest;
	  
	  g_return_if_fail (src != NULL);

	  for (x = info->x; x < xe; x++)
	    {
	      val = src[GRAY_PIX];
	      src += 1;

	      dest[0] = val;
	      dest[1] = val;
	      dest[2] = val;
	      dest += 3;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_gray_a (RenderInfo *info)
{
  guchar *src;
  guchar *dest;
  guint *alpha;
  gulong val;
  guint a;
  int dark_light;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  alpha = info->alpha;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0) && (y & check_mod))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src = info->src;
	  dest = info->dest;

	  dark_light = (y >> check_shift) + (info->x >> check_shift);

	  g_return_if_fail (src != NULL);

	  for (x = info->x; x < xe; x++)
	    {
	      a = alpha[src[ALPHA_G_PIX]];
	      if (dark_light & 0x1)
		val = blend_dark_check[(a | src[GRAY_PIX])];
	      else
		val = blend_light_check[(a | src[GRAY_PIX])];
	      src += 2;

	      dest[0] = val;
	      dest[1] = val;
	      dest[2] = val;
	      dest += 3;

	      if (((x + 1) & check_mod) == 0)
		dark_light += 1;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_rgb (RenderInfo *info)
{
  guchar *src;
  guchar *dest;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src = info->src;
	  dest = info->dest;

	  g_return_if_fail (src != NULL);
	  
	  /* replace this with memcpy, or better yet, avoid it altogether? */
	  for (x = info->x; x < xe; x++)
	    {
	      dest[0] = src[0];
	      dest[1] = src[1];
	      dest[2] = src[2];

	      src += 3;
	      dest += 3;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_rgb_a (RenderInfo *info)
{
  guchar *src;
  guchar *dest;
  guint *alpha;
  gulong r, g, b;
  guint a;
  int dark_light;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  alpha = info->alpha;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0) && (y & check_mod))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src = info->src;
	  dest = info->dest;

	  dark_light = (y >> check_shift) + (info->x >> check_shift);

	  g_return_if_fail (src != NULL);

	  for (x = info->x; x < xe; x++)
	    {
	      a = alpha[src[ALPHA_PIX]];
	      if (dark_light & 0x1)
		{
		  r = blend_dark_check[(a | src[RED_PIX])];
		  g = blend_dark_check[(a | src[GREEN_PIX])];
		  b = blend_dark_check[(a | src[BLUE_PIX])];
		}
	      else
		{
		  r = blend_light_check[(a | src[RED_PIX])];
		  g = blend_light_check[(a | src[GREEN_PIX])];
		  b = blend_light_check[(a | src[BLUE_PIX])];
		}

	      src += 4;

	      dest[0] = r;
	      dest[1] = g;
	      dest[2] = b;
	      dest += 3;

	      if (((x + 1) & check_mod) == 0)
		dark_light += 1;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_init_info (RenderInfo *info,
			GDisplay   *gdisp,
			int         x,
			int         y,
			int         w,
			int         h)
{
  gint i;
  Tag src_canvas_tag;
  gint num_inten, num_visible_inten; 
  gint num_visible_aux_chans; 
  Channel *single_visible_channel = NULL;
  GSList * list;

  info->gdisp = gdisp;
  info->x = x + gdisp->offset_x;
  info->y = y + gdisp->offset_y;
  info->w = w;
  info->h = h;
  info->scalesrc = SCALESRC (gdisp);
  info->scaledest = SCALEDEST (gdisp);
  info->src_x = UNSCALE (gdisp, info->x);
  info->src_y = UNSCALE (gdisp, info->y);
  info->src_w = UNSCALE (gdisp, (info->x + info->w)) - UNSCALE (gdisp, info->x) + 1;

  info->src_canvas = gimage_projection (gdisp->gimage);
  info->channels = gimage_channels (gdisp->gimage);
  info->src_width = canvas_width (info->src_canvas);
  src_canvas_tag = canvas_tag (info->src_canvas);
  info->src_bpp = tag_bytes (src_canvas_tag);
  info->src_num_channels = tag_num_channels (src_canvas_tag);

  if (tag_alpha (src_canvas_tag) == ALPHA_YES )
    num_inten = info->src_num_channels - 1;
  else
    num_inten = info->src_num_channels;

  num_visible_inten = 0; 
  for(i = 0; i < num_inten; i++)
    {
      info->visible[i] = gimage_get_component_visible (gdisp->gimage, i); 
      if(info->visible[i]) 
	num_visible_inten++; 
    }

  if (num_visible_inten == num_inten )
    info->intensity_visible = ALL_INTEN_VISIBLE;
  else if (num_visible_inten == 0)
    info->intensity_visible = NO_INTEN_VISIBLE;
  else if (num_visible_inten == 1)
    info->intensity_visible = EXACTLY_ONE_INTEN_VISIBLE;
  else
    info->intensity_visible = TWO_OR_MORE_NOT_ALL_INTEN_VISIBLE;

  list = info->channels;
  num_visible_aux_chans = 0; 
  while (list)
    {
      Channel *channel = (Channel *)(list->data);
      if (channel)
	{
	  gint visible = channel_visibility (channel);
	  if (visible)
	    {
	      single_visible_channel = channel;
	      num_visible_aux_chans++;  
	    }
	}
      list = g_slist_next (list);
    }

  if (num_visible_aux_chans == 0)
    info->aux_channels_visible = NO_AUX_VISIBLE;
  else if (num_visible_aux_chans == 1)
    info->aux_channels_visible = EXACTLY_ONE_AUX_VISIBLE;
  else if (num_visible_aux_chans >= 1)
    info->aux_channels_visible = ONE_OR_MORE_AUX_VISIBLE;

  if (num_visible_aux_chans == 1) 
    info->single_visible_channel = single_visible_channel;
  else
    info->single_visible_channel = NULL;

  info->dest = gximage_get_data ();
  info->dest_bpp = gximage_get_bpp ();
  info->dest_bpl = gximage_get_bpl ();
  info->dest_width = info->w * info->dest_bpp;

  info->scale = render_image_accelerate_scaling (w, info->x, info->src_bpp, info->scalesrc, info->scaledest);
  info->alpha = NULL;

  if (tag_alpha (canvas_tag (info->src_canvas)) == ALPHA_YES)
    info->alpha = render_image_init_alpha (gimage_projection_opacity (gdisp->gimage));
}

static guint*
render_image_init_alpha (gfloat mult)
{
  static guint *alpha_mult = NULL;
  static gfloat alpha_val = -1.0;
  int i;

  if (alpha_val != mult)
    {
      if (!alpha_mult)
	alpha_mult = g_new (guint, 256);

      alpha_val = mult;
      for (i = 0; i < 256; i++)
	alpha_mult[i] = ((i * (guint) (mult * 255)) / 255) << 8;
    }

  return alpha_mult;
}

static guchar*
render_image_accelerate_scaling (int width,
				 int start,
				 int  bpp,
				 int  scalesrc,
				 int  scaledest)
{
  static guchar *scale = NULL;
  static int swidth = -1;
  static int sstart = -1;
  guchar step;
  int i;

  if ((swidth != width) || (sstart != start))
    {
      if (!scale)
	scale = g_new (guchar, GXIMAGE_WIDTH + 1);

      step = scalesrc * bpp;

      for (i = 0; i <= width; i++)
	scale[i] = ((i + start + 1) % scaledest) ? 0 : step;
    }

  return scale;
}

void
set_visible_channels_row_u8(
             PixelRow * row,
	     gint *visible
             )
{
  gint    b;
  guint8 *data         = (guint8*) pixelrow_data (row);
  gint    num_channels = tag_num_channels (pixelrow_tag (row));
  gint    n;
  Alpha   alpha = tag_alpha (pixelrow_tag (row));
  gint    width        = pixelrow_width (row);  

  if (alpha == ALPHA_YES) 
    n = num_channels - 1; 
  else
    n = num_channels;

  while (width--)
    {
      for (b = 0; b < n; b++)
        data[b] =  visible[b] ? data[b] : 0;
      data += num_channels;
    }
}

void
set_visible_channels_row_u16(
             PixelRow * row,
	     gint *visible
             )
{
  gint    b;
  guint16 *data         = (guint16*) pixelrow_data (row);
  gint    num_channels = tag_num_channels (pixelrow_tag (row));
  gint    n;
  Alpha   alpha = tag_alpha (pixelrow_tag (row));
  gint    width        = pixelrow_width (row);  

  if (alpha == ALPHA_YES) 
    n = num_channels - 1; 
  else
    n = num_channels;

  while (width--)
    {
      for (b = 0; b < n; b++)
        data[b] =  visible[b] ? data[b] : 0;
      data += num_channels;
    }
}

void
set_visible_channels_row_float(
             PixelRow * row,
	     gint *visible
             )
{
  gint    b;
  gfloat *data         = (gfloat*) pixelrow_data (row);
  gint    num_channels = tag_num_channels (pixelrow_tag (row));
  gint    n;
  Alpha   alpha = tag_alpha (pixelrow_tag (row));
  gint    width        = pixelrow_width (row);  

  if (alpha == ALPHA_YES) 
    n = num_channels - 1; 
  else
    n = num_channels;

  while (width--)
    {
      for (b = 0; b < n; b++)
        data[b] =  visible[b] ? data[b] : 0.0;
      data += num_channels;
    }
}

void
set_visible_channels_row_float16(
             PixelRow * row,
	     gint *visible
             )
{
  gint    b;
  guint16 *data         = (guint16*) pixelrow_data (row);
  gint    num_channels = tag_num_channels (pixelrow_tag (row));
  gint    n;
  Alpha   alpha = tag_alpha (pixelrow_tag (row));
  gint    width        = pixelrow_width (row);  

  if (alpha == ALPHA_YES) 
    n = num_channels - 1; 
  else
    n = num_channels;

  while (width--)
    {
      for (b = 0; b < n; b++)
        data[b] =  visible[b] ? data[b] : ZERO_FLOAT16;
      data += num_channels;
    }
}

typedef void (*SetVisibleChannelsFunc) (PixelRow*, gint*);
static SetVisibleChannelsFunc 
set_visible_channels_func (
                   Tag tag
                   )
{
  switch (tag_precision (tag))
    {
    case PRECISION_U8:
      return set_visible_channels_row_u8;
    case PRECISION_U16:
      return set_visible_channels_row_u16;
    case PRECISION_FLOAT16:
      return set_visible_channels_row_float16;
    case PRECISION_FLOAT:
      return set_visible_channels_row_float;
    case PRECISION_NONE:
    default:
      g_warning ("set_visible_channels_func: bad precision");
    }
  return NULL;
}

#if 0
      pixelarea_init (&aux_channel_area, aux_channel_canvas, src_x, src_y, src_w, 1, FALSE);
      pixelarea_init (&comp_area, comp_canvas, 0, 0, src_w, 1, FALSE); 
      printf("aux_channel before\n");
      print_area(&aux_channel_area,0,1); 
      printf("comp_area before\n");
      print_area(&comp_area,0,1); 
#endif
#if 0
      pixelarea_init (&aux_channel_area, aux_channel_canvas, src_x, src_y, src_w, 1, FALSE);
      pixelarea_init (&comp_area, comp_canvas, 0, 0, src_w, 1, FALSE); 
      printf("aux_channel after\n");
      print_area(&aux_channel_area,0,1); 
      printf("comp_area after\n");
      print_area(&comp_area,0,1); 
#define ALL_INTEN_VISIBLE 0
#define NO_INTEN_VISIBLE 1
#define EXACTLY_ONE_INTEN_VISIBLE 2
#define TWO_OR_MORE_NOT_ALL_INTEN_VISIBLE 3  

#define EXACTLY_ONE_AUX_VISIBLE 0
#define NO_AUX_VISIBLE 1 
#define ONE_OR_MORE_AUX_VISIBLE 2 
#endif

#if 1
static guchar*
render_image_tile_fault (RenderInfo *info)
{
  PixelArea  area, comp_area;
  PixelRow comp_row;
  Tag tag = canvas_tag (info->src_canvas);
  guchar *data, *dest, *scale;
  gint src_x, src_y, src_w;
  gint width;
  void * pag;
  gint bytes = tag_bytes (tag); 
  gint b, step;
  GSList *list;
  src_x = info->src_x;
  src_y = info->src_y;
  src_w = info->src_w;
  width = info->w;

  if (info->intensity_visible == NO_INTEN_VISIBLE && 
	info->single_visible_channel) 
  {
      Canvas *aux_channel_canvas;
      aux_channel_canvas = drawable_data ( GIMP_DRAWABLE (info->single_visible_channel) );
      pixelarea_init (&area, aux_channel_canvas, src_x, src_y, src_w, 1, FALSE); 
      pixelarea_init (&comp_area, comp_canvas, 0, 0, src_w, 1, TRUE); 
      copy_area (&area, &comp_area); 
  }
  else 
  {
	/* if some RGB is visible */
	if (info->intensity_visible != NO_INTEN_VISIBLE) 
	{
	  if (info->intensity_visible == ALL_INTEN_VISIBLE &&
		info->aux_channels_visible == NO_AUX_VISIBLE)
	    return render_image_tile_fault1 (info);
	  
	  pixelarea_init (&area, info->src_canvas, src_x, src_y, src_w, 1, FALSE); 
	  pixelarea_init (&comp_area, comp_canvas, 0, 0, src_w, 1, TRUE); 
	  copy_area (&area, &comp_area); 

	  if (info->intensity_visible != ALL_INTEN_VISIBLE)
	    {	
	      pixelarea_init (&comp_area, comp_canvas, 0, 0, src_w, 1, TRUE); 
	      pag = pixelarea_register (1, &comp_area);
	      pixelarea_getdata (&comp_area, &comp_row, 0);
	      (*set_visible_channels_func (tag)) (&comp_row, info->visible);
	      pixelarea_process_stop (pag);
	    }
	}
	else    /* NO rgb visible, 0 or 2 or more aux channels visible */
	{
	  /* just fill the comp_canvas with black */
	}
       
       /* now if there are any aux channels visible composite them in */
	if (info->aux_channels_visible != NO_AUX_VISIBLE)
	{	
	  list = info->channels;
	  while (list)
	    {
	      Channel *channel = (Channel *)(list->data);
	      gint visible = channel_visibility (channel);
	      PixelRow *color;
	      PixelArea aux_channel_area;
	      gfloat opacity;
	      Canvas *aux_channel_canvas;
	  
	      if (visible)
	      {
		aux_channel_canvas = drawable_data ( GIMP_DRAWABLE (channel) );
		pixelarea_init (&aux_channel_area, aux_channel_canvas, src_x, src_y, src_w, 1, FALSE);
		pixelarea_init (&comp_area, comp_canvas, 0, 0, src_w, 1, TRUE); 
		color = channel_color (channel);
		opacity = channel_opacity (channel);
		
#if 0 
		combine_areas (&comp_area, &aux_channel_area, &comp_area, NULL, 
		  (unsigned char*)color, opacity, 
		   NORMAL_MODE, NULL, COMBINE_INTEN_A_CHANNEL_MASK);
#endif
#if 1 
		combine_areas (&comp_area, &aux_channel_area, &comp_area, NULL, 
		  (unsigned char*)color, opacity, 
		   NORMAL_MODE, NULL, COMBINE_INTEN_A_CHANNEL_SELECTION);
#endif
	      }
	      list = g_slist_next (list);
    	     }
	
        }
  }

  dest = tile_buf;
  scale = info->scale;
  step = info->scalesrc * info->src_bpp;

  pixelarea_init (&comp_area, comp_canvas, 0, 0, src_w, 1, FALSE); 
  pag = pixelarea_register (1, &comp_area);
  data = pixelarea_data (&comp_area);
  while (width--)
    {
      for (b = 0; b < bytes; b++)
	 *dest++ = data[b];

      if (*scale++ != 0)
	  data += step;
    }

  pixelarea_process_stop(pag);
  return tile_buf;
}
#endif

#if 1
static guchar*
render_image_tile_fault1 (RenderInfo *info)
{
  guchar *data;
  guchar *dest;
  guchar *scale;
  int width;
  int x_portion;
  int y_portion;
  int portion_width;
  int step;
  int x, b;
  
  /* the first portion x and y */
  x_portion = info->src_x;
  y_portion = info->src_y;
  
  /* fault in the first portion */ 
  canvas_portion_refro (info->src_canvas, x_portion, y_portion); 
  data = canvas_portion_data (info->src_canvas, info->src_x, info->src_y);
  if (!data)
    {
      canvas_portion_unref( info->src_canvas, x_portion, y_portion); 
      return NULL;
    }
  scale = info->scale;
  step = info->scalesrc * info->src_bpp;
  dest = tile_buf;
  
  /* the first portions width */ 
  portion_width = canvas_portion_width ( info->src_canvas, 
                                         x_portion,
                                         y_portion );
  x = info->src_x;
  width = info->w;

  while (width--)
    {
      for (b = 0; b < info->src_bpp; b++)
	*dest++ = data[b];

      if (*scale++ != 0)
	{
	  x += info->scalesrc;
	  data += step;

	  if (x >= x_portion + portion_width)
	    {
	      canvas_portion_unref (info->src_canvas, x_portion, y_portion);
              if (x >= info->src_width)
		return tile_buf;
	      x_portion += portion_width;
              canvas_portion_refro (info->src_canvas, x_portion, y_portion ); 
              data = canvas_portion_data (info->src_canvas, x_portion, y_portion );
              if(!data)
                {
                  canvas_portion_unref (info->src_canvas, x_portion, y_portion ); 
                  return NULL;
                }
	      portion_width = canvas_portion_width ( info->src_canvas, 
                                              x_portion,
                                              y_portion );
	    }
	}
    }
  canvas_portion_unref (info->src_canvas, x_portion, y_portion);
  return tile_buf;
}
#endif

/*************************/
/*  16 Bit channel data functions */
/*************************/
#define U16_TO_U8(x)  ((x)>>8)

static void
render_image_gray_u16 (RenderInfo *info)
{
  guint16 *src;
  guchar *dest;
  gulong val;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src =(guint16*)info->src;
	  dest = info->dest;

	  for (x = info->x; x < xe; x++)
	    {
	      val = U16_TO_U8(src[GRAY_PIX]);
	      src += 1;

	      dest[0] = val;
	      dest[1] = val;
	      dest[2] = val;
	      dest += 3;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_gray_a_u16 (RenderInfo *info)
{
  guint16 *src;
  guchar *dest;
  guint *alpha;
  gulong val;
  guint a;
  int dark_light;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  alpha = info->alpha;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0) && (y & check_mod))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src =(guint16*) info->src;
	  dest = info->dest;

	  dark_light = (y >> check_shift) + (info->x >> check_shift);

	  for (x = info->x; x < xe; x++)
	    {
	      a = alpha[U16_TO_U8(src[ALPHA_G_PIX])];
	      if (dark_light & 0x1)
		val = blend_dark_check[(a | U16_TO_U8(src[GRAY_PIX]))];
	      else
		val = blend_light_check[(a | U16_TO_U8(src[GRAY_PIX]))];
	      src += 2;

	      dest[0] = val;
	      dest[1] = val;
	      dest[2] = val;
	      dest += 3;

	      if (((x + 1) & check_mod) == 0)
		dark_light += 1;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_rgb_u16 (RenderInfo *info)
{
  guint16 *src;
  guchar *dest;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src =(guint16*) info->src;
	  dest = info->dest;

	  for (x = info->x; x < xe; x++)
	    {
	      dest[0] = U16_TO_U8(src[0]);
	      dest[1] = U16_TO_U8(src[1]);
	      dest[2] = U16_TO_U8(src[2]);

	      src += 3;
	      dest += 3;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_rgb_a_u16 (RenderInfo *info)
{
  guint16 *src;
  guchar *dest;
  guint *alpha;
  gulong r, g, b;
  guint a;
  int dark_light;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  alpha = info->alpha;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0) && (y & check_mod))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src = (guint16 *)info->src;
	  dest = info->dest;

	  dark_light = (y >> check_shift) + (info->x >> check_shift);

	  for (x = info->x; x < xe; x++)
	    {
	      a = alpha[U16_TO_U8(src[ALPHA_PIX])];
	      if (dark_light & 0x1)
		{
		  r = blend_dark_check[(a | U16_TO_U8(src[RED_PIX]))];
		  g = blend_dark_check[(a | U16_TO_U8(src[GREEN_PIX]))];
		  b = blend_dark_check[(a | U16_TO_U8(src[BLUE_PIX]))];
		}
	      else
		{
		  r = blend_light_check[(a | U16_TO_U8(src[RED_PIX]))];
		  g = blend_light_check[(a | U16_TO_U8(src[GREEN_PIX]))];
		  b = blend_light_check[(a | U16_TO_U8(src[BLUE_PIX]))];
		}

	      src += 4;

	      dest[0] = r;
	      dest[1] = g;
	      dest[2] = b;
	      dest += 3;

	      if (((x + 1) & check_mod) == 0)
		dark_light += 1;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

/*************************/
/*  Float channel data functions */
/*************************/
#define FLOAT_TO_8BIT( x ) (int)( 255*(x) + .5 ) 

static void
render_image_gray_float (RenderInfo *info)
{
  gfloat *src;
  gint src8bit;
  guchar *dest;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src =(gfloat*)info->src;
	  dest = info->dest;

	  for (x = info->x; x < xe; x++)
	    {
	      src8bit = FLOAT_TO_8BIT( src[GRAY_PIX] );
	      src += 1;

	      dest[0] = src8bit;
	      dest[1] = src8bit;
	      dest[2] = src8bit;
	      dest += 3;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_gray_a_float (RenderInfo *info)
{
  gfloat *src;
  guchar *dest;
  guint *alpha;
  gulong val;
  guint a;
  int dark_light;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  alpha = info->alpha;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0) && (y & check_mod))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src =(gfloat*) info->src;
	  dest = info->dest;

	  dark_light = (y >> check_shift) + (info->x >> check_shift);

	  for (x = info->x; x < xe; x++)
	    {
	      a = alpha[ FLOAT_TO_8BIT(src[ALPHA_G_PIX]) ];
	      if (dark_light & 0x1)
		val = blend_dark_check[(a | FLOAT_TO_8BIT(src[GRAY_PIX]))];
	      else
		val = blend_light_check[(a | FLOAT_TO_8BIT(src[GRAY_PIX]))];
	      src += 2;

	      dest[0] = val;
	      dest[1] = val;
	      dest[2] = val;
	      dest += 3;

	      if (((x + 1) & check_mod) == 0)
		  dark_light += 1;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

void
render_image_rgb_float (RenderInfo *info)
{
  gfloat *src;
  guchar *dest;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src =(gfloat*) info->src;
	  dest = info->dest;

	  for (x = info->x; x < xe; x++)
	    {
	      dest[0] = FLOAT_TO_8BIT(src[0]);
	      dest[1] = FLOAT_TO_8BIT(src[1]);
	      dest[2] = FLOAT_TO_8BIT(src[2]);

	      src += 3;
	      dest += 3;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_rgb_a_float (RenderInfo *info)
{
  gfloat *src;
  guchar *dest;
  guint *alpha;
  gulong r, g, b;
  guint a;
  int dark_light;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  alpha = info->alpha;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0) && (y & check_mod))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src = (gfloat *)info->src;
	  dest = info->dest;

	  dark_light = (y >> check_shift) + (info->x >> check_shift);

	  for (x = info->x; x < xe; x++)
	    {
	      a = alpha[FLOAT_TO_8BIT(src[ALPHA_PIX])];
	      if (dark_light & 0x1)
		{
		  r = blend_dark_check[(a | FLOAT_TO_8BIT(src[RED_PIX]))];
		  g = blend_dark_check[(a | FLOAT_TO_8BIT(src[GREEN_PIX]))];
		  b = blend_dark_check[(a | FLOAT_TO_8BIT(src[BLUE_PIX]))];
		}
	      else
		{
		  r = blend_light_check[(a | FLOAT_TO_8BIT(src[RED_PIX]))];
		  g = blend_light_check[(a | FLOAT_TO_8BIT(src[GREEN_PIX]))];
		  b = blend_light_check[(a | FLOAT_TO_8BIT(src[BLUE_PIX]))];
		}

	      src += 4;

	      dest[0] = r;
	      dest[1] = g;
	      dest[2] = b;
	      dest += 3;

	      if (((x + 1) & check_mod) == 0)
		dark_light += 1;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

/*************************/
/* 16bit float channel data functions */
/*************************/

static void
render_image_gray_float16 (RenderInfo *info)
{
  guint16 *src;
  gint src8bit;
  guchar *dest;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src =(guint16*)info->src;
	  dest = info->dest;

	  for (x = info->x; x < xe; x++)
	    {
	      src8bit = display_u8_from_float16(src[GRAY_PIX]) ;
	      src += 1;

	      dest[0] = src8bit;
	      dest[1] = src8bit;
	      dest[2] = src8bit;
	      dest += 3;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_gray_a_float16 (RenderInfo *info)
{
  guint8 src8bit;
  guint16 *src;
  guchar *dest;
  guint *alpha;
  gulong val;
  guint a;
  int dark_light;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  alpha = info->alpha;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0) && (y & check_mod))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src =(guint16*) info->src;
	  dest = info->dest;

	  dark_light = (y >> check_shift) + (info->x >> check_shift);

	  for (x = info->x; x < xe; x++)
	    {
	      src8bit = display_u8_from_float16 (src[GRAY_PIX]); 
	      a = alpha[ display_u8_alpha_from_float16 (src[ALPHA_G_PIX]) ];
	      if (dark_light & 0x1)
		val = blend_dark_check[(a | src8bit)];
	      else
		val = blend_light_check[(a | src8bit)];
	      src += 2;

	      dest[0] = val;
	      dest[1] = val;
	      dest[2] = val;
	      dest += 3;

	      if (((x + 1) & check_mod) == 0)
		dark_light += 1;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_rgb_float16 (RenderInfo *info)
{
  guint16 *src;
  guchar *dest;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src =(guint16*) info->src;
	  dest = info->dest;

	  for (x = info->x; x < xe; x++)
	    {
	      dest[0] = display_u8_from_float16 (src[RED_PIX]);
	      dest[1] = display_u8_from_float16 (src[GREEN_PIX]);
	      dest[2] = display_u8_from_float16 (src[BLUE_PIX]);

	      src += 3;
	      dest += 3;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_rgb_a_float16 (RenderInfo *info)
{
  guint16 *src;
  guchar *dest;
  guint *alpha;
  gulong r, g, b;
  guint a;
  int dark_light;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  alpha = info->alpha;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0) && (y & check_mod))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src = (guint16 *)info->src;
	  dest = info->dest;

	  dark_light = (y >> check_shift) + (info->x >> check_shift);

	  for (x = info->x; x < xe; x++)
	    {
	      a = alpha[ display_u8_alpha_from_float16 (src[ALPHA_PIX]) ];
	      r = display_u8_from_float16 (src[RED_PIX]);
    	      g = display_u8_from_float16 (src[GREEN_PIX]);
	      b = display_u8_from_float16 (src[BLUE_PIX]);
	      if (dark_light & 0x1)
		{
		  r = blend_dark_check[a | r];
		  g = blend_dark_check[a | g];
		  b = blend_dark_check[a | b];
		}
	      else
		{
		  r = blend_light_check[a | r];
		  g = blend_light_check[a | g];
		  b = blend_light_check[a | b];
		}

	      src += 4;

	      dest[0] = r;
	      dest[1] = g;
	      dest[2] = b; 
	      dest += 3;

	      if (((x + 1) & check_mod) == 0)
		dark_light += 1;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}
