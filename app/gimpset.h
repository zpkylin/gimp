#ifndef __GIMPSET_H__
#define __GIMPSET_H__

#include "gimpset_decl.h"
#include "gimpimage_decl.h"



#define GIMP_SET(obj) GTK_CHECK_CAST (obj, gimp_set_get_type (), GimpSet)


#define GIMP_SET_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gimp_set_get_type(), GimpSetClass)
     
     
#define GIMP_IS_SET(obj) GTK_CHECK_TYPE (obj, gimp_set_get_type())
     
/* Signals:
   add
   remove
*/

guint gimp_set_get_type (void);

GimpSet*	gimp_set_new	(void);
void		gimp_set_add	(GimpSet* gimpset, gint id, gpointer val);
void		gimp_set_remove	(GimpSet* gimpset, gint id);
gpointer	gimp_set_lookup	(GimpSet* gimpset, gint id);
void		gimp_set_foreach(GimpSet* gimpset, GHFunc func,
				 gpointer user_data);

#endif
