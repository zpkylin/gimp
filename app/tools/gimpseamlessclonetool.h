/* GIMP - The GNU Image Manipulation Program
 *
 * gimpseamlessclonetool.h
 * Copyright (C) 2011 Barak Itkin <lightningismyname@gmail.com>
 * Based on gimpcagetool.h (C) Michael Muré <batolettre@gmail.com>
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

#ifndef __GIMP_SEAMLESS_CLONE_TOOL_H__
#define __GIMP_SEAMLESS_CLONE_TOOL_H__


#include "gimpdrawtool.h"


#define GIMP_TYPE_SEAMLESS_CLONE_TOOL            (gimp_seamless_clone_tool_get_type ())
#define GIMP_SEAMLESS_CLONE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SEAMLESS_CLONE_TOOL, GimpSeamlessCloneTool))
#define GIMP_SEAMLESS_CLONE_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SEAMLESS_CLONE_TOOL, GimpSeamlessCloneToolClass))
#define GIMP_IS_SEAMLESS_CLONE_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SEAMLESS_CLONE_TOOL))
#define GIMP_IS_SEAMLESS_CLONE_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SEAMLESS_CLONE_TOOL))
#define GIMP_SEAMLESS_CLONE_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SEAMLESS_CLONE_TOOL, GimpSeamlessCloneToolClass))

#define GIMP_SEAMLESS_CLONE_TOOL_GET_OPTIONS(t)  (GIMP_SEAMLESS_CLONE_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpSeamlessCloneTool      GimpSeamlessCloneTool;
typedef struct _GimpSeamlessCloneToolClass GimpSeamlessCloneToolClass;

/**
 *
 */
struct _GimpSeamlessCloneTool
{
  GimpDrawTool    parent_instance;

  guint           state;

  gdouble         movement_start_x; /* Hold the initial x position */
  gdouble         movement_start_y; /* Hold the initial y position */
  
  gdouble         cursor_x; /* Hold the cursor x position */
  gdouble         cursor_y; /* Hold the cursor y position */

  GeglRectangle   paste_rect;
  GeglBuffer     *paste_buf;

  GeglNode       *render_node; /* A GEGL graph for rendering the clone operation */
  GeglNode       *paste_node;
  GeglNode       *translate_op;

  GimpImageMap   *image_map;   /* Used for preview of the resulting drawable */
};

#define gimp_seamless_clone_coords_in_paste(sct,c)                    \
                                                                      \
 (((sct)->paste_rect.x <= (c)->x)                                     \
  && ((sct)->paste_rect.x + (sct)->paste_rect.width <= (c)->x)        \
  && ((sct)->paste_rect.y <= (c)->y)                                  \
  && ((sct)->paste_rect.y + (sct)->paste_rect.height <= (c)->y))

struct _GimpSeamlessCloneToolClass
{
  GimpDrawToolClass parent_class;
};

void    gimp_seamless_clone_tool_register (GimpToolRegisterCallback  callback,
                                           gpointer                  data);

GType   gimp_seamless_clone_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_SEAMLESS_CLONE_TOOL_H__  */
