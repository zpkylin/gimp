
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "libgimpbase/gimpbase.h"
#include "core/core-enums.h"
#include "gimp-gegl-enums.h"
#include "gimp-intl.h"

/* enumerations from "./gimp-gegl-enums.h" */
GType
gimp_cage_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CAGE_MODE_CAGE_CHANGE, "GIMP_CAGE_MODE_CAGE_CHANGE", "cage-change" },
    { GIMP_CAGE_MODE_DEFORM, "GIMP_CAGE_MODE_DEFORM", "deform" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_CAGE_MODE_CAGE_CHANGE, NC_("cage-mode", "Create or adjust the cage"), NULL },
    { GIMP_CAGE_MODE_DEFORM, NC_("cage-mode", "Deform the cage\nto deform the image"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpCageMode", values);
      gimp_type_set_translation_context (type, "cage-mode");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_warp_behavior_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_WARP_BEHAVIOR_MOVE, "GIMP_WARP_BEHAVIOR_MOVE", "move" },
    { GIMP_WARP_BEHAVIOR_GROW, "GIMP_WARP_BEHAVIOR_GROW", "grow" },
    { GIMP_WARP_BEHAVIOR_SHRINK, "GIMP_WARP_BEHAVIOR_SHRINK", "shrink" },
    { GIMP_WARP_BEHAVIOR_SWIRL_CW, "GIMP_WARP_BEHAVIOR_SWIRL_CW", "swirl-cw" },
    { GIMP_WARP_BEHAVIOR_SWIRL_CCW, "GIMP_WARP_BEHAVIOR_SWIRL_CCW", "swirl-ccw" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_WARP_BEHAVIOR_MOVE, NC_("warp-behavior", "Move pixels"), NULL },
    { GIMP_WARP_BEHAVIOR_GROW, NC_("warp-behavior", "Grow area"), NULL },
    { GIMP_WARP_BEHAVIOR_SHRINK, NC_("warp-behavior", "Shrink area"), NULL },
    { GIMP_WARP_BEHAVIOR_SWIRL_CW, NC_("warp-behavior", "Swirl clockwise"), NULL },
    { GIMP_WARP_BEHAVIOR_SWIRL_CCW, NC_("warp-behavior", "Swirl counter-clockwise"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpWarpBehavior", values);
      gimp_type_set_translation_context (type, "warp-behavior");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

