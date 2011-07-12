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
#include <gegl-plugin.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/tile-manager.h"

#include "core/gimp.h"
#include "core/gimpbuffer.h"
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

#define DBG_CALL_NAME() g_print ("@@@ %s @@@\n", __func__)
#define SEAMLESS_CLONE_LIVE_PREVIEW TRUE

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

G_DEFINE_TYPE (GimpSeamlessCloneTool, gimp_seamless_clone_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_seamless_clone_tool_parent_class

static gboolean
gimp_seamless_clone_tool_initialize (GimpTool              *tool,
                                     GimpDisplay           *display,
                                     GError               **error);

static void
gimp_seamless_clone_tool_control (GimpTool       *tool,
                                  GimpToolAction  action,
                                  GimpDisplay    *display);

static void
gimp_seamless_clone_tool_update_image_map_easy (GimpSeamlessCloneTool *sct);

static void
gimp_seamless_clone_tool_options_notify (GimpTool         *tool,
                                         GimpToolOptions  *options,
                                         const GParamSpec *pspec);
static void
gimp_seamless_clone_tool_button_press (GimpTool            *tool,
                                       const GimpCoords    *coords,
                                       guint32              time,
                                       GdkModifierType      state,
                                       GimpButtonPressType  press_type,
                                       GimpDisplay         *display);
static void
gimp_seamless_clone_tool_motion (GimpTool         *tool,
                                 const GimpCoords *coords,
                                 guint32           time,
                                 GdkModifierType   state,
                                 GimpDisplay      *display);

void
gimp_seamless_clone_tool_button_release (GimpTool              *tool,
                                         const GimpCoords      *coords,
                                         guint32                time,
                                         GdkModifierType        state,
                                         GimpButtonReleaseType  release_type,
                                         GimpDisplay           *display);

static void
gimp_seamless_clone_tool_cursor_update (GimpTool         *tool,
                                        const GimpCoords *coords,
                                        GdkModifierType   state,
                                        GimpDisplay      *display);

static void
gimp_seamless_clone_tool_draw (GimpDrawTool *draw_tool);

static void
gimp_buffer_to_gegl_buffer_with_progress (GimpSeamlessCloneTool *sct);

static void
gimp_seamless_clone_tool_create_render_node (GimpSeamlessCloneTool *sct);

static void
gimp_seamless_clone_tool_render_node_update (GimpSeamlessCloneTool *sct);

static void
gimp_seamless_clone_tool_create_image_map (GimpSeamlessCloneTool *sct,
                                           GimpDrawable          *drawable);

static void
gimp_seamless_clone_tool_image_map_flush (GimpImageMap *image_map,
                                          GimpTool     *tool);

static void
gimp_seamless_clone_tool_image_map_update (GimpSeamlessCloneTool *ct);

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
                N_("_Seamless Clone"), "W",
                NULL, GIMP_HELP_TOOL_SEAMLESS_CLONE,
                GIMP_STOCK_TOOL_MOVE,
                data);
}

static void
gimp_seamless_clone_tool_class_init (GimpSeamlessCloneToolClass *klass)
{
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  tool_class->initialize = gimp_seamless_clone_tool_initialize;

  tool_class->control         = gimp_seamless_clone_tool_control;
  tool_class->options_notify  = gimp_seamless_clone_tool_options_notify;
  tool_class->button_press    = gimp_seamless_clone_tool_button_press;
  tool_class->button_release  = gimp_seamless_clone_tool_button_release;
  tool_class->motion          = gimp_seamless_clone_tool_motion;
  tool_class->cursor_update   = gimp_seamless_clone_tool_cursor_update;

  draw_tool_class->draw       = gimp_seamless_clone_tool_draw;
}

#define gimp_has_pixels_on_clipboard() TRUE
static gboolean
gimp_seamless_clone_tool_initialize (GimpTool              *tool,
                                     GimpDisplay           *display,
                                     GError               **error)
{
  /* TODO: add a check to see if there are pixels on the clipboard */
  if ((tool->display = display) == NULL)
    return FALSE;
  return TRUE;
}

static void
gimp_seamless_clone_tool_init (GimpSeamlessCloneTool *self)
{
  GimpTool *tool = GIMP_TOOL (self);

  DBG_CALL_NAME();
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_MOVE);

  self->state           = SEAMLESS_CLONE_STATE_NOTHING;

  self->paste_buf       = NULL;
  self->render_node     = NULL;
  self->image_map       = NULL;
  self->translate_op    = NULL;
}

static void
gimp_seamless_clone_tool_control (GimpTool       *tool,
                                  GimpToolAction  action,
                                  GimpDisplay    *display)
{
  GimpSeamlessCloneTool *sct = GIMP_SEAMLESS_CLONE_TOOL (tool);

  DBG_CALL_NAME();

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      if (sct->paste_buf)
        {
          gegl_buffer_destroy (sct->paste_buf);
          sct->paste_buf = NULL;
        }

      if (sct->render_node)
        {
          g_object_unref (sct->render_node);
          sct->render_node  = NULL;
          sct->translate_op = NULL;
        }

      if (sct->image_map)
        {
          gimp_tool_control_set_preserve (tool->control, TRUE);

          gimp_image_map_abort (sct->image_map);
          g_object_unref (sct->image_map);
          sct->image_map = NULL;

          gimp_tool_control_set_preserve (tool->control, FALSE);

          gimp_image_flush (gimp_display_get_image (tool->display));
        }

      tool->display = NULL;

      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

/**
 * gimp_seamless_clone_tool_update_image_map:
 * @sct - an instance of the #GimpSeamlessCloneTool
 *
 * This function makes sure everything is set up for previewing, and then
 * calls the preview update function
 */
static void
gimp_seamless_clone_tool_update_image_map_easy (GimpSeamlessCloneTool *sct)
{
  DBG_CALL_NAME();

  if (! sct->render_node)
    {
      gimp_seamless_clone_tool_create_render_node (sct);
    }

  if (! sct->image_map)
    {
      GimpImage    *image    = gimp_display_get_image (GIMP_TOOL(sct)->display);
      GimpDrawable *drawable = gimp_image_get_active_drawable (image);

      gimp_seamless_clone_tool_create_image_map (sct, drawable);
    }

  gimp_seamless_clone_tool_image_map_update (sct);
}

/* When one of the tool options was modified, we wish to take the following
 * steps:
 * 1. Update any GEGL node with information that was changed
 * 2. Make the preview update
 */
static void
gimp_seamless_clone_tool_options_notify (GimpTool         *tool,
                                         GimpToolOptions  *options,
                                         const GParamSpec *pspec)
{
  DBG_CALL_NAME();

  GIMP_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  /* If no display is open, then we don't have much to update in our case */
  if (! tool->display)
    return;

  /* Pause the on canvas drawing while we are updating, so that there won't be
   * any calls to the draw function with partially updated data structures */
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /* TODO:
   * Look at pspec->name to see which property was changed, and update the
   * relevant data
   */

  /* Now, update the preview */
  gimp_seamless_clone_tool_update_image_map_easy (GIMP_SEAMLESS_CLONE_TOOL (tool));

  /* Allow drawing to continue */
  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_seamless_clone_tool_button_press (GimpTool            *tool,
                                       const GimpCoords    *coords,
                                       guint32              time,
                                       GdkModifierType      state,
                                       GimpButtonPressType  press_type,
                                       GimpDisplay         *display)
{
  GimpSeamlessCloneTool *sct = GIMP_SEAMLESS_CLONE_TOOL (tool);

  if (sct->state == SEAMLESS_CLONE_STATE_NOTHING)
    {
      if (! sct->render_node)
        {
          gimp_seamless_clone_tool_create_render_node (sct);

          /* Center the paste on the cursor */
          sct->paste_x = (gint) (coords->x - sct->paste_w / 2);
          sct->paste_y = (gint) (coords->y - sct->paste_h / 2);
          if (sct->translate_op)
             gegl_node_set (sct->translate_op, "x", (gdouble)sct->paste_x, "y", (gdouble)sct->paste_y, NULL);
        }

      if (! sct->image_map)
        {
          GimpImage    *image    = gimp_display_get_image (GIMP_TOOL(sct)->display);
          GimpDrawable *drawable = gimp_image_get_active_drawable (image);

          gimp_seamless_clone_tool_create_image_map (sct, drawable);
          gimp_seamless_clone_tool_image_map_update (sct);
        }

#if SEAMLESS_CLONE_LIVE_PREVIEW
      gimp_seamless_clone_tool_image_map_update (sct);
#endif
    }
  printf ("Click at %f,%f\n", coords->x, coords->y);
  printf ("Paste is at %d,%d and it's %dx%d\n", sct->paste_x, sct->paste_y, sct->paste_w, sct->paste_h);

  if (gimp_seamless_clone_coords_in_paste (sct,coords))
    {
      printf ("The click was in the paste\n");
      sct->movement_start_x = coords->x;
      sct->movement_start_y = coords->y;

      sct->state = SEAMLESS_CLONE_STATE_MOTION;

      /* In order to receive motion events from the current click, we must
       * activate the tool control */
      gimp_tool_control_activate (tool->control);
    }
  else
    printf ("The click was NOT in the paste!\n");
}

static void
gimp_seamless_clone_tool_motion (GimpTool         *tool,
                                 const GimpCoords *coords,
                                 guint32           time,
                                 GdkModifierType   state,
                                 GimpDisplay      *display)
{
  GimpSeamlessCloneTool    *sct     = GIMP_SEAMLESS_CLONE_TOOL (tool);
  
  DBG_CALL_NAME();

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));
  
  sct->cursor_x = coords->x;
  sct->cursor_y = coords->y;

  if (sct->translate_op)
    gegl_node_set (sct->translate_op,
                   "x", (gdouble)(sct->paste_x + (gint)(coords->x - sct->movement_start_x)),
                   "y", (gdouble)(sct->paste_y + (gint)(coords->y - sct->movement_start_y)),
                   NULL);

  /* TODO:
   * We do want live preview of our tool, during move operations. However, the
   * preview should not be added before we make it quick enough - otherwise it
   * would be painfully slow.
   */
#if SEAMLESS_CLONE_LIVE_PREVIEW
  gimp_seamless_clone_tool_update_image_map_easy (sct);
#endif

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

void
gimp_seamless_clone_tool_button_release (GimpTool              *tool,
                                         const GimpCoords      *coords,
                                         guint32                time,
                                         GdkModifierType        state,
                                         GimpButtonReleaseType  release_type,
                                         GimpDisplay           *display)
{
  GimpSeamlessCloneTool    *sct      = GIMP_SEAMLESS_CLONE_TOOL (tool);

  DBG_CALL_NAME();

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (sct));

  if (sct->state == SEAMLESS_CLONE_STATE_MOTION)
    {
      /* Now deactivate the control to stop receiving motion events */
      gimp_tool_control_halt (tool->control);

      /* Commit the changes only if the release wasn't of cancel type */
      if (release_type != GIMP_BUTTON_RELEASE_CANCEL)
        {
          sct->paste_x += (gint) (coords->x - sct->movement_start_x);
          sct->paste_y += (gint) (coords->y - sct->movement_start_y);
        }

      /* Now display back according to the potentially updated result, and not
       * according to the current mouse position */
      if (sct->translate_op)
         gegl_node_set (sct->translate_op, "x", (gdouble)sct->paste_x, "y", (gdouble)sct->paste_y, NULL);

      sct->state = SEAMLESS_CLONE_STATE_READY;

      gimp_seamless_clone_tool_update_image_map_easy (sct);
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_seamless_clone_tool_cursor_update (GimpTool         *tool,
                                        const GimpCoords *coords,
                                        GdkModifierType   state,
                                        GimpDisplay      *display)
{
  GimpSeamlessCloneTool    *sct      = GIMP_SEAMLESS_CLONE_TOOL (tool);

  DBG_CALL_NAME();

  /* TODO: Needs fixing during motion */
  if (sct->state == SEAMLESS_CLONE_STATE_MOTION || gimp_seamless_clone_coords_in_paste (sct,coords))
    gimp_tool_control_set_cursor_modifier (tool->control, GIMP_CURSOR_MODIFIER_MOVE);
  else
    gimp_tool_control_set_cursor_modifier (tool->control, GIMP_CURSOR_MODIFIER_NONE);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_seamless_clone_tool_draw (GimpDrawTool *draw_tool)
{
  GimpSeamlessCloneTool    *sct        = GIMP_SEAMLESS_CLONE_TOOL (draw_tool);

  GimpCanvasGroup          *stroke_group;

  DBG_CALL_NAME();

  stroke_group = gimp_draw_tool_add_stroke_group (draw_tool);

  gimp_draw_tool_push_group (draw_tool, stroke_group);

  /* Draw the corner of the paste */
  if (sct->state == SEAMLESS_CLONE_STATE_MOTION)
    gimp_draw_tool_add_crosshair (draw_tool,
        (gdouble)(sct->paste_x + (gint)(sct->cursor_x - sct->movement_start_x)),
        (gdouble)(sct->paste_y + (gint)(sct->cursor_y - sct->movement_start_y)));

  gimp_draw_tool_pop_group (draw_tool);
}

/* TODO - extract this logic for general gimp->gegl buffer conversion */
static void
gimp_buffer_to_gegl_buffer_with_progress (GimpSeamlessCloneTool *sct)
{
  GimpProgress   *progress;
  GeglNode       *gegl;
  GeglNode       *input;
  GeglNode       *output;
  GeglProcessor  *processor;
  GeglBuffer     *buffer;
  gdouble         value;
  GimpContext    *context = & gimp_tool_get_options (GIMP_TOOL (sct)) -> parent_instance;
  GimpBuffer     *gimpbuf = context->gimp->global_buffer;
  TileManager    *tiles;
  GeglRectangle   tempR;

  DBG_CALL_NAME();

  progress = gimp_progress_start (GIMP_PROGRESS (sct),
                                  _("Saving the current clipboard"), FALSE);

  if (sct->paste_buf)
    {
      gegl_buffer_destroy (sct->paste_buf);
      sct->paste_buf = NULL;
    }

  gegl = gegl_node_new ();

  input = gegl_node_new_child (gegl,
                               "operation", "gimp:tilemanager-source",
                               "tile-manager", tiles = gimp_buffer_get_tiles (gimpbuf),
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
  tempR = * gegl_buffer_get_extent (buffer);
  sct->paste_w = tempR.width;
  sct->paste_h = tempR.height;

  tile_manager_unref (tiles);
}

/* The final graph would be
 *
 *     input    paste (at (0,0))
 *       |        |
 *       |        |
 *      / \    translate
 *     /   \     /
 *    /     \   /
 *   /      diff        input = the layer into we paste
 *   |        |         paste = the pattern we want to seamlessly paste
 *   |   interpolate    diff = paste - input
 *    \     /
 *     \   /
 *      add
 *       |
 *     output
 *
 * However, untill we have proper interpolation and meshing, we will replace it
 * by a no-op (nop).
 */
static void
gimp_seamless_clone_tool_create_render_node (GimpSeamlessCloneTool *sct)
{
  /* GimpSeamlessCloneOptions *options  = GIMP_SEAMLESS_CLONE_TOOL_GET_OPTIONS (sct); */
  GeglNode        *interpolate, *diff, *add, *paste; /* Render nodes */
  GeglNode        *input, *output; /* Proxy nodes*/
  GeglNode        *node; /* wraper to be returned */

  DBG_CALL_NAME();

  g_return_if_fail (sct->render_node == NULL);
  /* render_node is not supposed to be recreated */

  gimp_buffer_to_gegl_buffer_with_progress (sct);

  node = gegl_node_new ();

  input  = gegl_node_get_input_proxy  (node, "input");
  output = gegl_node_get_output_proxy (node, "output");

  diff = gegl_node_new_child (node,
                              "operation", "gegl:subtract",
                              NULL);

  interpolate = gegl_node_new_child (node,
                                     "operation", "gegl:nop",
                                     NULL);

  sct->translate_op = gegl_node_new_child (node,
                                           "operation", "gegl:translate",
                                           "x", 0.0, "y", 0.0,
                                           "filter", "nearest", NULL);

  add = gegl_node_new_child (node,
                              "operation", "gegl:add",
                              NULL);

  paste = gegl_node_new_child (node,
                              "operation", "gegl:buffer-source",
                              "buffer", sct->paste_buf,
                              NULL);

  sct->paste_node = paste;

  gegl_node_connect_to (paste, "output",
                        sct->translate_op, "input");

  gegl_node_connect_to (sct->translate_op, "output",
                        diff, "input");

  gegl_node_connect_to (input, "output",
                        add, "input");

  gegl_node_connect_to (input, "output",
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
  DBG_CALL_NAME();
  /* For now, do nothing */
}

static void
gimp_seamless_clone_tool_create_image_map (GimpSeamlessCloneTool *sct,
                                           GimpDrawable          *drawable)
{
  DBG_CALL_NAME();
  if (!sct->render_node)
    gimp_seamless_clone_tool_create_render_node (sct);

  sct->image_map = gimp_image_map_new (drawable,
                                       _("Seamless clone"),
                                       sct->render_node,
                                       NULL,
                                       NULL);

  g_signal_connect (sct->image_map, "flush",
                    G_CALLBACK (gimp_seamless_clone_tool_image_map_flush),
                    sct);
}

static void
gimp_seamless_clone_tool_image_map_flush (GimpImageMap *image_map,
                                          GimpTool     *tool)
{
  GimpImage *image = gimp_display_get_image (tool->display);

  DBG_CALL_NAME();
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

  DBG_CALL_NAME();

  /* TODO: get rid of this HACK */
  // ct->paste_rect.x = ct->paste_rect.y = 0;
  //gegl_buffer_set_extent (ct->paste_buf, &ct->paste_rect);
  g_debug ("PasteX,PasteY = %d, %d\n", ct->paste_x, ct->paste_y);
  gegl_rectangle_dump (gegl_buffer_get_extent (ct->paste_buf));
  //gegl_node_set (ct->paste_node, "buffer", ct->paste_buf, NULL);

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
