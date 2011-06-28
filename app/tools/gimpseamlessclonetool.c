/* GIMP - The GNU Image Manipulation Program
 *
 * gimpseamlessclonetool.c
 * Copyright (C) 2011 Barak Itkin <lightningismyname@gmail.com>
 * Based on gimpcagetool.c (C) Michael Muré <batolettre@gmail.com>
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

#include <string.h>
#include <stdlib.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/tile-manager.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpdrawable-shadow.h"
#include "core/gimpimage.h"
#include "core/gimpimagemap.h"
#include "core/gimplayer.h"
#include "core/gimpprogress.h"
#include "core/gimpprojection.h"

/*#include "gegl/gimpcageconfig.h"*/

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpseamlessclonetool.h"
#include "gimpseamlesscloneoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


enum
{
  /* No selection was activated to paste - this is the initial state when the
   * tool was selected */
  SEAMLESS_CLONE_STATE_NOTHING,
  /* After a selection was activated, it's mesh will be computed, and then we
   * are ready for interaction. This won't be done automatically when switching
   * to the tool, since it's a long computation and we don't want to make the UI
   * hang if a user accidently switched to this tool */
  SEAMLESS_CLONE_STATE_READY,
  /* Mark that the cursor is currently being moved, to know that we don't allow
   * anchoring. This is to prevent accidental anchors in the wrong place while
   * moving */
  SEAMLESS_CLONE_STATE_MOTION
};


static void       gimp_seamless_clone_tool_start              (GimpSeamlessCloneTool *ct,
                                                               GimpDisplay           *display);

static void       gimp_seamless_clone_tool_options_notify     (GimpTool              *tool,
                                                               GimpToolOptions       *options,
                                                               const GParamSpec      *pspec);
static void       gimp_seamless_clone_tool_button_press       (GimpTool              *tool,
                                                               const GimpCoords      *coords,
                                                               guint32                time,
                                                               GdkModifierType        state,
                                                               GimpButtonPressType    press_type,
                                                               GimpDisplay           *display);
static void       gimp_seamless_clone_tool_button_release     (GimpTool              *tool,
                                                               const GimpCoords      *coords,
                                                               guint32                time,
                                                               GdkModifierType        state,
                                                               GimpButtonReleaseType  release_type,
                                                               GimpDisplay           *display);
static gboolean   gimp_seamless_clone_tool_key_press          (GimpTool              *tool,
                                                               GdkEventKey           *kevent,
                                                               GimpDisplay           *display);
static void       gimp_seamless_clone_tool_motion             (GimpTool              *tool,
                                                               const GimpCoords      *coords,
                                                               guint32                time,
                                                               GdkModifierType        state,
                                                               GimpDisplay           *display);
static void       gimp_seamless_clone_tool_control            (GimpTool              *tool,
                                                               GimpToolAction         action,
                                                               GimpDisplay           *display);
static void       gimp_seamless_clone_tool_cursor_update      (GimpTool              *tool,
                                                               const GimpCoords      *coords,
                                                               GdkModifierType        state,
                                                               GimpDisplay           *display);
static void       gimp_seamless_clone_tool_oper_update        (GimpTool              *tool,
                                                               const GimpCoords      *coords,
                                                               GdkModifierType        state,
                                                               gboolean               proximity,
                                                               GimpDisplay           *display);

static void       gimp_seamless_clone_tool_draw               (GimpDrawTool          *draw_tool);

static void       gimp_seamless_clone_tool_compute_coef       (GimpSeamlessCloneTool          *ct,
                                                               GimpDisplay           *display);
static void       gimp_seamless_clone_tool_create_image_map   (GimpSeamlessCloneTool          *ct,
                                                               GimpDrawable          *drawable);
static void       gimp_seamless_clone_tool_image_map_flush    (GimpImageMap          *image_map,
                                                               GimpTool              *tool);
static void       gimp_seamless_clone_tool_image_map_update   (GimpSeamlessCloneTool          *ct);

static void       gimp_seamless_clone_tool_create_render_node (GimpSeamlessCloneTool          *ct);
static void       gimp_seamless_clone_tool_render_node_update (GimpSeamlessCloneTool          *ct);


G_DEFINE_TYPE (GimpSeamlessCloneTool, gimp_seamless_clone_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_seamless_clone_tool_parent_class


void
gimp_seamless_clone_tool_register (GimpToolRegisterCallback  callback,
                                   gpointer                  data)
{
  (* callback) (GIMP_TYPE_SEAMLESS_CLONE_TOOL,
                GIMP_TYPE_SEAMLESS_CLONE_OPTIONS,
                gimp_seamless_clone_options_gui,
                0,
                "gimp-seamless-clone-tool",
                _("Seamless Clone"),
                _("Seamless Clone: Paste a pattern into another area seamlessly"),
                N_("_Cage Transform"), "W",
                NULL, GIMP_HELP_TOOL_SEAMLESS_CLONE,
                GIMP_STOCK_TOOL_MOVE,
                data);
}

static void
gimp_seamless_clone_tool_class_init (GimpSeamlessCloneToolClass *klass)
{
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  tool_class->button_press   = gimp_seamless_clone_tool_button_press;
  tool_class->button_release = gimp_seamless_clone_tool_button_release;
  tool_class->key_press      = gimp_seamless_clone_tool_key_press;
  tool_class->motion         = gimp_seamless_clone_tool_motion;
  tool_class->cursor_update  = gimp_seamless_clone_tool_cursor_update;
  tool_class->oper_update    = gimp_seamless_clone_tool_oper_update;

  draw_tool_class->draw      = gimp_seamless_clone_tool_draw;
}

static void
gimp_seamless_clone_tool_init (GimpSeamlessCloneTool *self)
{
  GimpTool *tool = GIMP_TOOL (self);

  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_MOVE);

  /* self->config          = g_object_new (GIMP_TYPE_SEAMLESS_CLONE_CONFIG, NULL); */

  self->paste           = 
  self->image_map       = NULL;
}

static void
gimp_seamless_clone_tool_button_press (GimpTool            *tool,
                                       const GimpCoords    *coords,
                                       guint32              time,
                                       GdkModifierType      state,
                                       GimpButtonPressType  press_type,
                                       GimpDisplay         *display)
{
  GimpSeamlessCloneTool    *sct        = GIMP_SEAMLESS_CLONE_TOOL (tool);
  
  gint                      tx, ty;
  
  gimp_display_shell_transform_xy (shell,
                                   coords->x, coords->y,
                                   &tx, &ty);


  sct->movement_start_x = (gint) tx;
  sct->movement_start_y = (gint) ty;
}

static gboolean
gimp_seamless_clone_tool_key_press (GimpTool    *tool,
                                    GdkEventKey *kevent,
                                    GimpDisplay *display)
{
  GimpSeamlessCloneTool *sct = GIMP_SEAMLESS_CLONE_TOOL (tool);

  switch (kevent->keyval)
    {
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      /* Should paste here */
      break;

    default:
      break;
    }

  return FALSE;
}

static void
gimp_seamless_clone_tool_motion (GimpTool         *tool,
                                 const GimpCoords *coords,
                                 guint32           time,
                                 GdkModifierType   state,
                                 GimpDisplay      *display)
{
  GimpSeamlessCloneTool    *sct       = GIMP_SEAMLESS_CLONE_TOOL (tool);
  GimpSeamlessCloneOptions *options  = GIMP_SEAMLESS_CLONE_TOOL_GET_OPTIONS (ct);
  
  gint                      tx, ty;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));
  
  gimp_display_shell_transform_xy (shell,
                                   coords->x, coords->y,
                                   &tx, &ty);

  sct->cursor_x = (gint) tx;
  sct->cursor_y = (gint) ty;

  /* Now, the paste should be positioned at
   * sct->paste_x + sct->cursor_x - sct->movement_start_x
   * sct->paste_y + sct->cursor_y - sct->movement_start_y
   */

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_seamless_clone_tool_oper_update (GimpTool         *tool,
                                      const GimpCoords *coords,
                                      GdkModifierType   state,
                                      gboolean          proximity,
                                      GimpDisplay      *display)
{
  GimpSeamlessCloneTool *ct        = GIMP_SEAMLESS_CLONE_TOOL (tool);
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

  gimp_draw_tool_pause (draw_tool);

  ct->cursor_x        = coords->x;
  ct->cursor_y        = coords->y;

  gimp_draw_tool_resume (draw_tool);
}


void
gimp_seamless_clone_tool_button_release (GimpTool              *tool,
                                         const GimpCoords      *coords,
                                         guint32                time,
                                         GdkModifierType        state,
                                         GimpButtonReleaseType  release_type,
                                         GimpDisplay           *display)
{
  GimpSeamlessCloneTool    *ct      = GIMP_SEAMLESS_CLONE_TOOL (tool);
  GimpSeamlessCloneOptions *options = GIMP_SEAMLESS_CLONE_TOOL_GET_OPTIONS (ct);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (ct));

  gimp_tool_control_halt (tool->control);

  if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    {
      /* Cancelling */

      /* gimp_seamless_clone_config_reset_displacement (ct->config); */
    }
  else
    {
      /* Normal release */

    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_seamless_clone_tool_cursor_update (GimpTool         *tool,
                                        const GimpCoords *coords,
                                        GdkModifierType   state,
                                        GimpDisplay      *display)
{
  GimpSeamlessCloneTool       *ct       = GIMP_SEAMLESS_CLONE_TOOL (tool);
  GimpSeamlessCloneOptions    *options  = GIMP_SEAMLESS_CLONE_TOOL_GET_OPTIONS (ct);
  GimpCursorModifier  modifier = GIMP_CURSOR_MODIFIER_MOVE;
  /* See app/widgets/widgets-enums.h */

  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_seamless_clone_tool_draw (GimpDrawTool *draw_tool)
{
  GimpSeamlessCloneTool    *ct        = GIMP_SEAMLESS_CLONE_TOOL (draw_tool);
  GimpSeamlessCloneOptions *options   = GIMP_SEAMLESS_CLONE_TOOL_GET_OPTIONS (ct);

  stroke_group = gimp_draw_tool_add_stroke_group (draw_tool);

  gimp_draw_tool_push_group (draw_tool, stroke_group);

  /* Draw a crosshair showing the middle of the pasted area */
  gimp_draw_tool_add_crosshair (draw_tool, ct->paste_x, ct->paste_y);

  gimp_draw_tool_pop_group (draw_tool);
}

static void
gimp_seamless_clone_tool_compute_coef (GimpSeamlessCloneTool *ct,
                                       GimpDisplay  *display)
{
  GimpSeamlessCloneConfig *config = ct->config;
  GimpProgress   *progress;
  Babl           *format;
  GeglNode       *gegl;
  GeglNode       *input;
  GeglNode       *output;
  GeglProcessor  *processor;
  GeglBuffer     *buffer;
  gdouble         value;

  progress = gimp_progress_start (GIMP_PROGRESS (ct),
                                  _("Computing Cage Coefficients"), FALSE);

  if (ct->coef)
    {
      gegl_buffer_destroy (ct->coef);
      ct->coef = NULL;
    }

  format = babl_format_n (babl_type ("float"),
                          gimp_seamless_clone_config_get_n_points (config) * 2);


  gegl = gegl_node_new ();

  input = gegl_node_new_child (gegl,
                               "operation", "gimp:cage-coef-calc",
                               "config",    ct->config,
                               NULL);

  output = gegl_node_new_child (gegl,
                                "operation", "gegl:buffer-sink",
                                "buffer",    &buffer,
                                "format",    format,
                                NULL);

  gegl_node_connect_to (input, "output",
                        output, "input");

  processor = gegl_node_new_processor (output, NULL);

  while (gegl_processor_work (processor, &value))
    {
      if (progress)
        gimp_progress_set_value (progress, value);
    }

  if (progress)
    gimp_progress_end (progress);

  gegl_processor_destroy (processor);

  ct->coef = buffer;
  g_object_unref (gegl);

  ct->dirty_coef = FALSE;
}

static void
gimp_seamless_clone_tool_transform_progress (GObject          *object,
                                             const GParamSpec *pspec,
                                             GimpSeamlessCloneTool     *ct)
{
  GimpProgress *progress = GIMP_PROGRESS (ct);
  gdouble       value;

  g_object_get (object, "progress", &value, NULL);

  if (value == 0.0)
    {
      gimp_progress_start (progress, _("Cage Transform"), FALSE);
    }
  else if (value == 1.0)
    {
      gimp_progress_end (progress);
    }
  else
    {
      gimp_progress_set_value (progress, value);
    }
}

/* TODO - extract this logic for general gimp->gegl buffer conversion */
gimp_buffer_to_gegl_buffer_with_progress (GimpSeamlessCloneTool *sct)
{
  GimpProgress   *progress;
  GeglNode       *gegl;
  GeglNode       *input;
  GeglNode       *output;
  GeglProcessor  *processor;
  GeglBuffer     *buffer = GIMP_TOOL (sct) -> ;
  gdouble         value;

  progress = gimp_progress_start (GIMP_PROGRESS (ct),
                                  _("Saving the current clipboard"), FALSE);

  if (sct->paste_buf)
    {
      gegl_buffer_destroy (sct->paste_buf);
      sct->paste_buf = NULL;
    }

  gegl = gegl_node_new ();

  input = gegl_node_new_child (gegl,
                               "operation", "gimp:tilemanager-source",
                               "tile-manager", tiles,
                               NULL);

  output = gegl_node_new_child (gegl,
                                "operation", "gegl:buffer-sink",
                                "buffer",    &buffer,
                                NULL);

  gegl_node_connect_to (input, "output",
                        output, "input");

  processor = gegl_node_new_processor (output, NULL);

  while (gegl_processor_work (processor, &value))
    {
      if (progress)
        gimp_progress_set_value (progress, value);
    }

  if (progress)
    gimp_progress_end (progress);

  gegl_processor_destroy (processor);

  sct->paste_buf = buffer;
}

/* The final graph would be
 *
 *      input   paste
 *      /  \     /
 *     /    \   /
 *    /     diff
 *   |        |
 *   |   interpolate
 *    \     /
 *     \   /
 *      add
 *       |
 *     output
 *
 *
 * However, untill we have proper interpolation and meshing, we will replace it
 * by a simple blur of the layer. That's a very "rough" approximation of the
 * original algorithm
 */
static void
gimp_seamless_clone_tool_create_render_node (GimpSeamlessCloneTool *sct)
{
  GimpSeamlessCloneOptions *options  = GIMP_SEAMLESS_CLONE_TOOL_GET_OPTIONS (ct);
  GeglNode        *interpolate, *diff, *add, *paste; /* Render nodes */
  GeglNode        *input, *output; /* Proxy nodes*/
  GeglNode        *node; /* wraper to be returned */
  /* FIXME - we don't have a Gimp object here */
  GimpBuffer      *clipboard = gimp->global_buffer;
  GimpTileManager *clipboard_tiles;
  GeglBuffer      *paste_buf;

  g_return_if_fail (ct->render_node == NULL);
  /* render_node is not supposed to be recreated */

  gimp_buffer_to_gegl_buffer_with_progress (sct)

  node = gegl_node_new ();

  input  = gegl_node_get_input_proxy  (node, "input");
  output = gegl_node_get_output_proxy (node, "output");

  diff = gegl_node_new_child (node,
                              "operation", "gegl:subtract",
                              NULL);

  interpolate = gegl_node_new_child (node,
                                     "operation", "gegl:gaussian-blur",
                                     NULL);

  add = gegl_node_new_child (node,
                              "operation", "gegl:add",
                              NULL);

  paste = gegl_node_new_child (node,
                              "operation", "gegl:buffer-source",
                              "buffer", sct->paste_buf,
                              NULL);


  gegl_node_connect_to (input, "output",
                        diff, "input");

  gegl_node_connect_to (input, "output",
                        add, "input");

  gegl_node_connect_to (paste, "output",
                        diff, "aux");

  gegl_node_connect_to (diff, "output",
                        interpolate, "input");

  gegl_node_connect_to (interpolate, "output",
                        add, "aux");

  gegl_node_connect_to (add, "output",
                        output, "input");

  sct->render_node = node;
}

static void
gimp_seamless_clone_tool_render_node_update (GimpSeamlessCloneTool *sct)
{
  /* For now, do nothing */
}

static void
gimp_seamless_clone_tool_create_image_map (GimpSeamlessCloneTool *sct,
                                           GimpDrawable          *drawable)
{
  if (!ct->render_node)
    gimp_seamless_clone_tool_create_render_node (ct);

  ct->image_map = gimp_image_map_new (drawable,
                                      _("Seamless clone"),
                                      ct->render_node,
                                      NULL,
                                      NULL);

  g_signal_connect (ct->image_map, "flush",
                    G_CALLBACK (gimp_seamless_clone_tool_image_map_flush),
                    ct);
}

static void
gimp_seamless_clone_tool_image_map_flush (GimpImageMap *image_map,
                                          GimpTool     *tool)
{
  GimpImage *image = gimp_display_get_image (tool->display);

  gimp_projection_flush_now (gimp_image_get_projection (image));
  gimp_display_flush_now (tool->display);
}

static void
gimp_seamless_clone_tool_image_map_update (GimpSeamlessCloneTool *ct)
{
  GimpTool         *tool  = GIMP_TOOL (ct);
  GimpDisplayShell *shell = gimp_display_get_shell (tool->display);
  GimpItem         *item  = GIMP_ITEM (tool->drawable);
  gint              x, y;
  gint              w, h;
  gint              off_x, off_y;
  GeglRectangle     visible;

  gimp_display_shell_untransform_viewport (shell, &x, &y, &w, &h);

  gimp_item_get_offset (item, &off_x, &off_y);

  gimp_rectangle_intersect (x, y, w, h,
                            off_x,
                            off_y,
                            gimp_item_get_width  (item),
                            gimp_item_get_height (item),
                            &visible.x,
                            &visible.y,
                            &visible.width,
                            &visible.height);

  visible.x -= off_x;
  visible.y -= off_y;

  gimp_image_map_apply (ct->image_map, &visible);
}
