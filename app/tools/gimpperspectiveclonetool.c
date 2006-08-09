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

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp-transform-utils.h"
#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"
#include "core/gimptoolinfo.h"
#include "core/gimpdrawable-transform.h" /* for typedef X0, etc.. */

#include "paint/gimpperspectiveclone.h"
#include "paint/gimpperspectivecloneoptions.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewablebox.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"

#include "gimpperspectiveclonetool.h"
#include "gimppaintoptions-gui.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"

#define HANDLE_SIZE 10

#define TARGET_WIDTH  15
#define TARGET_HEIGHT 15


static gboolean      gimp_perspective_clone_tool_has_display   (GimpTool        *tool,
                                                              GimpDisplay     *display);
static GimpDisplay * gimp_perspective_clone_tool_has_image     (GimpTool        *tool,
                                                              GimpImage       *image);
static gboolean      gimp_perspective_clone_tool_initialize    (GimpTool        *tool,
                                                              GimpDisplay     *display);
static void          gimp_perspective_clone_tool_control       (GimpTool        *tool,
                                                              GimpToolAction   action,
                                                              GimpDisplay     *display);
static void          gimp_perspective_clone_tool_button_press  (GimpTool        *tool,
                                                              GimpCoords      *coords,
                                                              guint32          time,
                                                              GdkModifierType  state,
                                                              GimpDisplay     *display);
static void          gimp_perspective_clone_tool_motion        (GimpTool        *tool,
                                                              GimpCoords      *coords,
                                                              guint32          time,
                                                              GdkModifierType  state,
                                                              GimpDisplay     *display);
static void          gimp_perspective_clone_tool_cursor_update (GimpTool        *tool,
                                                              GimpCoords      *coords,
                                                              GdkModifierType  state,
                                                              GimpDisplay     *display);
static void          gimp_perspective_clone_tool_oper_update   (GimpTool        *tool,
                                                              GimpCoords      *coords,
                                                              GdkModifierType  state,
                                                              gboolean         proximity,
                                                              GimpDisplay     *display);

static void          gimp_perspective_clone_tool_draw          (GimpDrawTool    *draw_tool);
static void          gimp_perspective_clone_tool_transform_bounding_box (GimpPerspectiveCloneTool *clone_tool);
static void          gimp_perspective_clone_tool_bounds        (GimpPerspectiveCloneTool  *tool,
                                                              GimpDisplay             *display);
static void          gimp_perspective_clone_tool_prepare       (GimpPerspectiveCloneTool  *clone_tool,
                                                              GimpDisplay             *display);
static void          gimp_perspective_clone_tool_recalc        (GimpPerspectiveCloneTool  *clone_tool,
                                                              GimpDisplay             *display);

static GtkWidget   * gimp_perspective_clone_options_gui        (GimpToolOptions *tool_options);


G_DEFINE_TYPE (GimpPerspectiveCloneTool, gimp_perspective_clone_tool, GIMP_TYPE_PAINT_TOOL)

#define parent_class gimp_perspective_clone_tool_parent_class


void
gimp_perspective_clone_tool_register (GimpToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (GIMP_TYPE_PERSPECTIVE_CLONE_TOOL,
                GIMP_TYPE_PERSPECTIVE_CLONE_OPTIONS,
                gimp_perspective_clone_options_gui,
                GIMP_PAINT_OPTIONS_CONTEXT_MASK |
                GIMP_CONTEXT_PATTERN_MASK,
                "gimp-perspective-clone-tool",
                _("Perspective Clone"),
                _("Paint using Image Regions Preserving the Perspective of the Image"),
                N_("_Clone"), "C",
                NULL, GIMP_HELP_TOOL_PERSPECTIVE_CLONE,
                GIMP_STOCK_TOOL_PERSPECTIVE_CLONE,
                data);
}

static void
gimp_perspective_clone_tool_class_init (GimpPerspectiveCloneToolClass *klass)
{
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  tool_class->initialize    = gimp_perspective_clone_tool_initialize;
  tool_class->has_display   = gimp_perspective_clone_tool_has_display;
  tool_class->has_image     = gimp_perspective_clone_tool_has_image;
  tool_class->control       = gimp_perspective_clone_tool_control;
  tool_class->button_press  = gimp_perspective_clone_tool_button_press;
  tool_class->motion        = gimp_perspective_clone_tool_motion;
  tool_class->cursor_update = gimp_perspective_clone_tool_cursor_update;
  tool_class->oper_update   = gimp_perspective_clone_tool_oper_update;

  draw_tool_class->draw     = gimp_perspective_clone_tool_draw;
}

static void
gimp_perspective_clone_tool_init (GimpPerspectiveCloneTool *clone)
{
  gint i;
  GimpTool *tool = GIMP_TOOL (clone);

  gimp_tool_control_set_tool_cursor     (tool->control,
                                         GIMP_TOOL_CURSOR_CLONE);
  gimp_tool_control_set_action_object_2 (tool->control,
                                         "context/context-pattern-select-set");

  for (i = 0; i < TRAN_INFO_SIZE; i++)
  {
    clone->trans_info[i]     = 0.0;
    clone->old_trans_info[i] = 0.0;
  }

  gimp_matrix3_identity (&clone->transform);

  clone->use_grid       = FALSE;
  clone->use_handles    = TRUE;

  clone->is_set_matrix  = 0;

  /*clone->ngx              = 0;
  clone->ngy              = 0;
  clone->grid_coords      = NULL;
  clone->tgrid_coords     = NULL;*/

}

static gboolean
gimp_perspective_clone_tool_initialize (GimpTool    *tool,
                                      GimpDisplay *display)
{
  GimpPerspectiveCloneTool *clone_tool = GIMP_PERSPECTIVE_CLONE_TOOL (tool);

  if (display != tool->display)
  {
    gint i;

    /*  Set the pointer to the active display  */
    tool->display  = display;
    tool->drawable = gimp_image_active_drawable (display->image);

   /*  Find the transform bounds initializing */
    gimp_perspective_clone_tool_bounds (clone_tool, display);

    gimp_perspective_clone_tool_prepare (clone_tool, display);

    /*  Recalculate the transform tool  */
    gimp_perspective_clone_tool_recalc (clone_tool, display);

    /*  start drawing the bounding box and handles...  */
    gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);

    clone_tool->function = TRANSFORM_CREATING;

    /*  Save the current transformation info  */
    for (i = 0; i < TRAN_INFO_SIZE; i++)
      clone_tool->old_trans_info[i] = clone_tool->trans_info[i];
  }

  return TRUE;
}


static gboolean
gimp_perspective_clone_tool_has_display (GimpTool    *tool,
                                       GimpDisplay *display)
{
  GimpPerspectiveCloneTool *clone_tool = GIMP_PERSPECTIVE_CLONE_TOOL (tool);

  return (display == clone_tool->src_display ||
          GIMP_TOOL_CLASS (parent_class)->has_display (tool, display));
}

static GimpDisplay *
gimp_perspective_clone_tool_has_image (GimpTool  *tool,
                                     GimpImage *image)
{
  GimpPerspectiveCloneTool *clone_tool = GIMP_PERSPECTIVE_CLONE_TOOL (tool);
  GimpDisplay            *display;

  display = GIMP_TOOL_CLASS (parent_class)->has_image (tool, image);

  if (! display && clone_tool->src_display)
    {
      if (image && clone_tool->src_display->image == image)
        display = clone_tool->src_display;

      /*  NULL image means any display  */
      if (! image)
        display = clone_tool->src_display;
    }

  return display;
}

static void
gimp_perspective_clone_tool_control (GimpTool       *tool,
                                   GimpToolAction  action,
                                   GimpDisplay    *display)
{
  GimpPerspectiveCloneTool *clone_tool = GIMP_PERSPECTIVE_CLONE_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
      break;

    case GIMP_TOOL_ACTION_RESUME:
      /* only in the case that Mode: Modify Poligon is set " */
      gimp_perspective_clone_tool_bounds (clone_tool, display);
      gimp_perspective_clone_tool_recalc (clone_tool, display);
      break;

    case GIMP_TOOL_ACTION_HALT:
      clone_tool->src_display = NULL;
      g_object_set (GIMP_PAINT_TOOL (tool)->core,
                    "src-drawable", NULL,
                    NULL);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_perspective_clone_tool_button_press (GimpTool        *tool,
                                        GimpCoords      *coords,
                                        guint32          time,
                                        GdkModifierType  state,
                                        GimpDisplay     *display)
{
  GimpPaintTool             *paint_tool = GIMP_PAINT_TOOL (tool);
  GimpPerspectiveCloneTool    *clone_tool = GIMP_PERSPECTIVE_CLONE_TOOL (tool);
  GimpPerspectiveClone        *clone      = GIMP_PERSPECTIVE_CLONE (paint_tool->core);
  GimpPerspectiveCloneOptions *options;

  options = GIMP_PERSPECTIVE_CLONE_OPTIONS (tool->tool_info->tool_options);

  if (options->clone_mode == GIMP_MODIFY_PERSPECTIVE_PLANE)
  {
    if (clone_tool->function == TRANSFORM_CREATING)
      gimp_perspective_clone_tool_oper_update (tool, coords, state, TRUE, display);

    clone_tool->lastx = clone_tool->startx = coords->x;
    clone_tool->lasty = clone_tool->starty = coords->y;

    gimp_tool_control_activate (tool->control);
  }
  else
  {
    gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

    paint_tool->core->use_saved_proj = FALSE;

    if ((state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == GDK_CONTROL_MASK)
    {
      clone->set_source = TRUE;

      clone_tool->src_display = display;
    }
    else
    {
      clone->set_source = FALSE;

      if (options->clone_type == GIMP_IMAGE_CLONE &&
          options->sample_merged                  &&
          display == clone_tool->src_display)
      {
        paint_tool->core->use_saved_proj = TRUE;
      }
    }

    GIMP_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                  display);

    clone_tool->src_x = clone->src_x;
    clone_tool->src_y = clone->src_y;

    gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
  }
}

static void
gimp_perspective_clone_tool_prepare (GimpPerspectiveCloneTool  *clone_tool,
                                   GimpDisplay             *display)
{
  clone_tool->trans_info[X0] = (gdouble) clone_tool->x1;
  clone_tool->trans_info[Y0] = (gdouble) clone_tool->y1;
  clone_tool->trans_info[X1] = (gdouble) clone_tool->x2;
  clone_tool->trans_info[Y1] = (gdouble) clone_tool->y1;
  clone_tool->trans_info[X2] = (gdouble) clone_tool->x1;
  clone_tool->trans_info[Y2] = (gdouble) clone_tool->y2;
  clone_tool->trans_info[X3] = (gdouble) clone_tool->x2;
  clone_tool->trans_info[Y3] = (gdouble) clone_tool->y2;
}

void
gimp_perspective_clone_tool_recalc (GimpPerspectiveCloneTool *clone_tool,
                                  GimpDisplay            *display)
{
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  gimp_matrix3_identity (&clone_tool->transform);
  gimp_transform_matrix_perspective  (&clone_tool->transform,
                                       clone_tool->x1,
                                       clone_tool->y1,
                                       clone_tool->x2 - clone_tool->x1,
                                       clone_tool->y2 - clone_tool->y1,
                                       clone_tool->trans_info[X0],
                                       clone_tool->trans_info[Y0],
                                       clone_tool->trans_info[X1],
                                       clone_tool->trans_info[Y1],
                                       clone_tool->trans_info[X2],
                                       clone_tool->trans_info[Y2],
                                       clone_tool->trans_info[X3],
                                       clone_tool->trans_info[Y3]);

  gimp_perspective_clone_tool_transform_bounding_box (clone_tool);
}

static void
gimp_perspective_clone_tool_motion (GimpTool        *tool,
                                  GimpCoords      *coords,
                                  guint32          time,
                                  GdkModifierType  state,
                                  GimpDisplay     *display)
{
  GimpPerspectiveCloneTool *clone_tool = GIMP_PERSPECTIVE_CLONE_TOOL (tool);
  GimpPaintTool          *paint_tool = GIMP_PAINT_TOOL (tool);
  GimpPerspectiveClone     *clone      = GIMP_PERSPECTIVE_CLONE (paint_tool->core);
  GimpPerspectiveCloneToolClass *clone_tool_class;

  GimpPerspectiveCloneOptions   *options;
  options = GIMP_PERSPECTIVE_CLONE_OPTIONS (tool->tool_info->tool_options);

  if(options->clone_mode == GIMP_MODIFY_PERSPECTIVE_PLANE)
  {
    /*  if we are creating, there is nothing to be done so exit.  */
    if (clone_tool->function == TRANSFORM_CREATING /*|| ! tr_tool->use_grid*/)
      return;

    gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

    clone_tool->curx  = coords->x;
    clone_tool->cury  = coords->y;
    clone_tool->state = state;

    /*  recalculate the tool's transformation matrix  */
    clone_tool_class = GIMP_PERSPECTIVE_CLONE_TOOL_GET_CLASS (clone_tool);

    gdouble diff_x, diff_y;

    diff_x = clone_tool->curx - clone_tool->lastx;
    diff_y = clone_tool->cury - clone_tool->lasty;

    switch (clone_tool->function)
    {
      case TRANSFORM_HANDLE_NW:
        clone_tool->trans_info[X0] += diff_x;
        clone_tool->trans_info[Y0] += diff_y;
        break;
      case TRANSFORM_HANDLE_NE:
        clone_tool->trans_info[X1] += diff_x;
        clone_tool->trans_info[Y1] += diff_y;
        break;
      case TRANSFORM_HANDLE_SW:
        clone_tool->trans_info[X2] += diff_x;
        clone_tool->trans_info[Y2] += diff_y;
        break;
      case TRANSFORM_HANDLE_SE:
        clone_tool->trans_info[X3] += diff_x;
        clone_tool->trans_info[Y3] += diff_y;
        break;
      default:
        break;
    }

    gimp_perspective_clone_tool_recalc (clone_tool, display);

    clone_tool->lastx = clone_tool->curx;
    clone_tool->lasty = clone_tool->cury;

    gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
  }
  else if (options->clone_mode == GIMP_CLONE_PAINT)
  {
    /* hack */
    if(clone_tool->is_set_matrix==0)
    {
    // guardar matriz y poner is_set a 1
      clone_tool->is_set_matrix=1;
      //clone->transform = clone_tool->transform;
      (clone->transform).coeff[0][0] = (clone_tool->transform).coeff[0][0];
      (clone->transform).coeff[0][1] = (clone_tool->transform).coeff[0][1];
      (clone->transform).coeff[0][2] = (clone_tool->transform).coeff[0][2];
      (clone->transform).coeff[1][0] = (clone_tool->transform).coeff[1][0];
      (clone->transform).coeff[1][1] = (clone_tool->transform).coeff[1][1];
      (clone->transform).coeff[1][2] = (clone_tool->transform).coeff[1][2];
      (clone->transform).coeff[2][0] = (clone_tool->transform).coeff[2][0];
      (clone->transform).coeff[2][1] = (clone_tool->transform).coeff[2][1];
      (clone->transform).coeff[2][2] = (clone_tool->transform).coeff[2][2];

      //clone->transform_inv = clone_tool->transform;
      (clone->transform_inv).coeff[0][0] = (clone_tool->transform).coeff[0][0];
      (clone->transform_inv).coeff[0][1] = (clone_tool->transform).coeff[0][1];
      (clone->transform_inv).coeff[0][2] = (clone_tool->transform).coeff[0][2];
      (clone->transform_inv).coeff[1][0] = (clone_tool->transform).coeff[1][0];
      (clone->transform_inv).coeff[1][1] = (clone_tool->transform).coeff[1][1];
      (clone->transform_inv).coeff[1][2] = (clone_tool->transform).coeff[1][2];
      (clone->transform_inv).coeff[2][0] = (clone_tool->transform).coeff[2][0];
      (clone->transform_inv).coeff[2][1] = (clone_tool->transform).coeff[2][1];
      (clone->transform_inv).coeff[2][2] = (clone_tool->transform).coeff[2][2];
      gimp_matrix3_invert (&clone->transform_inv);

    // imprimir la matriz
      g_printerr("%f\t", (clone_tool->transform).coeff[0][0]);
      g_printerr("%f\t", (clone_tool->transform).coeff[0][1]);
      g_printerr("%f\n", (clone_tool->transform).coeff[0][2]);
      g_printerr("%f\t", (clone_tool->transform).coeff[1][0]);
      g_printerr("%f\t", (clone_tool->transform).coeff[1][1]);
      g_printerr("%f\n", (clone_tool->transform).coeff[1][2]);
      g_printerr("%f\t", (clone_tool->transform).coeff[2][0]);
      g_printerr("%f\t", (clone_tool->transform).coeff[2][1]);
      g_printerr("%f\n\n", (clone_tool->transform).coeff[2][2]);
    }

    gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

    if ((state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == GDK_CONTROL_MASK)
      clone->set_source = TRUE;
    else
      clone->set_source = FALSE;

    GIMP_TOOL_CLASS (parent_class)->motion (tool, coords, time, state, display);

    clone_tool->src_x = clone->src_x;
    clone_tool->src_y = clone->src_y;

    gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
  }
}


static void
gimp_perspective_clone_tool_cursor_update (GimpTool        *tool,
                                         GimpCoords      *coords,
                                         GdkModifierType  state,
                                         GimpDisplay     *display)
{
  GimpPerspectiveCloneTool      *clone_tool = GIMP_PERSPECTIVE_CLONE_TOOL (tool);
  GimpPerspectiveCloneOptions   *options;
  GimpCursorType      cursor   = GIMP_CURSOR_MOUSE;
  GimpCursorModifier  modifier = GIMP_CURSOR_MODIFIER_NONE;

  options = GIMP_PERSPECTIVE_CLONE_OPTIONS (tool->tool_info->tool_options);

  if (gimp_image_coords_in_active_pickable (display->image, coords,
                                            FALSE, TRUE))
  {
    cursor = GIMP_CURSOR_MOUSE;
  }

  if(options->clone_mode == GIMP_MODIFY_PERSPECTIVE_PLANE)
  {
    // cursores de perspectiva
    cursor = gimp_tool_control_get_cursor (tool->control);

    if (clone_tool->use_handles)
    {
      switch (clone_tool->function)
      {
        case TRANSFORM_HANDLE_NW:
          cursor = GIMP_CURSOR_CORNER_TOP_LEFT;
          break;

        case TRANSFORM_HANDLE_NE:
          cursor = GIMP_CURSOR_CORNER_TOP_RIGHT;
          break;

        case TRANSFORM_HANDLE_SW:
          cursor = GIMP_CURSOR_CORNER_BOTTOM_LEFT;
          break;

        case TRANSFORM_HANDLE_SE:
          cursor = GIMP_CURSOR_CORNER_BOTTOM_RIGHT;
          break;

        default:
          cursor = GIMP_CURSOR_CROSSHAIR_SMALL;
          break;
      }
    }

//     switch (options->type)
//     {
//       case GIMP_TRANSFORM_TYPE_LAYER:
//       case GIMP_TRANSFORM_TYPE_SELECTION:
//         break;
//
//       case GIMP_TRANSFORM_TYPE_PATH:
        if (! gimp_image_get_active_vectors (display->image))
          modifier = GIMP_CURSOR_MODIFIER_BAD;
//         break;
//     }
  }
  else
  {
    if (options->clone_type == GIMP_IMAGE_CLONE)
      {
        if ((state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == GDK_CONTROL_MASK)
          {
            cursor = GIMP_CURSOR_CROSSHAIR_SMALL;
          }
        else if (! GIMP_PERSPECTIVE_CLONE (GIMP_PAINT_TOOL (tool)->core)->src_drawable)
          {
            modifier = GIMP_CURSOR_MODIFIER_BAD;
          }
      }
  }

  gimp_tool_control_set_cursor          (tool->control, cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_perspective_clone_tool_oper_update (GimpTool        *tool,
                                       GimpCoords      *coords,
                                       GdkModifierType  state,
                                       gboolean         proximity,
                                       GimpDisplay     *display)
{
  GimpPerspectiveCloneTool    *clone_tool = GIMP_PERSPECTIVE_CLONE_TOOL (tool);
  GimpPerspectiveCloneOptions *options;

  options = GIMP_PERSPECTIVE_CLONE_OPTIONS (tool->tool_info->tool_options);


  if(options->clone_mode == GIMP_MODIFY_PERSPECTIVE_PLANE)
  {
    GimpDrawTool           *draw_tool = GIMP_DRAW_TOOL (tool);

    clone_tool->function = TRANSFORM_HANDLE_NONE;

    if (display != tool->display)
      return;

    if (clone_tool->use_handles)
    {
      gdouble closest_dist;
      gdouble dist;

      closest_dist = gimp_draw_tool_calc_distance (draw_tool, display,
                                                   coords->x, coords->y,
                                                   clone_tool->tx1, clone_tool->ty1);
      clone_tool->function = TRANSFORM_HANDLE_NW;

      dist = gimp_draw_tool_calc_distance (draw_tool, display,
                                           coords->x, coords->y,
                                           clone_tool->tx2, clone_tool->ty2);
      if (dist < closest_dist)
      {
        closest_dist = dist;
        clone_tool->function = TRANSFORM_HANDLE_NE;
      }

      dist = gimp_draw_tool_calc_distance (draw_tool, display,
                                           coords->x, coords->y,
                                           clone_tool->tx3, clone_tool->ty3);
      if (dist < closest_dist)
      {
        closest_dist = dist;
        clone_tool->function = TRANSFORM_HANDLE_SW;
      }

      dist = gimp_draw_tool_calc_distance (draw_tool, display,
                                           coords->x, coords->y,
                                           clone_tool->tx4, clone_tool->ty4);
      if (dist < closest_dist)
      {
        closest_dist = dist;
        clone_tool->function = TRANSFORM_HANDLE_SE;
      }
    }
  }
  else
  {
    GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                                 display);

    if (options->clone_type == GIMP_IMAGE_CLONE && proximity)
      {
        GimpPerspectiveClone *clone = GIMP_PERSPECTIVE_CLONE (GIMP_PAINT_TOOL (tool)->core);

        if (clone->src_drawable == NULL)
          {
            gimp_tool_replace_status (tool, display,
                                      _("Ctrl-Click to set a clone source."));
          }
        else
          {
            gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

            clone_tool->src_x = clone->src_x;
            clone_tool->src_y = clone->src_y;

            if (! clone->first_stroke)
              {
                if (options->align_mode == GIMP_CLONE_ALIGN_YES)
                  {
                    clone_tool->src_x = coords->x + clone->offset_x;
                    clone_tool->src_y = coords->y + clone->offset_y;
                  }
                else if (options->align_mode == GIMP_CLONE_ALIGN_REGISTERED)
                  {
                    clone_tool->src_x = coords->x;
                    clone_tool->src_y = coords->y;
                  }
              }

            gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
          }
      }
    }
}

static void
gimp_perspective_clone_tool_draw (GimpDrawTool *draw_tool)
{
  GimpTool                  *tool       = GIMP_TOOL (draw_tool);
  GimpPerspectiveCloneTool    *clone_tool = GIMP_PERSPECTIVE_CLONE_TOOL (draw_tool);
  GimpPerspectiveClone        *clone      = GIMP_PERSPECTIVE_CLONE (GIMP_PAINT_TOOL (tool)->core);
  GimpPerspectiveCloneOptions *options;

  options = GIMP_PERSPECTIVE_CLONE_OPTIONS (tool->tool_info->tool_options);

  if (clone_tool->use_handles)
  {
    /*  draw the bounding box  */
    gimp_draw_tool_draw_line (draw_tool,
                              clone_tool->tx1, clone_tool->ty1,
                              clone_tool->tx2, clone_tool->ty2,
                              FALSE);
    gimp_draw_tool_draw_line (draw_tool,
                              clone_tool->tx2, clone_tool->ty2,
                              clone_tool->tx4, clone_tool->ty4,
                              FALSE);
    gimp_draw_tool_draw_line (draw_tool,
                              clone_tool->tx3, clone_tool->ty3,
                              clone_tool->tx4, clone_tool->ty4,
                              FALSE);
    gimp_draw_tool_draw_line (draw_tool,
                              clone_tool->tx3, clone_tool->ty3,
                              clone_tool->tx1, clone_tool->ty1,
                              FALSE);

    /*  draw the tool handles  */
    gimp_draw_tool_draw_handle (draw_tool,
                                GIMP_HANDLE_SQUARE,
                                clone_tool->tx1, clone_tool->ty1,
                                HANDLE_SIZE, HANDLE_SIZE,
                                GTK_ANCHOR_CENTER,
                                FALSE);
    gimp_draw_tool_draw_handle (draw_tool,
                                GIMP_HANDLE_SQUARE,
                                clone_tool->tx2, clone_tool->ty2,
                                HANDLE_SIZE, HANDLE_SIZE,
                                GTK_ANCHOR_CENTER,
                                FALSE);
    gimp_draw_tool_draw_handle (draw_tool,
                                GIMP_HANDLE_SQUARE,
                                clone_tool->tx3, clone_tool->ty3,
                                HANDLE_SIZE, HANDLE_SIZE,
                                GTK_ANCHOR_CENTER,
                                FALSE);
    gimp_draw_tool_draw_handle (draw_tool,
                                GIMP_HANDLE_SQUARE,
                                clone_tool->tx4, clone_tool->ty4,
                                HANDLE_SIZE, HANDLE_SIZE,
                                GTK_ANCHOR_CENTER,
                                FALSE);
  }

  if (options->clone_type == GIMP_IMAGE_CLONE &&
      clone->src_drawable && clone_tool->src_display)
    {
      GimpDisplay *tmp_display;
      gint         off_x;
      gint         off_y;

      gimp_item_offsets (GIMP_ITEM (clone->src_drawable), &off_x, &off_y);

      tmp_display = draw_tool->display;
      draw_tool->display = clone_tool->src_display;

      gimp_draw_tool_draw_handle (draw_tool,
                                  GIMP_HANDLE_CROSS,
                                  clone_tool->src_x + off_x,
                                  clone_tool->src_y + off_y,
                                  TARGET_WIDTH, TARGET_WIDTH,
                                  GTK_ANCHOR_CENTER,
                                  FALSE);

      draw_tool->display = tmp_display;
    }

  GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
}

static void
gimp_perspective_clone_tool_transform_bounding_box (GimpPerspectiveCloneTool *clone_tool)
{
  g_return_if_fail (GIMP_IS_PERSPECTIVE_CLONE_TOOL (clone_tool));

  gimp_matrix3_transform_point (&clone_tool->transform,
                                 clone_tool->x1, clone_tool->y1,
                                 &clone_tool->tx1, &clone_tool->ty1);
  gimp_matrix3_transform_point (&clone_tool->transform,
                                 clone_tool->x2, clone_tool->y1,
                                 &clone_tool->tx2, &clone_tool->ty2);
  gimp_matrix3_transform_point (&clone_tool->transform,
                                 clone_tool->x1, clone_tool->y2,
                                 &clone_tool->tx3, &clone_tool->ty3);
  gimp_matrix3_transform_point (&clone_tool->transform,
                                 clone_tool->x2, clone_tool->y2,
                                 &clone_tool->tx4, &clone_tool->ty4);

  /*gimp_matrix3_transform_point (&tr_tool->transform,
                                 tr_tool->cx, tr_tool->cy,
                                 &tr_tool->tcx, &tr_tool->tcy);

  if (tr_tool->grid_coords && tr_tool->tgrid_coords)
  {
    gint i, k;
    gint gci;

    gci = 0;
    k   = (tr_tool->ngx + tr_tool->ngy) * 2;

    for (i = 0; i < k; i++)
    {
      gimp_matrix3_transform_point (&tr_tool->transform,
                                     tr_tool->grid_coords[gci],
                                     tr_tool->grid_coords[gci + 1],
                                     &tr_tool->tgrid_coords[gci],
                                     &tr_tool->tgrid_coords[gci + 1]);
      gci += 2;
    }
  }*/
}

static void
gimp_perspective_clone_tool_bounds (GimpPerspectiveCloneTool *tool,
                                  GimpDisplay            *display)
{
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  tool->x1 = 0;
  tool->y1 = 0;
  tool->x2 = display->image->width;
  tool->y2 = display->image->height;
}


/*  tool options stuff  */

static GtkWidget *
gimp_perspective_clone_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *mode;
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *combo;

  vbox = gimp_paint_options_gui (tool_options);

  mode = gimp_prop_enum_radio_frame_new (config, "clone-mode",
                                         _("Mode"), 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), mode, FALSE, FALSE, 0);
  gtk_widget_show (mode);

  frame = gimp_prop_enum_radio_frame_new (config, "clone-type",
                                          _("Source"), 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  button = gimp_prop_check_button_new (config, "sample-merged",
                                       _("Sample merged"));
  gimp_enum_radio_frame_add (GTK_FRAME (frame), button, GIMP_IMAGE_CLONE);

  hbox = gimp_prop_pattern_box_new (NULL, GIMP_CONTEXT (tool_options), 2,
                                    "pattern-view-type", "pattern-view-size");
  gimp_enum_radio_frame_add (GTK_FRAME (frame), hbox, GIMP_PATTERN_CLONE);

  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  combo = gimp_prop_enum_combo_box_new (config, "align-mode", 0, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Alignment:"), 0.0, 0.5,
                             combo, 1, FALSE);

  return vbox;
}
