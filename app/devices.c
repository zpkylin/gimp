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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "config.h"

#include <string.h>
#include <stdio.h>

#include "appenv.h"
#include "devices.h"
#include "gimpbrushlist.h"
#include "gimprc.h"
#include "gradient.h"
#include "tools.h"


/*  local functions */
static void     input_dialog_able_callback     (GtkWidget *w, guint32 deviceid, 
						gpointer data);
static void     input_dialog_disable_callback     (GtkWidget *w, guint32 deviceid, 
						gpointer data);


gint current_device = 0;

/*  the gtk input dialog  */

void 
input_dialog_create (void)
{
  static GtkWidget *inputd = NULL;

  if (!inputd)
    {
      inputd = gtk_input_dialog_new ();
     
      gtk_signal_connect (GTK_OBJECT(inputd), "destroy",
                          (GtkSignalFunc)input_dialog_destroy, &inputd);
      gtk_signal_connect_object (GTK_OBJECT(GTK_INPUT_DIALOG(inputd)->close_button),
                          "clicked",
                          (GtkSignalFunc)gtk_widget_hide,
                          GTK_OBJECT(inputd));
      gtk_widget_hide ( GTK_INPUT_DIALOG(inputd)->save_button);

      gtk_signal_connect (GTK_OBJECT(inputd), "enable_device",
                          (GtkSignalFunc)input_dialog_able_callback, NULL);
      gtk_signal_connect (GTK_OBJECT(inputd), "disable_device",
                          (GtkSignalFunc)input_dialog_disable_callback, NULL);
      gtk_widget_show (inputd);
    }
  else
    {
      if (!GTK_WIDGET_MAPPED (inputd))
	gtk_widget_show (inputd);
      else
	gdk_window_raise (inputd->window);
    }
}

static void
input_dialog_able_callback (GtkWidget *widget,
			    guint32    deviceid,
			    gpointer   data)
{
  current_device = deviceid;
}

static void
input_dialog_disable_callback (GtkWidget *widget,
			    guint32    deviceid,
			    gpointer   data)
{
  current_device = 0;
}


void
input_dialog_destroy (GtkWidget *w, gpointer data)
{
  *((GtkWidget **)data) = NULL;
}

