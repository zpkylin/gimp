#ifndef __GIMPSIGNAL_H__
#define __GIMPSIGNAL_H__

#include <gtk/gtksignal.h>

/* The names are encoded by the arguments */

typedef void (*GimpSignalPointer)(GtkObject*	object,
				  gpointer	arg1,
				  gpointer	user_data);

void gimp_marshaller_pointer (GtkObject*	object,
			      GtkSignalFunc	func,
			      gpointer		func_data,
			      GtkArg*		args);

typedef void (*GimpSignalInt)(GtkObject*	object,
			      gint		arg1,
			      gpointer		user_data);

void gimp_marshaller_int (GtkObject*	object,
			  GtkSignalFunc	func,
			  gpointer		func_data,
			  GtkArg*		args);

#endif
