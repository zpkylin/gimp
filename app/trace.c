
#include <stdio.h>
#include <stdarg.h>
#include "float16.h"
#include "pixelrow.h"
#include "pixelarea.h"
#include "tag.h"
#include "trace.h"

/* temporary logging facilities */


static char indent[] = "                                                 ";
static int level = 48;

void 
trace_enter  (
              char * func
              )
{
  printf ("%s%s () {\n", indent+level, func);
  level -= 2;
}


void 
trace_exit  (
             void
             )
{
  level += 2;
  printf ("%s}\n", indent+level);
}


void 
trace_begin  (
              char * format,
              ...
              )
{
  va_list args;
  char *buf;
  extern char * g_vsprintf ();
    
  va_start (args, format);
  buf = g_strdup_vprintf (format, &args);
  va_end (args);

  fputs (indent+level, stdout);
  fputs (buf, stdout);
  fputs (": {\n", stdout);

  level -= 2;
}


void 
trace_end  (
            void
            )
{
  level += 2;
  printf ("%s}\n", indent+level);
}


void 
trace_printf  (
               char * format,
               ...
               )
{
  va_list args;
  char *buf;
  extern char * g_vsprintf ();

  va_start (args, format);
  buf = g_strdup_vprintf (format, &args);
  va_end (args);

  fputs (indent+level, stdout);
  fputs (buf, stdout);
  fputc ('\n', stdout);
}


void
trace_timer_init (
                  TraceTimer * t
                  )
{
  gettimeofday (&t->a, NULL);  
}
 
void
trace_timer_tick (
                  TraceTimer * t,
                  char * pre,
                  char * post
                  )
{
  gettimeofday (&t->b, NULL);
  t->b.tv_usec += (1000000 * (t->b.tv_sec - t->a.tv_sec));
  printf ("%s %d %s", pre, (int) (t->b.tv_usec - t->a.tv_usec), post);
  gettimeofday (&t->a, NULL);  
}

void 
trace_time_before ( TraceTimer *t )
{
  gettimeofday (&t->a, NULL);  
}

void 
trace_time_after ( TraceTimer * t)
{
  gettimeofday (&t->b, NULL);
}

void 
print_elapsed (char *s, TraceTimer *t ) 
{
  unsigned int elapsed;
  if (t->b.tv_usec < t->a.tv_usec)
  {
    t->b.tv_usec += 1000000;
    t->b.tv_sec--;
  }
  elapsed = 1000000*(t->b.tv_sec - t->a.tv_sec) + (t->b.tv_usec - t->a.tv_usec); 
  printf ("in %s: %d\n", s, (int) (t->b.tv_usec - t->a.tv_usec) );
}

void 
kick_gdisp  (
             char * a,
             char * b
             )
{
  static TraceTimer t;
  static int init = 1;

  if (init)
    {
      trace_timer_init (&t);
      init = 0;
    }

  trace_timer_tick (&t, a, b);
}
  
void
pixelrow_print (
		 PixelRow *row
		) 
{
  ShortsFloat u;
  Tag t = pixelrow_tag (row);
  int i, k;
  int width = pixelrow_width (row);
  int chans = tag_num_channels (t);
  guchar * data = pixelrow_data (row);
 
  printf ("row: " ); 
  switch (tag_precision (t))
  {
    case PRECISION_U8:
	printf("prec u8\n");
	{
	  guint8 * d = (guint8 *)data; 
	  for (i = 0 ; i < width; i++)
		{
		  for(k = 0; k < chans; k++)
		    printf ("%d ",d[chans*i + k]);
	  	  printf(" ");
		}
	}
	  break;
    case PRECISION_U16:
	{
	  guint16 * d = (guint16 *)data; 
	  for (i = 0 ; i < width; i++)
		{
		  for(k = 0; k < chans; k++)
		    printf ("%d ",d[chans*i + k]);
	  	  printf(" ");
		}
	}
	  break;
    case PRECISION_FLOAT:
	{
	  gfloat * d = (gfloat *)data; 
	  for (i = 0 ; i < width; i++)
		{
		  for(k = 0; k < chans; k++)
		    printf ("%f ",d[chans*i + k]);
	  	  printf(" ");
		}
	}
	  break;

    case PRECISION_FLOAT16:
	{
	  guint16 * d = (guint16 *)data; 
	  for (i = 0 ; i < width; i++)
		{
		  for(k = 0; k < chans; k++)
		    printf ("%f ", FLT (d[chans*i + k], u));
	  	  printf(" ");
		}
	}
	  break;
    default:
	  break;
  } 
  printf ("\n");
}

void 
print_area_init(
                 PixelArea * pa,
                 Canvas * c,
                 int x,
                 int y,
                 int w,
                 int h,
                 int will_dirty,
		 char * name
		)
{
  pixelarea_init  ( pa,c, x, y, w, h, will_dirty);
  printf ("%s\n", name );
  print_area( pa, 0, 1);
}


void 
print_area (
			PixelArea *pa,
			int row,
			int how_many_rows
			)
{
  void * pag;
  for (pag = pixelarea_register (1, pa);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
     guchar * data = pixelarea_data (pa);
     int w = pixelarea_width (pa);
     int h = pixelarea_height (pa);
     int rowstride = pixelarea_rowstride (pa);
     Tag tag = pixelarea_tag (pa);
     int num_channels  = tag_num_channels (tag);
     int b, j, i; 
     ShortsFloat u;

      if (data == NULL)
	  printf ("warning: pixelarea_print data null\n");

      if (row == -1 )
	  row = 0;
      if (how_many_rows == -1)
	  how_many_rows = h; 

      data += rowstride * row;
      printf("pixelarea %x  w %d h %d data %x:\n", (int) pa, w, h, (int) data);
      switch (tag_precision (pixelarea_tag (pa)))
	{
	case PRECISION_U8:
	  {
	     guint8* d;
	     for (j = row; j < row + how_many_rows; j++) 
	     {
		printf (" %d : ", j);	
		d = (guint8*)data;
		for (i = 0;  i < w; i++)
		{
		  for(b= 0; b < num_channels; b++)
		    printf(" %d", *d++);
		  printf(" ");
		}
		data += rowstride;
		printf("\n");
	      }
	  }
	  break;
	  
	case PRECISION_U16:
	  {
	     guint16* d;
	     for (j = row; j < row + how_many_rows; j++) 
	     {
		printf (" %d : ", j);	
		d = (guint16*)data;
		for (i = 0;  i < w; i++)
		{
		  for(b= 0; b < num_channels; b++)
		    printf(" %d", *d++);
		  printf(" ");
		}
		data += rowstride;
		printf("\n");
	      }
	  }
	  break;

	case PRECISION_FLOAT:
	  {
	     gfloat* d;
	     for (j = row; j < row + how_many_rows; j++) 
	     {
		d = (gfloat*)data;
		for (i = 0;  i < w; i++)
		{
		  for(b= 0; b < num_channels; b++)
		    printf(" %f", *d++);
		  printf(" ");
		}
		data += rowstride;
		printf("\n");
	      }
	  }
	  break;
	
	case PRECISION_FLOAT16:
	  {
	     guint16* d;
	     for (j = row; j < row + how_many_rows; j++) 
	     {
		d = (guint16*)data;
		for (i = 0;  i < w; i++)
		{
		  for(b= 0; b < num_channels; b++)
		    printf(" %f", FLT (*d++, u));
		  printf(" ");
		}
		data += rowstride;
		printf("\n");
	      }
	  }
	  break;

	case PRECISION_NONE:
	  g_warning ("print_area: bad precision");
	  break;
	}
      }
}

void
tag_print (Tag t)
{
   Alpha a = tag_alpha (t);
   Precision p = tag_precision (t);
   Format f = tag_format (t);
   printf ("tag is %s %s %s\n", 
	 	tag_string_precision (p),
		tag_string_format(f),
		tag_string_alpha(a));
}
