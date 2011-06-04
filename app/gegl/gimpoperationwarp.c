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

#include "libgimpmath/gimpmath.h"

#include "gimp-gegl-types.h"

#include "gimpoperationwarp.h"


enum
{
  PROP_0
};


static void         gimp_operation_warp_finalize                (GObject             *object);
static void         gimp_operation_warp_get_property            (GObject             *object,
                                                                 guint                property_id,
                                                                 GValue              *value,
                                                                 GParamSpec          *pspec);
static void         gimp_operation_warp_set_property            (GObject             *object,
                                                                 guint                property_id,
                                                                 const GValue        *value,
                                                                 GParamSpec          *pspec);
static void         gimp_operation_warp_prepare                 (GeglOperation       *operation);
static gboolean     gimp_operation_warp_process                 (GeglOperation       *operation,
                                                                 GeglBuffer          *in_buf,
                                                                 GeglBuffer          *out_buf,
                                                                 const GeglRectangle *roi);
GeglRectangle       gimp_operation_warp_get_bounding_box        (GeglOperation       *operation);


G_DEFINE_TYPE (GimpOperationWarp, gimp_operation_warp,
                      GEGL_TYPE_OPERATION_FILTER)

#define parent_class gimp_operation_warp_parent_class


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
  operation_class->get_bounding_box        = gimp_operation_warp_get_bounding_box;

  filter_class->process                    = gimp_operation_warp_process;
}

static void
gimp_operation_warp_init (GimpOperationWarp *self)
{
}

static void
gimp_operation_warp_finalize (GObject *object)
{
  GimpOperationWarp *self = GIMP_OPERATION_WARP (object);

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

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_warp_prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static gboolean
gimp_operation_warp_process (GeglOperation       *operation,
                             GeglBuffer          *in_buf,
                             GeglBuffer          *out_buf,
                             const GeglRectangle *roi)
{
  GimpOperationWarp *ow    = GIMP_OPERATION_WARP (operation);

  return TRUE;
}

GeglRectangle
gimp_operation_warp_get_bounding_box (GeglOperation *operation)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation,
                                                                  "input");

  return result;
}
