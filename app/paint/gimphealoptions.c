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

#include <glib-object.h>

#include "libgimpconfig/gimpconfig.h"

#include "paint-types.h"
#include "gimphealoptions.h"

static void   gimp_heal_options_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static void   gimp_heal_options_get_property (GObject      *object,
                                              guint         property_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);

G_DEFINE_TYPE (GimpHealOptions, gimp_heal_options, GIMP_TYPE_PAINT_OPTIONS)

static void
gimp_heal_options_class_init (GimpHealOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_heal_options_set_property;
  object_class->get_property = gimp_heal_options_get_property;
}

static void
gimp_heal_options_init (GimpHealOptions *options)
{
}

static void
gimp_heal_options_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
}

static void
gimp_heal_options_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
}
