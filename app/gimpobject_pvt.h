#ifndef __GIMP_OBJECT_PVT_H__
#define __GIMP_OBJECT_PVT_H__
#include "gimpobject.h"
#include <gtk/gtkobject.h>

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
