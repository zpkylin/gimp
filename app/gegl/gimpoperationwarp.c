/* GIMP - The GNU Image Manipulation Program
 *
 * gimpoperationwarp.c
 * Copyright (C) 2011 Michael Muré <batolettre@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gegl-buffer-iterator.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"

#include "gimp-gegl-types.h"

#include "gimpoperationwarp.h"

#include "gimp-intl.h"

#include <stdio.h> /* for debug */

enum
{
  PROP_0,
  PROP_STRENGTH,
  PROP_SIZE,
  PROP_HARDNESS,
  PROP_STROKE,
  PROP_BEHAVIOR
};

static void         gimp_operation_warp_finalize        (GObject             *object);
static void         gimp_operation_warp_get_property    (GObject             *object,
                                                         guint                property_id,
                                                         GValue              *value,
                                                         GParamSpec          *pspec);
static void         gimp_operation_warp_set_property    (GObject             *object,
                                                         guint                property_id,
                                                         const GValue        *value,
                                                         GParamSpec          *pspec);
static void         gimp_operation_warp_prepare         (GeglOperation       *operation);
static void         gimp_operation_warp_stroke_changed  (GeglPath            *path,
                                                         const GeglRectangle *roi,
                                                         gpointer             userdata);
static gboolean     gimp_operation_warp_process         (GeglOperation       *operation,
                                                         GeglBuffer          *in_buf,
                                                         GeglBuffer          *out_buf,
                                                         const GeglRectangle *roi);
static void         gimp_operation_warp_stamp           (GimpOperationWarp   *ow,
                                                         gdouble              x,
                                                         gdouble              y);
static gdouble      gimp_operation_warp_get_influence   (GimpOperationWarp   *ow,
                                                         gdouble              x,
                                                         gdouble              y);
static void         gimp_operation_warp_calc_lut        (GimpOperationWarp   *ow);
static gdouble      gauss                               (gdouble              f);

G_DEFINE_TYPE (GimpOperationWarp, gimp_operation_warp,
                      GEGL_TYPE_OPERATION_FILTER)

#define parent_class gimp_operation_warp_parent_class

#define POW2(a) ((a)*(a))

static void
gimp_operation_warp_class_init (GimpOperationWarpClass *klass)
{
  GObjectClass               *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass         *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationFilterClass   *filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  object_class->get_property               = gimp_operation_warp_get_property;
  object_class->set_property               = gimp_operation_warp_set_property;
  object_class->finalize                   = gimp_operation_warp_finalize;

  operation_class->name                    = "gimp:warp";
  operation_class->categories              = "transform";
  operation_class->description             = "GIMP warp";

  operation_class->prepare                 = gimp_operation_warp_prepare;

  filter_class->process                    = gimp_operation_warp_process;

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_STRENGTH,
                                   "strength", _("Effect Strength"),
                                   0.0, 100.0, 1.0,
                                   GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_SIZE,
                                   "size", _("Effect Size"),
                                   1.0, 10000.0, 40.0,
                                   GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_HARDNESS,
                                   "hardness", _("Effect Harness"),
                                   0.0, 1.0, 0.5,
                                   GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_STROKE,
                                   "stroke", _("Stroke"),
                                   GEGL_TYPE_PATH,
                                   GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_BEHAVIOR,
                                 "behavior",
                                 N_("Behavior"),
                                 GIMP_TYPE_WARP_BEHAVIOR,
                                 GIMP_WARP_BEHAVIOR_MOVE,
                                 GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_operation_warp_init (GimpOperationWarp *self)
{
  self->last_point_set = FALSE;
  self->path_changed_handler = 0;
}

static void
gimp_operation_warp_finalize (GObject *object)
{
  GimpOperationWarp *self = GIMP_OPERATION_WARP (object);

  if (self->stroke)
    {
      g_object_unref (self->stroke);
      self->stroke = NULL;
    }

  if (self->lookup)
    {
      g_free (self->lookup);
      self->lookup = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_operation_warp_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GimpOperationWarp *self = GIMP_OPERATION_WARP (object);

  switch (property_id)
    {
    case PROP_STRENGTH:
      g_value_set_double (value, self->strength);
      break;
    case PROP_SIZE:
      g_value_set_double (value, self->size);
      break;
    case PROP_HARDNESS:
      g_value_set_double (value, self->hardness);
      break;
    case PROP_STROKE:
      g_value_set_object (value, self->stroke);
      break;
    case PROP_BEHAVIOR:
      g_value_set_enum (value, self->behavior);
      break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
gimp_operation_warp_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpOperationWarp *self = GIMP_OPERATION_WARP (object);

  switch (property_id)
    {
    case PROP_STRENGTH:
      self->strength = g_value_get_double (value);
      break;
    case PROP_SIZE:
      self->size = g_value_get_double (value);
      break;
    case PROP_HARDNESS:
      self->hardness = g_value_get_double (value);
      break;
    case PROP_STROKE:
      if (self->stroke)
        {
          g_signal_handler_disconnect (self->stroke, self->path_changed_handler);
          g_object_unref (self->stroke);
        }
      self->stroke = g_value_dup_object (value);
      self->path_changed_handler = g_signal_connect (self->stroke,
                                                     "changed",
                                                     G_CALLBACK(gimp_operation_warp_stroke_changed),
                                                     self);
      break;
    case PROP_BEHAVIOR:
      self->behavior = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_warp_prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format_n (babl_type ("float"), 2));
  gegl_operation_set_format (operation, "output", babl_format_n (babl_type ("float"), 2));
}

static void
gimp_operation_warp_stroke_changed (GeglPath            *path,
                                    const GeglRectangle *roi,
                                    gpointer             userdata)
{
  GeglRectangle rect = *roi;
  GimpOperationWarp   *ow    = GIMP_OPERATION_WARP (userdata);
  /* invalidate the incoming rectangle */

  rect.x -= ow->size/2;
  rect.y -= ow->size/2;
  rect.width += ow->size;
  rect.height += ow->size;

  gegl_operation_invalidate (userdata, &rect, FALSE);
}

static gboolean
gimp_operation_warp_process (GeglOperation       *operation,
                             GeglBuffer          *in_buf,
                             GeglBuffer          *out_buf,
                             const GeglRectangle *roi)
{
  GimpOperationWarp   *ow    = GIMP_OPERATION_WARP (operation);
  gdouble              dist;
  gdouble              stamps;
  gdouble              spacing = MAX (ow->size * 0.01, 0.5); /*1% spacing for starters*/

  Point                prev, next, lerp;
  gulong               i;
  GeglPathList        *event;

  printf("Process %p\n", ow);

  ow->buffer = gegl_buffer_dup (in_buf);

  event = gegl_path_get_path (ow->stroke);

  prev = *(event->d.point);

  while (event->next)
    {
      event = event->next;
      next = *(event->d.point);
      dist = point_dist (&next, &prev);
      stamps = dist / spacing;

      if (stamps < 1)
       {
        gimp_operation_warp_stamp (ow, next.x, next.y);
        prev = next;
       }
      else
       {
        for (i = 0; i < stamps; i++)
          {
            point_lerp (&lerp, &prev, &next, (i * spacing) / dist);
            gimp_operation_warp_stamp (ow, lerp.x, lerp.y);
          }
         prev = lerp;
       }
    }

  /* Affect the output buffer */
  gegl_buffer_copy (ow->buffer, roi, out_buf, roi);
  gegl_buffer_set_extent (out_buf, gegl_buffer_get_extent (in_buf));
  gegl_buffer_destroy (ow->buffer);

  /* prepare for the recomputing of the op */
  ow->last_point_set = FALSE;

  return TRUE;
}

static void
gimp_operation_warp_stamp (GimpOperationWarp *ow,
                           gdouble            x,
                           gdouble            y)
{
  GeglBufferIterator  *it;
  Babl                *format;
  gdouble              influence;
  gdouble              x_mean = 0.0;
  gdouble              y_mean = 0.0;
  gint                 x_iter, y_iter;
  GeglRectangle        area = {x - ow->size / 2.0,
                               y - ow->size / 2.0,
                               ow->size,
                               ow->size};

  /* first point of the stroke */
  if (!ow->last_point_set)
    {
      ow->last_x = x;
      ow->last_y = y;
      ow->last_point_set = TRUE;
      return;
    }

  format = babl_format_n (babl_type ("float"), 2);

  /* If needed, compute the mean deformation */
  if (ow->behavior == GIMP_WARP_BEHAVIOR_SMOOTH)
    {
      gint pixel_count = 0;

      it = gegl_buffer_iterator_new (ow->buffer, &area, format, GEGL_BUFFER_READ);

      while (gegl_buffer_iterator_next (it))
        {
          gint    n_pixels    = it->length;
          gfloat *coords      = it->data[0];

          while (n_pixels--)
            {
              x_mean += coords[0];
              y_mean += coords[1];
              coords += 2;
            }
          pixel_count += it->roi->width * it->roi->height;
        }
      x_mean /= pixel_count;
      y_mean /= pixel_count;
    }

  it = gegl_buffer_iterator_new (ow->buffer, &area, format, GEGL_BUFFER_READWRITE);

  while (gegl_buffer_iterator_next (it))
    {
      /* iterate inside the stamp roi */
      gint    n_pixels = it->length;
      gfloat *coords   = it->data[0];

      x_iter = it->roi->x; /* initial x         */
      y_iter = it->roi->y; /* and y coordinates */

      while (n_pixels--)
        {
          influence = gimp_operation_warp_get_influence (ow,
                                                         x_iter - x,
                                                         y_iter - y);

          switch (ow->behavior)
            {
              case GIMP_WARP_BEHAVIOR_MOVE:
                coords[0] += influence * (ow->last_x - x);
                coords[1] += influence * (ow->last_y - y);
                break;
              case GIMP_WARP_BEHAVIOR_GROW:
                coords[0] -= influence * (x_iter - x) / ow->size;
                coords[1] -= influence * (y_iter - y) / ow->size;
                break;
              case GIMP_WARP_BEHAVIOR_SHRINK:
                coords[0] += influence * (x_iter - x) / ow->size;
                coords[1] += influence * (y_iter - y) / ow->size;
                break;
              case GIMP_WARP_BEHAVIOR_SWIRL_CW:
                coords[0] += influence * (y_iter - y) / ow->size;
                coords[1] -= influence * (x_iter - x) / ow->size;
                break;
              case GIMP_WARP_BEHAVIOR_SWIRL_CCW:
                coords[0] -= influence * (y_iter - y) / ow->size;
                coords[1] += influence * (x_iter - x) / ow->size;
                break;
              case GIMP_WARP_BEHAVIOR_ERASE:
                coords[0] *= 1.0 - MIN (influence, 1.0);
                coords[1] *= 1.0 - MIN (influence, 1.0);
                break;
              case GIMP_WARP_BEHAVIOR_SMOOTH:
                coords[0] -= influence * (coords[0] - x_mean);
                coords[1] -= influence * (coords[1] - y_mean);
                break;
            }

          coords += 2;

          /* update x and y coordinates */
          x_iter++;
          if (x_iter >= (it->roi->x + it->roi->width))
            {
              x_iter = it->roi->x;
              y_iter++;
            }
        }
    }

  /* Memorize the stamp location for movement dependant behavior like move */
  ow->last_x = x;
  ow->last_y = y;
}

static gdouble
gimp_operation_warp_get_influence (GimpOperationWarp *ow,
                                   gdouble            x,
                                   gdouble            y)
{
  gfloat radius;

  if (!ow->lookup)
    {
      gimp_operation_warp_calc_lut (ow);
    }

  radius = sqrt(x*x+y*y);

  if (radius < 0.5 * ow->size + 1)
    return ow->strength * ow->lookup[(gint) RINT (radius)];
  else
    return 0.0;
}

/* set up lookup table */
static void
gimp_operation_warp_calc_lut (GimpOperationWarp  *ow)
{
  gint     length;
  gint     x;
  gdouble  exponent;

  length = ceil (0.5 * ow->size + 1.0);

  ow->lookup = g_malloc (length * sizeof (gfloat));

  if ((1.0 - ow->hardness) < 0.0000004)
    exponent = 1000000.0;
  else
    exponent = 0.4 / (1.0 - ow->hardness);

  for (x = 0; x < length; x++)
    {
      ow->lookup[x] = gauss (pow (2.0 * x / ow->size, exponent));
    }
}

static gdouble
gauss (gdouble f)
{
  /* This is not a real gauss function. */
  /* Approximation is valid if -1 < f < 1 */
  if (f < -0.5)
    {
      f = -1.0 - f;
      return (2.0 * f*f);
    }

  if (f < 0.5)
    return (1.0 - 2.0 * f*f);

  f = 1.0 - f;
  return (2.0 * f*f);
}
