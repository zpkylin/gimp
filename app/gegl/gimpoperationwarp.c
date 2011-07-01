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

enum
{
  PROP_0,
  PROP_STRENGTH,
  PROP_SIZE,
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

G_DEFINE_TYPE (GimpOperationWarp, gimp_operation_warp,
                      GEGL_TYPE_OPERATION_FILTER)

#define parent_class gimp_operation_warp_parent_class

#define STROKE_RATE 10
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
    case PROP_STROKE:
      if (self->stroke)
        g_object_unref (self->stroke);
      self->stroke = g_value_dup_object (value);
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

static gboolean
gimp_operation_warp_process (GeglOperation       *operation,
                             GeglBuffer          *in_buf,
                             GeglBuffer          *out_buf,
                             const GeglRectangle *roi)
{
  GimpOperationWarp   *ow    = GIMP_OPERATION_WARP (operation);
  gdouble              distance;
  Point                prev, next, lerp;
  gulong               i;
  GeglPathList        *event;

  ow->buffer = gegl_buffer_dup (in_buf);

  event = gegl_path_get_path (ow->stroke);

  prev = *(event->d.point);

  while (event->next)
    {
      event = event->next;
      next = *(event->d.point);

      /* we are doing a sort of interpolation 3d throught x,y and time */
      distance = sqrt (POW2(point_dist (&next, &prev)) + POW2(STROKE_RATE));

      for (i = 0; i <= distance; i++)
        {
          point_lerp (&lerp, &prev, &next, i / distance);
          gimp_operation_warp_stamp (ow, lerp.x, lerp.y);
        }

      prev = next;
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
  gfloat radius = sqrt(x*x+y*y);

  return (radius < ow->size / 2.0) ? ow->strength : 0.0;
}
