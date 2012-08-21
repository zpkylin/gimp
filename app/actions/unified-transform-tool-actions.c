/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimptexteditor.h"
#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "tools/gimptool.h"
#include "tools/gimpunifiedtransformtool.h"

#include "text-tool-actions.h"
#include "text-tool-commands.h"

#include "unified-transform-tool-actions.h"
#include "unified-transform-tool-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry unified_transform_tool_actions[] =
{
  { "unified-transform-tool-popup", NULL,
    NC_("unified-transform-tool-action", "Unified Transform Tool Menu"), NULL, NULL, NULL,
    NULL },

  { "unified-transform-tool-flip-horiz", GTK_STOCK_CUT,
    NC_("unified-transform-tool-action", "Flip Horizontally"), NULL, NULL,
    G_CALLBACK (unified_transform_tool_flip_horiz_callback),
    NULL },

  { "unified-transform-tool-summon-pivot", GTK_STOCK_COPY,
    NC_("unified-transform-tool-action", "Put pivot here"), NULL, NULL,
    G_CALLBACK (unified_transform_tool_summon_pivot_callback),
    NULL },
};

#define SET_HIDE_EMPTY(action,condition) \
        gimp_action_group_set_action_hide_empty (group, action, (condition) != 0)

void
unified_transform_tool_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "unified-transform-tool-action",
                                 unified_transform_tool_actions,
                                 G_N_ELEMENTS (unified_transform_tool_actions));
}

void
unified_transform_tool_actions_update (GimpActionGroup *group,
                                       gpointer         data)
{
  GimpTransformTool *tr_tool  = GIMP_TRANSFORM_TOOL (data);
  GimpDisplay       *display  = GIMP_TOOL (tr_tool)->display;

#define SET_VISIBLE(action,condition) \
        gimp_action_group_set_action_visible (group, action, (condition) != 0)
#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)
#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)

/*  SET_SENSITIVE ("text-tool-cut",             text_sel);
  SET_SENSITIVE ("text-tool-copy",            text_sel);
  SET_SENSITIVE ("text-tool-paste",           clip);
  SET_SENSITIVE ("text-tool-delete",          text_sel);
  SET_SENSITIVE ("text-tool-clear",           text_layer);
  SET_SENSITIVE ("text-tool-load",            image);
  SET_SENSITIVE ("text-tool-text-to-path",    text_layer);
  SET_SENSITIVE ("text-tool-text-along-path", text_layer && vectors);

  SET_VISIBLE ("text-tool-input-methods-menu", input_method_menu);
  */
}
