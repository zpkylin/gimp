/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp_brush_generated module Copyright 1998 Jay Cox <jaycox@earthlink.net>
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

#include "config.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef HAVE_RINT
#define rint(x) floor (x + 0.5)
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "appenv.h"
#include "gimpbrushgenerated.h"
#include "gimpbrushlist.h"
#include "float16.h"
#include "paint_core_16.h"
#include "gimprc.h"

#define OVERSAMPLING 5

double
smoothstep(double a, double b, double x);

static void
gimp_brush_generated_generate(GimpBrushGenerated *brush);


typedef void  
(*GimpBrushGeneratedComputeFunc) (GimpBrushGenerated *brush, 	
				gint width, gint height, gfloat *lookup);
static void 
gimp_brush_generated_compute_u8(GimpBrushGenerated *brush, 	
				gint width, gint height, gfloat *lookup);
static void 
gimp_brush_generated_compute_u16(GimpBrushGenerated *brush, 	
				gint width, gint height, gfloat *lookup);
static void 
gimp_brush_generated_compute_float(GimpBrushGenerated *brush, 	
				gint width, gint height, gfloat *lookup);
static void 
gimp_brush_generated_compute_float16(GimpBrushGenerated *brush, 	
				gint width, gint height, gfloat *lookup);

static GimpBrushGeneratedComputeFunc gimp_brush_generated_compute_funcs (Tag t);

static GimpObjectClass* parent_class;

static void
gimp_brush_generated_destroy(GtkObject *object)
{
  GTK_OBJECT_CLASS(parent_class)->destroy (object);
}

static void
gimp_brush_generated_class_init (GimpBrushGeneratedClass *klass)
{
  GtkObjectClass *object_class;
  
  object_class = GTK_OBJECT_CLASS(klass);

  parent_class = gtk_type_class (GIMP_TYPE_BRUSH);
  object_class->destroy =  gimp_brush_generated_destroy;
}


void
gimp_brush_generated_init(GimpBrushGenerated *brush)
{
  brush->radius        = 5.0;
  brush->hardness      = 0.0;
  brush->angle         = 0.0;
  brush->aspect_ratio  = 1.0;
  brush->freeze        = 0;
}

guint gimp_brush_generated_get_type(void)
{
  static GtkType type;
  if(!type){
    GtkTypeInfo info={
      "GimpBrushGenerated",
      sizeof(GimpBrushGenerated),
      sizeof(GimpBrushGeneratedClass),
      (GtkClassInitFunc)gimp_brush_generated_class_init,
      (GtkObjectInitFunc)gimp_brush_generated_init,
      NULL,
      NULL };
    type=gtk_type_unique(GIMP_TYPE_BRUSH, &info);
  }
  return type;
}

GimpBrushGenerated *gimp_brush_generated_new(float radius, float hardness,
				    float angle, float aspect_ratio)
{
  GimpBrushGenerated *brush;

/* set up normal brush data */
  brush = GIMP_BRUSH_GENERATED(gimp_type_new(gimp_brush_generated_get_type ()));

  GIMP_BRUSH(brush)->name = g_strdup ("Untitled");
  GIMP_BRUSH(brush)->spacing = 20;

/* set up gimp_brush_generated data */
  brush->radius = radius;
  brush->hardness = hardness;
  brush->angle = angle;
  brush->aspect_ratio = aspect_ratio;

 /* render brush mask */
  gimp_brush_generated_generate(brush);

  return brush;
}

GimpBrushGenerated *
gimp_brush_generated_load (const char *file_name)
{
  GimpBrushGenerated *brush;
  FILE *fp;
  char string[256];
  float fl;
  float version;
  if ((fp = fopen(file_name, "rb")) == NULL)
    return NULL;

/* make sure the file we are reading is the right type */
  fscanf(fp, "%8s", string);
  g_return_val_if_fail(strcmp(string, "GIMP-VBR") == 0, NULL);
/* make sure we are reading a compatible version */
  fscanf(fp, "%f", &version);
  g_return_val_if_fail(version < 2.0, NULL);

/* create new brush */
  brush = GIMP_BRUSH_GENERATED(gimp_type_new(gimp_brush_generated_get_type()));
  GIMP_BRUSH(brush)->filename = g_strdup (file_name);
  gimp_brush_generated_freeze(brush);

/* read name */
  fscanf(fp, "%255s", string);
  GIMP_BRUSH(brush)->name = g_strdup (string);
/* read brush spacing */
  fscanf(fp, "%f", &fl);
  GIMP_BRUSH(brush)->spacing = fl;
/* read brush radius */
  fscanf(fp, "%f", &fl);
  gimp_brush_generated_set_radius(brush, fl);
/* read brush hardness */
  fscanf(fp, "%f", &fl);
  gimp_brush_generated_set_hardness(brush, fl);
/* read brush aspect_ratio */
  fscanf(fp, "%f", &fl);
  gimp_brush_generated_set_aspect_ratio(brush, fl);
/* read brush angle */
  fscanf(fp, "%f", &fl);
  gimp_brush_generated_set_angle(brush, fl);

  fclose(fp);

  gimp_brush_generated_thaw(brush);

#if 1 
  {
    char * basename = strrchr (file_name, '/' );
    if (basename) 
      {
	basename++; 
	if (strcmp(default_brush, basename) == 0) 
	  {
	    select_brush (brush);
	    set_have_default_brush (TRUE); 
	  }
      } 
  }
#endif

  return brush;
}

void
gimp_brush_generated_save (GimpBrushGenerated *brush, 
			  const char *file_name)
{
  FILE *fp;
  if ((fp = fopen(file_name, "wb")) == NULL)
  {
    g_warning("Unable to save file %s", file_name);
    return;
  }
/* write magic header */
  fprintf(fp, "GIMP-VBR\n");
/* write version */
  fprintf(fp, "1.0\n");
/* write name */
  fprintf(fp, "%s\n", GIMP_BRUSH(brush)->name);
/* write brush spacing */
  fprintf(fp, "%f\n", (float)GIMP_BRUSH(brush)->spacing);
/* write brush radius */
  fprintf(fp, "%f\n", brush->radius);
/* write brush hardness */
  fprintf(fp, "%f\n", brush->hardness);
/* write brush aspect_ratio */
  fprintf(fp, "%f\n", brush->aspect_ratio);
/* write brush angle */
  fprintf(fp, "%f\n", brush->angle);

  fclose(fp);
}

void
gimp_brush_generated_freeze(GimpBrushGenerated *brush)
{
  g_return_if_fail (brush != NULL);
  g_return_if_fail (GIMP_IS_BRUSH_GENERATED (brush));
  
  brush->freeze++;
}

void
gimp_brush_generated_thaw(GimpBrushGenerated *brush)
{
  g_return_if_fail (brush != NULL);
  g_return_if_fail (GIMP_IS_BRUSH_GENERATED (brush));
  
  if (brush->freeze > 0)
    brush->freeze--;
  if (brush->freeze == 0)
    gimp_brush_generated_generate(brush);
}

double
smoothstep(double a, double b, double x)
{
    if (x < a)
        return 0;
    if (x >= b)
        return 1;
    x = (x - a)/(b - a); /* normalize to [0:1] */
    return (x*x * (3 - 2*x));
}

static
double gauss(double f)
{ 
    /* this aint' a real gauss function */
    if (f < -.5)
    {
      f = -1.0-f;
      return (2.0*f*f);
    }
    if (f < .5)
      return (1.0-2.0*f*f);
    f = 1.0 -f;
    return (2.0*f*f);
}

void
gimp_brush_generated_generate(GimpBrushGenerated *brush)
{
  register GimpBrush *gbrush = NULL;
  register int x, y;
  register double exponent;
  register float a;
  register int length;
  register float *lookup;
  double buffer[OVERSAMPLING];
  gdouble d;
  register double sum, c, s, tx, ty;
  int width, height;
  guchar *data;
  Tag tag;

  g_return_if_fail (brush != NULL);
  g_return_if_fail (GIMP_IS_BRUSH_GENERATED (brush));
  
  if (brush->freeze) /* if we are frozen defer rerendering till later */
    return;

  gbrush = GIMP_BRUSH(brush);

  if (gbrush->mask)
  {
    canvas_delete(gbrush->mask);
  }
  /* compute the range of the brush. should do a better job than this? */
  s = sin(brush->angle*M_PI/180.0);
  c = cos(brush->angle*M_PI/180.0);
  tx = MAXIMUM(fabs(c*ceil(brush->radius) - s*ceil(brush->radius)
		    /brush->aspect_ratio), 
	       fabs(c*ceil(brush->radius) + s*ceil(brush->radius)
		    /brush->aspect_ratio));
  ty = MAXIMUM(fabs(s*ceil(brush->radius) + c*ceil(brush->radius)
		    /brush->aspect_ratio),
	       fabs(s*ceil(brush->radius) - c*ceil(brush->radius)
		    /brush->aspect_ratio));
  if (brush->radius > tx)
    width = ceil(tx);
  else
    width = ceil(brush->radius);
  if (brush->radius > ty)
    height = ceil(ty);
  else
    height = ceil(brush->radius);

  /* compute the axis for spacing */
  GIMP_BRUSH(brush)->x_axis.x =      c*brush->radius;
  GIMP_BRUSH(brush)->x_axis.y = -1.0*s*brush->radius;

  GIMP_BRUSH(brush)->y_axis.x = (s*brush->radius / brush->aspect_ratio);
  GIMP_BRUSH(brush)->y_axis.y = (c*brush->radius / brush->aspect_ratio);
 

  if ((1.0 - brush->hardness) < 0.000001)
    exponent = 1000000; 
  else
    exponent = 1/(1.0 - brush->hardness);

/* set up lookup table */
  length = ceil(sqrt(2*ceil(brush->radius+1)*ceil(brush->radius+1))+1) * OVERSAMPLING;
  lookup = g_malloc(length * sizeof(float));
  sum = 0.0;
  for (x = 0; x < OVERSAMPLING; x++)
  {
    d = fabs((x+.5)/OVERSAMPLING - .5);
    if (d > brush->radius)
      buffer[x] = 0.0;
    else
/*      buffer[x] =  (1.0 - pow(d/brush->radius, exponent));*/
/*      buffer[x] =  gauss(pow(d/brush->radius, exponent));*/
	buffer[x] = 1.0 - smoothstep( brush->hardness, 1.0,d/brush->radius);
    sum += buffer[x];
  }
  for (x = 0; d < brush->radius || sum > .00001; d += 1.0/OVERSAMPLING)
  {
    sum -= buffer[x%OVERSAMPLING];
    if (d > brush->radius)
      buffer[x%OVERSAMPLING] = 0.0;
    else
/*      buffer[x%OVERSAMPLING] =  (1.0 - pow(d/brush->radius, exponent));*/
/*      buffer[x%OVERSAMPLING] =  gauss(pow(d/brush->radius, exponent));*/
	buffer[x%OVERSAMPLING] = 1.0 - smoothstep( brush->hardness, 1.0 ,d/brush->radius);
    sum += buffer[x%OVERSAMPLING];
    lookup[x++] =  sum/(float)OVERSAMPLING;
  }
  while (x < length)
  {
    lookup[x++] = 0.0;
  }

  
  tag = tag_new ( default_precision, FORMAT_GRAY, ALPHA_NO ); 
  (*gimp_brush_generated_compute_funcs(tag))(brush, width, height, lookup);

  g_free (lookup);
  gtk_signal_emit_by_name(GTK_OBJECT(brush), "dirty");
/*  brush_changed_notify(GIMP_BRUSH(brush)); */
}

static GimpBrushGeneratedComputeFunc 
gimp_brush_generated_compute_funcs (
                   Tag tag
                   )
{
  switch (tag_precision (tag))
    {
    case PRECISION_U8:
      return gimp_brush_generated_compute_u8;
    case PRECISION_U16:
      return gimp_brush_generated_compute_u16;
    case PRECISION_FLOAT:
      return gimp_brush_generated_compute_float;
    case PRECISION_FLOAT16:
      return gimp_brush_generated_compute_float16;
    case PRECISION_NONE:
    default:
      g_warning ("gimp_brush_generated_compute_func: bad precision");
    }
  return NULL;
}

static void
gimp_brush_generated_compute_float16( 
				GimpBrushGenerated *brush,
				gint width,
				gint height,
			  	gfloat *lookup	 
				)
{
  GimpBrush *gbrush = GIMP_BRUSH(brush);
  guint16 *centerp;
  gdouble a, d, c, s, tx, ty;
  gint x,y;
  gint canvas_w, canvas_h;
  guint16 *data;
  guint16 val;
  gint w = width;
  gint h = height;
  Tag tag = tag_new ( default_precision, FORMAT_GRAY, ALPHA_NO ); 
  ShortsFloat u;

  /* make the canvas */
  gbrush->mask = canvas_new( tag, width*2 + 1, height*2 + 1, STORAGE_FLAT);
  canvas_portion_refrw (gbrush->mask,0,0);
  data = (guint16*)canvas_portion_data (gbrush->mask, 0, 0);  
  canvas_w = canvas_width (gbrush->mask); 
  canvas_h = canvas_height (gbrush->mask); 
  centerp = &data[height*canvas_w + width];
	
  s = sin(brush->angle*M_PI/180.0);
  c = cos(brush->angle*M_PI/180.0);

  /* compute one half and mirror it */
  for (y = 0; y <= h; y++)
  {
    for (x = -w; x <= w; x++)
    {
      tx = c*x - s*y;
      ty = c*y + s*x;
      ty *= brush->aspect_ratio;
      d = sqrt(tx*tx + ty*ty);
      if (d < brush->radius+1)
	a = lookup[(int)rint(d*OVERSAMPLING)];
      else
	a = 0.0;
      a = CLAMP (a, 0, 1.0);
      val = FLT16 (a,u);
      centerp[   y*canvas_w + x] = val;
      centerp[-1*y*canvas_w - x] = val;
    }
  }
  canvas_portion_unref (gbrush->mask, 0, 0);
}

static void
gimp_brush_generated_compute_float( 
				GimpBrushGenerated *brush,
				gint width,
				gint height,
			  	gfloat *lookup	 
				)
{
  GimpBrush *gbrush = GIMP_BRUSH(brush);
  gfloat *centerp;
  gdouble a, d, c, s, tx, ty;
  gint x,y;
  gint canvas_w, canvas_h;
  gfloat *data;
  gint w = width;
  gint h = height;
  Tag tag = tag_new ( default_precision, FORMAT_GRAY, ALPHA_NO ); 

  /* make the canvas */
  gbrush->mask = canvas_new( tag, width*2 + 1, height*2 + 1, STORAGE_FLAT);
  canvas_portion_refrw (gbrush->mask,0,0);
  data = (gfloat*)canvas_portion_data (gbrush->mask, 0, 0);  
  canvas_w = canvas_width (gbrush->mask); 
  canvas_h = canvas_height (gbrush->mask); 
  centerp = &data[height*canvas_w + width];
	
  s = sin(brush->angle*M_PI/180.0);
  c = cos(brush->angle*M_PI/180.0);

  /* compute one half and mirror it */
  for (y = 0; y <= h; y++)
  {
    for (x = -w; x <= w; x++)
    {
      tx = c*x - s*y;
      ty = c*y + s*x;
      ty *= brush->aspect_ratio;
      d = sqrt(tx*tx + ty*ty);
      if (d < brush->radius+1)
	a = lookup[(int)rint(d*OVERSAMPLING)];
      else
	a = 0.0;
      a = CLAMP (a, 0, 1.0);
      centerp[   y*canvas_w + x] = a;
      centerp[-1*y*canvas_w - x] = a;
    }
  }
  canvas_portion_unref (gbrush->mask, 0, 0);
}

static void
gimp_brush_generated_compute_u16( 
				GimpBrushGenerated *brush,
				gint width,
				gint height,
			  	gfloat *lookup	 
				)
{
  GimpBrush *gbrush = GIMP_BRUSH(brush);
  guint16 *centerp;
  gdouble a, d, c, s, tx, ty;
  gint x,y;
  gint canvas_w, canvas_h;
  guint16 *data;
  gint w = width;
  gint h = height;
  Tag tag = tag_new ( default_precision, FORMAT_GRAY, ALPHA_NO ); 

  /* make the canvas */
  gbrush->mask = canvas_new( tag, width*2 + 1, height*2 + 1, STORAGE_FLAT);
  canvas_portion_refrw (gbrush->mask,0,0);
  data = (guint16*)canvas_portion_data (gbrush->mask, 0, 0);  
  canvas_w = canvas_width (gbrush->mask); 
  canvas_h = canvas_height (gbrush->mask); 
  centerp = &data[height*canvas_w + width];
	
  s = sin(brush->angle*M_PI/180.0);
  c = cos(brush->angle*M_PI/180.0);

  /* compute one half and mirror it */
  for (y = 0; y <= h; y++)
  {
    for (x = -w; x <= w; x++)
    {
      tx = c*x - s*y;
      ty = c*y + s*x;
      ty *= brush->aspect_ratio;
      d = sqrt(tx*tx + ty*ty);
      if (d < brush->radius+1)
	a = lookup[(int)rint(d*OVERSAMPLING)];
      else
	a = 0.0;
      a = CLAMP (a * 65535.0, 0, 65535);
      centerp[   y*canvas_w + x] = a;
      centerp[-1*y*canvas_w - x] = a;
    }
  }
  canvas_portion_unref (gbrush->mask, 0, 0);
}

static void
gimp_brush_generated_compute_u8( 
				GimpBrushGenerated *brush,
				gint width,
				gint height,
			  	gfloat *lookup	 
				)
{
  GimpBrush *gbrush = GIMP_BRUSH(brush);
  guchar *centerp;
  gdouble a, d, c, s, tx, ty;
  gint x,y;
  gint canvas_w, canvas_h;
  guint8 *data;
  gint w = width;
  gint h = height;
  Tag tag = tag_new ( default_precision, FORMAT_GRAY, ALPHA_NO ); 

  /* make the canvas */
  gbrush->mask = canvas_new( tag, width*2 + 1, height*2 + 1, STORAGE_FLAT);
  canvas_portion_refrw (gbrush->mask,0,0);
  data = (guint8*)canvas_portion_data (gbrush->mask, 0, 0);  
  canvas_w = canvas_width (gbrush->mask); 
  canvas_h = canvas_height (gbrush->mask); 
  centerp = &data[height*canvas_w + width];
	
  s = sin(brush->angle*M_PI/180.0);
  c = cos(brush->angle*M_PI/180.0);

  /* compute one half and mirror it */
  for (y = 0; y <= h; y++)
  {
    for (x = -w; x <= w; x++)
    {
      tx = c*x - s*y;
      ty = c*y + s*x;
      ty *= brush->aspect_ratio;
      d = sqrt(tx*tx + ty*ty);
      if (d < brush->radius+1)
	a = lookup[(int)rint(d*OVERSAMPLING)];
      else
	a = 0.0;
      a = CLAMP (a * 255.0, 0, 255);
      centerp[   y*canvas_w + x] = a;
      centerp[-1*y*canvas_w - x] = a;
    }
  }
  canvas_portion_unref (gbrush->mask, 0, 0);
}
				

float
gimp_brush_generated_set_radius (GimpBrushGenerated* brush, float radius)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_GENERATED(brush), -1.0);
  if (radius < 0.0)
    radius = 0.0;
  else if(radius > 32767.0)
    radius = 32767.0;
  if (radius == brush->radius)
    return radius;
  brush->radius = radius;
  if (!brush->freeze)
    gimp_brush_generated_generate(brush);
  return brush->radius;
}

float
gimp_brush_generated_set_hardness (GimpBrushGenerated* brush, float hardness)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_GENERATED(brush), -1.0);
  if (hardness < 0.0)
    hardness = 0.0;
  else if(hardness > 1.0)
    hardness = 1.0;
  if (brush->hardness == hardness)
    return hardness;
  brush->hardness = hardness;
  if (!brush->freeze)
    gimp_brush_generated_generate(brush);
  return brush->hardness;
}

float
gimp_brush_generated_set_angle (GimpBrushGenerated* brush, float angle)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_GENERATED(brush), -1.0);
  if (angle < 0.0)
    angle = -1.0 * fmod(angle, 180.0);
  else if(angle > 180.0)
    angle = fmod(angle, 180.0);
  if (brush->angle == angle)
    return angle;
  brush->angle = angle;
  if (!brush->freeze)
    gimp_brush_generated_generate(brush);
  return brush->angle;
}

float
gimp_brush_generated_set_aspect_ratio (GimpBrushGenerated* brush, float ratio)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_GENERATED(brush), -1.0);
  if (ratio < 1.0)
    ratio = 1.0;
  else if(ratio > 1000)
    ratio = 1000;
  if (brush->aspect_ratio == ratio)
    return ratio;
  brush->aspect_ratio = ratio;
  if (!brush->freeze)
    gimp_brush_generated_generate(brush);
  return brush->aspect_ratio;
}

float 
gimp_brush_generated_get_radius (const GimpBrushGenerated* brush)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_GENERATED(brush), -1.0);
  return brush->radius;
}

float
gimp_brush_generated_get_hardness (const GimpBrushGenerated* brush)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_GENERATED(brush), -1.0);
  return brush->hardness;
}

float
gimp_brush_generated_get_angle (const GimpBrushGenerated* brush)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_GENERATED(brush), -1.0);
  return brush->angle;
}

float
gimp_brush_generated_get_aspect_ratio (const GimpBrushGenerated* brush)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_GENERATED(brush), -1.0);
  return brush->aspect_ratio;
}
