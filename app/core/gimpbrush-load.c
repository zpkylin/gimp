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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gimpbrush.h"
#include "gimpbrushlist.h"
#include "gimpsignal.h"
#include "gimprc.h"
#include "brush_header.h"

enum{
  DIRTY,
  RENAME,
  LAST_SIGNAL
};

static guint gimp_brush_signals[LAST_SIGNAL];
static GimpObjectClass* parent_class;

static void
gimp_brush_destroy(GtkObject *object)
{
  GimpBrush* brush=GIMP_BRUSH(object);
  if (brush->filename)
    g_free(brush->filename);
  if (brush->name)
    g_free(brush->name);
  if (brush->mask)
     canvas_delete(brush->mask);
  GTK_OBJECT_CLASS(parent_class)->destroy (object);

}

static void
gimp_brush_class_init (GimpBrushClass *klass)
{
  GtkObjectClass *object_class;
  GtkType type;
  
  object_class = GTK_OBJECT_CLASS(klass);

  parent_class = gtk_type_class (gimp_object_get_type ());
  
  type=object_class->type;

  object_class->destroy =  gimp_brush_destroy;

  gimp_brush_signals[DIRTY] =
    gimp_signal_new ("dirty", 0, type, 0, gimp_sigtype_void);

  gimp_brush_signals[RENAME] =
    gimp_signal_new ("rename", 0, type, 0, gimp_sigtype_void);

  gtk_object_class_add_signals (object_class, gimp_brush_signals, LAST_SIGNAL);
}

void
gimp_brush_init(GimpBrush *brush)
{
  brush->filename  = NULL;
  brush->name      = NULL;
  brush->spacing   = 20;
  brush->mask      = NULL;
  brush->x_axis.x    = 15.0;
  brush->x_axis.y    =  0.0;
  brush->y_axis.x    =  0.0;
  brush->y_axis.y    = 15.0;
}

GtkType gimp_brush_get_type(void)
{
  static GtkType type;
  if(!type){
    GtkTypeInfo info={
      "GimpBrush",
      sizeof(GimpBrush),
      sizeof(GimpBrushClass),
      (GtkClassInitFunc)gimp_brush_class_init,
      (GtkObjectInitFunc)gimp_brush_init,
      NULL,
      NULL };
    type=gtk_type_unique(gimp_object_get_type(), &info);
  }
  return type;
}

Canvas *
gimp_brush_get_mask (GimpBrush *brush)
{
  g_return_val_if_fail(GIMP_IS_BRUSH(brush), NULL);
  return brush->mask;
}

char *
gimp_brush_get_name (GimpBrush *brush)
{
  g_return_val_if_fail(GIMP_IS_BRUSH(brush), NULL);
  return brush->name;
}

void
gimp_brush_set_name (GimpBrush *brush, char *name)
{
  g_return_if_fail(GIMP_IS_BRUSH(brush));
  if (strcmp(brush->name, name) == 0)
    return;
  if (brush->name)
    g_free(brush->name);
  brush->name = g_strdup(name);
  gtk_signal_emit(GTK_OBJECT(brush), gimp_brush_signals[RENAME]);
}

GimpBrush *
gimp_brush_load(char *filename)
{
  FILE * fp;
  int bn_size;
  unsigned char buf [sz_BrushHeader];
  BrushHeader header;
  unsigned int * hp;
  int i;
  guint32 header_size;
  guint32 header_version;
  Tag tag;
  gint bytes;
  GimpBrush* brush;
  Canvas *brushmask;
  char *brushname;

  /*  Open the requested file  */
  if (! (fp = fopen (filename, "rb")))
    {
      return NULL;
    }

#if 1
  /*  Read in the header size and version */
  if ((fread (&header_size, 1, sizeof (guint32), fp)) < sizeof (guint32)) 
    {
      fclose (fp);
      return NULL;
    }

  if ((fread (&header_version, 1, sizeof (guint32), fp)) < sizeof (guint32)) 
    {
      fclose (fp);
      return NULL;
    }
	
  fseek (fp, - 2 * sizeof(guint32), SEEK_CUR);
#endif

  if ((fread (&buf, 1, sz_BrushHeader, fp)) < sz_BrushHeader) 
    {
      fclose (fp);
      return NULL;
    }

  /*  rearrange the bytes in each unsigned int  */
  hp = (unsigned int *) &header;
  for (i = 0; i < (sz_BrushHeader / 4); i++)
    hp [i] = (buf [i * 4] << 24) + (buf [i * 4 + 1] << 16) +
             (buf [i * 4 + 2] << 8) + (buf [i * 4 + 3]);

  /*  Check for correct file format */
  if (header.magic_number != GBRUSH_MAGIC)
    {
      /*  One thing that can save this error is if the brush is version 1  */
      if (header.version != 1)
	{
	  fclose (fp);
	  return NULL;
	}
    }

  if (header.version == 1 || header.version == 2)
    {
      if (header.version == 1)
      {
	 /*  If this is a version 1 brush, set the fp back 8 bytes  */
	 fseek (fp, -8, SEEK_CUR);
	 header.header_size += 8;
	 /*  spacing is not defined in version 1  */
	 header.spacing = 25;
      }

      /* If its version 1 or 2 the bytes field contains just
         the number of bytes and must be converted to a drawable type. */    

      if (header.type == 1)    
 	header.type = GRAY_GIMAGE;	   
      else if (header.type == 3)
        header.type = RGB_GIMAGE;	   
    }
  else if (header.version != FILE_VERSION)
    {
      g_message ("Unknown GIMP version %d in \"%s\"\n", header.version, filename);
      fclose (fp);
      return NULL;
    }
  
    tag = tag_from_drawable_type ( header.type);

    if (tag_precision (tag) != default_precision )
       { 
	fclose (fp);
	return NULL;
       }
   
  /*  Read in the brush name  */
  if ((bn_size = (header.header_size - sz_BrushHeader)))
  {
    brushname = (char *) g_malloc (sizeof (char) * bn_size);
    if ((fread (brushname, 1, bn_size, fp)) < bn_size)
    {
      g_message ("Error in GIMP brush file...aborting.");
      fclose (fp);
      return NULL;
    }
  }
  else
    brushname = g_strdup ("Untitled");

  switch(header.version)
  {
   case 1:
   case 2:
   case FILE_VERSION:
     /*  Get a new brush mask  */
     brushmask = canvas_new (tag, 
				header.width,
                            	header.height,
                            	STORAGE_FLAT );					


  /*  Read the brush mask data  */
     bytes = tag_bytes (tag); 
     canvas_portion_refrw (brushmask, 0, 0); 
     if ((fread (canvas_portion_data (brushmask,0,0), 1, header.width * header.height * bytes, fp)) 		< header.width * header.height * bytes)
     	g_message ("GIMP brush file appears to be truncated.");
     canvas_portion_unref (brushmask, 0, 0); 
     break;
   default:
     g_message ("Unknown brush format version #%d in \"%s\"\n", header.version, filename);
     fclose (fp);
     return NULL;
  }

  fclose (fp);

  /* Okay we are successful, create the brush object */

  brush = GIMP_BRUSH(gimp_type_new(gimp_brush_get_type()));
  brush->filename = g_strdup (filename);
  brush->name = brushname; 
  brush->mask = brushmask;
  brush->spacing = header.spacing;
  brush->x_axis.x = header.width  / 2.0;
  brush->x_axis.y = 0.0;
  brush->y_axis.x = 0.0;
  brush->y_axis.y = header.height / 2.0;

  /* Check if the current brush is the default one */

/* lets see if it works with out this for now */

#if 1 
  {
    char * basename = strrchr (filename, '/' );
    if (basename) 
      {
	basename++; 
	if (strcmp(default_brush, basename) == 0) 
	  {
	    select_brush (brush);
	    set_have_default_brush (TRUE); 
	  }
      } 
  }
#endif

  return brush;
}
