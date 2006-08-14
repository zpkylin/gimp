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

#include "core/gimpchannel.h"      
#include "core/gimpimage.h"
#include "core/gimppickable.h"
#include "core/gimptoolinfo.h"

#include "paint/gimpheal.h"
#include "paint/gimphealoptions.h"  

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewablebox.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"

#include "gimphealtool.h"
#include "gimppaintoptions-gui.h"
#include "gimptoolcontrol.h"
    
#include "gimp-intl.h"

#define TARGET_WIDTH  15
#define TARGET_HEIGHT 15

static void   gimp_heal_tool_button_press  (GimpTool        *tool,
                                            GimpCoords      *coords,
                                            guint32          time,
                                            GdkModifierType  state,
                                            GimpDisplay     *display);

static void   gimp_heal_tool_motion        (GimpTool        *tool,
                                            GimpCoords      *coords,
                                            guint32          time,
                                            GdkModifierType  state,
                                            GimpDisplay     *display);

static void   gimp_heal_tool_cursor_update (GimpTool        *tool,
                                            GimpCoords      *coords,
                                            GdkModifierType  state,
                                            GimpDisplay     *display);

static void   gimp_heal_tool_oper_update   (GimpTool        *tool,
                                            GimpCoords      *coords,
                                            GdkModifierType  state,
                                            gboolean         proximity,
                                            GimpDisplay     *display);

static void   gimp_heal_tool_draw          (GimpDrawTool *draw_tool);

static GtkWidget * gimp_heal_options_gui   (GimpToolOptions *tool_options);

G_DEFINE_TYPE (GimpHealTool, gimp_heal_tool, GIMP_TYPE_PAINT_TOOL)

void
gimp_heal_tool_register (GimpToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (GIMP_TYPE_HEAL_TOOL,               /* tool type */
                GIMP_TYPE_HEAL_OPTIONS,            /* tool option type */
                gimp_heal_options_gui,             /* options gui */
                GIMP_PAINT_OPTIONS_CONTEXT_MASK |  /* context properties */
                GIMP_CONTEXT_PATTERN_MASK,
                "gimp-heal-tool",                  /* identifier */
                _("Heal"),                         /* blurb */
                _("Heal image irregularities"),    /* help */
                N_("_Heal"),                       /* menu path */
                "H",                               /* menu accelerator */
                NULL,                              /* help domain */
                GIMP_HELP_TOOL_HEAL,               /* help data */
                GIMP_STOCK_TOOL_HEAL,              /* stock id */
                data);                             /* register */
}

static void
gimp_heal_tool_class_init (GimpHealToolClass *klass)
{
  /* get parent classes where we override methods */
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  /* override the methods */
  tool_class->button_press  = gimp_heal_tool_button_press;
  tool_class->motion        = gimp_heal_tool_motion;
  tool_class->cursor_update = gimp_heal_tool_cursor_update;
  tool_class->oper_update   = gimp_heal_tool_oper_update;

  draw_tool_class->draw     = gimp_heal_tool_draw;
}

static void
gimp_heal_tool_init (GimpHealTool *heal)
{
  GimpTool *tool = GIMP_TOOL (heal);

  gimp_tool_control_set_tool_cursor     (tool->control,
                                         GIMP_TOOL_CURSOR_HEAL);

  gimp_tool_control_set_action_object_2 (tool->control,
                                         "context/context-pattern-select-set");
}

static void
gimp_heal_tool_button_press (GimpTool        *tool,
                             GimpCoords      *coords,
                             guint32          time,
                             GdkModifierType  state,
                             GimpDisplay     *display)
{
  GimpPaintTool   *paint_tool   = GIMP_PAINT_TOOL (tool);
  GimpHealTool    *heal_tool    = GIMP_HEAL_TOOL (tool);
  GimpHeal        *heal         = GIMP_HEAL (paint_tool->core);

  /* pause the tool before drawing */
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /* use_saved_proj tells whether we keep the unmodified pixel projection
   * around.  in this case no.  */
  paint_tool->core->use_saved_proj = FALSE;

  /* state holds a set of bit-flags to indicate the state of modifier keys and
   * mouse buttons in various event types. Typical modifier keys are Shift,
   * Control, Meta, Super, Hyper, Alt, Compose, Apple, CapsLock or ShiftLock. 
   * Part of gtk -> GdkModifierType */
  if ((state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == GDK_CONTROL_MASK) 
    /* we enter here only if CTRL is pressed */
    {
      /* mark that the source display has been set.  defined in gimpheal.h */
      heal->set_source = TRUE;

      /* if currently active display != the heal tools source display */
      if (display != heal_tool->src_display)
        {
          /* check if the heal tools src display has been previously set */
          if (heal_tool->src_display)
            /* if so remove the pointer to the display */
            g_object_remove_weak_pointer (G_OBJECT (heal_tool->src_display),
                                          (gpointer *) &heal_tool->src_display);

          /* set the heal tools source display to the currently active
           * display */
          heal_tool->src_display = display;
          /* add a pointer to the display */
          g_object_add_weak_pointer (G_OBJECT (display),
                                     (gpointer *) &heal_tool->src_display);
        }
    }
  else
    {
      /* note that the source is not being set */
      heal->set_source = FALSE;

      if (display == heal_tool->src_display)
        {
          paint_tool->core->use_saved_proj = TRUE;
        }
    }

  /* chain up to call the parents functions */
  GIMP_TOOL_CLASS (gimp_heal_tool_parent_class)->button_press (tool, coords, time, state,
                                                display);

  /* set the tool display's source position to match the current heal 
   * implementation source position */
  heal_tool->src_x = heal->src_x;
  heal_tool->src_y = heal->src_y;

  /* resume drawing */
  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_heal_tool_motion (GimpTool        *tool,
                       GimpCoords      *coords,
                       guint32          time,
                       GdkModifierType  state,
                       GimpDisplay     *display)
{
  GimpHealTool    *heal_tool    = GIMP_HEAL_TOOL (tool);
  GimpPaintTool   *paint_tool   = GIMP_PAINT_TOOL (tool);
  GimpHeal        *heal         = GIMP_HEAL (paint_tool->core);

  /* pause drawing */
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /* check if CTRL is pressed */
  if ((state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == GDK_CONTROL_MASK)
    /* if yes the source has been set */
    heal->set_source = TRUE;
  else
    /* if no the source has not been set */
    heal->set_source = FALSE;

  /* chain up to the parent classes motion function */
  GIMP_TOOL_CLASS (gimp_heal_tool_parent_class)->motion (tool, coords, time, state, display);

  /* set the tool display's source to be the same as the heal implementation
   * source */
  heal_tool->src_x = heal->src_x;
  heal_tool->src_y = heal->src_y;

  /* resume drawing */
  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_heal_tool_cursor_update (GimpTool        *tool,
                              GimpCoords      *coords,
                              GdkModifierType  state,
                              GimpDisplay     *display)
{
  GimpCursorType        cursor   = GIMP_CURSOR_MOUSE;
  GimpCursorModifier    modifier = GIMP_CURSOR_MODIFIER_NONE;

  /* if the cursor is in an active area */
  if (gimp_image_coords_in_active_pickable (display->image, coords,
                                            FALSE, TRUE))
    {
      /* set the cursor to the normal cursor */
      cursor = GIMP_CURSOR_MOUSE;
    }

  if ((state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == GDK_CONTROL_MASK)
    {
      cursor = GIMP_CURSOR_CROSSHAIR_SMALL;
    }
  else if (! GIMP_HEAL (GIMP_PAINT_TOOL (tool)->core)->src_drawable)
    {
      modifier = GIMP_CURSOR_MODIFIER_BAD;
    }

  /* set the cursor and the modifier cursor */
  gimp_tool_control_set_cursor          (tool->control, cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  /* chain up to the parent class */
  GIMP_TOOL_CLASS (gimp_heal_tool_parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_heal_tool_oper_update (GimpTool        *tool,
                            GimpCoords      *coords,
                            GdkModifierType  state,
                            gboolean         proximity,
                            GimpDisplay     *display)
{
  GimpHealTool    *heal_tool = GIMP_HEAL_TOOL (tool);

  /* chain up to the parent class */
  GIMP_TOOL_CLASS (gimp_heal_tool_parent_class)->oper_update (tool, coords, state, proximity,
                                               display);

  /* FIXME: what does proximity mean? */
  if (proximity)
    {
      GimpHeal *heal = GIMP_HEAL (GIMP_PAINT_TOOL (tool)->core);

      if (heal->src_drawable == NULL)
        {
          /* if we haven't set the source drawable yet, make a notice to do so */
          gimp_tool_replace_status (tool, display,
                                    _("Ctrl-Click to set a heal source."));
        }
      else
        {
          /* pause drawing */
          gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

          /* set the tool source to match the implementation source */
          heal_tool->src_x = heal->src_x;
          heal_tool->src_y = heal->src_y;

          if (! heal->first_stroke)
            {
              heal_tool->src_x = coords->x + heal->offset_x;
              heal->src_y = coords->y + heal->offset_y;
            }

          /* resume drawing */
          gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
        }
    }
}

static void
gimp_heal_tool_draw (GimpDrawTool *draw_tool)
{
  GimpTool        *tool      = GIMP_TOOL (draw_tool);
  GimpHealTool    *heal_tool = GIMP_HEAL_TOOL (draw_tool);
  GimpHeal        *heal      = GIMP_HEAL (GIMP_PAINT_TOOL (tool)->core);

  /* If we have a source drawable and display we can do the drawing */
  if (heal->src_drawable && heal_tool->src_display)
    {
      /* make a temporary display and keep track of offsets */
      GimpDisplay *tmp_display;
      gint         off_x;
      gint         off_y;

      /* gimp_item_offsets reveals the X and Y offsets of the first parameter.
       * this gets the location of the drawable. */
      gimp_item_offsets (GIMP_ITEM (heal->src_drawable), &off_x, &off_y);

      /* store the display for later */
      tmp_display = draw_tool->display;

      /* set the display */
      draw_tool->display = heal_tool->src_display;

      /* convenience function to simplify drawing */
      gimp_draw_tool_draw_handle (draw_tool,
                                  GIMP_HANDLE_CROSS,
                                  heal_tool->src_x + off_x,
                                  heal_tool->src_y + off_y,
                                  TARGET_WIDTH, TARGET_HEIGHT,
                                  GTK_ANCHOR_CENTER,
                                  FALSE);

      /* FIXME: why do we reset the display after calling the
       * gimp_draw_tool_draw_handle function? */
      draw_tool->display = tmp_display;
    }

  /* chain up to the parent draw function */
  GIMP_DRAW_TOOL_CLASS (gimp_heal_tool_parent_class)->draw (draw_tool);
}

/* FIXME:  Tailor this to the healing brush */
static GtkWidget *
gimp_heal_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *combo;

  config = G_OBJECT (tool_options);

  vbox = gimp_paint_options_gui (tool_options);

  return vbox;
}
