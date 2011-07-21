/* GIMP - The GNU Image Manipulation Program
 *
 * gimpoperationseamlessclone.c
 * Copyright (C) 2011 Barak Itkin <lightningismyname@gmail.com>
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

/* TODO: release resources, mainly the mesh pattern and points */

#include "config.h"
#include <gegl.h>
#include <cairo.h>

#include "gimp-gegl-types.h"

#include "gimpoperationseamlessclone.h"
#include "poly2tri-c/poly2tri.h"

#include <stdio.h> /* TODO: get rid of this debugging way! */

enum
{
  PROP0,
  PROP_POINTS,
  PROP_X,
  PROP_Y
};

static void
gimp_operation_seamless_clone_init (GimpOperationSeamlessClone *self);

static void
gimp_operation_seamless_clone_get_property (GObject    *object,
                                            guint       property_id,
                                            GValue     *value,
                                            GParamSpec *pspec);

static void
gimp_operation_seamless_clone_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);

static gboolean
gimp_operation_seamless_clone_process (GeglOperation       *operation,
                                       GeglBuffer          *output,
                                       const GeglRectangle *roi);

static GeglRectangle
gimp_operation_seamless_clone_get_bounding_box (GeglOperation *operation);

static void
gimp_operation_seamless_clone_prepare (GeglOperation *operation);

G_DEFINE_TYPE (GimpOperationSeamlessClone, gimp_operation_seamless_clone,
               GEGL_TYPE_OPERATION_SOURCE)

#define parent_class gimp_operation_seamless_clone_parent_class

static void
gimp_operation_seamless_clone_class_init (GimpOperationSeamlessCloneClass *klass)
{
  GObjectClass             *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass       *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationSourceClass *source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  object_class->set_property = gimp_operation_seamless_clone_set_property;
  object_class->get_property = gimp_operation_seamless_clone_get_property;

  operation_class->prepare          = gimp_operation_seamless_clone_prepare;
  operation_class->get_bounding_box = gimp_operation_seamless_clone_get_bounding_box;
  operation_class->name             = "gimp:seamless-clone";
  operation_class->categories       = "programming";
  operation_class->description      = "GIMP seamless clone operation";

  source_class->process    = gimp_operation_seamless_clone_process;

  g_object_class_install_property (object_class, PROP_POINTS,
                                   g_param_spec_pointer ("points",
                                                         "Points",
                                                         "A GPtrArray* of struct {gint x, y;} containing the points",
                                                         G_PARAM_READWRITE));

  /* Workaround around wrong ROI calculation in GEGL for Sink ops when needs_full is set */
  g_object_class_install_property (object_class, PROP_X,
                                   g_param_spec_int ("x",
                                                     "X",
                                                     "The minimal x of the scaning rect",
                                                     -G_MAXINT, G_MAXINT, 0,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_Y,
                                   g_param_spec_int ("y",
                                                     "Y",
                                                     "The minimal Y of the scaning rect",
                                                     -G_MAXINT, G_MAXINT, 0,
                                                     G_PARAM_READWRITE));

}

static void
gimp_operation_seamless_clone_init (GimpOperationSeamlessClone *self)
{
  self->mesh = NULL;
}

static void
gimp_operation_seamless_clone_get_property (GObject    *object,
                                            guint       property_id,
                                            GValue     *value,
                                            GParamSpec *pspec)
{
  GimpOperationSeamlessClone *self = GIMP_OPERATION_SEAMLESS_CLONE (object);

  switch (property_id)
    {
    case PROP_POINTS:
      g_value_set_pointer (value, self->ptList);
      break;
    case PROP_X:
      g_value_set_int (value, self->x);
      break;
    case PROP_Y:
      g_value_set_int (value, self->y);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_seamless_clone_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec)
{
  GimpOperationSeamlessClone *self = GIMP_OPERATION_SEAMLESS_CLONE (object);

  switch (property_id)
    {
    case PROP_POINTS:
      self->ptList = g_value_get_pointer (value);
      break;
    case PROP_X:
      self->x = g_value_get_int (value);
      break;
    case PROP_Y:
      self->y = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

typedef struct {
  gint x, y;
} SPoint;

static void
gimp_operation_seamless_clone_make_mesh (GimpOperationSeamlessClone *self)
{
  P2tCDT *CDT;
  SPoint *pt;
  gint    i;

  gint min_x = G_MAXINT, max_x = -G_MAXINT, min_y = G_MAXINT, max_y = -G_MAXINT;

  GPtrArray *tmpPts, *tris;

  if (self->mesh != NULL)
    return;

  tmpPts = g_ptr_array_new ();

  printf ("Making mesh from %d points!\n", self->ptList->len);

  for (i = 0; i < self->ptList->len; i++)
    {
      pt = (SPoint*) g_ptr_array_index (self->ptList, i);
      min_x = MIN (pt->x, min_x);
      min_y = MIN (pt->y, min_y);
      max_x = MAX (pt->x, max_x);
      max_y = MAX (pt->y, max_y);
      g_ptr_array_add (tmpPts, p2t_point_new_dd (pt->x, pt->y));
    }

  self->mesh_bounds.x = min_x;
  self->mesh_bounds.y = min_y;
  self->mesh_bounds.width = max_x - min_x;
  self->mesh_bounds.height = max_y - min_y;

  CDT = p2t_cdt_new (tmpPts);
  p2t_cdt_triangulate (CDT);

  tris = p2t_cdt_get_triangles (CDT);
  printf ("The mesh has %d triangles!\n", tris->len);

  self->mesh = cairo_pattern_create_mesh ();

  for (i = 0; i < tris->len; i++)
    {
      P2tPoint* tpt;
      cairo_mesh_pattern_begin_patch (self->mesh);

      tpt = p2t_triangle_get_point (triangle_index(tris,i), 0);
      cairo_mesh_pattern_move_to (self->mesh, tpt->x, tpt->y);

      tpt = p2t_triangle_get_point (triangle_index(tris,i), 1);
      cairo_mesh_pattern_line_to (self->mesh, tpt->x, tpt->y);

      tpt = p2t_triangle_get_point (triangle_index(tris,i), 2);
      cairo_mesh_pattern_line_to (self->mesh, tpt->x, tpt->y);

      tpt = p2t_triangle_get_point (triangle_index(tris,i), 0);
      cairo_mesh_pattern_line_to (self->mesh, tpt->x, tpt->y);

      cairo_mesh_pattern_set_corner_color_rgb (self->mesh, 0, 1, 0, 0);
      cairo_mesh_pattern_set_corner_color_rgb (self->mesh, 1, 0, 1, 0);
      cairo_mesh_pattern_set_corner_color_rgb (self->mesh, 2, 0, 0, 1);

      cairo_mesh_pattern_end_patch (self->mesh);
    }

  p2t_cdt_free (CDT);
}

static GeglRectangle
gimp_operation_seamless_clone_get_bounding_box (GeglOperation *operation)
{
  return GIMP_OPERATION_SEAMLESS_CLONE (operation)->mesh_bounds;
}

static gboolean
gimp_operation_seamless_clone_process (GeglOperation       *operation,
                                       GeglBuffer          *output,
                                       const GeglRectangle *roi)
{
  GimpOperationSeamlessClone *self = GIMP_OPERATION_SEAMLESS_CLONE (operation);

  guchar          *data = g_new0 (guchar, roi->width * roi->height * 4);
  cairo_t         *cr;
  cairo_surface_t *surface;

  surface = cairo_image_surface_create_for_data (data,
                                                 CAIRO_FORMAT_ARGB32,
                                                 roi->width,
                                                 roi->height,
                                                 roi->width * 4);

  cr = cairo_create (surface);
  // cairo_translate (cr, -roi->x, -roi->y);
  cairo_set_source (cr, self->mesh);
  cairo_paint (cr);

  gegl_buffer_set (output, roi, babl_format ("B'aG'aR'aA u8"), data,
                   GEGL_AUTO_ROWSTRIDE);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
  g_free (data);

  return  TRUE;
}

static void
gimp_operation_seamless_clone_prepare (GeglOperation *operation)
{
  GimpOperationSeamlessClone *self = GIMP_OPERATION_SEAMLESS_CLONE (operation);

  printf ("Preparing!\n");

  gegl_operation_set_format (operation, "output", babl_format ("RaGaBaA float"));

  gimp_operation_seamless_clone_make_mesh (self);
}
