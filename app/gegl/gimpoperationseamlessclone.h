/* GIMP - The GNU Image Manipulation Program
 *
 * gimpoperationseamlessclone.h
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

#ifndef __GIMP_OPERATION_SEAMLESS_CLONE_H__
#define __GIMP_OPERATION_SEAMLESS_CLONE_H__


#include <gegl-plugin.h>
#include <operation/gegl-operation-source.h>
#include <cairo.h>

#define GIMP_TYPE_OPERATION_SEAMLESS_CLONE            (gimp_operation_seamless_clone_get_type ())
#define GIMP_OPERATION_SEAMLESS_CLONE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_SEAMLESS_CLONE, GimpOperationSeamlessClone))
#define GIMP_OPERATION_SEAMLESS_CLONE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_SEAMLESS_CLONE, GimpOperationSeamlessCloneClass))
#define GIMP_IS_OPERATION_SEAMLESS_CLONE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_SEAMLESS_CLONE))
#define GIMP_IS_OPERATION_SEAMLESS_CLONE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_SEAMLESS_CLONE))
#define GIMP_OPERATION_SEAMLESS_CLONE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_SEAMLESS_CLONE, GimpOperationSeamlessCloneClass))


typedef struct _GimpOperationSeamlessCloneClass GimpOperationSeamlessCloneClass;

struct _GimpOperationSeamlessClone
{
  GeglOperationSource      parent_instance;

  GPtrArray             *ptList;
  gint                   x, y;

  cairo_pattern_t       *mesh;
  GeglRectangle          mesh_bounds;
};

struct _GimpOperationSeamlessCloneClass
{
  GeglOperationSourceClass  parent_class;
};


GType   gimp_operation_seamless_clone_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_OPERATION_SEAMLESS_CLONE_H__ */
