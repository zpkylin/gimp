#ifndef __GIMPSET_PVT_H__
#define __GIMPSET_PVT_H__

#include "gimpset.h"
#include "gimpobject_pvt.h"

struct _GimpSet{
	GimpObject gobject;
	GHashTable* hash;
};

struct _GimpSetClass{
	GimpObjectClass parent_class;
};

typedef struct _GimpSetClass GimpSetClass;

#endif
