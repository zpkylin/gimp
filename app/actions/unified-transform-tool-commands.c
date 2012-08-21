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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"

#include "tools/gimpunifiedtransformtool.h"

#include "unified-transform-tool-commands.h"

#include "gimp-intl.h"


/*  public functions  */

void
unified_transform_tool_flip_horiz_callback (GtkAction *action,
                                            gpointer   data)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (data);

  gimp_unified_transform_tool_flip_horiz (tr_tool);
}

void
unified_transform_tool_summon_pivot_callback (GtkAction *action,
                                              gpointer   data)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (data);

  gimp_unified_transform_tool_summon_pivot (tr_tool);
}
