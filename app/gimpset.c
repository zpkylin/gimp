#include <gtk/gtkobject.h>
#include <gtk/gtksignal.h>

#include "gimpset_pvt.h"
#include "gimpsignal.h"

enum
{
  ADD,
  REMOVE,
  LAST_SIGNAL
};

static guint gimp_set_signals [LAST_SIGNAL];
static GimpObjectClass* parent_class;



static guint
gimp_set_hash_func (gpointer p)
{
  return *(guint*)&p;
}

static void
gimp_set_destroy (GtkObject* set)
{
  g_hash_table_destroy (GIMP_SET(set)->hash);
  GTK_OBJECT_CLASS(parent_class)->destroy (set);
}

static void
gimp_set_init (GimpSet* set)
{
  set->hash = g_hash_table_new (gimp_set_hash_func, NULL);
}

static void
gimp_set_class_init (GimpSetClass* klass)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);

  object_class->destroy = gimp_set_destroy;
  parent_class = gtk_type_class (gimp_object_get_type ());
  
  gimp_set_signals[ADD]=
    gtk_signal_new ("add",
		    GTK_RUN_FIRST,
		    object_class->type,
		    0,
		    gimp_marshaller_pointer,
		    GTK_TYPE_NONE, 1,
		    GTK_TYPE_OBJECT);
  gimp_set_signals[REMOVE]=
    gtk_signal_new ("remove",
		    GTK_RUN_FIRST,
		    object_class->type,
		    0,
		    gimp_marshaller_pointer,
		    GTK_TYPE_NONE, 1,
		    GTK_TYPE_OBJECT);
  gtk_object_class_add_signals (object_class, gimp_set_signals, LAST_SIGNAL);
}


DEF_GET_TYPE(GimpSet, gimp_set, gimp_object);


GimpSet*
gimp_set_new (void)
{
  return GIMP_SET (gtk_type_new (gimp_set_get_type ()));
}

void
gimp_set_add (GimpSet* set, gint id, gpointer val)
{
  /* UGLY cast! */
  g_hash_table_insert (set->hash, *((gpointer*)&id), val);
  gtk_signal_emit (GTK_OBJECT(set), gimp_set_signals[ADD], val);
}

void 
gimp_set_remove (GimpSet* set, gint id)
{
  gpointer p = gimp_set_lookup (set, id);

  g_hash_table_remove (set->hash, *((gpointer*)&id));
  gtk_signal_emit (GTK_OBJECT(set),
		   gimp_set_signals[REMOVE],
		   p);
}

gpointer
gimp_set_lookup (GimpSet* set, gint id)
{
  return g_hash_table_lookup (set->hash, *((gpointer*)&id));
}

void
gimp_set_foreach(GimpSet* gimpset, GHFunc func, gpointer user_data)
{
  g_hash_table_foreach(gimpset->hash, func, user_data);
}

