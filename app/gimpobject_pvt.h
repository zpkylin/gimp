#ifndef __GIMP_OBJECT_PVT_H__
#define __GIMP_OBJECT_PVT_H__
#include <gtk/gtkobject.h>
#include "gimpobject.h"

struct _GimpObject
{
	GtkObject object;
};

struct _GimpObjectClass
{
	GtkObjectClass parent_class;
};

typedef struct _GimpObjectClass GimpObjectClass;

#endif
