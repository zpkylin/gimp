#include "gimpsignal.h"

void gimp_marshaller_pointer (GtkObject*	object,
			      GtkSignalFunc	func,
			      gpointer		func_data,
			      GtkArg*		args)
{
  (*(GimpSignalPointer)func) (object,
			      GTK_VALUE_POINTER (args[0]),
			      func_data);
}


void gimp_marshaller_int (GtkObject*	object,
			      GtkSignalFunc	func,
			      gpointer		func_data,
			      GtkArg*		args)
{
  (*(GimpSignalInt)func) (object,
			  GTK_VALUE_INT (args[0]),
			  func_data);
}
